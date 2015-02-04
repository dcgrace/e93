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

static bool
	hotKeyRegistered=false;
extern Tcl_Interp
	*theTclInterpreter;							/* pointer to TCL interpreter we are using */

void WaitForDraw(HWND hwnd)
/*	Do whatever is need to make sure any pending draw commands are done
 */
{
	HDC
		hdc;
		
	GdiFlush();				// make sure any pending draw commands are flushed
	hdc=GetDC(hwnd);
	GetPixel(hdc,0,0);		// this forces any draw commands to run and blocks until they finish
	ReleaseDC(hwnd,hdc);
}


UINT32 GetPerformanceCounter()
/*	Return the current tick count. Tick counter runs at 1,000,000 Hz
*/
{
	LARGE_INTEGER
		fTick,
		qTick;
	UINT32
		ticks;
				
	if(QueryPerformanceFrequency(&fTick))
	{
		QueryPerformanceCounter(&qTick);
		ticks=(UINT32)(((double)qTick.QuadPart/(double)fTick.QuadPart)*(double)1000000.0);
	}
	else
	{
		ticks=GetTickCount()*1000;		// if no Performance Counter, then use 1000Hz timer * 1000
	}
	return(ticks);
}

void RegisterControlBreakHotKey()
/*	Register the applications one hot key, if its not already registered
 */
{
	if(!hotKeyRegistered)
	{
		if(RegisterHotKey(NULL,1,0,VK_PAUSE))
		{
			hotKeyRegistered=true;
		}
	}
}

void UnregisterControlBreakHotKey()
/*	Unregister the applications one hot key, if it was registered
 */
{
	if(hotKeyRegistered)
	{
		UnregisterHotKey(NULL,1);	/* if we are de-activating, get rid of the hot key */
		hotKeyRegistered=false;
	}
}

void ConvertBSlashToFSlash(char *theStr)
/*	Passed a string that may contain back slashes, convert them to forward slashes.
 */
{
	int
		i;

	i=0;
	while(theStr[i])
	{
		if(theStr[i]=='\\')
		{
			theStr[i]='/';
		}
		i++;
	}
}

void ConvertFSlashToBSlash(char *theStr)
/*	Passed a string that may contain forward slashes, convert them to back slashes.
 */
{
	int
		i;

	i=0;
	while(theStr[i])
	{
		if(theStr[i]=='/')
		{
			theStr[i]='\\';
		}
		i++;
	}
}

void FreeStringList(char **theList)
/* free a list of strings created by calling NewStringList
 */
{
	UINT32
		theIndex;

	theIndex=0;
	while(theList&&theList[theIndex])
	{
		MDisposePtr(theList[theIndex]);
		theIndex++;
	}
	MDisposePtr(theList);
}

bool AddStringToList(char *theString,char ***theList,UINT32 *numElements)
/* add theString to theList
 * at any point, theList can be deleted by calling FreeStringList
 * if there is a problem, do not add the element, SetError, and return FALSE
 */
{
	char
		**newList;
	UINT32
		newLength;

	newLength=sizeof(char *)*((*numElements)+2);
	if(newList=(char **)MResizePtr(*theList,newLength))
	{
		(*theList)=newList;
		if((*theList)[*numElements]=(char *)MNewPtr(strlen(theString)+1))
		{
			strcpy((*theList)[(*numElements)++],theString);
			(*theList)[*numElements]=NULL;									/* keep the list terminated with a NULL */
			return(true);
		}
	}
	return(false);
}

char **NewStringList()
/* Create an empty string list
 * if there is a problem, SetError, return NULL
 */
{
	char
		**theList;

	if(theList=(char **)MNewPtr(sizeof(char *)))
	{
		theList[0]=NULL;
	}
	return(theList);
}

INT32 GetWindowsVersion()
/* Return what Windows OS we are running.
 */
{
	OSVERSIONINFO
		versionInformation;

	versionInformation.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&versionInformation);
	return(versionInformation.dwPlatformId);
}

char *GetEditorLocalVersion()
/* return a pointer to a constant string, that tells something about this local
 * version of the editor (used by the "version" command)
 * it is ok to return NULL
 */
{
	return(GUI_VERSION);
}

static char
	errorString[4096];	// need a place to sprintf error messages

void SetCommDlgError()
/* When an error occurs within a common dialog function, call this
 * and it will turn the system information into an extended library
 * error
 */
{
	sprintf(errorString, "Common dialog error %d", CommDlgExtendedError());
	SetError("Windows", "Windows", errorString);
}

void SetWindowsError()
/* When an error occurs within a system function, call this
 * and it will turn the system information into an extended library
 * error
 */
{
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,GetLastError(),
					MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
					errorString,sizeof(errorString),NULL);
					
	SetError("Windows","Windows", errorString);
}

void OutToDebugger(char *format,...)
/*
 *	Output the string to the debugger.
 *
 *	Input:	char *format,...			Varible argument printf style string
 *
 *	Output:	nothing
 *
 *	Possible Errors:	none
 */
{
	va_list
		args;

	va_start(args,format);
	vsprintf(errorString,format,args);			/* make the string */
	va_end(args);

	OutputDebugString(errorString);
}

