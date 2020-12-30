#pragma once
#include <string>

bool IsCapsicainInTray();
bool ShowInTaskbar();
bool ShowInTaskbarMinimized();
bool ShowInTraybar(bool enabled, int activeLayer);
void updateTrayIcon(bool enabled, int activeLayer);