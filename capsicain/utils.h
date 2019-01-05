#pragma once
#include <string>
#include <chrono>
#include <vector>

void raise_process_priority(void);
std::string startProgram(std::string processname, std::string dir);
std::string startProgramSameFolder(std::string path);
void closeOrKillProgram(std::string processName);

unsigned int millisecondsSinceTimepoint(std::chrono::steady_clock::time_point timepoint);
std::chrono::steady_clock::time_point timepointNow();

bool stringStartsWith(std::string haystack, std::string needle);
std::string stringGetLastToken(std::string line);
std::string stringGetFirstToken(std::string line);
std::string stringToLower(std::string str);
std::string stringToUpper(std::string str);
std::vector<std::string> stringSplit(const std::string &line, char delimiter);
