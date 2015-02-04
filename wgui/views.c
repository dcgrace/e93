// Low level stuff needed to manage views
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

#define	STAGINGBUFFERSIZE		8192										// number of bytes used in the staging buffer (must be larger than STAGINGMAXADD)
#define	STAGINGMAXADD			256											// maximum number of bytes that will ever be added to the staging buffer in one call to GetCharacterEquiv

#define	ForceStyleIntoRange(theStyle)	((theStyle)&=(VIEWMAXSTYLES-1))		// call this to make sure that the given style is in range

static const char hexConv[]="0123456789ABCDEF";								// used for speedy hex conversions (for display of non-ascii codes)

// when text is being built, or widths are being calculated, these globals
// are used, they allow for much more efficient creation of text because things
// are not being passed all over for each character
static UINT8
	gceStagingBuffer[STAGINGBUFFERSIZE];									// this is where get character equivalent builds its output
static UINT32
	gceBufferIndex;															// offset into the staging buffer
static UINT32
	gceColumnNumber;														// running column number
static INT
	*gceWidthTable;															// width table to use while calculating positions
static UINT32
	gceTabSize;																// number of spaces to use per tab
static INT32
	gceRunningWidth;														// running width

static GUIFONT *GetFontForStyle(GUIVIEWSTYLE *theStyles,UINT32 desiredStyle)
// return the font that should be used for the given style
// if there is none specified, then use the default
{
	if(theStyles[desiredStyle].theFont)							// if there is a font loaded for this style, return it
	{
		return(theStyles[desiredStyle].theFont);
	}
	return(theStyles[0].theFont);								// otherwise, return the default font
}

static GUICOLOR *GetForegroundForStyle(GUIVIEWSTYLE *theStyles,UINT32 desiredStyle)
// return the foreground color that should be used for the given style
// if there is none specified, then use the default
{
	if(theStyles[desiredStyle].foregroundColor)					// if there is a color loaded for this style, return it
	{
		return(theStyles[desiredStyle].foregroundColor);
	}
	return(theStyles[0].foregroundColor);						// otherwise, return the default color
}

static GUICOLOR *GetBackgroundForStyle(GUIVIEWSTYLE *theStyles,UINT32 desiredStyle)
// return the background color that should be used for the given style
// if there is none specified, then use the default
{
	if(theStyles[desiredStyle].backgroundColor)					// if there is a color loaded for this style, return it
	{
		return(theStyles[desiredStyle].backgroundColor);
	}
	return(theStyles[0].backgroundColor);						// otherwise, return the default color
}


bool PointInRECT(int x, int y, RECT *bounds)
// wrap the M$ PtInRect function in a PointInRECT function so that
// we my have fewer differences between the XWindows code and ours
{
	POINT
		thePt;
		
	thePt.x=x;
	thePt.y=y;

	return PtInRect(bounds,thePt) != 0;
}


bool PointInView(EDITORVIEW *theView,INT32 x,INT32 y)
// return true if parent window relative x/y are in theView
// false if not
{
	return(PointInRECT(x,y,&(((GUIVIEWINFO *)theView->viewInfo)->bounds)));
}

/*
void LocalClickToViewClick(EDITORVIEW *theView,INT32 clickX,INT32 clickY,INT32 *xPosition,INT32 *lineNumber)
// given a point local to the window that contains theView,
// convert it into a view position
{
	GUIVIEWINFO
		*theViewInfo;
	INT32
		viewY;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	(*xPosition)=clickX-theViewInfo->textBounds.x;
	viewY=clickY-theViewInfo->textBounds.y;
	if(viewY<0)
	{
		viewY-=theViewInfo->lineHeight;							// account for cases about 0
	}
	(*lineNumber)=viewY/((INT32)(theViewInfo->lineHeight));
}

void HandleViewKeyEvent(EDITORVIEW *theView,XEvent *theEvent)
// handle an incoming key event for theView
{
	VIEWEVENT
		theViewEvent;
	EDITORKEY
		theKeyData;

	theViewEvent.theView=theView;							// point at the view
	theViewEvent.eventType=VET_KEYDOWN;						// let it know what type
	if(KeyEventToEditorKey(theEvent,&theKeyData))			// fill in the key record
	{
		theViewEvent.eventData=&theKeyData;
		HandleViewEvent(&theViewEvent);						// pass this off to high level handler
	}
}
*/

static bool GetCharacterEquiv(UINT8 theChar)
// put the correct characters into the gceStagingBuffer at gceBufferIndex
// update gceColumnNumber, gceBufferIndex, and gceRunningWidth
// if an end of line character is seen, return false, else return true
// NOTE: this is allowed to add at most STAGINGMAXADD characters to the staging buffer
// When called, there must be at least that many bytes available, or
// there will be trouble
{
	UINT32
		i,
		numTabSpaces;

	if(theChar>=' '&&gceWidthTable[theChar])				// make sure the font can display this, if not, display it as hex
	{
		gceStagingBuffer[gceBufferIndex++]=theChar;
		gceRunningWidth+=gceWidthTable[theChar];
		gceColumnNumber++;
	}
	else
	{
		switch(theChar)
		{
			case '\n':
				return(false);
				break;
			case '\t':
				if(gceTabSize)
				{
					numTabSpaces=gceTabSize-(gceColumnNumber%gceTabSize);
					for(i=0;i<numTabSpaces;i++)
					{
						gceStagingBuffer[gceBufferIndex++]=' ';			// space over to tab stop
					}
					gceRunningWidth+=gceWidthTable[' ']*numTabSpaces;	// add in space to running width
					gceColumnNumber+=numTabSpaces;						// push to correct column
				}
				else
				{
					gceRunningWidth+=gceWidthTable[gceStagingBuffer[gceBufferIndex++]='['];
					gceRunningWidth+=gceWidthTable[gceStagingBuffer[gceBufferIndex++]=hexConv[theChar>>4]];
					gceRunningWidth+=gceWidthTable[gceStagingBuffer[gceBufferIndex++]=hexConv[theChar&0x0F]];
					gceRunningWidth+=gceWidthTable[gceStagingBuffer[gceBufferIndex++]=']'];
					gceColumnNumber+=4;
				}
				break;
			default:
				// this is a little scary looking, but it is fast
				gceRunningWidth+=gceWidthTable[gceStagingBuffer[gceBufferIndex++]='['];
				gceRunningWidth+=gceWidthTable[gceStagingBuffer[gceBufferIndex++]=hexConv[theChar>>4]];
				gceRunningWidth+=gceWidthTable[gceStagingBuffer[gceBufferIndex++]=hexConv[theChar&0x0F]];
				gceRunningWidth+=gceWidthTable[gceStagingBuffer[gceBufferIndex++]=']'];
				gceColumnNumber+=4;
				break;
		}
	}
	return(true);
}

static bool PositionAtPixel(EDITORVIEW *theView,UINT32 linePosition,INT32 xPosition,UINT32 *charPosition,UINT32 *charColumn,INT32 *charStartPixel,UINT32 *charWidth)
// given linePosition which points to the character at the start of a line, and xPosition
// as a "pixel" position along the line, determine the offset to the character
// at xPosition, the pixel where that character begins, and the width of that character.
// if linePosition is past EOF, charPosition will be returned as linePosition,
// charColumn will be 0, charStartPixel will be VIEWLEFTMARGIN, and charWidth will be 0
// if the xPosition is past the end of the line, or end of the data, this will return false
{
	GUIVIEWINFO
		*theViewInfo;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		haveStyle,
		inLine,
		loopDone;
	INT32
		lastRunningWidth;
	UINT32
		lastColumnNumber;
	UINT32
		stylePosition,
		numStyleElements,
		currentStyle;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	PositionToChunkPosition(theView->parentBuffer->textUniverse,linePosition,&theChunk,&theOffset);

	if((haveStyle=GetStyleRange(theView->parentBuffer->styleUniverse,linePosition,&stylePosition,&numStyleElements,&currentStyle)))
	{
		ForceStyleIntoRange(currentStyle);
		stylePosition+=numStyleElements;					// style will change at this position
	}

	loopDone=false;
	inLine=true;

	gceTabSize=theViewInfo->tabSize;
	gceRunningWidth=lastRunningWidth=VIEWLEFTMARGIN;
	gceColumnNumber=lastColumnNumber=0;
	gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);

	while(!loopDone&&inLine&&theChunk)						// run out to the position where the text begins
	{
		if(haveStyle&&(linePosition>=stylePosition))		// time to update style?
		{
			if((haveStyle=GetStyleRange(theView->parentBuffer->styleUniverse,linePosition,&stylePosition,&numStyleElements,&currentStyle)))
			{
				ForceStyleIntoRange(currentStyle);
				stylePosition+=numStyleElements;			// style will change at this position
			}
			gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
		}
		gceBufferIndex=0;
		if((inLine=GetCharacterEquiv(theChunk->data[theOffset])))
		{
			if(gceRunningWidth>xPosition)
			{
				loopDone=true;
			}
			else
			{
				lastColumnNumber=gceColumnNumber;			// remember these
				lastRunningWidth=gceRunningWidth;
				linePosition++;
				theOffset++;
				if(theOffset>=theChunk->totalBytes)			// see if time for a new chunk
				{
					theChunk=theChunk->nextHeader;
					theOffset=0;
				}
			}
		}
	}
	*charPosition=linePosition;								// return the results
	*charColumn=lastColumnNumber;
	*charStartPixel=lastRunningWidth;
	*charWidth=gceRunningWidth-lastRunningWidth;

	return(theChunk&&inLine);
}

