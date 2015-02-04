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
	EDITORRECT
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
	Window
		cursorWindow;							// x InputOnly window used to make the cursor change to caret within the view (this is ALL it is used for!)
	EDITORRECT
		bounds;									// boundary of this view within its parent window
	EDITORRECT
		textBounds;								// roughly the bounds, but rounded down for the current font
	UINT32
		topLine;								// current first line of edit universe that is displayed in the view
	INT32
		leftPixel;								// current offset from left where text is being drawn in this view
	UINT32
		lineHeight;								// number of pixels per line vertically
	UINT32
		lineAscent;								// tells where the baseline of text is for each line in the view
	UINT32
		maxLeftOverhang;						// gives the maximum font character left overhang for any character in any style of this view
	UINT32
		maxRightOverhang;						// gives the maximum font character right overhang for any character in any style of this view
	UINT32
		numLines;								// number of lines that can be displayed
	UINT32
		tabSize;								// number of columns per tab
	GUICOLOR
		*selectionForegroundColor,				// color to use when drawing selection foreground (NULL if inversion should be used)
		*selectionBackgroundColor;				// color to use when drawing selection background (NULL if inversion should be used)
	bool
		viewActive;								// if true, then view is displayed as active
	bool
		cursorOn;								// if true, and no selection, and view active, and blinkState, then cursor will be shown
	bool
		blinkState;								// tells what state (true = on) (false = off) the cursor blink is currently in
	UINT32
		nextBlinkTime;							// tells the next time that the cursor should change blink state
	bool
		oldCursorShowing;						// holds showing state of cursor across Start/EndCursorChange
	SELECTIONUNIVERSE
		*heldSelectionUniverse;					// keeps track of selections that were in the view across calls to Start/EndSelectionChange
	STYLEUNIVERSE
		*heldStyleUniverse;						// keeps track of styles that were in the view across calls to Start/EndStyleChange
	GUIVIEWSTYLE
		viewStyles[VIEWMAXSTYLES];				// array of styles
} GUIVIEWINFO;

bool PointInView(EDITORVIEW *theView,INT32 x,INT32 y);
void LocalClickToViewClick(EDITORVIEW *theView,INT32 clickX,INT32 clickY,INT32 *xPosition,INT32 *lineNumber);
void HandleViewKeyEvent(EDITORVIEW *theView,XEvent *theEvent);
void GetEditorViewGraphicToTextPosition(EDITORVIEW *theView,UINT32 linePosition,INT32 xPosition,UINT32 *betweenOffset,UINT32 *charOffset);
void GetEditorViewTextToGraphicPosition(EDITORVIEW *theView,UINT32 thePosition,INT32 *xPosition,bool limitMax,UINT32 *slopLeft,UINT32 *slopRight);
void GetEditorViewTextInfo(EDITORVIEW *theView,UINT32 *topLine,UINT32 *numLines,INT32 *leftPixel,UINT32 *numPixels);
void DrawView(EDITORVIEW *theView);
void SetViewBounds(EDITORVIEW *theView,EDITORRECT *newBounds);
void DisposeEditorView(EDITORVIEW *theView);
EDITORVIEW *CreateEditorView(EDITORWINDOW *theWindow,EDITORVIEWDESCRIPTOR *theDescriptor);
void InvalidateViewPortion(EDITORVIEW *theView,UINT32 startLine,UINT32 endLine,UINT32 startPixel,UINT32 endPixel);
void ScrollViewPortion(EDITORVIEW *theView,UINT32 startLine,UINT32 endLine,INT32 numLines,UINT32 startPixel,UINT32 endPixel,INT32 numPixels);
void SetViewTopLeft(EDITORVIEW *theView,UINT32 newTopLine,INT32 newLeftPixel);
void SetEditorViewTopLine(EDITORVIEW *theView,UINT32 lineNumber);
UINT32 GetEditorViewTabSize(EDITORVIEW *theView);
bool SetEditorViewTabSize(EDITORVIEW *theView,UINT32 theSize);
bool GetEditorViewStyleFont(EDITORVIEW *theView,UINT32 theStyle,char *theFont,UINT32 stringBytes);
bool SetEditorViewStyleFont(EDITORVIEW *theView,UINT32 theStyle,char *theFont);
void ClearEditorViewStyleFont(EDITORVIEW *theView,UINT32 theStyle);
bool GetEditorViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR *foregroundColor);
bool SetEditorViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR foregroundColor);
void ClearEditorViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle);
bool GetEditorViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR *backgroundColor);
bool SetEditorViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR backgroundColor);
void ClearEditorViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle);
bool GetEditorViewSelectionForegroundColor(EDITORVIEW *theView,EDITORCOLOR *foregroundColor);
bool SetEditorViewSelectionForegroundColor(EDITORVIEW *theView,EDITORCOLOR foregroundColor);
void ClearEditorViewSelectionForegroundColor(EDITORVIEW *theView);
bool GetEditorViewSelectionBackgroundColor(EDITORVIEW *theView,EDITORCOLOR *backgroundColor);
bool SetEditorViewSelectionBackgroundColor(EDITORVIEW *theView,EDITORCOLOR backgroundColor);
void ClearEditorViewSelectionBackgroundColor(EDITORVIEW *theView);
void ResetEditorViewCursorBlink(EDITORVIEW *theView);
void ViewCursorTask(EDITORVIEW *theView);
void ViewsStartSelectionChange(EDITORBUFFER *theBuffer);
void ViewsEndSelectionChange(EDITORBUFFER *theBuffer);
void ViewsStartStyleChange(EDITORBUFFER *theBuffer);
void ViewsEndStyleChange(EDITORBUFFER *theBuffer);
void ViewsStartTextChange(EDITORBUFFER *theBuffer);
void ViewsEndTextChange(EDITORBUFFER *theBuffer);
bool TrackView(EDITORVIEW *theView,XEvent *theEvent);
void EditorActivateView(EDITORVIEW *theView);
void EditorDeactivateView(EDITORVIEW *theView);
