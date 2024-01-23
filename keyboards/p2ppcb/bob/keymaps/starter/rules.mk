# You can overwrite USB descriptor of info.json here

# OPT_DEFS += -DMANUFACTURER=\"DecentKeyboards\"
OPT_DEFS += -DPRODUCT=\"P2PPCB\ Starter\ Kit\ Bob\"
# OPT_DEFS += -DDEVICE_VER=0x0001
# OPT_DEFS += -DPRODUCT_ID=0xDE26
# OPT_DEFS += -DVENDOR_ID=0xFEED

# links test code for pre-delivery inspection
SRC += keyboards/p2ppcb/rp2040/bitbang_i2c.c \
	   keyboards/p2ppcb/rp2040/rp2040_test.c
