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

BOOL CALLBACK ListBoxHandler(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);
bool SimpleListBoxDialog(char *theTitle,UINT32 numElements,char **listElements,bool *selectedElements,bool *cancel);
void ReportMessage(char *format,...);
bool OkDialog(char *theText);
bool YesNoCancelDialog(char *theText,bool *yes,bool *cancel);
bool OkCancelDialog(char *theText,bool *cancel);
BOOL PASCAL SimpleTextHandler(HWND hwnd,unsigned msg,WPARAM wparam,LPARAM lparam);
bool GetSimpleTextDialog(char *theTitle,char *enteredText,UINT32 stringBytes,bool *cancel);
BOOL PASCAL SearchReplaceHandler(HWND hwnd,unsigned msg,WPARAM wparam,LPARAM lparam);
bool SearchReplaceDialog(EDITORBUFFER *searchUniverse,EDITORBUFFER *replaceUniverse,bool *backwards,bool *wrapAround,bool *selectionExpr,bool *ignoreCase,bool *limitScope,bool *replaceProc,UINT16 *searchType,bool *cancel);
void FreeOpenFileDialogPaths(char **thePaths);
bool OpenFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,char ***listElements,bool *cancel);
void ExtractFilenameFromFullpath(const char *fullPath,char *filename,char *path);
bool SaveFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel);
BOOL APIENTRY ChoosePathDialogHookProc(HWND hDlg,UINT message,UINT wParam,INT32 lParam);
bool ChoosePathDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel);
bool ChooseFontDialog(char *theTitle,char *theFont,UINT32 stringBytes,bool *cancel);
