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


/*
typedef struct tokenlist
	{
	char	*token;
	int		tokenNum;
	} TOKENLIST;
*/

void WaitForDraw(HWND hwnd);
UINT32 GetPerformanceCounter();
void RegisterControlBreakHotKey();
void UnregisterControlBreakHotKey();
void ConvertBSlashToFSlash(char *theStr);
void ConvertFSlashToBSlash(char *theStr);
void FreeStringList(char **theList);
bool AddStringToList(char *theString,char ***theList,UINT32 *numElements);
char **NewStringList();
INT32 GetWindowsVersion();
char *GetEditorLocalVersion();
void SetCommDlgError();
void SetWindowsError();
void OutToDebugger(char *format,...);
int strcasecmp(const char *string1,const char *string2);
bool MatchToken(char *theString,TOKENLIST *theList,int *theToken);
bool ParseLine(const char *inLine,int *inIndex,char *outLine,int *outIndex,int inLineSize,int outLineSize);
bool MakeArgLine(char *inLine,char *outLine,int *lineLength);
bool MakeArgs(char *theLine,int theLength,int *argc,char *(*argv[]));
void UnMakeArgs(char *argv[]);
UINT32 CountNumberOfLineFeeds(char *theStr);
UINT32 RemoveCRFromString(char *theStr);
void CopyStringAndAddCRToLF(char *srcPtr,char *dstPtr,UINT32 maxDst);
bool NextStringArgument(char **argStrings);
void EditorBeep();
void ShowBusy();
void ShowNotBusy();
