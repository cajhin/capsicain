#pragma once
#include <string>
#include <chrono>
#include <vector>

struct Stroke 
{
	unsigned short scancode = 0;
	bool downstroke = true;
};

void raise_process_priority(void);
std::string startProgramSameFolder(std::string path);
void closeOrKillProgram(std::string processName);
bool parseConfig(std::vector<std::string> &config);
bool parseConfigSection(std::string sectionName, std::vector<std::string>& config);
bool configHasKey(std::string section, std::string key, std::vector<std::string> iniLines);
int configReadInt(std::string section, std::string key, int & value, std::vector<std::string> iniLines);
unsigned int millisecondsSinceTimepoint(std::chrono::steady_clock::time_point timepoint);
std::chrono::steady_clock::time_point timepointNow();
bool stringStartsWith(std::string haystack, std::string needle);
std::string stringGetLastToken(std::string line);
bool parseCombo(std::string &funcParams, std::string * scLabels, std::vector<Stroke> &strokeSeq, bool &retflag);
bool parseModCombo(std::string line, unsigned short &key, unsigned short (&mods)[5], std::vector<Stroke> &strokeSequence, std::string scLabels[]);
std::string stringGetFirstToken(std::string line);
std::string stringToLower(std::string str);
std::string stringToUpper(std::string str);
std::vector<std::string> stringSplit(const std::string &line, char delimiter);
std::string startProgram(std::string processname, std::string dir);
