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


// defines and data structures for text boxes

#define	TEXTBOXBORDERWIDTH	3

typedef struct textBox
{
	EDITORWINDOW
		*parentWindow;					// the window that contains this text box
	EDITORRECT
		theRect;						// position and size within parent window
	EDITORCOLOR
		focusBackgroundColor,			// color to use when drawing background when we have the focus
		focusForegroundColor,			// color to use when drawing foreground when we have the focus
		nofocusBackgroundColor,			// color to use when drawing background when we do not have focus
		nofocusForegroundColor;			// color to use when drawing foreground when we do not have focus
	EDITORVIEW
		*theView;						// the view that this text box item made
} TEXTBOX;

typedef struct textBoxDescriptor		// describes a list box to creation routines
{
	EDITORBUFFER
		*theBuffer;						// the buffer on which to view
	EDITORRECT
		theRect;						// position and size within parent window
	UINT32
		topLine;
	INT32
		leftPixel;
	UINT32
		tabSize;
	EDITORCOLOR
		focusBackgroundColor,			// color to use when drawing background when it has focus
		focusForegroundColor,			// color to use when drawing foreground when it has focus
		nofocusBackgroundColor,			// color to use when drawing background when it does not have focus
		nofocusForegroundColor;			// color to use when drawing foreground when it does not have focus
	char
		*fontName;						// name of font to use
} TEXTBOXDESCRIPTOR;

bool PointInTextBox(TEXTBOX *theTextBox,INT32 x,INT32 y);
void DrawTextBox(TEXTBOX *theTextBox,bool hasFocus);
bool TrackTextBox(TEXTBOX *theTextBox,XEvent *theEvent);
void RepositionTextBox(TEXTBOX *theTextBox,EDITORRECT *newRect);
void HandleTextBoxKeyEvent(TEXTBOX *theTextBox,XEvent *theEvent);
void ActivateTextBox(TEXTBOX *theTextBox);
void DeactivateTextBox(TEXTBOX *theTextBox);
void HandleTextBoxPeriodicProc(TEXTBOX *theTextBox);
TEXTBOX *CreateTextBox(EDITORWINDOW *theWindow,TEXTBOXDESCRIPTOR *theDescription);
void DisposeTextBox(TEXTBOX *theTextBox);
bool CreateTextBoxItem(DIALOGITEM *theItem,void *theDescriptor);