static void PixelAtPosition(EDITORVIEW *theView,UINT32 thePosition,INT32 *xPosition,bool limitMax)
// given thePosition within theView, determine the "pixel" offset to the start of the character
// of the line thePosition is in.
// if thePosition is passed in past the end of the buffer, this will return 0
// NOTE: if limitMax is true, this will abort as soon as the returned position
// grows larger than that passed in
{
	GUIVIEWINFO
		*theViewInfo;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		linePosition;
	UINT32
		theLine,
		theLineOffset;
	bool
		haveStyle;
	UINT32
		stylePosition,
		numStyleElements,
		currentStyle;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	PositionToLinePosition(theView->parentBuffer->textUniverse,thePosition,&theLine,&theLineOffset,&theChunk,&theOffset);

	linePosition=thePosition-theLineOffset;

	if((haveStyle=GetStyleRange(theView->parentBuffer->styleUniverse,linePosition,&stylePosition,&numStyleElements,&currentStyle)))
	{
		ForceStyleIntoRange(currentStyle);
		stylePosition+=numStyleElements;					// style will change at this position
	}

	gceTabSize=theViewInfo->tabSize;
	gceRunningWidth=VIEWLEFTMARGIN;
	gceColumnNumber=0;
	gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
	while((linePosition<thePosition)&&theChunk&&(!limitMax||(gceRunningWidth<*xPosition)))	// run out to the given position
	{
		if(haveStyle&&(linePosition>=stylePosition))		// time to update style?
		{
			if((haveStyle=GetStyleRange(theView->parentBuffer->styleUniverse,linePosition,&stylePosition,&numStyleElements,&currentStyle)))
			{
				ForceStyleIntoRange(currentStyle);
				stylePosition+=numStyleElements;			// style will change at this position
			}
			gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
		}
		gceBufferIndex=0;
		GetCharacterEquiv(theChunk->data[theOffset]);
		linePosition++;
		theOffset++;
		if(theOffset>=theChunk->totalBytes)					// see if time for a new chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	(*xPosition)=gceRunningWidth;
}

void GetEditorViewGraphicToTextPosition(EDITORVIEW *theView,UINT32 linePosition,INT32 xPosition,UINT32 *betweenOffset,UINT32 *charOffset)
// given linePosition which points to the character at the start of a line, and xPosition
// as a "pixel" position along the line, determine two offsets:
// betweenOffset is the offset of the character boundary closest to the xPosition,
// charOffset is the offset of the character closest to the xPosition
// if linePosition is past EOF, both offsets will be returned as 0
{
	UINT32
		charPosition;
	UINT32
		charColumn;
	INT32
		charStartPixel;
	UINT32
		charWidth;
	bool
		inLine;

	inLine=PositionAtPixel(theView,linePosition,xPosition,&charPosition,&charColumn,&charStartPixel,&charWidth);
	*betweenOffset=*charOffset=charPosition-linePosition;
	if(inLine&&(xPosition>(charStartPixel+((INT32)charWidth/2))))
	{
		(*betweenOffset)++;
	}
}

void GetEditorViewTextToGraphicPosition(EDITORVIEW *theView,UINT32 thePosition,INT32 *xPosition,bool limitMax,UINT32 *slopLeft,UINT32 *slopRight)
// given thePosition within theView, determine the "pixel" offset to the start of the character
// of the line thePosition is in.
// if thePosition is passed in past the end of the buffer, this will return the width of the last line
// NOTE: if limitMax is true, xPosition arrives at this function with a maximum 
// pixel position. This routine is then allowed to stop as soon as it
// sees the return result-slopLeft as greater than the passed value.
// NOTE: slopLeft and slopRight are used to allow fonts which have overhang
// into the previous or next character to be dealt with correctly.
// slopLeft tells how far the character at thePosition draws to the left of
// its given position.
// slopRight tells how far the character 1 to the left of thePosition draws
// to the right (past its character width).
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;
	*slopLeft=theViewInfo->maxLeftOverhang;
	*slopRight=theViewInfo->maxRightOverhang;
	if(limitMax)					// if limiting to pixel position, move out by max overhang
	{
		*xPosition+=*slopRight;
	}
	PixelAtPosition(theView,thePosition,xPosition,limitMax);
}

void GetEditorViewTextInfo(EDITORVIEW *theView,UINT32 *topLine,UINT32 *numLines,INT32 *leftPixel,UINT32 *numPixels)
// get information about the given view
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;
	*topLine=theViewInfo->topLine;
	*numLines=theViewInfo->numLines;
	*leftPixel=theViewInfo->leftPixel;
	*numPixels=theViewInfo->textBounds.left+theViewInfo->textBounds.right;
}

static void InvertViewCursor(HDC winHDC,EDITORVIEW *theView)
/*	invert the cursor wherever it happens to be in theView
	If this is called during a WM_PAINT message, winHDC has the DC to draw into, else
	it's set to NULL and we get a DC for the view to draw into.
 */
{
	GUIVIEWINFO
		*theViewInfo;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		theLine,
		theLineOffset;
	INT32
		xPosition,
		maxXPosition;
	RECT
		cursorRect;
	HDC
		hdc;
	HWND
		hwnd;
	WINDOWLISTELEMENT
		*theWindowElement;
	HPALETTE
		savePalette;
	int
		oldROP2;
	HRGN
		tmpRgn;
	UINT32
		theStyle;
	UINT32
		slopLeft,
		slopRight;
		
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			/* point at the information record for this view */

	PositionToLinePosition(theView->parentBuffer->textUniverse,GetSelectionCursorPosition(theView->parentBuffer->selectionUniverse),&theLine,&theLineOffset,&theChunk,&theOffset);
	if((theLine>=theViewInfo->topLine)&&(theLine<theViewInfo->topLine+theViewInfo->numLines))	/* make sure cursor is on the view */
	{
		maxXPosition=xPosition=theViewInfo->leftPixel+(theViewInfo->textBounds.right-theViewInfo->textBounds.left);
		GetEditorViewTextToGraphicPosition(theView,GetSelectionCursorPosition(theView->parentBuffer->selectionUniverse),&xPosition,true,&slopLeft,&slopRight);
		if((xPosition>=theViewInfo->leftPixel)&&(xPosition<maxXPosition))	/* see if on view horizontally */
		{
			theStyle=0;													// always paint the cursor in style 0
			cursorRect.left=theViewInfo->textBounds.left+(xPosition-theViewInfo->leftPixel);
			cursorRect.right=cursorRect.left+1;
			cursorRect.top=theViewInfo->textBounds.top+theViewInfo->lineHeight*(theLine-theViewInfo->topLine);
			cursorRect.bottom=cursorRect.top+theViewInfo->lineHeight;

			tmpRgn=CreateRectRgnIndirect(&cursorRect);


			if(winHDC)
			{
				oldROP2=SetROP2(winHDC,R2_XORPEN);
				FillRgn(winHDC,tmpRgn,theViewInfo->xorBrush);
				SetROP2(winHDC,oldROP2);
			}
			else
			{
				theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
				hwnd=theWindowElement->hwnd;
				hdc=GetDC(hwnd);
				savePalette=SelectPalette(hdc,editorPalette,FALSE);
				RealizePalette(hdc);
				oldROP2=SetROP2(hdc,R2_XORPEN);
				FillRgn(hdc,tmpRgn,theViewInfo->xorBrush);
				SetROP2(hdc,oldROP2);
				SelectPalette(hdc,savePalette,FALSE);
				RealizePalette(hdc);
				ReleaseDC(hwnd,hdc);
			}
			DeleteObject(tmpRgn);
		}
	}
}

void ResetCursorBlinkTime(EDITORVIEW *theView)
/*	Kill the timer that blinks the caret, and set it fresh, if the view is active.
 */
{
	GUIVIEWINFO
		*theViewInfo;
	HWND
		hwnd;
	WINDOWLISTELEMENT
		*theWindowElement;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			/* point at the information record for this view */
	if(theViewInfo->viewActive)
	{
		theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
		hwnd=theWindowElement->hwnd;
		SetCursorBlinkTime();
	}
}

static bool CursorShowing(EDITORVIEW *theView)
// returns true if the cursor is showing
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view
	if((IsSelectionEmpty(theView->parentBuffer->selectionUniverse))&&theViewInfo->cursorOn&&theViewInfo->blinkState&&theViewInfo->viewActive)
	{
		return(true);
	}
	return(false);
}

static void DrawViewCursor(HDC hdc,EDITORVIEW *theView)
// look at the cursor state for this view, and draw it as needed
// it will be drawn into the current port, and current clip region
{
	if(CursorShowing(theView))
	{
		InvertViewCursor(hdc,theView);								/* if it is showing, then draw it */
	}
}

static void AddLineSelectionRegion(EDITORVIEW *theView,HRGN theRegion,UINT32 currentPosition,INT32 yWindowOffset)
// Read bytes from the line of text at theChunk, theOffset
// Add selected pieces to theRegion
// NOTE: this must work as an exact mirror to DrawViewLine
// so that the region accurately represents what would be selected
// if the view was drawn
{
	GUIVIEWINFO
		*theViewInfo;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	INT32
		xWindowOffset,
		xWindowEndOffset;
	INT32
		skippedWidth;
	UINT32
		currentStyle,
		nextStyle;
	bool
		currentSelection,
		nextSelection;
	INT32
		scrollX;					// pixel offset in text where we need to start drawing
	INT32
		textXOffset;				// window relative pixel offset where text is being drawn
	INT32
		distanceToStart;
	RECT
		selectionRect;
	UINT32
		charWidth;
	HRGN
		tmpRgn;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	xWindowOffset=theViewInfo->textBounds.left;
	xWindowEndOffset=theViewInfo->textBounds.right;

	selectionRect.top=yWindowOffset;
	selectionRect.bottom=selectionRect.top+theViewInfo->lineHeight;

	scrollX=theViewInfo->leftPixel;							// get pixel offset in text where we need to start drawing

	PositionAtPixel(theView,currentPosition,scrollX,&currentPosition,&gceColumnNumber,&skippedWidth,&charWidth);

	PositionToChunkPosition(theView->parentBuffer->textUniverse,currentPosition,&theChunk,&theOffset);

	gceBufferIndex=0;										// begin at start of buffer
	gceRunningWidth=0;

	distanceToStart=skippedWidth-scrollX;

	if(distanceToStart>(xWindowEndOffset-xWindowOffset))	// do not go past the end of the draw area
	{
		distanceToStart=xWindowEndOffset-xWindowOffset;
	}

	textXOffset=xWindowOffset+distanceToStart;		// move the offset slightly back to beginning of first character which enters the view (or forward if view is scrolled negatively)

	currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);
	ForceStyleIntoRange(currentStyle);

	currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);
	gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
	
	while(theChunk&&((textXOffset+gceRunningWidth)<xWindowEndOffset)&&GetCharacterEquiv(theChunk->data[theOffset]))		// run through the text
	{
		nextStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition+1);
		ForceStyleIntoRange(nextStyle);
		nextSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition+1);

		if((currentStyle!=nextStyle)||(currentSelection!=nextSelection)||(gceBufferIndex>STAGINGBUFFERSIZE-STAGINGMAXADD))	// is something about to change? (if so, output what we have so far) (this will also output if the staging buffer is getting full)
		{
			if(currentSelection)
			{
				selectionRect.left=textXOffset;
				selectionRect.right=selectionRect.left+gceRunningWidth;
				tmpRgn=CreateRectRgnIndirect(&selectionRect);
				CombineRgn(theRegion,theRegion,tmpRgn,RGN_OR);
				DeleteObject(tmpRgn);
			}

			textXOffset+=gceRunningWidth;

			gceBufferIndex=0;								// begin at start of buffer
			gceRunningWidth=0;

			currentStyle=nextStyle;
			currentSelection=nextSelection;
			gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
		}

		currentPosition++;
		theOffset++;
		if(theOffset>=theChunk->totalBytes)					// push through end of chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	// ran off the end of the line, or the end of the universe
	if(gceBufferIndex)
	{
		if(currentSelection)
		{
			selectionRect.left=textXOffset;
			selectionRect.right=selectionRect.left+gceRunningWidth;
			tmpRgn=CreateRectRgnIndirect(&selectionRect);
			CombineRgn(theRegion,theRegion,tmpRgn,RGN_OR);
			DeleteObject(tmpRgn);
		}
		textXOffset+=gceRunningWidth;
	}
	
	if(textXOffset<xWindowEndOffset)							// do we need to go out to the end of the line?
	{
		if(textXOffset<xWindowOffset)							// did this end before the draw area began?
		{
			textXOffset=xWindowOffset;
		}
		currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);			// get style and selection information for newline (or off of end)
		ForceStyleIntoRange(currentStyle);
		currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);

		if(currentSelection)
		{
			selectionRect.left=textXOffset;
			selectionRect.right=selectionRect.left+(xWindowEndOffset-textXOffset);
			tmpRgn=CreateRectRgnIndirect(&selectionRect);
			CombineRgn(theRegion,theRegion,tmpRgn,RGN_OR);
			DeleteObject(tmpRgn);
		}
	}
}

