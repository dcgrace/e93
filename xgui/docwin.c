// Document window handling
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

typedef struct
{
	EDITORVIEW
		*theView;
	bool
		verticalScrollBarOnLeft;				// gives the preference for vertical scroll bar placement
	SCROLLBAR
		*verticalScrollBar,
		*horizontalScrollBar;
	GUIFONT
		*statusBarFont;							// info about currently selected status bar font
	GUICOLOR
		*backgroundColor,						// color to use when drawing background
		*foregroundColor;						// color to use when drawing foreground
} GUIDOCUMENTWINDOWINFO;

static void UnSetDocumentForegroundColor(EDITORWINDOW *theWindow)
// free the foreground color currently in use by theWindow (which must be a document window)
{
	GUIDOCUMENTWINDOWINFO
		*theWindowInfo;

	theWindowInfo=(GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo;
	FreeColor(theWindowInfo->foregroundColor);
}

static bool SetDocumentForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR foreground,bool removeOld)
// set the foreground color used to draw theWindow
{
	GUIDOCUMENTWINDOWINFO
		*theWindowInfo;
	GUICOLOR
		*newColor;

	theWindowInfo=(GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo;
	if((newColor=AllocColor(foreground)))
	{
		if(removeOld)
		{
			UnSetDocumentForegroundColor(theWindow);
		}
		theWindowInfo->foregroundColor=newColor;
		return(true);
	}
	return(false);
}

static void UnSetDocumentBackgroundColor(EDITORWINDOW *theWindow)
// free the background color currently in use by theWindow (which must be a document window)
{
	GUIDOCUMENTWINDOWINFO
		*theWindowInfo;

	theWindowInfo=(GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo;
	FreeColor(theWindowInfo->backgroundColor);
}

static bool SetDocumentBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR background,bool removeOld)
// set the background color used to draw theWindow
{
	GUIDOCUMENTWINDOWINFO
		*theWindowInfo;
	GUICOLOR
		*newColor;

	theWindowInfo=(GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo;
	if((newColor=AllocColor(background)))
	{
		if(removeOld)
		{
			UnSetDocumentBackgroundColor(theWindow);
		}
		theWindowInfo->backgroundColor=newColor;
		return(true);
	}
	return(false);
}

static void GetDocumentWindowRects(EDITORWINDOW *theWindow,EDITORRECT *contentRect,EDITORRECT *viewRect,EDITORRECT *verticalRect,EDITORRECT *horizontalRect,EDITORRECT *statusRect)
// get the rectangles for all the areas of a document window
{
	XWindowAttributes
		theAttributes;
	GUIFONT
		*statusBarFont;							// info about currently selected status bar font
	UINT32
		statusHeight;
	GUIDOCUMENTWINDOWINFO
		*theWindowInfo;

	theWindowInfo=(GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo;
	XGetWindowAttributes(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,&theAttributes);

	contentRect->x=theAttributes.border_width;
	contentRect->y=theAttributes.border_width;
	contentRect->w=theAttributes.width;
	contentRect->h=theAttributes.height;

	statusRect->x=theAttributes.border_width;
	statusRect->y=theAttributes.border_width;
	statusRect->w=theAttributes.width;

	statusBarFont=((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->statusBarFont;
	statusHeight=statusBarFont->ascent+statusBarFont->descent;

	statusRect->h=statusHeight+STATUSBARLINEHEIGHT;

	viewRect->x=theAttributes.border_width;					// move over for the left border
	viewRect->y=statusRect->y+statusRect->h;				// leave room for status bar

	if(theWindowInfo->verticalScrollBarOnLeft)
	{
		viewRect->x+=SCROLLBARWIDTH;						// if scroll bar on left, then step over it to begin the view
	}

	if(theAttributes.width>SCROLLBARWIDTH)
	{
		viewRect->w=theAttributes.width-SCROLLBARWIDTH;		// pull back for scroll bar, and right free area
	}
	else
	{
		viewRect->w=0;
	}
	if((UINT32)theAttributes.height>(statusHeight+STATUSBARLINEHEIGHT+SCROLLBARWIDTH))
	{
		viewRect->h=theAttributes.height-statusHeight-STATUSBARLINEHEIGHT-SCROLLBARWIDTH;		// pull back for bottom scroll bar, and bottom free area
	}
	else
	{
		viewRect->h=0;										// view cant fit!
	}

	if(theWindowInfo->verticalScrollBarOnLeft)
	{
		verticalRect->x=theAttributes.border_width;			// if on left, then place it just after the border
	}
	else
	{
		verticalRect->x=viewRect->x+viewRect->w;			// if on right, place just after the view
	}

	verticalRect->y=viewRect->y;
	verticalRect->w=SCROLLBARWIDTH;
	verticalRect->h=viewRect->h;

	horizontalRect->x=viewRect->x;
	horizontalRect->y=viewRect->y+viewRect->h;
	horizontalRect->w=viewRect->w;
	horizontalRect->h=SCROLLBARWIDTH;
}

static void CreateStatusString(EDITORWINDOW *theWindow,char *theString)
// create status string for theWindow
// NOTE: theString must be large enough to hold the status
{
	EDITORBUFFER
		*theBuffer;
	UINT32
		startPosition,						// selection information
		endPosition,
		startLine,
		endLine,
		startLinePosition,
		endLinePosition,
		totalSegments,
		totalSpan;

	theBuffer=theWindow->theBuffer;

	EditorGetSelectionInfo(theBuffer,theBuffer->selectionUniverse,&startPosition,&endPosition,&startLine,&endLine,&startLinePosition,&endLinePosition,&totalSegments,&totalSpan);

	sprintf(theString,"%s%sLines:%d Chars:%d",
			AtUndoCleanPoint(theBuffer)?"  ":"* ",
			theWindow->theBuffer->theTask?"<Task running> ":"",
			(int)theBuffer->textUniverse->totalLines,
			(int)theBuffer->textUniverse->totalBytes
			);

	if(!totalSegments)
	{
		sprintf(&theString[strlen(theString)]," Cursor(%d:%d %d)",(int)startLine+1,(int)startLinePosition,(int)startPosition);
	}
	else
	{
		sprintf(&theString[strlen(theString)]," Selections: Number:%d TotalChars:%d Start(%d:%d %d) End(%d:%d %d)",
				(int)totalSegments,
				(int)totalSpan,
				(int)startLine+1,(int)startLinePosition,(int)startPosition,
				(int)endLine+1,(int)endLinePosition,(int)endPosition
				);
	}
}

static void DrawStatusBar(EDITORWINDOW *theWindow)
// draw the status bar for theWindow
{
	int
		linePosition;
	XRectangle
		regionBox;
	WINDOWLISTELEMENT
		*theWindowElement;
	GC
		graphicsContext;
	Region
		invalidRegion;
	EDITORRECT
		contentRect,
		viewRect,
		verticalRect,
		horizontalRect,
		statusRect;
	GUIFONT
		*statusBarFont;
	int
		stringWidth;
	char
		statusString[256];											// the status string

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;		// get the x window information associated with this document window
	graphicsContext=theWindowElement->graphicsContext;				// get graphics context for this window

	invalidRegion=XCreateRegion();

	GetDocumentWindowRects(theWindow,&contentRect,&viewRect,&verticalRect,&horizontalRect,&statusRect);

	regionBox.x=statusRect.x;
	regionBox.y=statusRect.y;
	regionBox.width=statusRect.w;
	regionBox.height=statusRect.h;
	XUnionRectWithRegion(&regionBox,invalidRegion,invalidRegion);		// create a region which is the rectangle of the entire status bar
	XIntersectRegion(invalidRegion,theWindowElement->invalidRegion,invalidRegion);	// intersect with what is invalid in this window

	if(!XEmptyRegion(invalidRegion))								// see if it has an invalid area
	{
		XSetRegion(xDisplay,graphicsContext,invalidRegion);			// set clipping to what's invalid
		XSetFillStyle(xDisplay,graphicsContext,FillSolid);
		XClipBox(invalidRegion,&regionBox);							// get bounds of invalid region

		CreateStatusString(theWindow,&statusString[0]);
		statusBarFont=((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->statusBarFont;
		XSetFont(xDisplay,graphicsContext,statusBarFont->theXFont->fid);		// point to the font

		stringWidth=XTextWidth(statusBarFont->theXFont,&statusString[0],strlen(statusString));

		XSetForeground(xDisplay,graphicsContext,statusBackgroundColor->theXPixel);
		XFillRectangle(xDisplay,theWindowElement->xWindow,graphicsContext,regionBox.x,regionBox.y,regionBox.width,regionBox.height);	// fill with white
		XSetForeground(xDisplay,graphicsContext,statusForegroundColor->theXPixel);
		XDrawString(xDisplay,theWindowElement->xWindow,graphicsContext,statusRect.x+VIEWLEFTMARGIN,statusRect.y+statusBarFont->ascent,&statusString[0],strlen(statusString));

		linePosition=statusRect.y+statusRect.h-STATUSBARLINEHEIGHT;
		XDrawLine(xDisplay,theWindowElement->xWindow,graphicsContext,statusRect.x,linePosition,statusRect.w,linePosition);
		linePosition+=STATUSBARLINEHEIGHT-1;
		XSetForeground(xDisplay,graphicsContext,white->theXPixel);
		XDrawLine(xDisplay,theWindowElement->xWindow,graphicsContext,statusRect.x,linePosition,statusRect.w,linePosition);
		XSetClipMask(xDisplay,graphicsContext,None);				// get rid of clip mask
	}
	XDestroyRegion(invalidRegion);									// get rid of invalid region for the view
}

static void DrawGrowArea(EDITORWINDOW *theWindow)
// draw the grow box area of the window
{
	WINDOWLISTELEMENT
		*theWindowElement;
	GC
		graphicsContext;
	XRectangle
		regionBox;
	EDITORRECT
		contentRect,
		viewRect,
		verticalRect,
		horizontalRect,
		statusRect;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;			// get the x window information associated with this document window
	graphicsContext=theWindowElement->graphicsContext;					// get graphics context for this window

	GetDocumentWindowRects(theWindow,&contentRect,&viewRect,&verticalRect,&horizontalRect,&statusRect);

	regionBox.x=verticalRect.x;
	regionBox.y=horizontalRect.y;
	regionBox.width=verticalRect.w;
	regionBox.height=horizontalRect.h;

	XSetFillStyle(xDisplay,graphicsContext,FillSolid);
	XSetForeground(xDisplay,graphicsContext,gray1->theXPixel);
	XFillRectangle(xDisplay,theWindowElement->xWindow,graphicsContext,regionBox.x,regionBox.y,regionBox.width,regionBox.height);	// fill with white
}

void AdjustDocumentWindowStatus(EDITORWINDOW *theWindow)
// adjust the status of the document window
// this includes updating the status bar, icon, and anything else status related
{
	EDITORRECT
		contentRect,
		viewRect,
		verticalRect,
		horizontalRect,
		statusRect;
	XWMHints
		theHints;

	GetDocumentWindowRects(theWindow,&contentRect,&viewRect,&verticalRect,&horizontalRect,&statusRect);
	statusRect.h-=STATUSBARLINEHEIGHT;								// just invalidate the status area
	InvalidateWindowRect(theWindow,&statusRect);

	// adjust the pixmap for the icon based on the window's "clean" status
	theHints.flags=IconPixmapHint;
	if(AtUndoCleanPoint(theWindow->theBuffer))
	{
		theHints.icon_pixmap=documentIconPixmap;				// point at document icon
	}
	else
	{
		theHints.icon_pixmap=documentModifiedIconPixmap;
	}
	if(theHints.icon_pixmap!=((WINDOWLISTELEMENT *)theWindow->userData)->currentIcon)
	{
		((WINDOWLISTELEMENT *)theWindow->userData)->currentIcon=theHints.icon_pixmap;	// remember this so we can tell if it changed
		XSetWMHints(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,&theHints);
	}
}

static void AdjustDocumentWindowViewStatus(EDITORVIEW *theView)
// adjust the status bar of the document window with the given view
{
	AdjustDocumentWindowStatus(theView->parentWindow);
}

static void AdjustDocumentWindowScrollThumbs(EDITORVIEW *theView)
// look at the data of the passed view, and
// adjust the thumbs of its scroll bars so that
// they are correct
{
	UINT32
		scrollBarLeftPixel;

	AdjustScrollBar(((GUIDOCUMENTWINDOWINFO *)theView->parentWindow->windowInfo)->verticalScrollBar,theView->parentBuffer->textUniverse->totalLines,((GUIVIEWINFO *)theView->viewInfo)->topLine);
	// NOTE: since views allow for having a left pixel <0, but the scroll bars do not, peg the bar at 0
	if(((GUIVIEWINFO *)theView->viewInfo)->leftPixel>=0)
	{
		scrollBarLeftPixel=(UINT32)((GUIVIEWINFO *)theView->viewInfo)->leftPixel;
	}
	else
	{
		scrollBarLeftPixel=0;
	}
	// since the view is allowed to scroll beyond the length of the horizontal scroll bar, pin the bar at the limit
	if(((GUIVIEWINFO *)theView->viewInfo)->leftPixel>HSCROLLMAXWIDTH)
	{
		scrollBarLeftPixel=HSCROLLMAXWIDTH;
	}
	AdjustScrollBar(((GUIDOCUMENTWINDOWINFO *)theView->parentWindow->windowInfo)->horizontalScrollBar,HSCROLLMAXWIDTH,scrollBarLeftPixel);
}

static void AdjustDocumentWindowScrollThumbsAndStatus(EDITORVIEW *theView)
// adjust the parts of the window that change when the view changes
{
	AdjustDocumentWindowScrollThumbs(theView);
	AdjustDocumentWindowViewStatus(theView);
}

static void DrawDocumentWindow(EDITORWINDOW *theWindow)
// a document window needs to be updated, this
// will draw it
// NOTE: the window's invalidRegion is set to the area which is in need of update
{
	DrawStatusBar(theWindow);
	DrawScrollBar(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->verticalScrollBar);		// draw the scroll bars
	DrawScrollBar(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->horizontalScrollBar);
	DrawGrowArea(theWindow);
	DrawView(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->theView);	// draw this view
}

void UpdateDocumentWindow(EDITORWINDOW *theWindow)
// make sure the window is up to date
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;				// get the x window information associated with this document window
	if(!XEmptyRegion(theWindowElement->invalidRegion))						// see if it has an invalid area
	{
		DrawDocumentWindow(theWindow);
		XDestroyRegion(theWindowElement->invalidRegion);					// clear out the invalid region now that the window has been drawn
		theWindowElement->invalidRegion=XCreateRegion();
	}
}

void AdjustDocumentWindowForNewSize(EDITORWINDOW *theWindow)
// if the window has changed size, then the
// controls will be needing update, so adjust them for the new
// size
{
	EDITORRECT
		contentRect,
		viewRect,
		verticalRect,
		horizontalRect,
		statusRect;

	GetDocumentWindowRects(theWindow,&contentRect,&viewRect,&verticalRect,&horizontalRect,&statusRect);

	SetViewBounds(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->theView,&viewRect);		// change the bounds of the view

	RepositionScrollBar(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->verticalScrollBar,verticalRect.x,verticalRect.y,verticalRect.h,false);
	RepositionScrollBar(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->horizontalScrollBar,horizontalRect.x,horizontalRect.y,horizontalRect.w,true);
}

void DocumentWindowButtonPress(EDITORWINDOW *theWindow,XEvent *theEvent)
// handle button press events for document windows (see what the user is clicking
// on, and attempt to deal with it)
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	if(!TrackView(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->theView,theEvent))
	{
		if(!TrackScrollBar(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->verticalScrollBar,theEvent))
		{
			TrackScrollBar(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->horizontalScrollBar,theEvent);
		}
	}
}

void DocumentPeriodicProc(EDITORWINDOW *theWindow)
// this is called roughly twice a second to flash cursors, or do whatever in the document
// when the document has focus
{
	ViewCursorTask(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->theView);			// flash the cursor in the view
}

void DocumentWindowEvent(EDITORWINDOW *theWindow,XEvent *theEvent)
// handle an event arriving for a document window
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	switch(theEvent->type)
	{
		case ButtonPress:
			XSetInputFocus(xDisplay,theWindowElement->xWindow,RevertToNone,CurrentTime);	// give it the focus
			DocumentWindowButtonPress(theWindow,theEvent);										// go handle the button press
			break;
		case KeyPress:
			HandleViewKeyEvent(((GUIDOCUMENTWINDOWINFO *)theWindowElement->theEditorWindow->windowInfo)->theView,theEvent);
			break;
		case ClientMessage:
			if(theEvent->xclient.send_event)
			{
				if(theEvent->xclient.data.l[0]==(int)deleteWindowAtom)
				{
					HandleShellCommand("close",1,&(theWindow->theBuffer->contentName));		// tell shell to close this
				}
			}
			break;
		default:
			break;
	}
}

static void ScrollEditViewEvent(EDITORVIEW *theView,UINT32 eventCode,bool repeating)
// send a scroll event to the view
{
	VIEWEVENT
		theRecord;
	EDITORKEY
		theKeyData;

	if(!repeating)
	{
		theRecord.eventType=VET_KEYDOWN;
	}
	else
	{
		theRecord.eventType=VET_KEYREPEAT;
	}
	theRecord.theView=theView;
	theKeyData.isVirtual=true;
	theKeyData.keyCode=eventCode;		// this is the key
	theKeyData.modifiers=0;				// no modifiers
	theRecord.eventData=&theKeyData;
	HandleViewEvent(&theRecord);
}

static void ScrollEditViewUpEvent(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be scrolled up
{
	ScrollEditViewEvent((EDITORVIEW *)parameters,VVK_SCROLLUP,repeating);
}

static void ScrollEditViewDownEvent(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be scrolled down
{
	ScrollEditViewEvent((EDITORVIEW *)parameters,VVK_SCROLLDOWN,repeating);
}

static void ScrollEditViewLeftEvent(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be scrolled left
{
	ScrollEditViewEvent((EDITORVIEW *)parameters,VVK_SCROLLLEFT,repeating);
}

static void ScrollEditViewRightEvent(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be scrolled right
{
	ScrollEditViewEvent((EDITORVIEW *)parameters,VVK_SCROLLRIGHT,repeating);
}

static void ScrollEditViewPageUpEvent(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be paged up
{
	ScrollEditViewEvent((EDITORVIEW *)parameters,VVK_DOCUMENTPAGEUP,repeating);
}

static void ScrollEditViewPageDownEvent(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be paged down
{
	ScrollEditViewEvent((EDITORVIEW *)parameters,VVK_DOCUMENTPAGEDOWN,repeating);
}

static void ScrollEditViewPageLeftEvent(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be paged left
{
	ScrollEditViewEvent((EDITORVIEW *)parameters,VVK_DOCUMENTPAGELEFT,repeating);
}

static void ScrollEditViewPageRightEvent(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be paged right
{
	ScrollEditViewEvent((EDITORVIEW *)parameters,VVK_DOCUMENTPAGERIGHT,repeating);
}

static void ScrollEditViewThumbVerticalEvent(SCROLLBAR *theScrollBar,UINT32 newValue,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be positioned vertically
{
	VIEWEVENT
		theRecord;
	VIEWPOSEVENTDATA
		thePosData;

	theRecord.eventType=VET_POSITIONVERTICAL;
	theRecord.theView=(EDITORVIEW *)parameters;
	thePosData.modifiers=0;
	thePosData.position=newValue;
	theRecord.eventData=&thePosData;
	HandleViewEvent(&theRecord);
}

static void ScrollEditViewThumbHorizontalEvent(SCROLLBAR *theScrollBar,UINT32 newValue,void *parameters)
// tell the editor that the view in the window with theScrollBar is to be positioned horizontally
{
	VIEWEVENT
		theRecord;
	VIEWPOSEVENTDATA
		thePosData;

	theRecord.eventType=VET_POSITIONHORIZONTAL;
	theRecord.theView=(EDITORVIEW *)parameters;
	thePosData.modifiers=0;
	thePosData.position=newValue;
	theRecord.eventData=&thePosData;
	HandleViewEvent(&theRecord);
}

static void DisposeDocumentWindow(EDITORWINDOW *theWindow)
// close a given window, kill off the view
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;		// get the x window associated with this document window
	DisposeScrollBar(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->horizontalScrollBar);
	DisposeScrollBar(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->verticalScrollBar);
	DisposeEditorView(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->theView);	// get rid of the view, unlink it from everywhere
	XDestroyRegion(theWindowElement->invalidRegion);				// destroy invalid region
	XFreeGC(xDisplay,theWindowElement->graphicsContext);			// get rid of the graphics context for this window
	FocusAwayFrom(theWindow);										// move focus off this window
	XDestroyWindow(xDisplay,theWindowElement->xWindow);				// tell x to make the window go away
	WaitUntilDestroyed(theWindow);
	UnSetDocumentBackgroundColor(theWindow);						// get rid of colors allocated for it
	UnSetDocumentForegroundColor(theWindow);
	UnlinkWindowElement(theWindowElement);							// unlink it from our list
	MDisposePtr(theWindowElement);
	FreeFont(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->statusBarFont);
	MDisposePtr(theWindow->windowInfo);
	MDisposePtr(theWindow);
}

static EDITORWINDOW *CreateDocumentWindow(EDITORBUFFER *theBuffer,EDITORRECT *theRect,char *theTitle,char *fontName,UINT32 tabSize,EDITORCOLOR foreground,EDITORCOLOR background)
// open a new document window with one EDITORVIEW onto theBuffer
// set the view(s) to use the given defaults
{
	EDITORRECT
		contentRect,
		viewRect,
		verticalRect,
		horizontalRect,
		statusRect;
	EDITORWINDOW
		*theWindow;
	GUIDOCUMENTWINDOWINFO
		*theDocumentWindowInfo;
	WINDOWLISTELEMENT
		*theWindowElement;
	SCROLLBARDESCRIPTOR
		scrollDescriptor;
	EDITORVIEWDESCRIPTOR
		viewDescriptor;
	XSetWindowAttributes
		theAttributes;
	UINT32
		valueMask;
	Atom
		protocols[2];																	// number of protocols
	XWMHints
		theHints;																		// hints to tell window manager how to treat these windows
	XSizeHints
		theSizeHints;																	// window size info (used to REALLY convince X that I know where I want my own window)

	if((theWindow=(EDITORWINDOW *)MNewPtr(sizeof(EDITORWINDOW))))						// create editor window
	{
		theWindow->theBuffer=theBuffer;

		if((theDocumentWindowInfo=(GUIDOCUMENTWINDOWINFO *)MNewPtr(sizeof(GUIDOCUMENTWINDOWINFO))))	// make structure to contain document window info
		{
			if((theDocumentWindowInfo->statusBarFont=LoadFont(STATUSBARFONT)))			// load in the font
			{
				if((theWindowElement=(WINDOWLISTELEMENT *)MNewPtr(sizeof(WINDOWLISTELEMENT))))	// create a window list element
				{
					LinkWindowElement(theWindowElement);								// link it to the global window list that we maintain
					theWindowElement->theEditorWindow=theWindow;						// point to our high level concept of window
					theWindow->windowType=EWT_DOCUMENT;									// set the editor window type to "document"
					theWindow->userData=theWindowElement;								// point back to the window element
					theWindow->windowInfo=theDocumentWindowInfo;						// point to the edit info

					if(SetDocumentForegroundColor(theWindow,foreground,false))			// set the colors for this window
					{
						if(SetDocumentBackgroundColor(theWindow,background,false))
						{
							theAttributes.background_pixmap=None;
							theAttributes.background_pixel=theDocumentWindowInfo->backgroundColor->theXPixel;
							theAttributes.border_pixmap=None;
							theAttributes.border_pixel=theDocumentWindowInfo->foregroundColor->theXPixel;
							theAttributes.colormap=xColormap;
							valueMask=CWBackPixmap|CWBackPixel|CWBorderPixmap|CWBorderPixel|CWColormap;

							// Due to X's nasty error handling, we assume this create does not fail!!!

							theWindowElement->realTopWindow=theWindowElement->xWindow=XCreateWindow(xDisplay,RootWindow(xDisplay,xScreenNum),theRect->x,theRect->y,theRect->w,theRect->h,XWINDOWBORDERWIDTH,CopyFromParent,InputOutput,DefaultVisual(xDisplay,xScreenNum),valueMask,&theAttributes);

							theSizeHints.flags=USSize|USPosition|PMinSize;
							theSizeHints.x=theRect->x;
							theSizeHints.y=theRect->y;
							theSizeHints.width=theRect->w;
							theSizeHints.height=theRect->h;
							theSizeHints.min_width=1;
							theSizeHints.min_height=1;
							XSetWMNormalHints(xDisplay,theWindowElement->xWindow,&theSizeHints);

							XStoreName(xDisplay,theWindowElement->xWindow,theTitle);
							XSetIconName(xDisplay,theWindowElement->xWindow,theTitle);
							XSelectInput(xDisplay,theWindowElement->xWindow,ExposureMask|ButtonPressMask|ButtonReleaseMask|FocusChangeMask|KeyPressMask|StructureNotifyMask);	// want to see these events

							protocols[0]=takeFocusAtom;
							protocols[1]=deleteWindowAtom;
							XSetWMProtocols(xDisplay,theWindowElement->xWindow,&(protocols[0]),2);	// tell X we would like to participate in these protocols

							theHints.flags=InputHint|StateHint|WindowGroupHint;
							theHints.input=True;
							theHints.initial_state=NormalState;
							theHints.window_group=((WINDOWLISTELEMENT *)rootMenuWindow->userData)->xWindow;	// all documents belong to the group headed by the main menu window
							XSetWMHints(xDisplay,theWindowElement->xWindow,&theHints);

							theWindowElement->graphicsContext=XCreateGC(xDisplay,theWindowElement->xWindow,0,NULL);	// create context in which to draw the text (0 for mask) use default

							theWindowElement->invalidRegion=XCreateRegion();						// create new empty invalid region

							theWindowElement->currentIcon=(Pixmap)NULL;								// no icon associated with this yet

							theDocumentWindowInfo->verticalScrollBarOnLeft=verticalScrollBarOnLeft;	// copy global preference
							GetDocumentWindowRects(theWindow,&contentRect,&viewRect,&verticalRect,&horizontalRect,&statusRect);	// get sizes of areas within the window

							viewDescriptor.theBuffer=theBuffer;
							viewDescriptor.theRect=viewRect;										// place where the view is going
							viewDescriptor.topLine=0;
							viewDescriptor.leftPixel=0;
							viewDescriptor.tabSize=tabSize;

							viewDescriptor.backgroundColor=background;
							viewDescriptor.foregroundColor=foreground;

							viewDescriptor.fontName=fontName;
							viewDescriptor.active=true;												// create the view active
							viewDescriptor.viewTextChangedVector=AdjustDocumentWindowScrollThumbsAndStatus;		// fix the thumbs, and status when the view text changes
							viewDescriptor.viewPositionChangedVector=AdjustDocumentWindowScrollThumbs;			// fix the thumbs when the view position changes
							viewDescriptor.viewSelectionChangedVector=AdjustDocumentWindowViewStatus;			// fix the status when the selection changes
							if((theDocumentWindowInfo->theView=CreateEditorView(theWindow,&viewDescriptor)))	// create a view for this area, set up its font stuff
							{
								scrollDescriptor.x=verticalRect.x;									// set up description of the scroll bar
								scrollDescriptor.y=verticalRect.y;
								scrollDescriptor.length=verticalRect.h;
								scrollDescriptor.horizontal=false;
								scrollDescriptor.thumbMax=0;
								scrollDescriptor.thumbPosition=0;
								scrollDescriptor.stepLowerProc=ScrollEditViewUpEvent;
								scrollDescriptor.stepHigherProc=ScrollEditViewDownEvent;
								scrollDescriptor.pageLowerProc=ScrollEditViewPageUpEvent;
								scrollDescriptor.pageHigherProc=ScrollEditViewPageDownEvent;
								scrollDescriptor.thumbProc=ScrollEditViewThumbVerticalEvent;
								scrollDescriptor.procParameters=theDocumentWindowInfo->theView;

								if((theDocumentWindowInfo->verticalScrollBar=CreateScrollBar(theWindow,&scrollDescriptor)))
								{
									scrollDescriptor.x=horizontalRect.x;
									scrollDescriptor.y=horizontalRect.y;
									scrollDescriptor.length=horizontalRect.w;
									scrollDescriptor.horizontal=true;
									scrollDescriptor.thumbMax=0;
									scrollDescriptor.thumbPosition=0;
									scrollDescriptor.stepLowerProc=ScrollEditViewLeftEvent;
									scrollDescriptor.stepHigherProc=ScrollEditViewRightEvent;
									scrollDescriptor.pageLowerProc=ScrollEditViewPageLeftEvent;
									scrollDescriptor.pageHigherProc=ScrollEditViewPageRightEvent;
									scrollDescriptor.thumbProc=ScrollEditViewThumbHorizontalEvent;
									scrollDescriptor.procParameters=theDocumentWindowInfo->theView;

									if((theDocumentWindowInfo->horizontalScrollBar=CreateScrollBar(theWindow,&scrollDescriptor)))
									{
										AdjustDocumentWindowScrollThumbsAndStatus(theDocumentWindowInfo->theView);		// force the thumbs and status to be up to date
										XMapRaised(xDisplay,theWindowElement->xWindow);						// map window to the display
										WaitUntilMapped(theWindow);
										return(theWindow);
									}
									DisposeScrollBar(theDocumentWindowInfo->verticalScrollBar);
								}
								DisposeEditorView(theDocumentWindowInfo->theView);
							}
							XDestroyRegion(theWindowElement->invalidRegion);					// destroy invalid region
							XFreeGC(xDisplay,theWindowElement->graphicsContext);				// if something went wrong, get rid of graphics context
							XDestroyWindow(xDisplay,theWindowElement->xWindow);					// get rid of the window
							WaitUntilDestroyed(theWindow);

							UnSetDocumentBackgroundColor(theWindow);
						}
						UnSetDocumentForegroundColor(theWindow);
					}
					UnlinkWindowElement(theWindowElement);
					MDisposePtr(theWindowElement);
				}
				FreeFont(theDocumentWindowInfo->statusBarFont);
			}
			MDisposePtr(theDocumentWindowInfo);
		}
		MDisposePtr(theWindow);
	}
	ReportMessage("Failed to create window\n");
	return((EDITORWINDOW *)NULL);
}

// routines callable by the editor

void MinimizeDocumentWindow(EDITORWINDOW *theWindow)
// minimize the passed window
{
	MinimizeEditorWindow(theWindow);
}

void UnminimizeDocumentWindow(EDITORWINDOW *theWindow)
// unminimize the passed window
{
	UnminimizeEditorWindow(theWindow);
}

void SetTopDocumentWindow(EDITORWINDOW *theWindow)
// set the passed document window to the top of the display
{
	SetTopEditorWindow(theWindow);
}

EDITORWINDOW *GetActiveDocumentWindow()
// return the document window that has the user's attention, or NULL if there is none
// depending on the system, this may or may not return the top-most document window
// (For instance under Motif, the active document window may be the one with the
// cursor over it)
{
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	int
		focusRevert;

	XGetInputFocus(xDisplay,&xWindow,&focusRevert);			// find out who has the focus
	if((theWindowElement=LocateWindowElement(xWindow)))		// see if it is one of our windows
	{
		if(theWindowElement->theEditorWindow->windowType==EWT_DOCUMENT)
		{
			return(theWindowElement->theEditorWindow);		// if the window with focus is a document window, then return it
		}
	}
	return(GetTopEditorWindowType(EWT_DOCUMENT));			// otherwise, the active one is the top one
}

EDITORWINDOW *GetTopDocumentWindow()
// return the top-most document window, or NULL if there is none
// this indirectly uses the realTopWindow fields of the window elements
{
	return(GetTopEditorWindowType(EWT_DOCUMENT));
}

bool GetSortedDocumentWindowList(UINT32 *numElements,EDITORWINDOW ***theList)
// return a list of all document windows -- sorted top to bottom
// if there is a problem, SetError, and return false
// otherwise return true. If true is returned, theList must be
// MDisposePtr'ed by the caller (even if numElements is 0)
{
	return(GetSortedEditorWindowTypeList(EWT_DOCUMENT,numElements,theList));
}

EDITORVIEW *GetDocumentWindowCurrentView(EDITORWINDOW *theWindow)
// passed a document window, return the current view of the window
{
	return(((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->theView);			// currently, document windows contain one view only
}

bool GetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle,UINT32 stringBytes)
// copy theWindow's title into theTitle
// if theWindow's title is longer than stringBytes, return false
{
	char
		*theName;

	if(XFetchName(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,&theName))
	{
		strncpy(theTitle,theName,stringBytes);
		XFree(theName);
	}
	else
	{
		strncpy(theTitle,"no name",stringBytes);
	}
	if(stringBytes)
	{
		theTitle[stringBytes-1]='\0';
	}
	return(true);
}

bool SetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle)
// replace theTitle of theWindow
// if there is a problem, return false
{
	XStoreName(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,theTitle);
	XSetIconName(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,theTitle);
	return(true);
}

bool GetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *foregroundColor)
// get the foreground color in use by theWindow
// if there is a problem, SetError, and return false
{
	GUIDOCUMENTWINDOWINFO
		*theWindowInfo;
	XColor
		theColor;

	theWindowInfo=(GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo;
	theColor.pixel=theWindowInfo->foregroundColor->theXPixel;				// get pixel of foreground for this window
	XQueryColor(xDisplay,xColormap,&theColor);
	XColorToEditorColor(&theColor,foregroundColor);
	return(true);
}

bool SetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR foregroundColor)
// set the foreground color to be used in theWindow
// if there is a problem, leave the color unchanged, set the error
// and return false
{
	if(SetDocumentForegroundColor(theWindow,foregroundColor,true))
	{
		XSetWindowBorder(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->foregroundColor->theXPixel);
		return(true);
	}
	return(false);
}

bool GetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *backgroundColor)
// get the background color in use by theWindow
// if there is a problem, SetError, and return false
{
	GUIDOCUMENTWINDOWINFO
		*theWindowInfo;
	XColor
		theColor;

	theWindowInfo=(GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo;
	theColor.pixel=theWindowInfo->backgroundColor->theXPixel;				// get pixel of background for this window
	XQueryColor(xDisplay,xColormap,&theColor);
	XColorToEditorColor(&theColor,backgroundColor);
	return(true);
}

bool SetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR backgroundColor)
// set the background color to be used in theWindow
// if there is a problem, leave the color unchanged, set the error
// and return false
{
	EDITORRECT
		contentRect,
		viewRect,
		verticalRect,
		horizontalRect,
		statusRect;

	if(SetDocumentBackgroundColor(theWindow,backgroundColor,true))
	{
		XSetWindowBackground(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,((GUIDOCUMENTWINDOWINFO *)theWindow->windowInfo)->backgroundColor->theXPixel);
		XClearWindow(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow);
		GetDocumentWindowRects(theWindow,&contentRect,&viewRect,&verticalRect,&horizontalRect,&statusRect);
		InvalidateWindowRect(theWindow,&contentRect);			// invalidate the whole thing, because we cleared it all
		return(true);
	}
	return(false);
}

bool GetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect)
// get the rectangle of theWindow
// if there is a problem, SetError, and return false
// NOTE: once again, X windows makes this MUCH more of a pain than it needs
// to be. We have to get the attributes of our REAL top window for the x, and y
// and of our actual window for the width, and height... sheesh....
{
	XWindowAttributes
		theAttributes;

	XGetWindowAttributes(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->realTopWindow,&theAttributes);
	theRect->x=theAttributes.x;
	theRect->y=theAttributes.y;

	XGetWindowAttributes(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,&theAttributes);
	theRect->w=theAttributes.width;
	theRect->h=theAttributes.height;

	return(true);
}

bool SetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect)
// set theWindow to theRect
// if there is a problem, leave theRect unchanged, set the error
// and return false
{
	XWindowAttributes
		theAttributes;

	XMoveResizeWindow(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,theRect->x,theRect->y,theRect->w,theRect->h);
	
	// XMapWindow(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->realTopWindow);
	// XSync(xDisplay, false);
	XMapRaised(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->realTopWindow);						// map window to the display
	// WaitUntilMapped(theWindow);
	XGetWindowAttributes(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->realTopWindow,&theAttributes);
	
	int
		deltaX = theRect->x - theAttributes.x,
		deltaY = theRect->y - theAttributes.y;

	XMoveWindow(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,theRect->x - deltaX,theRect->y - deltaY);
	
	return(true);
}

EDITORWINDOW *OpenDocumentWindow(EDITORBUFFER *theBuffer,EDITORRECT *theRect,char *theTitle,char *fontName,UINT32 tabSize,EDITORCOLOR foreground,EDITORCOLOR background)
// open a document window onto theBuffer
// if there is a problem, undo anything that was done,
// set the error, then return NULL
{
	return(CreateDocumentWindow(theBuffer,theRect,theTitle,fontName,tabSize,foreground,background));
}

void CloseDocumentWindow(EDITORWINDOW *theWindow)
// close an open document window
{
	DisposeDocumentWindow(theWindow);
}
