# P2PPCB mainboards

See: <https://github.com/hajimen/p2ppcb_software>.

Maintainer: [hajimen](https://github.com/hajimen)

## How to add your own layout

Add a new directory in the `keymaps` directory of each mainboard. `rules.mk` and `keymap.c` can do most. 
For example, you can configure USB descriptors by `rules.mk`, like below:

```
OPT_DEFS += -DMANUFACTURER=\"DecentKeyboards\"
OPT_DEFS += -DPRODUCT=\"P2PPCB\ Starter\ Kit\ Bob\"
OPT_DEFS += -DDEVICE_VER=0x0001
OPT_DEFS += -DPRODUCT_ID=0xDE26
OPT_DEFS += -DVENDOR_ID=0xFEED
```

## When Raspberry Pi Pico's LED Flickers

You accidentally wrote Bob's firmware to Charlotte, or Charlotte's firmware to Bob.
