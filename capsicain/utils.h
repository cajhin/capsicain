#pragma once
#include <string>

void raise_process_priority(void);
std::string startProgramSameFolder(std::string path);
void closeOrKillProgram(std::string processName);
bool parseConfig(std::vector<std::string> &config);
std::string startProgram(std::string processname, std::string dir);
