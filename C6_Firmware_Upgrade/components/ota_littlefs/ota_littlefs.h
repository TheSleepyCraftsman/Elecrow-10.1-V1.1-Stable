/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Perform LittleFS OTA update
 *
 * Reads firmware binary from LittleFS "storage" partition, validates the
 * image header, and transfers it to the slave co-processor via esp_hosted OTA.
 *
 * @param delete_after_use Whether to delete firmware file after flash succeeds
 * @return ESP_HOSTED_SLAVE_OTA_COMPLETED  — transfer succeeded
 * @return ESP_HOSTED_SLAVE_OTA_NOT_REQUIRED — slave already runs this version (version check enabled)
 * @return ESP_HOSTED_SLAVE_OTA_FAILED — transfer or validation failed
 * @return ESP_ERR_NO_MEM — allocation failure
 */
esp_err_t ota_littlefs_perform(bool delete_after_use);

#ifdef __cplusplus
}
#endif
