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


#include "precision_touchpad.h"
#include <string.h>
#include "timer.h"
#include "gpio.h"

static report_mouse_t local_mouse_report  = {0};
static report_precision_touchpad_t local_precision_touchpad_report = {0};
static bool precision_touchpad_force_send = false;
static bool precision_touchpad_fallback_mouse_force_send = false;

extern const precision_touchpad_driver_t precision_touchpad_driver;

/**
 * @brief Keyboard level code pointing device initialisation
 *
 */
__attribute__((weak)) void precision_touchpad_init_kb(void) {}

/**
 * @brief User level code pointing device initialisation
 *
 */
__attribute__((weak)) void precision_touchpad_init_user(void) {}

/**
 * @brief Weak function allowing for keyboard level precision touchpad report modification
 *
 * Takes report_precision_touchpad_t struct allowing modification at keyboard level then returns report_precision_touchpad_t.
 *
 * @param[in] precision_touchpad_report report_precision_touchpad_t
 * @return report_precision_touchpad_t
 */
__attribute__((weak)) report_precision_touchpad_t precision_touchpad_task_kb(report_precision_touchpad_t precision_touchpad_report) {
    return precision_touchpad_task_user(precision_touchpad_report);
}

/**
 * @brief Weak function allowing for user level mouse report modification
 *
 * Takes report_mouse_t struct allowing modification at user level then returns report_mouse_t.
 *
 * @param[in] precision_touchpad_report report_precision_touchpad_t
 * @return report_precision_touchpad_t
 */
__attribute__((weak)) report_precision_touchpad_t precision_touchpad_task_user(report_precision_touchpad_t precision_touchpad_report) {
    return precision_touchpad_report;
}

/**
 * @brief Weak function allowing for keyboard level fallback mouse report modification
 *
 * Takes report_mouse_t struct allowing modification at keyboard level then returns report_mouse_t.
 *
 * @param[in] mouse_report report_mouse_t
 * @return report_mouse_t
 */
__attribute__((weak)) report_mouse_t precision_touchpad_fallback_mouse_task_kb(report_mouse_t mouse_report) {
    return precision_touchpad_fallback_mouse_task_user(mouse_report);
}

/**
 * @brief Weak function allowing for user level fallback mouse report modification
 *
 * Takes report_mouse_t struct allowing modification at user level then returns report_mouse_t.
 *
 * @param[in] mouse_report report_mouse_t
 * @return report_mouse_t
 */
__attribute__((weak)) report_mouse_t precision_touchpad_fallback_mouse_task_user(report_mouse_t mouse_report) {
    return mouse_report;
}

/**
 * @brief Initialises precision touchpad
 *
 * Initialises precision touchpad, perform driver init and optional keyboard/user level code.
 */
__attribute__((weak)) void precision_touchpad_init(void) {
    {
        precision_touchpad_driver.init();
        palSetLineMode(PRECISION_TOUCHPAD_RDY_PIN, PAL_MODE_INPUT);
    }

    precision_touchpad_init_kb();
    precision_touchpad_init_user();
}

/**
 * @brief Sends processed precision touchpad and fallback mouse reports to host
 *
 * This sends the precision touchpad and fallback mouse reports generated by precision_touchpad_task if changed since the last report.
 *
 */
__attribute__((weak)) bool precision_touchpad_send(void) {
    static report_precision_touchpad_t old_precision_touchpad_report = {};
    static report_mouse_t old_mose_report = {};
    bool should_send_precision_touchpad_report = has_precision_touchpad_report_changed(&local_precision_touchpad_report, &old_precision_touchpad_report);
    bool should_send_mouse_report = has_mouse_report_changed(&local_mouse_report, &old_mose_report);

    if (should_send_precision_touchpad_report) {
        host_precision_touchpad_send(&local_precision_touchpad_report);
    }
    // Everything is absolute.
    memcpy(&old_precision_touchpad_report, &local_precision_touchpad_report, sizeof(local_precision_touchpad_report));

    if (should_send_mouse_report) {
        host_mouse_send(&local_mouse_report);
    }
    // Just buttons are absolute, other x/y/h/v are relative.
    uint8_t buttons = local_mouse_report.buttons;
    memset(&local_mouse_report, 0, sizeof(local_mouse_report));
    local_mouse_report.buttons = buttons;
    memcpy(&old_mose_report, &local_mouse_report, sizeof(local_mouse_report));

    return should_send_precision_touchpad_report || should_send_mouse_report;
}

/**
 * @brief Retrieves and processes precision touchpad data.
 *
 * This function is part of the keyboard loop and retrieves the mouse report from the precision touchpad driver.
 * It applies any optional configuration e.g. rotation or axis inversion and then initiates a send.
 *
 */
__attribute__((weak)) bool precision_touchpad_task(void) {
    if (!precision_touchpad_driver.read_report()) {
        return false;
    }
    local_precision_touchpad_report = precision_touchpad_driver.get_precision_touchpad_report();
    local_mouse_report = precision_touchpad_driver.get_fallback_mouse_report();

    // allow kb to intercept and modify report
    local_precision_touchpad_report = precision_touchpad_task_kb(local_precision_touchpad_report);
    local_mouse_report = precision_touchpad_fallback_mouse_task_kb(local_mouse_report);
    const bool send_report     = precision_touchpad_send() || precision_touchpad_force_send;
    precision_touchpad_force_send = false;

    return send_report;
}

/**
 * @brief Gets current precision touchpad report used by precision touchpad task
 *
 * @return report_precision_touchpad_t
 */
report_precision_touchpad_t precision_touchpad_get_report(void) {
    return local_precision_touchpad_report;
}

/**
 * @brief Sets precision touchpad report used be precision touchpad task
 *
 * @param[in] precision_touchpad_report
 */
void precision_touchpad_set_report(report_precision_touchpad_t precision_touchpad_report) {
    precision_touchpad_force_send = has_precision_touchpad_report_changed(&local_precision_touchpad_report, &precision_touchpad_report);
    memcpy(&local_precision_touchpad_report, &precision_touchpad_report, sizeof(local_precision_touchpad_report));
}

/**
 * @brief Gets current precision touchpad CPI if supported
 *
 * Gets current cpi from precision touchpad driver if supported and returns it as uint16_t
 *
 * @return cpi value as uint16_t
 */
uint16_t precision_touchpad_get_cpi(void) {
    return precision_touchpad_driver.get_cpi();
}

/**
 * @brief Set precision touchpad CPI if supported
 *
 * Takes a uint16_t value to set precision touchpad cpi if supported by driver.
 *
 * @param[in] cpi uint16_t value.
 */
void precision_touchpad_set_cpi(uint16_t cpi) {
    precision_touchpad_driver.set_cpi(cpi);
}

/**
 * @brief Gets current fallback mouse report used by precision touchpad task
 *
 * @return report_mouse_t
 */
report_mouse_t precision_touchpad_fallback_mouse_get_report(void) {
    return local_mouse_report;
}

/**
 * @brief Sets fallback mouse report used be precision touchpad task
 *
 * @param[in] mouse_report
 */
void precision_touchpad_fallback_mouse_set_report(report_mouse_t mouse_report) {
    precision_touchpad_fallback_mouse_force_send = has_mouse_report_changed(&local_mouse_report, &mouse_report);
    memcpy(&local_mouse_report, &mouse_report, sizeof(local_mouse_report));
}
