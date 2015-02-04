// Shell commands specific to X version of e93
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


#include	"includes.h"

enum
{
	VSBP_LEFT,
	VSBP_RIGHT,
};

static TOKENLIST verticalScrollBarPlacements[]=
{
	{"left",VSBP_LEFT},
	{"right",VSBP_RIGHT},
	{"",0}
};

static int Cmd_RaiseRootMenu(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// Raise the root menu to the top window on the screen
{
	if(objc==1)
	{
		RaiseRootMenu();
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

static int Cmd_ScrollBarPlacement(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// Specify preferred placement for vertical scroll bars
{
	int
		thePlacement;

	if(objc==2)
	{
		if(MatchToken(Tcl_GetStringFromObj(objv[1],NULL),verticalScrollBarPlacements,&thePlacement))
		{
			verticalScrollBarOnLeft=(thePlacement==VSBP_LEFT);	// set global preference
			return(TCL_OK);
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"Invalid placement",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

// routines callable by the editor

bool AddSupplementalShellCommands(Tcl_Interp *theInterpreter)
// if a given environment desires additional shell commands
// it can add them here
// if there is a problem, SetError, and return false
{
	Tcl_CreateObjCommand(theInterpreter,"raiserootmenu",Cmd_RaiseRootMenu,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"scrollbarplacement",Cmd_ScrollBarPlacement,NULL,NULL);
	return(true);
}

