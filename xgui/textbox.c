// Text box handling for dialogs
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

static void InvalidateTextBox(TEXTBOX *theTextBox)
// invalidate the text box within its parent window
{
	InvalidateWindowRect(theTextBox->parentWindow,&theTextBox->theRect);
}

static void GetTextBoxControlRects(TEXTBOX *theTextBox,EDITORRECT *borderRect,EDITORRECT *textContentRect)
// return the rectangles that enclose the various pieces of theTextBox
// if the text box starts getting too small, certain rects may be returned with 0 width or
// height
{
	EDITORRECT
		insetRect;

	insetRect=*borderRect=theTextBox->theRect;					// border rect is the whole thing

	insetRect.x+=TEXTBOXBORDERWIDTH;							// inset from the border to the content area of the text box
	insetRect.y+=TEXTBOXBORDERWIDTH;
	if(insetRect.w>2*TEXTBOXBORDERWIDTH)
	{
		insetRect.w-=2*TEXTBOXBORDERWIDTH;
	}
	else
	{
		insetRect.w=0;
	}
	if(insetRect.h>2*TEXTBOXBORDERWIDTH)
	{
		insetRect.h-=2*TEXTBOXBORDERWIDTH;
	}
	else
	{
		insetRect.h=0;
	}
	*textContentRect=insetRect;
}

bool PointInTextBox(TEXTBOX *theTextBox,INT32 x,INT32 y)
// return true if parent window relative x/y are in theTextBox
// false if not
{
	return(PointInRECT(x,y,&theTextBox->theRect));
}

void DrawTextBox(TEXTBOX *theTextBox,bool hasFocus)
// redraw theTextBox in its parent window, obey the invalid area of the parent's window
{
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	GC
		graphicsContext;
	XRectangle
		regionBox;
	Region
		invalidRegion;
	EDITORRECT
		borderRect,
		textContentRect;

	theWindowElement=(WINDOWLISTELEMENT *)theTextBox->parentWindow->userData;		// get the x window information associated with this window
	xWindow=theWindowElement->xWindow;												// get x window for this window
	graphicsContext=theWindowElement->graphicsContext;								// get graphics context for this window

	GetTextBoxControlRects(theTextBox,&borderRect,&textContentRect);

	invalidRegion=XCreateRegion();

	regionBox.x=borderRect.x;
	regionBox.y=borderRect.y;
	regionBox.width=borderRect.w;
	regionBox.height=borderRect.h;
	XUnionRectWithRegion(&regionBox,invalidRegion,invalidRegion);					// create a region which is the rectangle of the text box
	XIntersectRegion(invalidRegion,theWindowElement->invalidRegion,invalidRegion);	// intersect with what is invalid in this window

	if(!XEmptyRegion(invalidRegion))												// see if it has an invalid area
	{
		XSetRegion(xDisplay,graphicsContext,invalidRegion);							// set clipping to what's invalid
		OutlineShadowRectangle(xWindow,graphicsContext,&borderRect,gray1,white,TEXTBOXBORDERWIDTH);	// drop in border
		if(hasFocus)
		{
			OutlineShadowRectangle(xWindow,graphicsContext,&borderRect,black,black,TEXTBOXBORDERWIDTH-1);	// drop in border
		}
		XSetClipMask(xDisplay,graphicsContext,None);								// get rid of clip mask
	}
	XDestroyRegion(invalidRegion);													// get rid of invalid region
	DrawView(theTextBox->theView);
}

bool TrackTextBox(TEXTBOX *theTextBox,XEvent *theEvent)
// a button was pressed, see if it was in theTextBox, if so, track it, and return true
// if it was not within the text box, return false
{
	return(TrackView(theTextBox->theView,theEvent));	// only thing tracking in the text box is the view
}

void RepositionTextBox(TEXTBOX *theTextBox,EDITORRECT *newRect)
// change the position of theTextBox within its parent
// be nice, and only invalidate it if it actually changes
{
	EDITORRECT
		borderRect,
		textContentRect;

	if((theTextBox->theRect.x!=newRect->x)||(theTextBox->theRect.y!=newRect->y)||(theTextBox->theRect.w!=newRect->w)||(theTextBox->theRect.h!=newRect->h))
	{
		InvalidateTextBox(theTextBox);					// invalidate where it is now
		theTextBox->theRect=*newRect;					// copy over the new rectangle
		GetTextBoxControlRects(theTextBox,&borderRect,&textContentRect);
		SetViewBounds(theTextBox->theView,&textContentRect);	// move the view over
		InvalidateTextBox(theTextBox);					// invalidate where it is going
	}
}

void HandleTextBoxKeyEvent(TEXTBOX *theTextBox,XEvent *theEvent)
// A keyboard event is arriving for the text box, handle it here
{
	HandleViewKeyEvent(theTextBox->theView,theEvent);
}

void ActivateTextBox(TEXTBOX *theTextBox)
// when the text box gets focus, call this to make it active
{
	SetEditorViewStyleForegroundColor(theTextBox->theView,0,theTextBox->focusForegroundColor);
	SetEditorViewStyleBackgroundColor(theTextBox->theView,0,theTextBox->focusBackgroundColor);
	EditorActivateView(theTextBox->theView);
	ResetEditorViewCursorBlink(theTextBox->theView);
	InvalidateTextBox(theTextBox);						// invalidate where it is now
}

void DeactivateTextBox(TEXTBOX *theTextBox)
// when the text box loses focus, call this to make it inactive
{
	SetEditorViewStyleForegroundColor(theTextBox->theView,0,theTextBox->nofocusForegroundColor);
	SetEditorViewStyleBackgroundColor(theTextBox->theView,0,theTextBox->nofocusBackgroundColor);
	EditorDeactivateView(theTextBox->theView);
	InvalidateTextBox(theTextBox);						// invalidate where it is now
}

