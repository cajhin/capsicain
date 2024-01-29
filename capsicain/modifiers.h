#pragma once
#include "scancodes.h"

#ifndef MAPPINGS_H
#define MAPPINGS_H

const int NUMBER_OF_MODIFIERS = 32;
using MOD = uint32_t;

MOD getModifierBitmaskForVcode(int vcode);

unsigned short getModifierForBitmask(MOD bitmask);

bool isModifier(int vcode);

bool isRealModifier(int vcode);

bool isVirtualModifier(int vcode);

#define BITMASK_LSHIFT      0x001
#define BITMASK_LCTRL       0x002
#define BITMASK_LWIN        0x004
#define BITMASK_LALT        0x008
#define BITMASK_RSHIFT      0x010
#define BITMASK_RCTRL       0x020
#define BITMASK_RWIN        0x040
#define BITMASK_RALT        0x080
#define BITMASK_MOD9        0x100
#define BITMASK_MOD10       0x200
#define BITMASK_MOD11       0x400
#define BITMASK_MOD12       0x800
#define BITMASK_MOD13       0x1000
#define BITMASK_MOD14       0x2000
#define BITMASK_MOD15       0x4000
#define BITMASK_MOD16       0x8000
#define BITMASK_MOD17       0x10000
#define BITMASK_MOD18       0x20000
#define BITMASK_MOD19       0x40000
#define BITMASK_MOD20       0x80000
#define BITMASK_MOD21       0x100000
#define BITMASK_MOD22       0x200000
#define BITMASK_MOD23       0x400000
#define BITMASK_MOD24       0x800000
#define BITMASK_MOD25       0x1000000
#define BITMASK_MOD26       0x2000000
#define BITMASK_MOD27       0x4000000
#define BITMASK_MOD28       0x8000000
#define BITMASK_MOD29       0x10000000
#define BITMASK_MOD30       0x20000000
#define BITMASK_MOD31       0x40000000
#define BITMASK_MOD32       0x80000000

#define IS_SHIFT_DOWN (modifierState.modifierDown & BITMASK_LSHIFT || modifierState.modifierDown & BITMASK_RSHIFT)
#define IS_LSHIFT_DOWN (modifierState.modifierDown & BITMASK_LSHIFT)
#define IS_RSHIFT_DOWN (modifierState.modifierDown & BITMASK_RSHIFT)
#define IS_LCTRL_DOWN (modifierState.modifierDown & BITMASK_LCTRL)
#define IS_RCTRL_DOWN (modifierState.modifierDown & BITMASK_RCTRL)
#define IS_LALT_DOWN (modifierState.modifierDown & BITMASK_LALT)
#define IS_RALT_DOWN (modifierState.modifierDown & BITMASK_RALT)
#define IS_LWIN_DOWN (modifierState.modifierDown & BITMASK_LWIN)
#define IS_RWIN_DOWN (modifierState.modifierDown & BITMASK_RWIN)
#define IS_MOD9_DOWN (modifierState.modifierDown & BITMASK_MOD9)
#define IS_MOD10_DOWN (modifierState.modifierDown & BITMASK_MOD10)
#define IS_MOD11_DOWN (modifierState.modifierDown & BITMASK_MOD11)
#define IS_MOD12_DOWN (modifierState.modifierDown & BITMASK_MOD12)
#define IS_MOD13_DOWN (modifierState.modifierDown & BITMASK_MOD13)
#define IS_MOD14_DOWN (modifierState.modifierDown & BITMASK_MOD14)
#define IS_MOD15_DOWN (modifierState.modifierDown & BITMASK_MOD15)
#define IS_MOD16_DOWN (modifierState.modifierDown & BITMASK_MOD16)
#define IS_MOD17_DOWN (modifierState.modifierDown & BITMASK_MOD17)
#define IS_MOD18_DOWN (modifierState.modifierDown & BITMASK_MOD18)
#define IS_MOD19_DOWN (modifierState.modifierDown & BITMASK_MOD19)
#define IS_MOD20_DOWN (modifierState.modifierDown & BITMASK_MOD20)
#define IS_MOD21_DOWN (modifierState.modifierDown & BITMASK_MOD21)
#define IS_MOD22_DOWN (modifierState.modifierDown & BITMASK_MOD22)
#define IS_MOD23_DOWN (modifierState.modifierDown & BITMASK_MOD23)
#define IS_MOD24_DOWN (modifierState.modifierDown & BITMASK_MOD24)
#define IS_MOD25_DOWN (modifierState.modifierDown & BITMASK_MOD25)
#define IS_MOD26_DOWN (modifierState.modifierDown & BITMASK_MOD26)
#define IS_MOD27_DOWN (modifierState.modifierDown & BITMASK_MOD27)
#define IS_MOD28_DOWN (modifierState.modifierDown & BITMASK_MOD28)
#define IS_MOD29_DOWN (modifierState.modifierDown & BITMASK_MOD29)
#define IS_MOD30_DOWN (modifierState.modifierDown & BITMASK_MOD30)
#define IS_MOD31_DOWN (modifierState.modifierDown & BITMASK_MOD31)
#define IS_MOD32_DOWN (modifierState.modifierDown & BITMASK_MOD32)


#endif