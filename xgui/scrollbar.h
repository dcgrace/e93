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


// defines and data structures for scrollBars

#define	SCROLLBARWIDTH	20										// number of pixels wide that the scroll bars are
#define	ARROWLENGTH		20										// number of pixels long that the arrows are (if possible)
#define	THUMBLENGTH		20										// minimum number of pixels long that the thumb is (if possible)

typedef struct scrollBar
{
	EDITORWINDOW
		*parentWindow;											// the window that contains this scroll bar
	bool
		horizontal;
	INT32
		x,
		y;
	UINT32
		length;													// length in pixels of the scroll bar
	UINT32
		thumbMax;												// indicates the maximum number of positions for the thumb
	UINT32
		thumbPosition;											// indicates the current position of the thumb
	bool
		highlightLowArrow,										// used during tracking to remember what is highlighted
		highlightHighArrow,
		highlightLowPage,
		highlightHighPage,
		highlightThumb;
	void
		(*thumbProc)(struct scrollBar *theScrollBar,UINT32 newPosition,void *parameters);		// routine called when scroll bar is tracking the thumb
	void
		(*stepLowerProc)(struct scrollBar *theScrollBar,bool repeating,void *parameters);	// routine called when scroll bar is stepping to lower values
	void
		(*stepHigherProc)(struct scrollBar *theScrollBar,bool repeating,void *parameters);	// routine called when scroll bar is stepping to higher values
	void
		(*pageLowerProc)(struct scrollBar *theScrollBar,bool repeating,void *parameters);	// routine called when scroll bar is paging to lower values
	void
		(*pageHigherProc)(struct scrollBar *theScrollBar,bool repeating,void *parameters);	// routine called when scroll bar is paging to higher values
	void
		*procParameters;										// data passed to routines
} SCROLLBAR;

typedef struct scrollBarDescriptor								// describes a scroll bar to creation routines
{
	bool
		horizontal;
	INT32
		x,
		y;
	UINT32
		length;													// length in pixels of the scroll bar
	UINT32
		thumbMax;												// indicates the maximum number of positions for the thumb
	UINT32
		thumbPosition;											// indicates the current position of the thumb
	void
		(*thumbProc)(struct scrollBar *theScrollBar,UINT32 newPosition,void *parameters);		// routine called when scroll bar is tracking the thumb
	void
		(*stepLowerProc)(struct scrollBar *theScrollBar,bool repeating,void *parameters);	// routine called when scroll bar is stepping to lower values
	void
		(*stepHigherProc)(struct scrollBar *theScrollBar,bool repeating,void *parameters);	// routine called when scroll bar is stepping to higher values
	void
		(*pageLowerProc)(struct scrollBar *theScrollBar,bool repeating,void *parameters);	// routine called when scroll bar is paging to lower values
	void
		(*pageHigherProc)(struct scrollBar *theScrollBar,bool repeating,void *parameters);	// routine called when scroll bar is paging to higher values
	void
		*procParameters;										// data passed to routines
} SCROLLBARDESCRIPTOR;


bool PointInScrollBar(SCROLLBAR *theScrollBar,INT32 x,INT32 y);
void DrawScrollBar(SCROLLBAR *theScrollBar);
bool TrackScrollBar(SCROLLBAR *theScrollBar,XEvent *theEvent);
void RepositionScrollBar(SCROLLBAR *theScrollBar,INT32 x,INT32 y,UINT32 length,bool horizontal);
void AdjustScrollBar(SCROLLBAR *theScrollBar,UINT32 thumbMax,UINT32 thumbPosition);
SCROLLBAR *CreateScrollBar(EDITORWINDOW *theWindow,SCROLLBARDESCRIPTOR *theDescription);
void DisposeScrollBar(SCROLLBAR *theScrollBar);
