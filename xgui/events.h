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


int XErrorEventHandler(Display *theDisplay,XErrorEvent *theEvent);
int ComposeXLookupString(XKeyEvent *event_struct,char *buffer_return,int bytes_buffer,KeySym *keysym_return);
bool KeyEventToEditorKey(XEvent *theEvent,EDITORKEY *theKeyData);
bool EditorKeyNameToKeyCode(char *theName,UINT32 *theCode);
char *EditorKeyCodeToKeyName(UINT32 theCode);
void XStateToEditorModifiers(unsigned int state,UINT32 numClicks,UINT32 *editorModifiers);
bool EditorGetKeyPress(UINT32 *keyCode,UINT32 *editorModifiers,bool wait,bool clearBuffered);
void ResetMultipleClicks();
UINT32 HandleMultipleClicks(XEvent *theEvent);
bool StillDown(unsigned int theButton,UINT32 numTicks);
void UpdateEditorWindows();
void ResetCursorBlinkTime();
void WaitTicks(UINT32 numTicks);
void AlarmIntervalProc(int unused);
void StartAlarmTimer();
void EditorQuit();
void EditorDoBackground();
void EditorSetModal();
void EditorClearModal();
void EditorEventLoop(int argc,char *argv[]);
void DialogEventLoop(EDITORWINDOW *theDialog,bool *dialogComplete);
void ClearAbort();
bool CheckAbort();
bool InitEvents();
void UnInitEvents();
