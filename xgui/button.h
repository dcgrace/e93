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


// defines and data structures for buttons

#define	MAXBUTTONNAME		256									// maximum length of button name (including terminator)

typedef struct button
{
	EDITORWINDOW
		*parentWindow;											// the window that contains this button
	EDITORRECT
		theRect;
	GUIFONT
		*theFont;												// info about currently selected font
	char
		buttonName[MAXBUTTONNAME];
	bool
		highlight;												// used during tracking to remember if highlighted
	bool
		hasKey;													// true if this button has a key code associated with it
	KeySym
		theKeySym;												// equivalent key code for this button
	bool
		*pressedState;											// boolean set to true if button pressed
	void
		(*pressedProc)(struct button *theButton,void *parameters);	// routine called when the button has been pressed
	void
		*pressedProcParameters;									// data passed to pressedProc routine
} BUTTON;

typedef struct buttonDescriptor									// describes a button to creation routines
{
	EDITORRECT
		theRect;
	char
		*buttonName;
	char
		*fontName;
	bool
		hasKey;													// true if this button has a key code associated with it
	KeySym
		theKeySym;												// equivalent key code for this button
	bool
		*pressedState;											// boolean set to true if button pressed
	void
		(*pressedProc)(struct button *theButton,void *parameters);	// routine called when the button has been pressed
	void
		*pressedProcParameters;									// data passed to pressedProc routine
} BUTTONDESCRIPTOR;

void PressButtonItem(DIALOGITEM *theItem);
bool CreateButtonItem(DIALOGITEM *theItem,void *theDescriptor);
