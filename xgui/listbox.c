// List box handling
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

#define		LISTBOXTOPOFFSET	4				// pixel offsets within the list box that keep the text of the items from drawing too close to the edge
#define		LISTBOXLEFTOFFSET	4
#define		LISTBOXBOTTOMOFFSET	4

#define		ITEMSPACETOP		2				// pixels to add to each line of text
#define		ITEMSPACEBOTTOM		2

static void InvalidateListBox(LISTBOX *theListBox)
// invalidate the list box within its parent window
{
	InvalidateWindowRect(theListBox->parentWindow,&theListBox->theRect);
}

static void GetListBoxControlRects(LISTBOX *theListBox,EDITORRECT *borderRect,EDITORRECT *listContentRect,EDITORRECT *scrollBarRect)
// return the rectangles that enclose the various pieces of theListBox
// if the list box starts getting too small, certain rects may be returned with 0 width or
// height
{
	EDITORRECT
		insetRect;

	insetRect=(*borderRect)=theListBox->theRect;				// border rect is the whole thing

	insetRect.x+=LISTBOXBORDERWIDTH;							// inset from the border to the content area of the list box
	insetRect.y+=LISTBOXBORDERWIDTH;
	if(insetRect.w>2*LISTBOXBORDERWIDTH)
	{
		insetRect.w-=2*LISTBOXBORDERWIDTH;
	}
	else
	{
		insetRect.w=0;
	}
	if(insetRect.h>2*LISTBOXBORDERWIDTH)
	{
		insetRect.h-=2*LISTBOXBORDERWIDTH;
	}
	else
	{
		insetRect.h=0;
	}
	listContentRect->x=insetRect.x;
	listContentRect->y=insetRect.y;
	if(insetRect.w>SCROLLBARWIDTH)
	{
		listContentRect->w=insetRect.w-SCROLLBARWIDTH;
		if(theListBox->verticalScrollBarOnLeft)
		{
			scrollBarRect->x=insetRect.x;
			listContentRect->x+=SCROLLBARWIDTH;
		}
		else
		{
			scrollBarRect->x=insetRect.x+insetRect.w-SCROLLBARWIDTH;
		}
		scrollBarRect->w=SCROLLBARWIDTH;
	}
	else
	{
		listContentRect->w=0;
		scrollBarRect->x=insetRect.x;
		scrollBarRect->w=insetRect.w;
	}
	listContentRect->h=insetRect.h;

	scrollBarRect->y=insetRect.y;
	scrollBarRect->h=insetRect.h;
}

static void InvalidateListContents(LISTBOX *theListBox)
// invalidate the list area of theListBox
{
	EDITORRECT
		borderRect,
		listContentRect,
		scrollBarRect;

	GetListBoxControlRects(theListBox,&borderRect,&listContentRect,&scrollBarRect);
	InvalidateWindowRect(theListBox->parentWindow,&listContentRect);
}

static void GetListBoxElementRect(LISTBOX *theListBox,UINT32 theElement,EDITORRECT *theRect)
// get back the rectangle for theElement in theListBox
// NOTE: if theElement is invalid, or theElement is not displayed,
// an empty rectangle is returned
{
	EDITORRECT
		borderRect,
		listContentRect,
		scrollBarRect;

	if(theElement>=theListBox->topLine&&theElement<theListBox->numElements&&theElement<theListBox->topLine+theListBox->numLines)
	{
		GetListBoxControlRects(theListBox,&borderRect,&listContentRect,&scrollBarRect);

		theRect->x=listContentRect.x;
		theRect->y=listContentRect.y+LISTBOXTOPOFFSET+(theListBox->lineHeight)*(theElement-theListBox->topLine);
		theRect->w=listContentRect.w;
		theRect->h=theListBox->lineHeight;
	}
	else
	{
		theRect->x=theRect->y=0;
		theRect->w=theRect->h=0;
	}
}

static void RecalculateListBoxElementInfo(LISTBOX *theListBox)
// calculate some often needed constants
{
	EDITORRECT
		borderRect,
		listContentRect,
		scrollBarRect;

	GetListBoxControlRects(theListBox,&borderRect,&listContentRect,&scrollBarRect);

	theListBox->lineHeight=ITEMSPACETOP+theListBox->theFont->ascent+theListBox->theFont->descent+ITEMSPACEBOTTOM;

// find out how many lines can be displayed in the box given the size, and font
	if(listContentRect.h>(LISTBOXTOPOFFSET+LISTBOXBOTTOMOFFSET))
	{
		theListBox->numLines=(listContentRect.h-LISTBOXTOPOFFSET-LISTBOXBOTTOMOFFSET)/theListBox->lineHeight;
	}
	else
	{
		theListBox->numLines=0;
	}
}

