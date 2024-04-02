/* Copyright 2024 Hajime NAKAZATO <hajime@kaoriha.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precision_touchpad.h"
#include "gpio.h"
#include "wait.h"
#include "timer.h"
#include <stddef.h>

#ifdef PRECISION_TOUCHPAD_DEBUG
#    include "debug.h"
#    include "print.h"
#    define pt_dprintf(...) dprintf(__VA_ARGS__)
#else
#    define pt_dprintf(...) \
        do {                \
        } while (0)
#endif

#define CONSTRAIN_HID(amt) ((amt) < INT8_MIN ? INT8_MIN : ((amt) > INT8_MAX ? INT8_MAX : (amt)))
#define CONSTRAIN_HID_XY(amt) ((amt) < XY_REPORT_MIN ? XY_REPORT_MIN : ((amt) > XY_REPORT_MAX ? XY_REPORT_MAX : (amt)))

static report_precision_touchpad_t local_precision_touchpad_report = {0};
static report_mouse_t local_mouse_report = {0};
bool precision_touchpad_input_mode_touchpad = false;
bool precision_touchpad_surface_switch_enabled = false;
bool precision_touchpad_button_switch_enabled = false;

typedef union {
    struct {
        bool button_1 : 1;
        bool button_2 : 1;
        bool button_3 : 1;
        bool button_4 : 1;
        bool button_5 : 1;
        bool button_6 : 1;
        bool button_7 : 1;
        bool button_8 : 1;
    } PACKED b;
    uint8_t v;
} mouse_buttons_t;

#if defined(PRECISION_TOUCHPAD_DRIVER_azoteq_iqs5xx)
static i2c_status_t azoteq_iqs5xx_init_status = 1;
static uint16_t scan_time = 0;

void azoteq_iqs5xx_init(void) {
    i2c_init();
    azoteq_iqs5xx_wake();
    azoteq_iqs5xx_reset_suspend(true, false, true);
    wait_ms(100);
    azoteq_iqs5xx_wake();
    if (azoteq_iqs5xx_get_product() != AZOTEQ_IQS5XX_UNKNOWN) {
        azoteq_iqs5xx_setup_resolution();
        azoteq_iqs5xx_init_status = azoteq_iqs5xx_set_report_rate(AZOTEQ_IQS5XX_REPORT_RATE, AZOTEQ_IQS5XX_ACTIVE, false);
        azoteq_iqs5xx_init_status |= azoteq_iqs5xx_set_event_mode(true, false);  // Always EVENT_MODE on
        azoteq_iqs5xx_init_status |= azoteq_iqs5xx_set_reati(true, false);
#    if defined(AZOTEQ_IQS5XX_ROTATION_90)
        azoteq_iqs5xx_init_status |= azoteq_iqs5xx_set_xy_config(false, true, true, true, false);
#    elif defined(AZOTEQ_IQS5XX_ROTATION_180)
        azoteq_iqs5xx_init_status |= azoteq_iqs5xx_set_xy_config(true, true, false, true, false);
#    elif defined(AZOTEQ_IQS5XX_ROTATION_270)
        azoteq_iqs5xx_init_status |= azoteq_iqs5xx_set_xy_config(true, false, true, true, false);
#    else
        azoteq_iqs5xx_init_status |= azoteq_iqs5xx_set_xy_config(false, false, false, true, false);
#    endif
        azoteq_iqs5xx_init_status |= azoteq_iqs5xx_set_gesture_config(true);
        wait_ms(AZOTEQ_IQS5XX_REPORT_RATE + 1);
    }
};

volatile int debug_x = 1;
volatile uint8_t debug_counter = 0;

bool azoteq_iqs5xx_refresh_report(void) {
    debug_enable = true;
    report_precision_touchpad_t temp_precision_touchpad_report = {0};
    report_mouse_t temp_mouse_report     = {0};
    static uint8_t previous_button_state = 0;
    static uint8_t read_error_count      = 0;
    static bool was_active               = false;

    if (azoteq_iqs5xx_init_status == I2C_STATUS_SUCCESS) {
        if (!palReadLine(PRECISION_TOUCHPAD_RDY_PIN)) {
            if (precision_touchpad_input_mode_touchpad) {
                return false;
            }
            if (was_active) {
                pt_dprintf("IQS5XX - was_active occurs.\n");
                was_active = false;
                local_mouse_report = temp_mouse_report;
                return true;
            }
            return false;
        }
        azoteq_iqs5xx_base_data_t base_data = {0};
        azoteq_iqs5xx_precision_touchpad_data_t precision_touchpad_data = {0};
        i2c_status_t status          = azoteq_iqs5xx_get_precision_touchpad_data(&base_data, &precision_touchpad_data);
        bool         ignore_movement = false;

        if (status == I2C_STATUS_SUCCESS) {
            // pt_dprintf("IQS5XX - previous cycle time: %d \n", base_data.previous_cycle_time);

            // no neeed if always EVENT_MODE on
            // if (base_data.previous_cycle_time == 0) {  // I don't know why, but it occurs and have invalid data.
            //     pt_dprintf("IQS5XX - base_data.previous_cycle_time == 0 occurs.\n");
            //     return false;
            // }

            read_error_count = 0;
            mouse_buttons_t mouse_buttons = {0};
            was_active = true;
            if (!precision_touchpad_input_mode_touchpad) {
                if (base_data.gesture_events_0.single_tap || base_data.gesture_events_0.press_and_hold) {
                    pt_dprintf("IQS5XX - Single tap/hold.\n");
                    mouse_buttons.b.button_1 = true;
                } else if (base_data.gesture_events_1.two_finger_tap) {
                    pt_dprintf("IQS5XX - Two finger tap.\n");
                    mouse_buttons.b.button_2 = true;
                } else if (base_data.gesture_events_0.swipe_x_neg) {
                    pt_dprintf("IQS5XX - X-.\n");
                    mouse_buttons.b.button_4 = true;
                    ignore_movement     = true;
                } else if (base_data.gesture_events_0.swipe_x_pos) {
                    pt_dprintf("IQS5XX - X+.\n");
                    mouse_buttons.b.button_5 = true;
                    ignore_movement     = true;
                } else if (base_data.gesture_events_0.swipe_y_neg) {
                    pt_dprintf("IQS5XX - Y-.\n");
                    mouse_buttons.b.button_6 = true;
                    ignore_movement     = true;
                } else if (base_data.gesture_events_0.swipe_y_pos) {
                    pt_dprintf("IQS5XX - Y+.\n");
                    mouse_buttons.b.button_3 = true;
                    ignore_movement     = true;
                } else if (base_data.gesture_events_1.zoom) {
                    if (AZOTEQ_IQS5XX_COMBINE_H_L_BYTES(base_data.x.h, base_data.x.l) < 0) {
                        pt_dprintf("IQS5XX - Zoom out.\n");
                        mouse_buttons.b.button_7 = true;
                    } else if (AZOTEQ_IQS5XX_COMBINE_H_L_BYTES(base_data.x.h, base_data.x.l) > 0) {
                        pt_dprintf("IQS5XX - Zoom in.\n");
                        mouse_buttons.b.button_8 = true;
                    }
                } else if (base_data.gesture_events_1.scroll) {
                    pt_dprintf("IQS5XX - Scroll.\n");
                    temp_mouse_report.h = CONSTRAIN_HID(AZOTEQ_IQS5XX_COMBINE_H_L_BYTES(base_data.x.h, base_data.x.l));
                    temp_mouse_report.v = CONSTRAIN_HID(AZOTEQ_IQS5XX_COMBINE_H_L_BYTES(base_data.y.h, base_data.y.l));
                }
                if (base_data.number_of_fingers == 1 && !ignore_movement) {
                    temp_mouse_report.x = CONSTRAIN_HID_XY(AZOTEQ_IQS5XX_COMBINE_H_L_BYTES(base_data.x.h, base_data.x.l));
                    temp_mouse_report.y = CONSTRAIN_HID_XY(AZOTEQ_IQS5XX_COMBINE_H_L_BYTES(base_data.y.h, base_data.y.l));
                }
                temp_mouse_report.buttons = mouse_buttons.v;

                previous_button_state = temp_mouse_report.buttons;
            }
            local_mouse_report = temp_mouse_report;

            bool confidence = !base_data.system_info_1.palm_detect;
            static bool flip_contact_id[5] = {false};
            static bool active_finger[5] = {false};
            static uint16_t last_x[16] = {0};
            static uint16_t last_y[16] = {0};
            uint8_t ii = 0;
            for (uint8_t i = 0; i < 5; i++) {
                azoteq_iqs5xx_finger_data_t *f = &(precision_touchpad_data.finger[i]);
                collection_precision_touchpad_contact_t *c = &(temp_precision_touchpad_report.contact[ii]);
                c->pressure = AZOTEQ_IQS5XX_SWAP_H_L_BYTES(f->touch_strength);
                c->tip = c->pressure > 0;
                if (!c->tip && !active_finger[i]) {
                    continue;
                }
                c->contact_id = i + (flip_contact_id[i] ? 0 : 8);
                active_finger[i] = c->tip;
                if (c->tip) {
                    c->confidence = confidence;
                    c->x = AZOTEQ_IQS5XX_SWAP_H_L_BYTES(f->x);
                    last_x[c->contact_id] = c->x;
                    c->y = AZOTEQ_IQS5XX_SWAP_H_L_BYTES(f->y);
                    last_y[c->contact_id] = c->y;
                } else {
                    c->confidence = true;
                    flip_contact_id[i] = !flip_contact_id[i];
                    c->x = last_x[c->contact_id];
                    c->y = last_y[c->contact_id];
                }
                ii++;
            }
            temp_precision_touchpad_report.contact_count = ii;
            scan_time += base_data.previous_cycle_time * 10;
            temp_precision_touchpad_report.scan_time = scan_time;
            local_precision_touchpad_report = temp_precision_touchpad_report;
        } else {
            if (read_error_count > 10) {
                read_error_count      = 0;
                previous_button_state = 0;
            } else {
                read_error_count++;
            }
            temp_mouse_report.buttons = previous_button_state;
            pt_dprintf("IQS5XX - get report failed: %d \n", status);
        }
    } else {
        pt_dprintf("IQS5XX - Init failed: %d \n", azoteq_iqs5xx_init_status);
    }

    return true;
}

report_precision_touchpad_t azoteq_iqs5xx_get_precision_touchpad_report(void) {
    return local_precision_touchpad_report;
}

report_mouse_t azoteq_iqs5xx_get_fallback_mouse_report(void) {
    return local_mouse_report;
}

// clang-format off
const precision_touchpad_driver_t precision_touchpad_driver = {
    .init       = azoteq_iqs5xx_init,
    .get_precision_touchpad_report = azoteq_iqs5xx_get_precision_touchpad_report,
    .get_fallback_mouse_report = azoteq_iqs5xx_get_fallback_mouse_report,
    .refresh_report = azoteq_iqs5xx_refresh_report
};
// clang-format on
#else
__attribute__((weak)) void precision_touchpad_driver_init(void) {}
__attribute__((weak)) report_precision_touchpad_t precision_touchpad_driver_get_precision_touchpad_report() {
    return local_precision_touchpad_report;
}
__attribute__((weak)) report_mouse_t precision_touchpad_driver_get_fallback_mouse_report() {
    return local_mouse_report;
}
__attribute__((weak)) bool precision_touchpad_driver_refresh_report() {
    return false;
}
#endif
