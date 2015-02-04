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


extern int
	errno;

extern Display
	*xDisplay;
extern int
	xScreenNum;
extern Colormap
	xColormap;

extern bool
	xFail;

extern Cursor
	arrowCursor,
	caretCursor;

extern GUICOLOR
	*black,
	*white,
	*gray0,
	*gray1,
	*gray2,
	*gray3;

extern UINT32
	showingBusy;

extern bool
	verticalScrollBarOnLeft;

extern GUICOLOR
	*statusBackgroundColor,
	*statusForegroundColor;

extern Pixmap
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

extern Atom
	selectionAtom,
	incrAtom,
	takeFocusAtom,
	deleteWindowAtom;

extern WINDOWLISTELEMENT
	*windowListHead;

extern EDITORWINDOW
	*rootMenuWindow;

extern volatile UINT32
	timer;

extern bool
	timeToQuit;
