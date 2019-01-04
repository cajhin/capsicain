#pragma oncce
#include "pch.h"
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <windows.h>
#include <tlhelp32.h>

#include "config.h"
#include "capsicain.h"
#include "utils.h"
#include "scancodes.h"
#include "modifiers.h"

using namespace std;

//cut comments, tab to space, trim, single blanks, lowercase
void normalizeLine(string &line)
{
	auto idxComment = line.find_first_of(';');
	if (string::npos != idxComment)
		line.erase(idxComment);

	replace(line.begin(), line.end(), '\t', ' ');

	line.erase(0, line.find_first_not_of(' '));
	line.erase(line.find_last_not_of(' ') + 1);

	std::string::size_type pos;
	while ((pos = line.find("  ")) != std::string::npos)
		line.replace(pos, 2, " ");

	std::transform(line.begin(), line.end(), line.begin(), ::tolower);
}

bool parseConfig(vector<string> &config)
{
	string line;
	int numlines = 0;
	ifstream f("capsicain.ini");
	if (!f.is_open())
		return false;
	while (getline(f, line)) {
		normalizeLine(line);
		if (line == "")
			continue;
		if (line.substr(0, 1) == "["  && numlines > 0)
			config.push_back("[SECTIONEND]");
		config.push_back(line);
		numlines++;
	}
	if (f.bad())
	{
		cout << "error while reading .ini file";
		return false;
	}
	else
		config.push_back("[SECTIONEND]");

	return true;
}

bool parseConfigSection(string sectionName, vector<string> &iniLinesInSection)
{
	string sectName = stringToLower(sectionName);
	string line;
	bool inSection = false;
	ifstream f("capsicain.ini");
	if (!f.is_open())
		return false;
	while (getline(f, line))
	{
		normalizeLine(line);
		if (line == "")
			continue;
		if (stringStartsWith(line, "[" + sectName + "]"))
		{
			inSection = true;
			continue;
		}
		if (inSection)
		{
			if (stringStartsWith(line, "["))
				break;
			iniLinesInSection.push_back(line);
		}
	}
	if (f.bad())
	{
		cout << "error while reading .ini file";
		return false;
	}
	return inSection;
}

bool configHasKey(string section, string key, vector<string> iniLines)
{
	bool inSection = false;
	std::transform(section.begin(), section.end(), section.begin(), ::tolower);
	for (string line : iniLines)
	{
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);
		if (line == section)
			inSection = true;
		if (inSection && line == "[ENDSECTION]")
			return false;
		if (line == key)
			return true;
	}
	return false;
}

bool configReadString(string section, string key, std::string &value, vector<string> iniLines)
{
	bool inSection = false;
	std::transform(section.begin(), section.end(), section.begin(), tolower);
	key += " ";
	for (string line : iniLines)
	{
		std::transform(key.begin(), key.end(), key.begin(), tolower);
		if (line == section)
			inSection = true;
		if (inSection && line == "[ENDSECTION]")
			return false;
		if (line.compare(0, key.length(), key) == 0)
		{
			value = line.substr(line.find_last_of(' ') + 1);
			return true;
		}
	}
	return false;
}

bool configReadInt(string section, string key, int &value, vector<string> iniLines)
{
	string val;
	if (!configReadString(section, key, val, iniLines))
		return false;
	try
	{
		value = stoi(val);
	}
	catch (...)
	{
		cout << endl << "Error: not a number: " << val;
		return false;
	}
	return true;
}

//ini parsing
std::string stringGetFirstToken(std::string line)
{
	return line.substr(0, line.find_first_of(" [>"));
}
std::string stringGetLastToken(std::string line)
{
	return line.substr(line.find_last_of(' ') + 1);
}

// parse "A B"
bool parseSimpleMapping(std::string line, unsigned char &keyIn, unsigned char &keyOut, std::string scLabels[])
{
	string a = stringToUpper(stringGetFirstToken(line));
	string b = stringToUpper(stringGetLastToken(line));
	keyIn = getScancode(a, scLabels);
	keyOut = getScancode(b, scLabels);
	if ((keyIn == 0 && a != "nop") || (keyOut == 0 && b != "nop"))
		return false; //invalid key label
	return true;
}

// parse "A B C"
bool parseThreeTokenMapping(std::string line, unsigned char &keyA, unsigned char &keyB, unsigned char &keyC, std::string scLabels[])
{
	vector<string> labels = stringSplit(line, ' ');
	if (labels.size() != 3)
		return false;

	keyA = getScancode(labels[0], scLabels);
	keyB = getScancode(labels[1], scLabels);
	keyC = getScancode(labels[2], scLabels);
	if ( (keyA == 0 && labels[0] != "nop") ||
		 (keyB == 0 && labels[1] != "nop") ||
		 (keyC == 0 && labels[2] != "nop")
		)
		return false; //invalid key label
	return true;
}

//convert ("xy^_v.", 'v') to 000010
unsigned short parseModString(string modString, char filter)
{
	string binString = "";
	for (int i = 0; i < modString.length(); i++)
	{
		if (modString[i] == filter)
			binString += '1';
		else
			binString += '0';
	}
	return std::stoi(binString, nullptr, 2);
}

