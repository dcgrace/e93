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

#include "includes.h"

static bool lastMDIShouldZoom = false;

bool MDIShouldZoom()
// an accessor to the lastMDIZoomState bool
{
	return lastMDIShouldZoom;
}

void InitMDIShouldZoom(bool zoomed)
// called once at startup with the value from the Windows registry
{
	lastMDIShouldZoom = zoomed;
}

void SetMDIShouldZoom(bool zoomed)
// if the MDI zoom state has changed, write the new value to the Windows registry
{
	if (zoomed != lastMDIShouldZoom)
	{
		lastMDIShouldZoom = zoomed;
		WriteMDIShouldZoomRegistrySetting(zoomed);
	}
}

void LinkWindowElement(WINDOWLISTELEMENT *theElement)
/* passed a WINDOWLISTELEMENT, link it to the start of the global list
 */
{
	if(windowListHead)									/* point previous back to new */
	{
		windowListHead->previousElement=theElement;
	}
	theElement->nextElement=windowListHead;				/* point to the old head of the list */
	theElement->previousElement=NULL;					/* no previous element */
	windowListHead=theElement;							/* point the head at the new element */
}

void UnlinkWindowElement(WINDOWLISTELEMENT *theElement)
/* passed a WINDOWLISTELEMENT, unlink it from the global list
 */
{
	if(theElement->nextElement)
	{
		theElement->nextElement->previousElement=theElement->previousElement;
	}
	if(theElement->previousElement)
	{
		theElement->previousElement->nextElement=theElement->nextElement;
	}
	else
	{
		windowListHead=theElement->nextElement;			/* this one was at the head, so update */
	}
}

static void UnSetDocumentForegroundColor(EDITORWINDOW *theWindow)
// free the foreground color currently in use by theWindow (which must be a document window)
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			/* point at the information record for this view */
	if(theViewInfo->foregroundColor)
	{
		FreeColor(theViewInfo->foregroundColor);
		theViewInfo->foregroundColor=NULL;
		if(theViewInfo->xorBrush)
		{
			DeleteObject(theViewInfo->xorBrush);
			theViewInfo->xorBrush=0;
		}
	}
}

static bool SetDocumentForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR foreground,bool removeOld)
// set the foreground color used to draw theWindow
{
	GUICOLOR
		*newColor;
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			/* point at the information record for this view */
	if((newColor=AllocColor(foreground)))
	{
		if(removeOld)
		{
			UnSetDocumentForegroundColor(theWindow);
		}
		theViewInfo->foregroundColor=newColor;
		if(theViewInfo->backgroundColor)
		{
			theViewInfo->xorBrush=CreateXORBrush(theViewInfo->backgroundColor->colorIndex,theViewInfo->foregroundColor->colorIndex);
		}
		return(true);
	}
	return(false);
}

static void UnSetDocumentBackgroundColor(EDITORWINDOW *theWindow)
// free the background color currently in use by theWindow (which must be a document window)
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			/* point at the information record for this view */
	if(theViewInfo->backgroundColor)
	{
		FreeColor(theViewInfo->backgroundColor);
		theViewInfo->backgroundColor=NULL;
		if(theViewInfo->xorBrush)
		{
			DeleteObject(theViewInfo->xorBrush);
			theViewInfo->xorBrush=0;
		}
	}
}

static bool SetDocumentBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR background,bool removeOld)
// set the background color used to draw theWindow
{
	GUICOLOR
		*newColor;
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			/* point at the information record for this view */
	if((newColor=AllocColor(background)))
	{
		if(removeOld)
		{
			UnSetDocumentBackgroundColor(theWindow);
		}
		theViewInfo->backgroundColor=newColor;
		if(theViewInfo->foregroundColor)
		{
			theViewInfo->xorBrush=CreateXORBrush(theViewInfo->backgroundColor->colorIndex,theViewInfo->foregroundColor->colorIndex);
		}
		return(true);
	}
	return(false);
}

