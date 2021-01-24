// bootstrapped with helpful pointers from 
//https://www.codeguru.com/cpp/w-p/system/keyboard/article.php/c2825/Manipulating-the-Keyboard-Lights-in-Windows-NT.htm

#include "pch.h"
#include <windows.h>
#include <winioctl.h>
#include "led.h"
#include <iostream>
#include "capsicain.h"
#include "scancodes.h"

//set the actual LED
int ledSendCommand(HANDLE hKeyboard, UINT ledKeySC, bool ledOn)
{
    KEYBOARD_INDICATOR_PARAMETERS InputBuffer;    // Input buffer for DeviceIoControl
    ULONG               DataLength = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
    ULONG               ReturnedLength; // Number of bytes returned in output buffer

    //LED query always returns 0 for UnitId 0 - the HP laptop keyboard has no LED indicators.
    //// Preserve current indicators' state
    //KEYBOARD_INDICATOR_PARAMETERS OutputBuffer;   // Output buffer for DeviceIoControl
    //if (!DeviceIoControl(hKeyboard, IOCTL_KEYBOARD_QUERY_INDICATORS,
    //    &InputBuffer, DataLength,
    //    &OutputBuffer, DataLength,
    //    &ReturnedLength, NULL))
    //    return GetLastError();

    //check the actual state of all LED keys, because keyboard0 may not have LEDs (always returns 0)
    int ledBitmask = 0;
    if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0)
        ledBitmask |= LED_BITMASK_CAPS;
    if ((GetKeyState(VK_SCROLL) & 0x0001) != 0)
        ledBitmask |= LED_BITMASK_SCRLOCK;
    if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0)
        ledBitmask |= LED_BITMASK_NUMLOCK;

    //modify with the requested LED
    int requestedBitmask = 0;
    if (ledKeySC == SC_CAPS)
        requestedBitmask = LED_BITMASK_CAPS;
    else if (ledKeySC == SC_SCRLOCK)
        requestedBitmask = LED_BITMASK_SCRLOCK;
    else if (ledKeySC == SC_NUMLOCK)
        requestedBitmask = LED_BITMASK_NUMLOCK;
    else
        return false;

    // Real mask to be set
    InputBuffer.LedFlags = ledOn ? ledBitmask | requestedBitmask : ledBitmask & ~requestedBitmask;
    IFTRACE std::cout << std::endl << "LED out:" << std::hex << InputBuffer.LedFlags;

    InputBuffer.UnitId = 0;
    if (!DeviceIoControl(hKeyboard, IOCTL_KEYBOARD_SET_INDICATORS,
        &InputBuffer, DataLength,
        NULL, 0, &ReturnedLength, NULL))
        return GetLastError();

    return 0;
}

//toggles the specified LED on all attached keyboards
//possible LEDs:  SC_CAPS, SC_SCRLOCK, SC_NUMLOCK
bool WINAPI setLED(UINT ledKeySC, bool ledOn)
{
    UINT nDevices = 0;
    PRAWINPUTDEVICELIST pRawInputDeviceList;
    UINT dlSize = sizeof(RAWINPUTDEVICELIST);

    //get a list of all hardware devices
    if (GetRawInputDeviceList(NULL, &nDevices, dlSize) != 0)
    {
        std::cout << std::endl << "ERROR: GetRawInputDeviceList() found no keyboards";
        return false;
    }
    pRawInputDeviceList = (RAWINPUTDEVICELIST*)malloc((size_t)dlSize * nDevices);
    if (pRawInputDeviceList == NULL)
    {
        std::cout << std::endl << "ERROR: Cannot malloc for device list";
        return false;
    }
    GetRawInputDeviceList(pRawInputDeviceList, &nDevices,
        sizeof(RAWINPUTDEVICELIST));

    for (UINT devNum = 0; devNum < nDevices; devNum++)
    {
        if (pRawInputDeviceList[devNum].dwType == RIM_TYPEKEYBOARD)
        {
            std::cout << std::endl << devNum << " = keyboard";

            char DeviceName[256] = "";
            unsigned int DeviceNameLength = 256;

            GetRawInputDeviceInfo(pRawInputDeviceList[devNum].hDevice, RIDI_DEVICENAME, NULL, &DeviceNameLength);
            GetRawInputDeviceInfo(pRawInputDeviceList[devNum].hDevice, RIDI_DEVICENAME, DeviceName, &DeviceNameLength);
            std::cout << std::endl << devNum << "-" << DeviceName;

            HANDLE hKeyboard = CreateFileA(DeviceName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hKeyboard == INVALID_HANDLE_VALUE)
            {
                std::cout << std::endl << "Invalid handle, cannot open keyboard";
                continue;
            }            

            int res = ledSendCommand(hKeyboard, ledKeySC, ledOn);
            if (res != 0)
                std::cout << std::endl << "Error: cannot set LED state: " << res;

            CloseHandle(hKeyboard);
        }
    }

    // after the job, free the RAWINPUTDEVICELIST
    free(pRawInputDeviceList);

    return true;
}

