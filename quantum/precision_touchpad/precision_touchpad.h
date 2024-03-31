/*
Copyright 2024 Hajime NAKAZATO <hajime@kaoriha.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdint.h>
#include "host.h"
#include "report.h"

#if defined(PRECISION_TOUCHPAD_DRIVER_azoteq_iqs5xx)
#    include "i2c_master.h"
#    include "drivers/sensors/azoteq_iqs5xx.h"
#else
void           precision_touchpad_driver_init(void);
report_precision_touchpad_t precision_touchpad_driver_get_precision_touchpad_report();
report_mouse_t precision_touchpad_driver_get_fallback_mouse_report();
uint16_t       precision_touchpad_driver_get_cpi(void);
void           precision_touchpad_driver_set_cpi(uint16_t cpi);
bool           precision_touchpad_driver_refresh_report();
#endif

typedef struct {
    void (*init)(void);
    report_precision_touchpad_t (*get_precision_touchpad_report)(void);
    report_mouse_t (*get_fallback_mouse_report)(void);
    void (*set_cpi)(uint16_t);
    uint16_t (*get_cpi)(void);
    bool (*refresh_report)(void);
} precision_touchpad_driver_t;

#define XY_REPORT_MIN INT8_MIN
#define XY_REPORT_MAX INT8_MAX
typedef int16_t clamp_range_t;

void           precision_touchpad_init(void);
bool           precision_touchpad_task(void);
bool           precision_touchpad_send(void);
report_precision_touchpad_t precision_touchpad_get_report(void);
void           precision_touchpad_set_report(report_precision_touchpad_t precision_touchpad_report);
uint16_t       precision_touchpad_get_cpi(void);
void           precision_touchpad_set_cpi(uint16_t cpi);

void           precision_touchpad_init_kb(void);
void           precision_touchpad_init_user(void);
report_precision_touchpad_t precision_touchpad_task_kb(report_precision_touchpad_t precision_touchpad_report);
report_precision_touchpad_t precision_touchpad_task_user(report_precision_touchpad_t precision_touchpad_report);

report_mouse_t precision_touchpad_fallback_mouse_get_report(void);
void           precision_touchpad_fallback_mouse_set_report(report_mouse_t mouse_report);
report_mouse_t precision_touchpad_fallback_mouse_task_kb(report_mouse_t mouse_report);
report_mouse_t precision_touchpad_fallback_mouse_task_user(report_mouse_t mouse_report);
