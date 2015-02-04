// TCL commands that e93 supports
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
// #include "fapi.h"
// #include "futils.h"

// NOTES about TCL, and its subtle interactions with the editor
//
// First, Tcl should never be allowed to evaluate script while an editor
// window, view, buffer, selectionlist, etc... is in a state of flux that
// would cause it to draw improperly if the script caused the editor to
// update its windows. This is because certain editor operations that
// scripts can call, can force updates of all editor windows.
//
// Second, if a script is running, and decides to operate on a window that
// is in some state of flux, (say in the middle of a replace-all), it must
// be prohibited from doing so, and the operation must fail. NOTE, that by
// rule one, if you are executing script in the middle of replace-all, that
// all buffers must be in a state where they can be drawn.
//
// Finally, if a script is run while some code of the editor is relying on
// a given window, or buffer to be around, the code MUST set the
// buffer Busy, or check that the buffer still exists after
// the script is run.
// If a buffer is set Busy, no script command, or side-effect can allow the
// buffer to be modified in ANY way, including text/selection changes, undo
// changes, new views, deleting views, etc...


typedef bool OptionParseFunction(Tcl_Interp *localInterpreter,char *optionName,int optionValue,void *theVariable,int objc,Tcl_Obj *const objv[],int *optionIndex);

typedef struct
{
	char
		*optionString;
	int
		optionValue;
	OptionParseFunction
		*theFunction;
} OPTIONDESCRIPTORRECORD;

typedef struct
{
	EDITORBUFFER
		*theBuffer;
	SELECTIONUNIVERSE
		*theSelection;
} PARSE_MARK;

static TOKENLIST menuRelationshipTokens[]=
{
	{"nextsibling",MR_NEXTSIBLING},
	{"previoussibling",MR_PREVIOUSSIBLING},
	{"firstchild",MR_FIRSTCHILD},
	{"lastchild",MR_LASTCHILD},
	{"",0}
};

static TOKENLIST cursorMovementTokens[]=
{
	{"leftchar",RPM_mBACKWARD|RPM_CHAR},
	{"rightchar",RPM_CHAR},
	{"leftword",RPM_mBACKWARD|RPM_WORD},
	{"rightword",RPM_WORD},
	{"upline",RPM_mBACKWARD|RPM_LINE},
	{"downline",RPM_LINE},
	{"startline",RPM_mBACKWARD|RPM_LINEEDGE},
	{"endline",RPM_LINEEDGE},
	{"startparagraph",RPM_mBACKWARD|RPM_PARAGRAPHEDGE},
	{"endparagraph",RPM_PARAGRAPHEDGE},
	{"uppage",RPM_mBACKWARD|RPM_PAGE},
	{"downpage",RPM_PAGE},
	{"startdoc",RPM_mBACKWARD|RPM_DOCEDGE},
	{"enddoc",RPM_DOCEDGE},
	{"",0}
};

typedef enum
{
	SM_EXPRESSION=0,
	SM_MAPPING,
	SM_AT,
	SM_ROOT,
} SYNTAX_MAP_COMMANDS;

static TOKENLIST syntaxMapCommands[]=
{
	{"exp",SM_EXPRESSION},
	{"map",SM_MAPPING},
	{"at",SM_AT},
	{"root",SM_ROOT},
	{"",0}
};


#define	TEMPSTRINGBUFFERLENGTH	1024

static UINT32 ForcePositionIntoRange(EDITORBUFFER *theBuffer,UINT32 thePosition)
// take thePosition, and make sure it is in range for theBuffer
{
	if(thePosition>theBuffer->textUniverse->totalBytes)
	{
		thePosition=theBuffer->textUniverse->totalBytes;
	}
	return(thePosition);
}

static void ArrangePositions(UINT32 *startPosition,UINT32 *endPosition)
// Make sure startPosition is <= endPosition
// if not, flip them around
{
	UINT32
		temp;

	if(*startPosition>*endPosition)
	{
		temp=*startPosition;
		*startPosition=*endPosition;
		*endPosition=temp;
	}
}

EDITORWINDOW *GetEditorWindow(Tcl_Interp *localInterpreter,Tcl_Obj *theName)
// see if theString is a window, if so, return it
// if not, return NULL, and set the Tcl result
{
	EDITORWINDOW
		*theWindow;
	char
		*theString;

	theString=Tcl_GetString(theName);
	if((theWindow=LocateEditorDocumentWindow(theString)))
	{
		return(theWindow);
	}
	Tcl_AppendResult(localInterpreter,"Failed to locate window '",theString,"'",NULL);
	return(NULL);
}

EDITORBUFFER *GetEditorBuffer(Tcl_Interp *localInterpreter,Tcl_Obj *theName)
// see if theString is a buffer, if so, return it
// if not, return NULL, and set the Tcl result
{
	EDITORBUFFER
		*theBuffer;
	char
		*theString;

	theString=Tcl_GetString(theName);
	if((theBuffer=LocateBuffer(theString)))
	{
		return(theBuffer);
	}
	Tcl_AppendResult(localInterpreter,"Failed to locate buffer '",theString,"'", NULL);
	return(NULL);
}

bool GetINT32(Tcl_Interp *localInterpreter,Tcl_Obj *theObject,INT32 *theNumber)
// convert theObject to a INT32
// return false if the conversion failed, and fill the Tcl result
{
	long
		theLong;

	if(Tcl_ExprLongObj(localInterpreter,theObject,&theLong)==TCL_OK)
	{
		*theNumber=(INT32)theLong;
		return(true);
	}
	return(false);
}

bool GetUINT32(Tcl_Interp *localInterpreter,Tcl_Obj *theObject,UINT32 *theNumber)
// convert theObject to a UINT32
// return false if the conversion failed, and fill the Tcl result
{
	long
		theLong;

	if(Tcl_ExprLongObj(localInterpreter,theObject,&theLong)==TCL_OK)
	{
		*theNumber=(UINT32)theLong;					// ### this should really be fixed to be a true unsigned
		return(true);
	}
	return(false);
}

bool GetUINT32String(Tcl_Interp *localInterpreter,char *theString,UINT32 *theNumber)
// convert theString to a UINT32
// return false if the conversion failed, and fill the Tcl result
{
	long
		theLong;

	if(Tcl_ExprLong(localInterpreter,theString,&theLong)==TCL_OK)
	{
		*theNumber=(UINT32)theLong;					// ### this should really be fixed to be a true unsigned
		return(true);
	}
	return(false);
}

bool GetRectangle(Tcl_Interp *localInterpreter,Tcl_Obj *const *theObjs,EDITORRECT *theRect)
// convert theObjs (of which there must be at least 4) to an EDITORRECT
// return false if the conversion failed, and fill the Tcl result
{
	if(GetINT32(localInterpreter,theObjs[0],&theRect->x))
	{
		if(GetINT32(localInterpreter,theObjs[1],&theRect->y))
		{
			if(GetUINT32(localInterpreter,theObjs[2],&theRect->w))
			{
				if(GetUINT32(localInterpreter,theObjs[3],&theRect->h))
				{
					return(true);
				}
			}
		}
	}
	return(false);
}

static bool GetNibble(char theCharacter,UINT32 *theNibble)
// interpret theCharacter as a hex nibble, and return it
// if there is a problem return false
{
	if(theCharacter>='0'&&theCharacter<='9')
	{
		*theNibble=theCharacter-'0';
		return(true);
	}
	else
	{
		if(theCharacter>='A'&&theCharacter<='F')
		{
			*theNibble=10+theCharacter-'A';
			return(true);
		}
		else
		{
			if(theCharacter>='a'&&theCharacter<='f')
			{
				*theNibble=10+theCharacter-'a';
				return(true);
			}
		}
	}
	return(false);
}

static bool GetColor(Tcl_Interp *localInterpreter,Tcl_Obj *theObject,EDITORCOLOR *theColor)
// convert theObject to a color value
// return false if the conversion failed, and fill the Tcl result
{
	UINT32
		index,
		theNibble;
	char
		theCharacter;
	bool
		fail;
	char
		*theString;

	fail=false;
	theString=Tcl_GetString(theObject);
	if(!EditorColorNameToColor(theString,theColor))
	{
		*theColor=0;
		index=0;
		while(!fail&&(theCharacter=theString[index]))
		{
			if(GetNibble(theCharacter,&theNibble))
			{
				(*theColor)<<=4;
				(*theColor)|=theNibble;
				index++;
			}
			else
			{
				fail=true;
			}
		}
		(*theColor)&=0xFFFFFF;		// if number was large, ignore preceding digits
		if(!index)					// no characters in the input string
		{
			fail=true;
		}
	}
	if(fail)
	{
		Tcl_AppendResult(localInterpreter,"Invalid color:", theString, NULL);
	}
	return(!fail);
}

bool GetBoolean(Tcl_Interp *localInterpreter,Tcl_Obj *theObject,bool *theValue)
// convert theObject to a boolean value
// return false if the conversion failed, and fill the Tcl result
{
	int
		intValue;

	if(Tcl_GetBoolean(localInterpreter,Tcl_GetString(theObject),&intValue)==TCL_OK)
	{
		*theValue=intValue?true:false;
		return(true);
	}
	return(false);
}

static bool OptionBoolean(Tcl_Interp *localInterpreter,char *optionName,int optionValue,void *theVariable,int objc,Tcl_Obj *const objv[],int *optionIndex)
// Handle parsing of a boolean option
{
	// some compiler (IRIX?) complained about casting an int to a bool
	*((bool *)theVariable)=(optionValue != 0);
	return(true);
}

static bool OptionEnum(Tcl_Interp *localInterpreter,char *optionName,int optionValue,void *theVariable,int objc,Tcl_Obj *const objv[],int *optionIndex)
// Handle parsing of a enumeration option
{
	*((int *)theVariable)=optionValue;
	return(true);
}

/* unused code
static bool OptionUINT32(Tcl_Interp *localInterpreter,char *optionName,int optionValue,void *theVariable,int objc,Tcl_Obj *const objv[],int *optionIndex)
// Return an unsigned integer
{
	if((*optionIndex)<objc)
	{
		if(GetUINT32(localInterpreter,objv[*optionIndex],(UINT32 *)theVariable))
		{
			(*optionIndex)++;
			return(true);
		}
	}
	else
	{
		Tcl_AppendResult(localInterpreter,optionName,": Missing arguments",NULL);
	}
	return(false);
}
*/

static bool OptionMark(Tcl_Interp *localInterpreter,char *optionName,int optionValue,void *theVariable,int objc,Tcl_Obj *const objv[],int *optionIndex)
// Attempt to locate a currently existing mark within an editor buffer
{
	MARKLIST
		*theMark;
	char
		*markName;
	PARSE_MARK
		*parseMark;

	if((*optionIndex)<objc)
	{
		parseMark=(PARSE_MARK *)theVariable;
		markName=Tcl_GetString(objv[*optionIndex]);
		(*optionIndex)++;
		if((theMark=LocateEditorMark(parseMark->theBuffer,markName)))
		{
			parseMark->theSelection=theMark->selectionUniverse;
			return(true);
		}
		else
		{
			Tcl_AppendResult(localInterpreter,optionName,": Failed to locate mark '",markName,"'",NULL);
		}
	}
	else
	{
		Tcl_AppendResult(localInterpreter,optionName,": Missing arguments",NULL);
	}
	return(false);
}

static bool LocateOptionMatch(OPTIONDESCRIPTORRECORD *theOptions,char *theOption,int *matchIndex)
// attempt to locate an option that matches theOption
// if one was located, return true with the index in matchIndex
// else return false
{
	int
		i;
	bool
		found,reachedEnd;

	i=0;
	found=reachedEnd=false;
	while(!found&&!reachedEnd)
	{
		if(theOptions[i].optionString)
		{
			if(strcmp(theOption,theOptions[i].optionString)==0)
			{
				*matchIndex=i;
				found=true;
			}
			i++;
		}
		else
		{
			reachedEnd=true;
		}
	}
	return(found);
}

static void AddOptionsToResult(Tcl_Interp *localInterpreter,OPTIONDESCRIPTORRECORD *theOptions)
// used when reporting bad options
{
	int
		i;

	Tcl_AppendResult(localInterpreter,theOptions[0].optionString,NULL);
	i=1;
	while(theOptions[i].optionString)
	{
		if(theOptions[i+1].optionString)
		{
			Tcl_AppendResult(localInterpreter,", ",NULL);
		}
		else
		{
			Tcl_AppendResult(localInterpreter,", or ",NULL);
		}
		Tcl_AppendResult(localInterpreter,theOptions[i].optionString,NULL);
		i++;
	}
}

static bool ParseOptions(Tcl_Interp *localInterpreter,OPTIONDESCRIPTORRECORD *theOptions,void **theVariables,int objc,Tcl_Obj *const objv[],int *optionIndex,bool unrecognizedIsError)
// parse command options given a description of the options, and a list of parameters for them
// to modify
// if there is a problem, set the error string in localInterpreter, and return false
// NOTE: if unrecognizedIsError is false, this will stop parsing at the first
// unrecognized token, and return true
// if unrecognizedIsError is true, this will only return true if the list of
// options can be parsed correctly until the end.
{
	bool
		fail,
		mismatch;
	char
		*optionName;
	int
		matchIndex;

	fail=mismatch=false;
	while(!fail&&!mismatch&&((*optionIndex)<objc))
	{
		optionName=Tcl_GetString(objv[*optionIndex]);
		(*optionIndex)++;
		if(LocateOptionMatch(theOptions,optionName,&matchIndex))
		{
			fail=!theOptions[matchIndex].theFunction(localInterpreter,optionName,theOptions[matchIndex].optionValue,theVariables[matchIndex],objc,objv,optionIndex);
		}
		else
		{
			mismatch=true;
			if(unrecognizedIsError)
			{
				Tcl_AppendResult(localInterpreter,"bad option \"",optionName,"\": must be ",NULL);
				AddOptionsToResult(localInterpreter,theOptions);
				fail=true;
			}
		}
	}
	return(!fail);
}

static UINT32
	modifierFlagTable[]=
	{
		EEM_CAPSLOCK,
		EEM_SHIFT,
		EEM_CTL,
		EEM_MOD0,
		EEM_MOD1,
		EEM_MOD2,
		EEM_MOD3,
		EEM_MOD4,
		EEM_MOD5,
		EEM_MOD6,
		EEM_MOD7
	};

static bool GetModifierMaskAndValue(Tcl_Interp *localInterpreter,Tcl_Obj *theObject,UINT32 *modifierMask,UINT32 *modifierValue)
// convert theObject of modifiers into a mask, and value
// if there is a problem, set the Tcl result, and return false
{
	int
		i,
		theLength;
	char
		*theString;
	bool
		fail;

	*modifierMask=0;
	*modifierValue=0;
	theString=Tcl_GetString(theObject);
	theLength=strlen(theString);
	fail=false;
	if(theLength==11)				// must be an entry for each flag
	{
		for(i=0;(!fail)&&(i<theLength);i++)
		{
			switch(theString[i])
			{
				case 'x':
				case 'X':
					break;
				case '0':
					*modifierMask|=modifierFlagTable[i];
					break;
				case '1':
					*modifierMask|=modifierFlagTable[i];
					*modifierValue|=modifierFlagTable[i];
					break;
				default:
					Tcl_AppendResult(localInterpreter,"Invalid modifiers",NULL);
					fail=true;
					break;
			}
		}
	}
	else
	{
		Tcl_AppendResult(localInterpreter,"Invalid modifiers (incorrect length)",NULL);
		fail=true;
	}
	return(!fail);
}

static void ModifierMaskAndValueToString(UINT32 modifierMask,UINT32 modifierValue,char *theString)
// given a modifier mask, and value, make a modifier string
// NOTE: theString must be long enough to hold the result (which is never longer than 12 characters
// including the terminator)
{
	int
		i;

	for(i=0;i<11;i++)
	{
		if(modifierMask&modifierFlagTable[i])
		{
			if(modifierValue&modifierFlagTable[i])
			{
				theString[i]='1';
			}
			else
			{
				theString[i]='0';
			}
		}
		else
		{
			theString[i]='X';
		}
	}
	theString[i]='\0';
}

static bool GetVarFlagList(Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[],bool *flagList)
// write values from the given variable array to the given boolean array
// if there is a problem, set the TCL error, and return false
{
	Tcl_Obj
		*varValue;
	int
		currentCount;
	bool
		fail;

	fail=false;
	for(currentCount=0;currentCount<objc&&!fail;currentCount++)
	{
		if((varValue=Tcl_ObjGetVar2(localInterpreter,objv[currentCount],NULL,TCL_LEAVE_ERR_MSG)))
		{
			fail=!GetBoolean(localInterpreter,varValue,&(flagList[currentCount]));
		}
		else
		{
			fail=true;
		}
	}
	return(!fail);
}

