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


void InvalidateWindowStatusBar(HWND hwnd);
void FillWindowStatusBar(HWND statusHwnd,HDC hdc);
void DrawWindowStatusBar(HWND statusHwnd,HDC hdc);
bool CheckWhatStatusButtonClickedOn(INT32 x,INT32 y,char **cmdString);
void InvalidateGlobalStatusBar();
void FillGlobalStatusBar(HWND statusHwnd,HDC hdc);
void DrawGlobalStatusFields(HWND statusHwnd,HDC hdc);
UINT32 GetGlobalStatusBarStringWidth(char *string);
bool SetGlobalStatusField(UINT32 field,UINT32 width,char *theString);
bool SetGlobalStatusButton(UINT32 field,UINT32 width,char *theString,char *cmdString);
bool SetWindowStatusBarFont(char *fontString);
bool SetGlobalStatusBarFont(char *fontString);
bool TrackStatusButton(INT32 x,INT32 y,char **cmdString);
void UninitStatusBar();
bool InitStatusBar();