static void CreateSelectionRegion(EDITORVIEW *theView,HRGN theRegion)
// run through the lines in the view and create a region which defines what
// is selected
{
	GUIVIEWINFO
		*theViewInfo;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset,
		currentPosition;
	UINT32
		i;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	gceTabSize=theViewInfo->tabSize;

	for(i=0;i<theViewInfo->numLines;i++)
	{
		LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine+i,&theChunk,&theOffset,&currentPosition);	// point to data for line
		AddLineSelectionRegion(theView,theRegion,currentPosition,theViewInfo->textBounds.top+i*theViewInfo->lineHeight);
	}
}

static void AddStyleDifferencesToRegion(EDITORVIEW *theView,HRGN theRegion,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 currentPosition,INT32 yWindowOffset)
// Run through the line, looking for a place where the current style differs
// from the style that was there before. Then, because a style change can cause
// a font change, and because a font change can move text left or right,
// invalidate from the start of the change to the right side of the view.
// NOTE: this must work as an exact mirror to DrawViewLine
{
	GUIVIEWINFO
		*theViewInfo;
	UINT32
		lineLength,
		maxPosition;
	UINT32
		oldStyleStart,
		newStyleStart;
	UINT32
		oldStyleElements,
		newStyleElements;
	UINT32
		oldStyle,
		newStyle;
	bool
		hadOldStyle,
		hadNewStyle;
	RECT
		styleRect;
	INT32
		xPosition;
	bool
		done;
	HRGN
		tmpRgn;
		
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	ChunkPositionToNextLine(theView->parentBuffer->textUniverse,theChunk,theOffset,&theChunk,&theOffset,&lineLength);
	maxPosition=currentPosition+lineLength;					// go no further than this looking for a change

	done=false;
	while(!done&&currentPosition<maxPosition)
	{
		hadOldStyle=GetStyleRange(theViewInfo->heldStyleUniverse,currentPosition,&oldStyleStart,&oldStyleElements,&oldStyle);
		hadNewStyle=GetStyleRange(theView->parentBuffer->styleUniverse,currentPosition,&newStyleStart,&newStyleElements,&newStyle);

		if(hadOldStyle||hadNewStyle)						// make sure there are some changes to look at
		{
			if(oldStyle!=newStyle)							// is there a change?
			{
				// currentPosition now contains the location of the style change, and the change is guaranteed to be on the line of interest
				xPosition=theViewInfo->leftPixel+(theViewInfo->textBounds.right-theViewInfo->textBounds.left);	// limit search to this distance
				PixelAtPosition(theView,currentPosition,&xPosition,true);
				xPosition-=theViewInfo->maxLeftOverhang;	// move back by the maximum overhang
				if(xPosition<(INT32)(theViewInfo->leftPixel+(theViewInfo->textBounds.right-theViewInfo->textBounds.left)))	// see if any work to do
				{
					if(xPosition<theViewInfo->leftPixel)	// does the change start off the left of view?
					{
						xPosition=theViewInfo->leftPixel;	// just fudge to the left
					}
					styleRect.left=theViewInfo->textBounds.left+(xPosition-theViewInfo->leftPixel);
					styleRect.top=yWindowOffset;
					styleRect.right=theViewInfo->maxRightOverhang+theViewInfo->maxLeftOverhang+styleRect.left+((theViewInfo->textBounds.right-theViewInfo->textBounds.left)-(xPosition-theViewInfo->leftPixel));
					styleRect.bottom=styleRect.top+theViewInfo->lineHeight;
					tmpRgn=CreateRectRgnIndirect(&styleRect);
					CombineRgn(theRegion,theRegion,tmpRgn,RGN_OR);
					DeleteObject(tmpRgn);
				}
				done=true;
			}
			else
			{
				// move forward to the nearest change
				if(hadOldStyle)			// see if we had an old style
				{
					currentPosition=oldStyleStart+oldStyleElements;
					if(hadNewStyle)		// if had new style too, see if it changes sooner
					{
						if(currentPosition>newStyleStart+newStyleElements)
						{
							currentPosition=newStyleStart+newStyleElements;
						}
					}
				}
				else					// no old style, so must have had new
				{
					currentPosition=newStyleStart+newStyleElements;
				}
			}
		}
		else
		{
			done=true;										// no styles defined out here, so no changes
		}
	}
}

static void DrawViewTextPieceWFill(HDC hdc,EDITORVIEW *theView,bool selected,UINT32 theStyle,INT32 xWindowOffset,UINT32 width,INT32 yWindowOffset)
// draw the piece of text in the staging buffer into the view in the given style and selection
{
	GUIVIEWINFO
		*theViewInfo;
	GUICOLOR
		*foreground,
		*background;
	HFONT
		oldFont;
	RECT
		fillClipRect;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view
	
	oldFont=(HFONT)SelectObject(hdc,(HGDIOBJ)GetFontForStyle(theViewInfo->viewStyles,theStyle)->font);
	if(selected)
	{
		if(!theViewInfo->selectionForegroundColor&&!theViewInfo->selectionBackgroundColor)		// if neither specified, invert
		{
			foreground=GetBackgroundForStyle(theViewInfo->viewStyles,theStyle);
			background=GetForegroundForStyle(theViewInfo->viewStyles,theStyle);
		}
		else	// otherwise, do not invert
		{
			if(theViewInfo->selectionForegroundColor)
			{
				foreground=theViewInfo->selectionForegroundColor;
			}
			else
			{
				foreground=GetForegroundForStyle(theViewInfo->viewStyles,theStyle);
			}

			if(theViewInfo->selectionBackgroundColor)
			{
				background=theViewInfo->selectionBackgroundColor;
			}
			else
			{
				background=GetBackgroundForStyle(theViewInfo->viewStyles,theStyle);
			}
		}
	}
	else
	{
		foreground=GetForegroundForStyle(theViewInfo->viewStyles,theStyle);
		background=GetBackgroundForStyle(theViewInfo->viewStyles,theStyle);
	}

	SetBkColor(hdc,background->colorPaletteIndex);
	SetTextColor(hdc,foreground->colorPaletteIndex);

//
// Setup a rect for ExtTextOut to fill and clip to when it draws the text
//
	fillClipRect.left=xWindowOffset;
	fillClipRect.right=fillClipRect.left+width;
	fillClipRect.top=yWindowOffset;
	fillClipRect.bottom=fillClipRect.top+theViewInfo->lineHeight;

	ExtTextOut(hdc,xWindowOffset,yWindowOffset+theViewInfo->lineAscent,ETO_OPAQUE|ETO_CLIPPED,&fillClipRect,(char *)gceStagingBuffer,gceBufferIndex,NULL);
	SelectObject(hdc,(HGDIOBJ)oldFont);
}

static void DrawViewTextPiece(HDC hdc,EDITORVIEW *theView,bool selected,UINT32 theStyle,INT32 xWindowOffset,UINT32 width,INT32 yWindowOffset)
// draw the piece of text in the staging buffer into the view in the given style and selection
{
	GUIVIEWINFO
		*theViewInfo;
	GUICOLOR
		*foreground,
		*background;
	HFONT
		oldFont;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view
	oldFont=(HFONT)SelectObject(hdc,(HGDIOBJ)GetFontForStyle(theViewInfo->viewStyles,theStyle)->font);
	if(selected)
	{
		if(!theViewInfo->selectionForegroundColor&&!theViewInfo->selectionBackgroundColor)		// if neither specified, invert
		{
			foreground=GetBackgroundForStyle(theViewInfo->viewStyles,theStyle);
			background=GetForegroundForStyle(theViewInfo->viewStyles,theStyle);
		}
		else	// otherwise, do not invert
		{
			if(theViewInfo->selectionForegroundColor)
			{
				foreground=theViewInfo->selectionForegroundColor;
			}
			else
			{
				foreground=GetForegroundForStyle(theViewInfo->viewStyles,theStyle);
			}

			if(theViewInfo->selectionBackgroundColor)
			{
				background=theViewInfo->selectionBackgroundColor;
			}
			else
			{
				background=GetBackgroundForStyle(theViewInfo->viewStyles,theStyle);
			}
		}
	}
	else
	{
		foreground=GetForegroundForStyle(theViewInfo->viewStyles,theStyle);
		background=GetBackgroundForStyle(theViewInfo->viewStyles,theStyle);
	}
	SetBkMode(hdc,TRANSPARENT);
	SetTextColor(hdc,foreground->colorPaletteIndex);

//
// Setup a rect for ExtTextOut to fill and clip to when it draws the text
//
	ExtTextOut(hdc,xWindowOffset,yWindowOffset+theViewInfo->lineAscent,0,NULL,(char *)gceStagingBuffer,gceBufferIndex,NULL);
	SelectObject(hdc,(HGDIOBJ)oldFont);
}

