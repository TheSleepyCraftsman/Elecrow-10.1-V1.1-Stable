/*
 * Elecrow ESP32-P4 10.1" - ESP32-H2 OpenThread/Zigbee RCP Firmware
 *
 * The H2 acts as a 802.15.4 Radio Co-Processor (RCP) using the
 * OpenThread Spinel protocol over UART1 to the P4 host.
 *
 * Hardware connections (Elecrow V1.1 schematic):
 *   H2 GPIO 23 (U1TXD) -> wireless socket -> P4 GPIO 54 (RXD2)
 *   H2 GPIO 24 (U1RXD) -> wireless socket -> P4 GPIO 53 (TXD2)
 *   H2 GPIO  9 (IO9)   -> wireless socket -> P4 GPIO  9 (BOOT)
 *
 * Based on: esp-idf/examples/openthread/ot_rcp
 */

#include <stdio.h>
#include <unistd.h>
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_openthread.h"
#include "esp_ot_config.h"
#include "esp_vfs_eventfd.h"
#include "driver/uart.h"

#if !SOC_IEEE802154_SUPPORTED
#error "RCP is only supported for SoCs with IEEE 802.15.4 (e.g. ESP32-H2)"
#endif

static const char *TAG = "H2_ZB_RCP";

extern void otAppNcpInit(otInstance *instance);

static void ot_task_worker(void *aContext)
{
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config  = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config  = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };

    ESP_ERROR_CHECK(esp_openthread_init(&config));

    /* Initialize the OpenThread NCP (Network Co-Processor) layer */
    otAppNcpInit(esp_openthread_get_instance());

    /* Run the main OpenThread loop */
    esp_openthread_launch_mainloop();

    esp_vfs_eventfd_unregister();
    vTaskDelete(NULL);
}

#include "driver/gpio.h"

void app_main(void)
{
    /* Release all external socket GPIOs on H2 to avoid contention on shared I2C/SPI lines.
     * GPIO 0 and 1 are wired to I2C1_SCL and I2C1_SDA on the socket!
     * Note: Do NOT touch GPIO 15-20 which are connected to internal SPI Flash! */
    const gpio_num_t release_pins[] = {
        GPIO_NUM_0, GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5,
        GPIO_NUM_8, GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14,
        GPIO_NUM_22, GPIO_NUM_25
    };
    for (size_t i = 0; i < sizeof(release_pins) / sizeof(release_pins[0]); i++) {
        gpio_reset_pin(release_pins[i]);
        gpio_set_direction(release_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(release_pins[i], GPIO_FLOATING);
    }

    /* eventfds used: ot task queue + radio driver */
    esp_vfs_eventfd_config_t eventfd_config = {
        .max_fds = 2,
    };

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_vfs_eventfd_register(&eventfd_config));

    xTaskCreate(ot_task_worker, "ot_rcp_main", 3072,
                xTaskGetCurrentTaskHandle(), 5, NULL);
}
