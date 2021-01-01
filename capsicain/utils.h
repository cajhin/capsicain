#pragma once
#include <string>
#include <chrono>
#include <vector>

void raise_process_priority(void);
void copyToClipBoard(std::string text);
std::string startProgram(std::string processname, std::string dir);
std::string startProgramSameFolder(std::string path);
void closeOrKillProgram(std::string processName);

unsigned int timeMillisecondsSinceTimepoint(std::chrono::steady_clock::time_point timepoint);
std::chrono::steady_clock::time_point timeSetTimepointNow();
long timeBetweenTimepointsMS(std::chrono::steady_clock::time_point timepoint1, std::chrono::steady_clock::time_point timepoint2);

bool stringStartsWith(std::string haystack, std::string needle);
std::string stringGetLastToken(std::string line);
std::string stringGetRestBehindFirstToken(std::string line);
std::string stringGetFirstToken(std::string line);
std::string stringToLower(std::string str);
std::string stringToUpper(std::string str);
std::vector<std::string> stringSplit(const std::string &line, char delimiter);
bool stringToInt(std::string strval, int& result);