static void DrawViewLineWFill(HDC hdc,EDITORVIEW *theView,UINT32 currentPosition,INT32 xWindowOffset,INT32 xWindowEndOffset,INT32 yWindowOffset)
// Read bytes from the line of text at theChunk, theOffset
// Draw them onto the view
// This is responsible for drawing ALL of the pixels in the view for the line
// on which it was called (even if that line contains no text)
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	GUIVIEWINFO
		*theViewInfo;
	INT32
		skippedWidth;
	UINT32
		currentStyle,
		nextStyle;
	bool
		currentSelection,
		nextSelection;
	INT32
		scrollX;					// pixel offset in text where we need to start drawing
	INT32
		textXOffset;				// window relative pixel offset where text is being drawn
	INT32
		distanceToStart;
	UINT32
		charWidth;
	RECT
		tmpRect;
	HBRUSH
		fillBrush;
			
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	scrollX=theViewInfo->leftPixel+(xWindowOffset-theViewInfo->textBounds.left);	// get pixel offset in text where we need to start drawing

	PositionAtPixel(theView,currentPosition,scrollX,&currentPosition,&gceColumnNumber,&skippedWidth,&charWidth);

	PositionToChunkPosition(theView->parentBuffer->textUniverse,currentPosition,&theChunk,&theOffset);

	gceBufferIndex=0;								// begin at start of buffer
	gceRunningWidth=0;

	distanceToStart=skippedWidth-scrollX;

	if(distanceToStart>0)							// blank space before text starts?
	{
		if(distanceToStart>(xWindowEndOffset-xWindowOffset))	// do not go past the end of the draw area
		{
			distanceToStart=xWindowEndOffset-xWindowOffset;
		}
		tmpRect.left=xWindowOffset;
		tmpRect.right=tmpRect.left+distanceToStart;
		tmpRect.top=yWindowOffset;
		tmpRect.bottom=tmpRect.top+theViewInfo->lineHeight;
		FillRect(hdc,&tmpRect,theViewInfo->viewStyles[0].backgroundColor->colorBrush);
	}

	textXOffset=xWindowOffset+distanceToStart;		// move the offset slightly back to beginning of first character which enters the view (or forward if view is scrolled negatively)

	currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);
	ForceStyleIntoRange(currentStyle);
	currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);
	gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);

	while(theChunk&&((textXOffset+gceRunningWidth)<xWindowEndOffset)&&GetCharacterEquiv(theChunk->data[theOffset]))		// fill in all of the text needed
	{
		nextStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition+1);
		ForceStyleIntoRange(nextStyle);
		nextSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition+1);

		if((currentStyle!=nextStyle)||(currentSelection!=nextSelection)||(gceBufferIndex>STAGINGBUFFERSIZE-STAGINGMAXADD))	// is something about to change? (if so, output what we have so far) (this will also output if the staging buffer is getting full)
		{
			DrawViewTextPieceWFill(hdc,theView,currentSelection,currentStyle,textXOffset,gceRunningWidth,yWindowOffset);

			textXOffset+=gceRunningWidth;

			gceBufferIndex=0;								// begin at start of buffer
			gceRunningWidth=0;

			currentStyle=nextStyle;
			currentSelection=nextSelection;
			gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
		}

		currentPosition++;
		theOffset++;
		if(theOffset>=theChunk->totalBytes)					// push through end of chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	// ran off the end of the line, or the end of the universe
	if(gceBufferIndex)
	{
		DrawViewTextPieceWFill(hdc,theView,currentSelection,currentStyle,textXOffset,gceRunningWidth,yWindowOffset);
		textXOffset+=gceRunningWidth;
	}
	
	if(textXOffset<xWindowEndOffset)							// do we need to draw out to the end of the line?
	{
		if(textXOffset<xWindowOffset)							// did this end before the draw area began?
		{
			textXOffset=xWindowOffset;
		}
		currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);			// get style and selection information for newline (or off of end)
		ForceStyleIntoRange(currentStyle);
		currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);

		if(currentSelection)
		{

			if(!theViewInfo->selectionForegroundColor&&!theViewInfo->selectionBackgroundColor)		// if neither specified, invert
			{
				fillBrush=GetForegroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
			}
			else	// otherwise, do not invert
			{
				if(theViewInfo->selectionBackgroundColor)
				{
					fillBrush=theViewInfo->selectionBackgroundColor->colorBrush;
				}
				else
				{
					fillBrush=GetBackgroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
				}
			}
		}
		else
		{
			fillBrush=GetBackgroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
		}
		tmpRect.left=textXOffset;
		tmpRect.right=tmpRect.left+(xWindowEndOffset-textXOffset);
		tmpRect.top=yWindowOffset;
		tmpRect.bottom=tmpRect.top+theViewInfo->lineHeight;
		FillRect(hdc,&tmpRect,fillBrush);
	}
}

static void DrawViewLine(HDC hdc,EDITORVIEW *theView,UINT32 currentPosition,INT32 xWindowOffset,INT32 xWindowEndOffset,INT32 yWindowOffset)
// Read bytes from the line of text at theChunk, theOffset
// Draw them onto the view
// This is responsible for drawing ALL of the pixels in the view for the line
// on which it was called (even if that line contains no text)
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	GUIVIEWINFO
		*theViewInfo;
	INT32
		skippedWidth;
	UINT32
		currentStyle,
		nextStyle;
	bool
		currentSelection,
		nextSelection;
	INT32
		scrollX;					// pixel offset in text where we need to start drawing
	INT32
		textXOffset;				// window relative pixel offset where text is being drawn
	INT32
		distanceToStart;
	UINT32
		charWidth;
			
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	xWindowOffset-=theViewInfo->maxLeftOverhang;
	xWindowEndOffset+=theViewInfo->maxRightOverhang+theViewInfo->maxLeftOverhang;
	
	scrollX=theViewInfo->leftPixel+(xWindowOffset-theViewInfo->textBounds.left);	// get pixel offset in text where we need to start drawing

	PositionAtPixel(theView,currentPosition,scrollX,&currentPosition,&gceColumnNumber,&skippedWidth,&charWidth);

	PositionToChunkPosition(theView->parentBuffer->textUniverse,currentPosition,&theChunk,&theOffset);

	gceBufferIndex=0;								// begin at start of buffer
	gceRunningWidth=0;

	distanceToStart=skippedWidth-scrollX;

	if(distanceToStart>0)							// blank space before text starts?
	{
		if(distanceToStart>(xWindowEndOffset-xWindowOffset))	// do not go past the end of the draw area
		{
			distanceToStart=xWindowEndOffset-xWindowOffset;
		}
	}

	textXOffset=xWindowOffset+distanceToStart;		// move the offset slightly back to beginning of first character which enters the view (or forward if view is scrolled negatively)

	currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);
	ForceStyleIntoRange(currentStyle);
	currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);
	gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);

	while(theChunk&&((textXOffset+gceRunningWidth)<xWindowEndOffset)&&GetCharacterEquiv(theChunk->data[theOffset]))		// fill in all of the text needed
	{
		nextStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition+1);
		ForceStyleIntoRange(nextStyle);
		nextSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition+1);

		if((currentStyle!=nextStyle)||(currentSelection!=nextSelection)||(gceBufferIndex>STAGINGBUFFERSIZE-STAGINGMAXADD))	// is something about to change? (if so, output what we have so far) (this will also output if the staging buffer is getting full)
		{
			DrawViewTextPiece(hdc,theView,currentSelection,currentStyle,textXOffset,gceRunningWidth,yWindowOffset);

			textXOffset+=gceRunningWidth;

			gceBufferIndex=0;								// begin at start of buffer
			gceRunningWidth=0;

			currentStyle=nextStyle;
			currentSelection=nextSelection;
			gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
		}

		currentPosition++;
		theOffset++;
		if(theOffset>=theChunk->totalBytes)					// push through end of chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	// ran off the end of the line, or the end of the universe
	if(gceBufferIndex)
	{
		DrawViewTextPiece(hdc,theView,currentSelection,currentStyle,textXOffset,gceRunningWidth,yWindowOffset);
		textXOffset+=gceRunningWidth;
	}
}

static void FillViewLine(HDC hdc,EDITORVIEW *theView,UINT32 currentPosition,INT32 xWindowOffset,INT32 xWindowEndOffset,INT32 yWindowOffset)
// Read bytes from the line of text at theChunk, theOffset
// Draw them onto the view
// This is responsible for drawing ALL of the pixels in the view for the line
// on which it was called (even if that line contains no text)
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	GUIVIEWINFO
		*theViewInfo;
	INT32
		skippedWidth;
	UINT32
		currentStyle,
		nextStyle;
	bool
		currentSelection,
		nextSelection;
	INT32
		scrollX;					// pixel offset in text where we need to start drawing
	INT32
		textXOffset;				// window relative pixel offset where text is being drawn
	INT32
		distanceToStart;
	UINT32
		charWidth;
	RECT
		tmpRect;
	HBRUSH
		fillBrush;
			
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	scrollX=theViewInfo->leftPixel+(xWindowOffset-theViewInfo->textBounds.left);	// get pixel offset in text where we need to start drawing

	PositionAtPixel(theView,currentPosition,scrollX,&currentPosition,&gceColumnNumber,&skippedWidth,&charWidth);

	PositionToChunkPosition(theView->parentBuffer->textUniverse,currentPosition,&theChunk,&theOffset);

	gceBufferIndex=0;								// begin at start of buffer
	gceRunningWidth=0;

	distanceToStart=skippedWidth-scrollX;

	if(distanceToStart>0)							// blank space before text starts?
	{
		if(distanceToStart>(xWindowEndOffset-xWindowOffset))	// do not go past the end of the draw area
		{
			distanceToStart=xWindowEndOffset-xWindowOffset;
		}
		tmpRect.left=xWindowOffset;
		tmpRect.right=tmpRect.left+distanceToStart;
		tmpRect.top=yWindowOffset;
		tmpRect.bottom=tmpRect.top+theViewInfo->lineHeight;
		FillRect(hdc,&tmpRect,theViewInfo->viewStyles[0].backgroundColor->colorBrush);
	}

	textXOffset=xWindowOffset+distanceToStart;		// move the offset slightly back to beginning of first character which enters the view (or forward if view is scrolled negatively)

	currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);
	ForceStyleIntoRange(currentStyle);
	currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);
	gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);

	while(theChunk&&((textXOffset+gceRunningWidth)<xWindowEndOffset)&&GetCharacterEquiv(theChunk->data[theOffset]))		// fill in all of the text needed
	{
		nextStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition+1);
		ForceStyleIntoRange(nextStyle);
		nextSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition+1);

		if((currentStyle!=nextStyle)||(currentSelection!=nextSelection)||(gceBufferIndex>STAGINGBUFFERSIZE-STAGINGMAXADD))	// is something about to change? (if so, output what we have so far) (this will also output if the staging buffer is getting full)
		{
			if(currentSelection)
			{
				if(!theViewInfo->selectionForegroundColor&&!theViewInfo->selectionBackgroundColor)		// if neither specified, invert
				{
					fillBrush=GetForegroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
				}
				else	// otherwise, do not invert
				{
					if(theViewInfo->selectionBackgroundColor)
					{
						fillBrush=theViewInfo->selectionBackgroundColor->colorBrush;
					}
					else
					{
						fillBrush=GetBackgroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
					}
				}
			}
			else
			{
				fillBrush=GetBackgroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
			}
			tmpRect.left=textXOffset;
			tmpRect.right=tmpRect.left+gceRunningWidth;
			tmpRect.top=yWindowOffset;
			tmpRect.bottom=tmpRect.top+theViewInfo->lineHeight;
			FillRect(hdc,&tmpRect,fillBrush);

			textXOffset+=gceRunningWidth;

			gceBufferIndex=0;								// begin at start of buffer
			gceRunningWidth=0;

			currentStyle=nextStyle;
			currentSelection=nextSelection;
			gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
		}

		currentPosition++;
		theOffset++;
		if(theOffset>=theChunk->totalBytes)					// push through end of chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	// ran off the end of the line, or the end of the universe
	if(gceBufferIndex)
	{
		if(currentSelection)
		{
			if(!theViewInfo->selectionForegroundColor&&!theViewInfo->selectionBackgroundColor)		// if neither specified, invert
			{
				fillBrush=GetForegroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
			}
			else	// otherwise, do not invert
			{
				if(theViewInfo->selectionBackgroundColor)
				{
					fillBrush=theViewInfo->selectionBackgroundColor->colorBrush;
				}
				else
				{
					fillBrush=GetBackgroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
				}
			}
		}
		else
		{
			fillBrush=GetBackgroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
		}
		tmpRect.left=textXOffset;
		tmpRect.right=tmpRect.left+gceRunningWidth;
		tmpRect.top=yWindowOffset;
		tmpRect.bottom=tmpRect.top+theViewInfo->lineHeight;
		FillRect(hdc,&tmpRect,fillBrush);
		textXOffset+=gceRunningWidth;
	}
	
	if(textXOffset<xWindowEndOffset)							// do we need to draw out to the end of the line?
	{
		if(textXOffset<xWindowOffset)							// did this end before the draw area began?
		{
			textXOffset=xWindowOffset;
		}
		currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);			// get style and selection information for newline (or off of end)
		ForceStyleIntoRange(currentStyle);
		currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);

		if(currentSelection)
		{

			if(!theViewInfo->selectionForegroundColor&&!theViewInfo->selectionBackgroundColor)		// if neither specified, invert
			{
				fillBrush=GetForegroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
			}
			else	// otherwise, do not invert
			{
				if(theViewInfo->selectionBackgroundColor)
				{
					fillBrush=theViewInfo->selectionBackgroundColor->colorBrush;
				}
				else
				{
					fillBrush=GetBackgroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
				}
			}
		}
		else
		{
			fillBrush=GetBackgroundForStyle(theViewInfo->viewStyles,currentStyle)->colorBrush;
		}
		tmpRect.left=textXOffset;
		tmpRect.right=tmpRect.left+(xWindowEndOffset-textXOffset);
		tmpRect.top=yWindowOffset;
		tmpRect.bottom=tmpRect.top+theViewInfo->lineHeight;
		FillRect(hdc,&tmpRect,fillBrush);
	}
}

