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
