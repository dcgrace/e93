// Menu handling
// Copyright (C) 2000 Core Technologies.

// This file is part of e93.
//
// e93 is free software; you can redistribute it and/or modify
// it under the terms of the e93 LICENSE AGREEMENT.
//
// e93 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// e93 LICENSE AGREEMENT for more details.
//
// You should have received a copy of the e93 LICENSE AGREEMENT
// along with e93; see the file "LICENSE.TXT".


#include	"includes.h"

#define NoSymbol 0
#define MAX_MENU_NAME_LENGTH 1024
#define MAX_MENU_ITEMS 16384

typedef UINT32 KeySym;

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

// this definition sets EDITORMENU to the structure needed by the gui
struct editorMenu				// actual structure of an editor menu
{
	char
		*itemText;				// name of the item is stored here (semi-redundant with OS's name, but easier to get to)
	char
		*attributes;			// attribute string is stored here
	char
		*dataText;				// the text of the menu (used when menu is clicked on)
	char
		*commandString;			// command string is kept here (this is the string that is displayed as the menu name when there is a key sequence that can be used to select it)
	bool
		hasKey;					// true if this item has a key code associated with it
	KeySym
		theKeySym;				// keySymCode that we are looking for (if any)
	UINT32						
		keyModifiers,			// used in conjuction with theKeySym, supplies the modifier. theKeySym is only the base keycode
		modifierMask;			// what modifers to chaeck, some may be ignored
	bool
		active;					// tells if the menu is active (can be selected) or not
	bool
		separator;				// tells if the menu item is a separator
	EDITORMENU					// linked list of menus
		*parent,
		*nextSibling,
		*previousSibling,
		*firstChild;
	HMENU
		parentHandle;			// the Windows menu that this menu is a child of
	MENUITEMINFO
		*menuInfo;				// the Windows menu information structure we maintain
};

static HMENU
	mdiMenu,
	editorTopMenu = NULL;
static UINT32
	globalMenuID;

typedef struct KeyName
	{
 	char *name;
 	UINT32 theKeySym;
	}
	KeyName;

// This table gives names only to keys that have no printable character
// Keys that have a printable character will be known by that character
KeyName
	keyNameTable[] =
	{
// X Windows compatable names

		{"space",		VK_SPACE},

		{"BackSpace",	VK_BACK},		// back space, back char
		{"Tab",			VK_TAB},

		{"Clear",		VK_CLEAR},
		{"Return",		VK_RETURN},		// Return, enter
		{"Pause",		VK_PAUSE},		// Pause, hold
		{"Scroll_Lock", VK_SCROLL},
		{"Sys_Req", 	VK_PRINT},
		{"Escape",		VK_ESCAPE},
		{"Delete",		VK_DELETE},		// Delete, rubout

		// VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39)
		// VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A)
		// So we'll just convert the theKeySym into a string

// the "F" keys
		{"F1",			VK_F1},
		{"F2",			VK_F2},
		{"F3",			VK_F3},
		{"F4",			VK_F4},
		{"F5",			VK_F5},
		{"F6",			VK_F6},
		{"F7",			VK_F7},
		{"F8",			VK_F8},
		{"F9",			VK_F9},
		{"F10",			VK_F10},
		{"F11",			VK_F11},
		{"F12",			VK_F12},
		{"F13",			VK_F13},
		{"F14",			VK_F14},
		{"F15",			VK_F15},
		{"F16",			VK_F16},
		{"F17",			VK_F17},
		{"F18",			VK_F18},
		{"F19",			VK_F19},
		{"F20",			VK_F20},
		{"F21",			VK_F21},
		{"F22",			VK_F22},
		{"F23",			VK_F23},
		{"F24",			VK_F24},


// Cursor control & motion
		{"Home",        VK_HOME},
		{"Left",        VK_LEFT}, 		// Move left, left arrow
		{"Up",          VK_UP},			// Move up, up arrow
		{"Right",       VK_RIGHT},		// Move right, right arrow
		{"Down",        VK_DOWN}, 		// Move down, down arrow
		{"Prior",       VK_PRIOR},		// Prior, previous
		{"Page_Up",     VK_PRIOR},
		{"Next",        VK_NEXT}, 		// Next
		{"Page_Down",   VK_NEXT},
		{"End",         VK_END},  		// EOL
		{"Begin",       0xFF58},  		// BOL

// Misc Functions
		{"Select",		VK_SELECT},
		{"Print",		VK_PRINT},
//		{"Execute",     VK_EXECUTE},	// Execute, run, do (repeated below to give KP_Enter precedence)
		{"Insert",		VK_INSERT},		// Insert, insert here
//		{"Undo",        	      		// Undo, oops
//		{"Redo",        		  		// redo, again
//		{"Menu",	 	VK_MENU},
//		{"Find",        				// Find, search
		{"Cancel",		VK_CANCEL},		// Cancel, stop, abort, exit (Ctrl Break)
		{"Help",		VK_HELP},		// Help
		{"Break",       VK_PAUSE},
//		{"Mode_switch",        			// Character set switch
//		{"script_switch",       		// Alias for mode_switch
		{"Num_Lock",     VK_NUMLOCK},

// Keypad Functions
		{"KP_Divide",	VK_DIVIDE},
		{"KP_Multiply",	VK_MULTIPLY},
		{"KP_Subtract",	VK_SUBTRACT},
		{"KP_Add",		VK_ADD},
		{"KP_Enter",	VK_EXECUTE},
		{"Execute",     VK_EXECUTE},	// Execute, run, do
		
		{"KP_Home",		VK_HOME},
		{"KP_Up",		VK_UP},
		{"KP_Prior",	VK_PRIOR},
		{"KP_Page_Up",	VK_PRIOR},
		{"KP_Left",		VK_LEFT},
		{"KP_Right",	VK_RIGHT},
		{"KP_End",		VK_END},
		{"KP_Down",		VK_DOWN},
		{"KP_Next",		VK_NEXT},
		{"KP_Page_Down",VK_NEXT},
		{"KP_Insert",	VK_INSERT},
		{"KP_Delete",	VK_DELETE},
		
		{"KP_Separator",VK_SEPARATOR},
		{"KP_Decimal",	VK_DECIMAL},
		
// with numlock
		{"KP_0",		VK_NUMPAD0},
		{"KP_1",		VK_NUMPAD1},
		{"KP_2",		VK_NUMPAD2},
		{"KP_3",		VK_NUMPAD3},
		{"KP_4",		VK_NUMPAD4},
		{"KP_5",		VK_NUMPAD5},
		{"KP_6",		VK_NUMPAD6},
		{"KP_7",		VK_NUMPAD7},
		{"KP_8",		VK_NUMPAD8},
		{"KP_9",		VK_NUMPAD9},

// Modifiers
		{"Shift",		VK_SHIFT},		
		{"Ctrl",		VK_CONTROL},		
		{"Alt",	 		VK_MENU},

		{"Shift_L",		VK_LSHIFT},		// Left shift
		{"Shift_R",     VK_RSHIFT},		// Right shift
		{"Control_L",   VK_LCONTROL},	// Left control
		{"Control_R",   VK_RCONTROL},	// Right control
		{"Caps_Lock",   VK_CAPITAL},	// Caps lock
		{"Shift_Lock",  VK_CAPITAL},	// Shift lock

//		{"Meta_L",      				// Left meta
//		{"Meta_R",      				// Right meta
		{"Alt_L",       VK_LMENU},		// Left alt
		{"Alt_R",       VK_RMENU},		// Right alt
//		{"Super_L",     		 		// Left super
//		{"Super_R",     		 		// Right super
//		{"Hyper_L",     		 		// Left hyper
//		{"Hyper_R",     		 		// Right hyper

// Windows specific

		{"Snapshot",	VK_SNAPSHOT},
		
		{"WinKey",		VK_APPS},
		{"LeftWin",		VK_LWIN},
		{"RightWin",	VK_RWIN},

		{"Attention",	VK_ATTN},
		{"Crsel",		VK_CRSEL},
		{"Exsel",		VK_EXSEL},
		{"Ereof",		VK_EREOF},
		{"Play",		VK_PLAY},
		{"Zoom",		VK_ZOOM},
		{"No Name",		VK_NONAME},
		{"PA1",			VK_PA1},
		{"OEM Clr",		VK_OEM_CLEAR},

		{NULL,	' '}					// terminate the list
	};

