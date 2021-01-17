#pragma once
#include <string>
#include <chrono>
#include <vector>

void raise_process_priority(void);
void copyToClipBoard(std::string text);
std::string startProgram(std::string processname, std::string dir);
std::string startProgramSameFolder(std::string path);
void closeOrKillProgram(std::string processName);

unsigned long timeSinceTimepointMS(std::chrono::steady_clock::time_point timepoint);
unsigned long timeSinceTimepointUS(std::chrono::steady_clock::time_point timepoint);
std::chrono::steady_clock::time_point timeGetTimepointNow();
unsigned long timeBetweenTimepointsUS(std::chrono::steady_clock::time_point timepoint1, std::chrono::steady_clock::time_point timepoint2);

bool stringStartsWith(std::string haystack, std::string needle);
std::string stringGetLastToken(std::string line);
std::string stringGetRestBehindFirstToken(std::string line);
std::string stringCutFirstToken(std::string& line);
std::string stringCopyFirstToken(std::string line);
std::string stringToLower(std::string str);
std::string stringToUpper(std::string str);
std::vector<std::string> stringSplit(const std::string &line, char delimiter);
bool stringToInt(std::string strval, int& result);
bool stringReplace(std::string& haystack, const std::string& needle, const std::string& newneedle);
std::string stringIntToHex(const unsigned int i, unsigned int minLength);
