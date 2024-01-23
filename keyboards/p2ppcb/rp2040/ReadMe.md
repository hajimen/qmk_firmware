# P2PPCB RP2040

For pre-delivery inspection, there are special test hardwares for P2PPCB mainboards. 
`rp2040_test.*` are for the test. `bitbang_i2c.*` are also for the test.
You can ignore them.

About `rp2040_common.*`, 
`detect_board()` and `check_board_type()` are useful functions to avoid confusion between 
Charlotte and Bob. `check_board_type()` makes Raspberry Pi Pico's LED flicker when the check fails.
It is called in `bob.c` or `charlotte.c`.