static bool PutVarFlagList(Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[],bool *flagList)
// write the boolean variables back into Tcl variables
// if there is a problem, set the TCL error, and return false
{
	int
		currentCount;
	bool
		fail;

	fail=false;
	for(currentCount=0;currentCount<objc&&!fail;currentCount++)
	{
		fail=!Tcl_ObjSetVar2(localInterpreter,objv[currentCount],NULL,Tcl_NewStringObj(flagList[currentCount]?"1":"0",-1),TCL_LEAVE_ERR_MSG);
	}
	return(!fail);
}

static bool ShellBufferNotBusy(Tcl_Interp *localInterpreter,EDITORBUFFER *theBuffer)
// check the given buffer's busy state
// if it is true, set the Tcl result, and return false
{
	if(!BufferBusy(theBuffer))
	{
		return(true);
	}
	else
	{
		Tcl_AppendResult(localInterpreter,"Buffer '",theBuffer->contentName,"' is busy",NULL);
	}
	return(false);
}

static bool CurrentClipboardNotBusy(Tcl_Interp *localInterpreter)
// make sure the current clipboard is not busy, if it is, set the Tcl result
// and return false
{
	EDITORBUFFER
		*clipboardBuffer;

	if((clipboardBuffer=EditorGetCurrentClipboard()))
	{
		return(ShellBufferNotBusy(localInterpreter,clipboardBuffer));
	}
	return(true);
}

// ACTUAL TCL COMMAND IMPLEMENTATIONS

