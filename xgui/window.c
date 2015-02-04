// Generic window handling
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

static bool
	hadInvalidation=false;

void GetEditorScreenDimensions(UINT32 *theWidth,UINT32 *theHeight)
// return the dimensions of the screen that the editor is running on
{
	XWindowAttributes
		theAttributes;

	XGetWindowAttributes(xDisplay,RootWindow(xDisplay,xScreenNum),&theAttributes);
	*theWidth=theAttributes.width;
	*theHeight=theAttributes.height;
}

void InvalidateWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect)
// invalidate theRect of theWindow, so it is drawn the next time we redraw
{
	XRectangle
		windowRect;

	if(theRect->w&&theRect->h)	// NOTE: this gets around a bug in X11/MOTIF (does not properly handle adding 0 width or height to a region!)
	{
		windowRect.x=theRect->x;
		windowRect.y=theRect->y;
		windowRect.width=theRect->w;
		windowRect.height=theRect->h;
		XUnionRectWithRegion(&windowRect,((WINDOWLISTELEMENT *)(theWindow->userData))->invalidRegion,((WINDOWLISTELEMENT *)(theWindow->userData))->invalidRegion);	// add to invalid region of the window
		hadInvalidation=true;
	}
}

void InvalidateWindowRegion(EDITORWINDOW *theWindow,Region theRegion)
// invalidate theRegion of theWindow, so it is drawn the next time we redraw
{
	XUnionRegion(theRegion,((WINDOWLISTELEMENT *)theWindow->userData)->invalidRegion,((WINDOWLISTELEMENT *)theWindow->userData)->invalidRegion);	// make this part invalid
	hadInvalidation=true;
}

void RedrawInvalidWindows()
// run though the window list, and redraw anything that
// has invalid area
// NOTE: for efficiency, this checks a global which tells
// if any invalidations have happened, and if not, it does nothing
{
	WINDOWLISTELEMENT
		*theElement;

	if(hadInvalidation)
	{
		hadInvalidation=false;										// clear now, so updates can cause further invalidation (not that they should want to, but just to be correct)
		theElement=windowListHead;									// update all windows that need it
		while(theElement)
		{
			switch(theElement->theEditorWindow->windowType)
			{
				case EWT_DOCUMENT:
					UpdateDocumentWindow(theElement->theEditorWindow);
					break;
				case EWT_DIALOG:
					UpdateDialogWindow(theElement->theEditorWindow);
					break;
				case EWT_MENU:
					UpdateMenuWindow(theElement->theEditorWindow);
					break;
				default:
					break;
			}
			theElement=theElement->nextElement;
		}
	}
}

void SetRealTopWindow(WINDOWLISTELEMENT *theElement)
// given the xWindow field of theElement, find the actual top level window
// and set the realTopLevelWindow field of theElement
// NOTE: this is ugly, but since a window manager can take it upon itself to change the
// sibling relationship of my windows (argggg!) there is really no other choice
{
	Window
		nextUp,
		root,
		parent,
		*children;
	unsigned int
		numChildren;

	nextUp=theElement->xWindow;
	do
	{
		theElement->realTopWindow=nextUp;
		XQueryTree(xDisplay,nextUp,&root,&parent,&children,&numChildren);
		XFree((char *)children);
		nextUp=parent;
	}
	while(parent!=root);
}

static Bool MapEventPredicate(Display *theDisplay,XEvent *theEvent,char *arg)
// use this to locate the mapping event for the given window
{
	if((theEvent->type==MapNotify)&&(theEvent->xmap.window==*((Window *)arg)))
	{
		return(True);
	}
	return(False);
}

static Bool FocusEventPredicate(Display *theDisplay,XEvent *theEvent,char *arg)
// use this to locate the focus event for the given window
{
	if((theEvent->type==FocusIn)&&(theEvent->xfocus.window==*((Window *)arg)))
	{
		return(True);
	}
	return(False);
}

static Bool DestroyEventPredicate(Display *theDisplay,XEvent *theEvent,char *arg)
// use this to locate the destroy event for the given window (actually, this
// gets ANY event for the window -- that way, no stray events are left after
// the window is gone)
{
	if(theEvent->xmap.window==*((Window *)arg))
	{
		return(True);
	}
	return(False);
}

void WaitUntilMappedNoFocus(EDITORWINDOW *theWindow)
// wait until a mapping event for theWindow arrives
{
	XEvent
		theEvent;
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	XIfEvent(xDisplay,&theEvent,MapEventPredicate,(char *)&theWindowElement->xWindow);	// wait here until our window is mapped
	SetRealTopWindow(theWindowElement);													// once mapped, get the actual top level window
}

