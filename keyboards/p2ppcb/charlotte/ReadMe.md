# P2PPCB mainboard Charlotte

This codebase implements:

- I2C master and slave (for split keyboards)
- Custom matrix scan
- LED key connection detection
- LED on / off control

## I2C Master and Slave

The implementation includes HAL of ChibiOS. `charlotte_i2c*` files are them.
If you want to remove I2C codes from your firmware, set `OPT_DEFS += -DCHARLOTTE_USE_I2C=FALSE`
in your `rules.mk` of your keymap.

From the viewpoint of ChibiOS build system, `charlotte_i2c*` are Community HAL. `hal_community.h` is included by 
ChibiOS build system.

`i2c_master.*` and `i2c_slave.*` are equivalent to QMK's.

For split keyborards, mimic the `keymaps/ntcs` implementation.

## Custom matrix scan

Normal keys are the same as Bob.

Usually QMK scans the matrix in a transaction of USB interrupt transfer. `matrix_scan_custom()` does the job. 
This design assumes that scanning is fast enough, because a interrupt transaction should finish in 10 ms. 
The deadline is not a problem for normal keys. 
But for LED keys, reading switches and driving LEDs are multiplexed by time division. `matrix_scan_custom()` 
shouldn't wait for the reading time. Therefore Charlotte firmware reads LED keys asynchronously. `adc_callback()` stores 
the scanned switch states to `matrix_led_sw[][]` independent of USB. `matrix_scan_custom()` loads the switch states,
not scans.

## LED Key Connection Detection

```
bool is_led_connected(int led_s, int led_d);
```

This feature is useful for split keyboards, because it allows to connect USB to left or right (not at the same time, of course).
You can implement QMK's `is_keyboard_left()` with this feature and asymmetric LED key matrix.
`keymaps/ntcs` is an example. The left part of NTCS has `LED_S0` - `LED_D0` key, and the right part doesn't.
So the following code in `keymaps/ntcs/keymap.c` works.

```
bool is_keyboard_left(void) {
    return is_led_connected(0, 0);
}
```

## LED on / off control

`MATRIX_LED_ONOFF[][]` is the interface. See `keymaps/starter/keymap.c` for an example.

## Not used

`analog.*` are unused. I implemented them for chicking ChibiOS, but 
no need for the codebase itself.
