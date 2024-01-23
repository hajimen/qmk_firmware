# MCU name
MCU = STM32F072

# Build Options
#   change yes to no to disable
#
VIA_ENABLE = yes
# Do not enable SLEEP_LED_ENABLE. it uses the same timer with BACKLIGHT_ENABLE
SLEEP_LED_ENABLE = no       # Breathing sleep LED during USB suspend
RGBLIGHT_ENABLE = yes       # Enable keyboard RGB underglow

CUSTOM_MATRIX = lite
SRC += matrix.c
OLED_ENABLE = no		    # When OLED is not connected, enabling OLED driver makes oled_task() unacceptably slow
ANALOG_DRIVER_REQUIRED = yes
