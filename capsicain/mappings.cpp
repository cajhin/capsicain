#include "pch.h"
#include "scancodes.h"
#include "mappings.h"


//	`1234567890-=	~!@#$%^&*()_+
//	qwertzuiop[]	QWERTYUIOP{}
//	asdfghjkl;'\	ASDFGHJKL:"|
//	\zxcvbnm,.		|ZXCVBNM<>?
void map_Qwerty_Qwertz(unsigned short &scancode)
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
void map_Qwerty_WorkmanJ(unsigned short &scancode)
{
	switch (scancode)
	{
	case SC_Q:		break;
	case SC_W:		scancode = SC_D;		break;
	case SC_E:		scancode = SC_R;		break;
	case SC_R:		scancode = SC_W;		break;
	case SC_T:		scancode = SC_B;		break;
	case SC_Y:		scancode = SC_Z;		break;
	case SC_U:		scancode = SC_F;		break;
	case SC_I:		scancode = SC_U;		break;
	case SC_O:		scancode = SC_P;		break;
	case SC_P:		scancode = SC_SEMI;		break;
	case SC_LBRACK:	case SC_RBRACK:		break;
	////
	case SC_A:		break;
	case SC_S:		break;	
	case SC_D:		scancode = SC_H;		break;
	case SC_F:		scancode = SC_T;		break;
	case SC_G:		break;
	case SC_H:		scancode = SC_J;		break;
	case SC_J:		scancode = SC_N;		break;
	case SC_K:		scancode = SC_E;		break;
	case SC_L:		scancode = SC_O;		break;
	case SC_SEMI:	scancode = SC_I;		break;
	case SC_APOS:	break;
	case SC_BSLASH:	break;
	/////
	case SC_LBSLASH: 		break;//ISO kb only
	case SC_Z:		scancode = SC_Y;		break;
	case SC_X:		break;
	case SC_C:		scancode = SC_M;		break;
	case SC_V:		scancode = SC_C;		break;
	case SC_B:		scancode = SC_V;		break;
	case SC_N:		scancode = SC_K;		break;
	case SC_M:		scancode = SC_L;		break;
	case SC_COMMA:		break;
	case SC_DOT:		break;
	case SC_SLASH:		break;
	}

}