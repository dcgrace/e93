// Scroll bar handling
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

// Some terminology:
// low arrow is the arrow that attempts to move the thumb to lower values
// high arrow moves it higher

#define	SCROLLREPEATDELAY		10				// number of ticks to wait before repeating

static void GetScrollBarControlRects(SCROLLBAR *theScrollBar,EDITORRECT *lowArrowRect,EDITORRECT *highArrowRect,EDITORRECT *thumbRect,EDITORRECT *slideRect,EDITORRECT *pageLowRect,EDITORRECT *pageHighRect)
// return the rectangles that enclose the various pieces of theScrollBar
// if the scroll bar starts getting too small, certain rects may be returned with 0 width or
// height
{
	UINT32
		lowLength,
		highLength,
		thumbLength,
		thumbSpace;

	if(theScrollBar->length<2*ARROWLENGTH)			// problem fitting arrows?
	{
		lowLength=(theScrollBar->length+1)/2;		// get the length of the two arrows, and work backwards from there
		highLength=theScrollBar->length-lowLength;
	}
	else
	{
		lowLength=highLength=ARROWLENGTH;
	}
	if(theScrollBar->horizontal)
	{
		lowArrowRect->x=theScrollBar->x;
		lowArrowRect->w=lowLength;
		lowArrowRect->y=theScrollBar->y;
		lowArrowRect->h=SCROLLBARWIDTH;

		highArrowRect->w=highLength;
		highArrowRect->x=(theScrollBar->x+theScrollBar->length)-highLength;
		highArrowRect->y=theScrollBar->y;
		highArrowRect->h=SCROLLBARWIDTH;

		slideRect->x=lowArrowRect->x+lowArrowRect->w;
		slideRect->w=highArrowRect->x-slideRect->x;
		slideRect->y=theScrollBar->y;
		slideRect->h=SCROLLBARWIDTH;

		thumbLength=slideRect->w/(theScrollBar->thumbMax+1);

		if(thumbLength<THUMBLENGTH)
		{
			thumbLength=THUMBLENGTH;
		}

		if(thumbLength>=slideRect->w)
		{
			thumbRect->w=slideRect->w;
		}
		else
		{
			thumbRect->w=thumbLength;
		}
		thumbSpace=slideRect->w-thumbRect->w;
		if(theScrollBar->thumbMax)
		{
			thumbRect->x=slideRect->x+(thumbSpace*theScrollBar->thumbPosition)/theScrollBar->thumbMax;
		}
		else
		{
			thumbRect->x=slideRect->x;
		}
		thumbRect->y=theScrollBar->y;
		thumbRect->h=SCROLLBARWIDTH;

		pageLowRect->x=slideRect->x;
		pageLowRect->y=slideRect->y;
		pageLowRect->w=thumbRect->x-slideRect->x;
		pageLowRect->h=slideRect->h;

		pageHighRect->x=thumbRect->x+thumbRect->w;
		pageHighRect->y=slideRect->y;
		pageHighRect->w=slideRect->x+slideRect->w-pageHighRect->x;
		pageHighRect->h=slideRect->h;
	}
	else
	{
		lowArrowRect->x=theScrollBar->x;
		lowArrowRect->w=SCROLLBARWIDTH;
		lowArrowRect->y=theScrollBar->y;
		lowArrowRect->h=lowLength;

		highArrowRect->x=theScrollBar->x;
		highArrowRect->w=SCROLLBARWIDTH;
		highArrowRect->y=(theScrollBar->y+theScrollBar->length)-highLength;
		highArrowRect->h=highLength;

		slideRect->x=theScrollBar->x;
		slideRect->w=SCROLLBARWIDTH;
		slideRect->y=lowArrowRect->y+lowArrowRect->h;
		slideRect->h=highArrowRect->y-slideRect->y;

		thumbRect->x=theScrollBar->x;
		thumbRect->w=SCROLLBARWIDTH;

		thumbLength=slideRect->h/(theScrollBar->thumbMax+1);

		if(thumbLength<THUMBLENGTH)
		{
			thumbLength=THUMBLENGTH;
		}

		if(thumbLength>=slideRect->h)
		{
			thumbRect->h=slideRect->h;
		}
		else
		{
			thumbRect->h=thumbLength;
		}
		thumbSpace=slideRect->h-thumbRect->h;

		if(theScrollBar->thumbMax)
		{
			thumbRect->y=slideRect->y+(thumbSpace*theScrollBar->thumbPosition)/theScrollBar->thumbMax;
		}
		else
		{
			thumbRect->y=slideRect->y;
		}

		pageLowRect->x=slideRect->x;
		pageLowRect->y=slideRect->y;
		pageLowRect->w=slideRect->w;
		pageLowRect->h=thumbRect->y-slideRect->y;

		pageHighRect->x=slideRect->x;
		pageHighRect->y=thumbRect->y+thumbRect->h;
		pageHighRect->w=slideRect->w;
		pageHighRect->h=slideRect->y+slideRect->h-pageHighRect->y;
	}
}