// This table gives names to keys that have a single character, but may also
// be known by a name such as "Space" or "Comma"
// These are X Windows compatable key names
KeyName
	charNameTable[] =
	{
		{"space",			' '},
		{"exclam",			'!'},
		{"quotedbl",		'"'},
		{"numbersign",		'#'},
		{"dollar",			'$'},
		{"percent",			'%'},
		{"ampersand",		'&'},
		{"apostrophe",		'\''},
		{"quoteright",		'’'},		// deprecated
		{"parenleft",		'('},
		{"parenright",		')'},
		{"asterisk",		'*'},
		{"plus",			'+'},
		{"comma",			','},
		{"minus",			'-'},
		{"period",			'.'},
		{"slash",			'/'},
		{"colon",			':'},
		{"semicolon",		';'},
		{"less",			'<'},
		{"equal",			'='},
		{"greater",			'>'},
		{"question",		'?'},
		{"at",				'@'},
		{"bracketleft",		'['},
		{"backslash",		'\\'},
		{"bracketright",	']'},
		{"asciicircum",		'^'},
		{"underscore",		'_'},
		{"grave",			'`'},
		{"quoteleft",		'‘'},		// deprecated
		{"braceleft",		'{'},
		{"bar",				'|'},
		{"braceright",		'}'},
		{"asciitilde",		'~'},

		// strange characters
		{"Linefeed",		'\r'}, 		// Linefeed, LF
		{"umlaut",			'¨'},
		{"quotedblleft",	'“'},
		{"quotedblright",	'”'},

		{"guillemotleft",	'‘'},
		{"guillemotright",	'’'},
		{"brokenbar",		'¦'},
		{NULL,				' '}
	};

static EDITORMENU
	*firstMenu;

static char
	localErrorFamily[]="Wguimenus";

enum
{
	BADRELATIONSHIP,
	BADPATH,
	BADKEYNAME,
	BADMODIFIERS
};

static char *errorMembers[]=
{
	"BadRelationship",
	"BadPath",
	"BadKeyName",
	"BadModifiers"
};

static char *errorDescriptions[]=
{
	"Invalid menu relationship",
	"Invalid menu path",
	"Invalid key name",
	"Invalid modifier"
};

static EDITORMENU *LocateMenuWithKeyAndModifers(EDITORMENU *topLevel, KeySym theKeySym, UINT32 keyModifiers)
// attempt to locate an active menu below topLevel, that has theKeySym defined
// if one can be located, return it, else return NULL
{
	EDITORMENU
		*currentMenu,
		*locatedMenu;

	currentMenu=GetRelatedEditorMenu(topLevel,MR_FIRSTCHILD);
	locatedMenu=NULL;
	while(currentMenu&&!locatedMenu)
	{
		if(!(locatedMenu=LocateMenuWithKeyAndModifers(currentMenu,theKeySym,keyModifiers)))			// recurse to try to find child that has this key
		{
			if(currentMenu->hasKey&&currentMenu->active)
			{
				if((currentMenu->theKeySym==theKeySym)&&((currentMenu->modifierMask&keyModifiers)==(currentMenu->modifierMask&currentMenu->keyModifiers)))
				{
					locatedMenu=currentMenu;
				}
			}
		}
		currentMenu=GetRelatedEditorMenu(currentMenu,MR_NEXTSIBLING);
	}
	return(locatedMenu);
}

static char
	keyNameString[2];

