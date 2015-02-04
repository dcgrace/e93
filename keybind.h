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


typedef struct keyBinding EDITORKEYBINDING;

char *GetKeyBindingText(EDITORKEYBINDING *theBinding);
void GetKeyBindingCodeAndModifiers(EDITORKEYBINDING *theBinding,UINT32 *keyCode,UINT32 *modifierMask,UINT32 *modifierValue);
EDITORKEYBINDING *LocateNextKeyBinding(EDITORKEYBINDING *currentBinding);
EDITORKEYBINDING *LocateKeyBindingMatch(UINT32 keyCode,UINT32 modifierValue);
bool DeleteEditorKeyBinding(UINT32 keyCode,UINT32 modifierMask,UINT32 modifierValue);
bool CreateEditorKeyBinding(UINT32 keyCode,UINT32 modifierMask,UINT32 modifierValue,char *dataText);
bool InitKeyBindingTable();
void UnInitKeyBindingTable();
