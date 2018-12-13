#pragma once
#include "pch.h"

#include "scancodes.h"

void initScanCodeLabel()
{
	SCL[SC_NOP] = "SC_NOP";  //unassigned, unknonw, NoOPeration
	SCL[SC_ESCAPE] = "SC_ESCAPE";
	SCL[SC_1] = "SC_1";
}