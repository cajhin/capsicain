#pragma once
#include <string>
#include <chrono>

void raise_process_priority(void);
std::string startProgramSameFolder(std::string path);
void closeOrKillProgram(std::string processName);
bool parseConfig(std::vector<std::string> &config);
bool configHasKey(std::string section, std::string key, std::vector<std::string> iniLines);
int configReadInt(std::string section, std::string key, int & value, std::vector<std::string> iniLines);
unsigned int millisecondsSinceTimepoint(std::chrono::steady_clock::time_point timepoint);
std::chrono::steady_clock::time_point timepointNow();
std::string startProgram(std::string processname, std::string dir);
