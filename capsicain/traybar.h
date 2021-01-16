#pragma once
#include <string>
#include <windows.h> 

bool IsCapsicainForegroundWindow();
bool IsCapsicainInTray();
bool ShowInTaskbar();
bool ShowInTaskbarMinimized();
bool ShowInTraybar(bool enabled, bool recording, int activeConfig);
void updateTrayIcon(bool enabled, bool recording, int activeConfig);