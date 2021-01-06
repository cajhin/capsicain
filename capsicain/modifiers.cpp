#pragma once
#include "pch.h"

#include "modifiers.h"

//stores {SC_LSHIFT/42 = 00001b},{VK_LCTRL = 010b}, ... {VK_MOD15, 100000000000000b}
unsigned short modifierToBitmask[2][NUMBER_OF_MODIFIERS] =
{
    {SC_LSHIFT, SC_LCTRL, SC_LALT, SC_LWIN,
    SC_RSHIFT, SC_RCTRL, SC_RALT, SC_RWIN,
    VK_MOD9, VK_MOD10, VK_MOD11, VK_MOD12,
    VK_MOD13, VK_MOD14, VK_MOD15} ,

    {BITMASK_LSHIFT, BITMASK_LCTRL, BITMASK_LALT, BITMASK_LWIN,
    BITMASK_RSHIFT, BITMASK_RCTRL, BITMASK_RALT, BITMASK_RWIN,
    BITMASK_MOD9, BITMASK_MOD10, BITMASK_MOD11, BITMASK_MOD12,
    BITMASK_MOD13, BITMASK_MOD14, BITMASK_MOD15}
};

//returns 0 if vcode is not a modifier
unsigned short getModifierBitmaskForVcode(int vcode)
{
    if (vcode < 0)
        return 0;

    for (int i = 0; i < NUMBER_OF_MODIFIERS; i++)
        if (modifierToBitmask[0][i] == vcode)
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

bool isModifier(int vcode)
{
    if (vcode < 0)
        return false;

    return (getModifierBitmaskForVcode(vcode) == 0 ? false : true);
}

bool isRealModifier(int vcode)
{
    unsigned short bitmask = getModifierBitmaskForVcode(vcode);
    return ((bitmask & 0xFF) > 0);
}
bool isVirtualModifier(int vcode)
{
    unsigned short bitmask = getModifierBitmaskForVcode(vcode);
    return ((bitmask & 0x7F00) > 0);
}

bool isModifierDown(int vcode, unsigned short modState)
{
    for (int i = 0; i < NUMBER_OF_MODIFIERS; i++)
    {
        if (modifierToBitmask[0][i] == vcode)
            return (modifierToBitmask[1][i] & modState) > 0;
        else
            break;
    }
    return false;
}