static void DrawViewLines(HDC hdc,HDC memHdc,EDITORVIEW *theView,HRGN updateRegion)
// draw the lines into theView, obey the invalid rect of the view
{
	GUIVIEWINFO
		*theViewInfo;
	UINT32
		topLineToDraw,
		numLinesToDraw;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset,
		currentPosition;
	UINT32
		i;
	RECT
		lineRect,
		updateRect;
	HRGN
		lineRgn;
				
	
	GetRgnBox(updateRegion,&updateRect);
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view
	gceTabSize=theViewInfo->tabSize;
	topLineToDraw=(updateRect.top-theViewInfo->textBounds.top)/theViewInfo->lineHeight;	// screen relative top line to draw
	numLinesToDraw=(((updateRect.bottom-theViewInfo->textBounds.top)+(theViewInfo->lineHeight-1)))/theViewInfo->lineHeight-topLineToDraw;	// get number of lines to draw
	for(i=topLineToDraw;i<topLineToDraw+numLinesToDraw;i++)
	{
	
		lineRect.left=updateRect.left;
		lineRect.right=updateRect.right;
		lineRect.top=theViewInfo->textBounds.top+i*theViewInfo->lineHeight;
		lineRect.bottom=lineRect.top+theViewInfo->lineHeight;
		lineRgn=CreateRectRgnIndirect(&lineRect);
		if(CombineRgn(lineRgn,lineRgn,updateRegion,RGN_AND)!=NULLREGION)
		{
			OffsetRgn(lineRgn,0,-(int)(theViewInfo->textBounds.top+i*theViewInfo->lineHeight));
			SelectClipRgn(memHdc,lineRgn);
			LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine+i,&theChunk,&theOffset,&currentPosition);	// point to data for this line
			if(theViewInfo->maxLeftOverhang || theViewInfo->maxRightOverhang)
			{
				FillViewLine(memHdc,theView,currentPosition,updateRect.left,updateRect.right,0);
				DrawViewLine(memHdc,theView,currentPosition,updateRect.left,updateRect.right,0);
				BitBlt(hdc,updateRect.left,theViewInfo->textBounds.top+i*theViewInfo->lineHeight,(updateRect.right-updateRect.left),theViewInfo->lineHeight,memHdc,updateRect.left,0,SRCCOPY);
			}
			else
			{
				DrawViewLineWFill(memHdc,theView,currentPosition,updateRect.left,updateRect.right,0);
				BitBlt(hdc,updateRect.left,theViewInfo->textBounds.top+i*theViewInfo->lineHeight,(updateRect.right-updateRect.left),theViewInfo->lineHeight,memHdc,updateRect.left,0,SRCCOPY);
			}
		}
		DeleteObject(lineRgn);
	}
}

void DrawView(HWND hwnd,HDC hdc,HRGN updateRegion)
/* draw the given window in its parent window
 * also handle highlighting of text
 */
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	HRGN
		visiRgn;
	RECT
		fRect,
		updateRect;
	HDC
		memHdc;
	HBITMAP
		oldBitmap;
				
   	theWindow=(EDITORWINDOW *)GetWindowLong(hwnd,EDITORWINDOW_DATA_OFFSET);
	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */

	GetRgnBox(updateRegion,&updateRect);					// get the bounds rect for the update region
	if(updateRect.left<theViewInfo->textBounds.left)		// do we need to fill the left edge
	{
		fRect.left=theViewInfo->bounds.left;
		fRect.right=theViewInfo->textBounds.left-theViewInfo->bounds.left;
		fRect.top=theViewInfo->bounds.top;
		fRect.bottom=theViewInfo->bounds.bottom;
		FillRect(hdc,&fRect,theViewInfo->viewStyles[0].backgroundColor->colorBrush);
	}

	if(updateRect.top<theViewInfo->textBounds.top)			// do we need to fill the top
	{
		fRect.left=theViewInfo->bounds.left;
		fRect.right=theViewInfo->bounds.right;
		fRect.top=theViewInfo->bounds.top;
		fRect.bottom=theViewInfo->textBounds.top;
		FillRect(hdc,&fRect,theViewInfo->viewStyles[0].backgroundColor->colorBrush);
	}

	if(updateRect.bottom>theViewInfo->textBounds.bottom)	// do we need to fill the bottom
	{
		fRect.left=theViewInfo->bounds.left;
		fRect.right=theViewInfo->bounds.right;
		fRect.top=theViewInfo->textBounds.top+(theViewInfo->numLines*theViewInfo->lineHeight);
		fRect.bottom=theViewInfo->bounds.bottom;
		FillRect(hdc,&fRect,theViewInfo->viewStyles[0].backgroundColor->colorBrush);
	}

	visiRgn=CreateRectRgnIndirect(&theViewInfo->textBounds);
	if(CombineRgn(visiRgn,visiRgn,updateRegion,RGN_AND)!=NULLREGION)
	{
		SelectClipRgn(hdc,visiRgn);
		memHdc=CreateCompatibleDC(hdc);
		oldBitmap=(HBITMAP)SelectObject(memHdc,(HGDIOBJ)theViewInfo->lineHBitmap);
		SetMapMode(memHdc,MM_TEXT);
		SetTextAlign(memHdc,TA_LEFT|TA_BASELINE);
		SelectPalette(memHdc,editorPalette,FALSE);

		DrawViewLines(hdc,memHdc,theView,visiRgn);
		DrawViewCursor(hdc,theView);
		SelectObject(memHdc,(HGDIOBJ)oldBitmap);
		DeleteDC(memHdc);
	}
	DeleteObject(visiRgn);
}

void RecalculateViewFontInfo(EDITORVIEW *theView)
// recalculate the cached information for theView
// from its current size and set of styles
{
	INT32
		viewHeight;
	GUIVIEWINFO
		*theViewInfo;
	UINT32
		maxDescent;
	UINT32
		i;
		
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	
	maxDescent=0;
	theViewInfo->lineHeight=0;
	theViewInfo->lineAscent=0;
	theViewInfo->maxLeftOverhang=0;
	theViewInfo->maxRightOverhang=0;
	theViewInfo->minCharWidth=UINT_MAX;
	for(i=0;i<VIEWMAXSTYLES;i++)
	{
		if(theViewInfo->viewStyles[i].theFont)					// see if a font is assigned for this style
		{
			if(theViewInfo->lineAscent<(theViewInfo->viewStyles[i].theFont->ascent))
			{
				theViewInfo->lineAscent=theViewInfo->viewStyles[i].theFont->ascent;
			}
			if(maxDescent<(theViewInfo->viewStyles[i].theFont->descent))
			{
				maxDescent=theViewInfo->viewStyles[i].theFont->descent;
			}
			if(theViewInfo->maxLeftOverhang<theViewInfo->viewStyles[i].theFont->maxLeftOverhang)
			{
				theViewInfo->maxLeftOverhang=theViewInfo->viewStyles[i].theFont->maxLeftOverhang;
			}
			if(theViewInfo->maxRightOverhang<theViewInfo->viewStyles[i].theFont->maxRightOverhang)
			{
				theViewInfo->maxRightOverhang=theViewInfo->viewStyles[i].theFont->maxRightOverhang;
			}
		}
	}
	theViewInfo->lineHeight=theViewInfo->lineAscent+maxDescent;

	if(theViewInfo->lineHeight<=0)
	{
		theViewInfo->lineHeight=1;								// lines are minimally one pixel tall (so we do not get an infinite number of lines on the view)
	}

	theViewInfo->textBounds=theViewInfo->bounds;				// start with copy of the view bounds
	theViewInfo->textBounds.top+=VIEWTOPMARGIN;					// leave a small margin at the top

	if((theViewInfo->bounds.bottom-theViewInfo->bounds.top)>(VIEWTOPMARGIN+VIEWBOTTOMMARGIN))
	{
		viewHeight=(theViewInfo->bounds.bottom-theViewInfo->bounds.top)-VIEWTOPMARGIN-VIEWBOTTOMMARGIN;	// find out how much can be used for the text
		theViewInfo->numLines=viewHeight/theViewInfo->lineHeight;	// get integer number of lines in this view
		theViewInfo->textBounds.bottom=theViewInfo->textBounds.top+(theViewInfo->numLines*theViewInfo->lineHeight);	// alter the textBounds to be the exact rectangle to hold the text
	}
	else
	{
		theViewInfo->numLines=0;
		theViewInfo->textBounds.bottom=theViewInfo->textBounds.top;							// no room to draw text
	}
	
	if(theViewInfo->lineHBitmap)
	{
		DeleteObject(theViewInfo->lineHBitmap);
	}
	HDC
		hdc=CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);
		
	theViewInfo->lineHBitmap=CreateCompatibleBitmap(hdc,GetDeviceCaps(hdc,HORZRES),theViewInfo->lineHeight);
	DeleteDC(hdc);
}

static void UnSetViewStyleFont(EDITORVIEW *theView,UINT32 theStyle)
// free the font currently in use by theStyle of theView
// NOTE: it is a bad idea to call this for the default style until the window
// will no longer be used, since the default is used when the given style
// is not available
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->viewStyles[theStyle].theFont)
	{
		FreeFont(theViewInfo->viewStyles[theStyle].theFont);
		theViewInfo->viewStyles[theStyle].theFont=NULL;			// mark this font as unused
	}
}

static bool SetViewStyleFont(EDITORVIEW *theView,UINT32 theStyle,char *fontName)
// set the font used to draw theView
// NOTE: it is necessary to call RecalculateViewFontInfo
// after calling this (that is left to the caller, since he may need to make
// many calls to this in a row and the recalculation can be left until after
// the last call)
{
	GUIVIEWINFO
		*theViewInfo;
	GUIFONT
		*newFont;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if((newFont=LoadFont(fontName)))
	{
		UnSetViewStyleFont(theView,theStyle);					// get rid of old font that was there (if any)
		theViewInfo->viewStyles[theStyle].theFont=newFont;
		return(true);
	}
	return(false);
}

