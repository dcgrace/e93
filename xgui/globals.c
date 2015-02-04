// Globals needed by the GUI
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

Display
	*xDisplay;				// the x display that the editor is currently running on
int
	xScreenNum;				// the screen number on xDisplay where we will be opening windows
Colormap
	xColormap;				// tells which color map is in use

bool
	xFail;					// set by X's error vector (so we can tell when certain routines which neglect to return status fail)

Cursor
	arrowCursor,			// arrow cursor
	caretCursor;			// insertion cursor definition

GUICOLOR
	*black,					// black
	*white,					// white
	*gray0,					// darkest gray
	*gray1,					// medium dark gray
	*gray2,					// medium light gray
	*gray3;					// lightest gray

UINT32
	showingBusy;			// tells depth of busy being shown

bool
	verticalScrollBarOnLeft;	// true if the vertical scroll bar should be placed at the left hand side of document windows

GUICOLOR
	*statusBackgroundColor,	// status bar colors
	*statusForegroundColor;

Pixmap
	leftArrowPixmap,
	rightArrowPixmap,
	upArrowPixmap,
	downArrowPixmap,
	checkBoxPixmap,
	checkedBoxPixmap,
	subMenuPixmap,
	documentIconPixmap,
	documentModifiedIconPixmap,
	editorIconPixmap;

Atom
	selectionAtom,			// atoms defined at startup
	incrAtom,
	takeFocusAtom,
	deleteWindowAtom;

WINDOWLISTELEMENT
	*windowListHead;		// head of the window list

EDITORWINDOW
	*rootMenuWindow;		// this keeps track of the root of the menu window hierarchy (also base window for application)

volatile UINT32
	timer;					// this runs at the alarm rate, and is used to create short delays, it is never reset

bool
	timeToQuit;