void ReAdjustDocumentColors(EDITORWINDOW *theWindow)
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			/* point at the information record for this view */
	if(theViewInfo->xorBrush)
	{
		DeleteObject(theViewInfo->xorBrush);
	}
	if(theViewInfo->foregroundColor && theViewInfo->backgroundColor)
	{
		theViewInfo->xorBrush=CreateXORBrush(theViewInfo->backgroundColor->colorIndex,theViewInfo->foregroundColor->colorIndex);
	}
}

void AdjustDocumentWindowStatus(EDITORWINDOW *theWindow)
/*	The x or y position of the cursor has changed, or some other status
	update it.
 */
{
	HDC
		hdc;
	GUIVIEWINFO
		*theViewInfo;
	HWND
		hwnd;
	WINDOWLISTELEMENT
		*theWindowElement;
	EDITORVIEW
		*theView;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			/* point at the information record for this view */
	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
	hwnd=theWindowElement->hwnd;
	if(theViewInfo->viewActive)
	{
		hdc=GetDC(hwnd);
		DrawWindowStatusBar(hwnd,hdc);
		ReleaseDC(hwnd,hdc);
	}
	else
	{
		InvalidateWindowStatusBar(hwnd);
	}
}

void AdjustDocumentWindowViewStatus(EDITORVIEW *theView)
/* adjust the status bar of the document window with the given view
 */
{
	AdjustDocumentWindowStatus(theView->parentWindow);
}

static void AdjustDocumentWindowScrollThumbsAndStatus(EDITORVIEW *theView)
/* adjust the parts of the window that change when the view changes
 */
{
	AdjustDocumentWindowScrollThumbs(theView);
	AdjustDocumentWindowViewStatus(theView);
}

void AdjustDocumentWindowScrollThumbs(EDITORVIEW *theView)
/* look at the data of the passed view, and
 * adjust the thumbs of its scroll bars so that
 * they are correct
 */
{
	GUIVIEWINFO
		*theViewInfo;
	HWND
		hwnd;
	WINDOWLISTELEMENT
		*theWindowElement;
	SCROLLINFO
		scrollInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				/* point at the information record for this view */
	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
	hwnd=theWindowElement->hwnd;

	scrollInfo.cbSize=sizeof(SCROLLINFO);
	scrollInfo.fMask=SIF_PAGE|SIF_RANGE|SIF_POS;
	scrollInfo.nMin=0;
	scrollInfo.nMax=theView->parentBuffer->textUniverse->totalLines+theViewInfo->numLines-1;		/* set vertical range to number of lines */
	scrollInfo.nPage=theViewInfo->numLines;
	scrollInfo.nPos=theViewInfo->topLine;						/* set vertical thumb position to current top line */
	SetScrollInfo(hwnd,SB_VERT,&scrollInfo,TRUE);

	scrollInfo.cbSize=sizeof(SCROLLINFO);
	scrollInfo.fMask=SIF_PAGE|SIF_RANGE|SIF_POS;
	scrollInfo.nMin=0;
	scrollInfo.nMax=EDITMAXWIDTH;								/* set horz to EDITMAXWIDTH */
	scrollInfo.nPage=theViewInfo->bounds.right-theViewInfo->bounds.left;
	scrollInfo.nPos=theViewInfo->leftPixel;						/* set horz thumb to current leftPixel */
	SetScrollInfo(hwnd,SB_HORZ,&scrollInfo,TRUE);
}

void ScrollEditViewEvent(EDITORVIEW *theView,UINT32 eventCode,bool repeating)
/* send a scroll event to the view
 */
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
	theKeyData.keyCode=eventCode;		/* this is the key */
	theKeyData.modifiers=0;
	theRecord.eventData=&theKeyData;
	HandleViewEvent(&theRecord);
}

