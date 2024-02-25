#pragma once
#include "pch.h"

#include "modifiers.h"

//stores {SC_LSHIFT/42 = 00001b},{VK_LCTRL = 010b}, ... {VK_MOD15, 100000000000000b}
MOD modifierToBitmask[2][NUMBER_OF_MODIFIERS] =
{
    {SC_LSHIFT, SC_LCTRL, SC_LALT, SC_LWIN,
    SC_RSHIFT, SC_RCTRL, SC_RALT, SC_RWIN,
    VK_MOD9, VK_MOD10, VK_MOD11, VK_MOD12,
    VK_MOD13, VK_MOD14, VK_MOD15, VK_MOD16,
    VK_MOD17, VK_MOD18, VK_MOD19, VK_MOD20,
    VK_MOD21, VK_MOD22, VK_MOD23, VK_MOD24,
    VK_MOD25, VK_MOD26, VK_MOD27, VK_MOD28,
    VK_MOD29, VK_MOD30, VK_MOD31, VK_MOD32},

    {BITMASK_LSHIFT, BITMASK_LCTRL, BITMASK_LALT, BITMASK_LWIN,
    BITMASK_RSHIFT, BITMASK_RCTRL, BITMASK_RALT, BITMASK_RWIN,
    BITMASK_MOD9, BITMASK_MOD10, BITMASK_MOD11, BITMASK_MOD12,
    BITMASK_MOD13, BITMASK_MOD14, BITMASK_MOD15, BITMASK_MOD16,
    BITMASK_MOD17, BITMASK_MOD18, BITMASK_MOD19, BITMASK_MOD20,
    BITMASK_MOD21, BITMASK_MOD22, BITMASK_MOD23, BITMASK_MOD24,
    BITMASK_MOD25, BITMASK_MOD26, BITMASK_MOD27, BITMASK_MOD28,
    BITMASK_MOD29, BITMASK_MOD30, BITMASK_MOD31, BITMASK_MOD32}
};

//returns 0 if vcode is not a modifier
MOD getModifierBitmaskForVcode(int vcode)
{
    if (vcode < 0)
        return 0;

    for (int i = 0; i < NUMBER_OF_MODIFIERS; i++)
        if (modifierToBitmask[0][i] == vcode)
            return modifierToBitmask[1][i];
    return 0;
}
unsigned short getModifierForBitmask(MOD bitmask)
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
    MOD bitmask = getModifierBitmaskForVcode(vcode);
    return ((bitmask & 0xFF) > 0);
}
bool isVirtualModifier(int vcode)
{
    MOD bitmask = getModifierBitmaskForVcode(vcode);
    return ((bitmask & 0x7F00) > 0);
}