bool PointInScrollBar(SCROLLBAR *theScrollBar,INT32 x,INT32 y)
// return true if parent window relative x/y are in theScrollBar
// false if not
{
	EDITORRECT
		theRect;

	theRect.x=theScrollBar->x;
	theRect.y=theScrollBar->y;
	if(theScrollBar->horizontal)
	{
		theRect.w=theScrollBar->length;
		theRect.h=SCROLLBARWIDTH;
	}
	else
	{
		theRect.w=SCROLLBARWIDTH;
		theRect.h=theScrollBar->length;
	}
	return(PointInRECT(x,y,&theRect));
}

static void InvalidateScrollBar(SCROLLBAR *theScrollBar)
// invalidate the scroll bar within its parent window
{
	EDITORRECT
		invalidRect;

	invalidRect.x=theScrollBar->x;
	invalidRect.y=theScrollBar->y;
	if(theScrollBar->horizontal)
	{
		invalidRect.w=theScrollBar->length;
		invalidRect.h=SCROLLBARWIDTH;
	}
	else
	{
		invalidRect.w=SCROLLBARWIDTH;
		invalidRect.h=theScrollBar->length;
	}
	InvalidateWindowRect(theScrollBar->parentWindow,&invalidRect);
}

static void DrawLeftArrow(Window xWindow,GC graphicsContext,EDITORRECT *theRect,bool highlight)
// draw a left arrow control into theRect of theWindow
{
	if(theRect->w&&theRect->h)
	{
		if(!highlight)
		{
			XSetForeground(xDisplay,graphicsContext,gray3->theXPixel);
		}
		else
		{
			XSetForeground(xDisplay,graphicsContext,white->theXPixel);
		}
		XSetBackground(xDisplay,graphicsContext,gray0->theXPixel);
		XCopyPlane(xDisplay,leftArrowPixmap,xWindow,graphicsContext,0,ARROWLENGTH-theRect->h,theRect->w,theRect->h,theRect->x,theRect->y,1);
		XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		XDrawRectangle(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y,theRect->w-1,theRect->h-1);
	}
}

static void DrawRightArrow(Window xWindow,GC graphicsContext,EDITORRECT *theRect,bool highlight)
// draw a right arrow control into theRect of theWindow
{
	if(theRect->w&&theRect->h)
	{
		if(!highlight)
		{
			XSetForeground(xDisplay,graphicsContext,gray3->theXPixel);
		}
		else
		{
			XSetForeground(xDisplay,graphicsContext,white->theXPixel);
		}
		XSetBackground(xDisplay,graphicsContext,gray0->theXPixel);
		XCopyPlane(xDisplay,rightArrowPixmap,xWindow,graphicsContext,0,ARROWLENGTH-theRect->h,theRect->w,theRect->h,theRect->x,theRect->y,1);
		XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		XDrawRectangle(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y,theRect->w-1,theRect->h-1);
	}
}

