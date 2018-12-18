#pragma once
#include "pch.h"

#ifndef MAPPINGS_H
#define MAPPINGS_H
#include "mappings.h"
#endif



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
