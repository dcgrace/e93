// Separator handling (draw separation lines for dialogs)
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

static void InvalidateSeparator(SEPARATOR *theSeparator)
// invalidate theSeparator within its parent window
{
	EDITORRECT
		theRect;

	if(theSeparator->horizontal)
	{
		theRect.x=theSeparator->start;
		theRect.w=theSeparator->end-theSeparator->start;
		theRect.y=theSeparator->index;
		theRect.h=2;
	}
	else
	{
		theRect.x=theSeparator->index;
		theRect.w=2;
		theRect.y=theSeparator->start;
		theRect.h=theSeparator->end-theSeparator->start;
	}
	InvalidateWindowRect(theSeparator->parentWindow,&theRect);
}

static void DrawSeparator(SEPARATOR *theSeparator)
// redraw theSeparator in its parent window, obey the invalid area of the parent's window
{
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	GC
		graphicsContext;

	theWindowElement=(WINDOWLISTELEMENT *)theSeparator->parentWindow->userData;		// get the x window information associated with this window
	xWindow=theWindowElement->xWindow;												// get x window for this window
	graphicsContext=theWindowElement->graphicsContext;								// get graphics context for this window

	XSetRegion(xDisplay,graphicsContext,theWindowElement->invalidRegion);			// clip to area we wish to draw in

	if(theSeparator->horizontal)
	{
		XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		XDrawLine(xDisplay,xWindow,graphicsContext,theSeparator->start,theSeparator->index,theSeparator->end,theSeparator->index);
		XSetForeground(xDisplay,graphicsContext,white->theXPixel);
		XDrawLine(xDisplay,xWindow,graphicsContext,theSeparator->start,theSeparator->index+1,theSeparator->end,theSeparator->index+1);
	}
	else
	{
		XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		XDrawLine(xDisplay,xWindow,graphicsContext,theSeparator->index,theSeparator->start,theSeparator->index,theSeparator->end);
		XSetForeground(xDisplay,graphicsContext,white->theXPixel);
		XDrawLine(xDisplay,xWindow,graphicsContext,theSeparator->index+1,theSeparator->start,theSeparator->index+1,theSeparator->end);
	}
	XSetClipMask(xDisplay,graphicsContext,None);									// get rid of clip mask
}

static SEPARATOR *CreateSeparator(EDITORWINDOW *theWindow,SEPARATORDESCRIPTOR *theDescription)
// create a separator item in theWindow, invalidate the area of theWindow
// where the separator is to be placed
// return a pointer to it if successful
// SetError, and return NULL if not
{
	SEPARATOR
		*theSeparator;

	if((theSeparator=(SEPARATOR *)MNewPtr(sizeof(SEPARATOR))))			// create data structure
	{
		theSeparator->parentWindow=theWindow;
		theSeparator->horizontal=theDescription->horizontal;
		theSeparator->index=theDescription->index;
		theSeparator->start=theDescription->start;
		theSeparator->end=theDescription->end;

		InvalidateSeparator(theSeparator);
		return(theSeparator);
	}
	return(NULL);
}

static void DisposeSeparator(SEPARATOR *theSeparator)
// unlink theSeparator from its parent window, and delete it
// invalidate the area of the window where theSeparator was
{
	InvalidateSeparator(theSeparator);									// invalidate where it is now
	MDisposePtr(theSeparator);
}

// dialog interface routines

static void DrawSeparatorItem(DIALOGITEM *theItem)
// draw the separator pointed to by theItem
{
	DrawSeparator((SEPARATOR *)theItem->itemStruct);
}

static void DisposeSeparatorItem(DIALOGITEM *theItem)
// dispose of the local separator structure pointed to by theItem
{
	DisposeSeparator((SEPARATOR *)theItem->itemStruct);
}

bool CreateSeparatorItem(DIALOGITEM *theItem,void *theDescription)
// create the separator, and fill in theItem
// if there is a problem, SetError, return false
{
	if((theItem->itemStruct=(void *)CreateSeparator(theItem->theDialog->parentWindow,(SEPARATORDESCRIPTOR *)theDescription)))
	{
		theItem->disposeProc=DisposeSeparatorItem;
		theItem->drawProc=DrawSeparatorItem;
		theItem->trackProc=NULL;
		theItem->earlyKeyProc=NULL;
		theItem->wantFocus=false;
		theItem->focusChangeProc=NULL;
		theItem->focusKeyProc=NULL;
		theItem->focusPeriodicProc=NULL;
		return(true);
	}
	return(false);
}