bool parseCombo(std::string &funcParams, std::string * scLabels, std::vector<KeyEvent> &strokeSeq)
{
	vector<string> labels = stringSplit(funcParams, '+');
	unsigned short sc;
	for (string label : labels)
	{
		sc = getScancode(label, scLabels);
		if (sc == 0)
			return false;
		strokeSeq.push_back({ sc, true });
	}
	size_t len = strokeSeq.size();
	for (size_t i = len; i > 0; i--)	//copy upstrokes in reverse order
		strokeSeq.push_back({ strokeSeq.at(i - 1).scancode,false });
	return true;
}

//parse H  [^^^v .--. ....] > function(param)
bool parseModCombo(std::string line, unsigned short &key, unsigned short(&mods)[3], std::vector<KeyEvent> &strokeSequence, std::string scLabels[])
{
	line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

	string strkey = stringGetFirstToken(line);
	if (strkey.length() < 1)
		return false;
	unsigned short tmpKey = getScancode(strkey, scLabels);
	if (tmpKey == 0)
		return false;
	key = tmpKey;

	size_t modIdx1 = line.find_first_of('[') + 1;
	size_t modIdx2 = line.find_first_of(']');
	if (modIdx1 < 2 || modIdx2 < 3 || modIdx1 > modIdx2)
		return false;
	string mod = line.substr(modIdx1, modIdx2 - modIdx1);

	mods[0] = parseModString(mod, 'v'); //and 
	mods[1] = parseModString(mod, '!'); //not 
	mods[2] = parseModString(mod, 't'); //tap
//	mods[3] = parseModString(mod, '-'); //nop
//	mods[4] = parseModString(mod, '.'); //for

	//extract function name + param
	size_t funcIdx1 = line.find_first_of('>') + 1;
	if (funcIdx1 < 2)
		return false;
	size_t funcIdx2 = line.find_first_of('(');
	if (funcIdx2 < funcIdx1 + 2)
		return false;
	size_t funcIdx3 = line.find_first_of(')');
	if (funcIdx3 < funcIdx2 + 2)
		return false;
	string funcName = line.substr(funcIdx1, funcIdx2 - funcIdx1);
	funcIdx2++;
	string funcParams = line.substr(funcIdx2, funcIdx3 - funcIdx2);

	//translate 'function' into a char sequence
	vector<KeyEvent> strokeSeq;
	if (funcName == "key")
	{
		unsigned short sc = getScancode(funcParams, scLabels);
		if (sc == 0)
			return false;
		strokeSeq.push_back({ sc, true });
		strokeSeq.push_back({ sc, false });
	}
	else if (funcName == "combo")
	{
		if (!parseCombo(funcParams, scLabels, strokeSeq))
			return false;
	}
	else if (funcName == "combontimes")
	{
		vector<string> comboTimes = stringSplit(funcParams, ',');
		if (comboTimes.size() != 2)
			return false;
		if (!parseCombo(comboTimes.at(0), scLabels, strokeSeq))
			return false;
		int times = stoi(comboTimes.at(1));
		auto len = strokeSeq.size();
		for (int j = 1; j < times; j++)
			for (int i = 0; i < len; i++)
				strokeSeq.push_back(strokeSeq.at(i));
	}
	else if (funcName == "altchar")
	{
		strokeSeq.push_back({ SC_CPS_ESC, true }); //temp release LSHIFT if it is currently down
		strokeSeq.push_back({ BITMASK_RALT , true });
		strokeSeq.push_back({ BITMASK_LSHIFT | BITMASK_RSHIFT | BITMASK_LCTRL | BITMASK_RCTRL , false });
		strokeSeq.push_back({ SC_CPS_ESC, false });
		for (int i = 0; i < funcParams.length(); i++)
		{
			char c = funcParams[i];
			if (c < '0' || c > '9')
				return false;
			string temp = "NP";
			temp += c;
			unsigned short sc = getScancode(temp, scLabels);
			strokeSeq.push_back({ sc, true });
			strokeSeq.push_back({ sc, false });
		}
		strokeSeq.push_back({ SC_CPS_ESC, false });
	}
	else if (funcName == "moddedkey")
	{
		vector<string> modKeyParams = stringSplit(funcParams, ',');
		if (modKeyParams.size() != 2)
			return false;
		unsigned short sc = getScancode(modKeyParams[0], scLabels);
		if (sc == SC_NOP)
			return false;

		unsigned short modsPress = parseModString(modKeyParams[1], 'v'); //and (press if up)
		unsigned short modsRelease = parseModString(modKeyParams[1], '!'); //not (release if down)

		strokeSeq.push_back({ SC_CPS_ESC, true });
		if (modsPress > 0)
			strokeSeq.push_back({ modsPress, true });
		if (modsRelease > 0)
			strokeSeq.push_back({ modsRelease, false });
		strokeSeq.push_back({ SC_CPS_ESC, false });
		strokeSeq.push_back({ sc, true });
		strokeSeq.push_back({ sc, false });
		strokeSeq.push_back({ SC_CPS_ESC, false }); //second UP does UNDO the temp mod changes
	}
	else if (funcName == "type")
	{

	}
	else
		return false;

	strokeSequence = strokeSeq;
	return true;
}