static void DrawUpArrow(Window xWindow,GC graphicsContext,EDITORRECT *theRect,bool highlight)
// draw an up arrow control into theRect of theWindow
{
	if(theRect->w&&theRect->h)
	{
		if(!highlight)
		{
			XSetForeground(xDisplay,graphicsContext,gray3->theXPixel);
		}
		else
		{
			XSetForeground(xDisplay,graphicsContext,white->theXPixel);
		}
		XSetBackground(xDisplay,graphicsContext,gray0->theXPixel);
		XCopyPlane(xDisplay,upArrowPixmap,xWindow,graphicsContext,0,ARROWLENGTH-theRect->h,theRect->w,theRect->h,theRect->x,theRect->y,1);
		XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		XDrawRectangle(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y,theRect->w-1,theRect->h-1);
	}
}

static void DrawDownArrow(Window xWindow,GC graphicsContext,EDITORRECT *theRect,bool highlight)
// draw a down arrow control into theRect of theWindow
{
	if(theRect->w&&theRect->h)
	{
		if(!highlight)
		{
			XSetForeground(xDisplay,graphicsContext,gray3->theXPixel);
		}
		else
		{
			XSetForeground(xDisplay,graphicsContext,white->theXPixel);
		}
		XSetBackground(xDisplay,graphicsContext,gray0->theXPixel);
		XCopyPlane(xDisplay,downArrowPixmap,xWindow,graphicsContext,0,ARROWLENGTH-theRect->h,theRect->w,theRect->h,theRect->x,theRect->y,1);
		XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		XDrawRectangle(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y,theRect->w-1,theRect->h-1);
	}
}

static void DrawPageIndicator(Window xWindow,GC graphicsContext,EDITORRECT *theRect,bool horizontal,bool highlight)
// draw a page control into theRect of theWindow
{
	if(theRect->w&&theRect->h)
	{
		if(!highlight)
		{
			XSetForeground(xDisplay,graphicsContext,gray1->theXPixel);
		}
		else
		{
			XSetForeground(xDisplay,graphicsContext,gray0->theXPixel);
		}
		if(horizontal)
		{
			if(theRect->h>1)
			{
				XFillRectangle(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y+1,theRect->w,theRect->h-2);	// fill the inset rectangle
			}
			XSetForeground(xDisplay,graphicsContext,black->theXPixel);
			XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y,theRect->x+theRect->w-1,theRect->y);
			XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y+theRect->h-1,theRect->x+theRect->w-1,theRect->y+theRect->h-1);
		}
		else
		{
			if(theRect->w>1)
			{
				XFillRectangle(xDisplay,xWindow,graphicsContext,theRect->x+1,theRect->y,theRect->w-2,theRect->h);	// fill the inset rectangle
			}
			XSetForeground(xDisplay,graphicsContext,black->theXPixel);
			XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y,theRect->x,theRect->y+theRect->h-1);
			XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x+theRect->w-1,theRect->y,theRect->x+theRect->w-1,theRect->y+theRect->h-1);
		}
	}
}

static void DrawThumb(Window xWindow,GC graphicsContext,EDITORRECT *theRect,bool highlight)
// draw a thumb control into theRect of theWindow
{
	if(theRect->w&&theRect->h)
	{
		if(theRect->w>1&&theRect->h>1)
		{
			if(!highlight)
			{
				XSetForeground(xDisplay,graphicsContext,gray3->theXPixel);
			}
			else
			{
				XSetForeground(xDisplay,graphicsContext,white->theXPixel);
			}
			XFillRectangle(xDisplay,xWindow,graphicsContext,theRect->x+1,theRect->y+1,theRect->w-2,theRect->h-2);	// fill the inset rectangle
		}
		XSetForeground(xDisplay,graphicsContext,black->theXPixel);
		XDrawRectangle(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y,theRect->w-1,theRect->h-1);			// draw the outline
		XSetForeground(xDisplay,graphicsContext,white->theXPixel);
		XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x+1,theRect->y+1,theRect->x+theRect->w-2,theRect->y+1);
		XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x+1,theRect->y+1,theRect->x+1,theRect->y+theRect->h-2);
	}
}

