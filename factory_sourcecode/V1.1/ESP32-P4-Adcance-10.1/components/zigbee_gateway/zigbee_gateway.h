/*
 * Zigbee Gateway Component - Public API
 *
 * Provides a Zigbee Coordinator running on the ESP32-P4 host,
 * communicating with the ESP32-H2 RCP via UART2 (GPIO 53/54).
 *
 * Hardware: Elecrow CrowPanel Advanced 10.1" V1.1
 *   P4 GPIO 53 (TXD2) -> H2 GPIO 24 (U1RXD)
 *   P4 GPIO 54 (RXD2) -> H2 GPIO 23 (U1TXD)
 *   P4 GPIO  9        -> H2 GPIO  9 (IO9 / BOOT)
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/* Maximum number of tracked Zigbee devices */
#define ZB_GW_MAX_DEVICES  32

/* Zigbee device descriptor */
typedef struct {
    uint16_t short_addr;           /* 16-bit network address */
    uint8_t  ieee_addr[8];         /* 64-bit IEEE address */
    uint8_t  endpoint;             /* Primary endpoint */
    uint16_t device_id;            /* Zigbee device ID */
    char     name[32];             /* Human-readable name (if known) */
    bool     online;               /* Last known state */
} zb_gw_device_t;

/* Callback for new device joins */
typedef void (*zb_gw_device_joined_cb_t)(const zb_gw_device_t *device);

/* Callback for device leaves */
typedef void (*zb_gw_device_left_cb_t)(uint16_t short_addr);

/**
 * @brief Initialize and start the Zigbee gateway coordinator.
 *
 * Starts the ZBOSS stack connected to the H2 RCP via UART2.
 * Must be called after NVS is initialized.
 *
 * @return ESP_OK on success
 */
esp_err_t zigbee_gateway_start(void);

/**
 * @brief Register callback for device join events.
 */
void zigbee_gateway_set_join_callback(zb_gw_device_joined_cb_t cb);

/**
 * @brief Register callback for device leave events.
 */
void zigbee_gateway_set_leave_callback(zb_gw_device_left_cb_t cb);

/**
 * @brief Open the network for new device joins.
 * @param duration_seconds How long to allow joins (max 254, 0=close).
 */
esp_err_t zigbee_gateway_open_network(uint8_t duration_seconds);

/**
 * @brief Get the list of currently known devices.
 * @param devices Output buffer (at least ZB_GW_MAX_DEVICES entries)
 * @param count   Output: number of devices found
 */
esp_err_t zigbee_gateway_get_devices(zb_gw_device_t *devices, uint8_t *count);

/**
 * @brief Get the coordinator's current channel and PAN ID.
 */
uint8_t  zigbee_gateway_get_channel(void);
uint16_t zigbee_gateway_get_pan_id(void);

/**
 * @brief Returns true if the Zigbee network is up and running.
 */
bool zigbee_gateway_is_running(void);

#ifdef __cplusplus
}
#endif
