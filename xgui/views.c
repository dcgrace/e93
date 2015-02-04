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
static UINT16
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

bool PointInView(EDITORVIEW *theView,INT32 x,INT32 y)
// return true if parent window relative x/y are in theView
// false if not
{
	return(PointInRECT(x,y,&(((GUIVIEWINFO *)theView->viewInfo)->bounds)));
}

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
		*xPosition+=*slopLeft;
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
	*numPixels=theViewInfo->textBounds.w;
}

static void InvertViewCursor(EDITORVIEW *theView)
// invert the cursor wherever it happens to be in theView
// theView's window's current graphics context will be used to constrain the draw
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
	UINT32
		slopLeft,
		slopRight;
	EDITORRECT
		cursorRect;
	Window
		xWindow;
	GC
		graphicsContext;
	UINT32
		theStyle;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;					// point at the information record for this view

	PositionToLinePosition(theView->parentBuffer->textUniverse,GetSelectionCursorPosition(theView->parentBuffer->selectionUniverse),&theLine,&theLineOffset,&theChunk,&theOffset);
	if((theLine>=theViewInfo->topLine)&&(theLine<theViewInfo->topLine+theViewInfo->numLines))	// make sure cursor is on the view
	{
		maxXPosition=xPosition=theViewInfo->leftPixel+theViewInfo->textBounds.w;
		GetEditorViewTextToGraphicPosition(theView,GetSelectionCursorPosition(theView->parentBuffer->selectionUniverse),&xPosition,true,&slopLeft,&slopRight);

		theStyle=0;													// always paint the cursor in style 0

		if((xPosition>=theViewInfo->leftPixel)&&(xPosition<maxXPosition))	// see if on view horizontally
		{
			cursorRect.x=theViewInfo->textBounds.x+(xPosition-theViewInfo->leftPixel);
			cursorRect.w=1;
			cursorRect.y=theViewInfo->textBounds.y+theViewInfo->lineHeight*(theLine-theViewInfo->topLine);
			cursorRect.h=theViewInfo->lineHeight;

			xWindow=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->xWindow;		// get x window for this view
			graphicsContext=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->graphicsContext;	// get graphics context for this window

			XSetFunction(xDisplay,graphicsContext,GXxor);			// prepare to fill in xor format
			XSetFillStyle(xDisplay,graphicsContext,FillSolid);		// use solid fill

			XSetForeground(xDisplay,graphicsContext,GetForegroundForStyle(theViewInfo->viewStyles,theStyle)->theXPixel^GetBackgroundForStyle(theViewInfo->viewStyles,theStyle)->theXPixel);	// this is the value to xor with

			XFillRectangle(xDisplay,xWindow,graphicsContext,cursorRect.x,cursorRect.y,cursorRect.w,cursorRect.h);

			XSetFunction(xDisplay,graphicsContext,GXcopy);			// put function back
		}
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

static void DrawViewCursor(EDITORVIEW *theView)
// look at the cursor state for this view, and draw it as needed
// it will be drawn into the current port, and current clip region
{
	if(CursorShowing(theView))
	{
		InvertViewCursor(theView);							// if it is showing, then draw it
	}
}

static void AddSelectionDifferencesToRegion(EDITORVIEW *theView,Region theRegion,UINT32 currentPosition,INT32 yWindowOffset)
// Read bytes from the line of text at theChunk, theOffset
// Add pieces where the current selection, and the held selection differ to theRegion
// NOTE: this must work as an exact mirror to DrawViewLine
// NOTE: this adds extra space to the left and right of the region
// as it creates it, to accomodate the overhang of fonts
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
	bool
		oldSelection,
		oldNextSelection,
		currentSelection,
		nextSelection;
	UINT32
		currentStyle,
		nextStyle;
	INT32
		scrollX;					// pixel offset in text where we need to start drawing
	INT32
		textXOffset;				// window relative pixel offset where text is being drawn
	INT32
		distanceToStart;
	XRectangle
		selectionRect;
	UINT32
		charWidth;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	xWindowOffset=theViewInfo->textBounds.x;
	xWindowEndOffset=theViewInfo->textBounds.x+theViewInfo->textBounds.w;

	selectionRect.y=yWindowOffset;
	selectionRect.height=theViewInfo->lineHeight;

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

	textXOffset=xWindowOffset+distanceToStart;				// move the offset slightly back to beginning of first character which enters the view (or forward if view is scrolled negatively)

	oldSelection=GetSelectionAtPosition(theViewInfo->heldSelectionUniverse,currentPosition);
	currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);

	currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);
	ForceStyleIntoRange(currentStyle);

	gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);

	while(theChunk&&((textXOffset+gceRunningWidth)<xWindowEndOffset)&&GetCharacterEquiv(theChunk->data[theOffset]))		// run through the text
	{
		oldNextSelection=GetSelectionAtPosition(theViewInfo->heldSelectionUniverse,currentPosition+1);
		nextSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition+1);

		nextStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition+1);
		ForceStyleIntoRange(nextStyle);

		if((currentStyle!=nextStyle)||(oldSelection!=oldNextSelection)||(currentSelection!=nextSelection)||(gceBufferIndex>STAGINGBUFFERSIZE-STAGINGMAXADD))	// is something about to change? (if so, output what we have so far) (this will also output if the staging buffer is getting full)
		{
			if(currentSelection!=oldSelection)
			{
				selectionRect.x=textXOffset-theViewInfo->maxLeftOverhang;
				selectionRect.width=gceRunningWidth+theViewInfo->maxLeftOverhang+theViewInfo->maxRightOverhang;
				if(selectionRect.x<xWindowOffset)			// pin to bounds
				{
					selectionRect.width-=(xWindowOffset-selectionRect.x);
					selectionRect.x=xWindowOffset;
				}
				if(selectionRect.x+selectionRect.width>xWindowEndOffset)
				{
					selectionRect.width=xWindowEndOffset-selectionRect.x;
				}
				XUnionRectWithRegion(&selectionRect,theRegion,theRegion);
			}

			textXOffset+=gceRunningWidth;

			gceBufferIndex=0;								// begin at start of buffer
			gceRunningWidth=0;

			oldSelection=oldNextSelection;
			currentSelection=nextSelection;
			currentStyle=nextStyle;
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
		if(currentSelection!=oldSelection)
		{
			selectionRect.x=textXOffset-theViewInfo->maxLeftOverhang;
			selectionRect.width=gceRunningWidth+theViewInfo->maxLeftOverhang+theViewInfo->maxRightOverhang;
			if(selectionRect.x<xWindowOffset)					// pin to bounds
			{
				selectionRect.width-=(xWindowOffset-selectionRect.x);
				selectionRect.x=xWindowOffset;
			}
			if(selectionRect.x+selectionRect.width>xWindowEndOffset)
			{
				selectionRect.width=xWindowEndOffset-selectionRect.x;
			}
			XUnionRectWithRegion(&selectionRect,theRegion,theRegion);
		}
		textXOffset+=gceRunningWidth;
	}

	if(textXOffset<xWindowEndOffset)							// do we need to go out to the end of the line?
	{
		if(textXOffset<xWindowOffset)							// did this end before the draw area began?
		{
			textXOffset=xWindowOffset;
		}
		oldSelection=GetSelectionAtPosition(theViewInfo->heldSelectionUniverse,currentPosition);
		currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);			// get selection information for newline (or off of end)

		if(currentSelection!=oldSelection)
		{
			selectionRect.x=textXOffset;
			selectionRect.width=xWindowEndOffset-textXOffset;
			if(selectionRect.x<xWindowOffset)					// pin to bounds
			{
				selectionRect.width-=(xWindowOffset-selectionRect.x);
				selectionRect.x=xWindowOffset;
			}
			if(selectionRect.x+selectionRect.width>xWindowEndOffset)
			{
				selectionRect.width=xWindowEndOffset-selectionRect.x;
			}
			XUnionRectWithRegion(&selectionRect,theRegion,theRegion);
		}
	}
}

