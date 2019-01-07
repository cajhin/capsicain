#pragma once
#include "scancodes.h"

#ifndef MAPPINGS_H
#define MAPPINGS_H

const int NUMBER_OF_MODIFIERS = 15;

unsigned short getBitmaskForModifier(unsigned short modifier);

unsigned short getModifierForBitmask(unsigned short bitmask);

bool isModifier(unsigned short scancode);

bool isModifierDown(unsigned short modifier, unsigned short modState);

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


#define IS_SHIFT_DOWN (globalState.modifiers & BITMASK_LSHIFT || globalState.modifiers & BITMASK_RSHIFT)
#define IS_LSHIFT_DOWN (globalState.modifiers & BITMASK_LSHIFT)
#define IS_RSHIFT_DOWN (globalState.modifiers & BITMASK_RSHIFT)
#define IS_LCTRL_DOWN (globalState.modifiers & BITMASK_LCTRL)
#define IS_RCTRL_DOWN (globalState.modifiers & BITMASK_RCTRL)
#define IS_LALT_DOWN (globalState.modifiers & BITMASK_LALT)
#define IS_RALT_DOWN (globalState.modifiers & BITMASK_RALT)
#define IS_LWIN_DOWN (globalState.modifiers & BITMASK_LWIN)
#define IS_RWIN_DOWN (globalState.modifiers & BITMASK_RWIN)
#define IS_MOD9_DOWN (globalState.modifiers & BITMASK_MOD9)
#define IS_MOD10_DOWN (globalState.modifiers & BITMASK_MOD10)
#define IS_MOD11_DOWN (globalState.modifiers & BITMASK_MOD11)
#define IS_MOD12_DOWN (globalState.modifiers & BITMASK_MOD12)
#define IS_MOD13_DOWN (globalState.modifiers & BITMASK_MOD13)
#define IS_MOD14_DOWN (globalState.modifiers & BITMASK_MOD14)
#define IS_MOD15_DOWN (globalState.modifiers & BITMASK_MOD15)


#endif