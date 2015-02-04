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


// defines and data structures for check boxes

#define	MAXCHECKBOXNAME		256									// maximum length of check box name (including terminator)

typedef struct checkBox
{
	EDITORWINDOW
		*parentWindow;											// the window that contains this check box
	EDITORRECT
		theRect;
	GUIFONT
		*theFont;												// info about currently selected font
	char
		checkBoxName[MAXCHECKBOXNAME];
	bool
		highlight;												// used during tracking to remember if highlighted
	bool
		hasKey;													// true if this check box has a key code associated with it
	KeySym
		theKeySym;												// equivalent key code for this check box
	bool
		*pressedState;											// boolean set to true if check box pressed
	void
		(*pressedProc)(struct checkBox *theCheckBox,void *parameters);	// routine called when the check box has been pressed
	void
		*pressedProcParameters;									// data passed to pressedProc routine
} CHECKBOX;

typedef struct checkBoxDescriptor								// describes a check box to creation routines
{
	EDITORRECT
		theRect;
	char
		*checkBoxName;
	char
		*fontName;
	bool
		hasKey;													// true if this check box has a key code associated with it
	KeySym
		theKeySym;												// equivalent key code for this check box
	bool
		*pressedState;											// boolean set to true if check box checked
	void
		(*pressedProc)(struct checkBox *theCheckBox,void *parameters);	// routine called when the check box has been pressed
	void
		*pressedProcParameters;									// data passed to pressedProc routine
} CHECKBOXDESCRIPTOR;

bool CreateCheckBoxItem(DIALOGITEM *theItem,void *theDescriptor);