void HandleTextBoxPeriodicProc(TEXTBOX *theTextBox)
// call this to handle periodic things a text box must do
{
	ViewCursorTask(theTextBox->theView);
}

TEXTBOX *CreateTextBox(EDITORWINDOW *theWindow,TEXTBOXDESCRIPTOR *theDescription)
// create a text box in theWindow, invalidate the area of theWindow
// where the text box is to be placed
// return a pointer to it if successful
// SetError, return NULL if not
{
	TEXTBOX
		*theTextBox;
	EDITORRECT
		borderRect,
		textContentRect;
	EDITORVIEWDESCRIPTOR
		viewDescriptor;

	if((theTextBox=(TEXTBOX *)MNewPtr(sizeof(TEXTBOX))))		// create text box data structure
	{
		theTextBox->parentWindow=theWindow;
		theTextBox->theRect=theDescription->theRect;

		theTextBox->focusBackgroundColor=theDescription->focusBackgroundColor;	// copy over the colors
		theTextBox->focusForegroundColor=theDescription->focusForegroundColor;
		theTextBox->nofocusBackgroundColor=theDescription->nofocusBackgroundColor;
		theTextBox->nofocusForegroundColor=theDescription->nofocusForegroundColor;

		GetTextBoxControlRects(theTextBox,&borderRect,&textContentRect);

		viewDescriptor.theBuffer=theDescription->theBuffer;		// the buffer on which to view
		viewDescriptor.theRect=textContentRect;					// position and size within parent window
		viewDescriptor.topLine=theDescription->topLine;
		viewDescriptor.leftPixel=theDescription->leftPixel;
		viewDescriptor.tabSize=theDescription->tabSize;
		viewDescriptor.backgroundColor=theTextBox->nofocusBackgroundColor;
		viewDescriptor.foregroundColor=theTextBox->nofocusForegroundColor;
		viewDescriptor.fontName=theDescription->fontName;		// initial font to use
		viewDescriptor.active=false;							// tells if the view is created active or not
		viewDescriptor.viewTextChangedVector=NULL;
		viewDescriptor.viewSelectionChangedVector=NULL;
		viewDescriptor.viewPositionChangedVector=NULL;

		if((theTextBox->theView=CreateEditorView(theTextBox->parentWindow,&viewDescriptor)))
		{
			InvalidateTextBox(theTextBox);
			return(theTextBox);
		}
		MDisposePtr(theTextBox);
	}
	return(NULL);
}

void DisposeTextBox(TEXTBOX *theTextBox)
// unlink theTextBox from its parent window, and delete it
// invalidate the area of the window where the text box was
{
	InvalidateTextBox(theTextBox);					// invalidate where it is now
	DisposeEditorView(theTextBox->theView);
	MDisposePtr(theTextBox);
}

// dialog interface routines

static void DrawTextBoxItem(DIALOGITEM *theItem)
// draw the view pointed to by theItem
{
	DrawTextBox((TEXTBOX *)theItem->itemStruct,(theItem==theItem->theDialog->focusItem));	// draw the text box
}

static bool TrackTextBoxItem(DIALOGITEM *theItem,XEvent *theEvent)
// attempt to track the text box pointed to by theItem
{
	if(PointInTextBox((TEXTBOX *)theItem->itemStruct,theEvent->xbutton.x,theEvent->xbutton.y))
	{
		DialogTakeFocus(theItem);
		return(TrackTextBox((TEXTBOX *)theItem->itemStruct,theEvent));
	}
	return(false);
}

static void DisposeTextBoxItem(DIALOGITEM *theItem)
// dispose of the local text box structure pointed to by theItem
{
	DisposeTextBox((TEXTBOX *)theItem->itemStruct);
}

static void TextBoxItemFocusChange(DIALOGITEM *theItem)
// the focus is changing into, or out of this view item, deal with it
{
	if(theItem==theItem->theDialog->focusItem)
	{
		ActivateTextBox((TEXTBOX *)theItem->itemStruct);
	}
	else
	{
		DeactivateTextBox((TEXTBOX *)theItem->itemStruct);
	}
}

static void TextBoxItemFocusKey(DIALOGITEM *theItem,XEvent *theEvent)
// give the key to the text box
{
	HandleTextBoxKeyEvent((TEXTBOX *)theItem->itemStruct,theEvent);
}

static void TextBoxItemPeriodicProc(DIALOGITEM *theItem)
// this is used to flash the cursor of the view
{
	HandleTextBoxPeriodicProc((TEXTBOX *)theItem->itemStruct);
}

bool CreateTextBoxItem(DIALOGITEM *theItem,void *theDescriptor)
// create the textBox, and fill in theItem
// if there is a problem, SetError, return false
{
	if((theItem->itemStruct=(void *)CreateTextBox(theItem->theDialog->parentWindow,(TEXTBOXDESCRIPTOR *)theDescriptor)))
	{
		theItem->disposeProc=DisposeTextBoxItem;
		theItem->drawProc=DrawTextBoxItem;
		theItem->trackProc=TrackTextBoxItem;
		theItem->earlyKeyProc=NULL;
		theItem->wantFocus=true;
		theItem->focusChangeProc=TextBoxItemFocusChange;
		theItem->focusKeyProc=TextBoxItemFocusKey;
		theItem->focusPeriodicProc=TextBoxItemPeriodicProc;		// set up cursor flash task
		return(true);
	}
	return(false);
}
