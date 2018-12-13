#include <string>
#include <filesystem>
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>

#include "capsicain.h"
#include "utils.h"

using namespace std;

void raise_process_priority(void)
{
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

void close_single_program(void *program_instance) {
    CloseHandle(program_instance);
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


string startProgram(string processname, string dir)
{
	string ret = "";
	string path = dir + "\\" + processname;
	if (0xffffffff == GetFileAttributes(path.c_str()))
	{
		ret = "file/path does not exist: " + processname;
	}
	else if (FindProcessId(processname))
	{
		ret = "already running: " + processname;
	}
	else
	{
		ShellExecuteA(NULL, "open", processname.c_str(), NULL, dir.c_str(), SW_SHOWDEFAULT);
	}
	return ret;
}

string startProgram(string processname)
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, sizeof(buffer));
	string::size_type pos = string(buffer).find_last_of("\\/");
	string homedir = (string(buffer).substr(0, pos));
	return startProgram(processname, homedir);
}