void WaitUntilMapped(EDITORWINDOW *theWindow)
// wait until a mapping event for theWindow arrives, then set the focus to that window, and
// wait for it to complete
{
	XEvent
		theEvent;
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	XIfEvent(xDisplay,&theEvent,MapEventPredicate,(char *)&theWindowElement->xWindow);	// wait here until our window is mapped
	XSetInputFocus(xDisplay,theWindowElement->xWindow,RevertToNone,CurrentTime);
	XIfEvent(xDisplay,&theEvent,FocusEventPredicate,(char *)&theWindowElement->xWindow);	// wait here until our window is mapped
	SetRealTopWindow(theWindowElement);													// once mapped, get the actual top level window
}

void WaitUntilDestroyed(EDITORWINDOW *theWindow)
// wait until a the destroy event for theWindow arrives, eat all other events arriving for the window
// while we wait
{
	XEvent
		theEvent;
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	do
	{
		XIfEvent(xDisplay,&theEvent,DestroyEventPredicate,(char *)&theWindowElement->xWindow);	// wait here until our window is destroyed
	}
	while(theEvent.type!=DestroyNotify);
}

void LinkWindowElement(WINDOWLISTELEMENT *theElement)
// passed a WINDOWLISTELEMENT, link it to the start of the global list
{
	if(windowListHead)									// point previous back to new
	{
		windowListHead->previousElement=theElement;
	}
	theElement->nextElement=windowListHead;				// point to the old head of the list
	theElement->previousElement=NULL;					// no previous element
	windowListHead=theElement;							// point the head at the new element
}

void UnlinkWindowElement(WINDOWLISTELEMENT *theElement)
// passed a WINDOWLISTELEMENT, unlink it from the global list
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
		windowListHead=theElement->nextElement;			// this one was at the head, so update
	}
}

WINDOWLISTELEMENT *LocateWindowElement(Window xWindow)
// passed an xWindow, return its list element, or NULL if none is found
// NOTE: this will find the element if its xWindow field matches xWindow, OR
// if its realTopWindow field matches xWindow
{
	WINDOWLISTELEMENT
		*theElement;
	bool
		found;

	theElement=windowListHead;
	found=false;
	while(theElement&&!found)
	{
		if((theElement->xWindow==xWindow)||(theElement->realTopWindow==xWindow))
		{
			found=true;
		}
		else
		{
			theElement=theElement->nextElement;
		}
	}
	return(theElement);
}

void MinimizeEditorWindow(EDITORWINDOW *theWindow)
// iconify the passed window
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;						// get the x window information associated with this document window
	XIconifyWindow(xDisplay,theWindowElement->xWindow,xScreenNum);
}

void UnminimizeEditorWindow(EDITORWINDOW *theWindow)
// undo what MinimizeEditorWindow did
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;						// get the x window information associated with this document window
	XMapWindow(xDisplay,theWindowElement->xWindow);
}

EDITORWINDOW *GetTopEditorWindow()
// return the top-most editor window, or NULL if there is none
// this indirectly uses the realTopWindow fields of the window elements
{
	WINDOWLISTELEMENT
		*theElement;
	EDITORWINDOW
		*theWindow;
	Window
		root,
		parent,
		*children;
	unsigned int
		child,
		numChildren;

	XQueryTree(xDisplay,RootWindow(xDisplay,xScreenNum),&root,&parent,&children,&numChildren);
	theWindow=NULL;
	for(child=numChildren;!theWindow&&(child>0);child--)
	{
		if((theElement=LocateWindowElement(children[child-1])))
		{
			theWindow=theElement->theEditorWindow;
		}
	}
	XFree((char *)children);
	return(theWindow);
}

void SetTopEditorWindow(EDITORWINDOW *theWindow)
// set the passed document window to the top of the display
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;						// get the x window information associated with this document window
	XMapWindow(xDisplay,theWindowElement->xWindow);									// make sure it's not iconified -- Thanks Michael Manning
	XRaiseWindow(xDisplay,theWindowElement->xWindow);								// place it on top
	XSetInputFocus(xDisplay,theWindowElement->xWindow,RevertToNone,CurrentTime);	// give it the focus
}

