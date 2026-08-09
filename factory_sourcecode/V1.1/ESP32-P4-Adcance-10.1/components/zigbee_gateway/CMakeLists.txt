if(CONFIG_ZIGBEE_GATEWAY_ENABLED)
    set(ZB_REQUIRES espressif__esp-zigbee-lib)
    idf_component_get_property(zb_lib_dir espressif__esp-zigbee-lib COMPONENT_DIR)
    set(ZB_INCLUDES "${zb_lib_dir}/include/compat")
else()
    set(ZB_REQUIRES "")
    set(ZB_INCLUDES "")
endif()

idf_component_register(
    SRCS "zigbee_gateway.c"
    INCLUDE_DIRS "." ${ZB_INCLUDES}
    REQUIRES nvs_flash esp_event esp_driver_uart ${ZB_REQUIRES}
)
