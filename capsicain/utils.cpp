#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <windows.h>
#include <tlhelp32.h>

#include "capsicain.h"
#include "utils.h"
#include "scancodes.h"
#include "mappings.h"

using namespace std;

void raise_process_priority(void)
{
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

DWORD FindProcessId(string processName)
{
	char *procNameChar = &processName[0u];
	char* p = strrchr(procNameChar, '\\');
	if (p)
		procNameChar = p + 1;

	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	Process32First(processesSnapshot, &processInfo);
	if (!_stricmp(procNameChar, processInfo.szExeFile))
	{
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		if (!_stricmp(procNameChar, processInfo.szExeFile))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}


string startProgram(string processName, string dir)
{
	string ret = "";
	string path = dir + "\\" + processName;
	if (0xffffffff == GetFileAttributes(path.c_str()))
	{
		ret = "file/path does not exist: " + processName;
	}
	else if (FindProcessId(processName))
	{
		ret = "already running: " + processName;
	}
	else
	{
		ShellExecuteA(NULL, "open", processName.c_str(), NULL, dir.c_str(), SW_SHOWDEFAULT);
	}
	return ret;
}

string startProgramSameFolder(string processName)
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, sizeof(buffer));
	string::size_type pos = string(buffer).find_last_of("\\/");
	string homedir = (string(buffer).substr(0, pos));
	return startProgram(processName, homedir);
}

BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam)
{
	DWORD dwID;

	GetWindowThreadProcessId(hwnd, &dwID);

	if (dwID == (DWORD)lParam)
	{
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}

	return TRUE;
}

// Finds a process by its name ("theapp.exe")
// Sends a "Close please" msg, if it is still running it kills it.
void closeOrKillProgram(string processName)
{
	DWORD timeoutMS = 1000;

	DWORD pid = FindProcessId(processName);
	if (pid == 0)
	{
		cout << endl << "Cannot find/stop process: " << processName;
		return;
	}
	HANDLE   hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);


	// TerminateAppEnum() posts WM_CLOSE to all windows whose PID
	// matches your process's.
	EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)pid);


	int result = 1; // 0=fail; 1=close; 2=kill
	if (WaitForSingleObject(hProc, timeoutMS) != WAIT_OBJECT_0)
		result = (TerminateProcess(hProc, 0) ? 2 : 0);
	CloseHandle(hProc);

	if (result == 0)
		cout << " Cannot stop process: " << processName;
	else if (result == 1)
		cout << " Stopped.";
	else if (result == 2)
		cout << " Killed.";
}

void normalizeLine(string &line)
{
	replace(line.begin(), line.end(), '\t', ' ');
	auto idxComment = line.find_first_of(';');
	if(string::npos != idxComment)
		line.erase(idxComment);
	line.erase(0, line.find_first_not_of(' '));
	line.erase(line.find_last_not_of(' ') + 1);
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
		if (line.substr(0,1) == "["  && numlines>0)
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

bool parseConfigSection(string sectionName, vector<string> &config)
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
				return true;
			config.push_back(line);
		}
	}
	if (f.bad())
	{
		cout << "error while reading .ini file";
		return false;
	}
	return false;
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

int configReadInt(string section, string key, int &value, vector<string> iniLines)
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
			value = stoi(line.substr(line.find_last_of(' ') + 1));
			return true;
		}
	}
	return false;
}

unsigned int millisecondsSinceTimepoint(chrono::steady_clock::time_point timepoint)
{
	return (int)chrono::duration_cast<chrono::milliseconds>(std::chrono::steady_clock::now() - timepoint).count();
}
std::chrono::steady_clock::time_point timepointNow()
{
	return std::chrono::steady_clock::now();
}

//String stuff
bool stringStartsWith(string haystack, string needle)
{
	return (haystack.compare(0, needle.length(), needle) == 0);
}
std::string stringToLower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}
std::string stringToUpper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

std::vector<string> stringSplit(const std::string &line, char delimiter)
{
	vector<string> res;

	string s = line;
	std::replace(s.begin(), s.end(), delimiter, ' ');
	std::stringstream ss(s);
	std::string token;
	while (std::getline(ss, token, ' ')) 
		res.push_back(token);

	return res;
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

bool parseCombo(std::string &funcParams, std::string * scLabels, std::vector<Stroke> &strokeSeq)
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
		strokeSeq.push_back({ strokeSeq.at(i-1).scancode,false });
	return true;
}

//parse H  [^^^v .--. ....] > key(BACK)
bool parseModCombo(std::string line, unsigned short &key, unsigned short (&mods)[5], std::vector<Stroke> &strokeSequence, std::string scLabels[])
{
	line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

	string strkey = stringGetFirstToken(line);
	if (strkey.length() < 1)
		return false;
	unsigned short tmpKey = getScancode(strkey, scLabels);
	if (tmpKey == 0)
		return false;
	key = tmpKey;

	size_t modIdx1 = line.find_first_of('[') +1;
	size_t modIdx2 = line.find_first_of(']');
	if (modIdx1 < 2 || modIdx2 < 3 || modIdx1 > modIdx2)
		return false;
	string mod = line.substr(modIdx1, modIdx2 - modIdx1);

	mods[0] = parseModString(mod, 'v'); //and 
	mods[1] = parseModString(mod, '!'); //not 
	mods[2] = parseModString(mod, '-'); //nop
	mods[3] = parseModString(mod, '.'); //for
	mods[4] = parseModString(mod, 't'); //tap

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
	string funcName = line.substr(funcIdx1, funcIdx2-funcIdx1);
	funcIdx2++;
	string funcParams = line.substr(funcIdx2, funcIdx3-funcIdx2);

	//translate 'function' into a char sequence
	vector<Stroke> strokeSeq;
	if (funcName == "key")
	{
		unsigned short sc=getScancode(funcParams, scLabels);
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
		vector<string> comboTimes= stringSplit(funcParams, ',');
		if (comboTimes.size() != 2)
			return false;
		if (!parseCombo(comboTimes.at(0), scLabels, strokeSeq))
			return false;
		int times = stoi(comboTimes.at(1));
		auto len = strokeSeq.size();
		for(int j = 1;j<times;j++)
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
			unsigned short sc = getScancode(temp,scLabels);
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
		if(modsPress > 0)
			strokeSeq.push_back({ modsPress, true });
		if(modsRelease > 0)
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