static char *KeySymToKeyNameWithCase(KeySym theKeySym, bool isUpper)
// given a theKeySym, get its name
// if the code is invalid, return NULL
// NOTE: the returned string is const, and should not be modified or deleted
//
// If you got the key code by passing the character of a modified key
// you won't get that same character back.
// You will get the base unmodifed key converted to the ascii char by the OS.
// This essentially remaps your character to the base character of the same key that your char appears on.
{
	UINT32
		charValue;
	bool
		found = false;
	char
		*theName = NULL;
	int
		i;

	// see if this key-code has been defined in our key-code to key-name conversion table
	for (i = 0; !found && keyNameTable[i].name; i++)
	{
		if (theKeySym == keyNameTable[i].theKeySym)
		{
			found = true;
			theName = keyNameTable[i].name;
		}
	}

	// If that fails see if the OS has a simple ascii value for this key code.
	// If so just print the ascii char into the string.

	if (!found && ((charValue = MapVirtualKey(theKeySym, 2)) != 0))
	{
		// if this menu uses the Shift modifer, list the key in uppercase. It's a bit redundant since we list "Shift", but I still like it
		sprintf(keyNameString, "%c", isUpper ? toupper(charValue) : tolower(charValue));
		found = true;
		theName = keyNameString;
	}

	// if the name is a single character
	if (found && strlen(theName) == 1)
	{
		found = false;
		// see if that character has been defined in our char to key-name conversion table, if so use the name
		for (i = 0; !found && charNameTable[i].name; i++)
		{
			if(theName[0] == (char)(charNameTable[i].theKeySym))
			{
				theName = charNameTable[i].name;
				found = true;
			}
		}
	}

	return (theName);
}

static void SetMenuDisplayName(EDITORMENU *theMenu)
// determine the display name for the menu
// may be: itemText, commandString or nothing
{
	// **
	// because MS treat them special, top level menus may not include a tab in their name
	// whereas the others must have it, so we do not list the commandString in the name
	// only in that "special" case
	// **
	if(theMenu->hasKey && !(theMenu->parentHandle == editorTopMenu))				// if this menu has a command key, add commandString to the display name
	{
		theMenu->menuInfo->dwTypeData = theMenu->commandString;
	}
	else
	{
		theMenu->menuInfo->dwTypeData = theMenu->itemText;							// the name has no command key
	}

	if(theMenu->separator)															// determine if this is a separator
	{
		theMenu->menuInfo->dwTypeData = "";											// if it is, the menu will have no display name
	}
}

static int UpdateWindowsMenu(EDITORMENU *theMenu)
// update menu name and active/inactive state, return non-zero if all goes well
{
	int
		result = 0;
	
	SetMenuDisplayName(theMenu);

	if(theMenu->parentHandle)	// see if we have a Windows menu yet
	{
		if(result = SetMenuItemInfo(theMenu->parentHandle, theMenu->menuInfo->wID, false, theMenu->menuInfo))
		{
			if(theMenu->parentHandle==editorTopMenu)
			{
				DrawMenuBar(frameWindowHwnd);
			}
		}
	}
	
	return result;
}
 
static void AddCommandStringsToMenu(EDITORMENU *theMenu)
// build a command string from the KeySym and modifiers
// and store it in theMenu, e.g. "Save As... Ctrl+Shift S"
{
	char
		*theKeyName,
		commandStrBuff[MAX_MENU_NAME_LENGTH];
	int
		i;
	bool
		modifierUsed = false,													// track if we need to add a + between modifiers
		isUpper = (theMenu->keyModifiers & EEM_SHIFT) != 0;						// should we show the key as uppercase in the menu?

	sprintf(commandStrBuff, "%s\t", theMenu->itemText);							// itemName with a tab before the command

	for(i = 0; ModifierTable[i].windowsKeyCode; i++)							// append all the modifer names, delimit them with + if more than one
	{
		if (theMenu->keyModifiers & (ModifierTable[i].editorKeyCode))
		{
			if (theKeyName = EditorKeyCodeToKeyName(ModifierTable[i].windowsKeyCode))
			{
				if (modifierUsed)
				{
					strcat(commandStrBuff,"+");
				}
				strcat(commandStrBuff, theKeyName);
				modifierUsed = TRUE;
			}
		}
	}

	if (modifierUsed)
	{
		sprintf(commandStrBuff,"%s ", commandStrBuff);							// put a space between the modifers and the keyname
	}

	if(theKeyName = KeySymToKeyNameWithCase(theMenu->theKeySym, isUpper))
	{
		strcat(commandStrBuff, theKeyName);										// key name at the end
	}
	else
	{
		sprintf(commandStrBuff, "%s", theMenu->itemText);						// if we didn't find a key, no commandstring is listed, although the menu is still techinically bound
	}
		
	if((theMenu->commandString = (char *)MNewPtr(strlen(commandStrBuff) + 1)))	// make room for 0 terminator
	{
		sprintf(theMenu->commandString,"%s",commandStrBuff);					// copy in the command key string
	}
}

static EDITORMENU *GetRelatedEditorMenu(EDITORMENU *theMenu,int menuRelationship)
// return the menu with menuRelationship to theMenu
// return NULL if there is no menu with menuRelationship
{
	EDITORMENU
		*outputMenu;

	outputMenu=NULL;
	switch(menuRelationship)
	{
		case MR_NEXTSIBLING:
			if(theMenu->parent)
			{
				if(theMenu->nextSibling!=theMenu->parent->firstChild)
				{
					outputMenu=theMenu->nextSibling;
				}
			}
			else
			{
				if(theMenu->nextSibling!=firstMenu)
				{
					outputMenu=theMenu->nextSibling;
				}
			}
			break;
		case MR_PREVIOUSSIBLING:
			if(theMenu->parent)
			{
				if(theMenu!=theMenu->parent->firstChild)
				{
					outputMenu=theMenu->previousSibling;
				}
			}
			else
			{
				if(theMenu!=firstMenu)
				{
					if(theMenu->previousSibling!=firstMenu)
					{
						outputMenu=theMenu->previousSibling;
					}
				}
				else
				{
					outputMenu=firstMenu;
				}
			}
			break;
		case MR_FIRSTCHILD:
			if(theMenu)
			{
				outputMenu=theMenu->firstChild;
			}
			else
			{
				outputMenu=firstMenu;
			}
			break;
		case MR_LASTCHILD:
			if(theMenu)
			{
				if(theMenu->firstChild)
				{
					outputMenu=theMenu->firstChild->previousSibling;
				}
			}
			else
			{
				if(firstMenu)
				{
					outputMenu=firstMenu->previousSibling;
				}
			}
			break;
		default:
			break;
	}
	return(outputMenu);
}

static char
	message[255];

static bool KeyNameToSymAndMods(const char *theName, KeySym *theKeySym, UINT32 *keyModifiers)

