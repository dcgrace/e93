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


#define	VIEWTOPMARGIN			4		// number of pixels to leave at the top of the view
#define	VIEWBOTTOMMARGIN		4		// number of pixels to leave at the bottom of the view
#define	VIEWLEFTMARGIN			10		// number of pixels to the left of the text in the view that we call the left hand side

#define	VIEWMAXSTYLES			128		// maximum number of styles in a view (must be a power of 2)

typedef struct viewDescriptor			// describes a view to creation routines
{
	EDITORBUFFER
		*theBuffer;						// the buffer on which to view
	RECT
		theRect;						// position and size within parent window
	UINT32
		topLine;
	INT32
		leftPixel;
	UINT32
		tabSize;
	EDITORCOLOR
		backgroundColor;				// default color to use when drawing background (style 0)
	EDITORCOLOR
		foregroundColor;				// default color to use when drawing foreground (style 0)
	char
		*fontName;						// default font to use (style 0)
	bool
		active;							// tells if the view is created active or not
	void
		(*viewTextChangedVector)(struct editorView *theView);	// this vector is called by the view when it has finished a text change
	void
		(*viewSelectionChangedVector)(struct editorView *theView);	// this vector is called by the view when it has finished a selection change
	void
		(*viewPositionChangedVector)(struct editorView *theView);	// this vector is called by the view when it has finished a position change
} EDITORVIEWDESCRIPTOR;

typedef struct
{
	GUIFONT
		*theFont;								// pointer to font in use (NULL if default should be used)
	GUICOLOR
		*foregroundColor,						// color to use when drawing foreground (NULL if default should be used)
		*backgroundColor;						// color to use when drawing background (NULL if default should be used)
} GUIVIEWSTYLE;

typedef struct
{
	RECT
		bounds;				/* boundary of this view within its parent window */
	RECT
		textBounds;			/* roughly the bounds, but rounded down for the current font */
	UINT32
		topLine;			/* current first line of edit universe that is displayed in the view */
	INT32
		leftPixel;			/* current offset from left where text is being drawn in this view */
	UINT32
		lineHeight;			/* number of pixels per line vertically */
	UINT32
		lineAscent;			// tells where the baseline of text is for each line in the view
	UINT32
		numLines;			/* number of lines that can be displayed */
	UINT32
		tabSize;			/* number of columns per tab */
	UINT32
		minCharWidth,
		maxLeftOverhang,
		maxRightOverhang;
	GUICOLOR
		*selectionForegroundColor,				// color to use when drawing selection foreground (NULL if inversion should be used)
		*selectionBackgroundColor;				// color to use when drawing selection background (NULL if inversion should be used)
	GUICOLOR
		*backgroundColor,						// color to use when drawing background
		*foregroundColor;						// color to use when drawing foreground
	HBRUSH
		xorBrush;			/* a brush of the XOR of foregroundColor and backgroundColor for the cursor */
	HBITMAP
		lineHBitmap;
	bool
		viewActive;			/* if TRUE, then view is displayed as active */
	bool
		cursorOn;			/* if TRUE, and no selection, and view active, and blinkState, then cursor will be shown */
	bool
		oldCursorShowing;	/* holds showing state of cursor across Start/EndCursorChange */
	bool
		blinkState;			/* tells what state (TRUE = on) (FALSE = off) the cursor blink is currently in */
	HRGN
		oldSelectedRegion;	/* holds the view's selected region across calls to Start/EndSelectionChange */
	char
		*titleString;
	STYLEUNIVERSE
		*heldStyleUniverse;						// keeps track of styles that were in the view across calls to Start/EndStyleChange
	GUIVIEWSTYLE
		viewStyles[VIEWMAXSTYLES];				// array of styles
} GUIVIEWINFO;

HFONT CreateFontForDC(HDC hdc,char *theFont);
bool SetupViewFont(EDITORVIEW *theView,char *theFont);
void BlinkViewCursor(EDITORVIEW *theView);
void ViewStartSelectionChange(EDITORVIEW *theView);
void ViewEndSelectionChange(EDITORVIEW *theView);
void ResetCursorBlinkTime(EDITORVIEW *theView);
void ViewsStartTextChange(EDITORBUFFER *theUniverse);
void DrawView(HWND hwnd,HDC hdc,HRGN updateRegion);
EDITORVIEW *CreateEditorView(EDITORWINDOW *theWindow,EDITORVIEWDESCRIPTOR *theDescriptor);
void DisposeEditorView(EDITORVIEW *theView);
void RecalculateViewFontInfo(EDITORVIEW *theView);
