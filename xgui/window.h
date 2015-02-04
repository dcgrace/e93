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


// this is the WINDOWLISTELEMENT structure which allows us to get
// back from an X window to the editor window structures

typedef struct windowListElement				// this structure is used to locate our data based on an x window ID
{
	Window
		xWindow;								// x window id
	Window
		realTopWindow;							// since window managers can reparent my windows (bad concept), and since it is necessary keep track of the hierarchy of editor windows, when a window is created, the true root level parent is determined and stored here
	GC
		graphicsContext;						// graphics context for the given window
	Region
		invalidRegion;							// the region that collects our invalidation information
	EDITORWINDOW
		*theEditorWindow;						// pointer to our editor window structure
	Pixmap
		currentIcon;							// tells which icon is currently in use for the window (NULL if none)
	struct windowListElement
		*nextElement,							// next one of our windows, or NULL if none
		*previousElement;						// previous window, or NULL if none
} WINDOWLISTELEMENT;


void GetEditorScreenDimensions(UINT32 *theWidth,UINT32 *theHeight);
void InvalidateWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect);
void InvalidateWindowRegion(EDITORWINDOW *theWindow,Region theRegion);
void RedrawInvalidWindows();
void SetRealTopWindow(WINDOWLISTELEMENT *theElement);
void WaitUntilMappedNoFocus(EDITORWINDOW *theWindow);
void WaitUntilMapped(EDITORWINDOW *theWindow);
void WaitUntilDestroyed(EDITORWINDOW *theWindow);
void LinkWindowElement(WINDOWLISTELEMENT *theElement);
void UnlinkWindowElement(WINDOWLISTELEMENT *theElement);
WINDOWLISTELEMENT *LocateWindowElement(Window xWindow);
void MinimizeEditorWindow(EDITORWINDOW *theWindow);
void UnminimizeEditorWindow(EDITORWINDOW *theWindow);
EDITORWINDOW *GetTopEditorWindow();
void SetTopEditorWindow(EDITORWINDOW *theWindow);
EDITORWINDOW *GetTopEditorWindowType(UINT16 windowType);
bool GetSortedEditorWindowTypeList(UINT16 windowType,UINT32 *numElements,EDITORWINDOW ***theList);
EDITORWINDOW *GetEditorFocusWindow();
void FocusAwayFrom(EDITORWINDOW *theWindow);