static void AddStyleDifferencesToRegion(EDITORVIEW *theView,Region theRegion,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 currentPosition,INT32 yWindowOffset)
// Run through the line, looking for a place where the current style differs
// from the style that was there before. Then, because a style change can cause
// a font change, and because a font change can move text left or right,
// invalidate from the start of the change to the right side of the view.
// NOTE: this must work as an exact mirror to DrawViewLine
// NOTE: this adds extra space to the left and right of the region
// as it creates it, to accomodate the overhang of fonts
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
	XRectangle
		styleRect;
	INT32
		xPosition;
	bool
		done;

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
				xPosition=theViewInfo->leftPixel+theViewInfo->textBounds.w;	// limit search to this distance
				PixelAtPosition(theView,currentPosition,&xPosition,true);
				xPosition-=theViewInfo->maxLeftOverhang;	// move back by the maximum overhang
				if(xPosition<(INT32)(theViewInfo->leftPixel+theViewInfo->textBounds.w))	// see if any work to do
				{
					if(xPosition<theViewInfo->leftPixel)	// does the change start off the left of view?
					{
						xPosition=theViewInfo->leftPixel;	// just fudge to the left
					}
					styleRect.x=theViewInfo->textBounds.x+(xPosition-theViewInfo->leftPixel);
					styleRect.y=yWindowOffset;
					styleRect.width=theViewInfo->textBounds.w-(xPosition-theViewInfo->leftPixel);
					styleRect.height=theViewInfo->lineHeight;
					XUnionRectWithRegion(&styleRect,theRegion,theRegion);
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

static void DrawViewBackgroundPiece(EDITORVIEW *theView,bool selected,UINT32 theStyle,INT32 xWindowOffset,UINT32 width,INT32 yWindowOffset)
// draw the background into the view in the given style and selection
{
	Window
		xWindow;
	GC
		graphicsContext;
	GUIVIEWINFO
		*theViewInfo;
	GUICOLOR
		*background;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view
	graphicsContext=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->graphicsContext;	// get graphics context for this window
	xWindow=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->xWindow;					// get x window for this window

	if(selected)
	{
		if(!theViewInfo->selectionForegroundColor&&!theViewInfo->selectionBackgroundColor)		// if neither specified, invert
		{
			background=GetForegroundForStyle(theViewInfo->viewStyles,theStyle);
		}
		else	// otherwise, do not invert
		{
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
		background=GetBackgroundForStyle(theViewInfo->viewStyles,theStyle);
	}

	XSetForeground(xDisplay,graphicsContext,background->theXPixel);	// set fill color

	XFillRectangle(xDisplay,xWindow,graphicsContext,xWindowOffset,yWindowOffset,width,theViewInfo->lineHeight);
}

static void DrawViewTextPiece(EDITORVIEW *theView,bool selected,UINT32 theStyle,INT32 xWindowOffset,UINT32 width,INT32 yWindowOffset,INT32 clipOffset)
// draw the piece of text in the staging buffer into the view in the given style and selection
{
	Window
		xWindow;
	GC
		graphicsContext;
	GUIVIEWINFO
		*theViewInfo;
	GUICOLOR
		*foreground;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view
	graphicsContext=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->graphicsContext;	// get graphics context for this window
	xWindow=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->xWindow;					// get x window for this window

	XSetFont(xDisplay,graphicsContext,GetFontForStyle(theViewInfo->viewStyles,theStyle)->theXFont->fid);	// point to the font needed for this draw
	if(selected)
	{
		if(!theViewInfo->selectionForegroundColor&&!theViewInfo->selectionBackgroundColor)		// if neither specified, invert
		{
			foreground=GetBackgroundForStyle(theViewInfo->viewStyles,theStyle);
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
		}
	}
	else
	{
		foreground=GetForegroundForStyle(theViewInfo->viewStyles,theStyle);
	}

	XSetForeground(xDisplay,graphicsContext,foreground->theXPixel);

// code that would be here if there was no hack
//	XDrawString(xDisplay,xWindow,graphicsContext,xWindowOffset,yWindowOffset+theViewInfo->lineAscent,(char *)gceStagingBuffer,gceBufferIndex);

//------------------------------------------------------------------------------------------------------------------
// NOTE: this hack brought to you by X windows....
// If XDrawString has to clip the text at the right boundary, it draws much slower than
// if it does not. So.... we detect that case and make 2 calls to draw (which are usually much faster
// than 1)
// The first call draws the entire line up to the first non-clipping character
// The second call just draws the clipping character
{
	UINT32
		charactersBack,
		pixelsBack;

	if(xWindowOffset+(INT32)width>clipOffset)
	{
		charactersBack=0;
		pixelsBack=0;
		while((charactersBack<gceBufferIndex)&&((xWindowOffset+(INT32)(width-pixelsBack))>clipOffset))
		{
			charactersBack++;
			pixelsBack+=gceWidthTable[gceStagingBuffer[gceBufferIndex-charactersBack]];
		}
		XDrawString(xDisplay,xWindow,graphicsContext,xWindowOffset,yWindowOffset+theViewInfo->lineAscent,(char *)gceStagingBuffer,gceBufferIndex-charactersBack);
		XDrawString(xDisplay,xWindow,graphicsContext,xWindowOffset+width-pixelsBack,yWindowOffset+theViewInfo->lineAscent,(char *)&gceStagingBuffer[gceBufferIndex-charactersBack],charactersBack);
	}
	else	// no problem, the text is not going to clip
	{
		XDrawString(xDisplay,xWindow,graphicsContext,xWindowOffset,yWindowOffset+theViewInfo->lineAscent,(char *)gceStagingBuffer,gceBufferIndex);
	}
}
//------------------------------------------------------------------------------------------------------------------
}

static void DrawViewLine(EDITORVIEW *theView,UINT32 currentPosition,INT32 xWindowOffset,INT32 xWindowEndOffset,INT32 yWindowOffset)
// Read bytes from the line of text at currentPosition
// Draw them onto the view
// This is responsible for drawing ALL of the pixels in the view for the line
// on which it was called (even if that line contains no text)
// NOTE: this performs the draw in 2 passes.
// The first pass fills the background for the line, and the second
// pass draws the text over the background.
// The draw is done this way so that fonts which have pixels which
// overhang their character cells will draw without being clipped off
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	Window
		xWindow;
	GUIVIEWINFO
		*theViewInfo;
	INT32
		skippedWidth;
	GC
		graphicsContext;
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

	UINT32
		beginningPosition;
	CHUNKHEADER
		*beginningChunk;
	UINT32
		beginningOffset;
	UINT32
		beginningColumn;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	// point at the information record for this view
	graphicsContext=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->graphicsContext;	// get graphics context for this window
	xWindow=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->xWindow;					// get x window for this window

	scrollX=theViewInfo->leftPixel+(xWindowOffset-theViewInfo->textBounds.x);	// get pixel offset in text where we need to start drawing

	PositionAtPixel(theView,currentPosition,scrollX,&currentPosition,&gceColumnNumber,&skippedWidth,&charWidth);

	PositionToChunkPosition(theView->parentBuffer->textUniverse,currentPosition,&theChunk,&theOffset);

	beginningPosition=currentPosition;				// remember these for later
	beginningChunk=theChunk;
	beginningOffset=theOffset;
	beginningColumn=gceColumnNumber;

	gceBufferIndex=0;								// begin at start of buffer
	gceRunningWidth=0;

	distanceToStart=skippedWidth-scrollX;

	if(distanceToStart>0)							// blank space before text starts?
	{
		if(distanceToStart>(xWindowEndOffset-xWindowOffset))	// do not go past the end of the draw area
		{
			distanceToStart=xWindowEndOffset-xWindowOffset;
		}
		XSetFillStyle(xDisplay,graphicsContext,FillSolid);
		XSetForeground(xDisplay,graphicsContext,theViewInfo->viewStyles[0].backgroundColor->theXPixel);
		XFillRectangle(xDisplay,xWindow,graphicsContext,xWindowOffset,yWindowOffset,distanceToStart,theViewInfo->lineHeight);	// fill with background color
	}

	// first draw all background

	textXOffset=xWindowOffset+distanceToStart;		// move the offset slightly back to beginning of first character which enters the view (or forward if view is scrolled negatively)

	currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);
	ForceStyleIntoRange(currentStyle);
	currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);
	gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);

	while(theChunk&&((textXOffset+gceRunningWidth)<xWindowEndOffset)&&GetCharacterEquiv(theChunk->data[theOffset]))		// fill in all of the background needed
	{
		nextStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition+1);
		ForceStyleIntoRange(nextStyle);
		nextSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition+1);

		if((currentStyle!=nextStyle)||(currentSelection!=nextSelection)||(gceBufferIndex>STAGINGBUFFERSIZE-STAGINGMAXADD))	// is something about to change? (if so, output background so far) (this will also output if the staging buffer is getting full)
		{
			DrawViewBackgroundPiece(theView,currentSelection,currentStyle,textXOffset,gceRunningWidth,yWindowOffset);

			textXOffset+=gceRunningWidth;

			gceBufferIndex=0;						// begin at start of buffer
			gceRunningWidth=0;

			currentStyle=nextStyle;
			currentSelection=nextSelection;
			gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
		}

		currentPosition++;
		theOffset++;
		if(theOffset>=theChunk->totalBytes)			// push through end of chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}

	// ran off the end of the line, or the end of the universe
	if(gceBufferIndex)
	{
		DrawViewBackgroundPiece(theView,currentSelection,currentStyle,textXOffset,gceRunningWidth,yWindowOffset);
		textXOffset+=gceRunningWidth;
	}

	if(textXOffset<xWindowEndOffset)				// do we need to draw out to the end of the line?
	{
		if(textXOffset<xWindowOffset)				// did this end before the draw area began?
		{
			textXOffset=xWindowOffset;
		}
		currentStyle=GetStyleAtPosition(theView->parentBuffer->styleUniverse,currentPosition);	// get style and selection information for newline (or off of end)
		ForceStyleIntoRange(currentStyle);
		currentSelection=GetSelectionAtPosition(theView->parentBuffer->selectionUniverse,currentPosition);

		DrawViewBackgroundPiece(theView,currentSelection,currentStyle,textXOffset,xWindowEndOffset-textXOffset,yWindowOffset);
	}


	// next draw all the text on top of the background

	currentPosition=beginningPosition;				// restore these from above
	theChunk=beginningChunk;
	theOffset=beginningOffset;
	gceColumnNumber=beginningColumn;

	gceBufferIndex=0;								// begin at start of buffer
	gceRunningWidth=0;

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
			DrawViewTextPiece(theView,currentSelection,currentStyle,textXOffset,gceRunningWidth,yWindowOffset,xWindowEndOffset);

			textXOffset+=gceRunningWidth;

			gceBufferIndex=0;						// begin at start of buffer
			gceRunningWidth=0;

			currentStyle=nextStyle;
			currentSelection=nextSelection;
			gceWidthTable=&(GetFontForStyle(theViewInfo->viewStyles,currentStyle)->charWidths[0]);
		}

		currentPosition++;
		theOffset++;
		if(theOffset>=theChunk->totalBytes)			// push through end of chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	// ran off the end of the line, or the end of the universe
	if(gceBufferIndex)
	{
		DrawViewTextPiece(theView,currentSelection,currentStyle,textXOffset,gceRunningWidth,yWindowOffset,xWindowEndOffset);
	}
}

