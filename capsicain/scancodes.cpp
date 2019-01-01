#pragma once
#include "pch.h"
#include <iostream>

#include "scancodes.h"
#include "utils.h"

using namespace std;

unsigned char getScancode(string label, string* arr)
{
	string ucLabel = stringToUpper(label);
	for (int i = 0; i < 256; i++)
	{
		if (arr[i] == ucLabel)
			return i;
	}
	return 0;
}

void checkAddLabel(int index, string label, string arr[])
{
	if (arr[index] != "")
		cout << endl << "ERROR initScancodeLabels: duplicate scancode in scancodes.h: 'SC_" << label << "' (index 0x" << hex << index << ")" << endl;
	else
		arr[index] = label;
}

//GENERATED
//this must be in sync with config.h
//to sync automatically, run this regex in Notepad++ over scancodes.h enum:
//from:  (.*)SC_(.*?) (.*)
//to:    checkAddLabel\( SC_\2, "\2", arr\);
void initScancodeLabels(string arr[])
{
	checkAddLabel(SC_NOP, "NOP", arr);
	checkAddLabel(SC_ESCAPE, "ESCAPE", arr);
	checkAddLabel(SC_1, "1", arr);
	checkAddLabel(SC_2, "2", arr);
	checkAddLabel(SC_3, "3", arr);
	checkAddLabel(SC_4, "4", arr);
	checkAddLabel(SC_5, "5", arr);
	checkAddLabel(SC_6, "6", arr);
	checkAddLabel(SC_7, "7", arr);
	checkAddLabel(SC_8, "8", arr);
	checkAddLabel(SC_9, "9", arr);
	checkAddLabel(SC_0, "0", arr);
	checkAddLabel(SC_MINUS, "MINUS", arr);
	checkAddLabel(SC_EQUALS, "EQUALS", arr);
	checkAddLabel(SC_BACK, "BACK", arr);
	checkAddLabel(SC_TAB, "TAB", arr);
	checkAddLabel(SC_Q, "Q", arr);
	checkAddLabel(SC_W, "W", arr);
	checkAddLabel(SC_E, "E", arr);
	checkAddLabel(SC_R, "R", arr);
	checkAddLabel(SC_T, "T", arr);
	checkAddLabel(SC_Y, "Y", arr);
	checkAddLabel(SC_U, "U", arr);
	checkAddLabel(SC_I, "I", arr);
	checkAddLabel(SC_O, "O", arr);
	checkAddLabel(SC_P, "P", arr);
	checkAddLabel(SC_LBRACK, "LBRACK", arr);
	checkAddLabel(SC_RBRACK, "RBRACK", arr);
	checkAddLabel(SC_RETURN, "RETURN", arr);
	checkAddLabel(SC_LCTRL, "LCTRL", arr);
	checkAddLabel(SC_A, "A", arr);
	checkAddLabel(SC_S, "S", arr);
	checkAddLabel(SC_D, "D", arr);
	checkAddLabel(SC_F, "F", arr);
	checkAddLabel(SC_G, "G", arr);
	checkAddLabel(SC_H, "H", arr);
	checkAddLabel(SC_J, "J", arr);
	checkAddLabel(SC_K, "K", arr);
	checkAddLabel(SC_L, "L", arr);
	checkAddLabel(SC_SEMI, "SEMI", arr);
	checkAddLabel(SC_APOS, "APOS", arr);
	checkAddLabel(SC_GRAVE, "GRAVE", arr);
	checkAddLabel(SC_LSHIFT, "LSHIFT", arr);
	checkAddLabel(SC_BSLASH, "BSLASH", arr);
	checkAddLabel(SC_Z, "Z", arr);
	checkAddLabel(SC_X, "X", arr);
	checkAddLabel(SC_C, "C", arr);
	checkAddLabel(SC_V, "V", arr);
	checkAddLabel(SC_B, "B", arr);
	checkAddLabel(SC_N, "N", arr);
	checkAddLabel(SC_M, "M", arr);
	checkAddLabel(SC_COMMA, "COMMA", arr);
	checkAddLabel(SC_DOT, "DOT", arr);
	checkAddLabel(SC_SLASH, "SLASH", arr);
	checkAddLabel(SC_RSHIFT, "RSHIFT", arr);
	checkAddLabel(SC_MULT, "MULT", arr);
	checkAddLabel(SC_LALT, "LALT", arr);
	checkAddLabel(SC_SPACE, "SPACE", arr);
	checkAddLabel(SC_CAPS, "CAPS", arr);
	checkAddLabel(SC_F1, "F1", arr);
	checkAddLabel(SC_F2, "F2", arr);
	checkAddLabel(SC_F3, "F3", arr);
	checkAddLabel(SC_F4, "F4", arr);
	checkAddLabel(SC_F5, "F5", arr);
	checkAddLabel(SC_F6, "F6", arr);
	checkAddLabel(SC_F7, "F7", arr);
	checkAddLabel(SC_F8, "F8", arr);
	checkAddLabel(SC_F9, "F9", arr);
	checkAddLabel(SC_F10, "F10", arr);
	checkAddLabel(SC_NUMLOCK, "NUMLOCK", arr);
	checkAddLabel(SC_SCRLOCK, "SCRLOCK", arr);
	checkAddLabel(SC_NP7, "NP7", arr);
	checkAddLabel(SC_NP8, "NP8", arr);
	checkAddLabel(SC_NP9, "NP9", arr);
	checkAddLabel(SC_NPSUB, "NPSUB", arr);
	checkAddLabel(SC_NP4, "NP4", arr);
	checkAddLabel(SC_NP5, "NP5", arr);
	checkAddLabel(SC_NP6, "NP6", arr);
	checkAddLabel(SC_NPADD, "NPADD", arr);
	checkAddLabel(SC_NP1, "NP1", arr);
	checkAddLabel(SC_NP2, "NP2", arr);
	checkAddLabel(SC_NP3, "NP3", arr);
	checkAddLabel(SC_NP0, "NP0", arr);
	checkAddLabel(SC_NPDOT, "NPDOT", arr);
	checkAddLabel(SC_LBSLASH, "LBSLASH", arr);
	checkAddLabel(SC_F11, "F11", arr);
	checkAddLabel(SC_F12, "F12", arr);
	checkAddLabel(SC_F13, "F13", arr);
	checkAddLabel(SC_F14, "F14", arr);
	checkAddLabel(SC_F15, "F15", arr);
	checkAddLabel(SC_MOD12, "MOD12", arr); //CAPSICAIN virtual modifiers
	checkAddLabel(SC_MOD13, "MOD13", arr);
	checkAddLabel(SC_MOD14, "MOD14", arr);
	checkAddLabel(SC_MOD15, "MOD15", arr);
	checkAddLabel(SC_MOD16, "MOD16", arr);
	checkAddLabel(SC_KANA, "KANA", arr);
	checkAddLabel(SC_ABNT_C1, "ABNT_C1", arr);
	checkAddLabel(SC_CONVERT, "CONVERT", arr);
	checkAddLabel(SC_NOCONVERT, "NOCONVERT", arr);
	checkAddLabel(SC_YEN, "YEN", arr);
	checkAddLabel(SC_ABNT_C2, "ABNT_C2", arr);
	checkAddLabel(SC_NPEQU, "NPEQU", arr);
	checkAddLabel(SC_PREVTRACK, "PREVTRACK", arr);
	checkAddLabel(SC_AT, "AT", arr);
	checkAddLabel(SC_COLON, "COLON", arr);
	checkAddLabel(SC_UNDERLINE, "UNDERLINE", arr);
	checkAddLabel(SC_KANJI, "KANJI", arr);
	checkAddLabel(SC_STOP, "STOP", arr);
	checkAddLabel(SC_AX, "AX", arr);
	checkAddLabel(SC_UNLABELED, "UNLABELED", arr);
	checkAddLabel(SC_NEXTTRACK, "NEXTTRACK", arr);
	checkAddLabel(SC_NPENT, "NPENT", arr);
	checkAddLabel(SC_RCTRL, "RCONTROL", arr);
	checkAddLabel(SC_MUTE, "MUTE", arr);
	checkAddLabel(SC_CALCULATOR, "CALCULATOR", arr);
	checkAddLabel(SC_PLAYPAUSE, "PLAYPAUSE", arr);
	checkAddLabel(SC_MEDIASTOP, "MEDIASTOP", arr);
	checkAddLabel(SC_TWOSUPERIOR, "TWOSUPERIOR", arr);
	checkAddLabel(SC_VOLUMEDOWN, "VOLUMEDOWN", arr);
	checkAddLabel(SC_VOLUMEUP, "VOLUMEUP", arr);
	checkAddLabel(SC_WEBHOME, "WEBHOME", arr);
	checkAddLabel(SC_NUMPADCOMMA, "NUMPADCOMMA", arr);
	checkAddLabel(SC_DIVIDE, "DIVIDE", arr);
	checkAddLabel(SC_SYSRQ, "SYSRQ", arr);
	checkAddLabel(SC_RALT, "RALT", arr);
	checkAddLabel(SC_PAUSE, "PAUSE", arr);
	checkAddLabel(SC_HOME, "HOME", arr);
	checkAddLabel(SC_UP, "UP", arr);
	checkAddLabel(SC_PGUP, "PGUP", arr);
	checkAddLabel(SC_LEFT, "LEFT", arr);
	checkAddLabel(SC_RIGHT, "RIGHT", arr);
	checkAddLabel(SC_END, "END", arr);
	checkAddLabel(SC_DOWN, "DOWN", arr);
	checkAddLabel(SC_PGDOWN, "PGDOWN", arr);
	checkAddLabel(SC_INSERT, "INSERT", arr);
	checkAddLabel(SC_DELETE, "DELETE", arr);
	checkAddLabel(SC_LWIN, "LWIN", arr);
	checkAddLabel(SC_RWIN, "RWIN", arr);
	checkAddLabel(SC_APPS, "APPS", arr);
	checkAddLabel(SC_POWER, "POWER", arr);
	checkAddLabel(SC_SLEEP, "SLEEP", arr);
	checkAddLabel(SC_WAKE, "WAKE", arr);
	checkAddLabel(SC_WEBSEARCH, "WEBSEARCH", arr);
	checkAddLabel(SC_WEBFAVORITES, "WEBFAVORITES", arr);
	checkAddLabel(SC_WEBREFRESH, "WEBREFRESH", arr);
	checkAddLabel(SC_WEBSTOP, "WEBSTOP", arr);
	checkAddLabel(SC_WEBFORWARD, "WEBFORWARD", arr);
	checkAddLabel(SC_WEBBACK, "WEBBACK", arr);
	checkAddLabel(SC_MYCOMPUTER, "MYCOMPUTER", arr);
	checkAddLabel(SC_MAIL, "MAIL", arr);
	checkAddLabel(SC_MEDIASELECT, "MEDIASELECT", arr);
	checkAddLabel(SC_CPS_ESC, "CPS_ESC", arr);
}
