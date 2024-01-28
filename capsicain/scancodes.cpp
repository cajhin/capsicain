#pragma once
#include "pch.h"
#include <iostream>
#include <Windows.h>  //for Sleep()

#include "scancodes.h"
#include "constants.h"
#include "utils.h"

using namespace std;

//returns -1 if label is not found, or if given SC_ string is not a number,
//otherwise an 8bit scancode (can cast to unsigned char without checks)
int getVcode(string label, string* arr)
{
    string ucLabel = stringToUpper(label);
    for (int i = 0; i < MAX_VCODES; i++)
    {
        if (arr[i] == ucLabel)
            return i;
    }
    return -1;
}

void checkAddLabel(int index, string label, string arr[])
{
    if (index >= MAX_VCODES)
    {
        cout << endl << endl <<"***********BUG: scancodes.h defines a virtual code "<< index << " that is bigger than MAX_VKEYS. Increase MAX_VKEYS !";
        Sleep(10000);
        exit(1);
    }
    if (arr[index] != "")
        cout << endl << "ERROR initScancodeLabels: duplicate scancode in scancodes.h: 'SC_" << label << "' (index 0x" << hex << index << ")" << endl;
    else
        arr[index] = label;
}

//List of all 'pretty' key labels that can be used in the .ini
//Map scancodes via scancode labels to key labels that are used in the .ini
//GENERATED from scancodes.h then modified - SC_EQUALS becomes '=' for example.
//All undefined scancodes get a generic label "SC_0XNN" (where NN is a hex value).
//to re-sync automatically, run this regex in Notepad++ over scancodes.h enum:
//from:  (.*)SC_(.*?) (.*)
//to:    checkAddLabel\( SC_\2, "\2", arr\);
//then make a bunch of modifications for nicer labels...
void defineAllPrettyVKLabels(string arr[])
{
    checkAddLabel(SC_NOP, "NOP", arr);
    checkAddLabel(SC_ESCAPE, "ESC", arr);
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
    checkAddLabel(SC_MINUS, "-", arr);
    checkAddLabel(SC_EQUALS, "=", arr);
    checkAddLabel(SC_BACK, "BSP", arr);
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
    checkAddLabel(SC_LBRACK, "[", arr);
    checkAddLabel(SC_RBRACK, "]", arr);
    checkAddLabel(SC_RETURN, "RET", arr);
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
    checkAddLabel(SC_SEMI, ";", arr);
    checkAddLabel(SC_APOS, "'", arr);
    checkAddLabel(SC_GRAVE, "`", arr);
    checkAddLabel(SC_LSHIFT, "LSHF", arr);
    checkAddLabel(SC_BSLASH, "\\", arr);
    checkAddLabel(SC_Z, "Z", arr);
    checkAddLabel(SC_X, "X", arr);
    checkAddLabel(SC_C, "C", arr);
    checkAddLabel(SC_V, "V", arr);
    checkAddLabel(SC_B, "B", arr);
    checkAddLabel(SC_N, "N", arr);
    checkAddLabel(SC_M, "M", arr);
    checkAddLabel(SC_COMMA, ",", arr);
    checkAddLabel(SC_DOT, ".", arr);
    checkAddLabel(SC_SLASH, "/", arr);
    checkAddLabel(SC_RSHIFT, "RSHF", arr);
    checkAddLabel(SC_NPMULT, "NP*", arr);
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
    checkAddLabel(SC_NPSUB, "NP-", arr);
    checkAddLabel(SC_NP4, "NP4", arr);
    checkAddLabel(SC_NP5, "NP5", arr);
    checkAddLabel(SC_NP6, "NP6", arr);
    checkAddLabel(SC_NPADD, "NP+", arr);
    checkAddLabel(SC_NP1, "NP1", arr);
    checkAddLabel(SC_NP2, "NP2", arr);
    checkAddLabel(SC_NP3, "NP3", arr);
    checkAddLabel(SC_NP0, "NP0", arr);
    checkAddLabel(SC_NPDOT, "NP.", arr);
    checkAddLabel(SC_ALTPRINT, "ALTPRINT", arr);
    checkAddLabel(SC_LBSLASH, "L\\", arr);
    checkAddLabel(SC_F11, "F11", arr);
    checkAddLabel(SC_F12, "F12", arr);
    checkAddLabel(SC_NPEQUALS1, "NP=", arr);
    checkAddLabel(SC_F13, "F13", arr);
    checkAddLabel(SC_F14, "F14", arr);
    checkAddLabel(SC_F15, "F15", arr);
    checkAddLabel(SC_F16, "F16", arr);
    checkAddLabel(SC_F17, "F17", arr);
    checkAddLabel(SC_F18, "F18", arr);
    checkAddLabel(SC_F19, "F19", arr);
    checkAddLabel(SC_F20, "F20", arr);
    checkAddLabel(SC_F21, "F21", arr);
    checkAddLabel(SC_F22, "F22", arr);
    checkAddLabel(SC_F23, "F23", arr);
    checkAddLabel(SC_KANA, "KANA", arr);
    checkAddLabel(SC_LANG2, "LANG2", arr);
    checkAddLabel(SC_LANG1, "LANG1", arr);
    checkAddLabel(SC_ABNT_C1, "ABNT_C1", arr);
    checkAddLabel(SC_CONVERT, "CONVERT", arr);
    checkAddLabel(SC_NOCONVERT, "NOCONVERT", arr);
    checkAddLabel(SC_YEN, "YEN", arr);
    checkAddLabel(SC_ABNT_C2, "ABNT_C2", arr);

    //poorly standardized special keys > 0x80, escaped with E0 (IKS state 2/3)
    checkAddLabel(SC_NPEQUALS2, "NPEQUALS2", arr);
    checkAddLabel(SC_PREVTRACK, "PREVTRACK", arr);
    checkAddLabel(SC_AT, "AT", arr);
    checkAddLabel(SC_COLON, "COLON", arr);
    checkAddLabel(SC_UNDERLINE, "UNDERLINE", arr);
    checkAddLabel(SC_KANJI, "KANJI", arr);
    checkAddLabel(SC_STOP, "STOP", arr);
    checkAddLabel(SC_AX, "AX", arr);
    checkAddLabel(SC_UNLABELED, "UNLABELED", arr);
    checkAddLabel(SC_NEXTTRACK, "NEXTTRACK", arr);
    checkAddLabel(SC_NPRET, "NPRET", arr);
    checkAddLabel(SC_RCTRL, "RCTRL", arr);
    checkAddLabel(SC_MUTE, "MUTE", arr);
    checkAddLabel(SC_CALCULATOR, "CALCULATOR", arr);
    checkAddLabel(SC_PLAYPAUSE, "PLAYPAUSE", arr);
    checkAddLabel(SC_MEDIASTOP, "MEDIASTOP", arr);
    checkAddLabel(SC_E0LSHF, "E0LSHF", arr);
    checkAddLabel(SC_VOLUMEDOWN, "VOLUMEDOWN", arr);
    checkAddLabel(SC_VOLUMEUP, "VOLUMEUP", arr);
    checkAddLabel(SC_WEBHOME, "WEBHOME", arr);
    checkAddLabel(SC_NUMPADCOMMA, "NP,", arr);
    checkAddLabel(SC_DIVIDE, "NP/", arr);
    checkAddLabel(SC_PRINT, "PRINT", arr);
    checkAddLabel(SC_RALT, "RALT", arr);
    checkAddLabel(SC_BREAK, "SC_BREAK", arr);
    checkAddLabel(SC_HOME, "HOME", arr);
    checkAddLabel(SC_UP, "UP", arr);
    checkAddLabel(SC_PGUP, "PGUP", arr);
    checkAddLabel(SC_LEFT, "LEFT", arr);
    checkAddLabel(SC_RIGHT, "RIGHT", arr);
    checkAddLabel(SC_END, "END", arr);
    checkAddLabel(SC_DOWN, "DOWN", arr);
    checkAddLabel(SC_PGDOWN, "PGDOWN", arr);
    checkAddLabel(SC_INSERT, "INS", arr);
    checkAddLabel(SC_DELETE, "DEL", arr);
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

    //fill all undefined scancodes with "SC_0XNN" so they can be referenced in ini
    char buffer[9];
    for (int i = 0; i <= 255; i++)
    {
        if (arr[i] == "")
        {
            snprintf(buffer, 9, "SC_0X%02X", i);
            arr[i].assign(buffer, 7);
        }
    }

    //define all Capsicain VK Virtual Keys >= 0x100
    //Capsicain virtual modifiers:
    //checkAddLabel(VK_CPS_ESC, "CPS_ESC", arr);  //only for logging; do not use this in ini
    checkAddLabel(VK_CPS_TEMPRELEASEKEYS, "TEMPRELEASEKEYS", arr);
    checkAddLabel(VK_CPS_TEMPRESTOREKEYS, "TEMPRESTOREKEYS", arr);
    checkAddLabel(VK_CPS_SLEEP, "SLEEP", arr);
    checkAddLabel(VK_CPS_DEADKEY, "DEADKEY", arr);
    checkAddLabel(VK_CPS_CONFIGSWITCH, "CONFIGSWITCH", arr);
    checkAddLabel(VK_CPS_CONFIGPREVIOUS, "CONFIGPREVIOUS", arr);
    checkAddLabel(VK_MOD9, "MOD9", arr);
    checkAddLabel(VK_MOD10, "MOD10", arr);
    checkAddLabel(VK_MOD11, "MOD11", arr);
    checkAddLabel(VK_MOD12, "MOD12", arr);
    checkAddLabel(VK_MOD13, "MOD13", arr);
    checkAddLabel(VK_MOD14, "MOD14", arr);
    checkAddLabel(VK_MOD15, "MOD15", arr);
    checkAddLabel(VK_CPS_CAPSON, "CAPSON", arr);
    checkAddLabel(VK_CPS_CAPSOFF, "CAPSOFF", arr);
    checkAddLabel(VK_CPS_RECORDMACRO, "RECMAC", arr);
    checkAddLabel(VK_CPS_RECORDSECRETMACRO, "RECSECMACRO", arr);
    checkAddLabel(VK_CPS_PLAYMACRO, "PLAYMAC", arr);
    checkAddLabel(VK_CPS_OBFUSCATED_SEQUENCE_START, "OBFUSEQSTART", arr);
    checkAddLabel(VK_CPS_PAUSE, "PAUSE", arr);  //this is not a real scancode, used to map the PAUSE combo E1 LCTRL SCRLCK
    checkAddLabel(VK_CPS_HOLDKEY, "HOLDKEY", arr);
    checkAddLabel(VK_CPS_HOLDMOD, "HOLDMOD", arr);
    checkAddLabel(VK_CPS_RELEASEKEYS, "RELEASEKEYS", arr);
    checkAddLabel(VK_CPS_DELAY, "DELAY", arr);
/* testing the VMK style config shift
    checkAddLabel(VK_SHFCFG0, "SHFCFG0", arr);
    checkAddLabel(VK_SHFCFG1, "SHFCFG1", arr);
    checkAddLabel(VK_SHFCFG2, "SHFCFG2", arr);
    checkAddLabel(VK_SHFCFG3, "SHFCFG3", arr);
    checkAddLabel(VK_SHFCFG4, "SHFCFG4", arr);
    checkAddLabel(VK_SHFCFG5, "SHFCFG5", arr);
    checkAddLabel(VK_SHFCFG6, "SHFCFG6", arr);
    checkAddLabel(VK_SHFCFG7, "SHFCFG7", arr);
    checkAddLabel(VK_SHFCFG8, "SHFCFG8", arr);
    checkAddLabel(VK_SHFCFG9, "SHFCFG9", arr);
*/

}
