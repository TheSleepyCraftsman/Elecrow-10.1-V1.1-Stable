#pragma once

#include "lvgl.h"
#include "esp_brookesia.hpp"
#include "zigbee_gateway.h"

class ZigbeeApp : public ESP_Brookesia_PhoneApp
{
public:
    ZigbeeApp();
    ~ZigbeeApp();

    bool init(void) override;
    bool run(void) override;
    bool back(void) override;
    bool close(void) override;

    void updateStatus(void);
    void refreshDeviceList(void);
    void startPairing(uint8_t seconds);
    void stopPairing(void);

private:
    uint16_t _width;
    uint16_t _height;
    bool _is_running;

    /* UI Objects */
    lv_obj_t *_main_cont;
    lv_obj_t *_header_cont;
    lv_obj_t *_status_badge;
    lv_obj_t *_channel_badge;
    lv_obj_t *_panid_badge;
    lv_obj_t *_device_count_label;

    lv_obj_t *_pair_btn;
    lv_obj_t *_pair_btn_label;
    lv_obj_t *_scan_indicator;

    lv_obj_t *_device_list_cont;
    lv_obj_t *_empty_state_obj;

    lv_timer_t *_update_timer;
    int _pairing_remaining_sec;

    static void pair_btn_event_cb(lv_event_t *e);
    static void refresh_btn_event_cb(lv_event_t *e);
    static void timer_cb(lv_timer_t *timer);
};