EDITORWINDOW *GetTopEditorWindowType(UINT16 windowType)
// return the top-most window of windowType, or NULL if there is none
// this indirectly uses the realTopWindow fields of the window elements
{
	WINDOWLISTELEMENT
		*theElement;
	EDITORWINDOW
		*theWindow;
	Window
		root,
		parent,
		*children;
	unsigned int
		child,
		numChildren;

	XQueryTree(xDisplay,RootWindow(xDisplay,xScreenNum),&root,&parent,&children,&numChildren);
	theWindow=NULL;
	for(child=numChildren;!theWindow&&(child>0);child--)
	{
		if((theElement=LocateWindowElement(children[child-1])))
		{
			if(theElement->theEditorWindow->windowType==windowType)
			{
				theWindow=theElement->theEditorWindow;
			}
		}
	}
	XFree((char *)children);
	return(theWindow);
}

bool GetSortedEditorWindowTypeList(UINT16 windowType,UINT32 *numElements,EDITORWINDOW ***theList)
// return a list of window pointers, sorted top to bottom for the given type
// if there is a problem, SetError, and return false
// if true is returned, then theList must be MDisposePtr'ed later (even if numElements
// is 0)
{
	WINDOWLISTELEMENT
		*theElement;
	Window
		root,
		parent,
		*children;
	unsigned int
		child,
		numChildren;
	bool
		fail;

	fail=false;
	*numElements=0;																	// no elements yet
	XQueryTree(xDisplay,RootWindow(xDisplay,xScreenNum),&root,&parent,&children,&numChildren);
	if(((*theList)=(EDITORWINDOW **)MNewPtr(sizeof(EDITORWINDOW *)*numChildren)))	// create a list of pointers large enough to hold the maximum number of windows located
	{
		for(child=numChildren;child>0;child--)
		{
			if((theElement=LocateWindowElement(children[child-1])))					// hunt down the window based on this
			{
				if(theElement->theEditorWindow->windowType==windowType)				// see if it belongs on the list
				{
					(*theList)[(*numElements)++]=theElement->theEditorWindow;		// assign it to the list
				}
			}
		}
	}
	else
	{
		fail=true;
	}
	XFree((char *)children);
	return(!fail);
}

EDITORWINDOW *GetEditorFocusWindow()
// return the editor window with the focus, or the top window
// if none has focus
// if there is no top window, return NULL
{
	EDITORWINDOW
		*theWindow;
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	int
		focusRevert;

	XGetInputFocus(xDisplay,&xWindow,&focusRevert);			// find out who has the focus
	if((theWindowElement=LocateWindowElement(xWindow)))		// see if it is one of ours
	{
		if((theWindowElement->theEditorWindow->windowType==EWT_DOCUMENT)||(theWindowElement->theEditorWindow->windowType==EWT_DIALOG))
		{
			return(theWindowElement->theEditorWindow);		// if there is one of our documents/dialogs with focus, it is preferred
		}
		else
		{
			if((theWindow=GetTopEditorWindowType(EWT_DIALOG)))		// try to find the top-most dialog
			{
				return(theWindow);
			}
			else
			{
				if((theWindow=GetTopEditorWindowType(EWT_DOCUMENT)))	// try to find the top-most document
				{
					return(theWindow);
				}
				else
				{
					return(theWindowElement->theEditorWindow);	// no document/dialog, so give back whatever it was we found in the first place
				}
			}
		}
	}
	if((theWindow=GetTopEditorWindowType(EWT_DIALOG)))		// if focus window was not ours try to find the top-most dialog
	{
		return(theWindow);
	}
	else
	{
		if((theWindow=GetTopEditorWindowType(EWT_DOCUMENT)))	// try to find the top-most document
		{
			return(theWindow);
		}
	}
	return(rootMenuWindow);									// as a last resort, give back the root menu
}

void FocusAwayFrom(EDITORWINDOW *theWindow)
// set the focus to the topmost window that is not theWindow
// this is done to fool X into not readjusting the focus when we
// delete theWindow
{
	WINDOWLISTELEMENT
		*theElement;
	bool
		haveFocus;
	Window
		focusWindow;
	Window
		root,
		parent,
		*children;
	unsigned int
		child,
		numChildren;

	XQueryTree(xDisplay,RootWindow(xDisplay,xScreenNum),&root,&parent,&children,&numChildren);
	haveFocus=false;
	focusWindow=0;		// stop compiler from complaining
	for(child=numChildren;!haveFocus&&(child>0);child--)
	{
		if((theElement=LocateWindowElement(children[child-1])))
		{
			if(theElement->theEditorWindow!=theWindow)
			{
				focusWindow=theElement->xWindow;
				haveFocus=true;
			}
		}
	}
	XFree((char *)children);
	if(haveFocus)
	{
		XSetInputFocus(xDisplay,focusWindow,RevertToNone,CurrentTime);	// give focus to this window
		XSync(xDisplay,false);
	}
}
