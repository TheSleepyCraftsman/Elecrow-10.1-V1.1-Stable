/*
 * Zigbee Gateway Component
 *
 * Zigbee Coordinator running on ESP32-P4, using the ESP32-H2 as
 * a Radio Co-Processor (RCP) via UART2.
 *
 * Hardware (Elecrow CrowPanel Advanced 10.1" V1.1 schematic):
 *   P4 GPIO 53 (TXD2) -> H2 GPIO 24 (U1RXD)   [UART TX to H2]
 *   P4 GPIO 54 (RXD2) -> H2 GPIO 23 (U1TXD)   [UART RX from H2]
 *   P4 GPIO  9        -> H2 GPIO  9 (IO9/BOOT) [H2 boot strapping]
 *   H2 EN             -> not wired (pulled to 3.3V)
 */

#include "zigbee_gateway.h"
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "string.h"

#if CONFIG_ZIGBEE_GATEWAY_ENABLED

#include "esp_zigbee_core.h"

static const char *TAG = "ZB_GW";

/* ------------------------------------------------------------------ */
/*  Internal state                                                       */
/* ------------------------------------------------------------------ */

static bool s_running = false;
static uint8_t  s_channel = 0;
static uint16_t s_pan_id  = 0;

static zb_gw_device_t  s_devices[ZB_GW_MAX_DEVICES];
static uint8_t         s_device_count = 0;
static SemaphoreHandle_t s_device_mutex = NULL;

static zb_gw_device_joined_cb_t s_join_cb  = NULL;
static zb_gw_device_left_cb_t   s_leave_cb = NULL;

static void start_commissioning_wrapper(uint8_t mode_mask)
{
    esp_zb_bdb_start_top_level_commissioning(mode_mask);
}

