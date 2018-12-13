#pragma once
#include <string>

void raise_process_priority(void);
std::string startProgram(std::string path);
std::string startProgram(std::string processname, std::string dir);