static void SetListBoxTopLine(LISTBOX *theListBox,UINT32 newTopLine)
// Set the top line of theListBox to the given value (or the closest value that
// is legal)
// update the scroll bar, and all that...
{
	UINT32
		maxLines;

	if(newTopLine+theListBox->numLines<theListBox->numElements)
	{
		maxLines=theListBox->numElements-theListBox->numLines;
	}
	else
	{
		if(theListBox->numElements>theListBox->numLines)
		{
			newTopLine=theListBox->numElements-theListBox->numLines;
			maxLines=theListBox->numElements-theListBox->numLines;
		}
		else
		{
			newTopLine=0;
			maxLines=0;
		}
	}
	if(theListBox->topLine!=newTopLine)
	{
		theListBox->topLine=newTopLine;
		AdjustScrollBar(theListBox->theScrollBar,maxLines,newTopLine);
		InvalidateListContents(theListBox);				// invalidate contents
		UpdateEditorWindows();							// redraw anything that is needed
	}
}

static void HomeListBoxToCurrentElement(LISTBOX *theListBox)
// make sure the current element is visible within theListBox
{
	if(theListBox->currentElement<theListBox->topLine)
	{
		SetListBoxTopLine(theListBox,theListBox->currentElement);
	}
	else
	{
		if(theListBox->currentElement>=theListBox->topLine+theListBox->numLines)
		{
			SetListBoxTopLine(theListBox,theListBox->currentElement-theListBox->numLines+1);
		}
	}
}

static void ClearListBoxSelections(LISTBOX *theListBox)
// run through the elements of theListBox, clearing the
// selections, invalidate any items that were changed
{
	UINT32
		currentElement;
	EDITORRECT
		itemRect;

	for(currentElement=0;currentElement<theListBox->numElements;currentElement++)
	{
		if(theListBox->selectedElements[currentElement])	// unselect any that were selected
		{
			theListBox->selectedElements[currentElement]=false;
			GetListBoxElementRect(theListBox,currentElement,&itemRect);
			InvalidateWindowRect(theListBox->parentWindow,&itemRect);
		}
	}
}

static void SetListBoxSelections(LISTBOX *theListBox)
// run through the elements of theListBox, setting the
// selections, invalidate any items that were changed
{
	UINT32
		currentElement;
	EDITORRECT
		itemRect;

	for(currentElement=0;currentElement<theListBox->numElements;currentElement++)
	{
		if(!theListBox->selectedElements[currentElement])	// select any that were unselected
		{
			theListBox->selectedElements[currentElement]=true;
			GetListBoxElementRect(theListBox,currentElement,&itemRect);
			InvalidateWindowRect(theListBox->parentWindow,&itemRect);
		}
	}
}

static bool GetPointedToElement(LISTBOX *theListBox,INT32 x,INT32 y,UINT32 *theElement,bool strict)
// return theElement as the index to the item of theListBox that
// falls under x,y in the parent window
// NOTE: if strict is true, and no item falls under the point, return false
// if strict is false, and no item falls under the point, return the item
// nearest the point (if the point is below all visible items, return
// the item just below the last visible item, if the point is above
// all visible items, return the item just above the first visible
// item.) The non-strict mode is used while scrolling.
{
	EDITORRECT
		borderRect,
		listContentRect,
		scrollBarRect;
	UINT32
		localElement;

	if(theListBox->numElements)
	{
		GetListBoxControlRects(theListBox,&borderRect,&listContentRect,&scrollBarRect);
		if(PointInRECT(x,y,&listContentRect)||((!strict)&&(y>=listContentRect.y)&&(y<listContentRect.y+(int)listContentRect.h)))
		{
			if(y>=listContentRect.y+LISTBOXTOPOFFSET)
			{
				localElement=(y-listContentRect.y-LISTBOXTOPOFFSET)/theListBox->lineHeight;
			}
			else
			{
				localElement=0;
			}
			if(localElement>=theListBox->numLines)
			{
				if(theListBox->numLines)
				{
					localElement=theListBox->numLines-1;
				}
				else
				{
					return(false);		// no lines displayed in list!
				}
			}
			if(localElement+theListBox->topLine<theListBox->numElements)
			{
				*theElement=localElement+theListBox->topLine;
			}
			else
			{
				*theElement=theListBox->numElements-1;
			}
			return(true);
		}
		else
		{
			if(!strict)
			{
				if(y<listContentRect.y)
				{
					if(theListBox->topLine)
					{
						*theElement=theListBox->topLine-1;
					}
					else
					{
						*theElement=0;
					}
				}
				else
				{
					if(theListBox->topLine+theListBox->numLines<theListBox->numElements)
					{
						*theElement=theListBox->topLine+theListBox->numLines;
					}
					else
					{
						*theElement=theListBox->numElements-1;
					}
				}
				return(true);
			}
		}
	}
	return(false);
}

bool PointInListBox(LISTBOX *theListBox,INT32 x,INT32 y)
// return true if parent window relative x/y are in theListBox
// false if not
{
	return(PointInRECT(x,y,&theListBox->theRect));
}