static void UnSetViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle)
// free the foreground color currently in use by theStyle of theView
// NOTE: it is a bad idea to call this for the default style until the window
// will no longer be used, since the default is used when the given style
// is not available
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->viewStyles[theStyle].foregroundColor)
	{
		FreeColor(theViewInfo->viewStyles[theStyle].foregroundColor);
		theViewInfo->viewStyles[theStyle].foregroundColor=NULL;
	}
}

static bool SetViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR foreground)
// set the foreground color used to draw theStyle of theView
// If this fails, it will leave the style unchanged, and return false
{
	GUIVIEWINFO
		*theViewInfo;
	GUICOLOR
		*newColor;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if((newColor=AllocColor(foreground)))
	{
		UnSetViewStyleForegroundColor(theView,theStyle);
		theViewInfo->viewStyles[theStyle].foregroundColor=newColor;
		return(true);
	}
	return(false);
}

static void UnSetViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle)
// free the background color currently in use by theStyle of theView
// NOTE: it is a bad idea to call this for the default style until the window
// will no longer be used, since the default is used when the given style
// is not available
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->viewStyles[theStyle].backgroundColor)
	{
		FreeColor(theViewInfo->viewStyles[theStyle].backgroundColor);
		theViewInfo->viewStyles[theStyle].backgroundColor=NULL;
	}
}

static bool SetViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR background)
// set the background color used to draw theStyle of theView
// If this fails, it will leave the style unchanged, and return false
{
	GUIVIEWINFO
		*theViewInfo;
	GUICOLOR
		*newColor;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if((newColor=AllocColor(background)))
	{
		UnSetViewStyleBackgroundColor(theView,theStyle);
		theViewInfo->viewStyles[theStyle].backgroundColor=newColor;
		return(true);
	}
	return(false);
}

static void UnSetViewSelectionForegroundColor(EDITORVIEW *theView)
// free the foreground color used to draw the selection in theView
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->selectionForegroundColor)
	{
		FreeColor(theViewInfo->selectionForegroundColor);
		theViewInfo->selectionForegroundColor=NULL;				// remember that there is no selection foreground color
	}
}

static bool SetViewSelectionForegroundColor(EDITORVIEW *theView,EDITORCOLOR foreground)
// set up the foreground color used to draw the selection in theView
// if there is a problem, leave the color unchanged, and return false
{
	GUIVIEWINFO
		*theViewInfo;
	GUICOLOR
		*newColor;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if((newColor=AllocColor(foreground)))
	{
		UnSetViewSelectionForegroundColor(theView);
		theViewInfo->selectionForegroundColor=newColor;
		return(true);
	}
	return(false);
}

static void UnSetViewSelectionBackgroundColor(EDITORVIEW *theView)
// free the background color used to draw the selection in theView
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->selectionBackgroundColor)
	{
		FreeColor(theViewInfo->selectionBackgroundColor);
		theViewInfo->selectionBackgroundColor=NULL;				// remember that there is no selection background color
	}
}

static bool SetViewSelectionBackgroundColor(EDITORVIEW *theView,EDITORCOLOR background)
// set up the background color used to draw the selection in theView
// if there is a problem, leave the color unchanged, and return false
{
	GUIVIEWINFO
		*theViewInfo;
	GUICOLOR
		*newColor;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if((newColor=AllocColor(background)))
	{
		UnSetViewSelectionBackgroundColor(theView);
		theViewInfo->selectionBackgroundColor=newColor;
		return(true);
	}
	return(false);
}

static void SetViewTabSize(EDITORVIEW *theView,UINT32 theSize)
// set the tabSize for theView
{
	if(theSize>255)
	{
		theSize=255;	// limit this
	}
	((GUIVIEWINFO *)(theView->viewInfo))->tabSize=theSize;
}

static void UnInitializeViewStyle(EDITORVIEW *theView,UINT32 theStyle)
// undo what InitializeViewStyle did
{
	UnSetViewStyleFont(theView,theStyle);
	UnSetViewStyleBackgroundColor(theView,theStyle);
	UnSetViewStyleForegroundColor(theView,theStyle);
}

static void UnInitializeViewStyles(EDITORVIEW *theView)
// get rid of style information tied to theView
{
	UINT32
		i;

	for(i=0;i<VIEWMAXSTYLES;i++)
	{
		UnSetViewStyleFont(theView,i);
		UnSetViewStyleBackgroundColor(theView,i);
		UnSetViewStyleForegroundColor(theView,i);
	}
}

static bool InitializeViewStyles(EDITORVIEW *theView,EDITORVIEWDESCRIPTOR *theDescriptor)
// initialize style information for theView
{
	UINT32
		i;
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	for(i=0;i<VIEWMAXSTYLES;i++)
	{
		theViewInfo->viewStyles[i].theFont=NULL;				// clear all pointers
		theViewInfo->viewStyles[i].foregroundColor=NULL;
		theViewInfo->viewStyles[i].backgroundColor=NULL;
	}

	// set up default style (which must be present)
	if(SetViewStyleForegroundColor(theView,0,theDescriptor->foregroundColor))
	{
		if(SetViewStyleBackgroundColor(theView,0,theDescriptor->backgroundColor))
		{
			if(SetViewStyleFont(theView,0,theDescriptor->fontName))
			{
				return(true);
			}
			UnSetViewStyleBackgroundColor(theView,0);
		}
		UnSetViewStyleForegroundColor(theView,0);
	}
	return(false);
}

void DisposeEditorView(EDITORVIEW *theView)
// unlink theView from everywhere, and dispose of it
{
	UnInitializeViewStyles(theView);					// get rid of style information for theView
	UnSetViewSelectionBackgroundColor(theView);			// get rid of any allocated selection colors
	UnSetViewSelectionForegroundColor(theView);
	UnlinkViewFromBuffer(theView);
	MDisposePtr(theView->viewInfo);
	MDisposePtr(theView);
}

EDITORVIEW *CreateEditorView(EDITORWINDOW *theWindow,EDITORVIEWDESCRIPTOR *theDescriptor)
// create a view of theBuffer in theWindow using theDescriptor
// if there is a problem, SetError, and return NULL
{
	EDITORVIEW
		*theView;
	GUIVIEWINFO
		*viewInfo;

	if((theView=(EDITORVIEW *)MNewPtr(sizeof(EDITORVIEW))))
	{
		if((viewInfo=(GUIVIEWINFO *)MNewPtr(sizeof(GUIVIEWINFO))))
		{
			// create a window which has the sole purpose of changing the cursor to a caret when it enters the view
			theView->viewInfo=viewInfo;

			viewInfo->topLine=theDescriptor->topLine;
			viewInfo->leftPixel=theDescriptor->leftPixel;		// initial pixel to display
			SetViewTabSize(theView,theDescriptor->tabSize);		// set, and limit the tab size
			viewInfo->selectionForegroundColor=NULL;			// select by inverting text colors by default
			viewInfo->selectionBackgroundColor=NULL;
			viewInfo->foregroundColor=NULL;						// select by inverting text colors by default
			viewInfo->backgroundColor=NULL;
			viewInfo->lineHBitmap=0;
			viewInfo->bounds=theDescriptor->theRect;			// copy bounds rectangle
			viewInfo->viewActive=theDescriptor->active;
			viewInfo->cursorOn=true;							// the cursor is turned on
			viewInfo->blinkState=false;							// next time it blinks, it will blink to on
			viewInfo->xorBrush=0;
			LinkViewToBuffer(theView,theDescriptor->theBuffer);
			theView->viewTextChangedVector=theDescriptor->viewTextChangedVector;
			theView->viewSelectionChangedVector=theDescriptor->viewSelectionChangedVector;
			theView->viewPositionChangedVector=theDescriptor->viewPositionChangedVector;
			theView->parentWindow=theWindow;

			if(InitializeViewStyles(theView,theDescriptor))
			{
				RecalculateViewFontInfo(theView);				// update view information based on fonts and stuff
				return(theView);
			}
		}
		MDisposePtr(theView);
	}
	return(NULL);
}

void InvalidateViewPortion(EDITORVIEW *theView,UINT32 startLine,UINT32 endLine,UINT32 startPixel,UINT32 endPixel)
// invalidate the area of the view between startLine, endLine, and startPixel, endPixel
// NOTE: startLine, endLine, startPixel, and endPixel are all relative to the displayed view
// so that startLine of 0 always means the top line, and startPixel of 0 always
// means the left-most pixel
// NOTE: startLine must be <= endLine, startPixel must be <= endPixel
// the invalidated area is exclusive of endLine, and endPixel
{
	UINT32
		totalPixels;
	GUIVIEWINFO
		*theViewInfo;
	WINDOWLISTELEMENT
		*theWindowElement;
	RECT
		invalidRect;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;						// point at the information record for this view
	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;

	totalPixels=theViewInfo->textBounds.right-theViewInfo->textBounds.left;
	if(theViewInfo->numLines&&totalPixels)								// make sure we can actually invalidate
	{
		if(startLine>theViewInfo->numLines)
		{
			startLine=theViewInfo->numLines;
		}
		if(endLine>theViewInfo->numLines)
		{
			endLine=theViewInfo->numLines;
		}

		if(startPixel>totalPixels)
		{
			startPixel=totalPixels;
		}
		if(endPixel>totalPixels)
		{
			endPixel=totalPixels;
		}
		invalidRect.left=theViewInfo->textBounds.left+startPixel;
		invalidRect.top=theViewInfo->textBounds.top+startLine*theViewInfo->lineHeight;
		invalidRect.right=invalidRect.left+(endPixel-startPixel);
		invalidRect.bottom=invalidRect.top+((endLine-startLine)*theViewInfo->lineHeight);
		InvalidateRect(theWindowElement->hwnd,&invalidRect,TRUE);
	}
}

static void InvalidateView(EDITORVIEW *theView)
// invalidate the entire area of the view
{
	GUIVIEWINFO
		*theViewInfo;
	WINDOWLISTELEMENT
		*theWindowElement;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;						// point at the information record for this view
	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
	InvalidateRect(theWindowElement->hwnd,&theViewInfo->bounds,TRUE);
}

