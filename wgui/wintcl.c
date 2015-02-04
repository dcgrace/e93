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


#include "includes.h"

static int Tcl_WindowsCascade(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Send a WM_MDICASCADE message to the Client window controlling our MDI windows
 */
{
	SendMessage(clientWindowHwnd,WM_MDICASCADE,0,0L);
	return(TCL_OK);
}

static int Tcl_WindowsTileHorz(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Send a MDITILE_HORIZONTAL message to the Client window controlling our MDI windows
 */
{
	SendMessage(clientWindowHwnd,WM_MDITILE,MDITILE_HORIZONTAL,0L);
	return(TCL_OK);
}

static int Tcl_WindowsTileVert(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Send a MDITILE_VERTICAL message to the Client window controlling our MDI windows
 */
{
	SendMessage(clientWindowHwnd,WM_MDITILE,MDITILE_VERTICAL,0L);
	return(TCL_OK);
}

static int Tcl_WindowsArrangeIcons(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Send a WM_MDIICONARRANGE message to the Client window controlling our MDI windows
 */
{
	SendMessage(clientWindowHwnd,WM_MDIICONARRANGE,0,0L);
	return(TCL_OK);
}

static int Tcl_WindowsPrint(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Passed a window, print the current selection or the entire document if no selection
 */
{
	EDITORWINDOW
		*theWindow;
	char
		*errorFamily,
		*errorFamilyMember,
		*errorDescription;
	if(objc==2)
	{
	if(theWindow=LocateEditorDocumentWindow(Tcl_GetString(objv[1])))
		{
			if(PrintEditorWindow(theWindow))
			{
				return(TCL_OK);
			}
			else
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				Tcl_AppendResult(localInterpreter,errorDescription,NULL);
			}
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"Failed to locate window '",Tcl_GetString(objv[1]),"'",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsSetGlobalStatusField(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Passed a fieldNumber, a width and a string, display the string in the status bar
	in a box that is width pixels wide, clipping the string to fit, at a relative position
	from left to right base off fieldNumber. If width is zero, any status entry that was displayed
	is removed.
 */
{
	UINT32
		field,
		width;
	char
		*errorFamily,
		*errorFamilyMember,
		*errorDescription;


	if(objc==4)
	{
		if(GetUINT32(localInterpreter,objv[1],&field))
		{
			if(GetUINT32(localInterpreter,objv[2],&width))
			{
				if(SetGlobalStatusField(field,width,Tcl_GetString(objv[3])))
				{
					return(TCL_OK);
				}
				else
				{
					GetError(&errorFamily,&errorFamilyMember,&errorDescription);
					Tcl_AppendResult(localInterpreter,errorDescription,NULL);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"fieldNumber width string");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsSetGlobalStatusButton(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Passed a fieldNumber, a width and a string, display the string in the status bar
	in a box that is width pixels wide, clipping the string to fit, at a relative position
	from left to right base off fieldNumber. If width is zero, any status entry that was displayed
	is removed.
 */
{
	UINT32
		field,
		width;
	char
		*errorFamily,
		*errorFamilyMember,
		*errorDescription;


	if(objc==5)
	{
		if(GetUINT32(localInterpreter,objv[1],&field))
		{
			if(GetUINT32(localInterpreter,objv[2],&width))
			{
				if(SetGlobalStatusButton(field,width,Tcl_GetString(objv[3]),Tcl_GetString(objv[4])))
				{
					return(TCL_OK);
				}
				else
				{
					GetError(&errorFamily,&errorFamilyMember,&errorDescription);
					Tcl_AppendResult(localInterpreter,errorDescription,NULL);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"fieldNumber width string cmdString");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsSetGlobalStatusBarFont(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Passed a editor font string, set the global status bar font to that
 */
{
	char
		*errorFamily,
		*errorFamilyMember,
		*errorDescription;

	if(objc==2)
	{
		if(SetGlobalStatusBarFont(Tcl_GetString(objv[1])))
		{
			return(TCL_OK);
		}
		else
		{
			GetError(&errorFamily,&errorFamilyMember,&errorDescription);
			Tcl_AppendResult(localInterpreter,errorDescription,NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"fontString");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsSetWindowStatusBarFont(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Passed a editor font string, set the windows status bar font to that
 */
{
	char
		*errorFamily,
		*errorFamilyMember,
		*errorDescription;

	if(objc==2)
	{
		if(SetWindowStatusBarFont(Tcl_GetString(objv[1])))
		{
			return(TCL_OK);
		}
		else
		{
			GetError(&errorFamily,&errorFamilyMember,&errorDescription);
			Tcl_AppendResult(localInterpreter,errorDescription,NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"fontString");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsGetGlobalStatusBarStringWidth(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Passed a string, return the number of pixels wide the string is for the current font selected in the global
	status bar.
 */
{
	int
		width;
	char
		tempString[256];

	if(objc==2)
	{
		width=GetGlobalStatusBarStringWidth(Tcl_GetString(objv[1]));
		sprintf(tempString,"%lu",width);
		Tcl_AppendResult(localInterpreter,tempString,NULL);
		return(TCL_OK);
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"string");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsWinHelp(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Passed a path to a Windows help file and a keyword, calls WinHelp API function, to get help on the keyword.
 */
{
	if(objc==3)
	{
		if(WinHelp(frameWindowHwnd,Tcl_GetString(objv[1]),HELP_PARTIALKEY,(DWORD)(Tcl_GetString(objv[2]))))
		{
			windowsHelpActive=true;
			return(TCL_OK);
		}
		else
		{
			char
				*errorFamily,
				*errorFamilyMember,
				*errorDescription;
				
			SetWindowsError();
			GetError(&errorFamily,&errorFamilyMember,&errorDescription);
			Tcl_AppendResult(localInterpreter,errorDescription,NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"helpfile keyword");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsHtmlHelp(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
/*	Passed a path to a Windows help html file and a keyword, calls HtmlHelp API function, to get help on the keyword.
 */
{
	HH_AKLINK
		link;
		
	link.cbStruct =     sizeof(HH_AKLINK);
	link.fReserved =    false;
	link.pszKeywords =  NULL; 
	link.pszUrl =       NULL; 
	link.pszMsgText =   NULL; 
	link.pszMsgTitle =  NULL; 
	link.pszWindow =    NULL;
	link.fIndexOnFail = TRUE;

	if(objc==2||objc==3)
	{				
		if(HtmlHelp(GetDesktopWindow(),Tcl_GetString(objv[1]),HH_DISPLAY_TOPIC,NULL))
		{
			if (objc==3)
			{
				link.pszKeywords =  Tcl_GetString(objv[2]); 
				HtmlHelp(GetDesktopWindow(),Tcl_GetString(objv[1]),HH_KEYWORD_LOOKUP,(DWORD_PTR)&link);
			}
			windowsHelpActive=true;
			return(TCL_OK);
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"can't find html help file \"",Tcl_GetString(objv[1]),"\"", NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"helpfile ?keyword?");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsDoPopupMenu(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
{
	int
		result,
		argc;
	char
		**argv;
	UINT32
		item;
	bool
		itemSelected;
	char
		*errorFamily,
		*errorFamilyMember,
		*errorDescription;

	result=TCL_ERROR;
	if(objc>=2)
	{
		if(Tcl_SplitList(localInterpreter,Tcl_GetString(objv[1]),&argc,(const char ***)&argv)==TCL_OK)
		{
			if(CreatePopupMenuAndReturnItem(argc,argv,&item,&itemSelected))
			{
				if(itemSelected)
				{
					Tcl_AppendElement(localInterpreter,argv[item-1]);
				}
				result=TCL_OK;
				ClearAbort();				// if user managed to send break sequence during the popup menu, we would rather not abort now!
			}
			else
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				Tcl_AppendResult(localInterpreter,errorDescription,NULL);
			}
			Tcl_Free((char *)argv);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"list");
	}
	return(result);
}

static int Tcl_WindowsOpenAnotherE93(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
{
	char
		*errorFamily,
		*errorFamilyMember,
		*errorDescription;

	if(objc==2)
	{
		if(GetBoolean(localInterpreter,objv[1],&openAnotherE93))
		{
			if(WriteOpenAnotherE93RegistrySetting(openAnotherE93))
			{
				return(TCL_OK);
			}
			else
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				Tcl_AppendResult(localInterpreter,errorDescription,NULL);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"number");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsSetWindowTitle(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
{
	if(objc==2)
	{
		SetWindowText(frameWindowHwnd,Tcl_GetString(objv[1]));
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"title");
	}
	return(TCL_ERROR);
}

static int Tcl_WindowsRunApplication(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
{
	char
		fileName[MAX_PATH],
		appDir[MAX_PATH];
	SHELLEXECUTEINFO
		execInfo;
	char
		*errorFamily,
		*errorFamilyMember,
		*errorDescription,
		*tclCommand;

	if(objc==2)
	{
		static const char
			*cmnd = "file nativename";
		Tcl_Obj
			*oldResult = Tcl_GetObjResult(localInterpreter);		// get the current Tcl result and hold it
			
		if ((tclCommand = (char *)MNewPtr(strlen(Tcl_GetString(objv[1])) + strlen(cmnd) + 2)) != NULL)
		{
			sprintf(tclCommand, "%s %s", cmnd, Tcl_GetString(objv[1]));
			if(Tcl_Eval(localInterpreter,tclCommand)==TCL_OK)
			{
				ExtractFilenameFromFullpath(Tcl_GetStringResult(localInterpreter),fileName,appDir);
				execInfo.cbSize=sizeof(execInfo);
				execInfo.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;
				execInfo.hwnd=NULL;
				execInfo.lpVerb=NULL;
				execInfo.lpFile=Tcl_GetString(objv[1]);
				execInfo.lpParameters=NULL;
				execInfo.lpDirectory=appDir;
				execInfo.nShow=SW_SHOW;
				execInfo.hInstApp=0;
				if(ShellExecuteEx(&execInfo))
				{
					CloseHandle(execInfo.hProcess);
					Tcl_SetObjResult(localInterpreter, oldResult);	// put the old Tcl result back to cover our tacks and leave things unmolested
					return(TCL_OK);
				}
				else
				{
					SetWindowsError();
					GetError(&errorFamily,&errorFamilyMember,&errorDescription);
					Tcl_AppendResult(localInterpreter,errorDescription,NULL);
				}
			}
		MDisposePtr(tclCommand);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"file.ext");
	}
	return(TCL_ERROR);
}

static char *ShowNum2String(UINT num)
{
	char
		*string = "normal";
		
	switch(num)
	{
		case SW_SHOWNORMAL:
			string = "normal";
			break;
		case SW_SHOWMINIMIZED:
			string = "minimize";
			break;
		case SW_SHOWMAXIMIZED:
			string = "maximize";
			break;
		default:
			string = "normal";
			break;
	}
		
	return string;
}

static UINT String2ShowNum(const char *string)
{
	if(strcasecmp(string, "normal") == 0)
	{
		return SW_NORMAL;
	}
	else if(strcasecmp(string, "maximize") == 0)
	{
		return SW_MAXIMIZE;
	}
	else if(strcasecmp(string, "minimize") == 0)
	{
		return SW_MINIMIZE;
	}
	return SW_NORMAL;
}

static int Tcl_WindowsGetFrameWindowPlacement(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// get the
{
	if(objc==1)
	{
		WINDOWPLACEMENT
			wp;
		char
			tempString[256];
			
		wp.length=sizeof(WINDOWPLACEMENT);

		GetWindowPlacement(frameWindowHwnd,&wp);

		sprintf(tempString,"%d %d %d %d %s",wp.rcNormalPosition.top,wp.rcNormalPosition.left,wp.rcNormalPosition.bottom,wp.rcNormalPosition.right,ShowNum2String(wp.showCmd));
		Tcl_AppendResult(localInterpreter,tempString,NULL);
		return(TCL_OK);
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

bool GetPosition(Tcl_Interp *localInterpreter,Tcl_Obj *const *theObjs,RECT *theRect)
// convert theObjs (of which there must be at least 4) to an RECT
// return false if the conversion failed, and fill the Tcl result
{
	if(Tcl_ExprLongObj(localInterpreter,theObjs[0],&(theRect->top))==TCL_OK)
	{
		if(Tcl_ExprLongObj(localInterpreter,theObjs[1],&(theRect->left))==TCL_OK)
		{
			if(Tcl_ExprLongObj(localInterpreter,theObjs[2],&(theRect->bottom))==TCL_OK)
			{
				if(Tcl_ExprLongObj(localInterpreter,theObjs[3],&(theRect->right))==TCL_OK)
				{
					return(true);
				}
			}
		}
	}
	return(false);
}

static int Tcl_WindowsSetFrameWindowPlacement(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
{
	if(objc==5 || objc==6)
	{
		RECT 
			theRect;
			
		if(GetPosition(localInterpreter,&(objv[1]),&theRect))
		{
			WINDOWPLACEMENT
				wp;
			
			wp.length=sizeof(wp);
			
			GetWindowPlacement(frameWindowHwnd,&wp);
			
			wp.rcNormalPosition.top = theRect.top;
			wp.rcNormalPosition.left = theRect.left;
			wp.rcNormalPosition.bottom = theRect.bottom;
			wp.rcNormalPosition.right = theRect.right;
			if (objc==6)
			{
				wp.showCmd = String2ShowNum(Tcl_GetString(objv[5]));
			}
			else
			{
				wp.showCmd = SW_SHOWNORMAL;
			}
			SetWindowPlacement(frameWindowHwnd,&wp);
		}
		
		return(TCL_OK);
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"top left bottom right");
	}
	return(TCL_ERROR);
}

static int Cmd_RaiseRootMenu(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// Raise the root menu to the top window on the screen (no effect on Windows)
{
	if(objc==1)
	{
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}


bool AddSupplementalShellCommands(Tcl_Interp *theInterpreter)
/* Add supplemental gui TCL commands, if there is a problem set error and return FALSE.
 */
{
	Tcl_CreateObjCommand(theInterpreter,"windowscascade",Tcl_WindowsCascade,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowstilehorz",Tcl_WindowsTileHorz,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowstilevert",Tcl_WindowsTileVert,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowsarrangeicons",Tcl_WindowsArrangeIcons,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowsprint",Tcl_WindowsPrint,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowssetglobalstatusfield",Tcl_WindowsSetGlobalStatusField,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowssetglobalstatusbutton",Tcl_WindowsSetGlobalStatusButton,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowsgetstatusbarstringwidth",Tcl_WindowsGetGlobalStatusBarStringWidth,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowswinhelp",Tcl_WindowsWinHelp,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowshtmlhelp",Tcl_WindowsHtmlHelp,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowsdopopupmenu",Tcl_WindowsDoPopupMenu,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowssetwindowtitle",Tcl_WindowsSetWindowTitle,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowsrunapplication",Tcl_WindowsRunApplication,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowsrunapplication",Tcl_WindowsRunApplication,NULL,NULL);

	Tcl_CreateObjCommand(theInterpreter,"raiserootmenu",Cmd_RaiseRootMenu,NULL,NULL);
	
	// these should just be linked vars
	Tcl_CreateObjCommand(theInterpreter,"windowssetglobalstatusbarfont",Tcl_WindowsSetGlobalStatusBarFont,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowssetwindowstatusbarfont",Tcl_WindowsSetWindowStatusBarFont,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowsgetframewindowplacement",Tcl_WindowsGetFrameWindowPlacement,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowssetframewindowplacement",Tcl_WindowsSetFrameWindowPlacement,NULL,NULL);

	Tcl_LinkVar(theInterpreter, "EEM_MOD0", (char *)&(ModifierTable[0].windowsKeyCode), TCL_LINK_INT);
	Tcl_LinkVar(theInterpreter, "EEM_MOD1", (char *)&(ModifierTable[4].windowsKeyCode), TCL_LINK_INT);
	Tcl_LinkVar(theInterpreter, "EEM_CTL", (char *)&(ModifierTable[2].windowsKeyCode), TCL_LINK_INT);
	
	Tcl_LinkVar(theInterpreter, "filetranslationmode", (char *)&translationModeOn, TCL_LINK_BOOLEAN);

	return(true);
}

extern Tcl_Interp
	*theTclInterpreter;							/* pointer to TCL interpreter we are using */

bool ExecuteTclCommand(char *command)
{
	int
		tclResult;

	ClearAbort();
	tclResult=Tcl_Eval(theTclInterpreter,command);
	if(tclResult!=TCL_OK)
	{
		ReportMessage("Tcl execute error:\n%.256s\n",Tcl_GetStringResult(theTclInterpreter));
	}
	return(true);
}