void DrawScrollBar(SCROLLBAR *theScrollBar)
// redraw theScrollBar in its parent window, obey the invalid area of the parent's window
{
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	GC
		graphicsContext;
	EDITORRECT
		lowArrowRect,
		highArrowRect,
		thumbRect,
		slideRect,
		pageLowRect,
		pageHighRect;

	theWindowElement=(WINDOWLISTELEMENT *)theScrollBar->parentWindow->userData;		// get the x window information associated with this window
	xWindow=theWindowElement->xWindow;												// get x window for this window
	graphicsContext=theWindowElement->graphicsContext;								// get graphics context for this window

	XSetRegion(xDisplay,graphicsContext,theWindowElement->invalidRegion);			// set clipping to what's invalid
	GetScrollBarControlRects(theScrollBar,&lowArrowRect,&highArrowRect,&thumbRect,&slideRect,&pageLowRect,&pageHighRect);
	if(theScrollBar->horizontal)
	{
		DrawLeftArrow(xWindow,graphicsContext,&lowArrowRect,theScrollBar->highlightLowArrow);
		DrawRightArrow(xWindow,graphicsContext,&highArrowRect,theScrollBar->highlightHighArrow);
	}
	else
	{
		DrawUpArrow(xWindow,graphicsContext,&lowArrowRect,theScrollBar->highlightLowArrow);
		DrawDownArrow(xWindow,graphicsContext,&highArrowRect,theScrollBar->highlightHighArrow);
	}
	DrawPageIndicator(xWindow,graphicsContext,&pageLowRect,theScrollBar->horizontal,theScrollBar->highlightLowPage);
	DrawPageIndicator(xWindow,graphicsContext,&pageHighRect,theScrollBar->horizontal,theScrollBar->highlightHighPage);
	DrawThumb(xWindow,graphicsContext,&thumbRect,theScrollBar->highlightThumb);
	XSetClipMask(xDisplay,graphicsContext,None);									// get rid of clip mask
}

static bool TrackLowArrow(SCROLLBAR *theScrollBar,XEvent *theEvent)
// track the low arrow of theScrollBar
{
	EDITORRECT
		lowArrowRect,
		highArrowRect,
		thumbRect,
		slideRect,
		pageLowRect,
		pageHighRect;
	UINT32
		initialTime;

	GetScrollBarControlRects(theScrollBar,&lowArrowRect,&highArrowRect,&thumbRect,&slideRect,&pageLowRect,&pageHighRect);
	if((theEvent->xbutton.button==Button1)&&PointInRECT(theEvent->xbutton.x,theEvent->xbutton.y,&lowArrowRect))
	{
		theScrollBar->highlightLowArrow=true;
		InvalidateScrollBar(theScrollBar);
		UpdateEditorWindows();												// redraw anything that is needed
		if(theScrollBar->stepLowerProc)
		{
			theScrollBar->stepLowerProc(theScrollBar,false,theScrollBar->procParameters);
		}
		initialTime=0;
		while(StillDown(Button1,1))
		{
			if(initialTime>SCROLLREPEATDELAY)
			{
				if(theScrollBar->stepLowerProc)
				{
					theScrollBar->stepLowerProc(theScrollBar,true,theScrollBar->procParameters);
				}
			}
			else
			{
				initialTime++;
			}
		}
		theScrollBar->highlightLowArrow=false;
		InvalidateScrollBar(theScrollBar);
		return(true);
	}
	return(false);
}