void ScrollViewPortion(EDITORVIEW *theView,UINT32 startLine,UINT32 endLine,INT32 numLines,UINT32 startPixel,UINT32 endPixel,INT32 numPixels)
// scroll the area of the view between startLine, endLine, and startPixel, endPixel
// by numLines, and numPixels
// if numLines > 0, the data is scrolled down, else it is scrolled up
// if numPixels > 0, the data is scrolled right, else it is scrolled left
// any area left open by the scroll will be left erased, and invalidated
// NOTE: startLine, endLine, startPixel, and endPixel are all relative to the displayed view
// so that startLine of 0 always means the top line, and startPixel of 0 always
// means the left-most pixel
// NOTE: startLine must be <= endLine, startPixel must be <= endPixel
// the scrolled area is exclusive of endLine, and endPixel
{

	HWND
		hwnd;
	UINT32
		totalPixels;
	RECT
		clipRect,
		sourceRect;
	GUIVIEWINFO
		*theViewInfo;
	WINDOWLISTELEMENT
		*theWindowElement;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;						/* point at the information record for this view */
	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
	hwnd=theWindowElement->hwnd;

	totalPixels=theViewInfo->textBounds.right-theViewInfo->textBounds.left;
	if(theViewInfo->numLines&&totalPixels)								/* make sure we can actually scroll */
	{
		if(startLine>theViewInfo->numLines)
		{
			startLine=theViewInfo->numLines;
		}
		if(endLine>theViewInfo->numLines)
		{
			endLine=theViewInfo->numLines;
		}

		if(startPixel>totalPixels)
		{
			startPixel=totalPixels;
		}
		if(endPixel>totalPixels)
		{
			endPixel=totalPixels;
		}

		if(numLines>((INT32)(endLine-startLine)))						/* allow a scroll that erases everything */
		{
			numLines=endLine-startLine;
		}
		else
		{
			if(numLines<((INT32)startLine-(INT32)endLine))
			{
				numLines=(INT32)startLine-(INT32)endLine;
			}
		}

		if(numPixels>((INT32)(endPixel-startPixel)))
		{
			numPixels=endPixel-startPixel;
		}
		else
		{
			if(numPixels<(INT32)startPixel-(INT32)endPixel)
			{
				numPixels=(INT32)startPixel-(INT32)endPixel;
			}
		}

		sourceRect.left=theViewInfo->textBounds.left+startPixel;
		sourceRect.right=theViewInfo->textBounds.left+endPixel;
		sourceRect.top=theViewInfo->textBounds.top+startLine*theViewInfo->lineHeight;
		sourceRect.bottom=theViewInfo->textBounds.top+endLine*theViewInfo->lineHeight;
		if(IntersectRect(&clipRect,&sourceRect,&(theViewInfo->textBounds)))
		{
			ScrollWindowEx(hwnd,(int)numPixels,(int)numLines*theViewInfo->lineHeight,&sourceRect,&clipRect,NULL,NULL,SW_INVALIDATE);
		}
	}
}

void SetViewTopLeft(EDITORVIEW *theView,UINT32 newTopLine,INT32 newLeftPixel)
// set the upper left corner of theView
// NOTE: newTop, and newLeft will be forced into bounds by this routine
{
	UINT32
		oldTopLine;
	INT32
		oldLeftPixel;
	UINT32
		numLines;
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;						// point at the information record for this view
	numLines=theView->parentBuffer->textUniverse->totalLines;
	oldTopLine=theViewInfo->topLine;
	oldLeftPixel=theViewInfo->leftPixel;
	Sleep(15);

	if(newTopLine>numLines)
	{
		newTopLine=numLines;
	}
	if(newLeftPixel>EDITMAXWIDTH)
	{
		newLeftPixel=EDITMAXWIDTH;
	}
	if(newLeftPixel<-EDITMAXWIDTH)
	{
		newLeftPixel=-EDITMAXWIDTH;
	}
	if(theViewInfo->topLine!=newTopLine || theViewInfo->leftPixel!=newLeftPixel)
	{
		UpdateEditorWindows();												/* make sure the display is up to date before changing it */
		theViewInfo->topLine=newTopLine;
		theViewInfo->leftPixel=newLeftPixel;
		ScrollViewPortion(theView,0,theViewInfo->numLines,oldTopLine-newTopLine,0,theViewInfo->textBounds.right-theViewInfo->textBounds.left,oldLeftPixel-newLeftPixel);
		if(theView->viewPositionChangedVector)								// see if someone needs to be called because of my change
		{
			theView->viewPositionChangedVector(theView);					// call him with my pointer
		}
		UpdateEditorWindows();												// redraw now to make the update more crisp
	}
}

void SetEditorViewTopLine(EDITORVIEW *theView,UINT32 lineNumber)
// set the line number of the top line that is
// displayed in the given edit view
// NOTE: this does no invalidation of the view
{
	((GUIVIEWINFO *)(theView->viewInfo))->topLine=lineNumber;
}

UINT32 GetEditorViewTabSize(EDITORVIEW *theView)
// get the current tab size for the given view
{
	return(((GUIVIEWINFO *)(theView->viewInfo))->tabSize);
}

bool SetEditorViewTabSize(EDITORVIEW *theView,UINT32 theSize)
// set the tab size to be used in theView
// this will force an update of theView
// if there is a problem, leave the tab size unchanged, set the error
// and return false
{
	SetViewTabSize(theView,theSize);
	InvalidateView(theView);
	return(true);
}

static void ForceStyleRange(UINT32 *theStyle)
// make sure the style is in range (for externally called routines)
{
	if(*theStyle>=VIEWMAXSTYLES)
	{
		*theStyle=VIEWMAXSTYLES-1;
	}
}

bool GetEditorViewStyleFont(EDITORVIEW *theView,UINT32 theStyle,char *theFont,UINT32 stringBytes)
// get the font that is in use in theView for the given style
// if the font will not fit within string bytes, return false
{
	GUIVIEWINFO
		*theViewInfo;

	ForceStyleRange(&theStyle);
	if(stringBytes)
	{
		theViewInfo=(GUIVIEWINFO *)theView->viewInfo;
		strncpy(theFont,theViewInfo->viewStyles[theStyle].theFont->theName,stringBytes);
		theFont[stringBytes-1]='\0';
		return(true);
	}
	return(false);
}

bool SetEditorViewStyleFont(EDITORVIEW *theView,UINT32 theStyle,char *theFont)
// set the font to be used in theView
// this will force an update of theView
// if there is a problem, leave the window font unchanged, set the error
// and return false
{
	ForceStyleRange(&theStyle);

	if(SetViewStyleFont(theView,theStyle,theFont))				// set new font
	{
		RecalculateViewFontInfo(theView);						// update additional information
		InvalidateView(theView);
		return(true);
	}
	return(false);
}

void ClearEditorViewStyleFont(EDITORVIEW *theView,UINT32 theStyle)
// clear the font to be used for theStyle in theView
// this will force an update of theView
{
	ForceStyleRange(&theStyle);
	UnSetViewStyleFont(theView,theStyle);
	RecalculateViewFontInfo(theView);							// update additional information
	InvalidateView(theView);
}

bool GetEditorViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR *foregroundColor)
// get the foreground color in use by theStyle of theView
// if there is a problem, SetError, and return false
{
	GUIVIEWINFO
		*theViewInfo;
		
	ForceStyleRange(&theStyle);
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->viewStyles[theStyle].foregroundColor)
	{
		*foregroundColor=theStyle=theViewInfo->viewStyles[theStyle].foregroundColor->theColor;		// get pixel of background for this style
		return(true);
	}
	return(false);
}

bool SetEditorViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR foregroundColor)
// set the foreground color to be used by theStyle of theView
// this will force an update of theView
// if there is a problem, leave the color unchanged, set the error
// and return false
{
	ForceStyleRange(&theStyle);
	if(SetViewStyleForegroundColor(theView,theStyle,foregroundColor))	// set new color
	{
		InvalidateView(theView);									// redraw it all
		return(true);
	}
	return(false);
}

void ClearEditorViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle)
// clear the foreground color to be used for theStyle in theView
// this will force an update of theView
{
	ForceStyleRange(&theStyle);
	UnSetViewStyleForegroundColor(theView,theStyle);
	InvalidateView(theView);
}

bool GetEditorViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR *backgroundColor)
// get the background color in use by theStyle of theView
// if there is a problem, SetError, and return false
{
	GUIVIEWINFO
		*theViewInfo;

	ForceStyleRange(&theStyle);
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->viewStyles[theStyle].backgroundColor)
	{
		*backgroundColor=theViewInfo->viewStyles[theStyle].backgroundColor->theColor;		// get pixel of background for this style
		return(true);
	}
	return(true);
}

bool SetEditorViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR backgroundColor)
// set the background color to be used by theStyle of theView
// this will force an update of theView
// if there is a problem, leave the color unchanged, set the error
// and return false
{
	ForceStyleRange(&theStyle);
	if(SetViewStyleBackgroundColor(theView,theStyle,backgroundColor))	// set new color
	{
		InvalidateView(theView);									// redraw it all
		return(true);
	}
	return(false);
}

void ClearEditorViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle)
// clear the background color to be used for theStyle in theView
// this will force an update of theView
{
	ForceStyleRange(&theStyle);
	UnSetViewStyleBackgroundColor(theView,theStyle);
	InvalidateView(theView);
}

bool GetEditorViewSelectionForegroundColor(EDITORVIEW *theView,EDITORCOLOR *foregroundColor)
// get the foreground color in use by the selection of theView
// NOTE: it is possible that no color has been specified.
// If that is the case, this will return false in haveForeground.
// if there is a problem, SetError, and return false
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;							// point at the information record for this view
	if(theViewInfo->selectionForegroundColor)
	{
		*foregroundColor=theViewInfo->selectionForegroundColor->theColor;
		return(true);
	}
	return(false);
}

bool SetEditorViewSelectionForegroundColor(EDITORVIEW *theView,EDITORCOLOR foregroundColor)
// set the foreground color to be used for selections in theView
// this will force an update of theView
// if there is a problem, leave the color unchanged, set the error
// and return false
// NOTE: if haveForeground is false, this will clear the foreground color
// making the selection draw the foreground as the inverse of the current
// color
{
	if(SetViewSelectionForegroundColor(theView,foregroundColor))	// set new color
	{
		InvalidateView(theView);								// redraw it all
		return(true);
	}
	return(false);
}

void ClearEditorViewSelectionForegroundColor(EDITORVIEW *theView)
// Clear the foreground color, making the selection draw the foreground
// as the inverse of the current color
{
	UnSetViewSelectionForegroundColor(theView);					// clear color
	InvalidateView(theView);									// redraw it all
}

bool GetEditorViewSelectionBackgroundColor(EDITORVIEW *theView,EDITORCOLOR *backgroundColor)
// get the background color in use by the selection of theView
// NOTE: it is possible that no color has been specified.
// If that is the case, this will return false
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;							// point at the information record for this view
	if(theViewInfo->selectionBackgroundColor)
	{
		*backgroundColor=theViewInfo->selectionBackgroundColor->theColor;
		return(true);
	}
	else
	{
		return(false);
	}
}

bool SetEditorViewSelectionBackgroundColor(EDITORVIEW *theView,EDITORCOLOR backgroundColor)
// set the background color to be used for selections in theView
// this will force an update of theView
// if there is a problem, leave the color unchanged, set the error
// and return false
// NOTE: if haveBackground is false, this will clear the background color
// making the selection draw the background as the inverse of the current
// color
{
	if(SetViewSelectionBackgroundColor(theView,backgroundColor))	// set new color
	{
		InvalidateView(theView);								// redraw it all
		return(true);
	}
	return(false);
}

