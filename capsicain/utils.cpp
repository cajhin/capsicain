#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <tlhelp32.h>

#include "capsicain.h"
#include "utils.h"

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
		if (stringStartsWith(line, "[" + sectionName + "]"))
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

bool stringStartsWith(string haystack, string needle)
{
	return (haystack.compare(0, needle.length(), needle) == 0);
}

std::string stringGetFirstToken(std::string line)
{
	return line.substr(0, line.find_first_of(' '));
}
std::string stringGetLastToken(std::string line)
{
	return line.substr(line.find_last_of(' ')+1);
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
