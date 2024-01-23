# Build Options
#   change yes to no to disable
#
BOARD = GENERIC_RP_RP2040

VIA_ENABLE = yes

SRC += charlotte_i2c_lld.c charlotte_i2c.c

CUSTOM_MATRIX = lite

SRC += keyboards/p2ppcb/rp2040/rp2040_common.c
EXTRAINCDIRS += keyboards/p2ppcb/rp2040/