void ClearEditorViewSelectionBackgroundColor(EDITORVIEW *theView)
// Clear the background color, making the selection draw the background
// as the inverse of the current color
{
	UnSetViewSelectionBackgroundColor(theView);					// clear color
	InvalidateView(theView);									// redraw it all
}

static void ViewStartCursorChange(EDITORVIEW *theView)
// when the state (not position) of the cursor
// is about to change, call this
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	theViewInfo->oldCursorShowing=CursorShowing(theView);		// remember if it was showing
}

static void ViewEndCursorChange(EDITORVIEW *theView)
// when the state (not position) of the cursor
// has changed, call this, and it will update the cursor as needed
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->oldCursorShowing!=CursorShowing(theView))	// see if the state has changed
	{
		InvertViewCursor(0,theView);								// if it is showing, then draw it
	}
}

void ViewStartSelectionChange(EDITORVIEW *theView)
// the selection in this view is about to change, so prepare for it
// since this could move the cursor, turn it off
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;		// point at the information record for this view
	theViewInfo->oldSelectedRegion=CreateRectRgn(0,0,0,0);					// create a region to hold what is now selected
	CreateSelectionRegion(theView,theViewInfo->oldSelectedRegion);
	ViewStartCursorChange(theView);						// begin cursor update
	theViewInfo->cursorOn=false;						// turn cursor off
	ViewEndCursorChange(theView);
}

void ViewEndSelectionChange(EDITORVIEW *theView)
// the selection in this view has changed, update it
{
	HRGN
		leftRgn,
		rightRgn,
		newSelectionRegion;
	GUIVIEWINFO
		*theViewInfo;
	WINDOWLISTELEMENT
		*theWindowElement;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;						// point at the information record for this view
	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;

	newSelectionRegion=CreateRectRgn(0,0,0,0);
	CreateSelectionRegion(theView,newSelectionRegion);
	CombineRgn(newSelectionRegion,newSelectionRegion,theViewInfo->oldSelectedRegion,RGN_XOR);
	if(theViewInfo->maxLeftOverhang)
	{
		UINT32
			offset,
			maxOffset;
			
		leftRgn=CreateRectRgn(0,0,0,0);
		CombineRgn(leftRgn,newSelectionRegion,leftRgn,RGN_COPY);
		maxOffset=theViewInfo->maxLeftOverhang;
		while(maxOffset)
		{
			offset=Min(maxOffset,theViewInfo->minCharWidth);
			OffsetRgn(leftRgn,-(int)offset,0);
			CombineRgn(newSelectionRegion,newSelectionRegion,leftRgn,RGN_OR);
			maxOffset-=offset;
		}
		DeleteObject(leftRgn);								/* get rid of the region we created */
	}
	if(theViewInfo->maxRightOverhang)
	{
		UINT32
			offset,
			maxOffset;

		rightRgn=CreateRectRgn(0,0,0,0);
		CombineRgn(rightRgn,newSelectionRegion,rightRgn,RGN_COPY);
		maxOffset=theViewInfo->maxRightOverhang;
		while(maxOffset)
		{
			offset=Min(maxOffset,theViewInfo->minCharWidth);
			OffsetRgn(rightRgn,offset,0);
			CombineRgn(newSelectionRegion,newSelectionRegion,rightRgn,RGN_OR);
			maxOffset-=offset;
		}
		DeleteObject(rightRgn);								/* get rid of the region we created */
	}
	InvalidateRgn(theWindowElement->hwnd,newSelectionRegion,TRUE);

	DeleteObject(theViewInfo->oldSelectedRegion);				/* get rid of the region we created */
	DeleteObject(newSelectionRegion);								/* get rid of the region we created */

	UpdateEditorWindows();

	ViewStartCursorChange(theView);						// begin cursor update
	theViewInfo->cursorOn=true;							// turn cursor back on
	ViewEndCursorChange(theView);
}

void BlinkViewCursor(EDITORVIEW *theView)
// calling this will cause the cursor in theView to change blink states, updating it as needed
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view
	ViewStartCursorChange(theView);
	theViewInfo->blinkState=!theViewInfo->blinkState;		// invert blink state
	ViewEndCursorChange(theView);
}

void ResetEditorViewCursorBlink(EDITORVIEW *theView)
// when this is called, it will reset the cursor blinking, forcing the
// cursor to the blink-on state, and resetting the blink timeout
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view
	if(!theViewInfo->blinkState)							// if it is currently off, blink it to on
	{
		BlinkViewCursor(theView);
	}
	ResetCursorBlinkTime(theView);
}

void ViewCursorTask(EDITORVIEW *theView)
// handle blinking the cursor for theView
{
	BlinkViewCursor(theView);
}

void ViewsStartSelectionChange(EDITORBUFFER *theBuffer)
// the selection in theBuffer is about to be changed in some way, do whatever is needed
// NOTE: a change in the selection could mean something as simple as a cursor position change
// NOTE also: selection changes cannot be nested, and nothing except the selection is allowed
// to be altered during a selection change
{
	EDITORVIEW
		*currentView;

	currentView=theBuffer->firstView;
	while(currentView)									// walk through all views, update each one as needed
	{
		ViewStartSelectionChange(currentView);
		currentView=currentView->nextBufferView;		// locate next view of this buffer
	}
}

void ViewsEndSelectionChange(EDITORBUFFER *theBuffer)
// the selection in theBuffer has finished being changed
// do whatever needs to be done
// to get it up to date
{
	EDITORVIEW
		*currentView;

	currentView=theBuffer->firstView;
	while(currentView)									// walk through all views, update each one as needed
	{
		ViewEndSelectionChange(currentView);
		if(currentView->viewSelectionChangedVector)		// see if someone needs to be called because of my change
		{
			currentView->viewSelectionChangedVector(currentView);	// call him with my pointer
		}
		currentView=currentView->nextBufferView;		// locate next view of this buffer
	}
}

static void ViewStartStyleChange(EDITORVIEW *theView)
// the styles in this view are about to change, so prepare for it
{
	GUIVIEWINFO
		*theViewInfo;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset,
		startPosition,
		endPosition,
		actualStart,
		length;
	UINT32
		theStyle;
	UINT32
		i;
	bool
		done;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;		// point at the information record for this view
	if((theViewInfo->heldStyleUniverse=OpenStyleUniverse()))	// get a style universe, and add the styles visible in this view to it
	{
		LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine,&theChunk,&theOffset,&startPosition);		// get position of top line in this view
		LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine+theViewInfo->numLines,&theChunk,&theOffset,&endPosition);		// get position past last line in this view
		done=false;
		i=startPosition;
		while(i<endPosition&&!done)						// copy the part of the style array that intersects the view
		{
			if(GetStyleRange(theView->parentBuffer->styleUniverse,i,&actualStart,&length,&theStyle))
			{
				ForceStyleIntoRange(theStyle);
				if(actualStart<endPosition)
				{
					if(actualStart+length>endPosition)
					{
						length=endPosition-actualStart;
					}
					SetStyleRange(theViewInfo->heldStyleUniverse,actualStart,length,theStyle);
					i=actualStart+length;
				}
				else
				{
					done=true;
				}
			}
			else
			{
				done=true;
			}
		}
	}
}

static void ViewEndStyleChange(EDITORVIEW *theView)
// the styles in this view have been changed, update the view
{
	GUIVIEWINFO
		*theViewInfo;
	HRGN
		newStyleRegion;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset,
		currentPosition;
	UINT32
		i;
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	if(theViewInfo->heldStyleUniverse)						// make sure we did not fail to create this at the start
	{
		newStyleRegion=CreateRectRgn(0,0,0,0);

		gceTabSize=theViewInfo->tabSize;
		for(i=0;i<theViewInfo->numLines;i++)
		{
			LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine+i,&theChunk,&theOffset,&currentPosition);	// point to data for line
			AddStyleDifferencesToRegion(theView,newStyleRegion,theChunk,theOffset,currentPosition,theViewInfo->textBounds.top+i*theViewInfo->lineHeight);
		}
		InvalidateRgn(theWindowElement->hwnd,newStyleRegion,TRUE);
		DeleteObject(newStyleRegion);								/* get rid of the region we created */
		CloseStyleUniverse(theViewInfo->heldStyleUniverse);	// get rid of memory this occupies
	}
}

void ViewsStartStyleChange(EDITORBUFFER *theBuffer)
// the style in theBuffer is about to be changed in some way, do whatever is needed
// NOTE also: style changes cannot be nested, and nothing except the style is allowed
// to be altered during a style change
{
	EDITORVIEW
		*currentView;

	currentView=theBuffer->firstView;
	while(currentView)									// walk through all views, update each one as needed
	{
		ViewStartStyleChange(currentView);
		currentView=currentView->nextBufferView;		// locate next view of this buffer
	}
}

void ViewsEndStyleChange(EDITORBUFFER *theBuffer)
// the style in theBuffer has finished being changed
// do whatever needs to be done
// to get it up to date
{
	EDITORVIEW
		*currentView;

	currentView=theBuffer->firstView;
	while(currentView)									// walk through all views, update each one as needed
	{
		ViewEndStyleChange(currentView);
		currentView=currentView->nextBufferView;		// locate next view of this edit universe
	}
}

void ViewsStartTextChange(EDITORBUFFER *theBuffer)
// the text in theBuffer is about to be changed in some way, do whatever is needed
// NOTE: when the text changes, it is possible that the selection information
// will also change, so that updates will be handled correctly
{
	EDITORVIEW
		*currentView;

	currentView=theBuffer->firstView;
	while(currentView)									// walk through all views, update each one as needed
	{
		ViewStartCursorChange(currentView);
		((GUIVIEWINFO *)(currentView->viewInfo))->cursorOn=false;	// turn cursor off while it is moved
		ViewEndCursorChange(currentView);
		currentView=currentView->nextBufferView;		// locate next view of this edit universe
	}
}

void ViewsEndTextChange(EDITORBUFFER *theBuffer)
// the text in theBuffer has finished being changed, and the views have been
// updated, do whatever needs to be done
{
	EDITORVIEW
		*currentView;

	currentView=theBuffer->firstView;
	while(currentView)								// walk through all views, update each one as needed
	{
		ViewStartCursorChange(currentView);
		((GUIVIEWINFO *)(currentView->viewInfo))->cursorOn=true;	// turn cursor back on when done
		ViewEndCursorChange(currentView);
		if(currentView->viewTextChangedVector)		// see if someone needs to be called because of my change
		{
			currentView->viewTextChangedVector(currentView);	// call him with my pointer
		}
		currentView=currentView->nextBufferView;		// locate next view of this edit universe
	}
}

void EditorActivateView(EDITORVIEW *theView)
// activate theView (this is not nestable)
{
	ViewStartSelectionChange(theView);
	((GUIVIEWINFO *)(theView->viewInfo))->viewActive=true;
	ViewEndSelectionChange(theView);
}

void EditorDeactivateView(EDITORVIEW *theView)
// deactivate theView (this is not nestable)
{
	ViewStartSelectionChange(theView);
	((GUIVIEWINFO *)(theView->viewInfo))->viewActive=false;
	ViewEndSelectionChange(theView);
}