void ScrollEditViewThumbVerticalEvent(EDITORVIEW *theView,UINT32 newValue)
/* tell the editor that the view in the window with theScrollBar is to be positioned vertically
 */
{
	VIEWEVENT
		theRecord;
	VIEWPOSEVENTDATA
		thePosData;

	theRecord.eventType=VET_POSITIONVERTICAL;
	theRecord.theView=theView;
	thePosData.modifiers=0;
	thePosData.position=newValue;
	theRecord.eventData=&thePosData;
	HandleViewEvent(&theRecord);
}

void ScrollEditViewThumbHorizontalEvent(EDITORVIEW *theView,UINT32 newValue)
/* tell the editor that the view in the window with theScrollBar is to be positioned horizontally
 */
{
	VIEWEVENT
		theRecord;
	VIEWPOSEVENTDATA
		thePosData;

	theRecord.eventType=VET_POSITIONHORIZONTAL;
	theRecord.theView=theView;
	thePosData.modifiers=0;
	thePosData.position=newValue;
	theRecord.eventData=&thePosData;
	HandleViewEvent(&theRecord);
}

/*
 *	This routines are all ones that the editor calls directly
 */

void UpdateEditorWindows()
/* redraw all editor windows that have been invalidated by previous expose or
 * graphics expose events. This goes from the top window to the bottom one.
 */
{
	WINDOWLISTELEMENT
		*curWindowElement;

	curWindowElement=windowListHead;
	while(curWindowElement)
	{
		UpdateWindow(curWindowElement->hwnd);
		curWindowElement=curWindowElement->nextElement;
	}
}

// routines callable by the editor

void MinimizeDocumentWindow(EDITORWINDOW *theWindow)
// minimize the passed window
{
	WINDOWLISTELEMENT
		*theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	
	ShowWindow(theWindowElement->hwnd, SW_SHOWMINIMIZED);
	UpdateEditorWindows();
}

void UnminimizeDocumentWindow(EDITORWINDOW *theWindow)
// unminimize the passed window
{
	WINDOWLISTELEMENT
		*theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
		
    SendMessage(clientWindowHwnd, WM_MDIACTIVATE, (WPARAM)theWindowElement->hwnd, 0);		// you must activate the window first or it will not redraw
	ShowWindow(theWindowElement->hwnd, SW_SHOWNORMAL); 
}

void SetTopDocumentWindow(EDITORWINDOW *theWindow)
/* set the passed document window to the top of the display
 */
{
	WINDOWLISTELEMENT
		*theWindowElement;
	
	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	
    SendMessage(clientWindowHwnd, WM_MDIACTIVATE, (WPARAM)theWindowElement->hwnd, 0);	// you must activate the window first or it will not redraw
			
	WINDOWPLACEMENT
		wp;
	
	wp.length=sizeof(WINDOWPLACEMENT);

	GetWindowPlacement(theWindowElement->hwnd,&wp);

	
	if (wp.showCmd == SW_SHOWMINIMIZED)
	{
		ShowWindow(theWindowElement->hwnd, SW_SHOWNORMAL);
	}
}

EDITORWINDOW *GetActiveDocumentWindow()
/* return the document window that has the user's attention, or NULL if there is none
 * depending on the system, this may or may not return the top-most document window
 */
{
    HWND
		hwnd;

	if(hwnd=(HWND)LOWORD(SendMessage(clientWindowHwnd,WM_MDIGETACTIVE,0,0)))
	{
		if(IsWindow(hwnd))
		{
	    	return((EDITORWINDOW *)GetWindowLong(hwnd,EDITORWINDOW_DATA_OFFSET));
	    }
	}
	return(NULL);
}

EDITORWINDOW *GetTopDocumentWindow()
/* return the top-most document window, or NULL if there is none
 */
{
	if(windowListHead)
	{
		return(windowListHead->theEditorWindow);
	}
	return(NULL);
}

