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


typedef struct
{
	EDITORMENU
		*ownerMenu;								// points to the menu that owns this window (if NULL, root menu owns window)
} GUIMENUWINDOWINFO;

EDITORMENU *GetRelatedEditorMenu(EDITORMENU *theMenu,int menuRelationship);
void UpdateMenuWindow(EDITORWINDOW *theWindow);
void DeactivateEditorMenu(EDITORMENU *theMenu);
void ActivateEditorMenu(EDITORMENU *theMenu);
char *GetEditorMenuName(EDITORMENU *theMenu);
bool SetEditorMenuName(EDITORMENU *theMenu,char *theName);
char *GetEditorMenuAttributes(EDITORMENU *theMenu);
bool SetEditorMenuAttributes(EDITORMENU *theMenu,char *theAttributes);
char *GetEditorMenuDataText(EDITORMENU *theMenu);
bool SetEditorMenuDataText(EDITORMENU *theMenu,char *theDataText);
bool GetEditorMenu(int argc,char *argv[],EDITORMENU **theMenu);
void DisposeEditorMenu(EDITORMENU *theMenu);
EDITORMENU *CreateEditorMenu(int argc,char *argv[],int menuRelationship,char *theName,char *theAttributes,char *dataText,bool active);
bool AttemptMenuKeyPress(XEvent *theEvent);
void MenuWindowEvent(EDITORWINDOW *theWindow,XEvent *theEvent);
void RaiseRootMenu();
bool InitEditorMenus();
void UnInitEditorMenus();