/* ------------------------------------------------------------------ */
/*  Zigbee stack signal handler                                          */
/* ------------------------------------------------------------------ */

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_message)
{
    uint32_t *p_sg_p = signal_message->p_app_signal;
    esp_err_t err_status = signal_message->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;

    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Zigbee stack initialised, forming network...");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        break;

    case ESP_ZB_BDB_SIGNAL_FORMATION:
        if (err_status == ESP_OK) {
            s_channel = esp_zb_get_current_channel();
            s_pan_id  = esp_zb_get_pan_id();
            s_running = true;
            ESP_LOGI(TAG, "Network formed: PAN ID 0x%04x, channel %d",
                     s_pan_id, s_channel);
            /* Open network for 3 minutes on first start */
            esp_zb_bdb_open_network(180);
        } else {
            ESP_LOGE(TAG, "Network formation failed (0x%02x), retrying...", err_status);
            esp_zb_scheduler_alarm(
                start_commissioning_wrapper,
                ESP_ZB_BDB_MODE_NETWORK_FORMATION, 1000);
        }
        break;

    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Network steering started — ready for device joins");
        }
        break;

    case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS:
        if (err_status == ESP_OK) {
            uint8_t open = *(uint8_t *)esp_zb_app_signal_get_params(p_sg_p);
            ESP_LOGI(TAG, "Network %s", open ? "OPEN for joins" : "CLOSED");
        }
        break;

    case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE: {
        esp_zb_zdo_signal_device_annce_params_t *dev =
            (esp_zb_zdo_signal_device_annce_params_t *)
            esp_zb_app_signal_get_params(p_sg_p);

        ESP_LOGI(TAG, "Device joined: short=0x%04x", dev->device_short_addr);

        /* Register the device */
        if (xSemaphoreTake(s_device_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (s_device_count < ZB_GW_MAX_DEVICES) {
                zb_gw_device_t *d = &s_devices[s_device_count++];
                memset(d, 0, sizeof(*d));
                d->short_addr = dev->device_short_addr;
                d->online = true;
                snprintf(d->name, sizeof(d->name), "Device 0x%04x",
                         dev->device_short_addr);
                if (s_join_cb) s_join_cb(d);
            }
            xSemaphoreGive(s_device_mutex);
        }
        break;
    }

    default:
        ESP_LOGD(TAG, "Signal: %s (0x%x), status: %s",
                 esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Zigbee task                                                          */
/* ------------------------------------------------------------------ */

static void zigbee_task(void *pvParameters)
{
    /* Coordinator configuration */
    esp_zb_cfg_t zb_cfg = {
        .esp_zb_role          = ESP_ZB_DEVICE_TYPE_COORDINATOR,
        .install_code_policy  = false,
        .nwk_cfg.zczr_cfg = {
            .max_children = 32,
        },
    };

    /* Platform config: radio via UART2 to H2 RCP */
    esp_zb_platform_config_t platform_cfg = {
        .radio_config = {
            .radio_mode = ZB_RADIO_MODE_UART_RCP,
            .radio_uart_config = {
                .port   = UART_NUM_2,
                .rx_pin = 54,           /* P4 RXD2 <- H2 GPIO 23 (U1TXD) */
                .tx_pin = 53,           /* P4 TXD2 -> H2 GPIO 24 (U1RXD) */
                .uart_config = {
                    .baud_rate  = 460800,
                    .data_bits  = UART_DATA_8_BITS,
                    .parity     = UART_PARITY_DISABLE,
                    .stop_bits  = UART_STOP_BITS_1,
                    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
                    .source_clk = UART_SCLK_DEFAULT,
                },
            },
        },
    };

    ESP_ERROR_CHECK(esp_zb_platform_config(&platform_cfg));
    esp_zb_init(&zb_cfg);

    /* Use all channels (11-26), let stack pick the best */
    esp_zb_set_channel_mask(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK);

    ESP_LOGI(TAG, "Starting Zigbee coordinator on UART2 TX=%d RX=%d", 53, 54);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();

    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                           */
/* ------------------------------------------------------------------ */

esp_err_t zigbee_gateway_start(void)
{
    s_device_mutex = xSemaphoreCreateMutex();
    if (!s_device_mutex) return ESP_ERR_NO_MEM;

    memset(s_devices, 0, sizeof(s_devices));
    s_device_count = 0;

    BaseType_t ret = xTaskCreate(zigbee_task, "zigbee_gw", 4096, NULL,
                                  configMAX_PRIORITIES - 5, NULL);
    if (ret != pdPASS) return ESP_FAIL;

    ESP_LOGI(TAG, "Zigbee gateway component started");
    return ESP_OK;
}

void zigbee_gateway_set_join_callback(zb_gw_device_joined_cb_t cb)
{
    s_join_cb = cb;
}

void zigbee_gateway_set_leave_callback(zb_gw_device_left_cb_t cb)
{
    s_leave_cb = cb;
}

esp_err_t zigbee_gateway_open_network(uint8_t duration_seconds)
{
    if (!s_running) return ESP_ERR_INVALID_STATE;
    esp_zb_bdb_open_network(duration_seconds);
    return ESP_OK;
}

esp_err_t zigbee_gateway_get_devices(zb_gw_device_t *devices, uint8_t *count)
{
    if (!devices || !count) return ESP_ERR_INVALID_ARG;
    if (xSemaphoreTake(s_device_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return ESP_ERR_TIMEOUT;
    memcpy(devices, s_devices, s_device_count * sizeof(zb_gw_device_t));
    *count = s_device_count;
    xSemaphoreGive(s_device_mutex);
    return ESP_OK;
}

uint8_t zigbee_gateway_get_channel(void)   { return s_channel; }
uint16_t zigbee_gateway_get_pan_id(void)   { return s_pan_id;  }
bool zigbee_gateway_is_running(void)       { return s_running;  }

#else /* CONFIG_ZIGBEE_GATEWAY_ENABLED not set */

/* Stub implementations so the rest of the project compiles without Zigbee */
esp_err_t zigbee_gateway_start(void)                                { return ESP_ERR_NOT_SUPPORTED; }
void zigbee_gateway_set_join_callback(zb_gw_device_joined_cb_t cb)  { }
void zigbee_gateway_set_leave_callback(zb_gw_device_left_cb_t cb)   { }
esp_err_t zigbee_gateway_open_network(uint8_t d)                    { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t zigbee_gateway_get_devices(zb_gw_device_t *d, uint8_t *c){ return ESP_ERR_NOT_SUPPORTED; }
uint8_t  zigbee_gateway_get_channel(void)                           { return 0; }
uint16_t zigbee_gateway_get_pan_id(void)                            { return 0; }
bool zigbee_gateway_is_running(void)                                 { return false; }

#endif /* CONFIG_ZIGBEE_GATEWAY_ENABLED */