EDITORWINDOW *GetNextDocumentWindow(EDITORWINDOW *previousWindow)
/* return the document just under previousWindow, or NULL if there is none
 */
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)previousWindow->userData;
	if(theWindowElement->nextElement)
	{
		return((EDITORWINDOW *)theWindowElement->nextElement->theEditorWindow);
	}
	return(NULL);
}

EDITORVIEW *GetDocumentWindowCurrentView(EDITORWINDOW *theWindow)
/* passed a document window, return the current view of the window
 */
{
	return((EDITORVIEW *)theWindow->windowInfo);
}

bool GetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle,UINT32 stringBytes)
/* copy theWindow's title into theTitle
 * if theWindow's title is longer than stringBytes, return FALSE
 */
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
	if((strlen(theViewInfo->titleString)+1)<=stringBytes)
	{
		strcpy(theTitle,theViewInfo->titleString);
		return(true);
	}
	return(false);
}

bool SetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle)
/* replace theTitle of theWindow
 * if there is a problem, return FALSE
 */
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	char
		*newTitle;
	WINDOWLISTELEMENT
		*theWindowElement;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	if(newTitle=(char *)MNewPtr(strlen(theTitle)+1))
	{
		strcpy(newTitle,theTitle);
		SetWindowText(theWindowElement->hwnd,newTitle);
		MDisposePtr((void *)theViewInfo->titleString);
		theViewInfo->titleString=newTitle;
		return(true);
	}
	return(false);
}

bool GetSortedDocumentWindowList(UINT32 *numElements,EDITORWINDOW ***theList)
/* return a list of all document windows -- sorted top to bottom
 * if there is a problem, SetError, and return FALSE
 * otherwise return TRUE. If TRUE is returned, theList must be
 * MDisposePtr'ed by the caller (even if numElements is 0)
 */
{
	WINDOWLISTELEMENT
		*theElement;
	unsigned int
		child,
		numChildren;
	bool
		fail;

	fail=false;
	*numElements=0;																	/* no elements yet */

	theElement=windowListHead;

	numChildren=0;
	while(theElement)
	{
		theElement=theElement->nextElement;
		numChildren++;
	}
	if((*theList)=(EDITORWINDOW **)MNewPtr(sizeof(EDITORWINDOW *)*numChildren))		/* create a list of pointers large enough to hold the maximum number of windows located */
	{
		theElement=windowListHead;
		for(child=numChildren;child>0;child--)
		{
			if(theElement->theEditorWindow->windowType==EWT_DOCUMENT)				/* see if it belongs on the list */
			{
				(*theList)[(*numElements)++]=theElement->theEditorWindow;			/* assign it to the list */
			}
			theElement=theElement->nextElement;
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

bool GetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *foregroundColor)
// get the foreground color in use by theWindow
// if there is a problem, SetError, and return false
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
	(*foregroundColor)=theViewInfo->foregroundColor->theColor;

	return(true);
}

bool SetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR foregroundColor)
// set the foreground color to be used in theWindow
// if there is a problem, leave the color unchanged, set the error
// and return false
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	WINDOWLISTELEMENT
		*theWindowElement;
	HWND
		hwnd;

	if(SetDocumentForegroundColor(theWindow,foregroundColor,true))
	{
		theView=(EDITORVIEW *)theWindow->windowInfo;
		theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
		theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
		hwnd=theWindowElement->hwnd;
		InvalidateRect(hwnd,NULL,TRUE);		/* have it redraw with the colors */
		return(true);
	}
	return(false);
}

bool GetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *backgroundColor)
// get the background color in use by theWindow
// if there is a problem, SetError, and return false
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
	(*backgroundColor)=theViewInfo->backgroundColor->theColor;

	return(true);
}