void DrawListBox(LISTBOX *theListBox,bool hasFocus)
// redraw theListBox in its parent window, obey the invalid area of the parent's window
{
	WINDOWLISTELEMENT
		*theElement;
	Window
		xWindow;
	GC
		graphicsContext;
	GUICOLOR
		*foreground,
		*background;
	XRectangle
		regionBox;
	Region
		invalidRegion;
	EDITORRECT
		borderRect,
		listContentRect,
		scrollBarRect,
		itemRect;
	UINT32
		activeHeight,
		currentElement;

	theElement=(WINDOWLISTELEMENT *)theListBox->parentWindow->userData;			// get the x window information associated with this window
	xWindow=theElement->xWindow;												// get x window for this window
	graphicsContext=theElement->graphicsContext;								// get graphics context for this window

	GetListBoxControlRects(theListBox,&borderRect,&listContentRect,&scrollBarRect);

	invalidRegion=XCreateRegion();

	regionBox.x=borderRect.x;
	regionBox.y=borderRect.y;
	regionBox.width=borderRect.w;
	regionBox.height=borderRect.h;
	XUnionRectWithRegion(&regionBox,invalidRegion,invalidRegion);					// create a region which is the rectangle of the list box
	XIntersectRegion(invalidRegion,theElement->invalidRegion,invalidRegion);		// intersect with what is invalid in this window

	if(!XEmptyRegion(invalidRegion))												// see if it has an invalid area
	{
		XSetRegion(xDisplay,graphicsContext,invalidRegion);							// set clipping to what's invalid

		OutlineShadowRectangle(xWindow,graphicsContext,&borderRect,gray1,white,LISTBOXBORDERWIDTH);	// drop in border
		if(hasFocus)
		{
			OutlineShadowRectangle(xWindow,graphicsContext,&borderRect,black,black,LISTBOXBORDERWIDTH-1);	// drop in border
		}

		XSubtractRegion(invalidRegion,invalidRegion,invalidRegion);					// clear the region
		regionBox.x=listContentRect.x;
		regionBox.y=listContentRect.y;
		regionBox.width=listContentRect.w;
		regionBox.height=listContentRect.h;
		XUnionRectWithRegion(&regionBox,invalidRegion,invalidRegion);					// create a region which is the rectangle of the list box
		XIntersectRegion(invalidRegion,theElement->invalidRegion,invalidRegion);		// intersect with what is invalid in this window

		if(!XEmptyRegion(invalidRegion))												// see if it has an invalid area
		{
			XSetRegion(xDisplay,graphicsContext,invalidRegion);							// set clipping to what's invalid

			if(hasFocus)
			{
				foreground=theListBox->focusForegroundColor;
				background=theListBox->focusBackgroundColor;
			}
			else
			{
				foreground=theListBox->nofocusForegroundColor;
				background=theListBox->nofocusBackgroundColor;
			}

			XSetForeground(xDisplay,graphicsContext,background->theXPixel);

			// fill space at top and bottom where no text draws
			XFillRectangle(xDisplay,xWindow,graphicsContext,listContentRect.x,listContentRect.y,listContentRect.w,LISTBOXTOPOFFSET);
			if((theListBox->numElements-theListBox->topLine)>theListBox->numLines)
			{
				activeHeight=LISTBOXTOPOFFSET+(theListBox->numLines*theListBox->lineHeight);
			}
			else
			{
				activeHeight=LISTBOXTOPOFFSET+((theListBox->numElements-theListBox->topLine)*theListBox->lineHeight);
			}
			XFillRectangle(xDisplay,xWindow,graphicsContext,listContentRect.x,listContentRect.y+activeHeight,listContentRect.w,listContentRect.h-activeHeight);

			XSetForeground(xDisplay,graphicsContext,foreground->theXPixel);
			XSetFont(xDisplay,graphicsContext,theListBox->theFont->theXFont->fid);		// point to the current font for this text

			for(currentElement=theListBox->topLine;(currentElement<theListBox->numElements)&&((currentElement-theListBox->topLine)<theListBox->numLines);currentElement++)
			{
				GetListBoxElementRect(theListBox,currentElement,&itemRect);
				if(!theListBox->selectedElements[currentElement])						// see if this one is selected, if so, draw it as such
				{
					XSetForeground(xDisplay,graphicsContext,background->theXPixel);
					XFillRectangle(xDisplay,xWindow,graphicsContext,itemRect.x,itemRect.y,itemRect.w,itemRect.h);
					XSetForeground(xDisplay,graphicsContext,foreground->theXPixel);
				}
				else
				{
					XSetForeground(xDisplay,graphicsContext,foreground->theXPixel);
					XFillRectangle(xDisplay,xWindow,graphicsContext,itemRect.x,itemRect.y,itemRect.w,itemRect.h);
					XSetForeground(xDisplay,graphicsContext,background->theXPixel);
				}
				XDrawString(xDisplay,xWindow,graphicsContext,itemRect.x+LISTBOXLEFTOFFSET,itemRect.y+ITEMSPACETOP+theListBox->theFont->ascent,theListBox->listElements[currentElement],strlen(theListBox->listElements[currentElement]));
			}
		}
		XSetClipMask(xDisplay,graphicsContext,None);								// get rid of clip mask
	}
	XDestroyRegion(invalidRegion);													// get rid of invalid region
	DrawScrollBar(theListBox->theScrollBar);
}