static bool TrackHighArrow(SCROLLBAR *theScrollBar,XEvent *theEvent)
// track the high arrow of theScrollBar
{
	EDITORRECT
		lowArrowRect,
		highArrowRect,
		thumbRect,
		slideRect,
		pageLowRect,
		pageHighRect;
	UINT32
		initialTime;

	GetScrollBarControlRects(theScrollBar,&lowArrowRect,&highArrowRect,&thumbRect,&slideRect,&pageLowRect,&pageHighRect);
	if((theEvent->xbutton.button==Button1)&&PointInRECT(theEvent->xbutton.x,theEvent->xbutton.y,&highArrowRect))
	{
		theScrollBar->highlightHighArrow=true;
		InvalidateScrollBar(theScrollBar);
		UpdateEditorWindows();												// redraw anything that is needed
		if(theScrollBar->stepHigherProc)
		{
			theScrollBar->stepHigherProc(theScrollBar,false,theScrollBar->procParameters);
		}
		initialTime=0;
		while(StillDown(Button1,1))
		{
			if(initialTime>SCROLLREPEATDELAY)
			{
				if(theScrollBar->stepHigherProc)
				{
					theScrollBar->stepHigherProc(theScrollBar,true,theScrollBar->procParameters);
				}
			}
			else
			{
				initialTime++;
			}
		}
		theScrollBar->highlightHighArrow=false;
		InvalidateScrollBar(theScrollBar);
		return(true);
	}
	return(false);
}

static bool TrackLowPage(SCROLLBAR *theScrollBar,XEvent *theEvent)
// track the low page of theScrollBar
{
	EDITORRECT
		lowArrowRect,
		highArrowRect,
		thumbRect,
		slideRect,
		pageLowRect,
		pageHighRect;
	UINT32
		initialTime;

	GetScrollBarControlRects(theScrollBar,&lowArrowRect,&highArrowRect,&thumbRect,&slideRect,&pageLowRect,&pageHighRect);
	if((theEvent->xbutton.button==Button1)&&PointInRECT(theEvent->xbutton.x,theEvent->xbutton.y,&pageLowRect))
	{
		theScrollBar->highlightLowPage=true;
		InvalidateScrollBar(theScrollBar);
		UpdateEditorWindows();												// redraw anything that is needed
		if(theScrollBar->pageLowerProc)
		{
			theScrollBar->pageLowerProc(theScrollBar,false,theScrollBar->procParameters);
		}
		initialTime=0;
		while(StillDown(Button1,1))
		{
			if(initialTime>SCROLLREPEATDELAY)
			{
				if(theScrollBar->pageLowerProc)
				{
					theScrollBar->pageLowerProc(theScrollBar,true,theScrollBar->procParameters);
				}
			}
			else
			{
				initialTime++;
			}
		}
		theScrollBar->highlightLowPage=false;
		InvalidateScrollBar(theScrollBar);
		return(true);
	}
	return(false);
}

static bool TrackHighPage(SCROLLBAR *theScrollBar,XEvent *theEvent)
// track the high page of theScrollBar
{
	EDITORRECT
		lowArrowRect,
		highArrowRect,
		thumbRect,
		slideRect,
		pageLowRect,
		pageHighRect;
	UINT32
		initialTime;

	GetScrollBarControlRects(theScrollBar,&lowArrowRect,&highArrowRect,&thumbRect,&slideRect,&pageLowRect,&pageHighRect);
	if((theEvent->xbutton.button==Button1)&&PointInRECT(theEvent->xbutton.x,theEvent->xbutton.y,&pageHighRect))
	{
		theScrollBar->highlightHighPage=true;
		InvalidateScrollBar(theScrollBar);
		UpdateEditorWindows();												// redraw anything that is needed
		if(theScrollBar->pageHigherProc)
		{
			theScrollBar->pageHigherProc(theScrollBar,false,theScrollBar->procParameters);
		}
		initialTime=0;
		while(StillDown(Button1,1))
		{
			if(initialTime>SCROLLREPEATDELAY)
			{
				if(theScrollBar->pageHigherProc)
				{
					theScrollBar->pageHigherProc(theScrollBar,true,theScrollBar->procParameters);
				}
			}
			else
			{
				initialTime++;
			}
		}
		theScrollBar->highlightHighPage=false;
		InvalidateScrollBar(theScrollBar);
		return(true);
	}
	return(false);
}

