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


// defines and data structures for static text

#define	MAXSTATICTEXTSTRING	512

typedef struct staticText
{
	EDITORWINDOW
		*parentWindow;											// the window that contains this text
	GUICOLOR
		*backgroundColor,										// color to use when drawing background
		*foregroundColor;										// color to use when drawing foreground
	EDITORRECT
		theRect;
	GUIFONT
		*theFont;												// info about currently selected font
	char
		theString[MAXSTATICTEXTSTRING];
} STATICTEXT;

typedef struct staticTextDescriptor								// describes static text to creation routines
{
	EDITORRECT
		theRect;
	EDITORCOLOR
		backgroundColor;										// color to use when drawing background
	EDITORCOLOR
		foregroundColor;										// color to use when drawing foreground
	char
		*theString;
	char
		*fontName;
} STATICTEXTDESCRIPTOR;

void ResetStaticTextItemText(DIALOGITEM *theItem,char *newString);
bool CreateStaticTextItem(DIALOGITEM *theItem,void *theDescription);
