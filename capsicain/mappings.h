#pragma once
#include "scancodes.h"

#ifndef MAPPINGS_H
#define MAPPINGS_H

const int NUMBER_OF_MODIFIERS = 15;

void flipZY(unsigned short &scancode);

unsigned short getBitmaskForModifier(unsigned short modifier);

unsigned short getModifierForBitmask(unsigned short bitmask);

bool isModifier(unsigned short scancode);

bool isModifierDown(unsigned short modifier, unsigned short modState);

#define BITMASK_LSHIFT		0x001
#define BITMASK_LCTRL		0x002
#define BITMASK_LWIN		0x004
#define BITMASK_LALT		0x008
#define BITMASK_RSHIFT		0x010
#define BITMASK_RCTRL		0x020
#define BITMASK_RWIN		0x040
#define BITMASK_RALT		0x080
#define BITMASK_CAPS		0x100
#define BITMASK_TAB			0x200
#define BITMASK_ESC			0x400
#define BITMASK_MOD12		0x800
#define BITMASK_MOD13		0x1000
#define BITMASK_MOD14		0x2000
#define BITMASK_MOD15		0x4000


#define IS_SHIFT_DOWN (state.modifiers & BITMASK_LSHIFT || state.modifiers & BITMASK_RSHIFT)
#define IS_LSHIFT_DOWN (state.modifiers & BITMASK_LSHIFT)
#define IS_RSHIFT_DOWN (state.modifiers & BITMASK_RSHIFT)
#define IS_LCTRL_DOWN (state.modifiers & BITMASK_LCTRL)
#define IS_LALT_DOWN (state.modifiers & BITMASK_LALT)
#define IS_LWIN_DOWN (state.modifiers & BITMASK_LWIN)
#define IS_CAPS_DOWN (state.modifiers & BITMASK_CAPS)
#define IS_TAB_DOWN (state.modifiers & BITMASK_TAB)
#define IS_ESC_DOWN (state.modifiers & BITMASK_ESC)
#define IS_MOD12_DOWN (state.modifiers & BITMASK_12)
#define IS_MOD13_DOWN (state.modifiers & BITMASK_13)
#define IS_MOD14_DOWN (state.modifiers & BITMASK_14)
#define IS_MOD15_DOWN (state.modifiers & BITMASK_15)


#endif