#include "ZigbeeApp.hpp"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "ZigbeeApp";

LV_IMG_DECLARE(img_app_zigbee);

ZigbeeApp::ZigbeeApp():
    ESP_Brookesia_PhoneApp("Zigbee", &img_app_zigbee, true),
    _width(0),
    _height(0),
    _is_running(false),
    _main_cont(NULL),
    _header_cont(NULL),
    _status_badge(NULL),
    _channel_badge(NULL),
    _panid_badge(NULL),
    _device_count_label(NULL),
    _pair_btn(NULL),
    _pair_btn_label(NULL),
    _scan_indicator(NULL),
    _device_list_cont(NULL),
    _empty_state_obj(NULL),
    _update_timer(NULL),
    _pairing_remaining_sec(0)
{
}

ZigbeeApp::~ZigbeeApp()
{
}

bool ZigbeeApp::init(void)
{
    ESP_LOGI(TAG, "ZigbeeApp init");
    return true;
}

bool ZigbeeApp::run(void)
{
    lv_area_t area = getVisualArea();
    _width = area.x2 - area.x1;
    _height = area.y2 - area.y1;
    _is_running = true;

    /* Main visual container with sleek dark background */
    _main_cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(_main_cont, _width, _height);
    lv_obj_align(_main_cont, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(_main_cont, lv_color_make(18, 20, 26), 0);
    lv_obj_set_style_pad_all(_main_cont, 16, 0);
    lv_obj_set_style_border_width(_main_cont, 0, 0);
    lv_obj_set_style_radius(_main_cont, 0, 0);
    lv_obj_set_flex_flow(_main_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_main_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 1. Header Card with Coordinator Network Stats */
    _header_cont = lv_obj_create(_main_cont);
    lv_obj_set_size(_header_cont, _width - 32, 100);
    lv_obj_set_style_bg_color(_header_cont, lv_color_make(28, 32, 42), 0);
    lv_obj_set_style_border_color(_header_cont, lv_color_make(45, 52, 68), 0);
    lv_obj_set_style_border_width(_header_cont, 1, 0);
    lv_obj_set_style_radius(_header_cont, 14, 0);
    lv_obj_set_style_pad_hor(_header_cont, 20, 0);
    lv_obj_set_style_pad_ver(_header_cont, 12, 0);
    lv_obj_clear_flag(_header_cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Title + Coordinator info */
    lv_obj_t *title_label = lv_label_create(_header_cont);
    lv_label_set_text(title_label, LV_SYMBOL_WIFI " Zigbee Coordinator (ESP32-H2)");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title_label, lv_color_make(255, 255, 255), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Badges Row */
    lv_obj_t *badge_row = lv_obj_create(_header_cont);
    lv_obj_set_size(badge_row, _width - 320, 36);
    lv_obj_align(badge_row, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(badge_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(badge_row, 0, 0);
    lv_obj_set_style_pad_all(badge_row, 0, 0);
    lv_obj_set_flex_flow(badge_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(badge_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(badge_row, 12, 0);
    lv_obj_clear_flag(badge_row, LV_OBJ_FLAG_SCROLLABLE);

    /* Online Status Badge */
    _status_badge = lv_label_create(badge_row);
    lv_obj_set_style_bg_color(_status_badge, lv_color_make(34, 75, 48), 0);
    lv_obj_set_style_bg_opa(_status_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(_status_badge, lv_color_make(80, 220, 120), 0);
    lv_obj_set_style_text_font(_status_badge, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_hor(_status_badge, 10, 0);
    lv_obj_set_style_pad_ver(_status_badge, 4, 0);
    lv_obj_set_style_radius(_status_badge, 6, 0);
    lv_label_set_text(_status_badge, "● ONLINE");

    /* Channel Badge */
    _channel_badge = lv_label_create(badge_row);
    lv_obj_set_style_bg_color(_channel_badge, lv_color_make(38, 44, 58), 0);
    lv_obj_set_style_bg_opa(_channel_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(_channel_badge, lv_color_make(200, 210, 230), 0);
    lv_obj_set_style_text_font(_channel_badge, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_hor(_channel_badge, 10, 0);
    lv_obj_set_style_pad_ver(_channel_badge, 4, 0);
    lv_obj_set_style_radius(_channel_badge, 6, 0);
    lv_label_set_text_fmt(_channel_badge, "CH %d", zigbee_gateway_get_channel());

    /* PAN ID Badge */
    _panid_badge = lv_label_create(badge_row);
    lv_obj_set_style_bg_color(_panid_badge, lv_color_make(38, 44, 58), 0);
    lv_obj_set_style_bg_opa(_panid_badge, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(_panid_badge, lv_color_make(200, 210, 230), 0);
    lv_obj_set_style_text_font(_panid_badge, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_hor(_panid_badge, 10, 0);
    lv_obj_set_style_pad_ver(_panid_badge, 4, 0);
    lv_obj_set_style_radius(_panid_badge, 6, 0);
    lv_label_set_text_fmt(_panid_badge, "PAN ID 0x%04X", zigbee_gateway_get_pan_id());

    /* Pair / Permit Join Button */
    _pair_btn = lv_btn_create(_header_cont);
    lv_obj_set_size(_pair_btn, 170, 48);
    lv_obj_align(_pair_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(_pair_btn, lv_color_make(235, 94, 40), 0);
    lv_obj_set_style_radius(_pair_btn, 10, 0);
    lv_obj_add_event_cb(_pair_btn, pair_btn_event_cb, LV_EVENT_CLICKED, this);

    _pair_btn_label = lv_label_create(_pair_btn);
    lv_label_set_text(_pair_btn_label, LV_SYMBOL_PLUS " Pair Device");
    lv_obj_set_style_text_font(_pair_btn_label, &lv_font_montserrat_16, 0);
    lv_obj_center(_pair_btn_label);

    /* 2. Device List Area */
    _device_list_cont = lv_obj_create(_main_cont);
    lv_obj_set_size(_device_list_cont, _width - 32, _height - 148);
    lv_obj_set_style_bg_color(_device_list_cont, lv_color_make(24, 28, 36), 0);
    lv_obj_set_style_border_color(_device_list_cont, lv_color_make(38, 44, 58), 0);
    lv_obj_set_style_border_width(_device_list_cont, 1, 0);
    lv_obj_set_style_radius(_device_list_cont, 14, 0);
    lv_obj_set_style_pad_all(_device_list_cont, 16, 0);
    lv_obj_set_flex_flow(_device_list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_device_list_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(_device_list_cont, 12, 0);

    /* Initial device list populate */
    refreshDeviceList();

    /* Periodic UI refresh timer (every 1 second) */
    _update_timer = lv_timer_create(timer_cb, 1000, this);

    return true;
}

bool ZigbeeApp::back(void)
{
    return close();
}

bool ZigbeeApp::close(void)
{
    _is_running = false;
    if (_update_timer) {
        lv_timer_del(_update_timer);
        _update_timer = NULL;
    }
    return true;
}

void ZigbeeApp::startPairing(uint8_t seconds)
{
    _pairing_remaining_sec = seconds;
    zigbee_gateway_open_network(seconds);
    if (_pair_btn_label) {
        lv_label_set_text_fmt(_pair_btn_label, LV_SYMBOL_REFRESH " Pairing (%ds)", _pairing_remaining_sec);
        lv_obj_set_style_bg_color(_pair_btn, lv_color_make(180, 60, 20), 0);
    }
}

void ZigbeeApp::stopPairing(void)
{
    _pairing_remaining_sec = 0;
    zigbee_gateway_open_network(0);
    if (_pair_btn_label) {
        lv_label_set_text(_pair_btn_label, LV_SYMBOL_PLUS " Pair Device");
        lv_obj_set_style_bg_color(_pair_btn, lv_color_make(235, 94, 40), 0);
    }
}

void ZigbeeApp::updateStatus(void)
{
    if (!_is_running) return;

    bool running = zigbee_gateway_is_running();
    if (_status_badge) {
        if (running) {
            lv_label_set_text(_status_badge, "● ONLINE");
            lv_obj_set_style_bg_color(_status_badge, lv_color_make(34, 75, 48), 0);
            lv_obj_set_style_text_color(_status_badge, lv_color_make(80, 220, 120), 0);
        } else {
            lv_label_set_text(_status_badge, "● STARTING");
            lv_obj_set_style_bg_color(_status_badge, lv_color_make(75, 60, 20), 0);
            lv_obj_set_style_text_color(_status_badge, lv_color_make(255, 190, 60), 0);
        }
    }

    if (_channel_badge) {
        lv_label_set_text_fmt(_channel_badge, "CH %d", zigbee_gateway_get_channel());
    }
    if (_panid_badge) {
        lv_label_set_text_fmt(_panid_badge, "PAN ID 0x%04X", zigbee_gateway_get_pan_id());
    }

    if (_pairing_remaining_sec > 0) {
        _pairing_remaining_sec--;
        if (_pairing_remaining_sec > 0) {
            lv_label_set_text_fmt(_pair_btn_label, LV_SYMBOL_REFRESH " Pairing (%ds)", _pairing_remaining_sec);
        } else {
            stopPairing();
            refreshDeviceList();
        }
    }
}

void ZigbeeApp::refreshDeviceList(void)
{
    if (!_device_list_cont) return;

    /* Clean existing children */
    lv_obj_clean(_device_list_cont);

    zb_gw_device_t devices[ZB_GW_MAX_DEVICES];
    uint8_t count = 0;
    esp_err_t err = zigbee_gateway_get_devices(devices, &count);

    if (err != ESP_OK || count == 0) {
        /* Empty State */
        lv_obj_t *empty_card = lv_obj_create(_device_list_cont);
        lv_obj_set_size(empty_card, _width - 64, _height - 180);
        lv_obj_set_style_bg_opa(empty_card, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(empty_card, 0, 0);
        lv_obj_set_flex_flow(empty_card, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(empty_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *icon = lv_label_create(empty_card);
        lv_label_set_text(icon, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_48, 0);
        lv_obj_set_style_text_color(icon, lv_color_make(90, 105, 130), 0);

        lv_obj_t *msg = lv_label_create(empty_card);
        lv_label_set_text(msg, "No Zigbee Devices Paired");
        lv_obj_set_style_text_font(msg, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(msg, lv_color_make(200, 210, 225), 0);
        lv_obj_set_style_pad_top(msg, 12, 0);

        lv_obj_t *sub = lv_label_create(empty_card);
        lv_label_set_text(sub, "Put your Zigbee smart plug, light, or sensor into pairing mode,\nthen tap 'Pair Device' above.");
        lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(sub, lv_color_make(130, 145, 165), 0);
        lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_pad_top(sub, 8, 0);
        return;
    }

    /* Populate Device Cards */
    for (int i = 0; i < count; i++) {
        lv_obj_t *card = lv_obj_create(_device_list_cont);
        lv_obj_set_size(card, _width - 64, 76);
        lv_obj_set_style_bg_color(card, lv_color_make(34, 40, 52), 0);
        lv_obj_set_style_border_color(card, lv_color_make(48, 56, 74), 0);
        lv_obj_set_style_border_width(card, 1, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_set_style_pad_hor(card, 16, 0);
        lv_obj_set_style_pad_ver(card, 10, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        /* Icon */
        lv_obj_t *dev_icon = lv_label_create(card);
        lv_label_set_text(dev_icon, LV_SYMBOL_POWER);
        lv_obj_set_style_text_font(dev_icon, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(dev_icon, lv_color_make(235, 94, 40), 0);
        lv_obj_align(dev_icon, LV_ALIGN_LEFT_MID, 0, 0);

        /* Name & Short Addr */
        lv_obj_t *name_lbl = lv_label_create(card);
        lv_label_set_text_fmt(name_lbl, "Zigbee Device [0x%04X]", devices[i].short_addr);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(name_lbl, lv_color_make(255, 255, 255), 0);
        lv_obj_align(name_lbl, LV_ALIGN_TOP_LEFT, 40, 0);

        /* IEEE MAC Address */
        lv_obj_t *mac_lbl = lv_label_create(card);
        lv_label_set_text_fmt(mac_lbl, "MAC: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X | EP: %d",
                             devices[i].ieee_addr[7], devices[i].ieee_addr[6],
                             devices[i].ieee_addr[5], devices[i].ieee_addr[4],
                             devices[i].ieee_addr[3], devices[i].ieee_addr[2],
                             devices[i].ieee_addr[1], devices[i].ieee_addr[0],
                             devices[i].endpoint);
        lv_obj_set_style_text_font(mac_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(mac_lbl, lv_color_make(150, 165, 185), 0);
        lv_obj_align(mac_lbl, LV_ALIGN_BOTTOM_LEFT, 40, 0);

        /* Status Badge */
        lv_obj_t *tag = lv_label_create(card);
        lv_label_set_text(tag, devices[i].online ? "ONLINE" : "OFFLINE");
        lv_obj_set_style_text_font(tag, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(tag, devices[i].online ? lv_color_make(80, 220, 120) : lv_color_make(180, 180, 180), 0);
        lv_obj_set_style_bg_color(tag, devices[i].online ? lv_color_make(34, 75, 48) : lv_color_make(50, 50, 50), 0);
        lv_obj_set_style_bg_opa(tag, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_hor(tag, 8, 0);
        lv_obj_set_style_pad_ver(tag, 3, 0);
        lv_obj_set_style_radius(tag, 4, 0);
        lv_obj_align(tag, LV_ALIGN_RIGHT_MID, 0, 0);
    }
}

void ZigbeeApp::pair_btn_event_cb(lv_event_t *e)
{
    ZigbeeApp *app = (ZigbeeApp *)lv_event_get_user_data(e);
    if (!app) return;

    if (app->_pairing_remaining_sec > 0) {
        app->stopPairing();
    } else {
        app->startPairing(60); /* 60 seconds pairing window */
    }
}

void ZigbeeApp::timer_cb(lv_timer_t *timer)
{
    ZigbeeApp *app = (ZigbeeApp *)timer->user_data;
    if (app) {
        app->updateStatus();
    }
}
