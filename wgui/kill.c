// Copyright (C) 2000 Core Technologies.

// This file is part of e93.
//
// e93 is free software; you can redistribute it and/or modify
// it under the terms of the e93 LICENSE AGREEMENT.
//
// e93 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// e93 LICENSE AGREEMENT for more details.
//
// You should have received a copy of the e93 LICENSE AGREEMENT
// along with e93; see the file "LICENSE.TXT".


#include "includes.h"

#include <tlhelp32.h>

#include "psapi.h"
#include "ntddk.h"

static char
	localErrorFamily[]="Kill";

enum
{
	NOPSAPI,
	NOTOOLHELP32,
	NONTQUERYINFORMATIONPROCESS,
	MISSINGTOOLHELP32FUNCTIONS,
	INFANTICIDEFAILED
};

static char *errorMembers[]=
{
	"NoPSAPI",
	"NoToolHelp32",
	"NoNtQueryInformationProcess",
	"MissingToolHelp32Functions",
	"InfanticideFailed"
};

static char *errorDescriptions[]=
{
	"PSAPI.DLL is required to kill processes under Windows NT, but was not available",
	"ToolHelp32 is required to kill processes, but was not available",
	"The NtQueryInformationProcess() function is required to kill processes under Windows NT, but was not available",
	"Some ToolHelp32 functions, which are required to kill processes under Windows, were not available",
	"Failed to kill a child process"
};

static char
	errorString[512];	// need a place to sprintf error messages
                              

// kill a single process, but not its children
// (the only way Windows knows how without lots of help, see below)
static bool TerminatePID(DWORD pid)
{
	bool
		TermSucc = false;
	HANDLE
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);

	if (hProcess != NULL)
	{
		TermSucc = TerminateProcess(hProcess, 0) != FALSE;
		CloseHandle(hProcess);
	}

	if (!TermSucc)
	{
		sprintf(errorString,"Failed to kill process pid: %d", pid);
		SetError(localErrorFamily,"KillFailed", errorString);
	}
	
	return TermSucc;
}


//
// kill a process and all of its child processes under Windows NT
//
static bool InfanticideNT(DWORD pid)
{
	HINSTANCE
		hPSApiLib,
		hInstLib;
	bool
		result = false;

	// PSAPI is distributed with the NT Resource Kit and may be redisributed, so we'll do that
	if ((hPSApiLib = LoadLibraryA("PSAPI.DLL")) != NULL)
	{
		// PSAPI Function Pointers.
		BOOL (WINAPI *lpfEnumProcesses)(DWORD *, DWORD, DWORD *);
		
		// Get procedure addresses.
		//
		// We are linking to these functions of PSAPI
		// explicitly, because otherwise a module using
		// this code would fail to load under Windows 98/2000/ME,
		// which does not have the NTDLL functions.
		lpfEnumProcesses = (BOOL (WINAPI *)(DWORD *, DWORD, DWORD *)) GetProcAddress(hPSApiLib, "EnumProcesses");

		// NTDLL.DLL is part of NT. However, the NtQueryInformationProcess that's in it is not officially suported by MS.
		// It is however the only way to do this.
		//
		// You have to get the MS DDK to get the headers and the library.
		// You can glean what NtQueryInformationProcess does from the NTDDK.H header.
		//
		// Unfortunaly, the DDK NTDDK.H header cannot be used unmodified with a Windows SDK app, so a modified one is included as NTDDK.H.
		//
		// We can't link to the the NTDDK.LIB library anyhow, because if we did we couldn't load under versions of Windows other than NT.
		//
		// Because this function is only required under NT, we can use it safely with NT 4 where we know it exists.
		// Future version don't have it, but also don't need it. (we have ToolHelp32, see below)
		//
		// However, we must load the DLL manually and use GetProcAddress() to find the function
		// so we can still load under other versions of Windows that don't have NTDLL.DLL.
		if ((hInstLib = LoadLibraryA("NTDLL.DLL")) != NULL)
		{
			// NTDLL Function Pointers.
			int (NTAPI *lpfNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
			
			// Get procedure addresses.
			//
			// We are linking to these functions of NTDLL
			// explicitly, because otherwise a module using
			// this code would fail to load under Windows 98/2000/ME,
			// which do not have NTDLL.DLL or its functions.
			lpfNtQueryInformationProcess = (int (NTAPI *)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG)) GetProcAddress(hInstLib, "NtQueryInformationProcess");

			if (lpfNtQueryInformationProcess != NULL)
			{
				DWORD
					aProcesses[1024],
					cbNeeded;
				PROCESS_BASIC_INFORMATION
					pbi;
			    	
				// Get the list of process IDs
			    if (lpfEnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
				{
				    // Calculate how many process IDs were returned
				    DWORD
				    	cProcesses = cbNeeded / sizeof(DWORD);
					
					// assume all is well, we might have no children
					result = true;
					
					// Get the process information for each ID
					for (unsigned i = 0; i < cProcesses; i++)
					{
						DWORD
							targetID = aProcesses[i];
						HANDLE
							hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetID);

						if (hProcess)    // OpenProcess did not fail, so get the process info for this process
						{
						    DWORD
						    	retLen;

						    // Zero the input buffer that NtQueryInformationProcess sees
						    memset(&pbi, 0, sizeof(pbi));

						    if (lpfNtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &retLen) >= 0) // NtQueryInformationProcess returns a negative on failure
						    {
				     			// if this process is our (pid's) direct decendent, recurse
						    	if (pbi.InheritedFromUniqueProcessId == pid)
						    	{
						    		// it was, so kill it and its children
						    		if (!InfanticideNT(targetID))
						    		{
										result = false;
										SetError(localErrorFamily,errorMembers[INFANTICIDEFAILED],errorDescriptions[INFANTICIDEFAILED]);
						    		}
						    	}
						    }
						    CloseHandle(hProcess);
						}
					}
				}
			}
			else
			{
				SetError(localErrorFamily,errorMembers[NONTQUERYINFORMATIONPROCESS],errorDescriptions[NONTQUERYINFORMATIONPROCESS]);
			}
			// Free the library.
			FreeLibrary(hInstLib);
		}
		// Free the library.
		FreeLibrary(hPSApiLib);
	}
	else
	{
		SetError(localErrorFamily,errorMembers[NOPSAPI],errorDescriptions[NOPSAPI]);
	}
		
	// remember to kill pid on the way out
	return result && TerminatePID(pid);
}