static void DrawViewLines(EDITORVIEW *theView)
// draw the lines into theView, obey the invalid region of the view
{
	GUIVIEWINFO
		*theViewInfo;
	XRectangle
		lineRect,
		regionBox;
	Region
		invalidTextRegion,
		lineRegion;
	UINT32
		topLineToDraw,
		bottomLineToDraw;
	INT32
		xWindowOffset;
	INT32
		xWindowEndOffset;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset,
		currentPosition;
	UINT32
		i;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	gceTabSize=theViewInfo->tabSize;

	regionBox.x=lineRect.x=theViewInfo->textBounds.x;
	regionBox.width=lineRect.width=theViewInfo->textBounds.w;
	lineRect.height=theViewInfo->lineHeight;
	regionBox.y=theViewInfo->textBounds.y;
	regionBox.height=theViewInfo->textBounds.h;

	invalidTextRegion=XCreateRegion();
	XUnionRectWithRegion(&regionBox,invalidTextRegion,invalidTextRegion);		// make rectangular region of entire text space
	XIntersectRegion(invalidTextRegion,((WINDOWLISTELEMENT *)theView->parentWindow->userData)->invalidRegion,invalidTextRegion);	// intersect with invalid region of window

	XClipBox(invalidTextRegion,&regionBox);				// get bounds for actual invalid text area

	topLineToDraw=(regionBox.y-theViewInfo->textBounds.y)/theViewInfo->lineHeight;	// screen relative top line to draw
	bottomLineToDraw=(regionBox.y-theViewInfo->textBounds.y+regionBox.height-1)/theViewInfo->lineHeight;	// get last line to draw

	for(i=topLineToDraw;i<=bottomLineToDraw;i++)
	{
		lineRect.y=theViewInfo->textBounds.y+i*theViewInfo->lineHeight;
		lineRegion=XCreateRegion();
		XUnionRectWithRegion(&lineRect,lineRegion,lineRegion);		// make region for the line we are about to draw
		XIntersectRegion(lineRegion,invalidTextRegion,lineRegion);	// see if it intersects with the invalid region
		if(!XEmptyRegion(lineRegion))
		{
			XClipBox(lineRegion,&regionBox);						// get bounds for this part
			LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine+i,&theChunk,&theOffset,&currentPosition);	// point to data for this line

			xWindowOffset=regionBox.x;
			xWindowEndOffset=regionBox.x+regionBox.width;

			// extend the draw area slightly to account for left and right text overhang

			xWindowOffset-=theViewInfo->maxRightOverhang;			// move back far enough to let any character which extends to the right to draw what it needs
			xWindowEndOffset+=theViewInfo->maxLeftOverhang;			// do same for right hand side

			DrawViewLine(theView,currentPosition,xWindowOffset,xWindowEndOffset,lineRect.y);
		}
		XDestroyRegion(lineRegion);
	}
	XDestroyRegion(invalidTextRegion);
}