static UINT32 CalculateThumbValue(UINT32 currentPixel,UINT32 numPixels,UINT32 thumbMax)
// given a pixel position within a range of pixels, return the thumbPosition within
// thumbMax
{
	if(numPixels)
	{
		return(((currentPixel*thumbMax)+(numPixels/2))/numPixels);
	}
	else
	{
		return(0);
	}
}

static bool TrackMetaThumb(SCROLLBAR *theScrollBar,XEvent *theEvent)
// see if a button other that Button1 is down in the scroll bar, if so
// track the thumb of theScrollBar
// while tracking the thumb of theScrollBar, it is rude for the application to change
// the position of the bar on the screen,
// if this is done, the results will be unpredictable
{
	EDITORRECT
		lowArrowRect,
		highArrowRect,
		thumbRect,
		slideRect,
		pageLowRect,
		pageHighRect;
	Window
		root,
		child;
	int
		rootX,
		rootY,
		windowX,
		windowY,
		testIndex;
	unsigned int
		state;
	UINT32
		scrollablePixels,
		lastValue,
		newValue;

	GetScrollBarControlRects(theScrollBar,&lowArrowRect,&highArrowRect,&thumbRect,&slideRect,&pageLowRect,&pageHighRect);
	if((theEvent->xbutton.button!=Button1)&&PointInRECT(theEvent->xbutton.x,theEvent->xbutton.y,&slideRect))
	{
		theScrollBar->highlightThumb=true;
		InvalidateScrollBar(theScrollBar);
		UpdateEditorWindows();												// redraw anything that is needed
		if(theScrollBar->horizontal)
		{
			scrollablePixels=slideRect.w-thumbRect.w;
			lastValue=CalculateThumbValue(thumbRect.x-slideRect.x,scrollablePixels,theScrollBar->thumbMax);
			while(StillDown(theEvent->xbutton.button,1))
			{
				if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theScrollBar->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
				{
					testIndex=windowX-slideRect.x;				// get current pointer position
					testIndex-=thumbRect.w/2;					// add to initial offset
					if(testIndex<0)								// pin to range
					{
						testIndex=0;
					}
					if(testIndex>(int)scrollablePixels)
					{
						testIndex=scrollablePixels;
					}
					newValue=CalculateThumbValue((UINT32)testIndex,scrollablePixels,theScrollBar->thumbMax);
					if(newValue!=lastValue)
					{
						if(theScrollBar->thumbProc)
						{
							theScrollBar->thumbProc(theScrollBar,newValue,theScrollBar->procParameters);
						}
						lastValue=newValue;
					}
				}
			}
		}
		else
		{
			scrollablePixels=slideRect.h-thumbRect.h;
			lastValue=CalculateThumbValue(thumbRect.y-slideRect.y,scrollablePixels,theScrollBar->thumbMax);
			while(StillDown(theEvent->xbutton.button,1))
			{
				if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theScrollBar->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
				{
					testIndex=windowY-slideRect.y;		// get current pointer position
					testIndex-=thumbRect.h/2;			// add to initial offset
					if(testIndex<0)						// pin to range
					{
						testIndex=0;
					}
					if(testIndex>(int)scrollablePixels)
					{
						testIndex=scrollablePixels;
					}
					newValue=CalculateThumbValue((UINT32)testIndex,scrollablePixels,theScrollBar->thumbMax);
					if(newValue!=lastValue)
					{
						if(theScrollBar->thumbProc)
						{
							theScrollBar->thumbProc(theScrollBar,newValue,theScrollBar->procParameters);
						}
						lastValue=newValue;
					}
				}
			}
		}
		theScrollBar->highlightThumb=false;
		InvalidateScrollBar(theScrollBar);
		return(true);
	}
	return(false);
}

