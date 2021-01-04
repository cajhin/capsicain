#pragma once
#include <string>

bool IsCapsicainInTray();
bool ShowInTaskbar();
bool ShowInTaskbarMinimized();
bool ShowInTraybar(bool enabled, int activeConfig);
void updateTrayIcon(bool enabled, int activeConfig);