// Given a string which is the name of any key on the keyboard, convert it into a KeySym.
// 
// add EEM_SHIFT into keyModifiers if the char is shifted on a key on the current keyboard
// 
// This will return the unmodifed theKeySym even if the character isn't unmodified on an key.
// The state information is discarded.
// 
// We need to do it this way to allow the use of "dead keys" and to allow
// keys to be used in addmenu that aren't base keys on non-US keyboards
// without having to rewrite the e93 rc files for each locale.
// 
// We return the correct key, but be warned if you convert that
// key-code back into a string you will not get the name of the modified character back.
// You will get the name of the unmodified base key.
// Therefore on a US keyboard, '{', '[' are equivilent and either may be used in addmenu.
// 
// This is OK since e93 keeps its modifiers separate from the key.
// So we can refer to the key by its base name, but still distinguish between them.
{
	UINT32
		i;
	bool
		found = false;
	UINT16
		stateAndCode;

	*theKeySym = NoSymbol;
	
	// see if this key-name has been defined in our key-code to key-name conversion table
	for (i = 0; !found && keyNameTable[i].name; i++)
	{
		// if(strcasecmp(theName, keyNameTable[i].name) == 0)		// for now we are case-insensitive
		if(strcmp(theName, keyNameTable[i].name) == 0)				// for now we are case-sensitive
		{
			found = TRUE;
			*theKeySym = keyNameTable[i].theKeySym;
		}
	}

	if (!found)
	{
		char
			theChar = 0;

		// if the name is one character, use it
		if ((strlen(theName) == 1))
		{
			theChar = theName[0];
		}
		else
		{
			// see if this key-name has been defined in our char to key-name conversion table
			// if it has convert the name to a character
			for (i = 0; !found && charNameTable[i].name; i++)
			{
				// if(strcasecmp(theName, charNameTable[i].name) == 0)	// for now we are case-insensitive
				if(strcmp(theName, charNameTable[i].name) == 0)			// for now we are case-sensitive
				{
					theChar = charNameTable[i].theKeySym;
				}
			}
		}

		if (theChar != 0)
		{
			// the name is a single char, find the character on the keyboard
			// and use its scan code with the state information stripped out for the KeySym value.

			stateAndCode = VkKeyScan(theChar);						// find this char on the keyboard

			if ((stateAndCode >> 8) != -1)							// see if it was found
			{
				found = true;
				*theKeySym = stateAndCode & 0XFF;					// mask out the key state information
				if (stateAndCode & 0X100)							// shift is set depending on the case of the char passed in, a 1 in the high byte is shifted on this keyboard (isupper is not good enough)
				{
					*keyModifiers |= EEM_SHIFT;						// EEM_SHIFT must be pressed with this keySym
				}
			}
		}
	}

	if (!found)
	{
		sprintf(message, "%s: %s", errorDescriptions[BADKEYNAME], theName);
		SetError(localErrorFamily, errorMembers[BADKEYNAME], message);
	}

	return(found);
}


static KeySym XStringToKeysym(const char *keyName)
// provide compatability with the XWindows call
{
	KeySym
		theKeySym;
	UINT32
		keyModifiers;
	
	KeyNameToSymAndMods(keyName, &theKeySym, &keyModifiers);					// get the KeySym and keyModifiers for theKeyName

	return theKeySym;
}

static char *XKeysymToString(KeySym theKeySym)
// provide compatability with the XWindows call
{
	return KeySymToKeyNameWithCase(theKeySym, false);
}

static EDITORMENU *LocateMenuWithItem(EDITORMENU *topLevel,UINT32 menuID)
// attempt to locate an active menu below topLevel, that has menuID defined
// if one can be located, return it, else return NULL
{
	EDITORMENU
		*currentMenu,
		*locatedMenu;

	currentMenu=GetRelatedEditorMenu(topLevel,MR_FIRSTCHILD);
	locatedMenu=NULL;
	while(currentMenu&&!locatedMenu)
	{
		if(!(locatedMenu=LocateMenuWithItem(currentMenu,menuID)))				// recurse to try to find child that has this key
		{
			if(currentMenu->menuInfo->wID==menuID)								// see if this item has the key
			{
				locatedMenu=currentMenu;
			}
		}
		currentMenu=GetRelatedEditorMenu(currentMenu,MR_NEXTSIBLING);
	}
	return(locatedMenu);
}

static bool LinkEditorMenuElement(EDITORMENU *theMenu,EDITORMENU *baseMenu,int menuRelationship)
// link theMenu to baseMenu with menuRelationship
// if baseMenu is NULL, then menuRelationship must be MR_FIRSTCHILD, or MR_LASTCHILD as it is not possible to
// link menus as siblings of the root
// if there is a problem, Set the error, and return false
{
	bool
		fail;

	fail=false;
	theMenu->firstChild=NULL;
	switch(menuRelationship)
	{
		case MR_NEXTSIBLING:
			if(baseMenu)
			{
				theMenu->nextSibling=baseMenu->nextSibling;
				theMenu->nextSibling->previousSibling=theMenu;
				theMenu->previousSibling=baseMenu;
				baseMenu->nextSibling=theMenu;
				theMenu->parent=baseMenu->parent;
			}
			else
			{
				SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
				fail=true;
			}
			break;
		case MR_PREVIOUSSIBLING:
			if(baseMenu)
			{
				theMenu->previousSibling=baseMenu->previousSibling;
				theMenu->previousSibling->nextSibling=theMenu;
				theMenu->nextSibling=baseMenu;
				baseMenu->previousSibling=theMenu;
				theMenu->parent=baseMenu->parent;
				if(theMenu->parent)
				{
					if(theMenu->parent->firstChild==baseMenu)
					{
						theMenu->parent->firstChild=theMenu;		// make us the first child if linked before old first child
					}
				}
				else
				{
					if(firstMenu==baseMenu)
					{
						firstMenu=theMenu;
					}
				}
			}
			else
			{
				SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
				fail=true;
			}
			break;
		case MR_FIRSTCHILD:
			theMenu->parent=baseMenu;									// set parent
			if(baseMenu)												// see if linking as a child of a current menu
			{
				if((theMenu->nextSibling=baseMenu->firstChild))
				{
					theMenu->previousSibling=theMenu->nextSibling->previousSibling;
					theMenu->nextSibling->previousSibling=theMenu;	// link me in as previous sibling of old first child if there was one
					theMenu->previousSibling->nextSibling=theMenu;	// link me in as next sibling
				}
				else
				{
					theMenu->previousSibling=theMenu;					// loop previous sibling to myself
					theMenu->nextSibling=theMenu;						// loop next sibling to myself
				}
				baseMenu->firstChild=theMenu;							// this is now the first child
			}
			else							// creating a child of the universe
			{
				if(firstMenu)
				{
					theMenu->nextSibling=firstMenu;
					theMenu->previousSibling=firstMenu->previousSibling;
					theMenu->previousSibling->nextSibling=theMenu;
					firstMenu->previousSibling=theMenu;
				}
				else
				{
					theMenu->previousSibling=theMenu;
					theMenu->nextSibling=theMenu;
				}
				firstMenu=theMenu;
			}
			break;
		case MR_LASTCHILD:
			theMenu->parent=baseMenu;									// set parent
			if(baseMenu)													// see if linking as a child of a current menu
			{
				if((theMenu->nextSibling=baseMenu->firstChild))
				{
					theMenu->previousSibling=theMenu->nextSibling->previousSibling;
					theMenu->nextSibling->previousSibling=theMenu;	// link me in as previous sibling of old first child if there was one
					theMenu->previousSibling->nextSibling=theMenu;	// link me in as next sibling
				}
				else
				{
					theMenu->previousSibling=theMenu;					// loop previous sibling to myself
					theMenu->nextSibling=theMenu;						// loop next sibling to myself
					baseMenu->firstChild=theMenu;
				}
			}
			else							// creating a child of the universe
			{
				if(firstMenu)
				{
					theMenu->nextSibling=firstMenu;
					theMenu->previousSibling=firstMenu->previousSibling;

					theMenu->previousSibling->nextSibling=theMenu;

					firstMenu->previousSibling=theMenu;
				}
				else
				{
					theMenu->previousSibling=theMenu;
					theMenu->nextSibling=theMenu;
					firstMenu=theMenu;
				}
			}
			break;
		default:
			SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
			fail=true;
			break;
	}
	return(!fail);
}

