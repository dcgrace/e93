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


#define		PATHSEP						'/'			// path separation character

// some default colors (in EDITORCOLOR representation)
#define		GRAY0						0x333333	// default gray colors
#define		GRAY1						0x666666
#define		GRAY2						0x999999
#define		GRAY3						0xCCCCCC

#define		WHITE						0xFFFFFF
#define		BLACK						0x000000

#define		XWINDOWBORDERWIDTH			0		// border width used when creating x windows
#define		DEFAULTVIEWFONT				"-*-lucidatypewriter-medium-r-normal-sans-12-*-*-*-*-*-*-*"	// good font for viewing text
#define		DEFAULTMENUFONT				"-*-new century schoolbook-bold-r-normal--14-*-*-*-*-*-*-*"
#define		DEFAULTDIALOGBUTTONFONT		"8x13bold"													// font to use for dialog buttons
#define		DEFAULTDIALOGSTATICFONT		"8x13bold"													// font to use for dialog static text

// stuff needed to make the gui interface go

// this definition sets EDITORFILE to the structure needed by the gui
struct editorFile
{
	int
		fileHandle;
};

// this definition sets EDITORMENU to the structure needed by the gui
struct editorMenu				// actual structure of an editor menu
{
	EDITORWINDOW
		*theWindow;				// if this menu currently has a window, it is pointed to by this (otherwise, its NULL)
	char
		*itemText;				// name of the item is stored here (semi-redundant with OS's name, but easier to get to)
	char
		*attributes;			// attribute string is stored here
	char
		*dataText;				// the text of the menu (used when menu is clicked on)
	char
		*commandString;			// command string is kept here (this is the string that is displayed at the end of the menu when there is a key sequence that can be used to select it)
	bool
		hasKey;					// true if this item has a key code associated with it
	KeySym
		theKeySym;				// keySymCode that we are looking for (if any)
	GUIFONT
		*theFont;				// the font that is used for this menu item
	bool
		active;					// tells if the menu is active (can be selected) or not
	bool
		separator;				// tells if the menu item is a separator
	bool
		highlight;				// when a menu button is drawn, this tells if it should draw in its highlighted (pressed) state
	EDITORMENU					// linked list of menus
		*parent,
		*nextSibling,
		*previousSibling,
		*firstChild;
};

// this definition sets EDITORTASK to the structure needed by the gui
struct editorTask
{
	int
		pid;						// process id of task
	bool
		closedStdin;				// set true if EOF has been sent to task by closing our half of its input pipe
	int
		fdin[2];					// file handles used to pipe data between us and the task (if one is active)
	int
		fdout[2];
};

// editor window types
enum
{
	EWT_DOCUMENT,					// an actual window used to edit text
	EWT_DIALOG,						// a dialog window
	EWT_MENU						// menu window
};