bool SetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR backgroundColor)
// set the background color to be used in theWindow
// if there is a problem, leave the color unchanged, set the error
// and return false
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	WINDOWLISTELEMENT
		*theWindowElement;
	HWND
		hwnd;

	if(SetDocumentBackgroundColor(theWindow,backgroundColor,true))
	{
		theView=(EDITORVIEW *)theWindow->windowInfo;
		theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
		theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
		hwnd=theWindowElement->hwnd;
		InvalidateRect(hwnd,NULL,TRUE);		/* have it redraw with the colors */
		return(true);
	}
	return(false);
}


bool GetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect)
/* get the rectangle of theWindow
 * if there is a problem, SetError, and return FALSE
 */
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	WINDOWLISTELEMENT
		*theWindowElement;
	WINDOWPLACEMENT
		winPos;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
	winPos.length=sizeof(winPos);
	GetWindowPlacement(theWindowElement->hwnd,&winPos);
	theRect->x=winPos.rcNormalPosition.left;
	theRect->y=winPos.rcNormalPosition.top;
	theRect->w=winPos.rcNormalPosition.right-winPos.rcNormalPosition.left;
	theRect->h=winPos.rcNormalPosition.bottom-winPos.rcNormalPosition.top;
	
	return(true);
}

bool SetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect)
// set theWindow to theRect
// if there is a problem, leave theRect unchanged, set the error
// and return false
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	WINDOWLISTELEMENT
		*theWindowElement;

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	// point at the information record for this view
	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
	
	if(MDIShouldZoom())							// are we in MDI window maximized mode?
	{
		// Even though we are maximized we want to set the window's rect.
		// This will be the rect the window will use if it is restored.
		// The obvious ways to do this, like MoveWindow() with draw set to FALSE, did not work.

		WINDOWPLACEMENT
			winPos;
			
		winPos.length=sizeof(winPos);
		
		GetWindowPlacement(theWindowElement->hwnd,&winPos);
		
		winPos.rcNormalPosition.top = theRect->y;
		winPos.rcNormalPosition.left = theRect->x;
		winPos.rcNormalPosition.bottom = theRect->y + theRect->h;
		winPos.rcNormalPosition.right = theRect->x + theRect->w;
		winPos.showCmd = SW_SHOWMAXIMIZED;
		
		SetWindowPlacement(theWindowElement->hwnd,&winPos);
	}
	else											// no, we aren't.
	{
		MoveWindow(theWindowElement->hwnd,theRect->x,theRect->y,theRect->w,theRect->h,TRUE);
		SendMessage(clientWindowHwnd,WM_MDIRESTORE,(WPARAM)theWindowElement->hwnd,0);
	}

	return(true);
}

void GetEditorScreenDimensions(UINT32 *theWidth,UINT32 *theHeight)
// Return the size of the outer frame window, not the screen.
{
	 RECT
	 	winRect;
	 	
	 GetClientRect(frameWindowHwnd, &winRect);
	
	(*theWidth) = winRect.right;
	(*theHeight) = winRect.bottom - globalStatbarPntData.dyStatbar;
}