static void UnlinkEditorMenuElement(EDITORMENU *theMenu)
// unlink theMenu from the menu tree
{
	if(theMenu->parent)
	{
		if(theMenu->parent->firstChild==theMenu)
		{
			if(theMenu->nextSibling!=theMenu)
			{
				theMenu->parent->firstChild=theMenu->nextSibling;
			}
			else
			{
				theMenu->parent->firstChild=NULL;
			}
		}
	}
	else
	{
		if(firstMenu==theMenu)
		{
			if(theMenu->nextSibling!=theMenu)
			{
				firstMenu=theMenu->nextSibling;
			}
			else
			{
				firstMenu=NULL;
			}
		}
	}
	theMenu->nextSibling->previousSibling=theMenu->previousSibling;
	theMenu->previousSibling->nextSibling=theMenu->nextSibling;
}

static bool isModifierString(char *theString)
// TODO: make certain the string only conatins 0's, 1's or X's
{
	int
		length = strlen(theString);
	bool
		result = false;

	if (length >= 11)	// x0010000000
	{
		result = true;
	}

	return result;
}

static bool SetBoundKey(EDITORMENU *theMenu,char *theString)
// convert a string of modifiers into a mask, and value
// if there is a problem, return false
{
	int
		i,
		theLength;
	bool
		fail;

	theMenu->modifierMask=0;
	theMenu->keyModifiers=0;
	theLength=strlen(theString);
	fail=false;
	if(isModifierString(theString))				// must be an entry for each flag
	{
		theLength=Min(11,theLength);			// only examine the first 11 chars
		for(i=0;(!fail)&&(i<theLength);i++)
		{
			switch(theString[i])
			{
				case 'x':
				case 'X':
					break;
				case '0':
					theMenu->modifierMask|=modifierFlagTable[i];
					break;
				case '1':
					theMenu->modifierMask|=modifierFlagTable[i];
					theMenu->keyModifiers|=modifierFlagTable[i];
					break;
				default:
					SetError(localErrorFamily,errorMembers[BADMODIFIERS],errorDescriptions[BADMODIFIERS]);
					fail=true;
					break;
			}
		}
	}
	else
	{
		fail=true;
		SetError(localErrorFamily,errorMembers[BADMODIFIERS],errorDescriptions[BADMODIFIERS]);
	}
	return(!fail);
}


static void UnAssignKeyEquivalent(EDITORMENU *theMenu)
// remove key code assigned to theMenu
{
	if(theMenu->hasKey)
	{
		if(theMenu->commandString)
		{
			MDisposePtr(theMenu->commandString);
			theMenu->commandString=NULL;
		}
		theMenu->hasKey=false;
		UpdateWindowsMenu(theMenu);								// the display name will change
	}
}

static bool AssignKeyEquivalent(EDITORMENU *theMenu,char *theKeyName)
// given a key name, assign that key to theMenu
// if the another menu already has the key assigned to it, get rid of it
// if theKeyName does not map to a valid key, then the menu will have no key equivalent
{
	bool
		fail = false;
	EDITORMENU
		*oldMenu;
	KeySym
		theKeySym = NoSymbol;
	UINT32
		keyModifiers = 0;
	
	UnAssignKeyEquivalent(theMenu);

	if (SetBoundKey(theMenu,theKeyName))											// support the bindkey style
	{
		theKeyName = &(theKeyName[11]);												// look for the name starting here, after the mask
		if (strlen(theKeyName) == 1)												// if this is a single char, and not a name
		{
			theKeyName[0] = tolower(theKeyName[0]);									// we are setting the modifer state, so we don't want KeyNameToSymAndMods adding shift
		}
	}
	else																			// no valid key mask, so make our own default version
	{
		theMenu->modifierMask = EEM_MOD0 | EEM_SHIFT | EEM_CTL | EEM_MOD1;			// we care about the state of all these keys
		theMenu->keyModifiers |= EEM_MOD0;											// MOD0 must always be pressed
	}
		
	if (KeyNameToSymAndMods(theKeyName, &theKeySym, &keyModifiers))					// get the KeySym and keyModifiers for theKeyName
	{
		keyModifiers |= theMenu->keyModifiers;										// add in our modifiers to the ones needed to produce the key
		if((oldMenu=LocateMenuWithKeyAndModifers(NULL,theKeySym,keyModifiers)))		// see if another menu item had this key
		{
			UnAssignKeyEquivalent(oldMenu);											// if so, take it
		}
		theMenu->theKeySym = theKeySym;
		theMenu->keyModifiers |= keyModifiers;										// add them to the modifiers already set
		keyModifiers &= theMenu->modifierMask;
		theMenu->hasKey=true;
		AddCommandStringsToMenu(theMenu);											// fill in the command string
		UpdateWindowsMenu(theMenu);													// the display name will change
	}
	else
	{
		fail = true;
	}

	return fail;
}