static bool TrackThumb(SCROLLBAR *theScrollBar,XEvent *theEvent)
// track the thumb of theScrollBar
// while tracking the thumb of theScrollBar, it is rude for the application to change
// the position of the bar on the screen,
// if this is done, the results will be unpredictable
{
	EDITORRECT
		lowArrowRect,
		highArrowRect,
		thumbRect,
		slideRect,
		pageLowRect,
		pageHighRect;
	Window
		root,
		child;
	int
		rootX,
		rootY,
		windowX,
		windowY,
		testIndex;
	unsigned int
		state;
	UINT32
		scrollablePixels,
		initialOffset,
		lastValue,
		newValue;

	GetScrollBarControlRects(theScrollBar,&lowArrowRect,&highArrowRect,&thumbRect,&slideRect,&pageLowRect,&pageHighRect);
	if((theEvent->xbutton.button==Button1)&&PointInRECT(theEvent->xbutton.x,theEvent->xbutton.y,&thumbRect))
	{
		theScrollBar->highlightThumb=true;
		InvalidateScrollBar(theScrollBar);
		UpdateEditorWindows();												// redraw anything that is needed
		if(theScrollBar->horizontal)
		{
			scrollablePixels=slideRect.w-thumbRect.w;
			initialOffset=thumbRect.x-slideRect.x;
			lastValue=CalculateThumbValue(initialOffset,scrollablePixels,theScrollBar->thumbMax);
			while(StillDown(Button1,1))
			{
				if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theScrollBar->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
				{
					testIndex=windowX-theEvent->xbutton.x;		// get current pointer position
					testIndex+=(INT32)initialOffset;			// add to initial offset
					if(testIndex<0)								// pin to range
					{
						testIndex=0;
					}
					if(testIndex>(int)scrollablePixels)
					{
						testIndex=scrollablePixels;
					}
					newValue=CalculateThumbValue((UINT32)testIndex,scrollablePixels,theScrollBar->thumbMax);
					if(newValue!=lastValue)
					{
						if(theScrollBar->thumbProc)
						{
							theScrollBar->thumbProc(theScrollBar,newValue,theScrollBar->procParameters);
						}
						lastValue=newValue;
					}
				}
			}
		}
		else
		{
			scrollablePixels=slideRect.h-thumbRect.h;
			initialOffset=thumbRect.y-slideRect.y;
			lastValue=CalculateThumbValue(initialOffset,scrollablePixels,theScrollBar->thumbMax);
			while(StillDown(Button1,1))
			{
				if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theScrollBar->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
				{
					testIndex=windowY-theEvent->xbutton.y;		// get current pointer position
					testIndex+=(INT32)initialOffset;			// add to initial offset
					if(testIndex<0)								// pin to range
					{
						testIndex=0;
					}
					if(testIndex>(int)scrollablePixels)
					{
						testIndex=scrollablePixels;
					}
					newValue=CalculateThumbValue((UINT32)testIndex,scrollablePixels,theScrollBar->thumbMax);
					if(newValue!=lastValue)
					{
						if(theScrollBar->thumbProc)
						{
							theScrollBar->thumbProc(theScrollBar,newValue,theScrollBar->procParameters);
						}
						lastValue=newValue;
					}
				}
			}
		}
		theScrollBar->highlightThumb=false;
		InvalidateScrollBar(theScrollBar);
		return(true);
	}
	return(false);
}

bool TrackScrollBar(SCROLLBAR *theScrollBar,XEvent *theEvent)
// a button was pressed, see if it was in theScrollBar, if so, track it, and return true
// if it was not within the scroll bar, return false
{
	if(!TrackLowArrow(theScrollBar,theEvent))
	{
		if(!TrackHighArrow(theScrollBar,theEvent))
		{
			if(!TrackMetaThumb(theScrollBar,theEvent))
			{
				if(!TrackLowPage(theScrollBar,theEvent))
				{
					if(!TrackHighPage(theScrollBar,theEvent))
					{
						if(!TrackThumb(theScrollBar,theEvent))
						{
							return(false);
						}
					}
				}
			}
		}
	}
	return(true);
}

