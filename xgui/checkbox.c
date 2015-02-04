// Dialog check box handler
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

static bool PointInCheckBox(CHECKBOX *theCheckBox,INT32 x,INT32 y)
// return true if parent window relative x/y are in theCheckBox
// false if not
{
	return(PointInRECT(x,y,&theCheckBox->theRect));
}

static void InvalidateCheckBox(CHECKBOX *theCheckBox)
// invalidate the check box within its parent window
{
	InvalidateWindowRect(theCheckBox->parentWindow,&theCheckBox->theRect);
}

static void DrawCheckBox(CHECKBOX *theCheckBox)
// redraw theCheckBox in its parent window, obey the invalid area of the parent's window
{
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	GC
		graphicsContext;
	GUICOLOR
		*upLeftHighlight,
		*downRightHighlight;
	INT32
		stringX,
		stringY;
	INT32
		xLeft,
		xRight,
		yTop,
		yBottom;

	theWindowElement=(WINDOWLISTELEMENT *)theCheckBox->parentWindow->userData;		// get the x window information associated with this window
	xWindow=theWindowElement->xWindow;												// get x window for this window
	graphicsContext=theWindowElement->graphicsContext;								// get graphics context for this window

	XSetRegion(xDisplay,graphicsContext,theWindowElement->invalidRegion);			// set clipping to what's invalid

	if(theCheckBox->theRect.w&&theCheckBox->theRect.h)
	{
		XSetForeground(xDisplay,graphicsContext,gray3->theXPixel);
		XFillRectangle(xDisplay,xWindow,graphicsContext,theCheckBox->theRect.x,theCheckBox->theRect.y,theCheckBox->theRect.w,theCheckBox->theRect.h);

		upLeftHighlight=black;
		downRightHighlight=white;
		xLeft=theCheckBox->theRect.x;
		xRight=xLeft+theCheckBox->theRect.w-1;
		yTop=theCheckBox->theRect.y;
		yBottom=yTop+theCheckBox->theRect.h-1;

		XSetForeground(xDisplay,graphicsContext,upLeftHighlight->theXPixel);
		XDrawLine(xDisplay,xWindow,graphicsContext,xLeft,yTop,xRight,yTop);
		XDrawLine(xDisplay,xWindow,graphicsContext,xLeft,yTop,xLeft,yBottom);
		XSetForeground(xDisplay,graphicsContext,downRightHighlight->theXPixel);
		XDrawLine(xDisplay,xWindow,graphicsContext,xRight,yTop,xRight,yBottom);
		XDrawLine(xDisplay,xWindow,graphicsContext,xLeft,yBottom,xRight,yBottom);

		XSetBackground(xDisplay,graphicsContext,gray3->theXPixel);
		if(theCheckBox->highlight)
		{
			XSetForeground(xDisplay,graphicsContext,white->theXPixel);
		}
		else
		{
			XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		}

		if(theCheckBox->pressedState&&(*theCheckBox->pressedState))
		{
			XCopyPlane(xDisplay,checkedBoxPixmap,xWindow,graphicsContext,0,0,CHECKBOXWIDTH,CHECKBOXHEIGHT,theCheckBox->theRect.x+2,theCheckBox->theRect.y+(theCheckBox->theRect.h-CHECKBOXHEIGHT)/2,1);
		}
		else
		{
			XCopyPlane(xDisplay,checkBoxPixmap,xWindow,graphicsContext,0,0,CHECKBOXWIDTH,CHECKBOXHEIGHT,theCheckBox->theRect.x+2,theCheckBox->theRect.y+(theCheckBox->theRect.h-CHECKBOXHEIGHT)/2,1);
		}

		stringX=theCheckBox->theRect.x+CHECKBOXWIDTH+4;
		stringY=theCheckBox->theRect.y+(theCheckBox->theRect.h/2)-(theCheckBox->theFont->ascent+theCheckBox->theFont->descent)/2+theCheckBox->theFont->ascent;

		XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		XSetFont(xDisplay,graphicsContext,theCheckBox->theFont->theXFont->fid);						// point to the current font for this check box
		XDrawString(xDisplay,xWindow,graphicsContext,stringX,stringY,&(theCheckBox->checkBoxName[0]),strlen(&(theCheckBox->checkBoxName[0])));
	}
	XSetClipMask(xDisplay,graphicsContext,None);									// get rid of clip mask
}

static bool TrackCheckBox(CHECKBOX *theCheckBox,XEvent *theEvent)
// see if the point given by theEvent falls over theCheckBox,
// if so, track it, and return true
// if not, return false
{
	bool
		onCheckBox,
		wasOnCheckBox;
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

	if((theEvent->xbutton.button==Button1)&&PointInCheckBox(theCheckBox,theEvent->xbutton.x,theEvent->xbutton.y))
	{
		theCheckBox->highlight=true;
		InvalidateCheckBox(theCheckBox);
		UpdateEditorWindows();												// redraw anything that is needed
		wasOnCheckBox=onCheckBox=true;
		while(StillDown(Button1,1))
		{
			if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theCheckBox->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
			{
				onCheckBox=PointInCheckBox(theCheckBox,windowX,windowY);
			}
			if(wasOnCheckBox!=onCheckBox)
			{
				theCheckBox->highlight=onCheckBox;
				InvalidateCheckBox(theCheckBox);
				UpdateEditorWindows();										// redraw anything that is needed
				wasOnCheckBox=onCheckBox;
			}
		}
		if(onCheckBox)
		{
			if(theCheckBox->pressedState)
			{
				if((*theCheckBox->pressedState))							// toggle the state
				{
					(*theCheckBox->pressedState)=false;
				}
				else
				{
					(*theCheckBox->pressedState)=true;
				}
			}
			if(theCheckBox->pressedProc)
			{
				theCheckBox->pressedProc(theCheckBox,theCheckBox->pressedProcParameters);
			}
		}
		theCheckBox->highlight=false;
		InvalidateCheckBox(theCheckBox);
		return(true);
	}
	return(false);
}