static void ExtractAttributeParameter(char *parameterInput,UINT32 *parameterIndex,char *parameterBuffer,UINT32 parameterBufferSize)
// extract the next parameter from parameterInput[parameterIndex] into parameter buffer
// if the parameter would be too long, just truncate it
// leave paramterIndex updated to point to the end of the given parameter, no matter
// how long it is
{
	UINT32
		outIndex;
	char
		theChar;
	bool
		done;

	outIndex=0;
	done=false;
	while((theChar=parameterInput[*parameterIndex])&&!done)		// make sure not end of string, or end of parameter
	{
		if(theChar!='\\')
		{
			(*parameterIndex)++;
			if(outIndex<parameterBufferSize)
			{
				parameterBuffer[outIndex++]=theChar;
			}
		}
		else
		{
			if(parameterInput[*parameterIndex+1]=='\\')			// see if quoted separator (not a separator at all then)
			{
				(*parameterIndex)+=2;
				if(outIndex<parameterBufferSize)
				{
					parameterBuffer[outIndex++]='\\';			// place separator into buffer
				}
			}
			else
			{
				done=true;										// ran into end of parameter list
			}
		}
	}
	if(outIndex<parameterBufferSize)
	{
		parameterBuffer[outIndex]='\0';							// terminate buffer
	}
	else
	{
		if(parameterBufferSize)
		{
			parameterBuffer[outIndex-1]='\0';					// terminate buffer by writing over last character
		}
	}
}

static bool RealizeAttributes(EDITORMENU *theMenu)
// Attempt to fill the parts of theMenu's record that are described in its string
// Currently supports:
// \Ffontname		specify the font to use when drawing the item (Not supported on Windows)
// \S				specify that this item is a menu separation line (text is not drawn)
//					Bind flags		 lsc01234567
// \Kkey			specify that mod1 <key> could be used to access the given menu
// \W				specifies that this menu item is the MDI window menu item
// if there is a problem, return false
{
	char
		parameterBuffer[256];							// max size for parameter string, anything larger gets truncated!
	UINT32
		attributeIndex;
	char
		theChar;
	bool
		fail=false;

	theMenu->separator=false;
	theMenu->hasKey=false;
	theMenu->theKeySym=NoSymbol;
	theMenu->commandString=NULL;						// no command string yet
	
	theMenu->keyModifiers=0;
	theMenu->modifierMask=0;

	attributeIndex=0;
	while(!fail&&(theChar=theMenu->attributes[attributeIndex]))
	{
		if(theChar!='\\')								// eat separators
		{
			ExtractAttributeParameter(theMenu->attributes,&attributeIndex,&(parameterBuffer[0]),256);
			switch(parameterBuffer[0])					// check command part
			{
				case 'F':
					break;
				case 'S':
					theMenu->separator=true;
					break;
				case 'K':
					fail = AssignKeyEquivalent(theMenu,&(parameterBuffer[1]));
					break;
				default:
					break;
			}
		}
		else
		{
			attributeIndex++;
		}
	}

	return !fail;
}

static void UnRealizeAttributes(EDITORMENU *theMenu)
// undo anything that RealizeAttributes did
{
	UnAssignKeyEquivalent(theMenu);
}

static bool CreateWindowsMenuItem(EDITORMENU *baseMenu,EDITORMENU *theMenu,int menuRelationship)
// Create a Windows menu item to match the Editor menu item
{
	bool
		result = true,																			// assume success unless something fails later
		fByPosition = true;																		// normally we insert by index
	UINT32
		menuPos = 0xFFFFFFFF;																	// if we insert by index, make it be the last in the list
	
	if ((menuRelationship == MR_FIRSTCHILD) || menuRelationship == MR_PREVIOUSSIBLING)			// they asked to be first, or previous...
	{
		menuPos = 0;																			// ...so if we insert by index, make it be the first in the list
	}
	
	theMenu->menuInfo->fType = theMenu->separator ? MFT_SEPARATOR : MFT_STRING;					// it is either a normal menu or a separator
	theMenu->menuInfo->fState = theMenu->active ? MFS_ENABLED : MFS_DISABLED | MFS_GRAYED;		// if it's not active then it's not selectable and it's gray
	
	if(baseMenu)																				// are we being inserted relative to some other menu?
	{
		switch(menuRelationship)
		{
			case MR_PREVIOUSSIBLING:
			case MR_NEXTSIBLING:
			{
				EDITORMENU
					*nextSib;
					
				theMenu->parentHandle = baseMenu->parentHandle;									// this menu shares base's parent, since we must be its sibling
				
				if ((nextSib = GetRelatedEditorMenu(theMenu, MR_NEXTSIBLING)) != NULL)			// if we can find the menu that follows us (next sibling)
				{
					menuPos = nextSib->menuInfo->wID;											// insert our new menu, by ID, before our next sibling
					fByPosition = false;														// the menuPos is now an ID, set fByPosition to use it as such
				}																				// otherwise we don't have a next sibling... so we'll insert by index below (default anyhow)
				break;
			}
			case MR_FIRSTCHILD:
			case MR_LASTCHILD:
			{
				if(baseMenu->menuInfo->hSubMenu == NULL)										// if the base menu is not yet a popup menu, make it be one
				{
					if(baseMenu->menuInfo->hSubMenu = CreatePopupMenu())
					{
						result = (UpdateWindowsMenu(baseMenu) != 0);							// update the menu
					}
				}
				
				theMenu->parentHandle = baseMenu->menuInfo->hSubMenu;							// our parent is now the popup, since we must be a child of the base
				break;
			}
		}
	}
	else
	{
		theMenu->parentHandle=editorTopMenu;													// no base, we must be a top-level menu, child of the root
		SetMenuDisplayName(theMenu);															// top-level menus may need their name altered due to Windows, this will apply the naming rules
	}
	
	if (result)
	{
		if(result = (InsertMenuItem(theMenu->parentHandle, menuPos, fByPosition, theMenu->menuInfo)!=0))
		{
			if(theMenu->parentHandle==editorTopMenu)
			{
				DrawMenuBar(frameWindowHwnd);
			}
		}
	}
	
	if(!result)
	{
		SetWindowsError();
	}

	return(result);
}