int strcasecmp(const char *string1,const char *string2)
/* given 2 strings, compare them in a case insensitive way, return 0 if they
 * match, 1 otherwise
 */
{
	return(_stricmp(string1,string2));
}

bool MatchToken(char *theString,TOKENLIST *theList,int *theToken)
/* given a string, see if it matches any of the tokens in the token list, if so, return
 * its token number in theToken, otherwise, return FALSE
 * This should not be case sensitive.
 */
{
	int
		i;
	bool
		found,reachedEnd;

	found=reachedEnd=false;
	i=0;
	while(!found&&!reachedEnd)
	{
		if(theList[i].token[0]!='\0')
		{
			if(strcasecmp(theString,theList[i].token)==0)
			{
				found=true;
				*theToken=theList[i].tokenNum;
			}
			i++;
		}
		else
			reachedEnd=true;
	}
	return(found);
}

UINT32 CountNumberOfLineFeeds(char *theStr)
/*	Passed a string, return the number of linefeeds in the string
 */
{
	UINT32
		i,
		lineFeeds;

	i=0;
	lineFeeds=0;
	while(theStr[i]!='\0')
	{
		if(theStr[i]=='\n')
		{
			lineFeeds++;
		}
		i++;
	}
	return(lineFeeds);
}

UINT32 RemoveCRFromString(char *theStr)
/* passed a pointer to a string, remove all CR that are followed by LF from it
   Return number of characters
 */
{
	UINT32
		inIndex,
		outIndex;

	inIndex=0;
	outIndex=0;
	while(theStr[inIndex]!='\0')
	{
		if(theStr[inIndex]==0x0D && theStr[inIndex+1]=='\n')	/* if it is a CR followed by a LF */
		{
			inIndex++;											/* then skip over the CR */
		}
		else
		{
			theStr[outIndex++]=theStr[inIndex++];				/* else, just move the character */
		}
	}
	theStr[outIndex]='\0';
	return(outIndex);
}

void CopyStringAndAddCRToLF(char *srcPtr,char *dstPtr,UINT32 maxDst)
{
	UINT32
		inIndex,
		outIndex;

	inIndex=0;
	outIndex=0;
	while((outIndex<(maxDst-1))&&(srcPtr[inIndex]!='\0'))
	{
		if(srcPtr[inIndex]=='\n')
		{
			dstPtr[outIndex++]=0x0D;
		}
		dstPtr[outIndex++]=srcPtr[inIndex++];
	}
	dstPtr[outIndex]='\0';
}

bool NextStringArgument(char **argStrings)
/*	Passed a pointer to a zero terminated run of strings move to the next one.
	If the string length is zero of the current or next return FALSE.
 */
{
	UINT32
		length;

	length=strlen((*argStrings));
	if(length)
	{
		(*argStrings)+=length+1;
		if(strlen((*argStrings)))
		{
			return(true);
		}
	}
	return(false);
}

void EditorBeep()
/*	Make a beep sound
 */
{
	if(!MessageBeep(MB_ICONEXCLAMATION))
	{
		FlashWindow(frameWindowHwnd,FALSE);
	}
}


static HCURSOR
	prevCursor;
static bool
	busy = false;
	
void ShowBusy()
/* the editor calls this when it is doing something that may take some
 * time,it should show the user (by changing the pointer or something)
 */
{
	if (!busy)
	{
		prevCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));
		busy = true;
	}
}

void ShowNotBusy()
/* when the editor is again ready to accept user input, it will call this
 * it should undo anything ShowBusy did
 * every ShowBusy will be exactly balanced by a call to ShowNotBusy
 */
{
	if (busy)
	{
		SetCursor(prevCursor);
		busy = false;
	}
}

bool LocateStartupScript(char *scriptPath)
// Search for the editor's startup script, return a path
// to the script which is located.
// NOTE: scriptPath must point to at least _MAX_PATH bytes
// If no script could be located, return false
// Search order:
// 		users home
// 		current directory
// 		executable directory
// 		the system path
{
	const char
		*path;

	scriptPath[0] = 0L;
	
 	Tcl_Eval(theTclInterpreter,"set home {}; catch {set home $env(HOMEDRIVE)$env(HOMEPATH)}; foreach directory \"[list $home] [list [pwd]] [list [file dirname [info nameofexecutable]]] [split $env(PATH) {;}]\" {set theFile $directory/e93lib/e93rc.tcl; if [file exists $theFile] {return [file attributes $theFile -shortname]}}");
	path = Tcl_GetStringResult(theTclInterpreter);

	if ((strlen(path) > 0) && (strlen(path) < _MAX_PATH))
	{
		strncpy(scriptPath, path, _MAX_PATH);
		return true;
	}
	else
	{
		fprintf(stderr, "failed to locate startup script\n");
		return false;
	}
}
