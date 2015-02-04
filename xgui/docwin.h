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


#define	EDITMINWIDTH		100
#define	EDITMAXWIDTH		((1<<30)-1)
#define	EDITMINHEIGHT		100
#define	EDITMAXHEIGHT		((1<<30)-1)

#define	HSCROLLMAXWIDTH		32767	// maximum width handled by the horizontal scroll bar

#define EDITTOPBORDER		0		// the view of an edit window is offset by these borders (strictly for looks)
#define EDITBOTTOMBORDER	0
#define EDITLEFTBORDER		0
#define EDITRIGHTBORDER		0

#define	STATUSBARFONT		"-*-lucidatypewriter-medium-r-normal-sans-12-*-*-*-*-*-*-*"

#define	STATUSBARLINEHEIGHT		2

void AdjustDocumentWindowStatus(EDITORWINDOW *theWindow);
void UpdateDocumentWindow(EDITORWINDOW *theWindow);
void AdjustDocumentWindowForNewSize(EDITORWINDOW *theWindow);
void DocumentWindowButtonPress(EDITORWINDOW *theWindow,XEvent *theEvent);
void DocumentPeriodicProc(EDITORWINDOW *theWindow);
void DocumentWindowEvent(EDITORWINDOW *theWindow,XEvent *theEvent);
void MinimizeDocumentWindow(EDITORWINDOW *theWindow);
void UnminimizeDocumentWindow(EDITORWINDOW *theWindow);
void SetTopDocumentWindow(EDITORWINDOW *theWindow);
EDITORWINDOW *GetActiveDocumentWindow();
EDITORWINDOW *GetTopDocumentWindow();
bool GetSortedDocumentWindowList(UINT32 *numElements,EDITORWINDOW ***theList);
EDITORVIEW *GetDocumentWindowCurrentView(EDITORWINDOW *theWindow);
bool GetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle,UINT32 stringBytes);
bool SetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle);
bool GetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *foregroundColor);
bool SetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR foregroundColor);
bool GetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *backgroundColor);
bool SetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR backgroundColor);
bool GetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect);
bool SetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect);
EDITORWINDOW *OpenDocumentWindow(EDITORBUFFER *theBuffer,EDITORRECT *theRect,char *theTitle,char *fontName,UINT32 tabSize,EDITORCOLOR foreground,EDITORCOLOR background);
void CloseDocumentWindow(EDITORWINDOW *theWindow);