static void DeleteWindowsMenuItem(EDITORMENU *theMenu)
//	Passed a menu item, delete the relevant Windows menu stuff
{
	int
		curPos;

	if(theMenu->parentHandle)						// top menu does not have a parent and should not be deleted
	{
		if(theMenu->menuInfo->hSubMenu)
		{
 			curPos=0;
 			while((GetSubMenu(theMenu->parentHandle,curPos)!=theMenu->menuInfo->hSubMenu) && (curPos<MAX_MENU_ITEMS))
 			{
 				curPos++;
 			}
 			RemoveMenu(theMenu->parentHandle,curPos,MF_BYPOSITION);
 			DestroyMenu(theMenu->menuInfo->hSubMenu);
		}
		else
		{
			DeleteMenu(theMenu->parentHandle,theMenu->menuInfo->wID,MF_BYCOMMAND);
		}
	}

	if(theMenu->parentHandle==editorTopMenu)
	{
		DrawMenuBar(frameWindowHwnd);
	}
}

static EDITORMENU *CreateEditorMenuElement(char *theName,char *theAttributes,char *dataText)
// create an editor menu structure (not linked to anything, but fill in
// theName, and theAttributes)
// if there is a problem, set the error, and return NULL
{
	EDITORMENU
		*theMenu;

	if((theMenu=(EDITORMENU *)MNewPtr(sizeof(EDITORMENU))))											// create editor menu element
	{
		if((theMenu->itemText=(char *)MNewPtr(strlen(theName)+1)))
		{
			strcpy(theMenu->itemText,theName);														// copy the base menu name
			if((theMenu->attributes=(char *)MNewPtr(strlen(theAttributes)+1)))
			{
				strcpy(theMenu->attributes,theAttributes);											// copy the attributes
				if((theMenu->dataText=(char *)MNewPtr(strlen(dataText)+1)))	
				{
					strcpy(theMenu->dataText,dataText);												// copy the data text for this menu
					if(theMenu->menuInfo=(MENUITEMINFO *)MNewPtr(sizeof(MENUITEMINFO)))				// make an MS Windows menu info block
					{
						theMenu->menuInfo->cbSize=sizeof(MENUITEMINFO);								// how large it is
						theMenu->menuInfo->wID=globalMenuID++;										// the ID Windows will know us by
						theMenu->menuInfo->hSubMenu = NULL;											// we have no submenu yet
						theMenu->parentHandle = NULL;												// we have no Windows parent menu yet
						theMenu->menuInfo->dwTypeData = theMenu->itemText;							// this is the default menu display name, it may change
						theMenu->menuInfo->fMask = MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE;	// set all the menu information
						if(RealizeAttributes(theMenu))												// parse through the attribute string, set up font, and command string
						{
							return(theMenu);
						}
					MDisposePtr(theMenu->menuInfo);
					}
				MDisposePtr(theMenu->dataText);
				}
				MDisposePtr(theMenu->attributes);
			}
			MDisposePtr(theMenu->itemText);
		}
		MDisposePtr(theMenu);
	}
	return(NULL);
}

static void DisposeEditorMenuElement(EDITORMENU *theMenu)
// dispose of an editor menu structure
{
	UnRealizeAttributes(theMenu);
	MDisposePtr(theMenu->dataText);
	MDisposePtr(theMenu->attributes);
	MDisposePtr(theMenu->itemText);
	MDisposePtr(theMenu->menuInfo);
	MDisposePtr(theMenu);
}

static EDITORMENU *CreateEditorMenuAtBase(EDITORMENU *baseMenu,int menuRelationship,char *theName,char *theAttributes,char *dataText,bool active)
// create a new menu with theName, as menuRelationship to baseMenu
// and return a pointer to it
// if baseMenu is NULL it indicates the top of the hierarchy
// therefore, menuRelationship must be MR_FIRSTCHILD, or MR_LASTCHILD as it is not possible to
// create menus which are siblings of the root
// theAttributes is a machine specific string that describes machine specific attributes of the menu
// if active is false, the menu should be created inactive
// if there is a problem, set the error, and return NULL
{
	EDITORMENU
		*theMenu;

	if((theMenu=CreateEditorMenuElement(theName,theAttributes,dataText)))
	{
		theMenu->active=active;												// set the active state
			if(LinkEditorMenuElement(theMenu,baseMenu,menuRelationship))
			{
				if(CreateWindowsMenuItem(baseMenu,theMenu,menuRelationship))	// make the MSWindows menu item
				{
					return(theMenu);
				}
			}
		DisposeEditorMenuElement(theMenu);
	}
	return(NULL);
}

void DisposeEditorMenu(EDITORMENU *theMenu)
// remove theMenu (and any children it may have)
// if NULL is passed, dispose of ALL menus
// NOTE: this calls itself recursively
{
	EDITORMENU
		*theParent,
		*subMenu;

	while((subMenu=GetRelatedEditorMenu(theMenu,MR_LASTCHILD)))	// see if this one has any children
	{
		DisposeEditorMenu(subMenu);
	}
	if(theMenu)
	{
		theParent=theMenu->parent;								// remember the parent, so after this is disposed of, he can be updated
		DeleteWindowsMenuItem(theMenu);							// delete the MSWindows menu item
		UnlinkEditorMenuElement(theMenu);						// unlink this item
		DisposeEditorMenuElement(theMenu);						// get rid of it
	}
}

bool EditorKeyNameToKeyCode(char *theName,UINT32 *theCode)
// given a string which is the name of an unmodified key on the keyboard,
// convert it into a keycode (in the case of X, we use KeySyms)
// if the name is not the name of any key on our keyboard, then return false
{
	KeySym
		theKeySym;

	if((theKeySym=XStringToKeysym(theName))!=NoSymbol)
	{
		*theCode=(UINT32)theKeySym;
		return(true);
	}
	return(false);
}

char *EditorKeyCodeToKeyName(UINT32 theCode)
// given a keycode, get its name
// if the code is invalid, return NULL
// NOTE: the returned string is static, and should not be modified or deleted
{
	return XKeysymToString((KeySym)theCode);
}

