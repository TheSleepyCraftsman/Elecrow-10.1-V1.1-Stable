#include "ds3231.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_memory_utils.h"
#include "esp_heap_caps.h"
#include "esp_ldo_regulator.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp_board_extra.h"

#include "esp_brookesia.hpp"
#include "app_examples/phone/squareline/src/phone_app_squareline.hpp"
#include "apps.h"
#include "../components/espressif__esp32_p4_function_ev_board/elecrow_ui/include/elecrow_ui.h"
#include "../components/espressif__esp32_p4_function_ev_board/bsp_stc8h1kxx.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp_timer.h"
#include "zigbee_gateway.h"

static const char *TAG = "main";

static TaskHandle_t battery_info_task_handle = NULL;     
uint32_t adc_voltage;
uint32_t bat_voltage;
uint32_t bat_level;
uint8_t bat_state;
uint8_t led_state;

/*
    The task to get battery information from stc8h1kxx via i2c
*/
void battery_info_task(void *param)
{
    vTaskDelay(pdMS_TO_TICKS(10000)); /* Wait 10s for boot animation to finish */
    while (1)
    {
        Battery_info_t battery_info = {0};
        if (bsp_display_lock(pdMS_TO_TICKS(100))) {
            stc8_battery_info_get(&battery_info);
            bsp_display_unlock();
        }
        adc_voltage = battery_info.adc_voltage;
        bat_voltage = battery_info.bat_voltage;
        bat_level   = battery_info.bat_level;
        bat_state   = battery_info.bat_state;
        led_state   = battery_info.led_state;
        if (battery_info.bat_voltage <= 3500 && battery_info.bat_state != 1 && battery_info.bat_state != 2 && battery_info.bat_voltage > 0) {
            ESP_LOGI(TAG, "Battery voltage low (%lu mV) and not charging, entering deep sleep...", battery_info.bat_voltage);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            esp_deep_sleep_start();
        }
        vTaskDelay(pdMS_TO_TICKS(15000)); /* Poll every 15 seconds */
    }
}