void RepositionScrollBar(SCROLLBAR *theScrollBar,INT32 x,INT32 y,UINT32 length,bool horizontal)
// change the position of theScrollBar within its parent
// be nice, and only invalidate it if it actually changes
{
	if((theScrollBar->x!=x)||(theScrollBar->y!=y)||(theScrollBar->length!=length)||(theScrollBar->horizontal!=horizontal))
	{
		InvalidateScrollBar(theScrollBar);					// invalidate where it is now
		theScrollBar->x=x;
		theScrollBar->y=y;
		theScrollBar->length=length;
		theScrollBar->horizontal=horizontal;
		InvalidateScrollBar(theScrollBar);					// invalidate where it is going
	}
}

void AdjustScrollBar(SCROLLBAR *theScrollBar,UINT32 thumbMax,UINT32 thumbPosition)
// adjust the thumbMax, and position of theScrollBar
// be nice, and only invalidate it if it actually changes
{
	EDITORRECT
		lowArrowRect,
		highArrowRect,
		thumbRect,
		slideRect,
		pageLowRect,
		pageHighRect;

	if((theScrollBar->thumbMax!=thumbMax)||(theScrollBar->thumbPosition!=thumbPosition))
	{
		GetScrollBarControlRects(theScrollBar,&lowArrowRect,&highArrowRect,&thumbRect,&slideRect,&pageLowRect,&pageHighRect);	// invalidate where the thumb was
		InvalidateWindowRect(theScrollBar->parentWindow,&pageLowRect);
		InvalidateWindowRect(theScrollBar->parentWindow,&thumbRect);
		InvalidateWindowRect(theScrollBar->parentWindow,&pageHighRect);
		theScrollBar->thumbMax=thumbMax;
		theScrollBar->thumbPosition=thumbPosition;
		GetScrollBarControlRects(theScrollBar,&lowArrowRect,&highArrowRect,&thumbRect,&slideRect,&pageLowRect,&pageHighRect);	// invalidate where the thumb is going
		InvalidateWindowRect(theScrollBar->parentWindow,&pageLowRect);
		InvalidateWindowRect(theScrollBar->parentWindow,&thumbRect);
		InvalidateWindowRect(theScrollBar->parentWindow,&pageHighRect);
	}
}

SCROLLBAR *CreateScrollBar(EDITORWINDOW *theWindow,SCROLLBARDESCRIPTOR *theDescription)
// create a scroll bar in theWindow, invalidate the area of theWindow
// where the scroll bar is to be placed
// return a pointer to it if successful
// SetError, return NULL if not
{
	SCROLLBAR
		*theScrollBar;

	if((theScrollBar=(SCROLLBAR *)MNewPtr(sizeof(SCROLLBAR))))		// create scroll bar data structure
	{
		theScrollBar->parentWindow=theWindow;
		theScrollBar->x=theDescription->x;
		theScrollBar->y=theDescription->y;
		theScrollBar->length=theDescription->length;
		theScrollBar->horizontal=theDescription->horizontal;
		theScrollBar->thumbMax=theDescription->thumbMax;
		theScrollBar->thumbPosition=theDescription->thumbPosition;

		theScrollBar->highlightLowArrow=false;
		theScrollBar->highlightHighArrow=false;
		theScrollBar->highlightLowPage=false;
		theScrollBar->highlightHighPage=false;
		theScrollBar->highlightThumb=false;

		theScrollBar->thumbProc=theDescription->thumbProc;
		theScrollBar->stepLowerProc=theDescription->stepLowerProc;
		theScrollBar->stepHigherProc=theDescription->stepHigherProc;
		theScrollBar->pageLowerProc=theDescription->pageLowerProc;
		theScrollBar->pageHigherProc=theDescription->pageHigherProc;
		theScrollBar->procParameters=theDescription->procParameters;
		InvalidateScrollBar(theScrollBar);
	}
	return(theScrollBar);
}

void DisposeScrollBar(SCROLLBAR *theScrollBar)
// unlink theScrollBar from its parent window, and delete it
// invalidate the area of the window where the scroll bar was
{
	InvalidateScrollBar(theScrollBar);					// invalidate where it is now
	MDisposePtr(theScrollBar);
}