bool GetEditorMenu(int argc,char *argv[],EDITORMENU **theMenu)
// traverse the elements of the menu path given by argc,argv
// and attempt to locate the menu that they point to
// if the path is invalid, return false
// otherwise return true with theMenu set to the located item
// NOTE: on entry, theMenu is used as the starting point for
// the search, if it is passed as NULL, it indicates that the search
// should start at the top
{
	EDITORMENU
		*currentMenu,
		*testMenu;
	int
		elementIndex;
	bool
		matching;

	currentMenu=testMenu=*theMenu;
	matching=true;
	for(elementIndex=0;(elementIndex<argc)&&matching;elementIndex++)
	{
		if(currentMenu)							// locate root of current menu
		{
			testMenu=currentMenu->firstChild;
		}
		else
		{
			testMenu=firstMenu;
		}
		matching=false;
		while(testMenu&&!matching)
		{
			if(strcmp(argv[elementIndex],testMenu->itemText)==0)
			{
				matching=true;					// located a match, this is the menu we want
			}
			else
			{
				testMenu=GetRelatedEditorMenu(testMenu,MR_NEXTSIBLING);
			}
		}
		currentMenu=testMenu;
	}
	*theMenu=testMenu;
	return(matching);
}

EDITORMENU *CreateEditorMenu(int argc,char *argv[],int menuRelationship,char *theName,char *theAttributes,char *dataText,bool active)
// create a new menu with theName, as menuRelationship to the passed argc,argv path
// the path is absolute within the menu hierarchy
// theAttributes is a machine specific string that describes machine specific attributes of the menu
// if active is false, the menu should be created inactive
// if there is a problem, set the error, and return false
{
	EDITORMENU
		*parentMenu,
		*testMenu;
	bool
		fail;

	parentMenu=NULL;									// start at the top
	if(GetEditorMenu(argc,argv,&parentMenu))			// locate item to link this one to
	{
		testMenu=parentMenu;
		fail=false;
		if(menuRelationship==MR_NEXTSIBLING||menuRelationship==MR_PREVIOUSSIBLING)	// see if hunting for sibling
		{
			if(parentMenu)								// if he wants sibling, better not be at top
			{
				testMenu=testMenu->parent;				// back up one
				if(GetEditorMenu(1,&theName,&testMenu))	// see if an item already exists with this name in the place we are putting it
				{
					if(testMenu!=parentMenu)			// cannot delete the one we want to add to!!
					{
						DisposeEditorMenu(testMenu);	// kill it before adding new item
					}
					else
					{
						SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
						fail=true;
					}
				}
			}
			else
			{
				SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
				fail=true;
			}
		}
		else
		{
			if(GetEditorMenu(1,&theName,&testMenu))			// see if an item already exists with this name in the place we are putting it
			{
				DisposeEditorMenu(testMenu);				// kill it before adding new item
			}
		}
		
		if(!fail)
		{
			return(CreateEditorMenuAtBase(parentMenu,menuRelationship,theName,theAttributes,dataText,active));
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[BADPATH],errorDescriptions[BADPATH]);
	}
	return(NULL);
}

bool ProcessWindowsMenuEvent(UINT32 menuID)
// Passed a menuID, see if its one of our menu items.
// If so, process the message, and return TRUE, else return FALSE that the message was not handled
// by us.
{
	bool
		handled;
	EDITORMENU
		*theMenu;

	handled=false;
	if(theMenu=LocateMenuWithItem(NULL,menuID))					// find the menu by Item number
	{
		HandleMenuEvent(theMenu);								// call the shell to handle this menu
		handled=true;
	}
	return(handled);
}


bool HandleMenuKeyEvent(UINT32 theKeySym,UINT32 keyModifiers)
// Passed a KeySym and keyModifiers, see if there is a menu item for the key.
// If so, process the message, and return TRUE, else return FALSE that the message was not handled
// by us.
{
	bool
		handled;
	EDITORMENU
		*theMenu;

	handled=false;
	if(theMenu=LocateMenuWithKeyAndModifers(NULL, theKeySym, keyModifiers))	// find the menu by Item number
	{
		HandleMenuEvent(theMenu);											// call the shell to handle this menu
		handled=true;
	}
	return(handled);
}

void UninitMenuManager()
{
	DestroyMenu(mdiMenu);
	DisposeEditorMenu(NULL);												// get rid of any menus that were created
	DestroyMenu(editorTopMenu);
}

bool InitMenuManager()
//	Initialize the menu manager. This creates a menu for the MDI frame window, creates the MDI client
//	window, and a empty menu for the MDI client window.
{
	CLIENTCREATESTRUCT
		ccs;
	RECT
		tmpRect;
	LPARAM
		tmp;

	firstMenu=NULL;
	globalMenuID=MAX_MENU_ITEMS;

	if(editorTopMenu=CreateMenu())
	{
		if(SetMenu(frameWindowHwnd,editorTopMenu))
		{
			DrawMenuBar(frameWindowHwnd);
			if(mdiMenu=CreateMenu())
			{
				ccs.hWindowMenu=mdiMenu;
				ccs.idFirstChild=IDM_FIRSTCHILD;
				if(clientWindowHwnd=CreateWindow("MDICLIENT",0L,WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL|WS_HSCROLL|WS_MAXIMIZE,0,0,0,0,frameWindowHwnd,(HMENU)0xCAC,programInstanceHandle,(LPSTR)&ccs))
				{
					ShowWindow(clientWindowHwnd,SW_SHOW);
					GetClientRect(frameWindowHwnd,&(tmpRect));
					tmp=tmpRect.right-tmpRect.left;
					tmp=tmp+((tmpRect.bottom-tmpRect.top)<<16);
					SendMessage(frameWindowHwnd,WM_SIZE,SIZE_MAXSHOW,tmp);
					UpdateWindow(frameWindowHwnd);
					return(true);
				}
				DestroyMenu(mdiMenu);
			}
			SetMenu(frameWindowHwnd,NULL);
		}
		DestroyMenu(editorTopMenu);
	}
	return(false);
}

char *GetEditorMenuDataText(EDITORMENU *theMenu)
// return a pointer to the data text string for theMenu
{
	return(theMenu->dataText);
}
