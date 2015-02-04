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


bool EditorKeyNameToKeyCode(char *theName,UINT32 *theCode);
char *EditorKeyCodeToKeyName(UINT32 theCode);
bool GetEditorMenu(int argc,char *argv[],EDITORMENU **theMenu);
EDITORMENU *CreateEditorMenu(int argc,char *argv[],int menuRelationship,char *theName,char *theAttributes,char *dataText,bool active);
bool ProcessWindowsMenuEvent(UINT32 menuID);
bool HandleMenuKeyEvent(UINT32 theKeySym,UINT32 keyModifiers);
void UninitMenuManager();
bool InitMenuManager();
void DeactivateEditorMenu(EDITORMENU *theMenu);
void ActivateEditorMenu(EDITORMENU *theMenu);
