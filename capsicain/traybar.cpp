#include "pch.h"
#include "traybar.h"
#include <string>
#include <windows.h>
#include "resource.h"
#include <iostream>

bool IsWindowVisible()
{
    return ::IsWindowVisible(::GetConsoleWindow());
}

bool ShowTraybar(std::string tooltip)
{
    UINT nID = 11;
    LPCTSTR lpszTip = "Capsicain"; // tooltip.c_str();

    HMODULE handleToMyself = ::GetModuleHandleA(NULL);
    HICON hIcon = LoadIcon(handleToMyself, MAKEINTRESOURCE(IDI_ICON1));
    if (!hIcon || !lpszTip)
    {
        std::cout << "Internal error: cannot load traybar icons :(" << std::endl;
        return false;
    }

    ::NOTIFYICONDATA tnid;
    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = ::GetConsoleWindow();
    tnid.uID = nID;
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = 0;
    tnid.hIcon = hIcon;
    lstrcpy(tnid.szTip, lpszTip);

    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    return (::Shell_NotifyIcon(NIM_ADD, &tnid) ? true : false);
}

bool DeleteTraybar()
{
    UINT nID = 11;

    ::ShowWindow(::GetConsoleWindow(), SW_SHOW);
    ::NOTIFYICONDATA tnid;
    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = ::GetConsoleWindow();
    tnid.uID = nID;
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = 0;

    return (::Shell_NotifyIcon(NIM_DELETE, &tnid) ? true : false);
}