extern esp_lcd_touch_handle_t tp;
static int s_prev_brightness = 0;
static bool s_enter_sleep_flag = false;
void touch_detect_task(void *param)
{
    while (1)
    {
        uint32_t inactive_ms = 0;
        if (bsp_display_lock(pdMS_TO_TICKS(50))) {
            inactive_ms = lv_disp_get_inactive_time(NULL);
            bsp_display_unlock();
        }

        if (inactive_ms < 60000) {
            /* User clicked/touched screen: restore brightness if it was sleeping */
            if (s_enter_sleep_flag) {
                s_enter_sleep_flag = false;
                bsp_display_brightness_set(s_prev_brightness);
            }
        }
        else {
            /* Inactive for > 60 seconds: turn off backlight */
            if (!s_enter_sleep_flag) {
                s_enter_sleep_flag = true;
                s_prev_brightness = bsp_display_brightness_get();
                bsp_display_brightness_set(0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


extern "C" void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(bsp_spiffs_mount());
    ESP_LOGI(TAG, "SPIFFS mount successfully");

#if CONFIG_EXAMPLE_ENABLE_SD_CARD
    esp_err_t sd_err = bsp_sdcard_mount();
    if (sd_err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_sdcard_mount failed: %s", esp_err_to_name(sd_err));
    }
    ESP_LOGI(TAG, "SD card mount successfully");
#endif

    ESP_ERROR_CHECK(bsp_extra_codec_init());

    stc8_i2c_init();
    /* Initialize DS3231 / DS1307 RTC and sync system clock */
    ds3231_init();
    ds3231_sync_to_system();
    Battery_info_t battery_info = {0};
    stc8_battery_info_get(&battery_info);
    ESP_LOGI(TAG, "adc_voltage = %lu mV", battery_info.adc_voltage);
    ESP_LOGI(TAG, "bat_voltage = %lu mV", battery_info.bat_voltage);
    ESP_LOGI(TAG, "bat_level = %d %%", battery_info.bat_level);
    ESP_LOGI(TAG, "bat_state = %d", battery_info.bat_state);
    ESP_LOGI(TAG, "led_state = %d", battery_info.led_state);
    if (battery_info.bat_voltage <= 3500 && battery_info.bat_state != 1 && battery_info.bat_state != 2) {
        ESP_LOGI(TAG, "Battery voltage low (%lu mV) and not charging, entering deep sleep...", battery_info.bat_voltage);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        esp_deep_sleep_start();
    }

    bsp_display_start();
    bsp_display_brightness_set(25);

    xTaskCreate(battery_info_task, "battery_info_task", 4096, NULL, 3, &battery_info_task_handle);
    xTaskCreate(touch_detect_task, "touch_detect_task", 2048, NULL, 3, NULL);

    bsp_display_lock(0);
    elecrow_screen();
    bsp_display_unlock();

    while (!elecrow_success)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }


    bsp_display_lock(0);

    ESP_Brookesia_Phone *phone = new ESP_Brookesia_Phone();
    assert(phone != nullptr && "Failed to create phone");

    ESP_Brookesia_PhoneStylesheet_t *phone_stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_1024_600_DARK_STYLESHEET();
    ESP_BROOKESIA_CHECK_NULL_EXIT(phone_stylesheet, "Create phone stylesheet failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->addStylesheet(*phone_stylesheet), "Add phone stylesheet failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->activateStylesheet(*phone_stylesheet), "Activate phone stylesheet failed");

    assert(phone->begin() && "Failed to begin phone");

    PhoneAppSquareline *smart_gadget = new PhoneAppSquareline();
    assert(smart_gadget != nullptr && "Failed to create phone app squareline");
    assert((phone->installApp(smart_gadget) >= 0) && "Failed to install phone app squareline");

    Calculator *calculator = new Calculator();
    assert(calculator != nullptr && "Failed to create calculator");
    assert((phone->installApp(calculator) >= 0) && "Failed to begin calculator");

    MusicPlayer *music_player = new MusicPlayer();
    assert(music_player != nullptr && "Failed to create music_player");
    assert((phone->installApp(music_player) >= 0) && "Failed to begin music_player");

    AppSettings *app_settings = new AppSettings();
    assert(app_settings != nullptr && "Failed to create app_settings");
    assert((phone->installApp(app_settings) >= 0) && "Failed to begin app_settings");

    ZigbeeApp *zigbee_app = new ZigbeeApp();
    assert(zigbee_app != nullptr && "Failed to create zigbee_app");
    assert((phone->installApp(zigbee_app) >= 0) && "Failed to begin zigbee_app");

    Game2048 *game_2048 = new Game2048();
    assert(game_2048 != nullptr && "Failed to create game_2048");
    assert((phone->installApp(game_2048) >= 0) && "Failed to begin game_2048");

    Camera *camera = new Camera(1024, 600);
    assert(camera != nullptr && "Failed to create camera");
    assert((phone->installApp(camera) >= 0) && "Failed to begin camera");

#if CONFIG_EXAMPLE_ENABLE_SD_CARD
    ESP_LOGW(TAG, "Using Video Player example requires inserting the SD card in advance and saving an MJPEG format video on the SD card");
    if (sd_err == ESP_OK) {
        AppVideoPlayer *app_video_player = new AppVideoPlayer();
        assert(app_video_player != nullptr && "Failed to create app_video_player");
        assert((phone->installApp(app_video_player) >= 0) && "Failed to begin app_video_player");
    }
#endif

    bsp_display_unlock();

#if CONFIG_ZIGBEE_GATEWAY_ENABLED
    /* Delay Zigbee coordinator startup by 5s to allow UI and touch drivers to settle */
    xTaskCreate([](void *) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "Starting Zigbee Gateway in background (5s after UI boot)...");
        esp_err_t zb_err = zigbee_gateway_start();
        if (zb_err != ESP_OK) {
            ESP_LOGW(TAG, "Zigbee gateway start failed: %s", esp_err_to_name(zb_err));
        }
        vTaskDelete(NULL);
    }, "zb_delayed_start", 4096, NULL, 3, NULL);
#endif
}
