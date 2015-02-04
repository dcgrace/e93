// Dialog push button handler
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

static bool PointInButton(BUTTON *theButton,INT32 x,INT32 y)
// return true if parent window relative x/y are in theButton
// false if not
{
	return(PointInRECT(x,y,&theButton->theRect));
}

static void InvalidateButton(BUTTON *theButton)
// invalidate the button within its parent window
{
	InvalidateWindowRect(theButton->parentWindow,&theButton->theRect);
}

static void DrawButton(BUTTON *theButton)
// redraw theButton in its parent window, obey the invalid area of the parent's window
{
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	GC
		graphicsContext;
	int
		stringLength,
		stringWidth;
	int
		stringX,
		stringY;
	GUICOLOR
		*background,
		*upLeftHighlight,
		*downRightHighlight;

	theWindowElement=(WINDOWLISTELEMENT *)theButton->parentWindow->userData;		// get the x window information associated with this window
	xWindow=theWindowElement->xWindow;												// get x window for this window
	graphicsContext=theWindowElement->graphicsContext;								// get graphics context for this window

	XSetRegion(xDisplay,graphicsContext,theWindowElement->invalidRegion);			// set clipping to what's invalid

	if(theButton->theRect.w&&theButton->theRect.h)
	{
		if(theButton->highlight)
		{
			background=gray2;
			upLeftHighlight=black;
			downRightHighlight=white;
		}
		else
		{
			background=gray3;
			upLeftHighlight=white;
			downRightHighlight=black;
		}
		XSetFillStyle(xDisplay,graphicsContext,FillSolid);
		XSetForeground(xDisplay,graphicsContext,background->theXPixel);
		XFillRectangle(xDisplay,xWindow,graphicsContext,theButton->theRect.x,theButton->theRect.y,theButton->theRect.w,theButton->theRect.h);

		OutlineShadowRectangle(xWindow,graphicsContext,&theButton->theRect,upLeftHighlight,downRightHighlight,2);

		stringLength=strlen(&(theButton->buttonName[0]));
		stringWidth=XTextWidth(theButton->theFont->theXFont,&(theButton->buttonName[0]),stringLength);
		stringX=theButton->theRect.x+(theButton->theRect.w/2)-(stringWidth/2);
		stringY=theButton->theRect.y+(theButton->theRect.h/2)-(theButton->theFont->ascent+theButton->theFont->descent)/2+theButton->theFont->ascent;

		if(theButton->highlight)
		{
			stringX--;
			stringY--;
		}

		XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		XSetFont(xDisplay,graphicsContext,theButton->theFont->theXFont->fid);		// point to the current font for this button
		XDrawString(xDisplay,xWindow,graphicsContext,stringX,stringY,&(theButton->buttonName[0]),stringLength);
	}
	XSetClipMask(xDisplay,graphicsContext,None);									// get rid of clip mask
}

static void PressButton(BUTTON *theButton)
// call this and theButton will behave as if it has been pressed
{
	theButton->highlight=true;
	InvalidateButton(theButton);
	UpdateEditorWindows();
	if(theButton->pressedState)
	{
		(*theButton->pressedState)=true;
	}
	if(theButton->pressedProc)
	{
		theButton->pressedProc(theButton,theButton->pressedProcParameters);
	}
	theButton->highlight=false;
	InvalidateButton(theButton);
}

static bool TrackButton(BUTTON *theButton,XEvent *theEvent)
// see if the point given by theEvent falls over theButton,
// if so, track it, and return true
// if not, return false
{
	bool
		onButton,
		wasOnButton;
	Window
		root,
		child;
	int
		rootX,
		rootY,
		windowX,
		windowY;
	unsigned int
		state;

	if((theEvent->xbutton.button==Button1)&&PointInButton(theButton,theEvent->xbutton.x,theEvent->xbutton.y))
	{
		theButton->highlight=true;
		InvalidateButton(theButton);
		UpdateEditorWindows();												// redraw anything that is needed
		wasOnButton=onButton=true;
		while(StillDown(Button1,1))
		{
			if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theButton->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
			{
				onButton=PointInButton(theButton,windowX,windowY);
			}
			if(wasOnButton!=onButton)
			{
				theButton->highlight=onButton;
				InvalidateButton(theButton);
				UpdateEditorWindows();										// redraw anything that is needed
				wasOnButton=onButton;
			}
		}
		if(onButton)
		{
			PressButton(theButton);
		}
		theButton->highlight=false;
		InvalidateButton(theButton);
		return(true);
	}
	return(false);
}

