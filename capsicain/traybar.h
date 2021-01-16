#pragma once
#include <string>

bool IsCapsicainInTray();
bool ShowInTaskbar();
bool ShowInTaskbarMinimized();
bool ShowInTraybar(bool enabled, bool recording, int activeConfig);
void updateTrayIcon(bool enabled, bool recording, int activeConfig);