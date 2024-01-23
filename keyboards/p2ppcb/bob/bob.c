#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "matrix.h"
#include QMK_KEYBOARD_H
#include "rp2040_common.h"

static bool no_scan = false;

void matrix_init_custom(void) {
    no_scan = !check_board_type(BOARD_TYPE_BOB);
    if (no_scan) {
        return;
    }

    setPinOutput(GP25);  // on-board LED
    writePinLow(GP25);

    setPinOutput(DS);
    writePinLow(DS);
    setPinOutput(CP);
    writePinLow(CP);
    for (int i = 0; i < MATRIX_ROWS; i ++) {
        wait_200ns();
        writePinHigh(CP);
        wait_200ns();
        writePinLow(CP);
    }
    for (int i = 0; i < MATRIX_COLS; i ++) {
        setPinInputLow(COLS[i]);
    }
}

bool matrix_scan_custom(matrix_row_t matrix[]) {
    if (no_scan) {
        return false;
    }

    bool changed = false;

    for (int i = 0; i < MATRIX_ROWS; i++) {
        if (i == 0) {
            writePinHigh(DS);
        }
        wait_200ns();
        writePinHigh(CP);
        wait_200ns();
        if (i == 0) {
            writePinLow(DS);
        }
        writePinLow(CP);

        // wait for switching/discharging
        chThdSleep(TIME_US2I(1));

        matrix_row_t current_row = 0U;
        uint32_t all = palReadPort(0);
        for (int ii = 0; ii < MATRIX_COLS; ii++) {
            if ((1U << COLS[ii]) & all) {
                current_row |= (1U << ii);
            }
        }
        changed |= (current_row != matrix[ROWS[i]]);
        matrix[ROWS[i]] = current_row;
    }
    wait_200ns();
    writePinHigh(CP);
    wait_200ns();
    writePinLow(CP);

    return changed;
}