void DrawView(EDITORVIEW *theView)
// draw the given view in its parent window
// also handle highlighting of text
// NOTE: the invalidRegion of the view's parent
// window should be set to the region that needs update
{
	XRectangle
		regionBox;
	GUIVIEWINFO
		*theViewInfo;
	WINDOWLISTELEMENT
		*theWindowElement;
	GC
		graphicsContext;
	Region
		viewInvalidRegion;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;						// point at the information record for this view

	theWindowElement=(WINDOWLISTELEMENT *)theView->parentWindow->userData;	// get the x window information associated with this editor window
	graphicsContext=theWindowElement->graphicsContext;					// get graphics context for this window

	viewInvalidRegion=XCreateRegion();
	regionBox.x=theViewInfo->bounds.x;
	regionBox.y=theViewInfo->bounds.y;
	regionBox.width=theViewInfo->bounds.w;
	regionBox.height=theViewInfo->bounds.h;
	XUnionRectWithRegion(&regionBox,viewInvalidRegion,viewInvalidRegion);	// create a region which is the rectangle of the entire view
	XIntersectRegion(viewInvalidRegion,theWindowElement->invalidRegion,viewInvalidRegion);	// intersect with what is invalid in this window

	if(!XEmptyRegion(viewInvalidRegion))								// see if it has an invalid area
	{
		XClipBox(viewInvalidRegion,&regionBox);							// get bounds of invalid region

		XSetRegion(xDisplay,graphicsContext,viewInvalidRegion);			// set clipping to what's invalid in the view

		XSetFillStyle(xDisplay,graphicsContext,FillSolid);
		XSetForeground(xDisplay,graphicsContext,theViewInfo->viewStyles[0].backgroundColor->theXPixel);

		if(regionBox.x<theViewInfo->textBounds.x)
		{
			XFillRectangle(xDisplay,theWindowElement->xWindow,graphicsContext,theViewInfo->bounds.x,theViewInfo->bounds.y,theViewInfo->textBounds.x-theViewInfo->bounds.x,theViewInfo->bounds.h);
		}

		if(regionBox.y<theViewInfo->textBounds.y)
		{
			XFillRectangle(xDisplay,theWindowElement->xWindow,graphicsContext,theViewInfo->bounds.x,theViewInfo->bounds.y,theViewInfo->bounds.w,theViewInfo->textBounds.y-theViewInfo->bounds.y);
		}

		if((regionBox.y+(int)regionBox.height)>(theViewInfo->textBounds.y+(int)theViewInfo->textBounds.h))
		{
			XFillRectangle(xDisplay,theWindowElement->xWindow,graphicsContext,theViewInfo->bounds.x,theViewInfo->textBounds.y+theViewInfo->textBounds.h,theViewInfo->bounds.w,(theViewInfo->bounds.y+theViewInfo->bounds.h)-(theViewInfo->textBounds.y-theViewInfo->textBounds.h));
		}

		DrawViewLines(theView);
		DrawViewCursor(theView);
		XSetClipMask(xDisplay,graphicsContext,None);					// get rid of clip mask
	}
	XDestroyRegion(viewInvalidRegion);									// get rid of invalid region for the view
}

