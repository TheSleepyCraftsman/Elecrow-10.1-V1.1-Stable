#include "ds3231.h"
#include "bsp/esp32_p4_function_ev_board.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <sys/time.h>
#include <string.h>

static const char *TAG = "ds3231";

static i2c_master_dev_handle_t s_rtc_dev_handle = NULL;
static bool s_rtc_present = false;

/* Convert binary decimal (BCD) to integer */
static inline uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0F);
}

/* Convert integer to binary decimal (BCD) */
static inline uint8_t dec2bcd(uint8_t val)
{
    return ((val / 10) << 4) + (val % 10);
}

esp_err_t ds3231_init(void)
{
    if (s_rtc_dev_handle != NULL) {
        return ESP_OK;
    }

    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_I2C_ADDR,
        .scl_speed_hz = CONFIG_BSP_I2C_CLK_SPEED_HZ,
    };

    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_rtc_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add DS3231 device to I2C bus: %s", esp_err_to_name(err));
        return err;
    }

    /* Probe device by attempting a 1-byte read of register 0x00 */
    uint8_t reg = 0x00;
    uint8_t test_sec = 0;
    err = i2c_master_transmit_receive(s_rtc_dev_handle, &reg, 1, &test_sec, 1, 200);
    if (err == ESP_OK) {
        s_rtc_present = true;
        ESP_LOGI(TAG, "DS3231 / DS1307 RTC detected at I2C address 0x%02X", DS3231_I2C_ADDR);
    } else {
        s_rtc_present = false;
        ESP_LOGI(TAG, "No DS3231 RTC detected at 0x%02X (optional module)", DS3231_I2C_ADDR);
    }

    return err;
}

bool ds3231_is_present(void)
{
    return s_rtc_present;
}

esp_err_t ds3231_get_time(struct tm *timeinfo)
{
    if (!s_rtc_present || s_rtc_dev_handle == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    if (timeinfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg = 0x00;
    uint8_t data[7] = {0};

    esp_err_t err = i2c_master_transmit_receive(s_rtc_dev_handle, &reg, 1, data, sizeof(data), 500);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read time from DS3231: %s", esp_err_to_name(err));
        return err;
    }

    memset(timeinfo, 0, sizeof(struct tm));
    timeinfo->tm_sec  = bcd2dec(data[0] & 0x7F);
    timeinfo->tm_min  = bcd2dec(data[1] & 0x7F);
    timeinfo->tm_hour = bcd2dec(data[2] & 0x3F); /* 24-hour mode */
    timeinfo->tm_wday = bcd2dec(data[3] & 0x07) - 1;
    timeinfo->tm_mday = bcd2dec(data[4] & 0x3F);
    timeinfo->tm_mon  = bcd2dec(data[5] & 0x1F) - 1;
    timeinfo->tm_year = bcd2dec(data[6]) + 100; /* Year since 1900: (2000 + yr) - 1900 = 100 + yr */
    timeinfo->tm_isdst = -1;

    return ESP_OK;
}

esp_err_t ds3231_set_time(const struct tm *timeinfo)
{
    if (!s_rtc_present || s_rtc_dev_handle == NULL) {
        return ESP_ERR_NOT_FOUND;
    }
    if (timeinfo == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[8];
    data[0] = 0x00; /* Register 0x00 start */
    data[1] = dec2bcd(timeinfo->tm_sec);
    data[2] = dec2bcd(timeinfo->tm_min);
    data[3] = dec2bcd(timeinfo->tm_hour) & 0x3F; /* 24-hr mode: bit 6=0 */
    data[4] = dec2bcd(timeinfo->tm_wday + 1);
    data[5] = dec2bcd(timeinfo->tm_mday);
    data[6] = dec2bcd(timeinfo->tm_mon + 1);
    data[7] = dec2bcd(timeinfo->tm_year >= 100 ? (timeinfo->tm_year - 100) : timeinfo->tm_year);

    esp_err_t err = i2c_master_transmit(s_rtc_dev_handle, data, sizeof(data), 500);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to write time to DS3231: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "DS3231 time updated: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    return ESP_OK;
}

esp_err_t ds3231_sync_to_system(void)
{
    struct tm rtc_tm;
    esp_err_t err = ds3231_get_time(&rtc_tm);
    if (err != ESP_OK) {
        return err;
    }

    /* Verify reasonable year (> 2020) */
    if (rtc_tm.tm_year < (2020 - 1900)) {
        ESP_LOGW(TAG, "DS3231 time is not valid (year < 2020)");
        return ESP_ERR_INVALID_STATE;
    }

    time_t rtc_time = mktime(&rtc_tm);
    struct timeval tv = {
        .tv_sec = rtc_time,
        .tv_usec = 0
    };
    settimeofday(&tv, NULL);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &rtc_tm);
    ESP_LOGI(TAG, "System time synchronized from DS3231 RTC: %s", buf);

    return ESP_OK;
}

esp_err_t ds3231_sync_from_system(void)
{
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2024 - 1900)) {
        return ESP_ERR_INVALID_STATE; /* System time not set yet */
    }

    return ds3231_set_time(&timeinfo);
}