static void TrackNormalListElements(LISTBOX *theListBox,XEvent *theEvent,UINT32 theElement)
// track list elements normally
{
	EDITORRECT
		itemRect;
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

	ClearListBoxSelections(theListBox);
	theListBox->selectedElements[theElement]=true;
	theListBox->currentElement=theElement;
	GetListBoxElementRect(theListBox,theElement,&itemRect);
	InvalidateWindowRect(theListBox->parentWindow,&itemRect);
	UpdateEditorWindows();
	while(StillDown(theEvent->xbutton.button,1))
	{
		if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theListBox->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
		{
			if(GetPointedToElement(theListBox,windowX,windowY,&theElement,false))
			{
				if(theElement!=theListBox->currentElement)
				{
					theListBox->selectedElements[theListBox->currentElement]=false;
					GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
					InvalidateWindowRect(theListBox->parentWindow,&itemRect);

					theListBox->selectedElements[theElement]=true;
					GetListBoxElementRect(theListBox,theElement,&itemRect);
					InvalidateWindowRect(theListBox->parentWindow,&itemRect);

					theListBox->currentElement=theElement;
					HomeListBoxToCurrentElement(theListBox);	// this will scroll the window if needed
					UpdateEditorWindows();
				}
			}
		}
	}
}

static void TrackShiftListElements(LISTBOX *theListBox,XEvent *theEvent,UINT32 theElement)
// track list elements in a mode which selects/deselects
{
	UINT32
		tempElement;
	EDITORRECT
		itemRect;
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
	bool
		fillMode;

	fillMode=theListBox->selectedElements[theElement]=theListBox->selectedElements[theElement]?false:true;
	theListBox->currentElement=theElement;
	GetListBoxElementRect(theListBox,theElement,&itemRect);
	InvalidateWindowRect(theListBox->parentWindow,&itemRect);
	UpdateEditorWindows();
	while(StillDown(theEvent->xbutton.button,1))
	{
		if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theListBox->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
		{
			if(GetPointedToElement(theListBox,windowX,windowY,&theElement,false))
			{
				if(theElement>theListBox->currentElement)	// fill from current to new
				{
					for(tempElement=theElement;tempElement>theListBox->currentElement;tempElement--)
					{
						theListBox->selectedElements[tempElement]=fillMode;
						GetListBoxElementRect(theListBox,tempElement,&itemRect);
						InvalidateWindowRect(theListBox->parentWindow,&itemRect);
					}
					theListBox->currentElement=theElement;
					HomeListBoxToCurrentElement(theListBox); // this will scroll the window if needed
					UpdateEditorWindows();
				}
				else
				{
					if(theElement<theListBox->currentElement)
					{
						for(tempElement=theElement;tempElement<theListBox->currentElement;tempElement++)
						{
							theListBox->selectedElements[tempElement]=fillMode;
							GetListBoxElementRect(theListBox,tempElement,&itemRect);
							InvalidateWindowRect(theListBox->parentWindow,&itemRect);
						}
						theListBox->currentElement=theElement;
						HomeListBoxToCurrentElement(theListBox); // this will scroll the window if needed
						UpdateEditorWindows();
					}
				}
			}
		}
	}
}

static void TrackHandListElements(LISTBOX *theListBox,XEvent *theEvent,UINT32 theElement)
// move around in the list box by tracking the mouse
{
	int
		initialY;
	int
		lineOffset;
	UINT32
		initialTopLine;
	UINT32
		newTopLine;
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

	initialY=theEvent->xbutton.y;
	initialTopLine=theListBox->topLine;

	while(StillDown(theEvent->xbutton.button,1))
	{
		if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theListBox->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
		{
			lineOffset=((initialY-windowY)/(int)theListBox->lineHeight);
			if(lineOffset<0)
			{
				if(initialTopLine>=(UINT32)-(lineOffset))
				{
					newTopLine=initialTopLine+lineOffset;
				}
				else
				{
					newTopLine=0;
				}
			}
			else
			{
				newTopLine=initialTopLine+lineOffset;
			}
			SetListBoxTopLine(theListBox,newTopLine);
		}
	}
}

