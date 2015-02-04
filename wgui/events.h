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

void EditorEventLoop(int argc,char *argv[]);
char *GetMainWindowID();
bool SetCursorBlinkTime();
void ClearAbort();
bool CheckAbort();
void EditorQuit();
bool EditorGetKeyPress(UINT32 *keyCode,UINT32 *editorModifiers,bool wait,bool clearBuffered);
int WINAPI WinMain(HINSTANCE inst,HINSTANCE previnst,LPSTR cmdline,int cmdshow);
LONG PASCAL MdiWindowProc(HWND hwnd,unsigned msg,WPARAM wparam,LPARAM lparam);
LONG PASCAL MPWindowProc(HWND hwnd,unsigned msg,WPARAM wparam,LPARAM lparam);
LONG PASCAL TkWindowProc(HWND hwnd,unsigned msg,WPARAM wparam,LPARAM lparam);
