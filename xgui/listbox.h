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


// defines and data structures for list boxes

#define	MAXLISTCHARBUFFER	64			// max characters in the character buffer
#define	LISTCHARTIMEOUT		40			// max number of ticks between consecutive typed characters
#define	LISTBOXBORDERWIDTH	3			// width of border that surrounds the list box

typedef struct listBox
{
	EDITORWINDOW
		*parentWindow;					// the window that contains this list
	EDITORRECT
		theRect;						// position within the parent
	GUICOLOR
		*focusBackgroundColor,			// color to use when drawing background during focus
		*focusForegroundColor,			// color to use when drawing foreground during focus
		*nofocusBackgroundColor,		// color to use when drawing background during non focus
		*nofocusForegroundColor;		// color to use when drawing foreground during non focus
	bool
		verticalScrollBarOnLeft;		// gives the preference for vertical scroll bar placement
	SCROLLBAR
		*theScrollBar;					// pointer to the scrollbar attached to this list
	UINT32
		numElements;					// tells how many elements are in the list
	char
		**listElements;					// pointer to NULL terminated array of pointers to items in the list
	bool
		*selectedElements;				// pointer to array of boolS which tell which items are selected
	UINT32
		currentElement;					// this is the element last operated on
	UINT32
		numLines;						// tells how many lines in the current font, and list box size can be displayed
	UINT32
		topLine;						// tells which line is displayed at the top of the list box
	UINT32
		lineHeight;						// height of a line of text
	UINT32
		charBufferLength;				// number of characters in the charBuffer
	char
		charBuffer[MAXLISTCHARBUFFER];	// keeps track of the characters typed
	UINT32
		charTimer;						// keeps track of the time between characters when hunting for an item (if the time is too long between characters, charBuffer is flushed)
	GUIFONT
		*theFont;						// info about currently selected font
	void
		(*pressedProc)(struct listBox *theListBox,void *parameters);	// routine called when the list items have been chosen (by double clicking, or hitting return in the list)
	void
		*pressedProcParameters;			// data passed to pressedProc routine
} LISTBOX;

typedef struct listBoxDescriptor		// describes a list box to creation routines
{
	EDITORRECT
		theRect;						// position and size within parent window
	EDITORCOLOR
		focusBackgroundColor,			// color to use when drawing background during focus
		focusForegroundColor,			// color to use when drawing foreground during focus
		nofocusBackgroundColor,			// color to use when drawing background during non focus
		nofocusForegroundColor;			// color to use when drawing foreground during non focus
	UINT32
		numElements;					// tells how many elements are in the list
	char
		**listElements;					// pointer to NULL terminated array of pointers to items in the list
	bool
		*selectedElements;				// pointer to array of bools which tell which items should be selected at startup
	UINT32
		topLine;						// tells which line is displayed at the top of the list box when it opens
	char
		*fontName;						// name of font to use for list items
	void
		(*pressedProc)(struct listBox *theListBox,void *parameters);	// routine called when the list items have been chosen (by double clicking in the list)
	void
		*pressedProcParameters;			// data passed to pressedProc routine
} LISTBOXDESCRIPTOR;


bool PointInListBox(LISTBOX *theListBox,INT32 x,INT32 y);
void DrawListBox(LISTBOX *theListBox,bool hasFocus);
bool TrackListBox(LISTBOX *theListBox,XEvent *theEvent);
void RepositionListBox(LISTBOX *theListBox,EDITORRECT *newRect);
void HandleListBoxKeyEvent(LISTBOX *theListBox,XEvent *theEvent);
void ResetListBoxLists(LISTBOX *theListBox,UINT32 numElements,char **theList,bool *selectedElements);
LISTBOX *CreateListBox(EDITORWINDOW *theWindow,LISTBOXDESCRIPTOR *theDescription);
void DisposeListBox(LISTBOX *theListBox);
void ResetListBoxItemLists(DIALOGITEM *theItem,UINT32 numElements,char **theList,bool *selectedElements);
bool CreateListBoxItem(DIALOGITEM *theItem,void *theDescriptor);
