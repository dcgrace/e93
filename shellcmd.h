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


EDITORWINDOW *GetEditorWindow(Tcl_Interp *localInterpreter,Tcl_Obj *theName);
EDITORBUFFER *GetEditorBuffer(Tcl_Interp *localInterpreter,Tcl_Obj *theName);
bool GetINT32(Tcl_Interp *localInterpreter,Tcl_Obj *theObject,INT32 *theNumber);
bool GetUINT32(Tcl_Interp *localInterpreter,Tcl_Obj *theObject,UINT32 *theNumber);
bool GetUINT32String(Tcl_Interp *localInterpreter,char *theString,UINT32 *theNumber);
bool GetRectangle(Tcl_Interp *localInterpreter,Tcl_Obj *const *theObjs,EDITORRECT *theRect);
bool GetBoolean(Tcl_Interp *localInterpreter,Tcl_Obj *theObject,bool *theValue);
bool CreateEditorShellCommands(Tcl_Interp *theInterpreter);
