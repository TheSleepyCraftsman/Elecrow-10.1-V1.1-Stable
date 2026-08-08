idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_hosted esp_timer nvs_flash esp_event
                    PRIV_REQUIRES esp_app_format ota_littlefs)
