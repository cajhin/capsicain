#pragma once
#include "pch.h"

#include "mappings.h"

//	`1234567890-=	~!@#$%^&*()_+
//	qwertzuiop[]	QWERTYUIOP{}
//	asdfghjkl;'\	ASDFGHJKL:"|
//	\zxcvbnm,.		|ZXCVBNM<>?
void flipZY(unsigned short &scancode)
{
	switch (scancode)
	{
	case SC_Y:		scancode = SC_Z;		break;
	case SC_Z:		scancode = SC_Y;		break;
	}
}

//	`1234567890-=	~!@#$%^&*()_+
//	qdrwbzfup;[]	QDRWBZFUP:{}
//	ashtgjneoi'\	ASHTGJNEOI"|
//	\yxmcvkl,./		|YXMCVKL<>?
//void map_Qwerty_WorkmanJ(unsigned short &scancode)


unsigned short modifierToBitmask[2][NUMBER_OF_MODIFIERS] =
{
	{SC_LSHIFT, SC_LCTRL, SC_LALT, SC_LWIN,
	SC_RSHIFT, SC_RCTRL, SC_RALT, SC_RWIN,
	SC_CAPS, SC_TAB, BITMASK_ESC, BITMASK_MOD12} ,

	{BITMASK_LSHIFT, BITMASK_LCTRL, BITMASK_LALT, BITMASK_LWIN,
	BITMASK_RSHIFT, BITMASK_RCTRL, BITMASK_RALT, BITMASK_RWIN,
	BITMASK_CAPS, BITMASK_TAB, BITMASK_ESC, BITMASK_MOD12}
};

//returns 0 if not a modifier
unsigned short getBitmaskForModifier(unsigned short modifier)
{
	for (int i = 0; i < NUMBER_OF_MODIFIERS; i++)
		if (modifierToBitmask[0][i] == modifier)
			return modifierToBitmask[1][i];
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