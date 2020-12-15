#pragma once
#include <string>
#include <vector>

const int CPS_ESC_SEQUENCE_TYPE_TEMPALTERMODIFIERS = 1;
const int CPS_ESC_SEQUENCE_TYPE_SLEEP = 2;

struct KeyEvent
{
    unsigned short scancode = 0;
    bool isDownstroke = true;
};

bool readSanitizeIniFile(std::vector<std::string>& iniLines);

std::vector<std::string> getSectionFromIni(std::string sectionName, std::vector<std::string> iniContent);
std::vector<std::string> getTaggedLinesFromIni(std::string tag, std::vector<std::string> iniContent);
bool configHasKey(std::string key, std::vector<std::string> iniLines);
bool configHasTaggedKey(std::string tag, std::string key, std::vector<std::string> sectionLines);
bool getStringValueForTaggedKey(std::string tag, std::string key, std::string & value, std::vector<std::string> sectionLines);
bool getStringValueForKey(std::string key, std::string & value, std::vector<std::string> sectionLines);
bool getIntValueForTaggedKey(std::string tag, std::string key, int & value, std::vector<std::string> sectionLines);
bool getIntValueForKey(std::string key, int & value, std::vector<std::string> sectionLines);
bool lexRule(std::string line, unsigned short &key, unsigned short(&mods)[3], std::vector<KeyEvent> &strokeSequence, std::string scLabels[]);
bool lexAlphaFromTo(std::string mapFromTo, unsigned char(&alphamap)[256], std::string scLabels[]);
bool lexScancodeMapping(std::string line, unsigned char & keyA, unsigned char & keyB, unsigned char & keyC, std::string scLabels[]);