static bool ButtonKeyOverride(BUTTON *theButton,XEvent *theEvent)
// this is called for buttons that may wish to grab keys before they go
// into the default focus of a dialog
// true is returned if the button wanted the key
{
	KeySym
		theKeySym;
	XKeyEvent
		localKeyEvent;														// make a local copy so we can strip annoying bits

	if(theButton->hasKey)
	{
		localKeyEvent=*(XKeyEvent *)theEvent;								// copy the event
		localKeyEvent.state&=(~(Mod1Mask|LockMask));						// remove these from consideration
		ComposeXLookupString(&localKeyEvent,NULL,0,&theKeySym);
		if(theKeySym==theButton->theKeySym)
		{
			PressButton(theButton);
			return(true);
		}
	}
	return(false);
}

/* unused code
static void RepositionButton(BUTTON *theButton,EDITORRECT *newRect)
// change the position of theButton within its parent
// be nice, and only invalidate it if it actually changes
{
	if((theButton->theRect.x!=newRect->x)||(theButton->theRect.y!=newRect->y)||(theButton->theRect.w!=newRect->w)||(theButton->theRect.h!=newRect->h))
	{
		InvalidateButton(theButton);						// invalidate where it is now
		theButton->theRect=*newRect;
		InvalidateButton(theButton);						// invalidate where it is going
	}
}
*/

static BUTTON *CreateButton(EDITORWINDOW *theWindow,BUTTONDESCRIPTOR *theDescriptor)
// create a button in theWindow, invalidate the area of theWindow
// where the button is to be placed
// return a pointer to it if successful
// SetError, and return NULL if not
{
	BUTTON
		*theButton;

	if((theButton=(BUTTON *)MNewPtr(sizeof(BUTTON))))		// create button data structure
	{
		theButton->parentWindow=theWindow;
		theButton->theRect=theDescriptor->theRect;
		strncpy(&(theButton->buttonName[0]),theDescriptor->buttonName,MAXBUTTONNAME);
		theButton->buttonName[MAXBUTTONNAME-1]='\0';		// terminate the string as needed
		theButton->highlight=false;							// not highlighted
		theButton->hasKey=theDescriptor->hasKey;			// true if this button has a key code associated with it
		theButton->theKeySym=theDescriptor->theKeySym;		// equivalent key code for this button
		theButton->pressedState=theDescriptor->pressedState;	// points to a bool (or to NULL) that is set to true if this button has been pressed
		theButton->pressedProc=theDescriptor->pressedProc;	// copy the procedure to be called when the button is pressed
		theButton->pressedProcParameters=theDescriptor->pressedProcParameters;	// copy parameters for pressed Proc
		if((theButton->theFont=LoadFont(theDescriptor->fontName)))
		{
			InvalidateButton(theButton);
			return(theButton);
		}
		MDisposePtr(theButton);
	}
	return(NULL);
}

static void DisposeButton(BUTTON *theButton)
// unlink theButton from its parent window, and delete it
// invalidate the area of the window where theButton was
{
	InvalidateButton(theButton);					// invalidate where it is now
	FreeFont(theButton->theFont);
	MDisposePtr(theButton);
}

// dialog interface routines

static void DrawButtonItem(DIALOGITEM *theItem)
// draw the button pointed to by theItem
{
	DrawButton((BUTTON *)theItem->itemStruct);
}

void PressButtonItem(DIALOGITEM *theItem)
// Do whatever would be done if the user pressed theItem
{
	PressButton((BUTTON *)theItem->itemStruct);
}

static bool TrackButtonItem(DIALOGITEM *theItem,XEvent *theEvent)
// track the button pointed to by theItem
{
	return(TrackButton((BUTTON *)theItem->itemStruct,theEvent));
}

static bool EarlyKeyButtonItem(DIALOGITEM *theItem,XEvent *theEvent)
// see if this button wants the key given by theEvent, if so, return true
{
	return(ButtonKeyOverride((BUTTON *)theItem->itemStruct,theEvent));
}

static void DisposeButtonItem(DIALOGITEM *theItem)
// dispose of the local button structure pointed to by theItem
{
	DisposeButton((BUTTON *)theItem->itemStruct);
}

bool CreateButtonItem(DIALOGITEM *theItem,void *theDescriptor)
// create the button, and fill in theItem
// if there is a problem, SetError, return false
{
	if((theItem->itemStruct=(void *)CreateButton(theItem->theDialog->parentWindow,(BUTTONDESCRIPTOR *)theDescriptor)))
	{
		theItem->disposeProc=DisposeButtonItem;
		theItem->drawProc=DrawButtonItem;
		theItem->trackProc=TrackButtonItem;
		theItem->earlyKeyProc=EarlyKeyButtonItem;
		theItem->wantFocus=false;
		theItem->focusChangeProc=NULL;
		theItem->focusKeyProc=NULL;
		theItem->focusPeriodicProc=NULL;
		return(true);
	}
	return(false);
}