static void RecalculateViewFontInfo(EDITORVIEW *theView)
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
	theViewInfo->lineAscent=0;
	theViewInfo->maxLeftOverhang=0;
	theViewInfo->maxRightOverhang=0;
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
	theViewInfo->textBounds.y+=VIEWTOPMARGIN;					// leave a small margin at the top

	if(theViewInfo->bounds.h>VIEWTOPMARGIN+VIEWBOTTOMMARGIN)
	{
		viewHeight=theViewInfo->bounds.h-VIEWTOPMARGIN-VIEWBOTTOMMARGIN;	// find out how much can be used for the text
		theViewInfo->numLines=viewHeight/theViewInfo->lineHeight;	// get integer number of lines in this view
		theViewInfo->textBounds.h=theViewInfo->numLines*theViewInfo->lineHeight;	// alter the textBounds to be the exact rectangle to hold the text
	}
	else
	{
		theViewInfo->numLines=0;
		theViewInfo->textBounds.h=0;							// no room to draw text
	}
}

void SetViewBounds(EDITORVIEW *theView,EDITORRECT *newBounds)
// set the bounds of theView to newBounds
// invalidate area where the view used to be but now is not
// and invalidate new areas of the view
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORRECT
		*viewBounds;
	unsigned int
		theWidth,
		theHeight;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view

	viewBounds=&(theViewInfo->bounds);							// point at the view bounds rect

	if((newBounds->x!=viewBounds->x)||(newBounds->y!=viewBounds->y)||(newBounds->w!=viewBounds->w)||(newBounds->h!=viewBounds->h))
	{
		InvalidateWindowRect(theView->parentWindow,viewBounds);			// invalidate old area
		InvalidateWindowRect(theView->parentWindow,newBounds);			// invalidate new area

		(*viewBounds)=*newBounds;								// copy new rect into place
		RecalculateViewFontInfo(theView);						// update additional information

		if(!(theWidth=theViewInfo->textBounds.w))
		{
			theWidth=1;
		}
		if(!(theHeight=theViewInfo->textBounds.h))
		{
			theHeight=1;
		}
		XMoveResizeWindow(xDisplay,theViewInfo->cursorWindow,theViewInfo->textBounds.x,theViewInfo->textBounds.y,theWidth,theHeight);	// adjust the cursor change window
	}
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
	XDestroyWindow(xDisplay,((GUIVIEWINFO *)theView->viewInfo)->cursorWindow);				// tell x to make the cursor window go away
	UnlinkViewFromBuffer(theView);
	MDisposePtr(theView->viewInfo);
	MDisposePtr(theView);
}