static bool CheckBoxKeyOverride(CHECKBOX *theCheckBox,XEvent *theEvent)
// this is called for check boxes that may wish to grab keys before they go
// into the default focus of a dialog
// true is returned if the check box wanted the key
{
	KeySym
		theKeySym;

	if(theCheckBox->hasKey)
	{
		ComposeXLookupString((XKeyEvent *)theEvent,NULL,0,&theKeySym);
		if(theKeySym==theCheckBox->theKeySym)
		{
			theCheckBox->highlight=true;
			InvalidateCheckBox(theCheckBox);
			if(theCheckBox->pressedState)
			{
				(*theCheckBox->pressedState)=true;
			}
			if(theCheckBox->pressedProc)
			{
				theCheckBox->pressedProc(theCheckBox,theCheckBox->pressedProcParameters);
			}
			theCheckBox->highlight=false;
			InvalidateCheckBox(theCheckBox);
			return(true);
		}
	}
	return(false);
}

/* unused code
static void RepositionCheckBox(CHECKBOX *theCheckBox,EDITORRECT *newRect)
// change the position of theCheckBox within its parent
// be nice, and only invalidate it if it actually changes
{
	if((theCheckBox->theRect.x!=newRect->x)||(theCheckBox->theRect.y!=newRect->y)||(theCheckBox->theRect.w!=newRect->w)||(theCheckBox->theRect.h!=newRect->h))
	{
		InvalidateCheckBox(theCheckBox);						// invalidate where it is now
		theCheckBox->theRect=*newRect;
		InvalidateCheckBox(theCheckBox);						// invalidate where it is going
	}
}
*/

static CHECKBOX *CreateCheckBox(EDITORWINDOW *theWindow,CHECKBOXDESCRIPTOR *theDescriptor)
// create a check box in theWindow, invalidate the area of theWindow
// where the check box is to be placed
// return a pointer to it if successful
// SetError, and return NULL if not
{
	CHECKBOX
		*theCheckBox;

	if((theCheckBox=(CHECKBOX *)MNewPtr(sizeof(CHECKBOX))))		// create check box data structure
	{
		theCheckBox->parentWindow=theWindow;
		theCheckBox->theRect=theDescriptor->theRect;
		strncpy(&(theCheckBox->checkBoxName[0]),theDescriptor->checkBoxName,MAXCHECKBOXNAME);
		theCheckBox->checkBoxName[MAXCHECKBOXNAME-1]='\0';		// terminate the string as needed
		theCheckBox->highlight=false;							// not highlighted
		theCheckBox->hasKey=theDescriptor->hasKey;				// true if this check box has a key code associated with it
		theCheckBox->theKeySym=theDescriptor->theKeySym;		// equivalent key code for this check box
		theCheckBox->pressedState=theDescriptor->pressedState;	// points to a bool (or to NULL) that is set to true if this check box has been pressed
		theCheckBox->pressedProc=theDescriptor->pressedProc;	// copy the procedure to be called when the check box is pressed
		theCheckBox->pressedProcParameters=theDescriptor->pressedProcParameters;	// copy parameters for pressed Proc
		if((theCheckBox->theFont=LoadFont(theDescriptor->fontName)))
		{
			InvalidateCheckBox(theCheckBox);
			return(theCheckBox);
		}
		MDisposePtr(theCheckBox);
	}
	return(NULL);
}

static void DisposeCheckBox(CHECKBOX *theCheckBox)
// unlink theCheckBox from its parent window, and delete it
// invalidate the area of the window where theCheckBox was
{
	InvalidateCheckBox(theCheckBox);					// invalidate where it is now
	FreeFont(theCheckBox->theFont);
	MDisposePtr(theCheckBox);
}

// dialog interface routines

static void DrawCheckBoxItem(DIALOGITEM *theItem)
// draw the check box pointed to by theItem
{
	DrawCheckBox((CHECKBOX *)theItem->itemStruct);
}

static bool TrackCheckBoxItem(DIALOGITEM *theItem,XEvent *theEvent)
// track the check box pointed to by theItem
{
	return(TrackCheckBox((CHECKBOX *)theItem->itemStruct,theEvent));
}

static bool EarlyKeyCheckBoxItem(DIALOGITEM *theItem,XEvent *theEvent)
// see if this check box wants the key given by theEvent, if so, return true
{
	return(CheckBoxKeyOverride((CHECKBOX *)theItem->itemStruct,theEvent));
}

static void DisposeCheckBoxItem(DIALOGITEM *theItem)
// dispose of the local check box structure pointed to by theItem
{
	DisposeCheckBox((CHECKBOX *)theItem->itemStruct);
}

bool CreateCheckBoxItem(DIALOGITEM *theItem,void *theDescriptor)
// create the check box, and fill in theItem
// if there is a problem, SetError, return false
{
	if((theItem->itemStruct=(void *)CreateCheckBox(theItem->theDialog->parentWindow,(CHECKBOXDESCRIPTOR *)theDescriptor)))
	{
		theItem->disposeProc=DisposeCheckBoxItem;
		theItem->drawProc=DrawCheckBoxItem;
		theItem->trackProc=TrackCheckBoxItem;
		theItem->earlyKeyProc=EarlyKeyCheckBoxItem;
		theItem->wantFocus=false;
		theItem->focusChangeProc=NULL;
		theItem->focusKeyProc=NULL;
		theItem->focusPeriodicProc=NULL;
		return(true);
	}
	return(false);
}
