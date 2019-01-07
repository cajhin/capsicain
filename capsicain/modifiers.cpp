#pragma once
#include "pch.h"

#include "modifiers.h"

unsigned short modifierToBitmask[2][NUMBER_OF_MODIFIERS] =
{
    {SC_LSHIFT, SC_LCTRL, SC_LALT, SC_LWIN,
    SC_RSHIFT, SC_RCTRL, SC_RALT, SC_RWIN,
    SC_MOD9, SC_MOD10, SC_MOD11, SC_MOD12,
    SC_MOD13, SC_MOD14, SC_MOD15} ,

    {BITMASK_LSHIFT, BITMASK_LCTRL, BITMASK_LALT, BITMASK_LWIN,
    BITMASK_RSHIFT, BITMASK_RCTRL, BITMASK_RALT, BITMASK_RWIN,
    BITMASK_MOD9, BITMASK_MOD10, BITMASK_MOD11, BITMASK_MOD12,
    BITMASK_MOD13, BITMASK_MOD14, BITMASK_MOD15}
};

//returns 0 if not a modifier
unsigned short getBitmaskForModifier(unsigned short modifier)
{
    for (int i = 0; i < NUMBER_OF_MODIFIERS; i++)
        if (modifierToBitmask[0][i] == modifier)
            return modifierToBitmask[1][i];
    return 0;
}
unsigned short getModifierForBitmask(unsigned short bitmask)
{
    for (int i = 0; i < NUMBER_OF_MODIFIERS; i++)
        if (modifierToBitmask[1][i] == bitmask)
            return modifierToBitmask[0][i];
    return 0;
}

bool isModifier(unsigned short scancode)
{
    return (getBitmaskForModifier(scancode) == 0 ? false : true);
}

bool isModifierDown(unsigned short modifier, unsigned short modState)
{
    for (int i = 0; i < NUMBER_OF_MODIFIERS; i++)
    {
        if (modifierToBitmask[0][i] == modifier)
            return (modifierToBitmask[1][i] & modState) > 0;
        else
            break;
    }
    return false;
}