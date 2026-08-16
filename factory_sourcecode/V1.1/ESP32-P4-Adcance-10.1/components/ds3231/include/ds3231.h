/*
 * DS3231 / DS1307 Real-Time Clock I2C Driver for ESP32-P4
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <time.h>
#include <stdbool.h>
#include <stdint.h>

#define DS3231_I2C_ADDR 0x68

/**
 * @brief Initialize the DS3231 / DS1307 RTC on the shared I2C bus.
 * @return ESP_OK if RTC is detected, ESP_ERR_NOT_FOUND if absent, or other error.
 */
esp_err_t ds3231_init(void);

/**
 * @brief Check if the DS3231 RTC is present and responding.
 */
bool ds3231_is_present(void);

/**
 * @brief Read the current date/time from the DS3231.
 * @param[out] timeinfo Pointer to struct tm to populate
 * @return ESP_OK on success
 */
esp_err_t ds3231_get_time(struct tm *timeinfo);

/**
 * @brief Write date/time to the DS3231.
 * @param[in] timeinfo Pointer to struct tm containing the time to set
 * @return ESP_OK on success
 */
esp_err_t ds3231_set_time(const struct tm *timeinfo);

/**
 * @brief Read time from DS3231 and set the ESP32 system clock (via settimeofday).
 * @return ESP_OK on success
 */
esp_err_t ds3231_sync_to_system(void);

/**
 * @brief Read current ESP32 system time (e.g. after NTP sync) and write to DS3231.
 * @return ESP_OK on success
 */
esp_err_t ds3231_sync_from_system(void);

#ifdef __cplusplus
}
#endif
