#pragma once
#include <string>
#include <vector>

struct Stroke
{
	unsigned short scancode = 0;
	bool isDownstroke = true;
};

bool parseConfig(std::vector<std::string> &config);
bool parseConfigSection(std::string sectionName, std::vector<std::string>& config);
bool configHasKey(std::string section, std::string key, std::vector<std::string> iniLines);
bool configReadString(std::string section, std::string key, std::string & value, std::vector<std::string> iniLines);
bool configReadInt(std::string section, std::string key, int & value, std::vector<std::string> iniLines);
bool parseCombo(std::string &funcParams, std::string * scLabels, std::vector<Stroke> &strokeSeq, bool &retflag);
bool parseModCombo(std::string line, unsigned short &key, unsigned short(&mods)[3], std::vector<Stroke> &strokeSequence, std::string scLabels[]);