static bool TrackListElements(LISTBOX *theListBox,XEvent *theEvent)
// Track the elements of the list
{
	UINT32
		theElement;
	EDITORRECT
		borderRect,
		listContentRect,
		scrollBarRect;
	UINT32
		numClicks,
		modifiers;

	GetListBoxControlRects(theListBox,&borderRect,&listContentRect,&scrollBarRect);
	if(PointInRECT(theEvent->xbutton.x,theEvent->xbutton.y,&listContentRect))
	{
		if(GetPointedToElement(theListBox,theEvent->xbutton.x,theEvent->xbutton.y,&theElement,true))
		{
			numClicks=HandleMultipleClicks(theEvent);
			XStateToEditorModifiers(theEvent->xbutton.state,numClicks,&modifiers);
			switch(theEvent->xbutton.button)
			{
				case Button1:
					if(!(modifiers&EEM_SHIFT))
					{
						TrackNormalListElements(theListBox,theEvent,theElement);
					}
					else
					{
						TrackShiftListElements(theListBox,theEvent,theElement);
					}
					if(numClicks&&theListBox->pressedProc)
					{
						theListBox->pressedProc(theListBox,theListBox->pressedProcParameters);
					}
					break;
				case Button2:
					TrackHandListElements(theListBox,theEvent,theElement);
					break;
				case Button3:
					TrackShiftListElements(theListBox,theEvent,theElement);
					break;
			}
			return(true);
		}
	}
	return(false);
}

bool TrackListBox(LISTBOX *theListBox,XEvent *theEvent)
// a button was pressed, see if it was in theListBox, if so, track it, and return true
// if it was not within the list box, return false
{
	if(!TrackScrollBar(theListBox->theScrollBar,theEvent))
	{
		if(!TrackListElements(theListBox,theEvent))
		{
			return(false);
		}
	}
	return(true);
}

static void ResetTypedCharacters(LISTBOX *theListBox)
// reset the typed character search buffer for theListBox
{
	theListBox->charBufferLength=0;						// number of characters in the charBuffer
	theListBox->charTimer=0;							// clear the typed character timer
}

static bool BetterMatch(LISTBOX *theListBox,UINT32 theElement,UINT32 previousElement)
// return true if theElement is a better match for the contents
// of currentBuffer, than previousElement
// NOTE: if there is a tie, previousElement wins
{
	int
		result;

	result=strcmp(theListBox->listElements[theElement],theListBox->listElements[previousElement]);
	if(result>0)
	{
		return(strcmp(theListBox->charBuffer,theListBox->listElements[previousElement])>0);
	}
	else
	{
		if(result<0)
		{
			return(strcmp(theListBox->charBuffer,theListBox->listElements[theElement])<=0);
		}
	}
	return(false);					// elements match, previous wins
}

static UINT32 SearchElements(LISTBOX *theListBox)
// search through the elements of theListBox, looking for
// the first, best match to charBuffer
// return the element located
// NOTE: if numElements is 0, this will return 0
{
	UINT32
		currentElement,
		bestMatchElement;

	bestMatchElement=0;
	for(currentElement=1;currentElement<theListBox->numElements;currentElement++)
	{
		if(BetterMatch(theListBox,currentElement,bestMatchElement))
		{
			bestMatchElement=currentElement;
		}
	}
	return(bestMatchElement);
}

static void CheckTypedChar(LISTBOX *theListBox,char theChar)
// See where theChar should be added to the search buffer,
// add it, reset timers as needed, and hunt for the contents
// of the search buffer in theListBox
// take the first, best match, set the current element to it,
// clear all other selections, and finally home the list to the current element
// This allows the user to type characters while a list box is open,
// locating the element of the list that is the best match to what
// has been typed.
// NOTE: this does not depend on the list to be sorted in any way,
// but the results are better if it is
{
	EDITORRECT
		itemRect;

	if(theListBox->charTimer+LISTCHARTIMEOUT<timer)
	{
		theListBox->charBufferLength=0;									// reset index to character buffer if timeout has expired
	}
	if(theListBox->charBufferLength+1<MAXLISTCHARBUFFER)
	{
		theListBox->charBuffer[theListBox->charBufferLength++]=theChar;	// if room, add the character to the buffer
		theListBox->charBuffer[theListBox->charBufferLength]='\0';		// terminate the string
	}
	if(theListBox->numElements)
	{
		ClearListBoxSelections(theListBox);
		theListBox->currentElement=SearchElements(theListBox);
		theListBox->selectedElements[theListBox->currentElement]=true;
		GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
		InvalidateWindowRect(theListBox->parentWindow,&itemRect);
		HomeListBoxToCurrentElement(theListBox); 		// this will scroll the window if needed
		UpdateEditorWindows();
	}
	theListBox->charTimer=timer;						// reset timer to current time
}