EDITORWINDOW *OpenDocumentWindow(EDITORBUFFER *theBuffer,EDITORRECT *theRect,char *theTitle,char *fontName,UINT32 tabSize,EDITORCOLOR foreground,EDITORCOLOR background)
/* open a document window onto theBuffer, place it at theRect
 * give the new window theTitle
 * if there is a problem, undo anything that was done,
 * set the error, then return NULL
 */
{
	EDITORWINDOW
		*theWindow;
	HWND
		hwnd;
	char
		*title;
	EDITORVIEWDESCRIPTOR
		viewDescriptor;
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	WINDOWLISTELEMENT
		*theWindowElement;
	DWORD
		style;

	if(theWindow=(EDITORWINDOW *)MNewPtrClr(sizeof(EDITORWINDOW)))							/* create editor window */
	{
		theWindow->theBuffer=theBuffer;

		if(title=(char *)MNewPtrClr(strlen(theTitle)+1))
		{
			if(theWindowElement=(WINDOWLISTELEMENT *)MNewPtrClr(sizeof(WINDOWLISTELEMENT)))	/* create a window list element */
			{
				theWindowElement->theEditorWindow=theWindow;							/* point to our high level concept of window */
				theWindow->userData=theWindowElement;									/* point back to the window element */
				LinkWindowElement(theWindowElement);
				strcpy(title,theTitle);

				theWindow->windowType=EWT_DOCUMENT;										/* set the editor window type to "document" */

				viewDescriptor.theBuffer=theBuffer;
				viewDescriptor.theRect.left=theRect->x;								/* place where the view is going */
				viewDescriptor.theRect.right=theRect->x+theRect->w;					/* place where the view is going */
				viewDescriptor.theRect.top=theRect->y;								/* place where the view is going */
				viewDescriptor.theRect.bottom=theRect->y+theRect->h;				/* place where the view is going */
				viewDescriptor.topLine=0;
				viewDescriptor.leftPixel=0;
				viewDescriptor.tabSize=tabSize;
				viewDescriptor.fontName=fontName;
				viewDescriptor.foregroundColor=foreground;
				viewDescriptor.backgroundColor=background;
				viewDescriptor.active=true;												/* create the view active */
				viewDescriptor.viewTextChangedVector=AdjustDocumentWindowScrollThumbsAndStatus;		/* fix the thumbs, and status when the view text changes */
				viewDescriptor.viewPositionChangedVector=AdjustDocumentWindowScrollThumbs;	/* fix the thumbs when the view position changes */
				viewDescriptor.viewSelectionChangedVector=AdjustDocumentWindowViewStatus;		/* fix the status when the selection changes */
				if(theWindow->windowInfo=CreateEditorView(theWindow,&viewDescriptor))
				{
					if(SetDocumentBackgroundColor(theWindow,background,false) && SetDocumentForegroundColor(theWindow,foreground,false))
					{
	    				style=WS_HSCROLL|WS_VSCROLL;
	    				if((theRect->w==0) && (theRect->h==0))		// if the width and height came in zero
	    				{
		    				theRect->w=(UINT32)CW_USEDEFAULT;		// size it to the default size
		    				theRect->h=(UINT32)CW_USEDEFAULT;
		    				style|=WS_MAXIMIZE;						// and start the window off maximized
		    			}
		    			else
		    			{
							if(MDIShouldZoom())
							{
				    			style|=WS_MAXIMIZE;					// start the window off maximized
							}
		    			}

						if(hwnd=CreateMDIWindow(childClass,title,style,theRect->x,theRect->y,theRect->w,theRect->h,clientWindowHwnd,programInstanceHandle,(LPARAM)theWindow))
						{
							theView=(EDITORVIEW *)theWindow->windowInfo;
							theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	// point at the information record for this view
							theViewInfo->titleString=title;
							return(theWindow);
						}
						else
						{
							SetWindowsError();
						}
					}
					DisposeEditorView((EDITORVIEW *)theWindow->windowInfo);
				}
				UnlinkWindowElement(theWindowElement);
				MDisposePtr(theWindowElement);
			}
			MDisposePtr(title);
		}
		MDisposePtr(theWindow);
	}
	return((EDITORWINDOW *)NULL);
}


void CloseDocumentWindow(EDITORWINDOW *theWindow)
// close an open document window
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	WINDOWLISTELEMENT
		*theWindowElement;

				
	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;					// point at the information record for this view
	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	SendMessage(clientWindowHwnd,WM_MDIDESTROY,(WPARAM)theWindowElement->hwnd,0);
	UnSetDocumentBackgroundColor(theWindow);						// get rid of colors allocated for it
	UnSetDocumentForegroundColor(theWindow);
	UnlinkWindowElement(theWindowElement);
	MDisposePtr(theViewInfo->titleString);
	DisposeEditorView(theView);
	MDisposePtr(theWindowElement);
	MDisposePtr(theWindow);
}
