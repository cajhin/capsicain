// bootstrapped with helpful pointers from 
//https://www.codeguru.com/cpp/w-p/system/keyboard/article.php/c2825/Manipulating-the-Keyboard-Lights-in-Windows-NT.htm

#pragma once
#include <windows.h>
//
// Define the keyboard indicators.
//
#define IOCTL_KEYBOARD_SET_INDICATORS        CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0002, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_TYPEMATIC       CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYBOARD_QUERY_INDICATORS      CTL_CODE(FILE_DEVICE_KEYBOARD, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define LED_BITMASK_CAPS 4
#define LED_BITMASK_SCRLOCK 1
#define LED_BITMASK_NUMLOCK 2

typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
    USHORT UnitId;       // Unit identifier 0..N for \Device\KeyboardPortN. Read always is 0 if Unit has no LEDs...
    USHORT LedFlags;     // LED indicator state.
} KEYBOARD_INDICATOR_PARAMETERS, * PKEYBOARD_INDICATOR_PARAMETERS;

bool WINAPI setLED(UINT ledKeySC, bool ledOn);
