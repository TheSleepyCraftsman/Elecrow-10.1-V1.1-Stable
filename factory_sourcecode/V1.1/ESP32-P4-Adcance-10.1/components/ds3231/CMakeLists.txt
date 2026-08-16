idf_component_register(
    SRCS "ds3231.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_i2c esp32_p4_function_ev_board
)