//
// Kill a process and all of its child processes on versions of Windows other than NT 4
//
// Windows 95/98 had Toolhelp32 and it has now returned with Windows 2000, so only NT can't use this code
//
static bool Infanticide(DWORD pid)
{
	HINSTANCE
		hInstLib;
	bool
		result = false;

	if ((hInstLib = LoadLibraryA("Kernel32.DLL")) != NULL)
	{
		// ToolHelp Function Pointers.
		HANDLE (WINAPI *lpfCreateToolhelp32Snapshot)(DWORD, DWORD);
		BOOL (WINAPI *lpfProcess32First)(HANDLE, LPPROCESSENTRY32);
		BOOL (WINAPI *lpfProcess32Next)(HANDLE, LPPROCESSENTRY32);

		// Get procedure addresses.
		//
		// We are linking to these functions of Kernel32
		// explicitly, because otherwise a module using
		// this code would fail to load under Windows NT,
		// which does not have the Toolhelp32
		// functions in Kernel32.dll.
		lpfCreateToolhelp32Snapshot = (HANDLE(WINAPI *)(DWORD,DWORD)) GetProcAddress(hInstLib, "CreateToolhelp32Snapshot");
		lpfProcess32First = (BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32)) GetProcAddress(hInstLib, "Process32First");
		lpfProcess32Next = (BOOL(WINAPI *)(HANDLE,LPPROCESSENTRY32)) GetProcAddress(hInstLib, "Process32Next");

		if (lpfProcess32Next != NULL && lpfProcess32First != NULL && lpfCreateToolhelp32Snapshot != NULL)
		{
		     // Get a handle to a Toolhelp snapshot of the systems processes.
		     HANDLE
		     	hSnapShot = lpfCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		     if (hSnapShot != INVALID_HANDLE_VALUE)
		     {
			     PROCESSENTRY32
			     	procentry;

			     // Get the first process' information.
			     procentry.dwSize = sizeof(PROCESSENTRY32);

			     BOOL
			     	bFlag = lpfProcess32First(hSnapShot, &procentry);

				// assume all is well, we might have no children
				result = true;
				
			     // While there are processes, keep looping.
			     while (bFlag)
			     {
			     	// if this process is our (pid's) direct decendent, recurse
			     	if (procentry.th32ParentProcessID == pid)
			     	{
			    		// it was, so kill it and its children
			    		if (!Infanticide(procentry.th32ProcessID))
			    		{
							result = false;
							SetError(localErrorFamily,errorMembers[INFANTICIDEFAILED],errorDescriptions[INFANTICIDEFAILED]);
			    		}
			     	}
					procentry.dwSize = sizeof(PROCESSENTRY32);
					bFlag = lpfProcess32Next(hSnapShot, &procentry);
			    }
		     }
	     }
		else
		{
			SetError(localErrorFamily,errorMembers[MISSINGTOOLHELP32FUNCTIONS],errorDescriptions[MISSINGTOOLHELP32FUNCTIONS]);
		}
		// Free the library.
		FreeLibrary(hInstLib);
	}
	else
	{
		SetError(localErrorFamily,errorMembers[NOTOOLHELP32],errorDescriptions[NOTOOLHELP32]);
	}
		
	// remember to kill pid on the way out
	return result && TerminatePID(pid);
}


//
// Kill a process and all of its child processes
// in a manner that makes Windows behave more like
// a real OS written by people who have a clue
// (this is only a simulation of OS cluefullness)
//
bool kill(DWORD pid)
{
	OSVERSIONINFO
		osver;

	// check to see if were running under Windows NT
	osver.dwOSVersionInfoSize = sizeof(osver);

	if (GetVersionEx(&osver))
	{
		// if we're on Windows NT:
		if (osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			// call the NT specific deep kill function
			return InfanticideNT(pid);
		}
	}
	
	return Infanticide(pid);
}
