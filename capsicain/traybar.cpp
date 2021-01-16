#include "pch.h"
#include "traybar.h"
#include <string>
#include <windows.h>
#include "resource.h"
#include <iostream>

const int TRAYBAR_UID = 11;

bool DeleteIconFromTraybar()
{
    ::NOTIFYICONDATA tnid;
    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = ::GetConsoleWindow();
    tnid.uID = TRAYBAR_UID;
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = 0;

    return (::Shell_NotifyIcon(NIM_DELETE, &tnid) ? true : false);
}

bool IsCapsicainForegroundWindow()
{
    return GetConsoleWindow() == GetForegroundWindow();
}
bool IsCapsicainVisible()
{
    return ::IsWindowVisible(::GetConsoleWindow());
}

bool IsCapsicainInTray()
{
    return !IsCapsicainVisible();  //could be better?
}

bool ShowInTaskbarMinimized()
{
    if (IsCapsicainInTray())
        DeleteIconFromTraybar();
    return ::ShowWindow(::GetConsoleWindow(), SW_MINIMIZE);
}

bool ShowInTaskbar()
{
    if (IsCapsicainInTray())
        DeleteIconFromTraybar();
    HWND myWindow = ::GetConsoleWindow();
    if (myWindow == NULL)
    {
        std::cout << std::endl << "BUG? GetConsoleWindow() failed";
        return false;
    }
    ShowWindow(myWindow, SW_RESTORE);
    SetForegroundWindow(myWindow);
    return true;
}


bool ShowInTraybar(bool enabled, bool recording, int activeConfig)
{
    LPCTSTR lpszTip = "Capsicain "; // tooltip.c_str();

    HMODULE handleToMyself = ::GetModuleHandleA(NULL);
    HICON hIcon;
    if( !enabled || activeConfig == 0)
        hIcon = LoadIcon(handleToMyself, MAKEINTRESOURCE(IDI_ICON_OFF));
    else if (recording)
        hIcon = LoadIcon(handleToMyself, MAKEINTRESOURCE(IDI_ICON_REC));
    else if (activeConfig >= 2 && activeConfig <= 9)
    {
        int dummy = (int)(102 + activeConfig); //MAKEINT macro ignores (int)
        hIcon = LoadIcon(handleToMyself, MAKEINTRESOURCE(dummy));
    }
    else
        hIcon = LoadIcon(handleToMyself, MAKEINTRESOURCE(IDI_ICON_ON));

    if (!hIcon || !lpszTip)
    {
        std::cout << "Internal error: cannot load traybar icons :(" << std::endl;
        return false;
    }

    ::NOTIFYICONDATA tnid;
    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = ::GetConsoleWindow();
    tnid.uID = TRAYBAR_UID;
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = 0;
    tnid.hIcon = hIcon;
    lstrcpy(tnid.szTip, lpszTip);

    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);

    bool res = (::Shell_NotifyIcon(NIM_ADD, &tnid) ? true : false);
    if(!res)
       res = (::Shell_NotifyIcon(NIM_MODIFY, &tnid) ? true : false);
    return res;
}

void updateTrayIcon(bool enabled, bool recording, int activeConfig)
{
    if (!IsCapsicainInTray())
        return;  //cannot change the taskbar icon

    ShowInTraybar(enabled, recording, activeConfig);
}
