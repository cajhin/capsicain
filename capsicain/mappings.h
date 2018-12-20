#pragma once
#include "scancodes.h"

#ifndef MAPPINGS_H
#define MAPPINGS_H

const int NUMBER_OF_MODIFIERS = 12;

void flipZY(unsigned short &scancode);

unsigned short getBitmaskForModifier(unsigned short modifier);

bool isModifier(unsigned short scancode);

bool isModifierDown(unsigned short modifier, unsigned short modState);

#define BITMASK_LSHIFT		0x001
#define BITMASK_LCTRL		0x002
#define BITMASK_LALT		0x004
#define BITMASK_LWIN		0x008
#define BITMASK_RSHIFT		0x010
#define BITMASK_RCTRL		0x020
#define BITMASK_RALT		0x040
#define BITMASK_RWIN		0x080
#define BITMASK_CAPS		0x100
#define BITMASK_TAB			0x200
#define BITMASK_ESC			0x400
#define BITMASK_MOD12		0x800


#define IS_SHIFT_DOWN (state.modifiers & 0x01 || state.modifiers & 0x10)
#define IS_LSHIFT_DOWN (state.modifiers & 0x01)
#define IS_RSHIFT_DOWN (state.modifiers & 0x10)
#define IS_LCONTROL_DOWN (state.modifiers & 0x02)
#define IS_LALT_DOWN (state.modifiers & 0x04)
#define IS_LWIN_DOWN (state.modifiers & 0x08)
#define IS_CAPS_DOWN (state.modifiers & BITMASK_CAPS)


#endif