static int Cmd_AddMenu(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// add a menu to the menu list
// NOTE: if a menu already exists, this will delete it before attempting
// a creation
// NOTE ALSO: if this deletes an element, it will also delete the element's children
{
	int
		theRelationship;
	bool
		active;
	int
		pathArgc;
	char
		**pathArgv;
	bool
		succeed;

	succeed=false;
	if(objc==7)
	{
		if(MatchToken(Tcl_GetString(objv[2]),menuRelationshipTokens,&theRelationship))
		{
			if(GetBoolean(localInterpreter,objv[3],&active))
			{
				if(Tcl_SplitList(localInterpreter,Tcl_GetString(objv[1]),&pathArgc,(const char ***)&pathArgv)==TCL_OK)
				{
					if(CreateEditorMenu(pathArgc,pathArgv,theRelationship,Tcl_GetString(objv[4]),Tcl_GetString(objv[5]),Tcl_GetString(objv[6]),active))
					{
						succeed=true;
					}
					else
					{
						GetError(&errorFamily,&errorFamilyMember,&errorDescription);
						Tcl_AppendResult(localInterpreter,errorDescription,NULL);
					}
					Tcl_Free((char *)pathArgv);
				}
			}
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"Syntax error in menu relationship field",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"menuPathList relationship active menuName menuModifiers menuFunction");
	}
	return(succeed?TCL_OK:TCL_ERROR);
}

static int Cmd_DeleteMenu(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// delete a menu from the menu list
// NOTE: if this deletes an element, it will also delete the element's children
{
	EDITORMENU
		*theMenu;
	UINT32
		theIndex;
	int
		pathArgc;
	char
		**pathArgv;
	bool
		fail;

	fail=false;
	if(objc>1)
	{
		for(theIndex=1;!fail&&(int)theIndex<objc;theIndex++)
		{
			if(Tcl_SplitList(localInterpreter,Tcl_GetString(objv[theIndex]),&pathArgc,(const char ***)&pathArgv)==TCL_OK)
			{
				theMenu=NULL;										// start at the root
				if(GetEditorMenu(pathArgc,pathArgv,&theMenu))
				{
					DisposeEditorMenu(theMenu);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Invalid menu specification",NULL);
					fail=true;
				}
				Tcl_Free((char *)pathArgv);
			}
			else
			{
				fail=true;
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"menuPathList ?menuPathList ...?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_BindKey(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// add a key binding to the bindings list
// NOTE: if a binding already exists, this will delete it before attempting
// a creation
{
	UINT32
		keyCode,
		modifierMask,
		modifierValue;

	if(objc==4)
	{
		if(EditorKeyNameToKeyCode(Tcl_GetString(objv[1]),&keyCode))						// find the code for this key
		{
			if(GetModifierMaskAndValue(localInterpreter,objv[2],&modifierMask,&modifierValue))
			{
				if(CreateEditorKeyBinding(keyCode,modifierMask,modifierValue,Tcl_GetString(objv[3])))
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
			Tcl_AppendResult(localInterpreter,"Invalid key code",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"keyName modifiers keyFunction");
	}
	return(TCL_ERROR);
}

static int Cmd_UnBindKey(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// delete a key binding from the bindings list
{
	UINT32
		keyCode,
		modifierMask,
		modifierValue;

	if(objc==3)
	{
		if(EditorKeyNameToKeyCode(Tcl_GetString(objv[1]),&keyCode))						// find the code for this key
		{
			if(GetModifierMaskAndValue(localInterpreter,objv[2],&modifierMask,&modifierValue))
			{
				if(DeleteEditorKeyBinding(keyCode,modifierMask,modifierValue))
				{
					return(TCL_OK);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Key binding does not exist",NULL);
				}
			}
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"Invalid key code",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"keyName modifiers");
	}
	return(TCL_ERROR);
}

static int Cmd_KeyBindings(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// dump all key bindings
{
	UINT32
		keyCode,
		modifierMask,
		modifierValue;
	EDITORKEYBINDING
		*currentBinding;
	UINT32
		currentCode,
		currentMask,
		currentValue;
	char
		*keyName,
		modifierString[TEMPSTRINGBUFFERLENGTH];
	bool
		haveName,
		haveModifiers;
	bool
		fail;
	bool
		didOne;

	if(objc>=1&&objc<=3)
	{
		fail=false;
		haveName=haveModifiers=false;
		if(objc>=2)
		{
			haveName=true;
			if(!EditorKeyNameToKeyCode(Tcl_GetString(objv[1]),&keyCode))						// find the code for this key
			{
				Tcl_AppendResult(localInterpreter,"Invalid key code",NULL);
				fail=true;
			}
		}
		if(!fail&&objc>=3)
		{
			haveModifiers=true;
			fail=!GetModifierMaskAndValue(localInterpreter,objv[2],&modifierMask,&modifierValue);
		}
		if(!fail)
		{
			currentBinding=keyBindingListHead;
			didOne=false;
			while(currentBinding)
			{
				GetKeyBindingCodeAndModifiers(currentBinding,&currentCode,&currentMask,&currentValue);

				if(!haveName||(currentCode==keyCode))
				{
					if(!haveModifiers||((currentMask==modifierMask)&&(currentValue==modifierValue)))
					{
						if((keyName=EditorKeyCodeToKeyName(currentCode)))
						{
							ModifierMaskAndValueToString(currentMask,currentValue,&(modifierString[0]));
							if(didOne)				// do not put return on last line
							{
								Tcl_AppendResult(localInterpreter,"\n",NULL);
							}
							Tcl_AppendResult(localInterpreter,"bindkey",NULL);
							Tcl_AppendElement(localInterpreter,keyName);
							Tcl_AppendElement(localInterpreter,&(modifierString[0]));
							Tcl_AppendElement(localInterpreter,GetKeyBindingText(currentBinding));
							didOne=true;
						}
					}
				}
				currentBinding=LocateNextKeyBinding(currentBinding);
			}
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"?keyName? ?modifiers?");
	}
	return(TCL_ERROR);
}

static int Cmd_WaitKey(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// wait for a keyboard press, return the string for the code, and modifiers
{
	UINT32
		keyCode,
		modifierValue;
	char
		*keyName,
		modifierString[TEMPSTRINGBUFFERLENGTH];

	if(objc==1)
	{
		EditorGetKeyPress(&keyCode,&modifierValue,true,false);
		if((keyName=EditorKeyCodeToKeyName(keyCode)))
		{
			ModifierMaskAndValueToString(0xFFFFFFFF,modifierValue,&(modifierString[0]));
			Tcl_AppendElement(localInterpreter,keyName);
			Tcl_AppendElement(localInterpreter,&(modifierString[0]));
			return(TCL_OK);
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"Unnamed key",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

static int Cmd_GetKey(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// check to see if there is a keyboard event in the queue, if so, get it, if not, return an error
{
	UINT32
		keyCode,
		modifierValue;
	char
		*keyName,
		modifierString[TEMPSTRINGBUFFERLENGTH];

	if(objc==1)
	{
		if(EditorGetKeyPress(&keyCode,&modifierValue,false,false))
		{
			if((keyName=EditorKeyCodeToKeyName(keyCode)))
			{
				ModifierMaskAndValueToString(0xFFFFFFFF,modifierValue,&(modifierString[0]));
				Tcl_AppendElement(localInterpreter,keyName);
				Tcl_AppendElement(localInterpreter,&(modifierString[0]));
				return(TCL_OK);
			}
			else
			{
				Tcl_AppendResult(localInterpreter,"Unnamed key",NULL);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

static int Cmd_NewBuffer(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a new buffer with the passed name
// if a buffer already exists with the passed name fail
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=EditorNewBuffer(Tcl_GetString(objv[1]))))				// create a new buffer
		{
			Tcl_AppendResult(localInterpreter,theBuffer->contentName,NULL);
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_OpenBuffer(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// open the passed file into a buffer, return the buffer name
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=EditorOpenBuffer(Tcl_GetString(objv[1]))))
		{
			Tcl_AppendResult(localInterpreter,theBuffer->contentName,NULL);
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"pathName");
	}
	return(TCL_ERROR);
}

static int Cmd_CloseBuffer(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// close the passed buffer as long as it is not in use, or being modified
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				EditorCloseBuffer(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_SaveBuffer(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// save the passed buffer
{
	EDITORBUFFER
		*theBuffer;
	bool
		result;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				result=EditorSaveBuffer(theBuffer);
				ClearBufferBusy(theBuffer);
				if(result)
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_SaveBufferAs(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// save the passed buffer to newPath
// rename the buffer to the new name, set clean
// NOTE: this returns the new name of theBuffer after it is saved
{
	EDITORBUFFER
		*theBuffer;
	bool
		result;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				result=EditorSaveBufferAs(theBuffer,Tcl_GetString(objv[2]));
				ClearBufferBusy(theBuffer);
				if(result)
				{
					Tcl_AppendResult(localInterpreter,theBuffer->contentName,NULL);
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName newPath");
	}
	return(TCL_ERROR);
}

static int Cmd_SaveBufferTo(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// save the passed buffer to a file, do not modify the buffer's "clean" status
{
	EDITORBUFFER
		*theBuffer;
	bool
		result;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				result=EditorSaveBufferTo(theBuffer,Tcl_GetString(objv[2]));
				ClearBufferBusy(theBuffer);
				if(result)
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName pathName");
	}
	return(TCL_ERROR);
}

static int Cmd_RevertBuffer(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// revert the passed buffer
{
	EDITORBUFFER
		*theBuffer;
	bool
		result;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				result=EditorRevertBuffer(theBuffer);
				ClearBufferBusy(theBuffer);
				if(result)
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_BufferDirty(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return 1 if the passed buffer is dirty, 0 if not
// if the buffer cannot be located, fail
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(AtUndoCleanPoint(theBuffer))
			{
				Tcl_AppendResult(localInterpreter,"0",NULL);
			}
			else
			{
				Tcl_AppendResult(localInterpreter,"1",NULL);
			}
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_FromFile(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return 1 if the passed buffer is linked to a file, 0 if not
// if the buffer cannot be located, fail
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(theBuffer->fromFile)
			{
				Tcl_AppendResult(localInterpreter,"1",NULL);
			}
			else
			{
				Tcl_AppendResult(localInterpreter,"0",NULL);
			}
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_HasWindow(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return 1 if the passed buffer has a window, 0 if not
// if the buffer cannot be located, fail
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(theBuffer->theWindow)
			{
				Tcl_AppendResult(localInterpreter,"1",NULL);
			}
			else
			{
				Tcl_AppendResult(localInterpreter,"0",NULL);
			}
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_ClearDirty(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// clear the dirty flag for the passed buffer
// if the buffer cannot be located, fail
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			SetBufferBusy(theBuffer);
			EditorStartTextChange(theBuffer);			// start text change, so that status bars can update if needed
			SetUndoCleanPoint(theBuffer);				// make it clean again
			EditorEndTextChange(theBuffer);				// change is complete
			ClearBufferBusy(theBuffer);
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_BufferList(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the current list of document buffers
{
	bool
		fail;
	EDITORBUFFER
		*testBuffer;

	fail=false;
	if(objc==1)
	{
		testBuffer=EditorGetFirstBuffer();
		while(testBuffer)
		{
			Tcl_AppendElement(localInterpreter,testBuffer->contentName);
			testBuffer=EditorGetNextBuffer(testBuffer);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_WindowList(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the current list of document windows
{
	bool
		fail;
	UINT32
		windowIndex,
		numWindows;
	EDITORWINDOW
		**windowList;

	fail=false;
	if(objc==1)
	{
		if(GetSortedDocumentWindowList(&numWindows,&windowList))
		{
			for(windowIndex=0;windowIndex<numWindows;windowIndex++)
			{
				Tcl_AppendElement(localInterpreter,windowList[windowIndex]->theBuffer->contentName);
			}
			MDisposePtr(windowList);
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_OpenWindow(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// open a window onto the given buffer
// if there is already a window open on the buffer, just bring it to the top
{
	EDITORBUFFER
		*theBuffer;
	EDITORWINDOW
		*theWindow;
	EDITORRECT
		theRect;
	UINT32
		tabSize;
	EDITORCOLOR
		foreground,
		background;

	if(objc==10)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(GetRectangle(localInterpreter,&(objv[2]),&theRect))
			{
				if(GetUINT32(localInterpreter,objv[7],&tabSize))
				{
					if(GetColor(localInterpreter,objv[8],&foreground))
					{
						if(GetColor(localInterpreter,objv[9],&background))
						{
							if(ShellBufferNotBusy(localInterpreter,theBuffer))
							{
								SetBufferBusy(theBuffer);
								theWindow=EditorOpenDocumentWindow(theBuffer,&theRect,Tcl_GetString(objv[6]),tabSize,foreground,background);
								ClearBufferBusy(theBuffer);
								if(theWindow)
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
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName x y w h fontName tabSize foregroundColor backgroundColor");
	}
	return(TCL_ERROR);
}

static int Cmd_CloseWindow(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// close a given window
{
	EDITORWINDOW
		*theWindow;
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			theBuffer=theWindow->theBuffer;								// hang onto this, we will need it after the window closes
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				EditorCloseDocumentWindow(theWindow);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName");
	}
	return(TCL_ERROR);
}

static int Cmd_SetRect(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the rectangle of a window
{
	EDITORWINDOW
		*theWindow;
	EDITORRECT
		theRect;
	bool
		result;

	if(objc==6)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(GetRectangle(localInterpreter,&(objv[2]),&theRect))
			{
				if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
				{
					SetBufferBusy(theWindow->theBuffer);
					result=SetEditorDocumentWindowRect(theWindow,&theRect);
					ClearBufferBusy(theWindow->theBuffer);
					if(result)
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
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName x y w h");
	}
	return(TCL_ERROR);
}

static int Cmd_GetRect(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// get the rectangle of a window
{
	EDITORWINDOW
		*theWindow;
	EDITORRECT
		theRect;
	bool
		result;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
			{
				SetBufferBusy(theWindow->theBuffer);
				result=GetEditorDocumentWindowRect(theWindow,&theRect);
				ClearBufferBusy(theWindow->theBuffer);
				if(result)
				{
					sprintf(tempString,"%d %d %d %d",(int)theRect.x,(int)theRect.y,(int)theRect.w,(int)theRect.h);
					Tcl_AppendResult(localInterpreter,tempString,NULL);
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName");
	}
	return(TCL_ERROR);
}

static int Cmd_SetFont(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the font of a window
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	UINT32
		theStyle;
	bool
		haveFont;
	bool
		result;
	char
		*fontName;

	if(objc==3||objc==4)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			fontName=Tcl_GetString(objv[2]);
			haveFont=fontName[0]!='\0';
			theStyle=0;
			if((objc==3)||GetUINT32(localInterpreter,objv[3],&theStyle))
			{
				if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
				{
					result=true;
					SetBufferBusy(theWindow->theBuffer);
					theView=GetDocumentWindowCurrentView(theWindow);
					if(haveFont||theStyle==0)
					{
						result=SetEditorViewStyleFont(theView,theStyle,fontName);
					}
					else
					{
						ClearEditorViewStyleFont(theView,theStyle);
					}
					ClearBufferBusy(theWindow->theBuffer);
					if(result)
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
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName fontName ?styleNum?");
	}
	return(TCL_ERROR);
}

static int Cmd_GetFont(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// get the font of a window/style
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	UINT32
		theStyle;
	bool
		result;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2||objc==3)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			theStyle=0;
			if((objc==2)||GetUINT32(localInterpreter,objv[2],&theStyle))
			{
				if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
				{
					SetBufferBusy(theWindow->theBuffer);
					theView=GetDocumentWindowCurrentView(theWindow);
					result=GetEditorViewStyleFont(theView,theStyle,&tempString[0],TEMPSTRINGBUFFERLENGTH);
					ClearBufferBusy(theWindow->theBuffer);
					if(result)
					{
						Tcl_AppendResult(localInterpreter,tempString,NULL);
						return(TCL_OK);
					}
					else
					{
						Tcl_AppendResult(localInterpreter,"Failed to get font",NULL);
					}
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName ?styleNum?");
	}
	return(TCL_ERROR);
}

static int Cmd_SetTabSize(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the tab size of a window
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	UINT32
		tabSize;
	bool
		result;

	if(objc==3)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(GetUINT32(localInterpreter,objv[2],&tabSize))
			{
				if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
				{
					SetBufferBusy(theWindow->theBuffer);
					theView=GetDocumentWindowCurrentView(theWindow);
					result=SetEditorViewTabSize(theView,tabSize);
					ClearBufferBusy(theWindow->theBuffer);
					if(result)
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
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName tabSize");
	}
	return(TCL_ERROR);
}

static int Cmd_GetTabSize(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// get the tab size of a window
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	UINT32
		tabSize;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
			{
				SetBufferBusy(theWindow->theBuffer);
				theView=GetDocumentWindowCurrentView(theWindow);
				tabSize=GetEditorViewTabSize(theView);
				ClearBufferBusy(theWindow->theBuffer);
				sprintf(tempString,"%d",(int)tabSize);
				Tcl_AppendResult(localInterpreter,tempString,NULL);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName");
	}
	return(TCL_ERROR);
}

static int Cmd_SetColors(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the colors of a window
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	bool
		haveForeground,
		haveBackground;
	EDITORCOLOR
		foregroundColor,
		backgroundColor;
	UINT32
		theStyle;
	bool
		result;
	char
		*foregroundName,
		*backgroundName;

	if(objc==4||objc==5)
	{
		foregroundName=Tcl_GetString(objv[2]);
		backgroundName=Tcl_GetString(objv[3]);

		haveForeground=foregroundName[0]!='\0';	// if these strings are 0 length, they indicate that the color should be cleared
		haveBackground=backgroundName[0]!='\0';

		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			theStyle=0;
			if((objc==4)||GetUINT32(localInterpreter,objv[4],&theStyle))
			{
				if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
				{
					theView=GetDocumentWindowCurrentView(theWindow);
					SetBufferBusy(theWindow->theBuffer);
					result=true;
					if(haveForeground||theStyle==0)
					{
						if(GetColor(localInterpreter,objv[2],&foregroundColor))
						{
							result=SetEditorViewStyleForegroundColor(theView,theStyle,foregroundColor);
							if(result&&(theStyle==0))
							{
								result=SetEditorDocumentWindowForegroundColor(theWindow,foregroundColor);
							}
							if(!result)
							{
								GetError(&errorFamily,&errorFamilyMember,&errorDescription);
								Tcl_AppendResult(localInterpreter,errorDescription,NULL);
							}
						}
						else
						{
							result=false;
						}
					}
					else
					{
						ClearEditorViewStyleForegroundColor(theView,theStyle);
					}
					if(result)
					{
						if(haveBackground||theStyle==0)
						{
							if(GetColor(localInterpreter,objv[3],&backgroundColor))
							{
								result=SetEditorViewStyleBackgroundColor(theView,theStyle,backgroundColor);
								if(result&&(theStyle==0))
								{
									result=SetEditorDocumentWindowBackgroundColor(theWindow,backgroundColor);
								}
								if(!result)
								{
									GetError(&errorFamily,&errorFamilyMember,&errorDescription);
									Tcl_AppendResult(localInterpreter,errorDescription,NULL);
								}
							}
							else
							{
								result=false;
							}
						}
						else
						{
							ClearEditorViewStyleBackgroundColor(theView,theStyle);
						}
					}
					ClearBufferBusy(theWindow->theBuffer);
					if(result)
					{
						return(TCL_OK);
					}
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName foregroundColor backgroundColor ?styleNum?");
	}
	return(TCL_ERROR);
}

static int Cmd_GetColors(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// get the colors of a window
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	bool
		haveForeground,
		haveBackground;
	EDITORCOLOR
		foregroundColor,
		backgroundColor;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];
	UINT32
		theStyle;

	if(objc==2||objc==3)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			theStyle=0;
			if((objc==2)||GetUINT32(localInterpreter,objv[2],&theStyle))
			{
				if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
				{
					SetBufferBusy(theWindow->theBuffer);
					theView=GetDocumentWindowCurrentView(theWindow);
					haveForeground=GetEditorViewStyleForegroundColor(theView,theStyle,&foregroundColor);
					haveBackground=GetEditorViewStyleBackgroundColor(theView,theStyle,&backgroundColor);
					ClearBufferBusy(theWindow->theBuffer);
					if(haveForeground)
					{
						sprintf(tempString,"%06X ",(int)foregroundColor);
						Tcl_AppendResult(localInterpreter,tempString,NULL);
					}
					else
					{
						Tcl_AppendResult(localInterpreter,"\"\" ",NULL);
					}
					if(haveBackground)
					{
						sprintf(tempString,"%06X",(int)backgroundColor);
						Tcl_AppendResult(localInterpreter,tempString,NULL);
					}
					else
					{
						Tcl_AppendResult(localInterpreter,"\"\"",NULL);
					}
					return(TCL_OK);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName ?styleNum?");
	}
	return(TCL_ERROR);
}

static int Cmd_SetSelectionColors(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the selection colors of a window
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	bool
		haveForeground,
		haveBackground;
	EDITORCOLOR
		foregroundColor,
		backgroundColor;
	bool
		result;
	char
		*foregroundName,
		*backgroundName;

	if(objc==4)
	{
		foregroundName=Tcl_GetString(objv[2]);
		backgroundName=Tcl_GetString(objv[3]);

		haveForeground=foregroundName[0]!='\0';	// if these strings are 0 length, they indicate that the color should be cleared
		haveBackground=backgroundName[0]!='\0';

		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(!haveForeground||GetColor(localInterpreter,objv[2],&foregroundColor))
			{
				if(!haveBackground||GetColor(localInterpreter,objv[3],&backgroundColor))
				{
					if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
					{
						SetBufferBusy(theWindow->theBuffer);
						theView=GetDocumentWindowCurrentView(theWindow);
						result=true;
						if(haveForeground)
						{
							result=SetEditorViewSelectionForegroundColor(theView,foregroundColor);
						}
						else
						{
							ClearEditorViewSelectionForegroundColor(theView);
						}
						if(result)
						{
							if(haveBackground)
							{
								result=SetEditorViewSelectionBackgroundColor(theView,backgroundColor);
							}
							else
							{
								ClearEditorViewSelectionBackgroundColor(theView);
							}
						}
						ClearBufferBusy(theWindow->theBuffer);
						if(result)
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
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName foregroundColor backgroundColor");
	}
	return(TCL_ERROR);
}

static int Cmd_GetSelectionColors(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// get the selection colors of a window
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	bool
		haveForeground,
		haveBackground;
	EDITORCOLOR
		foregroundColor,
		backgroundColor;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
			{
				SetBufferBusy(theWindow->theBuffer);
				theView=GetDocumentWindowCurrentView(theWindow);
				haveForeground=GetEditorViewSelectionForegroundColor(theView,&foregroundColor);
				haveBackground=GetEditorViewSelectionBackgroundColor(theView,&backgroundColor);
				ClearBufferBusy(theWindow->theBuffer);
				if(haveForeground)
				{
					sprintf(tempString,"%06X ",(int)foregroundColor);
					Tcl_AppendResult(localInterpreter,tempString,NULL);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"\"\" ",NULL);
				}
				if(haveBackground)
				{
					sprintf(tempString,"%06X",(int)backgroundColor);
					Tcl_AppendResult(localInterpreter,tempString,NULL);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"\"\"",NULL);
				}
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName");
	}
	return(TCL_ERROR);
}

static int Cmd_BufferVariables(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// show all the variables linked to the passed buffer
{
	EDITORBUFFER
		*theBuffer;
	VARIABLEBINDING
		*currentBinding;
	bool
		didOne;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			currentBinding=theBuffer->variableTable.variableBindingListHead;
			didOne=false;
			while(currentBinding)
			{
				if(didOne)				// do not put return on last line
				{
					Tcl_AppendResult(localInterpreter,"\n",NULL);
				}
				Tcl_AppendElement(localInterpreter,GetVariableBindingName(currentBinding));
				Tcl_AppendElement(localInterpreter,GetVariableBindingText(currentBinding));
				didOne=true;
				currentBinding=LocateNextVariableBinding(currentBinding);
			}
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_SetBufferVariable(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set/remove a buffer relative variable
{
	EDITORBUFFER
		*theBuffer;
	char
		*variableName;

	if(objc==3||objc==4)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			variableName=Tcl_GetString(objv[2]);
			if(objc==3)
			{
				if(DeleteVariableBinding(&(theBuffer->variableTable),Tcl_GetString(objv[2])))
				{
					return(TCL_OK);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Failed to locate variable '",Tcl_GetString(objv[2]),"'",NULL);
				}
			}
			else
			{
				if(CreateVariableBinding(&(theBuffer->variableTable),variableName,Tcl_GetString(objv[3])))
				{
					Tcl_AppendResult(localInterpreter,Tcl_GetString(objv[3]),NULL);
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName variableName ?variableText?");
	}
	return(TCL_ERROR);
}

static int Cmd_GetBufferVariable(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// get a variable hung off of a buffer
// these variables can be used by Tcl scripts to mark buffers, and associate
// arbitrary data with them
{
	EDITORBUFFER
		*theBuffer;
	VARIABLEBINDING
		*bufferVariable;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if((bufferVariable=LocateVariableBinding(&(theBuffer->variableTable),Tcl_GetString(objv[2]))))
			{
				Tcl_AppendResult(localInterpreter,GetVariableBindingText(bufferVariable),NULL);
				return(TCL_OK);
			}
			else
			{
				Tcl_AppendResult(localInterpreter,"Failed to locate variable '",Tcl_GetString(objv[2]),"'",NULL);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName variableName");
	}
	return(TCL_ERROR);
}

static int Cmd_SetTopWindow(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// make the passed window the top/active window
{
	EDITORWINDOW
		*theWindow;

	if(objc==2)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			SetTopDocumentWindow(theWindow);
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName");
	}
	return(TCL_ERROR);
}

static int Cmd_MinimizeWindow(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// attempt to minimize (in whatever way the current OS supports) the given document window
{
	EDITORWINDOW
		*theWindow;

	if(objc==2)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			MinimizeDocumentWindow(theWindow);
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName");
	}
	return(TCL_ERROR);
}

static int Cmd_UnminimizeWindow(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// attempt to un-minimize (in whatever way the current OS supports) the given document window
{
	EDITORWINDOW
		*theWindow;

	if(objc==2)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			UnminimizeDocumentWindow(theWindow);
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName");
	}
	return(TCL_ERROR);
}

static int Cmd_GetActiveWindow(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the active window name, or error if none
{
	EDITORWINDOW
		*theWindow;

	if(objc==1)
	{
		if((theWindow=GetActiveDocumentWindow()))
		{
			Tcl_AppendResult(localInterpreter,theWindow->theBuffer->contentName,NULL);
			return(TCL_OK);
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"No active window",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

static int Cmd_UpdateWindows(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// update any window that has invalid areas
// on some implementations, this may do nothing
{
	if(objc==1)
	{
		UpdateEditorWindows();
		return(TCL_OK);
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

static int Cmd_ScreenSize(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the dimensions of the screen that the editor is running on
{
	UINT32
		theWidth,
		theHeight;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==1)
	{
		GetEditorScreenDimensions(&theWidth,&theHeight);
		sprintf(tempString,"%u %u",theWidth,theHeight);
		Tcl_AppendResult(localInterpreter,tempString,NULL);
		return(TCL_OK);
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

static int Cmd_UndoToggle(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform an undo toggle in the given buffer
{
	EDITORBUFFER
		*theBuffer;
	bool
		result;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				result=EditorToggleUndo(theBuffer);
				Tcl_AppendResult(localInterpreter,result?"1":"0",NULL);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_Undo(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform an undo in the given buffer
{
	EDITORBUFFER
		*theBuffer;
	bool
		result;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				result=EditorUndo(theBuffer);
				Tcl_AppendResult(localInterpreter,result?"1":"0",NULL);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_Redo(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform a redo in the given buffer
{
	EDITORBUFFER
		*theBuffer;
	bool
		result;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				result=EditorRedo(theBuffer);
				Tcl_AppendResult(localInterpreter,result?"1":"0",NULL);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_BreakUndo(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// cause a break in the undo flow for the passed buffer
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				BeginUndoGroup(theBuffer);
				StrictEndUndoGroup(theBuffer);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_FlushUndos(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform a clear of the undo/redo buffers in the given buffer
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				EditorClearUndo(theBuffer);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_Cut(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform a cut in the given buffer
{
	EDITORBUFFER
		*theBuffer;
	EDITORBUFFER
		*theClipboard;
	bool
		fail;

	fail=false;
	if(objc==2||objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				if(objc==3)
				{
					theClipboard=LocateBuffer(Tcl_GetString(objv[2]));
				}
				else
				{
					theClipboard=EditorGetCurrentClipboard();
				}
				if(theClipboard)
				{
					if(ShellBufferNotBusy(localInterpreter,theClipboard))
					{
						SetBufferBusy(theClipboard);
						EditorCut(theBuffer,theClipboard);
						ClearBufferBusy(theClipboard);
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"No clipboard",NULL);
					fail=true;
				}
				ClearBufferBusy(theBuffer);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName ?clipboardBuffer?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_Copy(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform a copy in the given buffer
{
	EDITORBUFFER
		*theBuffer;
	EDITORBUFFER
		*theClipboard;
	bool
		fail;

	fail=false;
	if(objc==2||objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				if(objc==3)
				{
					theClipboard=LocateBuffer(Tcl_GetString(objv[2]));
				}
				else
				{
					theClipboard=EditorGetCurrentClipboard();
				}
				if(theClipboard)
				{
					if(ShellBufferNotBusy(localInterpreter,theClipboard))
					{
						SetBufferBusy(theClipboard);
						EditorCopy(theBuffer,theClipboard);
						ClearBufferBusy(theClipboard);
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"No clipboard",NULL);
					fail=true;
				}
				ClearBufferBusy(theBuffer);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName ?clipboardBuffer?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_Paste(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform a columnar paste in the given window/buffer
{
	EDITORBUFFER
		*theBuffer;
	EDITORBUFFER
		*theClipboard;
	EDITORVIEW
		*theView;
	bool
		fail;

	fail=false;
	if(objc==2||objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(theBuffer->theWindow)
				{
					theView=GetDocumentWindowCurrentView(theBuffer->theWindow);
				}
				else
				{
					theView=NULL;
				}
				SetBufferBusy(theBuffer);
				if(objc==3)
				{
					theClipboard=LocateBuffer(Tcl_GetString(objv[2]));
				}
				else
				{
					theClipboard=EditorGetCurrentClipboard();
				}
				if(theClipboard)
				{
					if(ShellBufferNotBusy(localInterpreter,theClipboard))
					{
						SetBufferBusy(theClipboard);
						EditorColumnarPaste(theBuffer,theView,theClipboard);
						ClearBufferBusy(theClipboard);
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"No clipboard",NULL);
					fail=true;
				}
				ClearBufferBusy(theBuffer);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName ?clipboardBuffer?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_SetClipboard(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the current clipboard to the given buffer (or NULL)
{
	EDITORBUFFER
		*theClipboard;

	if(objc==1)
	{
		EditorSetCurrentClipboard(NULL);
		return(TCL_OK);
	}
	else if(objc==2)
	{
		if((theClipboard=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theClipboard))
			{
				EditorSetCurrentClipboard(theClipboard);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"?clipboardBuffer?");
	}
	return(TCL_ERROR);
}

static int Cmd_GetClipboard(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the current clipboard, fail if there is none
{
	EDITORBUFFER
		*theClipboard;

	if(objc==1)
	{
		if((theClipboard=EditorGetCurrentClipboard()))
		{
			Tcl_AppendResult(localInterpreter,theClipboard->contentName,NULL);
			return(TCL_OK);
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"No clipboard",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

static int Cmd_OkDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a dialog with some text, and an OK button
{
	bool
		fail;

	fail=false;
	if(objc==2)
	{
		if(OkDialog(Tcl_GetString(objv[1])))
		{
			ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
		}
		else
		{
			GetError(&errorFamily,&errorFamilyMember,&errorDescription);
			Tcl_AppendResult(localInterpreter,errorDescription,NULL);
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogText");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_OkCancelDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a dialog with some text, and an OK button, and a cancel button
{
	bool
		cancel,
		fail;

	fail=false;
	if(objc==2)
	{
		if(OkCancelDialog(Tcl_GetString(objv[1]),&cancel))
		{
			if(cancel)
			{
				fail=true;
			}
			ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
		}
		else
		{
			GetError(&errorFamily,&errorFamilyMember,&errorDescription);
			Tcl_AppendResult(localInterpreter,errorDescription,NULL);
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogText");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_YesNoDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a dialog with some text, and yes, no, and cancel buttons
{
	bool
		yes,
		cancel,
		fail;

	fail=false;
	if(objc==2)
	{
		if(YesNoCancelDialog(Tcl_GetString(objv[1]),&yes,&cancel))
		{
			if(cancel)
			{
				fail=true;
			}
			else
			{
				Tcl_AppendResult(localInterpreter,yes?"1":"0",NULL);
			}
			ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
		}
		else
		{
			GetError(&errorFamily,&errorFamilyMember,&errorDescription);
			Tcl_AppendResult(localInterpreter,errorDescription,NULL);
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogText");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_TextDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a get text dialog
// if InitialText is not specified, then no initial text will be present in the dialog
// NOTE: since this can indirectly modify the current clipboard, the clipboard must not be busy
{
	bool
		cancel,
		fail;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	fail=false;
	if(objc==2||objc==3)
	{
		if(objc==3)
		{
			strncpy(tempString,Tcl_GetString(objv[2]),TEMPSTRINGBUFFERLENGTH-1);
			tempString[TEMPSTRINGBUFFERLENGTH-1]='\0';
		}
		else
		{
			tempString[0]='\0';
		}
		if(CurrentClipboardNotBusy(localInterpreter))
		{
			if(GetSimpleTextDialog(Tcl_GetString(objv[1]),&(tempString[0]),TEMPSTRINGBUFFERLENGTH,&cancel))
			{
				if(cancel)
				{
					fail=true;
				}
				else
				{
					Tcl_AppendResult(localInterpreter,&(tempString[0]),NULL);
				}
				ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
			}
			else
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				Tcl_AppendResult(localInterpreter,errorDescription,NULL);
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogTitle ?initialText?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_SearchDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a search dialog
// findBuffer, and replaceBuffer must exist, or an error will be returned
// NOTE: currently DialogTitle is ignored, later it should be implemented
// NOTE: since this can indirectly modify the current clipboard, the clipboard must not be busy
{
	bool
		flagList[6];
	UINT16
		searchMode;
	EDITORBUFFER
		*findBuffer,
		*replaceBuffer;
	bool
		cancel,
		result;

	result=false;
	if(objc==10)
	{
		if((findBuffer=GetEditorBuffer(localInterpreter,objv[2])))
		{
			if(ShellBufferNotBusy(localInterpreter,findBuffer))
			{
				if((replaceBuffer=GetEditorBuffer(localInterpreter,objv[3])))
				{
					if(ShellBufferNotBusy(localInterpreter,replaceBuffer))
					{
						if(GetVarFlagList(localInterpreter,6,&(objv[4]),&(flagList[0])))
						{
							SetBufferBusy(findBuffer);
							SetBufferBusy(replaceBuffer);
							if(CurrentClipboardNotBusy(localInterpreter))
							{
								if(SearchReplaceDialog(findBuffer,replaceBuffer,&(flagList[0]),&(flagList[1]),&(flagList[2]),&(flagList[3]),&(flagList[4]),&(flagList[5]),&searchMode,&cancel))
								{
									if(!cancel)
									{
										result=true;
									}
									ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
									PutVarFlagList(localInterpreter,6,&(objv[4]),&(flagList[0]));	// put these back even if user cancelled
								}
								else
								{
									GetError(&errorFamily,&errorFamilyMember,&errorDescription);
									Tcl_AppendResult(localInterpreter,errorDescription,NULL);
								}
							}
							ClearBufferBusy(replaceBuffer);
							ClearBufferBusy(findBuffer);
							if(result)
							{
								switch(searchMode)
								{
									case ST_FIND:
										Tcl_AppendResult(localInterpreter,"find",NULL);
										break;
									case ST_FINDALL:
										Tcl_AppendResult(localInterpreter,"findall",NULL);
										break;
									case ST_REPLACE:
										Tcl_AppendResult(localInterpreter,"replace",NULL);
										break;
									case ST_REPLACEALL:
										Tcl_AppendResult(localInterpreter,"replaceall",NULL);
										break;
								}
								return(TCL_OK);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogTitle findBuffer replaceBuffer backwardsVar wrapAroundVar selectionExprVar ignoreCaseVar limitScopeVar replaceProcVar");
	}
	return(TCL_ERROR);
}

static int Cmd_ListDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a list dialog, handle it, and if all is well, return a new list
// that contains the chosen elements of the old list
{
	UINT32
		theIndex;
	int
		listArgc;
	char
		**listArgv;
	bool
		*selectedElements;
	bool
		cancel,
		fail;

	fail=false;
	if(objc==3)
	{
		if(Tcl_SplitList(localInterpreter,Tcl_GetString(objv[2]),&listArgc,(const char ***)&listArgv)==TCL_OK)
		{
			if((selectedElements=(bool *)MNewPtrClr(sizeof(bool)*listArgc)))	// get array of "falses"
			{
				if(listArgc)
				{
					selectedElements[0]=true;	// select the first element if there is one
				}
				if(SimpleListBoxDialog(Tcl_GetString(objv[1]),listArgc,listArgv,selectedElements,&cancel))	// let user choose elements
				{
					if(!cancel)
					{
						for(theIndex=0;(int)theIndex<listArgc;theIndex++)
						{
							if(selectedElements[theIndex])
							{
								Tcl_AppendElement(localInterpreter,listArgv[theIndex]);	// make a result that contains the selected elements
							}
						}
					}
					else
					{
						fail=true;
					}
					ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
				}
				else
				{
					GetError(&errorFamily,&errorFamilyMember,&errorDescription);
					Tcl_AppendResult(localInterpreter,errorDescription,NULL);
					fail=true;
				}
				MDisposePtr(selectedElements);
			}
			else
			{
				Tcl_AppendResult(localInterpreter,"Failed to allocate memory for list result",NULL);
				fail=true;
			}
			Tcl_Free((char *)listArgv);
		}
		else
		{
			// SplitList will leave a result if it fails
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogTitle inputList");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_OpenDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a get file(s) dialog
// if InitialText is not specified, then no initial text will be present in the dialog
// NOTE: since this can indirectly modify the current clipboard, the clipboard must not be busy
{
	bool
		cancel,
		fail;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];
	UINT32
		theIndex;
	char
		**thePaths;

	fail=false;
	if(objc==2||objc==3)
	{
		if(objc==3)
		{
			strncpy(tempString,Tcl_GetString(objv[2]),TEMPSTRINGBUFFERLENGTH-1);
			tempString[TEMPSTRINGBUFFERLENGTH-1]='\0';
		}
		else
		{
			tempString[0]='\0';
		}
		if(CurrentClipboardNotBusy(localInterpreter))
		{
			if(OpenFileDialog(Tcl_GetString(objv[1]),&(tempString[0]),TEMPSTRINGBUFFERLENGTH,&thePaths,&cancel))
			{
				if(!cancel)
				{
					theIndex=0;
					while(thePaths[theIndex])
					{
						Tcl_AppendElement(localInterpreter,thePaths[theIndex++]);
					}
					FreeOpenFileDialogPaths(thePaths);
				}
				else
				{
					fail=true;
				}
				ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
			}
			else
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				Tcl_AppendResult(localInterpreter,errorDescription,NULL);
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogTitle ?initialText?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_SaveDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a save file dialog
// if InitialText is not specified, then no initial text will be present in the dialog
// NOTE: since this can indirectly modify the current clipboard, the clipboard must not be busy
{
	bool
		cancel,
		fail;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	fail=false;
	if(objc==2||objc==3)
	{
		if(objc==3)
		{
			strncpy(tempString,Tcl_GetString(objv[2]),TEMPSTRINGBUFFERLENGTH-1);
			tempString[TEMPSTRINGBUFFERLENGTH-1]='\0';
		}
		else
		{
			tempString[0]='\0';
		}
		if(CurrentClipboardNotBusy(localInterpreter))
		{
			if(SaveFileDialog(Tcl_GetString(objv[1]),&(tempString[0]),TEMPSTRINGBUFFERLENGTH,&cancel))
			{
				if(!cancel)
				{
					Tcl_AppendResult(localInterpreter,&(tempString[0]),NULL);
				}
				else
				{
					fail=true;
				}
				ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
			}
			else
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				Tcl_AppendResult(localInterpreter,errorDescription,NULL);
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogTitle ?initialText?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_PathDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a path choose dialog
// if InitialText is not specified, then no initial text will be present in the dialog
// NOTE: since this can indirectly modify the current clipboard, the clipboard must not be busy
{
	bool
		cancel,
		fail;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	fail=false;
	if(objc==2||objc==3)
	{
		if(objc==3)
		{
			strncpy(tempString,Tcl_GetString(objv[2]),TEMPSTRINGBUFFERLENGTH-1);
			tempString[TEMPSTRINGBUFFERLENGTH-1]='\0';
		}
		else
		{
			tempString[0]='\0';
		}
		if(CurrentClipboardNotBusy(localInterpreter))
		{
			if(ChoosePathDialog(Tcl_GetString(objv[1]),&(tempString[0]),TEMPSTRINGBUFFERLENGTH,&cancel))
			{
				if(!cancel)
				{
					Tcl_AppendResult(localInterpreter,&(tempString[0]),NULL);
				}
				else
				{
					fail=true;
				}
				ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
			}
			else
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				Tcl_AppendResult(localInterpreter,errorDescription,NULL);
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogTitle ?initialText?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}


static int Cmd_FontDialog(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a font selection dialog
// if InitialFont is not specified, then no initial text will be present in the dialog
// NOTE: since this can indirectly modify the current clipboard, the clipboard must not be busy
{
	bool
		cancel,
		fail;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	fail=false;
	if(objc==2||objc==3)
	{
		if(objc==3)
		{
			strncpy(tempString,Tcl_GetString(objv[2]),TEMPSTRINGBUFFERLENGTH-1);
			tempString[TEMPSTRINGBUFFERLENGTH-1]='\0';
		}
		else
		{
			tempString[0]='\0';
		}
		if(CurrentClipboardNotBusy(localInterpreter))
		{
			if(ChooseFontDialog(Tcl_GetString(objv[1]),&(tempString[0]),TEMPSTRINGBUFFERLENGTH,&cancel))
			{
				if(!cancel)
				{
					Tcl_AppendResult(localInterpreter,&(tempString[0]),NULL);
				}
				else
				{
					fail=true;
				}
				ClearAbort();				// if user managed to send break sequence during the dialog, we would rather not abort now!
			}
			else
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				Tcl_AppendResult(localInterpreter,errorDescription,NULL);
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"dialogTitle ?initialFont?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static void CreateSearchResult(char *theString,bool foundMatch,SELECTIONUNIVERSE *theSelection)
// When a find, findall, replace, or replaceall has finished executing, this
// is called to create the Tcl result for the search
// If the search was successful, this will return the range of the first
// match/replacement
// If the search was not successful, this will return an empty string
{
	UINT32
		matchPosition;
	UINT32
		startPosition,
		numElements;
	bool
		isActive;

	if(foundMatch)
	{
		matchPosition=GetSelectionCursorPosition(theSelection);	// get the position of the match/first match
		GetSelectionRange(theSelection,matchPosition,&startPosition,&numElements,&isActive);
		if(!isActive)
		{
			startPosition=matchPosition;
			numElements=0;
		}
		sprintf(theString,"%u %u",startPosition,startPosition+numElements);
	}
	else
	{
		theString[0]='\0';					// just return an empty string
	}
}


static OPTIONDESCRIPTORRECORD searchOptions[]=
{
	{"-backward",		true,	OptionBoolean},
	{"-forward",		false,	OptionBoolean},
	{"-wrap",			true,	OptionBoolean},
	{"-nowrap",			false,	OptionBoolean},
	{"-regex",			true,	OptionBoolean},
	{"-literal",		false,	OptionBoolean},
	{"-ignorecase",		true,	OptionBoolean},
	{"-case",			false,	OptionBoolean},
	{"-limitscope",		true,	OptionBoolean},
	{"-globalscope",	false,	OptionBoolean},
	{"-replacescript",	true,	OptionBoolean},
	{"-replaceliteral",	false,	OptionBoolean},
	{"-sourcemark",		0,		OptionMark},
	{"-destmark",		0,		OptionMark},
	{NULL,0,NULL}
};

static int Cmd_Find(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// find within the given buffer
// findBuffer must exist, or an error will be returned
// if there is no error, then if there as a match, the first match span is returned, if no
// matches, a null string is returned
{
	EDITORBUFFER
		*theBuffer;
	EDITORBUFFER
		*findBuffer;
	bool
		backwards,
		wrap,
		regex,
		ignoreCase,
		limitScope,
		replaceScript;
	PARSE_MARK
		sourceSelection,
		destSelection;
	void
		*parsedVars[]=
		{
			(void *)&backwards,
			(void *)&backwards,
			(void *)&wrap,
			(void *)&wrap,
			(void *)&regex,
			(void *)&regex,
			(void *)&ignoreCase,
			(void *)&ignoreCase,
			(void *)&limitScope,
			(void *)&limitScope,
			(void *)&replaceScript,
			(void *)&replaceScript,
			(void *)&sourceSelection,
			(void *)&destSelection,
		};
	int
		optionIndex;
	bool
		foundMatch;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];
	bool
		fail;

	fail=false;
	if(objc>=3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				if((findBuffer=GetEditorBuffer(localInterpreter,objv[2])))
				{
					if(ShellBufferNotBusy(localInterpreter,findBuffer))
					{
						SetBufferBusy(findBuffer);
						optionIndex=3;

						backwards=false;
						wrap=false;
						regex=false;
						ignoreCase=false;
						limitScope=false;
						replaceScript=false;

						sourceSelection.theBuffer=theBuffer;
						sourceSelection.theSelection=theBuffer->selectionUniverse;

						destSelection.theBuffer=theBuffer;
						destSelection.theSelection=theBuffer->selectionUniverse;

						if(ParseOptions(localInterpreter,searchOptions,parsedVars,objc,objv,&optionIndex,true))
						{
							if(EditorFind(theBuffer,sourceSelection.theSelection,findBuffer->textUniverse,backwards,wrap,regex,ignoreCase,&foundMatch,destSelection.theSelection))
							{
								CreateSearchResult(tempString,foundMatch,destSelection.theSelection);
								Tcl_AppendResult(localInterpreter,tempString,NULL);
							}
							else
							{
								GetError(&errorFamily,&errorFamilyMember,&errorDescription);
								Tcl_AppendResult(localInterpreter,errorDescription,NULL);
								fail=true;
							}
						}
						else
						{
							fail=true;
						}
						ClearBufferBusy(findBuffer);
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					fail=true;
				}
				ClearBufferBusy(theBuffer);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"searchInBuffer findBuffer ?flags?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_FindAll(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// find all within the given buffer
// findBuffer must exist, or an error will be returned
// if there is no error, then if there as a match, the first match span is returned, if no
// matches, a null string is returned
{
	EDITORBUFFER
		*theBuffer;
	EDITORBUFFER
		*findBuffer;
	bool
		backwards,
		wrap,
		regex,
		ignoreCase,
		limitScope,
		replaceScript;
	PARSE_MARK
		sourceSelection,
		destSelection;
	void
		*parsedVars[]=
		{
			(void *)&backwards,
			(void *)&backwards,
			(void *)&wrap,
			(void *)&wrap,
			(void *)&regex,
			(void *)&regex,
			(void *)&ignoreCase,
			(void *)&ignoreCase,
			(void *)&limitScope,
			(void *)&limitScope,
			(void *)&replaceScript,
			(void *)&replaceScript,
			(void *)&sourceSelection,
			(void *)&destSelection,
		};
	int
		optionIndex;
	bool
		foundMatch;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];
	bool
		fail;

	fail=false;
	if(objc>=3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				if((findBuffer=GetEditorBuffer(localInterpreter,objv[2])))
				{
					if(ShellBufferNotBusy(localInterpreter,findBuffer))
					{
						SetBufferBusy(findBuffer);
						optionIndex=3;

						backwards=false;
						wrap=false;
						regex=false;
						ignoreCase=false;
						limitScope=false;
						replaceScript=false;

						sourceSelection.theBuffer=theBuffer;
						sourceSelection.theSelection=theBuffer->selectionUniverse;

						destSelection.theBuffer=theBuffer;
						destSelection.theSelection=theBuffer->selectionUniverse;

						if(ParseOptions(localInterpreter,searchOptions,parsedVars,objc,objv,&optionIndex,true))
						{
							if(EditorFindAll(theBuffer,sourceSelection.theSelection,findBuffer->textUniverse,backwards,wrap,regex,ignoreCase,limitScope,&foundMatch,destSelection.theSelection))
							{
								CreateSearchResult(tempString,foundMatch,destSelection.theSelection);
								Tcl_AppendResult(localInterpreter,tempString,NULL);
							}
							else
							{
								GetError(&errorFamily,&errorFamilyMember,&errorDescription);
								Tcl_AppendResult(localInterpreter,errorDescription,NULL);
								fail=true;
							}
						}
						else
						{
							fail=true;
						}
						ClearBufferBusy(findBuffer);
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					fail=true;
				}
				ClearBufferBusy(theBuffer);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"searchInBuffer findBuffer ?flags?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_Replace(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// replace within the given buffer
// findBuffer and replaceBuffer must exist, or an error will be returned
// if there is no error, then if there as a match, the first match span is returned, if no
// matches, a null string is returned
{
	EDITORBUFFER
		*theBuffer;
	EDITORBUFFER
		*findBuffer,
		*replaceBuffer;
	bool
		backwards,
		wrap,
		regex,
		ignoreCase,
		limitScope,
		replaceScript;
	PARSE_MARK
		sourceSelection,
		destSelection;
	void
		*parsedVars[]=
		{
			(void *)&backwards,
			(void *)&backwards,
			(void *)&wrap,
			(void *)&wrap,
			(void *)&regex,
			(void *)&regex,
			(void *)&ignoreCase,
			(void *)&ignoreCase,
			(void *)&limitScope,
			(void *)&limitScope,
			(void *)&replaceScript,
			(void *)&replaceScript,
			(void *)&sourceSelection,
			(void *)&destSelection,
		};
	int
		optionIndex;
	bool
		foundMatch;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];
	bool
		fail;

	fail=false;
	if(objc>=3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				if((findBuffer=GetEditorBuffer(localInterpreter,objv[2])))
				{
					if(ShellBufferNotBusy(localInterpreter,findBuffer))
					{
						SetBufferBusy(findBuffer);
						if((replaceBuffer=GetEditorBuffer(localInterpreter,objv[3])))
						{
							if(ShellBufferNotBusy(localInterpreter,replaceBuffer))
							{
								SetBufferBusy(replaceBuffer);
								optionIndex=4;

								backwards=false;
								wrap=false;
								regex=false;
								ignoreCase=false;
								limitScope=false;
								replaceScript=false;

								sourceSelection.theBuffer=theBuffer;
								sourceSelection.theSelection=theBuffer->selectionUniverse;

								destSelection.theBuffer=theBuffer;
								destSelection.theSelection=theBuffer->selectionUniverse;

								if(ParseOptions(localInterpreter,searchOptions,parsedVars,objc,objv,&optionIndex,true))
								{
									if(EditorReplace(theBuffer,sourceSelection.theSelection,findBuffer->textUniverse,replaceBuffer->textUniverse,backwards,wrap,regex,ignoreCase,replaceScript,&foundMatch,destSelection.theSelection))
									{
										CreateSearchResult(tempString,foundMatch,destSelection.theSelection);
										Tcl_AppendResult(localInterpreter,tempString,NULL);
									}
									else
									{
										GetError(&errorFamily,&errorFamilyMember,&errorDescription);
										Tcl_AppendResult(localInterpreter,errorDescription,NULL);
										fail=true;
									}
								}
								else
								{
									fail=true;
								}
								ClearBufferBusy(replaceBuffer);
							}
							else
							{
								fail=true;
							}
						}
						else
						{
							fail=true;
						}
						ClearBufferBusy(findBuffer);
					}
				}
				else
				{
					fail=true;
				}
				ClearBufferBusy(theBuffer);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"searchInBuffer findBuffer replaceBuffer ?flags?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_ReplaceAll(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// replace all within the given buffer
// findBuffer and replaceBuffer must exist, or an error will be returned
// if there is no error, then if there as a match, the first match span is returned, if no
// matches, a null string is returned
{
	EDITORBUFFER
		*theBuffer;
	EDITORBUFFER
		*findBuffer,
		*replaceBuffer;
	bool
		backwards,
		wrap,
		regex,
		ignoreCase,
		limitScope,
		replaceScript;
	PARSE_MARK
		sourceSelection,
		destSelection;
	void
		*parsedVars[]=
		{
			(void *)&backwards,
			(void *)&backwards,
			(void *)&wrap,
			(void *)&wrap,
			(void *)&regex,
			(void *)&regex,
			(void *)&ignoreCase,
			(void *)&ignoreCase,
			(void *)&limitScope,
			(void *)&limitScope,
			(void *)&replaceScript,
			(void *)&replaceScript,
			(void *)&sourceSelection,
			(void *)&destSelection,
		};
	int
		optionIndex;
	bool
		foundMatch;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];
	bool
		fail;

	fail=false;
	if(objc>=3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				if((findBuffer=GetEditorBuffer(localInterpreter,objv[2])))
				{
					if(ShellBufferNotBusy(localInterpreter,findBuffer))
					{
						SetBufferBusy(findBuffer);
						if((replaceBuffer=GetEditorBuffer(localInterpreter,objv[3])))
						{
							if(ShellBufferNotBusy(localInterpreter,replaceBuffer))
							{
								SetBufferBusy(replaceBuffer);
								optionIndex=4;

								backwards=false;
								wrap=false;
								regex=false;
								ignoreCase=false;
								limitScope=false;
								replaceScript=false;

								sourceSelection.theBuffer=theBuffer;
								sourceSelection.theSelection=theBuffer->selectionUniverse;

								destSelection.theBuffer=theBuffer;
								destSelection.theSelection=theBuffer->selectionUniverse;

								if(ParseOptions(localInterpreter,searchOptions,parsedVars,objc,objv,&optionIndex,true))
								{
									if(EditorReplaceAll(theBuffer,sourceSelection.theSelection,findBuffer->textUniverse,replaceBuffer->textUniverse,backwards,wrap,regex,ignoreCase,limitScope,replaceScript,&foundMatch,destSelection.theSelection))
									{
										CreateSearchResult(tempString,foundMatch,destSelection.theSelection);
										Tcl_AppendResult(localInterpreter,tempString,NULL);
									}
									else
									{
										GetError(&errorFamily,&errorFamilyMember,&errorDescription);
										Tcl_AppendResult(localInterpreter,errorDescription,NULL);
										fail=true;
									}
								}
								else
								{
									fail=true;
								}
								ClearBufferBusy(replaceBuffer);
							}
							else
							{
								fail=true;
							}
						}
						else
						{
							fail=true;
						}
						ClearBufferBusy(findBuffer);
					}
				}
				else
				{
					fail=true;
				}
				ClearBufferBusy(theBuffer);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"searchInBuffer findBuffer replaceBuffer ?flags?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_SelectAll(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform a select all in the given buffer
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				EditorSelectAll(theBuffer);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_SelectedTextList(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the currently selected text
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		currentPosition;
	UINT32
		actualLength;
	UINT8
		*selectionBuffer;
	bool
		done;
	bool
		fail;

	fail=false;
	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				currentPosition=0;
				done=false;
				while(!done&&!fail)
				{
					if((selectionBuffer=EditorNextSelectionToBuffer(theBuffer,theBuffer->selectionUniverse,&currentPosition,1,&actualLength,&done)))
					{
						selectionBuffer[actualLength-1]='\0';					// terminate the string
						Tcl_AppendElement(localInterpreter,(char *)selectionBuffer);
						MDisposePtr(selectionBuffer);
					}
					else
					{
						if(!done)
						{
							GetError(&errorFamily,&errorFamilyMember,&errorDescription);
							Tcl_ResetResult(localInterpreter);
							Tcl_AppendResult(localInterpreter,errorDescription,NULL);
							fail=true;
						}
					}
				}
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_SelectLine(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// locate the given line, and select it
// if lineNumber is out of range, go to the nearest one
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		theLine;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(GetUINT32(localInterpreter,objv[2],&theLine))
				{
					if(theLine==0)		// if user enters 0, just take it as 1
					{
						theLine=1;
					}
					SetBufferBusy(theBuffer);
					EditorLocateLine(theBuffer,theLine-1);
					ClearBufferBusy(theBuffer);
					return(TCL_OK);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName lineNumber");
	}
	return(TCL_ERROR);
}

static int Cmd_GetSelectionEnds(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the start and end of the current selection
// of the passed buffer
// if there is no selection, the cursor position is returned in both values
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		startPosition,
		endPosition;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				GetSelectionEndPositions(theBuffer->selectionUniverse,&startPosition,&endPosition);
				sprintf(tempString,"%u %u",startPosition,endPosition);
				Tcl_AppendResult(localInterpreter,tempString,NULL);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_SetSelectionEnds(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the selection of the passed buffer
// if the given end position is < the start position,
// the positions will be reversed
// if any position is out of range, it will be forced in range
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		startPosition,
		endPosition;

	if(objc==4)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(GetUINT32(localInterpreter,objv[2],&startPosition))
				{
					if(GetUINT32(localInterpreter,objv[3],&endPosition))
					{
						SetBufferBusy(theBuffer);
						startPosition=ForcePositionIntoRange(theBuffer,startPosition);
						endPosition=ForcePositionIntoRange(theBuffer,endPosition);
						ArrangePositions(&startPosition,&endPosition);
						EditorSetNormalSelection(theBuffer,theBuffer->selectionUniverse,startPosition,endPosition);
						ClearBufferBusy(theBuffer);
						return(TCL_OK);
					}
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName startPosition endPosition");
	}
	return(TCL_ERROR);
}

static int Cmd_GetSelectionEndList(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return offsets of the ends of each selection segment
// if there is no selection, an empty list is returned
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		currentPosition,
		currentLength;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				currentPosition=0;
				while(GetSelectionAtOrAfterPosition(theBuffer->selectionUniverse,currentPosition,&currentPosition,&currentLength))
				{
					sprintf(tempString,"%u %u",currentPosition,currentPosition+currentLength);
					Tcl_AppendElement(localInterpreter,tempString);
					currentPosition+=currentLength;
				}
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_AddSelectionEndList(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// add list of ends given to the selection
// if any position is out of range, it will be forced in range
// if there is any overlap, the new segment boundaries will remain
{
	EDITORBUFFER
		*theBuffer;
	int
		listArgc;
	char
		**listArgv;
	int
		elementArgc;
	char
		**elementArgv;
	UINT32
		startPosition,
		endPosition;
	int
		i;
	bool
		fail;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(Tcl_SplitList(localInterpreter,Tcl_GetString(objv[2]),&listArgc,(const char ***)&listArgv)==TCL_OK)
				{
					SetBufferBusy(theBuffer);
					EditorStartSelectionChange(theBuffer);
					fail=false;
					for(i=0;!fail&&i<listArgc;i++)
					{
						if(Tcl_SplitList(localInterpreter,listArgv[i],&elementArgc,(const char ***)&elementArgv)==TCL_OK)
						{
							if(elementArgc==2)
							{
								if(GetUINT32String(localInterpreter,elementArgv[0],&startPosition))
								{
									if(GetUINT32String(localInterpreter,elementArgv[1],&endPosition))
									{
										startPosition=ForcePositionIntoRange(theBuffer,startPosition);
										endPosition=ForcePositionIntoRange(theBuffer,endPosition);
										ArrangePositions(&startPosition,&endPosition);
										if(endPosition>startPosition)	// make sure there is at least something to select
										{
											if(!SetSelectionRange(theBuffer->selectionUniverse,startPosition,endPosition-startPosition))
											{
												Tcl_AppendResult(localInterpreter,"Failed to set selection",NULL);
												fail=true;
											}
										}
									}
									else
									{
										fail=true;
									}
								}
								else
								{
									fail=true;
								}
							}
							else
							{
								Tcl_AppendResult(localInterpreter,"Exactly two elements are allowed per list entry",NULL);
								fail=true;
							}
							Tcl_Free((char *)elementArgv);
						}
						else
						{
							fail=true;
						}
					}
					EditorEndSelectionChange(theBuffer);
					ClearBufferBusy(theBuffer);
					Tcl_Free((char *)listArgv);
					if(!fail)
					{
						return(TCL_OK);
					}
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName selectionList");
	}
	return(TCL_ERROR);
}

static int Cmd_GetSelectionAtPosition(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return information about the selection at the given character position in the text
// The result is a value (0 if no selection, 1 if selection) and the position <= the given
// one where the current selection begins, and the position just after the end of the
// selection
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		thePosition;
	UINT32
		startPosition,
		numElements;
	bool
		isSelected;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(GetUINT32(localInterpreter,objv[2],&thePosition))
			{
				thePosition=ForcePositionIntoRange(theBuffer,thePosition);
				if(!GetSelectionRange(theBuffer->selectionUniverse,thePosition,&startPosition,&numElements,&isSelected))
				{
					numElements=theBuffer->textUniverse->totalBytes-startPosition;
				}
				sprintf(tempString,"%u %u %u",isSelected?1:0,startPosition,startPosition+numElements);
				Tcl_AppendElement(localInterpreter,tempString);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName position");
	}
	return(TCL_ERROR);
}

static int Cmd_SetMark(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set a mark in the given buffer
{
	EDITORBUFFER
		*theBuffer;
	MARKLIST
		*theMark;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				theMark=SetEditorMark(theBuffer,Tcl_GetString(objv[2]));
				ClearBufferBusy(theBuffer);
				if(theMark)
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName markName");
	}
	return(TCL_ERROR);
}

static int Cmd_ClearMark(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// clear a mark in the given buffer
{
	EDITORBUFFER
		*theBuffer;
	MARKLIST
		*theMark;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				if((theMark=LocateEditorMark(theBuffer,Tcl_GetString(objv[2]))))
				{
					ClearEditorMark(theBuffer,theMark);
				}
				ClearBufferBusy(theBuffer);
				if(theMark)
				{
					return(TCL_OK);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Failed to locate mark '",Tcl_GetString(objv[2]),"'",NULL);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName markName");
	}
	return(TCL_ERROR);
}

static int Cmd_GotoMark(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// goto a mark in the given buffer
{
	EDITORBUFFER
		*theBuffer;
	MARKLIST
		*theMark;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				if((theMark=LocateEditorMark(theBuffer,Tcl_GetString(objv[2]))))
				{
					GotoEditorMark(theBuffer,theMark);
				}
				ClearBufferBusy(theBuffer);
				if(theMark)
				{
					return(TCL_OK);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Failed to locate mark '",Tcl_GetString(objv[2]),"'",NULL);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName markName");
	}
	return(TCL_ERROR);
}

static int Cmd_MarkList(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return a list of marks for the given buffer
{
	EDITORBUFFER
		*theBuffer;
	MARKLIST
		*theMark;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			theMark=theBuffer->theMarks;
			while(theMark)
			{
				if(theMark->markName)
				{
					Tcl_AppendElement(localInterpreter,theMark->markName);
				}
				theMark=theMark->nextMark;
			}
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static OPTIONDESCRIPTORRECORD homeOptions[]=
{
	{"-lenient",		HT_LENIENT,		OptionEnum},
	{"-semistrict",		HT_SEMISTRICT,	OptionEnum},
	{"-strict",			HT_STRICT,		OptionEnum},
	{"-start",			false,			OptionBoolean},
	{"-end",			true,			OptionBoolean},
	{NULL,0,NULL}
};

static int Cmd_HomeWindow(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// home a document view to a given position, in the given way
// by default, the mode is HT_SEMISTRICT, and useEnd is false
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	UINT32
		startPosition,
		endPosition;
	bool
		fail;
	int
		mode;
	bool
		useEnd;
	void
		*parsedVars[]=
		{
			(void *)&mode,
			(void *)&mode,
			(void *)&mode,
			(void *)&useEnd,
			(void *)&useEnd,
		};
	int
		optionIndex;

	fail=false;
	if(objc>=4)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
			{
				if(GetUINT32(localInterpreter,objv[2],&startPosition))
				{
					if(GetUINT32(localInterpreter,objv[3],&endPosition))
					{
						startPosition=ForcePositionIntoRange(theWindow->theBuffer,startPosition);
						endPosition=ForcePositionIntoRange(theWindow->theBuffer,endPosition);
						ArrangePositions(&startPosition,&endPosition);
						optionIndex=4;

						mode=HT_SEMISTRICT;
						useEnd=false;

						if(ParseOptions(localInterpreter,homeOptions,parsedVars,objc,objv,&optionIndex,true))
						{
							SetBufferBusy(theWindow->theBuffer);
							theView=GetDocumentWindowCurrentView(theWindow);
							EditorHomeViewToRange(theView,startPosition,endPosition,useEnd,mode,mode);
							ResetEditorViewCursorBlink(theView);				// reset cursor blinking when homing view
							ClearBufferBusy(theWindow->theBuffer);
						}
						else
						{
							fail=true;
						}

					}
					else
					{
						fail=true;
					}
				}
				else
				{
					fail=true;
				}
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"theWindow startPosition endPosition ?flags?");
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_GetTopLeft(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// get the top line, and lefthand coordinate of the active view in the given window
// NOTE: to be consistent with line numbering starting at 1, we fudge the number in here
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
			{
				SetBufferBusy(theWindow->theBuffer);
				theView=GetDocumentWindowCurrentView(theWindow);
				GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
				sprintf(tempString,"%u %d",topLine+1,leftPixel);
				Tcl_AppendResult(localInterpreter,tempString,NULL);
				ClearBufferBusy(theWindow->theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName");
	}
	return(TCL_ERROR);
}

static int Cmd_SetTopLeft(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the top line, and lefthand coordinate of the active view in the given window
// NOTE: to be consistent with line numbering starting at 1, we fudge the number in here
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	UINT32
		topLine;
	INT32
		leftPixel;

	if(objc==4)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(GetUINT32(localInterpreter,objv[2],&topLine))
			{
				if(GetINT32(localInterpreter,objv[3],&leftPixel))
				{
					if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
					{
						SetBufferBusy(theWindow->theBuffer);
						theView=GetDocumentWindowCurrentView(theWindow);
						if(topLine==0)				// if line 0 was given, just make it line 1
						{
							topLine=1;
						}
						SetViewTopLeft(theView,topLine-1,leftPixel);
						ClearBufferBusy(theWindow->theBuffer);
						return(TCL_OK);
					}
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName topLine leftPixel");
	}
	return(TCL_ERROR);
}

static int Cmd_TextInfo(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the number of lines, and number of characters of text in the given buffer
{
	EDITORBUFFER
		*theBuffer;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				sprintf(tempString,"%u %u",theBuffer->textUniverse->totalLines,theBuffer->textUniverse->totalBytes);
				Tcl_AppendResult(localInterpreter,tempString,NULL);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_SelectionInfo(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return information about the selection:
// startPosition,endPosition,startLine,endLine,startLinePosition,endLinePosition,totalSegments,totalSpan
// NOTE: to be consistent with line numbering starting at 1, we fudge the numbers in here
{
	UINT32
		startPosition,						// selection information
		endPosition,
		startLine,
		endLine,
		startLinePosition,
		endLinePosition,
		totalSegments,
		totalSpan;
	EDITORBUFFER
		*theBuffer;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				EditorGetSelectionInfo(theBuffer,theBuffer->selectionUniverse,&startPosition,&endPosition,&startLine,&endLine,&startLinePosition,&endLinePosition,&totalSegments,&totalSpan);
				sprintf(tempString,"%u %u %u %u %u %u %u %u",startPosition,endPosition,startLine+1,endLine+1,startLinePosition,endLinePosition,totalSegments,totalSpan);
				Tcl_AppendResult(localInterpreter,tempString,NULL);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_PositionToLineOffset(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// given a position in a buffer, return a line number, and offset in characters into the line
// that matches the position
// NOTE: to be consistent with line numbering starting at 1, we fudge the number in here
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		thePosition,
		theLine,
		theLineOffset;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(GetUINT32(localInterpreter,objv[2],&thePosition))
			{
				if(ShellBufferNotBusy(localInterpreter,theBuffer))
				{
					SetBufferBusy(theBuffer);
					PositionToLinePosition(theBuffer->textUniverse,thePosition,&theLine,&theLineOffset,&theChunk,&theOffset);
					sprintf(tempString,"%u %u",theLine+1,theLineOffset);
					Tcl_AppendResult(localInterpreter,tempString,NULL);
					ClearBufferBusy(theBuffer);
					return(TCL_OK);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName position");
	}
	return(TCL_ERROR);
}

static int Cmd_LineOffsetToPosition(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// given a line, and offset in a buffer, return a position in the buffer that matches
// NOTE: to be consistent with line numbering starting at 1, we fudge the number in here
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		thePosition,
		theLine,
		theLineOffset,
		distanceMoved;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==4)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(GetUINT32(localInterpreter,objv[2],&theLine))
			{
				if(GetUINT32(localInterpreter,objv[3],&theLineOffset))
				{
					if(ShellBufferNotBusy(localInterpreter,theBuffer))
					{
						SetBufferBusy(theBuffer);
						if(theLine==0)
						{
							theLine=1;
						}
						LineToChunkPosition(theBuffer->textUniverse,theLine-1,&theChunk,&theOffset,&thePosition);	// get position that is the start of the given line
						ChunkPositionToNextLine(theBuffer->textUniverse,theChunk,theOffset,&theChunk,&theOffset,&distanceMoved);
						if(theLineOffset>distanceMoved)		// see if offset is past end of line, if so, pin it
						{
							theLineOffset=distanceMoved;
						}
						sprintf(tempString,"%u",thePosition+theLineOffset);
						Tcl_AppendResult(localInterpreter,tempString,NULL);
						ClearBufferBusy(theBuffer);
						return(TCL_OK);
					}
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName lineNumber offset");
	}
	return(TCL_ERROR);
}

static int Cmd_GetStyleAtPosition(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return information about the style at the given character position in the text
// The result is a value (style index) and the position <= the given
// one where the style begins, and the position just after the end of the
// style
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		thePosition;
	UINT32
		startPosition,
		numElements;
	UINT32
		theStyle;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(GetUINT32(localInterpreter,objv[2],&thePosition))
			{
				thePosition=ForcePositionIntoRange(theBuffer,thePosition);
				if(!GetStyleRange(theBuffer->styleUniverse,thePosition,&startPosition,&numElements,&theStyle))
				{
					numElements=theBuffer->textUniverse->totalBytes-startPosition;
				}
				sprintf(tempString,"%u %u %u",theStyle,startPosition,startPosition+numElements);
				Tcl_AppendElement(localInterpreter,tempString);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName position");
	}
	return(TCL_ERROR);
}

static int Cmd_SetStyle(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the style of text of the passed buffer
// if the given end position is < the start position,
// the positions will be reversed
// if any position is out of range, it will be forced in range
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		startPosition,
		endPosition;
	UINT32
		theStyle;

	if(objc==5)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(GetUINT32(localInterpreter,objv[2],&startPosition))
				{
					if(GetUINT32(localInterpreter,objv[3],&endPosition))
					{
						if(GetUINT32(localInterpreter,objv[4],&theStyle))
						{
							SetBufferBusy(theBuffer);
							startPosition=ForcePositionIntoRange(theBuffer,startPosition);
							endPosition=ForcePositionIntoRange(theBuffer,endPosition);
							ArrangePositions(&startPosition,&endPosition);
							EditorStartStyleChange(theBuffer);
							SetStyleRange(theBuffer->styleUniverse,startPosition,endPosition-startPosition,theStyle);
							EditorEndStyleChange(theBuffer);
							ClearBufferBusy(theBuffer);
							return(TCL_OK);
						}
					}
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName startPosition endPosition styleNum");
	}
	return(TCL_ERROR);
}

static int Cmd_SelectionToStyle(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// Set the style of a buffer to a given value anywhere text is selected
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		theStyle;
	bool
		fail;
	UINT32
		startPosition,
		numElements;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(GetUINT32(localInterpreter,objv[2],&theStyle))
				{
					SetBufferBusy(theBuffer);
					EditorStartStyleChange(theBuffer);
					fail=false;

					startPosition=0;
					while(!fail&&GetSelectionAtOrAfterPosition(theBuffer->selectionUniverse,startPosition,&startPosition,&numElements))
					{
						fail=!SetStyleRange(theBuffer->styleUniverse,startPosition,numElements,theStyle);
						startPosition+=numElements;
					}

					EditorEndStyleChange(theBuffer);
					ClearBufferBusy(theBuffer);
					return(TCL_OK);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName styleNum");
	}
	return(TCL_ERROR);
}

static int Cmd_StyleToSelection(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// Set the selection in a buffer everywhere a given style is in the buffer
// NOTE: this does not clear the old selection, it will just add to it
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		theStyle;
	bool
		fail;
	UINT32
		startPosition,
		numElements;
	UINT32
		sawStyle;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(GetUINT32(localInterpreter,objv[2],&theStyle))
				{
					SetBufferBusy(theBuffer);
					EditorStartSelectionChange(theBuffer);

					fail=false;
					startPosition=0;
					while(!fail&&GetStyleRange(theBuffer->styleUniverse,startPosition,&startPosition,&numElements,&sawStyle))
					{
						if(sawStyle==theStyle)
						{
							fail=!SetSelectionRange(theBuffer->selectionUniverse,startPosition,numElements);
						}
						startPosition+=numElements;
					}
					if(!fail)	// handle the last element if it happens to be in the default style
					{
						if(sawStyle==theStyle)
						{
							fail=!SetSelectionRange(theBuffer->selectionUniverse,startPosition,theBuffer->textUniverse->totalBytes-startPosition);
						}
					}

					EditorEndSelectionChange(theBuffer);
					ClearBufferBusy(theBuffer);
					return(TCL_OK);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName styleNum");
	}
	return(TCL_ERROR);
}

static void DisposeSplitExpressionName(char *theName)
// get rid of a split expression name created by SplitExpressionName
{
	MDisposePtr(theName);
}

static char *SplitExpressionName(char *theName,bool *useRegisterMatch,UINT32 *registerIndex)
// Split theName into a name, and information about any register match.
// Expression names may be specified as just a name, or a name followed by a ':'
// and then a register number.
// NOTE: if this fails, it will SetError, and return NULL
// NOTE: if this succeeds, the returned name should be disposed of by calling
// DisposeSplitExpressionName
{
	int
		theLength;
	char
		*newName;

	*useRegisterMatch=false;

	theLength=strlen(theName);
	if((newName=(char *)MNewPtr(theLength+1)))		// +1 to get space for termination
	{
		strcpy(newName,theName);					// copy it over
		if(theLength>=2)
		{
			if(newName[theLength-2]==':'&&newName[theLength-1]>='0'&&newName[theLength-1]<='9')	// see if indication of register specification
			{
				*useRegisterMatch=true;
				*registerIndex=newName[theLength-1]-'0';
				newName[theLength-2]='\0';	// terminate
			}
		}
	}
	return(newName);
}

static EXPRESSIONPIECE *GetExpressionPiece(Tcl_Interp *localInterpreter,SYNTAXMAP *theMap,char *theName)
// See if an expression piece of the given name can be added to the list
// If there is a problem, add it to the Tcl result, and return NULL
{
	char
		*newName;
	bool
		useRegisterMatch;
	UINT32
		registerIndex;
	SYNTAXEXPRESSION
		*theExpression;
	EXPRESSIONPIECE
		*thePiece;

	thePiece=NULL;
	if((newName=SplitExpressionName(theName,&useRegisterMatch,&registerIndex)))
	{
		if((theExpression=LocateSyntaxMapExpression(theMap,newName)))
		{
			if(!(thePiece=AddPieceToExpression(theExpression,useRegisterMatch,registerIndex)))
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				Tcl_AppendResult(localInterpreter,"Failed to add piece to expression '",newName,"': ",errorDescription,NULL);
			}
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"Failed to locate expression '",newName,"'",NULL);
		}
		DisposeSplitExpressionName(newName);
	}
	else
	{
		Tcl_AppendResult(localInterpreter,"Failed to split name '",theName,"'",NULL);
	}
	return(thePiece);
}

static bool ParseEndExpressions(Tcl_Interp *localInterpreter,SYNTAXMAP *theMap,SYNTAXSTYLEMAPPING *theMapping,char *endExpressionList)
// parse up the ending expressions, and add them to theMapping
// If there is a problem, add it to the Tcl result, and return false
{
	EXPRESSIONPIECE
		*expressionPiece;
	int
		argc;
	char
		**argv;
	int
		i;
	bool
		fail;

	fail=false;
	if(Tcl_SplitList(localInterpreter,endExpressionList,&argc,(const char ***)&argv)==TCL_OK)	// get list of end expressions
	{
		for(i=0;!fail&&i<argc;i++)
		{
			if((expressionPiece=GetExpressionPiece(localInterpreter,theMap,argv[i])))
			{
				if(AddMappingEndExpressionPiece(theMapping,expressionPiece))
				{
				}
				else
				{
					GetError(&errorFamily,&errorFamilyMember,&errorDescription);
					Tcl_AppendResult(localInterpreter,"Failed to end expression '",argv[i],"' to mapping: ",errorDescription,NULL);
					fail=true;
				}
			}
			else
			{
				fail=true;
			}
		}
		Tcl_Free((char *)argv);
	}
	else
	{
		fail=true;				// SplitList will leave a result if it fails
	}
	return(!fail);
}

static bool ParseMapping(Tcl_Interp *localInterpreter,SYNTAXMAP *theMap,int argc,char *argv[])
// Use the arguments to build a mapping onto theMap
// If there is a problem, add it to the Tcl result, and return false
{
	SYNTAXSTYLEMAPPING
		*theMapping;
	EXPRESSIONPIECE
		*expressionPiece;
	UINT32
		startStyle,
		betweenStyle,
		endStyle;

	if((theMapping=AddMappingToSyntaxMap(theMap,argv[0])))
	{
		if((expressionPiece=GetExpressionPiece(localInterpreter,theMap,argv[1])))
		{
			SetMappingStartExpressionPiece(theMapping,expressionPiece);
			if(ParseEndExpressions(localInterpreter,theMap,theMapping,argv[2]))
			{
				if(GetUINT32String(localInterpreter,argv[3],&startStyle))
				{
					if(GetUINT32String(localInterpreter,argv[4],&betweenStyle))
					{
						if(GetUINT32String(localInterpreter,argv[5],&endStyle))
						{
							SetMappingStyles(theMapping,startStyle,betweenStyle,endStyle);
							return(true);
						}
					}
				}
			}
		}
		RemoveMappingFromSyntaxMap(theMap,theMapping);
	}
	else
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		Tcl_AppendResult(localInterpreter,"Failed to add mapping '",argv[0],"' to map: ",errorDescription,NULL);
	}
	return(false);
}

static bool ParseSyntaxMap(Tcl_Interp *localInterpreter,SYNTAXMAP *theMap,char *mapContents)
// Parse the mapContents, and use it to build theMap
// If there is a problem, add it to the Tcl result, and return false
{
	bool
		fail;
	int
		listArgc;
	char
		**listArgv;
	int
		argc;
	char
		**argv;
	int
		mappinglistArgc;
	char
		**mappinglistArgv;
	int
		syntaxMapToken;
	SYNTAXSTYLEMAPPING
		*theMapping,
		*otherMapping;
	int
		i,j;

	fail=false;
	if(Tcl_SplitList(localInterpreter,mapContents,&listArgc,(const char ***)&listArgv)==TCL_OK)	// make list of command/parameter
	{
		for(i=0;(i<listArgc)&&!fail;i+=2)					// step over each command, parameter list pair
		{
			if(MatchToken(listArgv[i],syntaxMapCommands,&syntaxMapToken))
			{
				if(Tcl_SplitList(localInterpreter,listArgv[i+1],&argc,(const char ***)&argv)==TCL_OK)	// make parameters into arguments
				{
					switch(syntaxMapToken)
					{
						case SM_EXPRESSION:
							if(argc==2)
							{
								if(AddExpressionToSyntaxMap(theMap,argv[0],argv[1]))
								{
								}
								else
								{
									GetError(&errorFamily,&errorFamilyMember,&errorDescription);
									Tcl_AppendResult(localInterpreter,"Failed to add expression '",argv[0],"' to map: ",errorDescription,NULL);
									fail=true;
								}
							}
							else
							{
								Tcl_AppendResult(localInterpreter,"wrong # args: should be \"",listArgv[i]," expressionName expression\"",NULL);
								fail=true;
							}
							break;
						case SM_MAPPING:
							if(argc==6)
							{
								fail=!ParseMapping(localInterpreter,theMap,argc,argv);
							}
							else
							{
								Tcl_AppendResult(localInterpreter,"wrong # args: should be \"",listArgv[i]," mapName startExpressionName endExpressionList startStyle betweenStyle endStyle\"",NULL);
								fail=true;
							}
							break;
						case SM_AT:
							if(argc==2)
							{
								if((theMapping=LocateSyntaxStyleMapping(theMap,argv[0])))
								{
									if(Tcl_SplitList(localInterpreter,argv[1],&mappinglistArgc,(const char ***)&mappinglistArgv)==TCL_OK)	// make parameters into arguments
									{
										for(j=0;j<mappinglistArgc&&!fail;j++)
										{
											if((otherMapping=LocateSyntaxStyleMapping(theMap,mappinglistArgv[j])))
											{
												if(AddBetweenListElementToMapping(theMapping,otherMapping))
												{
												}
												else
												{
													GetError(&errorFamily,&errorFamilyMember,&errorDescription);
													Tcl_AppendResult(localInterpreter,"Failed to add mapping '",mappinglistArgv[j],"': ",errorDescription,NULL);
													fail=true;
												}
											}
											else
											{
												Tcl_AppendResult(localInterpreter,"Failed to locate mapping '",mappinglistArgv[j],"'",NULL);
												fail=true;
											}
										}
										Tcl_Free((char *)mappinglistArgv);
									}
									else
									{
										fail=true;
									}
								}
								else
								{
									Tcl_AppendResult(localInterpreter,"Failed to locate mapping '",argv[0],"'",NULL);
									fail=true;
								}
							}
							else
							{
								Tcl_AppendResult(localInterpreter,"wrong # args: should be \"",listArgv[i]," mapName betweenList\"",NULL);
								fail=true;
							}
							break;
						case SM_ROOT:
							for(j=0;j<argc&&!fail;j++)
							{
								if((otherMapping=LocateSyntaxStyleMapping(theMap,argv[j])))
								{
									if(AddRootListElementToMap(theMap,otherMapping))
									{
									}
									else
									{
										GetError(&errorFamily,&errorFamilyMember,&errorDescription);
										Tcl_AppendResult(localInterpreter,"Failed to add root mapping '",argv[j],"': ",errorDescription,NULL);
										fail=true;
									}
								}
								else
								{
									Tcl_AppendResult(localInterpreter,"Failed to locate mapping '",argv[j],"'",NULL);
									fail=true;
								}
							}
							break;
					}
					Tcl_Free((char *)argv);
				}
				else
				{
					fail=true;		// SplitList will leave a result if it fails
				}
			}
			else
			{
				Tcl_AppendResult(localInterpreter,"Invalid syntax map command '",listArgv[i],"'",NULL);
				fail=true;
			}
		}
		Tcl_Free((char *)listArgv);
	}
	else
	{
		fail=true;		// SplitList will leave a result if it fails
	}
	return(!fail);
}

static int Cmd_AddSyntaxMap(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// create a syntax highlighting map from a list of expressions and a list of mappings from expressions to styles
// NOTE: this can be used to replace an existing syntax map, but
// if the existing map is in use by any buffer which is busy, this will fail
// before it does anything
{
	EDITORBUFFER
		*theBuffer,
		*nextBuffer;
	SYNTAXMAP
		*oldMap,
		*theMap;
	bool
		fail;

	if(objc==3)
	{
		if((theMap=OpenSyntaxMap(Tcl_GetString(objv[1]))))
		{
			if(ParseSyntaxMap(localInterpreter,theMap,Tcl_GetString(objv[2])))
			{
				fail=false;
				if((oldMap=LocateSyntaxMap(Tcl_GetString(objv[1]))))	// see if there is an old map with this name that is about to be replaced
				{
					theBuffer=NULL;
					while(!fail&&(theBuffer=LocateNextEditorBufferOnMap(oldMap,theBuffer)))	// verify all buffers are not busy
					{
						fail=!ShellBufferNotBusy(localInterpreter,theBuffer);
					}
					if(!fail)							// move all buffers from old map to new map
					{
						nextBuffer=LocateNextEditorBufferOnMap(oldMap,NULL);
						while(nextBuffer)				// re-assign the maps
						{
							theBuffer=nextBuffer;		// point at the current buffer
							nextBuffer=LocateNextEditorBufferOnMap(oldMap,nextBuffer);	// move to the next before we do any modification to the current
							AssignSyntaxMap(theBuffer,theMap);	// if this fails, there's not much we can do anyway, so ignore it
						}
						UnlinkSyntaxMap(oldMap);
						CloseSyntaxMap(oldMap);
					}
				}
				if(!fail)
				{
					if(LinkSyntaxMap(theMap))	// link this map onto the global list
					{
						return(TCL_OK);
					}
					else
					{
						GetError(&errorFamily,&errorFamilyMember,&errorDescription);
						Tcl_AppendResult(localInterpreter,"Error linking syntax map '",Tcl_GetString(objv[1]),"': ",errorDescription,NULL);

						// failed to link new map to list, so untie any buffers that got linked to it
						theBuffer=NULL;
						while((theBuffer=LocateNextEditorBufferOnMap(theMap,theBuffer)))
						{
							AssignSyntaxMap(theBuffer,NULL);
						}
					}
				}
			}
			CloseSyntaxMap(theMap);
		}
		else
		{
			GetError(&errorFamily,&errorFamilyMember,&errorDescription);
			Tcl_AppendResult(localInterpreter,"Error creating syntax map '",Tcl_GetString(objv[1]),"': ",errorDescription,NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"syntaxMapName syntaxMap");
	}
	return(TCL_ERROR);
}

static int Cmd_DeleteSyntaxMap(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// delete a syntax map that was created by "syntaxmap"
// Remove the map from all buffers which are using it
// NOTE: if any buffer which is using the map is currently busy, this will
// fail before it does anything.
{
	EDITORBUFFER
		*theBuffer,
		*nextBuffer;
	SYNTAXMAP
		*theMap;
	bool
		fail;

	if(objc==2)
	{
		if((theMap=LocateSyntaxMap(Tcl_GetString(objv[1]))))	// find map to delete
		{
			fail=false;
			theBuffer=NULL;
			while(!fail&&(theBuffer=LocateNextEditorBufferOnMap(theMap,theBuffer)))	// verify all buffers are not busy
			{
				fail=!ShellBufferNotBusy(localInterpreter,theBuffer);
			}
			if(!fail)							// remove all buffers from the map, and kill the map
			{
				nextBuffer=LocateNextEditorBufferOnMap(theMap,NULL);
				while(nextBuffer)				// re-assign the maps
				{
					theBuffer=nextBuffer;		// point at the current buffer
					nextBuffer=LocateNextEditorBufferOnMap(theMap,nextBuffer);	// move to the next before we do any modification to the current
					AssignSyntaxMap(theBuffer,NULL);	// if this fails, there's not much we can do anyway, so ignore it
				}
				UnlinkSyntaxMap(theMap);
				CloseSyntaxMap(theMap);
				return(TCL_OK);
			}
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"Could not locate syntax map '",Tcl_GetString(objv[1]),"'",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"syntaxMap");
	}
	return(TCL_ERROR);
}

static int Cmd_SyntaxMaps(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the list of currently defined syntax maps
{
	bool
		fail;
	SYNTAXMAP
		*currentMap;

	fail=false;
	if(objc==1)
	{
		currentMap=NULL;
		while((currentMap=LocateNextSyntaxMap(currentMap)))
		{
			Tcl_AppendElement(localInterpreter,GetSyntaxMapName(currentMap));
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_SetSyntaxMap(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// Set the syntax map in use for the given buffer
// If an empty map string is specified, then remove any map from the current buffer
{
	EDITORBUFFER
		*theBuffer;
	SYNTAXMAP
		*theMap;
	char
		*mapName;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				theMap=NULL;
				mapName=Tcl_GetString(objv[2]);
				if(mapName[0]=='\0'||(theMap=LocateSyntaxMap(mapName)))
				{
					if(AssignSyntaxMap(theBuffer,theMap))
					{
						return(TCL_OK);
					}
					else
					{
						GetError(&errorFamily,&errorFamilyMember,&errorDescription);
						Tcl_AppendResult(localInterpreter,"Failed to assign syntax map: ",errorDescription,NULL);
					}
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Could not locate syntax map '",mapName,"'",NULL);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName syntaxMapName");
	}
	return(TCL_ERROR);
}

static int Cmd_GetSyntaxMap(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// read back the syntax map in use for the given buffer
{
	EDITORBUFFER
		*theBuffer;
	SYNTAXMAP
		*theMap;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if((theMap=GetAssignedSyntaxMap(theBuffer)))
			{
				Tcl_AppendElement(localInterpreter,GetSyntaxMapName(theMap));
			}
			else
			{
				Tcl_AppendElement(localInterpreter,"");
			}
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_SyntaxMapBuffers(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// Return the list of buffer names which are currently using the given syntax map
{
	EDITORBUFFER
		*theBuffer;
	SYNTAXMAP
		*theMap;

	if(objc==2)
	{
		if((theMap=LocateSyntaxMap(Tcl_GetString(objv[1]))))
		{
			theBuffer=NULL;
			while((theBuffer=LocateNextEditorBufferOnMap(theMap,theBuffer)))
			{
				Tcl_AppendElement(localInterpreter,theBuffer->contentName);
			}
			return(TCL_OK);
		}
		else
		{
			Tcl_AppendResult(localInterpreter,"Could not locate syntax map '",Tcl_GetString(objv[1]),"'",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"syntaxMapName");
	}
	return(TCL_ERROR);
}

static int Cmd_Clear(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform a selection clear in the given buffer
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				EditorDeleteSelection(theBuffer,theBuffer->selectionUniverse);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_Insert(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// insert the given text into the current selection
// of the passed buffer
{
	EDITORBUFFER
		*theBuffer;
	char
		*dataToAdd;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				dataToAdd=Tcl_GetString(objv[2]);
				EditorInsert(theBuffer,(UINT8 *)dataToAdd,strlen(dataToAdd));
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName textToInsert");
	}
	return(TCL_ERROR);
}

static int Cmd_InsertFile(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// insert the given file into the current selection
// of the passed buffer
{
	EDITORBUFFER
		*theBuffer;
	bool
		result;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				result=EditorInsertFile(theBuffer,Tcl_GetString(objv[2]));
				ClearBufferBusy(theBuffer);
				if(result)
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName pathName");
	}
	return(TCL_ERROR);
}

static int Cmd_AutoIndent(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// perform an autoindent on the passed buffer
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				EditorAutoIndent(theBuffer);
				ClearBufferBusy(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_MoveCursor(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// move the cursor in the given window by the relative mode
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	int
		relativeMode;

	if(objc==3)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
			{
				if(MatchToken(Tcl_GetString(objv[2]),cursorMovementTokens,&relativeMode))
				{
					SetBufferBusy(theWindow->theBuffer);
					theView=GetDocumentWindowCurrentView(theWindow);
					EditorMoveCursor(theView,(UINT16)relativeMode);
					ResetEditorViewCursorBlink(theView);				// reset cursor blinking when done moving
					ClearBufferBusy(theWindow->theBuffer);
					return(TCL_OK);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Invalid relative mode",NULL);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName relativeMode");
	}
	return(TCL_ERROR);
}

static int Cmd_Delete(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// delete the characters in the window between the current cursor position, and
// the given relative mode away, if there is a selection, it will be deleted instead
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	int
		relativeMode;

	if(objc==3)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
			{
				if(MatchToken(Tcl_GetString(objv[2]),cursorMovementTokens,&relativeMode))
				{
					SetBufferBusy(theWindow->theBuffer);
					theView=GetDocumentWindowCurrentView(theWindow);
					EditorDelete(theView,(UINT16)relativeMode);
					ClearBufferBusy(theWindow->theBuffer);
					return(TCL_OK);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Invalid relative mode",NULL);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName relativeMode");
	}
	return(TCL_ERROR);
}

static int Cmd_ExpandSelection(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// expand the selection in the given window in the given relative direction
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	int
		relativeMode;

	if(objc==3)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
			{
				if(MatchToken(Tcl_GetString(objv[2]),cursorMovementTokens,&relativeMode))
				{
					SetBufferBusy(theWindow->theBuffer);
					theView=GetDocumentWindowCurrentView(theWindow);
					EditorExpandNormalSelection(theView,(UINT16)relativeMode);
					ClearBufferBusy(theWindow->theBuffer);
					return(TCL_OK);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Invalid relative mode",NULL);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName relativeMode");
	}
	return(TCL_ERROR);
}

static int Cmd_ReduceSelection(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// reduce the selection in the given window in the given relative direction
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	int
		relativeMode;

	if(objc==3)
	{
		if((theWindow=GetEditorWindow(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theWindow->theBuffer))
			{
				if(MatchToken(Tcl_GetString(objv[2]),cursorMovementTokens,&relativeMode))
				{
					SetBufferBusy(theWindow->theBuffer);
					theView=GetDocumentWindowCurrentView(theWindow);
					EditorReduceNormalSelection(theView,(UINT16)relativeMode);
					ClearBufferBusy(theWindow->theBuffer);
					return(TCL_OK);
				}
				else
				{
					Tcl_AppendResult(localInterpreter,"Invalid relative mode",NULL);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"windowName relativeMode");
	}
	return(TCL_ERROR);
}

static int Cmd_SetWordChars(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the wordSpaceTable so that all of the characters in the passed argument
// are not considered space
{
	int
		i;
	char
		*theChars;

	if(objc==2)
	{
		for(i=0;i<256;i++)
		{
			wordSpaceTable[i]=true;
		}
		theChars=Tcl_GetString(objv[1]);

		for(i=0;theChars[i];i++)
		{
			wordSpaceTable[theChars[i]]=false;
		}
		return(TCL_OK);
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"wordCharacters");
	}
	return(TCL_ERROR);
}

static int Cmd_GetWordChars(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the list of characters that are considered parts of words
{
	char
		tempString[256];				// hold the list of characters that are part of a word
	int
		i,j;

	if(objc==1)
	{
		j=0;
		for(i=1;i<256;i++)				// never look at character 0, because it would terminate the string, and confuse things
		{
			if(!wordSpaceTable[i])
			{
				tempString[j++]=(char)i;	// set this one into the string
			}
		}
		tempString[j++]='\0';				// terminate the string
		Tcl_AppendElement(localInterpreter,&(tempString[0]));
		return(TCL_OK);
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

static int Cmd_Execute(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// execute the given tcl script, place the results in the given buffer
// if it still exists after the script is finished, otherwise throw them away.
{
	EDITORBUFFERHANDLE
		theHandle;
	EDITORBUFFER
		*theBuffer;
	UINT32
		startPosition,
		endPosition;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];
	int
		tclResult;
	const char
		*stringResult;

	if(objc==3)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				// NOTE: do not set busy here, we will allow executed script to mess with the window, but we must be careful:
				// If the buffer goes away as the result of executing the script, we have to make sure it is not referenced again

				EditorInitBufferHandle(&theHandle);
				EditorGrabBufferHandle(&theHandle,theBuffer);					// hold on to this buffer

				GetSelectionEndPositions(theBuffer->selectionUniverse,&startPosition,&endPosition);
				SetSelectionCursorPosition(theBuffer->auxSelectionUniverse,endPosition);	// move the aux cursor to the end of any selection/cursor, and start output there

				// do not call ClearAbort here, it will have already been called when the script that got us here was started
				tclResult=Tcl_EvalObj(localInterpreter,objv[2]);
				stringResult=Tcl_GetStringResult(localInterpreter);
				if(*stringResult)												// if no result, do not try to insert into buffer
				{
					if((theBuffer=EditorGetBufferFromHandle(&theHandle)))		// see if this buffer is still alive
					{
						if(tclResult!=TCL_OK)
						{
							sprintf(tempString,"Error in line %d: ",Tcl_GetErrorLine(localInterpreter));
							EditorAuxInsert(theBuffer,(UINT8 *)tempString,(UINT32)strlen(tempString));
						}
						EditorAuxInsert(theBuffer,(UINT8 *)stringResult,(UINT32)strlen(stringResult));
						EditorAuxInsert(theBuffer,(UINT8 *)"\n",1);
					}
				}
				EditorReleaseBufferHandle(&theHandle);							// do not need this any more
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName script");
	}
	return(TCL_ERROR);
}

static int Cmd_Task(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// start a task running in the given buffer, or send data to a task in a buffer
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		startPosition,
		endPosition;
	char
		*taskData,
		*completionProc = NULL;

	if(objc==3 || objc==4)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				GetSelectionEndPositions(theBuffer->selectionUniverse,&startPosition,&endPosition);
				SetSelectionCursorPosition(theBuffer->auxSelectionUniverse,endPosition);	// move the aux cursor to the end of any selection/cursor, and start output there
				taskData=Tcl_GetString(objv[2]);
				
				if(objc==4)
				{
					completionProc=Tcl_GetString(objv[3]);
				}
				
				if(WriteBufferTaskData(theBuffer,(UINT8 *)taskData,(UINT32)strlen(taskData), completionProc))
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName taskData ?completionProc?");
	}
	return(TCL_ERROR);
}

static int Cmd_UpdateTask(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// if a task is running in the given buffer, this can be called to
// update the status of the task
// If the buffer is busy, this will fail
// NOTE: while scripts are not running, the tasks on all buffers are
// continously updated, but while scripts are running, this must
// be called explicitly to get a buffer to update
{
	EDITORBUFFER
		*theBuffer;
	bool
		didUpdate;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(UpdateBufferTask(theBuffer,&didUpdate))
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_EOFTask(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// send an EOF to the task on the passed buffer, if there is a problem, complain
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(SendEOFToBufferTask(theBuffer))
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_KillTask(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// send a signal to the task on the passed buffer, if there is a problem, complain
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				if(KillBufferTask(theBuffer))
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
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_DetatchTask(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// disconect the task on the passed buffer from e93, if there is a problem, complain
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				DisconnectBufferTask(theBuffer);
				return(TCL_OK);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_TaskBytes(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the number of characters the last/current task has dumped into the given buffer
// NOTE: this count is only reset when as task is launched
{
	EDITORBUFFER
		*theBuffer;
	char
		tempString[TEMPSTRINGBUFFERLENGTH];

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			sprintf(tempString,"%u",theBuffer->taskBytes);
			Tcl_AppendResult(localInterpreter,tempString,NULL);
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_HasTask(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// see if a task is running in the given buffer, if so, return 1, else 0
{
	EDITORBUFFER
		*theBuffer;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			Tcl_AppendResult(localInterpreter,theBuffer->theTask?"1":"0",NULL);
			return(TCL_OK);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_Beep(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// make a noise
{
	if(objc==1)
	{
		EditorBeep();
		return(TCL_OK);
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
	}
	return(TCL_ERROR);
}

static int Cmd_CheckBuffer(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// check the text universe on the given buffer, report the results
// NOTE: this is mainly for debugging, and draws its own dialog boxes
{
	EDITORBUFFER
		*theBuffer;
	bool
		result;

	if(objc==2)
	{
		if((theBuffer=GetEditorBuffer(localInterpreter,objv[1])))
		{
			if(ShellBufferNotBusy(localInterpreter,theBuffer))
			{
				SetBufferBusy(theBuffer);
				result=UniverseSanityCheck(theBuffer->textUniverse);
				ClearBufferBusy(theBuffer);
				if(result)
				{
					return(TCL_OK);
				}
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"bufferName");
	}
	return(TCL_ERROR);
}

static int Cmd_ForceQuit(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// execute editor quit function (the editor WILL quit, no questions asked if you call this)
{
	bool
		fail;

	fail=false;
	if(objc==1)
	{
		EditorQuit();
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_Version(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// get version numbers
{
	char
		*localVersion;
	bool
		fail;

	fail=false;
	if(objc==1)
	{
		Tcl_AppendResult(localInterpreter,programName," ",VERSION,EDITION,NULL);
		if((localVersion=GetEditorLocalVersion()))
		{
			Tcl_AppendResult(localInterpreter," (",localVersion,")",NULL);
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,NULL);
		fail=true;
	}
	return(fail?TCL_ERROR:TCL_OK);
}

static int Cmd_ShellCommand(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// This exists to provide a stub for the shell commands which e93 will call internally
// for various functions. Normally, when the initial script is executed, it will define
// this function, but if the script does not define it, or fails before defining it,
// this acts as a safety net
{
	return(TCL_OK);
}

static int overrideCmd(const char *commandName, ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// forward to an original proc then see if we should un-suspend e93 because of what that may have done
{
	int
		result;
	Tcl_Obj
		**newObjv;
	int
		i;

	result=TCL_ERROR;
	if((newObjv=(Tcl_Obj **)MNewPtr(sizeof(Tcl_Obj *)*objc)))	// create an array of objects to copy parameters into
	{			
		newObjv[0]=Tcl_NewStringObj(commandName,-1);			// set the command to the original command
		Tcl_IncrRefCount(newObjv[0]);							// don't let this get deleted
		
		for(i=1;i<objc;i++)
		{
			newObjv[i] = Tcl_DuplicateObj(objv[i]);				// copy the argument
			Tcl_IncrRefCount(newObjv[i]);						// don't let this get deleted
		}
		
		EditorClearModal();										// let e93 start processing events again in case this command removes a grab
																// otherwise e93 can miss a mouseclick and loose focus

		result = Tcl_EvalObjv(localInterpreter, objc, newObjv, 0);
		
		for(i=0;i<objc;i++)
		{
			Tcl_DecrRefCount(newObjv[i]);						// allow new objects to be deleted
		}

		if(tkHasGrab())											// see if Tk has a grab -- if so, then become modal, otherwise don't
		{
			EditorSetModal();
		}
		else
		{
			EditorClearModal();
		}
		
		MDisposePtr(newObjv);
	}
	
	return result;
}

static int Cmd_Grab(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// forward to the original grab proc then see if we should un-suspend e93 because of what that may have done
{
	return overrideCmd("::override::grab", clientData, localInterpreter, objc, objv);
}

static int Cmd_Destroy(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// forward to the original destroy proc then see if we should un-suspend e93 because of what that may have done
{
	return overrideCmd("::override::destroy", clientData, localInterpreter, objc, objv);
}

static int Cmd_GetChannel(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// return the buffer being used for Tcl's channel named in objv 1
// if the buffer does not exist, fail
{
	EDITORBUFFER
		*theBuffer;
	char
		*channelId;
		
	if(objc==2)
	{
		channelId=Tcl_GetString(objv[1]);
		if (Tcl_GetChannel(localInterpreter, channelId, NULL) != NULL)	// see if there is such a channel
		{
			if((theBuffer=GetChannelBuffer(localInterpreter, channelId)))
			{
				Tcl_AppendResult(localInterpreter,theBuffer->contentName,NULL);
				return(TCL_OK);
			}
			else
			{
				Tcl_AppendResult(localInterpreter,"No buffer",NULL);
			}
		}
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"channelId");
	}
	return(TCL_ERROR);
}

static int Cmd_SetChannel(ClientData clientData,Tcl_Interp *localInterpreter,int objc,Tcl_Obj *const objv[])
// set the buffer to be used for Tcl's channel
// if the buffer does not exist, complain, but if it is deleted while it
// is being used as the output, that's ok -- the output is just discarded
{
	EDITORBUFFER
		*theBuffer=NULL;
	char
		*channelId;

	if(objc == 2 || objc == 3)
	{
		if(objc == 3)
		{
			if((theBuffer=GetEditorBuffer(localInterpreter,objv[2]))==NULL)
			{
				return(TCL_ERROR);
			}
		}
		
		channelId=Tcl_GetString(objv[1]);
		SetChannelBuffer(localInterpreter,channelId, theBuffer);
		return(TCL_OK);
	}
	else
	{
		Tcl_WrongNumArgs(localInterpreter,1,objv,"channelId ?bufferName?");
	}
	return(TCL_ERROR);
}

// Create commands so TCL can find them

bool CreateEditorShellCommands(Tcl_Interp *theInterpreter)
// create the shell commands that are implemented by the editor
// if there is a problem, SetError, return false
{
// menus and key bindings
	Tcl_CreateObjCommand(theInterpreter,"addmenu",Cmd_AddMenu,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"deletemenu",Cmd_DeleteMenu,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"bindkey",Cmd_BindKey,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"unbindkey",Cmd_UnBindKey,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"keybindings",Cmd_KeyBindings,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"waitkey",Cmd_WaitKey,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getkey",Cmd_GetKey,NULL,NULL);

// buffers and windows
	Tcl_CreateObjCommand(theInterpreter,"newbuffer",Cmd_NewBuffer,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"openbuffer",Cmd_OpenBuffer,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"closebuffer",Cmd_CloseBuffer,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"savebuffer",Cmd_SaveBuffer,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"savebufferas",Cmd_SaveBufferAs,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"savebufferto",Cmd_SaveBufferTo,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"revertbuffer",Cmd_RevertBuffer,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"isdirty",Cmd_BufferDirty,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"fromfile",Cmd_FromFile,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"haswindow",Cmd_HasWindow,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"cleardirty",Cmd_ClearDirty,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"bufferlist",Cmd_BufferList,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"windowlist",Cmd_WindowList,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"openwindow",Cmd_OpenWindow,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"closewindow",Cmd_CloseWindow,NULL,NULL);

	Tcl_CreateObjCommand(theInterpreter,"setrect",Cmd_SetRect,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getrect",Cmd_GetRect,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"setfont",Cmd_SetFont,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getfont",Cmd_GetFont,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"settabsize",Cmd_SetTabSize,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"gettabsize",Cmd_GetTabSize,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"setcolors",Cmd_SetColors,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getcolors",Cmd_GetColors,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"setselectioncolors",Cmd_SetSelectionColors,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getselectioncolors",Cmd_GetSelectionColors,NULL,NULL);

	Tcl_CreateObjCommand(theInterpreter,"buffervariables",Cmd_BufferVariables,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"setbuffervariable",Cmd_SetBufferVariable,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getbuffervariable",Cmd_GetBufferVariable,NULL,NULL);

	Tcl_CreateObjCommand(theInterpreter,"settopwindow",Cmd_SetTopWindow,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"minimizewindow",Cmd_MinimizeWindow,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"unminimizewindow",Cmd_UnminimizeWindow,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"activewindow",Cmd_GetActiveWindow,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"updatewindows",Cmd_UpdateWindows,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"screensize",Cmd_ScreenSize,NULL,NULL);

// undo/clipboard
	Tcl_CreateObjCommand(theInterpreter,"undotoggle",Cmd_UndoToggle,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"undo",Cmd_Undo,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"redo",Cmd_Redo,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"breakundo",Cmd_BreakUndo,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"flushundos",Cmd_FlushUndos,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"cut",Cmd_Cut,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"copy",Cmd_Copy,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"paste",Cmd_Paste,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"setclipboard",Cmd_SetClipboard,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getclipboard",Cmd_GetClipboard,NULL,NULL);

// dialogs
	Tcl_CreateObjCommand(theInterpreter,"okdialog",Cmd_OkDialog,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"okcanceldialog",Cmd_OkCancelDialog,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"yesnodialog",Cmd_YesNoDialog,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"textdialog",Cmd_TextDialog,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"searchdialog",Cmd_SearchDialog,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"listdialog",Cmd_ListDialog,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"opendialog",Cmd_OpenDialog,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"savedialog",Cmd_SaveDialog,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"pathdialog",Cmd_PathDialog,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"fontdialog",Cmd_FontDialog,NULL,NULL);

// search/replace
	Tcl_CreateObjCommand(theInterpreter,"find",Cmd_Find,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"findall",Cmd_FindAll,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"replace",Cmd_Replace,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"replaceall",Cmd_ReplaceAll,NULL,NULL);

// selection
	Tcl_CreateObjCommand(theInterpreter,"selectall",Cmd_SelectAll,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"selectedtextlist",Cmd_SelectedTextList,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"selectline",Cmd_SelectLine,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getselectionends",Cmd_GetSelectionEnds,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"setselectionends",Cmd_SetSelectionEnds,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getselectionendlist",Cmd_GetSelectionEndList,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"addselectionendlist",Cmd_AddSelectionEndList,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getselectionatposition",Cmd_GetSelectionAtPosition,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"setmark",Cmd_SetMark,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"clearmark",Cmd_ClearMark,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"gotomark",Cmd_GotoMark,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"marklist",Cmd_MarkList,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"homewindow",Cmd_HomeWindow,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"gettopleft",Cmd_GetTopLeft,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"settopleft",Cmd_SetTopLeft,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"textinfo",Cmd_TextInfo,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"selectioninfo",Cmd_SelectionInfo,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"positiontolineoffset",Cmd_PositionToLineOffset,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"lineoffsettoposition",Cmd_LineOffsetToPosition,NULL,NULL);

// styles and syntax highlighting
	Tcl_CreateObjCommand(theInterpreter,"getstyleatposition",Cmd_GetStyleAtPosition,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"setstyle",Cmd_SetStyle,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"selectiontostyle",Cmd_SelectionToStyle,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"styletoselection",Cmd_StyleToSelection,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"addsyntaxmap",Cmd_AddSyntaxMap,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"deletesyntaxmap",Cmd_DeleteSyntaxMap,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"syntaxmaps",Cmd_SyntaxMaps,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"setsyntaxmap",Cmd_SetSyntaxMap,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getsyntaxmap",Cmd_GetSyntaxMap,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"syntaxmapbuffers",Cmd_SyntaxMapBuffers,NULL,NULL);

// editing functions
	Tcl_CreateObjCommand(theInterpreter,"clear",Cmd_Clear,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"insert",Cmd_Insert,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"insertfile",Cmd_InsertFile,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"autoindent",Cmd_AutoIndent,NULL,NULL);

	Tcl_CreateObjCommand(theInterpreter,"movecursor",Cmd_MoveCursor,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"delete",Cmd_Delete,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"expandselection",Cmd_ExpandSelection,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"reduceselection",Cmd_ReduceSelection,NULL,NULL);

// misc functions
	Tcl_CreateObjCommand(theInterpreter,"setwordchars",Cmd_SetWordChars,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getwordchars",Cmd_GetWordChars,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"execute",Cmd_Execute,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"task",Cmd_Task,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"updatetask",Cmd_UpdateTask,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"eoftask",Cmd_EOFTask,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"killtask",Cmd_KillTask,NULL,NULL);
	
// new task command
	Tcl_CreateObjCommand(theInterpreter,"detatchtask",Cmd_DetatchTask,NULL,NULL);

	Tcl_CreateObjCommand(theInterpreter,"taskbytes",Cmd_TaskBytes,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"hastask",Cmd_HasTask,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"beep",Cmd_Beep,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"checkbuffer",Cmd_CheckBuffer,NULL,NULL);

	Tcl_CreateObjCommand(theInterpreter,"forceQUIT",Cmd_ForceQuit,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"version",Cmd_Version,NULL,NULL);

	Tcl_CreateObjCommand(theInterpreter,"ShellCommand",Cmd_ShellCommand,NULL,NULL);

// Built-in Tcl commands we DONT want
	Tcl_DeleteCommand(theInterpreter,"exit");

// replaced functions
 	Tcl_Eval(theTclInterpreter,"rename grab ::override::grab");
 	Tcl_CreateObjCommand(theInterpreter,"grab",Cmd_Grab,NULL,NULL);
	
 	Tcl_Eval(theTclInterpreter,"rename destroy ::override::destroy");
 	Tcl_CreateObjCommand(theInterpreter,"destroy",Cmd_Destroy,NULL,NULL);

	Tcl_CreateObjCommand(theInterpreter,"setchannelbuffer",Cmd_SetChannel,NULL,NULL);
	Tcl_CreateObjCommand(theInterpreter,"getchannelbuffer",Cmd_GetChannel,NULL,NULL);

	return(true);
}