EDITORVIEW *CreateEditorView(EDITORWINDOW *theWindow,EDITORVIEWDESCRIPTOR *theDescriptor)
// create a view in theWindow using theDescriptor
// if there is a problem, SetError, and return NULL
{
	EDITORVIEW
		*theView;
	GUIVIEWINFO
		*viewInfo;
	unsigned int
		theWidth,
		theHeight;

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
			viewInfo->bounds=theDescriptor->theRect;			// copy bounds rectangle
			viewInfo->viewActive=theDescriptor->active;
			viewInfo->cursorOn=true;							// the cursor is turned on
			viewInfo->blinkState=false;							// next time it blinks, it will blink to on
			viewInfo->nextBlinkTime=0;							// next time it checks, it will blink
			LinkViewToBuffer(theView,theDescriptor->theBuffer);
			theView->viewTextChangedVector=theDescriptor->viewTextChangedVector;
			theView->viewSelectionChangedVector=theDescriptor->viewSelectionChangedVector;
			theView->viewPositionChangedVector=theDescriptor->viewPositionChangedVector;
			theView->parentWindow=theWindow;

			if(InitializeViewStyles(theView,theDescriptor))
			{
				RecalculateViewFontInfo(theView);				// update view information based on fonts and stuff

				if(!(theWidth=viewInfo->textBounds.w))
				{
					theWidth=1;
				}
				if(!(theHeight=viewInfo->textBounds.h))
				{
					theHeight=1;
				}

				viewInfo->cursorWindow=XCreateWindow(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,viewInfo->textBounds.x,viewInfo->textBounds.y,theWidth,theHeight,0,0,InputOnly,DefaultVisual(xDisplay,xScreenNum),0,NULL);
				XDefineCursor(xDisplay,viewInfo->cursorWindow,caretCursor);			// this is the cursor to use in the view
				XMapRaised(xDisplay,viewInfo->cursorWindow);		// map window to the top of the pile

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
	EDITORRECT
		invalidRect;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;						// point at the information record for this view

	totalPixels=theViewInfo->textBounds.w;
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
		invalidRect.x=theViewInfo->textBounds.x+startPixel;
		invalidRect.y=theViewInfo->textBounds.y+startLine*theViewInfo->lineHeight;
		invalidRect.w=endPixel-startPixel;
		invalidRect.h=(endLine-startLine)*theViewInfo->lineHeight;
		InvalidateWindowRect(theView->parentWindow,&invalidRect);
	}
}

