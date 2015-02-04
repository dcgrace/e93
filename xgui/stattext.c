// Static text handler for dialogs
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

static void InvalidateStaticText(STATICTEXT *theText)
// invalidate theText within its parent window
{
	InvalidateWindowRect(theText->parentWindow,&theText->theRect);
}

static void DrawStaticText(STATICTEXT *theText)
// redraw theText in its parent window, obey the invalid area of the parent's window
{
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	GC
		graphicsContext;
	XRectangle
		textRect;
	Region
		textRegion;
	int
		stringLength,
		currentIndex,
		lastIndex;
	int
		stringX,
		stringY;

	theWindowElement=(WINDOWLISTELEMENT *)theText->parentWindow->userData;			// get the x window information associated with this window
	xWindow=theWindowElement->xWindow;												// get x window for this window
	graphicsContext=theWindowElement->graphicsContext;								// get graphics context for this window

	textRect.x=theText->theRect.x;
	textRect.y=theText->theRect.y;
	textRect.width=theText->theRect.w;
	textRect.height=theText->theRect.h;
	textRegion=XCreateRegion();
	XUnionRectWithRegion(&textRect,textRegion,textRegion);							// make region for the text we are about to draw
	XIntersectRegion(textRegion,theWindowElement->invalidRegion,textRegion);		// see if it intersects with the invalid region

	XSetRegion(xDisplay,graphicsContext,textRegion);								// clip to area we wish to draw text in

	XSetForeground(xDisplay,graphicsContext,theText->backgroundColor->theXPixel);
	XFillRectangle(xDisplay,xWindow,graphicsContext,theText->theRect.x,theText->theRect.y,theText->theRect.w,theText->theRect.h);

	XSetForeground(xDisplay,graphicsContext,theText->foregroundColor->theXPixel);
	XSetFont(xDisplay,graphicsContext,theText->theFont->theXFont->fid);				// point to the current font for this text

	stringX=theText->theRect.x;
	stringY=theText->theRect.y+theText->theFont->ascent;

	stringLength=strlen(&(theText->theString[0]));
	currentIndex=0;
	while(currentIndex<stringLength&&(stringY<theText->theRect.y+(int)theText->theRect.h))	// stop drawing when we get below the bottom edge
	{
		lastIndex=currentIndex;
		while(currentIndex<stringLength&&theText->theString[currentIndex]!='\n'&&(XTextWidth(theText->theFont->theXFont,&(theText->theString[lastIndex]),(currentIndex+1)-lastIndex)<(int)theText->theRect.w))	// race up to the next new line, or until we have to wrap
		{
			currentIndex++;
		}
		XDrawString(xDisplay,xWindow,graphicsContext,stringX,stringY,&(theText->theString[lastIndex]),currentIndex-lastIndex);
		stringY+=theText->theFont->ascent+theText->theFont->descent;
		if(theText->theString[currentIndex]=='\n')
		{
			currentIndex++;															// skip over the new line
		}
	}
	XSetClipMask(xDisplay,graphicsContext,None);									// get rid of clip mask
	XDestroyRegion(textRegion);
}

/* unused code
static void RepositionStaticText(STATICTEXT *theText,EDITORRECT *newRect)
// change the position of theText within its parent
// be nice, and only invalidate it if it actually changes
{
	if((theText->theRect.x!=newRect->x)||(theText->theRect.y!=newRect->y)||(theText->theRect.w!=newRect->w)||(theText->theRect.h!=newRect->h))
	{
		InvalidateStaticText(theText);						// invalidate where it is now
		theText->theRect=*newRect;
		InvalidateStaticText(theText);						// invalidate where it is going
	}
}
*/

static void ResetStaticText(STATICTEXT *theText,char *newString)
// place a new static text string into theText
{
	strncpy(theText->theString,newString,MAXSTATICTEXTSTRING-1);
	theText->theString[MAXSTATICTEXTSTRING-1]='\0';
	InvalidateStaticText(theText);
}

static STATICTEXT *CreateStaticText(EDITORWINDOW *theWindow,STATICTEXTDESCRIPTOR *theDescription)
// create a static text item in theWindow, invalidate the area of theWindow
// where the text is to be placed
// return a pointer to it if successful
// SetError, and return NULL if not
{
	STATICTEXT
		*theText;

	if((theText=(STATICTEXT *)MNewPtr(sizeof(STATICTEXT))))			// create static text data structure
	{
		theText->parentWindow=theWindow;
		theText->theRect=theDescription->theRect;

		theText->foregroundColor=AllocColor(theDescription->foregroundColor);
		theText->backgroundColor=AllocColor(theDescription->backgroundColor);

		strncpy(theText->theString,theDescription->theString,MAXSTATICTEXTSTRING-1);
		theText->theString[MAXSTATICTEXTSTRING-1]='\0';
		if((theText->theFont=LoadFont(theDescription->fontName)))
		{
			InvalidateStaticText(theText);
			return(theText);
		}
		MDisposePtr(theText);
	}
	return(NULL);
}

static void DisposeStaticText(STATICTEXT *theText)
// unlink theText from its parent window, and delete it
// invalidate the area of the window where theText was
{
	InvalidateStaticText(theText);				// invalidate where it is now
	FreeFont(theText->theFont);					// free the font
	FreeColor(theText->backgroundColor);
	FreeColor(theText->foregroundColor);
	MDisposePtr(theText);
}

// dialog interface routines

static void DrawStaticTextItem(DIALOGITEM *theItem)
// draw the static text pointed to by theItem
{
	DrawStaticText((STATICTEXT *)theItem->itemStruct);
}

static void DisposeStaticTextItem(DIALOGITEM *theItem)
// dispose of the local static text structure pointed to by theItem
{
	DisposeStaticText((STATICTEXT *)theItem->itemStruct);
}

void ResetStaticTextItemText(DIALOGITEM *theItem,char *newString)
// reset the text of theItem to newString, invalidate the item
{
	ResetStaticText((STATICTEXT *)theItem->itemStruct,newString);
}

bool CreateStaticTextItem(DIALOGITEM *theItem,void *theDescription)
// create the static text, and fill in theItem
// if there is a problem, SetError, return false
{
	if((theItem->itemStruct=(void *)CreateStaticText(theItem->theDialog->parentWindow,(STATICTEXTDESCRIPTOR *)theDescription)))
	{
		theItem->disposeProc=DisposeStaticTextItem;
		theItem->drawProc=DrawStaticTextItem;
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