static void ScrollListBoxLower(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// scroll bar for this box is reporting a click
{
	LISTBOX
		*theListBox;

	theListBox=(LISTBOX *)parameters;
	if(theListBox->topLine>0)
	{
		SetListBoxTopLine(theListBox,theListBox->topLine-1);
	}
}

static void ScrollListBoxHigher(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// scroll bar for this box is reporting a click
{
	LISTBOX
		*theListBox;

	theListBox=(LISTBOX *)parameters;
	SetListBoxTopLine(theListBox,theListBox->topLine+1);
}

static void PageListBoxLower(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// scroll bar for this box is reporting a click
{
	LISTBOX
		*theListBox;

	theListBox=(LISTBOX *)parameters;
	if(theListBox->topLine>theListBox->numLines)
	{
		SetListBoxTopLine(theListBox,theListBox->topLine-theListBox->numLines);
	}
	else
	{
		SetListBoxTopLine(theListBox,0);
	}
}

static void PageListBoxHigher(SCROLLBAR *theScrollBar,bool repeating,void *parameters)
// scroll bar for this box is reporting a click
{
	LISTBOX
		*theListBox;

	theListBox=(LISTBOX *)parameters;
	SetListBoxTopLine(theListBox,theListBox->topLine+theListBox->numLines);
}

static void ListBoxThumbProc(SCROLLBAR *theScrollBar,UINT32 newValue,void *parameters)
// scroll bar for this box is reporting a click in the thumb
{
	LISTBOX
		*theListBox;

	theListBox=(LISTBOX *)parameters;
	SetListBoxTopLine(theListBox,newValue);
}

void RepositionListBox(LISTBOX *theListBox,EDITORRECT *newRect)
// change the position of theListBox within its parent
// be nice, and only invalidate it if it actually changes
{
	EDITORRECT
		borderRect,
		listContentRect,
		scrollBarRect;

	if((theListBox->theRect.x!=newRect->x)||(theListBox->theRect.y!=newRect->y)||(theListBox->theRect.w!=newRect->w)||(theListBox->theRect.h!=newRect->h))
	{
		InvalidateListBox(theListBox);					// invalidate where it is now
		theListBox->theRect=*newRect;					// copy over the new rectangle
		RecalculateListBoxElementInfo(theListBox);		// recalculate some stuff
		GetListBoxControlRects(theListBox,&borderRect,&listContentRect,&scrollBarRect);
		RepositionScrollBar(theListBox->theScrollBar,scrollBarRect.x,scrollBarRect.y,scrollBarRect.h,false);	// move scroll bar
		InvalidateListBox(theListBox);					// invalidate where it is going
	}
}

void HandleListBoxKeyEvent(LISTBOX *theListBox,XEvent *theEvent)
// A keyboard event is arriving for the list box, handle it here
{
	EDITORRECT
		itemRect;
	XKeyEvent
		localKeyEvent;									// make a local copy so we can strip annoying bits
	EDITORKEY
		theKey;

	if(theListBox->numElements)
	{
		localKeyEvent=*(XKeyEvent *)theEvent;				// copy the event
		localKeyEvent.state&=(~LockMask);					// remove this from consideration
		if(KeyEventToEditorKey((XEvent *)&localKeyEvent,&theKey))
		{
			if(theKey.isVirtual)							// see if virtual key or not
			{
				ResetTypedCharacters(theListBox);
				switch(theKey.keyCode)
				{
					case VVK_UPARROW:
						if(theListBox->selectedElements[theListBox->currentElement])
						{
							if(theListBox->currentElement)
							{
								ClearListBoxSelections(theListBox);
								theListBox->currentElement--;
								theListBox->selectedElements[theListBox->currentElement]=true;
								GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
								InvalidateWindowRect(theListBox->parentWindow,&itemRect);
							}
						}
						else
						{
							ClearListBoxSelections(theListBox);
							theListBox->selectedElements[theListBox->currentElement]=true;
							GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
							InvalidateWindowRect(theListBox->parentWindow,&itemRect);
						}
						HomeListBoxToCurrentElement(theListBox);
						break;
					case VVK_DOWNARROW:
						if(theListBox->selectedElements[theListBox->currentElement])
						{
							if(theListBox->currentElement+1<theListBox->numElements)
							{
								ClearListBoxSelections(theListBox);
								theListBox->currentElement++;
								theListBox->selectedElements[theListBox->currentElement]=true;
								GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
								InvalidateWindowRect(theListBox->parentWindow,&itemRect);
							}
						}
						else
						{
							ClearListBoxSelections(theListBox);
							theListBox->selectedElements[theListBox->currentElement]=true;
							GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
							InvalidateWindowRect(theListBox->parentWindow,&itemRect);
						}
						HomeListBoxToCurrentElement(theListBox);
						break;
					case VVK_PAGEUP:
						if(theListBox->selectedElements[theListBox->currentElement])
						{
							if(theListBox->currentElement)
							{
								if(theListBox->currentElement>theListBox->numLines)
								{
									theListBox->currentElement-=theListBox->numLines;
								}
								else
								{
									theListBox->currentElement=0;
								}
								ClearListBoxSelections(theListBox);
								theListBox->selectedElements[theListBox->currentElement]=true;
								GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
								InvalidateWindowRect(theListBox->parentWindow,&itemRect);
							}
						}
						else
						{
							ClearListBoxSelections(theListBox);
							theListBox->selectedElements[theListBox->currentElement]=true;
							GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
							InvalidateWindowRect(theListBox->parentWindow,&itemRect);
						}
						HomeListBoxToCurrentElement(theListBox);
						break;
					case VVK_PAGEDOWN:
						if(theListBox->selectedElements[theListBox->currentElement])
						{
							if(theListBox->currentElement+1<theListBox->numElements)
							{
								if(theListBox->currentElement+theListBox->numLines<theListBox->numElements)
								{
									theListBox->currentElement+=theListBox->numLines;
								}
								else
								{
									theListBox->currentElement=theListBox->numElements-1;
								}
								ClearListBoxSelections(theListBox);
								theListBox->selectedElements[theListBox->currentElement]=true;
								GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
								InvalidateWindowRect(theListBox->parentWindow,&itemRect);
							}
						}
						else
						{
							ClearListBoxSelections(theListBox);
							theListBox->selectedElements[theListBox->currentElement]=true;
							GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
							InvalidateWindowRect(theListBox->parentWindow,&itemRect);
						}
						HomeListBoxToCurrentElement(theListBox);
						break;
					case VVK_HOME:
						ClearListBoxSelections(theListBox);
						theListBox->currentElement=0;
						theListBox->selectedElements[theListBox->currentElement]=true;
						GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
						InvalidateWindowRect(theListBox->parentWindow,&itemRect);
						HomeListBoxToCurrentElement(theListBox);
						break;
					case VVK_END:
						ClearListBoxSelections(theListBox);
						if(theListBox->numElements)
						{
							theListBox->currentElement=theListBox->numElements-1;
							theListBox->selectedElements[theListBox->currentElement]=true;
						}
						else
						{
							theListBox->currentElement=0;
						}
						GetListBoxElementRect(theListBox,theListBox->currentElement,&itemRect);
						InvalidateWindowRect(theListBox->parentWindow,&itemRect);
						HomeListBoxToCurrentElement(theListBox);
						break;
				}
			}
			else
			{
				if(theKey.keyCode=='a'&&(theKey.modifiers&EEM_MOD0))	// select all
				{
					ResetTypedCharacters(theListBox);
					SetListBoxSelections(theListBox);
				}
				else
				{
					// asc to search on
					CheckTypedChar(theListBox,theKey.keyCode);
				}
			}
		}
	}
}

void ResetListBoxLists(LISTBOX *theListBox,UINT32 numElements,char **theList,bool *selectedElements)
// reset the list in theListBox to theList with selectedElements
{
	UINT32
		maxLines;

	theListBox->numElements=numElements;
	theListBox->listElements=theList;
	theListBox->selectedElements=selectedElements;

	ResetTypedCharacters(theListBox);
	theListBox->topLine=theListBox->currentElement=0;

	if(theListBox->numLines<theListBox->numElements)
	{
		maxLines=theListBox->numElements-theListBox->numLines;
	}
	else
	{
		if(theListBox->numElements>theListBox->numLines)
		{
			maxLines=theListBox->numElements-theListBox->numLines;
		}
		else
		{
			maxLines=0;
		}
	}
	AdjustScrollBar(theListBox->theScrollBar,maxLines,0);
	InvalidateListContents(theListBox);				// invalidate contents
}

LISTBOX *CreateListBox(EDITORWINDOW *theWindow,LISTBOXDESCRIPTOR *theDescription)
// create a list box in theWindow, invalidate the area of theWindow
// where the list box is to be placed
// return a pointer to it if successful
// SetError, return NULL if not
{
	LISTBOX
		*theListBox;
	EDITORRECT
		borderRect,
		listContentRect,
		scrollBarRect;
	SCROLLBARDESCRIPTOR
		scrollDescriptor;

	if((theListBox=(LISTBOX *)MNewPtr(sizeof(LISTBOX))))	// create list box data structure
	{
		theListBox->parentWindow=theWindow;
		theListBox->theRect=theDescription->theRect;

		theListBox->focusForegroundColor=AllocColor(theDescription->focusForegroundColor);
		theListBox->focusBackgroundColor=AllocColor(theDescription->focusBackgroundColor);
		theListBox->nofocusForegroundColor=AllocColor(theDescription->nofocusForegroundColor);
		theListBox->nofocusBackgroundColor=AllocColor(theDescription->nofocusBackgroundColor);

		theListBox->verticalScrollBarOnLeft=verticalScrollBarOnLeft;	// copy global peference
		theListBox->numElements=theDescription->numElements;
		theListBox->listElements=theDescription->listElements;
		theListBox->selectedElements=theDescription->selectedElements;

		ResetTypedCharacters(theListBox);					// reset keyboard input delays

		theListBox->pressedProc=theDescription->pressedProc;
		theListBox->pressedProcParameters=theDescription->pressedProcParameters;

		if((theListBox->theFont=LoadFont(theDescription->fontName)))
		{
			RecalculateListBoxElementInfo(theListBox);				// once we know the font, and list box size, derive some information
			theListBox->currentElement=theDescription->topLine;
			if(theDescription->topLine+theListBox->numLines<theListBox->numElements)
			{
				theListBox->topLine=theDescription->topLine;
			}
			else
			{
				if(theListBox->numLines<=theListBox->numElements)
				{
					theListBox->topLine=theListBox->numElements-theListBox->numLines;
				}
				else
				{
					theListBox->topLine=0;
				}
			}
			GetListBoxControlRects(theListBox,&borderRect,&listContentRect,&scrollBarRect);
			scrollDescriptor.x=scrollBarRect.x;
			scrollDescriptor.y=scrollBarRect.y;
			scrollDescriptor.length=scrollBarRect.h;
			scrollDescriptor.horizontal=false;

			if(theListBox->numElements>theListBox->numLines)
			{
				scrollDescriptor.thumbMax=theListBox->numElements-theListBox->numLines;
			}
			else
			{
				scrollDescriptor.thumbMax=0;
			}
			scrollDescriptor.thumbPosition=theListBox->topLine;
			scrollDescriptor.stepLowerProc=ScrollListBoxLower;
			scrollDescriptor.stepHigherProc=ScrollListBoxHigher;
			scrollDescriptor.pageLowerProc=PageListBoxLower;
			scrollDescriptor.pageHigherProc=PageListBoxHigher;
			scrollDescriptor.thumbProc=ListBoxThumbProc;
			scrollDescriptor.procParameters=(void *)theListBox;

			if((theListBox->theScrollBar=CreateScrollBar(theWindow,&scrollDescriptor)))
			{
				InvalidateListBox(theListBox);
				return(theListBox);
			}
			FreeFont(theListBox->theFont);					// free the font
		}
		MDisposePtr(theListBox);
	}
	return(NULL);
}

void DisposeListBox(LISTBOX *theListBox)
// unlink theListBox from its parent window, and delete it
// invalidate the area of the window where the list box was
{
	InvalidateListBox(theListBox);					// invalidate where it is now

	DisposeScrollBar(theListBox->theScrollBar);
	FreeFont(theListBox->theFont);					// free the font

	FreeColor(theListBox->focusForegroundColor);
	FreeColor(theListBox->focusBackgroundColor);
	FreeColor(theListBox->nofocusForegroundColor);
	FreeColor(theListBox->nofocusBackgroundColor);

	MDisposePtr(theListBox);
}

// dialog interface routines

static void DrawListBoxItem(DIALOGITEM *theItem)
// draw the list box pointed to by theItem
{
	LISTBOX
		*theListBox;

	theListBox=(LISTBOX *)theItem->itemStruct;
	DrawListBox(theListBox,(theItem==theItem->theDialog->focusItem));
}

static bool TrackListBoxItem(DIALOGITEM *theItem,XEvent *theEvent)
// attempt to track the list box pointed to by theItem
{
	if(PointInListBox((LISTBOX *)theItem->itemStruct,theEvent->xbutton.x,theEvent->xbutton.y))
	{
		DialogTakeFocus(theItem);				// set the focus
		return(TrackListBox((LISTBOX *)theItem->itemStruct,theEvent));
	}
	return(false);
}

static void DisposeListBoxItem(DIALOGITEM *theItem)
// dispose of the local list box structure pointed to by theItem
{
	DisposeListBox((LISTBOX *)theItem->itemStruct);
}

static void ListBoxItemFocusChange(DIALOGITEM *theItem)
// focus status has changed for list box
// handle it here
{
	InvalidateListBox((LISTBOX *)theItem->itemStruct);
}

static void ListBoxItemFocusKey(DIALOGITEM *theItem,XEvent *theEvent)
// give the key to the list box
{
	HandleListBoxKeyEvent((LISTBOX *)theItem->itemStruct,theEvent);
}

void ResetListBoxItemLists(DIALOGITEM *theItem,UINT32 numElements,char **theList,bool *selectedElements)
// place a new list into theItem
{
	ResetListBoxLists((LISTBOX *)theItem->itemStruct,numElements,theList,selectedElements);
}

bool CreateListBoxItem(DIALOGITEM *theItem,void *theDescriptor)
// create the list box, and fill in theItem
// if there is a problem, SetError, return false
{
	if((theItem->itemStruct=(void *)CreateListBox(theItem->theDialog->parentWindow,(LISTBOXDESCRIPTOR *)theDescriptor)))
	{
		theItem->disposeProc=DisposeListBoxItem;
		theItem->drawProc=DrawListBoxItem;
		theItem->trackProc=TrackListBoxItem;
		theItem->earlyKeyProc=NULL;
		theItem->wantFocus=true;
		theItem->focusChangeProc=ListBoxItemFocusChange;
		theItem->focusKeyProc=ListBoxItemFocusKey;
		theItem->focusPeriodicProc=NULL;
		return(true);
	}
	return(false);
}