static void InvalidateView(EDITORVIEW *theView)
// invalidate the entire area of the view
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;						// point at the information record for this view
	InvalidateWindowRect(theView->parentWindow,&theViewInfo->bounds);
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
	Window
		xWindow;
	GC
		graphicsContext;
	UINT32
		totalPixels;
	INT32
		copySrcX,					// source x and y for copy
		copySrcY,
		copyDestX,
		copyDestY,
		invalidX,
		invalidY;
	UINT32
		copyWidth,
		copyHeight,
		invalidWidth,
		invalidHeight;
	EDITORRECT
		invalidRect;
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;						// point at the information record for this view
	xWindow=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->xWindow;					// get x window for this view
	graphicsContext=((WINDOWLISTELEMENT *)theView->parentWindow->userData)->graphicsContext;	// get graphicsContext for this view

	totalPixels=theViewInfo->textBounds.w;
	if(theViewInfo->numLines&&totalPixels)								// make sure we can actually scroll
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

		if(numLines>((INT32)(endLine-startLine)))						// allow a scroll that erases everything
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
		if(numLines||numPixels)
		{
			if(numLines>=0)												// see if scrolling down, or not scrolling
			{
				invalidY=copySrcY=startLine;
				copyDestY=startLine+numLines;
				copyHeight=(endLine-startLine)-numLines;
				invalidHeight=numLines;
			}
			else														// scrolling up
			{
				copySrcY=startLine-numLines;
				copyDestY=startLine;
				copyHeight=endLine-copySrcY;
				invalidY=endLine+numLines;
				invalidHeight=-numLines;
			}
			if(numPixels>=0)											// scrolling right, or not scrolling horizontally
			{
				invalidX=copySrcX=startPixel;
				copyDestX=startPixel+numPixels;
				copyWidth=(endPixel-startPixel)-numPixels;
				invalidWidth=numPixels;
			}
			else														// left
			{
				copySrcX=startPixel-numPixels;
				copyDestX=startPixel;
				copyWidth=endPixel-copySrcX;
				invalidX=endPixel+numPixels;
				invalidWidth=-numPixels;
			}
			if(copyWidth&&copyHeight)
			{
				XSetGraphicsExposures(xDisplay,graphicsContext,True);	// tell x to give events when part of scroll is revealed
				XGrabServer(xDisplay);									// keep anyone else from invalidating while the scroll is in progress (so they do not invalidate BEFORE the scroll, causing invalid data to be scrolled and never updated)
				XSync(xDisplay,False);									// get all invalidations
				UpdateEditorWindows();									// redraw everything which is invalid before scrolling
				XCopyArea(xDisplay,xWindow,xWindow,graphicsContext,theViewInfo->textBounds.x+copySrcX,theViewInfo->textBounds.y+copySrcY*theViewInfo->lineHeight,copyWidth,copyHeight*theViewInfo->lineHeight,theViewInfo->textBounds.x+copyDestX,theViewInfo->textBounds.y+copyDestY*theViewInfo->lineHeight);
				XUngrabServer(xDisplay);								// let go after the scroll
				XSetGraphicsExposures(xDisplay,graphicsContext,False);
			}
			if(invalidWidth)
			{
				invalidRect.x=theViewInfo->textBounds.x+invalidX;
				invalidRect.y=theViewInfo->textBounds.y+startLine*theViewInfo->lineHeight;
				invalidRect.w=invalidWidth;
				invalidRect.h=(endLine-startLine)*theViewInfo->lineHeight;
				InvalidateWindowRect(theView->parentWindow,&invalidRect);
			}
			if(invalidHeight)
			{
				invalidRect.x=theViewInfo->textBounds.x+startPixel;
				invalidRect.y=theViewInfo->textBounds.y+invalidY*theViewInfo->lineHeight;
				invalidRect.w=endPixel-startPixel;
				invalidRect.h=invalidHeight*theViewInfo->lineHeight;
				InvalidateWindowRect(theView->parentWindow,&invalidRect);
			}
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
	ScrollViewPortion(theView,0,theViewInfo->numLines,oldTopLine-newTopLine,0,theViewInfo->textBounds.w,oldLeftPixel-newLeftPixel);
	theViewInfo->topLine=newTopLine;
	theViewInfo->leftPixel=newLeftPixel;
	if(theView->viewPositionChangedVector)								// see if someone needs to be called because of my change
	{
		theView->viewPositionChangedVector(theView);					// call him with my pointer
	}
	UpdateEditorWindows();												// redraw now to make the update more crisp
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
// if there is no font in use, return an empty string
{
	GUIVIEWINFO
		*theViewInfo;
	unsigned long
		theValue;
	char
		*atomName;

	ForceStyleRange(&theStyle);
	if(stringBytes)
	{
		theViewInfo=(GUIVIEWINFO *)theView->viewInfo;
		if(theViewInfo->viewStyles[theStyle].theFont)
		{
			if(XGetFontProperty(GetFontForStyle(theViewInfo->viewStyles,theStyle)->theXFont,XA_FONT,&theValue))
			{
				if((atomName=XGetAtomName(xDisplay,theValue)))
				{
					strncpy(theFont,atomName,stringBytes);
					theFont[stringBytes-1]='\0';
					return(true);
				}
			}
		}
		else
		{
			theFont[0]='\0';
			return(true);
		}
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
// if the style does not have a foreground color associated with it, return false
{
	GUIVIEWINFO
		*theViewInfo;
	XColor
		theColor;

	ForceStyleRange(&theStyle);
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->viewStyles[theStyle].foregroundColor)
	{
		theColor.pixel=theViewInfo->viewStyles[theStyle].foregroundColor->theXPixel;	// get pixel of foreground for this style
		XQueryColor(xDisplay,xColormap,&theColor);
		XColorToEditorColor(&theColor,foregroundColor);
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
// if the style does not have a background color associated with it, return false
{
	GUIVIEWINFO
		*theViewInfo;
	XColor
		theColor;

	ForceStyleRange(&theStyle);
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->viewStyles[theStyle].backgroundColor)
	{
		theColor.pixel=theViewInfo->viewStyles[theStyle].backgroundColor->theXPixel;	// get pixel of background for this style
		XQueryColor(xDisplay,xColormap,&theColor);
		XColorToEditorColor(&theColor,backgroundColor);
		return(true);
	}
	return(false);
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
		InvalidateView(theView);								// redraw it all
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
// If that is the case, this will return false
{
	GUIVIEWINFO
		*theViewInfo;
	XColor
		theColor;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;							// point at the information record for this view
	if(theViewInfo->selectionForegroundColor)
	{
		theColor.pixel=theViewInfo->selectionForegroundColor->theXPixel;	// get pixel of foreground
		XQueryColor(xDisplay,xColormap,&theColor);
		XColorToEditorColor(&theColor,foregroundColor);
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
	XColor
		theColor;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;				// point at the information record for this view
	if(theViewInfo->selectionBackgroundColor)
	{
		theColor.pixel=theViewInfo->selectionBackgroundColor->theXPixel;	// get pixel of background
		XQueryColor(xDisplay,xColormap,&theColor);
		XColorToEditorColor(&theColor,backgroundColor);
		return(true);
	}
	return(false);
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
		InvertViewCursor(theView);								// if it is showing, then draw it
	}
}

static void ViewStartSelectionChange(EDITORVIEW *theView)
// the selection in this view is about to change, so prepare for it
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
		i;
	bool
		done;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;		// point at the information record for this view
	if((theViewInfo->heldSelectionUniverse=OpenSelectionUniverse()))	// get a selection universe, and add the selections visible in this view to it
	{
		LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine,&theChunk,&theOffset,&startPosition);		// get position of top line in this view
		LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine+theViewInfo->numLines,&theChunk,&theOffset,&endPosition);		// get position past last line in this view
		done=false;
		i=startPosition;
		while(i<endPosition&&!done)						// copy the part of the selection array that intersects the view
		{
			if(GetSelectionAtOrAfterPosition(theView->parentBuffer->selectionUniverse,i,&actualStart,&length))
			{
				if(actualStart<endPosition)
				{
					if(actualStart+length>endPosition)
					{
						length=endPosition-actualStart;
					}
					SetSelectionRange(theViewInfo->heldSelectionUniverse,actualStart,length);
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
	ViewStartCursorChange(theView);							// begin cursor update
	theViewInfo->cursorOn=false;							// turn cursor off
	ViewEndCursorChange(theView);
}

static void ViewEndSelectionChange(EDITORVIEW *theView)
// the selection in this view have been changed, update the view
{
	GUIVIEWINFO
		*theViewInfo;
	Region
		newSelectionRegion;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset,
		currentPosition;
	UINT32
		i;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	if(theViewInfo->heldSelectionUniverse)					// make sure we did not fail to create this at the start
	{
		newSelectionRegion=XCreateRegion();					// create a region to hold what is now in the selection

		gceTabSize=theViewInfo->tabSize;
		for(i=0;i<theViewInfo->numLines;i++)
		{
			LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine+i,&theChunk,&theOffset,&currentPosition);	// point to data for line
			AddSelectionDifferencesToRegion(theView,newSelectionRegion,currentPosition,theViewInfo->textBounds.y+i*theViewInfo->lineHeight);
		}
		InvalidateWindowRegion(theView->parentWindow,newSelectionRegion);
		XDestroyRegion(newSelectionRegion);					// get rid of the region we just created
		CloseSelectionUniverse(theViewInfo->heldSelectionUniverse);	// get rid of memory this occupies
	}
	UpdateEditorWindows();

	ViewStartCursorChange(theView);							// begin cursor update
	theViewInfo->cursorOn=true;								// turn cursor back on
	ViewEndCursorChange(theView);
}

static void BlinkViewCursor(EDITORVIEW *theView)
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
	ResetCursorBlinkTime();
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
	Region
		newStyleRegion;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset,
		currentPosition;
	UINT32
		i;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;			// point at the information record for this view

	if(theViewInfo->heldStyleUniverse)						// make sure we did not fail to create this at the start
	{
		newStyleRegion=XCreateRegion();						// create a region to hold what is now in the given style

		gceTabSize=theViewInfo->tabSize;
		for(i=0;i<theViewInfo->numLines;i++)
		{
			LineToChunkPosition(theView->parentBuffer->textUniverse,theViewInfo->topLine+i,&theChunk,&theOffset,&currentPosition);	// point to data for line
			AddStyleDifferencesToRegion(theView,newStyleRegion,theChunk,theOffset,currentPosition,theViewInfo->textBounds.y+i*theViewInfo->lineHeight);
		}
		InvalidateWindowRegion(theView->parentWindow,newStyleRegion);
		XDestroyRegion(newStyleRegion);						// get rid of the region we just created
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
		currentView=currentView->nextBufferView;		// locate next view of this buffer
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
		currentView=currentView->nextBufferView;		// locate next view of this buffer
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
		currentView=currentView->nextBufferView;		// locate next view of this buffer
	}
}

bool TrackView(EDITORVIEW *theView,XEvent *theEvent)
// call this to see if the click was in the view, and if so, track it
// if the click was not in the view, return false
{
	VIEWEVENT
		theRecord;
	VIEWCLICKEVENTDATA
		theClickData;
	GUIVIEWINFO
		*theViewInfo;
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
	UINT32
		numClicks,
		keyCode,
		modifiers;

	if(PointInView(theView,theEvent->xbutton.x,theEvent->xbutton.y))
	{
		numClicks=HandleMultipleClicks(theEvent);
		XStateToEditorModifiers(theEvent->xbutton.state,numClicks,&modifiers);
		switch(theEvent->xbutton.button)
		{
			case Button1:
				keyCode=0;
				break;
			case Button2:
				keyCode=1;
				break;
			case Button3:
				keyCode=2;
				break;
			default:
				keyCode=0;
				break;
		}
		theClickData.keyCode=keyCode;
		theClickData.modifiers=modifiers;

		theRecord.eventType=VET_CLICK;
		theRecord.theView=theView;
		theRecord.eventData=&theClickData;
		theViewInfo=(GUIVIEWINFO *)(theView->viewInfo);
		LocalClickToViewClick(theRecord.theView,theEvent->xbutton.x,theEvent->xbutton.y,&theClickData.xClick,&theClickData.yClick);
		if((theClickData.xClick>=0)&&(theClickData.xClick<(int)theViewInfo->textBounds.w))	// make sure initial click is in the text part of the view
		{
			if((theClickData.yClick>=0)&&(theClickData.yClick<(int)theViewInfo->numLines))	// make sure initial click is in the view
			{
				HandleViewEvent(&theRecord);
				while(StillDown(theEvent->xbutton.button,1))							// send repeat events for the view click
				{
					if(XQueryPointer(xDisplay,((WINDOWLISTELEMENT *)theView->parentWindow->userData)->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
					{
						theRecord.theView=theView;
						theRecord.eventType=VET_CLICKHOLD;
						theRecord.eventData=&theClickData;
						theClickData.modifiers=keyCode;
						theClickData.modifiers=modifiers;
						LocalClickToViewClick(theRecord.theView,windowX,windowY,&theClickData.xClick,&theClickData.yClick);
						HandleViewEvent(&theRecord);
					}
					UpdateEditorWindows();
				}
			}
		}
		return(true);
	}
	return(false);
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
