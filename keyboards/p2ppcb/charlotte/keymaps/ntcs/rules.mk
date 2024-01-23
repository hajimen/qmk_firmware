OPT_DEFS += -DPRODUCT=\"P2PPCB\ NTCS\"

SPLIT_KEYBOARD = yes
SPLIT_TRANSPORT = custom  # To avoid serial.c
QUANTUM_LIB_SRC += i2c_master.c \
				   i2c_slave.c
QUANTUM_SRC += $(QUANTUM_DIR)/split_common/transport.c \
			   $(QUANTUM_DIR)/split_common/transactions.c

OPT_DEFS += -DSPLIT_COMMON_TRANSACTIONS
NO_USB_STARTUP_CHECK = yes
