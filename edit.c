// Edit module
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

// The edit module handles the non-visible parts of editing not handled
// by text.c
// These include:
//  cursor position/selections
//  insertion and deletion of text
//  load/save
//  marks


#include	"includes.h"

#define	LOADSAVEBUFFERSIZE		65536

void FixBufferSelections(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 oldEndOffset,UINT32 newEndOffset)
// given a change at startOffset, run through the given selection lists for theBuffer,
// and fix them up
{
	MARKLIST
		*theMark;

	AdjustStylesForChange(theBuffer->styleUniverse,startOffset,oldEndOffset,newEndOffset);

	AdjustSelectionsForChange(theBuffer->selectionUniverse,startOffset,oldEndOffset,newEndOffset);
	AdjustSelectionsForChange(theBuffer->auxSelectionUniverse,startOffset,oldEndOffset,newEndOffset);
	theMark=theBuffer->theMarks;
	while(theMark)
	{
		AdjustSelectionsForChange(theMark->selectionUniverse,startOffset,oldEndOffset,newEndOffset);	// keep all the marks updated too
		theMark=theMark->nextMark;
	}
}

static INT32 GetNewLeftPixelLenient(INT32 startPixel,INT32 endPixel,INT32 viewLeft,UINT32 viewWidth,bool useEnd)
// When homing the view horizontally, this works out the best position for the left pixel
// This attempts to scroll only a small amount to keep the position in the view
{
	INT32
		newLeftPixel;
	INT32
		positionLeft;

	positionLeft=startPixel;
	if(useEnd)
	{
		positionLeft=endPixel;
	}
	if(positionLeft<viewLeft)
	{
		newLeftPixel=positionLeft-(positionLeft%HORIZONTALSCROLLTHRESHOLD);
	}
	else
	{
		if(positionLeft>=viewLeft+(INT32)viewWidth)
		{
			newLeftPixel=((positionLeft-viewWidth)/HORIZONTALSCROLLTHRESHOLD+1)*HORIZONTALSCROLLTHRESHOLD;
		}
		else
		{
			newLeftPixel=viewLeft;		// leave it where it is
		}
	}
	return(newLeftPixel);
}

static INT32 GetNewLeftPixelSemiStrict(INT32 startPixel,INT32 endPixel,INT32 viewLeft,UINT32 viewWidth,bool useEnd)
// When homing the view horizontally, this works out the best position for the left pixel
// If the position is currently off the screen, is more agressive about placing the position in the view,
// and will try to home the view to 0 as often as it can
{
	INT32
		newLeftPixel;

	newLeftPixel=viewLeft;
	if(!useEnd)
	{
		if(startPixel<newLeftPixel)
		{
			newLeftPixel=0;
		}
		if(endPixel>=newLeftPixel+(INT32)viewWidth)
		{
			newLeftPixel=endPixel-(INT32)viewWidth;
			newLeftPixel+=HORIZONTALSCROLLTHRESHOLD-(newLeftPixel%HORIZONTALSCROLLTHRESHOLD);
		}
		if(startPixel<newLeftPixel)
		{
			newLeftPixel=startPixel-HORIZONTALSCROLLTHRESHOLD;
		}
		if(newLeftPixel<0)
		{
			newLeftPixel=0;
		}
	}
	else
	{
		if(startPixel<newLeftPixel)
		{
			newLeftPixel=0;
		}
		if(startPixel>=newLeftPixel+(INT32)viewWidth)
		{
			newLeftPixel=startPixel-((INT32)viewWidth*3)/4;
		}
		if(endPixel>=newLeftPixel+(INT32)viewWidth)
		{
			newLeftPixel=endPixel-(INT32)viewWidth;
			newLeftPixel+=HORIZONTALSCROLLTHRESHOLD-(newLeftPixel%HORIZONTALSCROLLTHRESHOLD);
		}
		if(newLeftPixel<0)
		{
			newLeftPixel=0;
		}
	}

	return(newLeftPixel);
}

static INT32 GetNewLeftPixelStrict(INT32 startPixel,INT32 endPixel,INT32 viewLeft,UINT32 viewWidth,bool useEnd)
// When homing the view horizontally, this works out the best position for the left pixel
// This is more agressive about keeping the position in the view, and will work
// to keep it in the center half of the screen
{
	INT32
		newLeftPixel;
	INT32
		positionLeft;

	positionLeft=startPixel;
	if(useEnd)
	{
		positionLeft=endPixel;
	}

	if(positionLeft<viewLeft+((INT32)viewWidth/4))
	{
		newLeftPixel=positionLeft-((INT32)viewWidth/4);
	}
	else
	{
		if(positionLeft>=viewLeft+((INT32)viewWidth*3)/4)
		{
			newLeftPixel=positionLeft-((INT32)viewWidth*3)/4;
		}
		else
		{
			newLeftPixel=viewLeft;		// leave it where it is
		}
	}

	if(newLeftPixel<0)
	{
		newLeftPixel=0;
	}

	return(newLeftPixel);
}

void EditorHomeViewToRange(EDITORVIEW *theView,UINT32 startPosition,UINT32 endPosition,bool useEnd,int horizontalMovement,int verticalMovement)
// move theView so that startPosition to endPosition is displayed
// Attempt to move the view in the ways given by horizontalMovement/verticalMovement
// if useEnd is true, make sure that endPosition is on the view even if it means that
// startPosition is forced off the view.
// NOTE: startPosition must be <= endPosition
// NOTE: This function attempts to move the view in the way that a human
// will find the most pleasent. Because of this, it uses some unusual logic
// and should not be used by any routine that needs any guarantee of where
// the view will end up.
{
	UINT32
		startLine,
		endLine,
		newTopLine,
		topLine,
		numLines,
		numPixels,
		slopLeft,
		slopRight;
	INT32
		startLeftPixel,
		endLeftPixel;
	INT32
		leftPixel,
		newLeftPixel;
	CHUNKHEADER
		*theChunk;
	UINT32
		theLineOffset,
		theChunkOffset;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	PositionToLinePosition(theView->parentBuffer->textUniverse,startPosition,&startLine,&theLineOffset,&theChunk,&theChunkOffset);
	if(horizontalMovement!=HT_NONE)								// see if horizontal information is needed
	{
		GetEditorViewTextToGraphicPosition(theView,startPosition,&startLeftPixel,false,&slopLeft,&slopRight);
	}

	if(startPosition!=endPosition)
	{
		PositionToLinePosition(theView->parentBuffer->textUniverse,endPosition,&endLine,&theLineOffset,&theChunk,&theChunkOffset);

		if(horizontalMovement!=HT_NONE)							// see if horizontal information is needed
		{
			GetEditorViewTextToGraphicPosition(theView,endPosition,&endLeftPixel,false,&slopLeft,&slopRight);
		}
	}
	else
	{
		endLine=startLine;
		if(horizontalMovement!=HT_NONE)							// see if horizontal information is needed
		{
			endLeftPixel=startLeftPixel;
		}
	}

	newTopLine=topLine;
	if(!useEnd)													// see which position to home to
	{
		switch(verticalMovement)
		{
			case HT_NONE:
				break;
			case HT_LENIENT:
				if(startLine<topLine||!numLines)				// line is off the top, or display is 0 lines tall
				{
					newTopLine=startLine;						// so set top to new line
				}
				else
				{
					if(startLine>=topLine+numLines)				// line is off the bottom, so place it at the bottom
					{
						newTopLine=startLine-numLines+1;
					}
				}
				break;
			case HT_SEMISTRICT:
				if((startLine<topLine)||(startLine>=topLine+numLines)||(endLine>=topLine+numLines))
				{
					newTopLine=numLines/4;
					if(startLine<newTopLine)
					{
						newTopLine=0;
					}
					else
					{
						newTopLine=startLine-newTopLine;
					}
				}
				break;
			case HT_STRICT:
				newTopLine=numLines/4;
				if(startLine<newTopLine)
				{
					newTopLine=0;
				}
				else
				{
					newTopLine=startLine-newTopLine;
				}
				break;
		}
	}
	else
	{
		switch(verticalMovement)
		{
			case HT_NONE:
				break;
			case HT_LENIENT:
				if(endLine<topLine||!numLines)			// line is off the top, or display is 0 lines tall
				{
					newTopLine=endLine;					// so set top to new line
				}
				else
				{
					if(endLine>=topLine+numLines)		// line is off the bottom, so place it at the bottom
					{
						newTopLine=endLine-numLines+1;
					}
				}
				break;
			case HT_SEMISTRICT:
				if((startLine<topLine)||(endLine<topLine)||(endLine>=topLine+numLines))
				{
					newTopLine=numLines/4;
					if(endLine<newTopLine)
					{
						newTopLine=0;
					}
					else
					{
						newTopLine=endLine-newTopLine;
					}
				}
				break;
			case HT_STRICT:
				newTopLine=numLines/4;
				if(endLine<newTopLine)
				{
					newTopLine=0;
				}
				else
				{
					newTopLine=endLine-newTopLine;
				}
				break;
		}
	}

	newLeftPixel=leftPixel;
	switch(horizontalMovement)
	{
		case HT_NONE:
			break;
		case HT_LENIENT:
			newLeftPixel=GetNewLeftPixelLenient(startLeftPixel,endLeftPixel,leftPixel,numPixels,useEnd);
			break;
		case HT_SEMISTRICT:
			newLeftPixel=GetNewLeftPixelSemiStrict(startLeftPixel,endLeftPixel,leftPixel,numPixels,useEnd);
			break;
		case HT_STRICT:
			newLeftPixel=GetNewLeftPixelStrict(startLeftPixel,endLeftPixel,leftPixel,numPixels,useEnd);
			break;
	}

	if((newTopLine!=topLine)||(newLeftPixel!=leftPixel))
	{
		SetViewTopLeft(theView,newTopLine,newLeftPixel);
	}
}

void EditorHomeViewToSelection(EDITORVIEW *theView,bool useEnd,int horizontalMovement,int verticalMovement)
// move theView so that the selected text is displayed
// If the selected text is larger than the view, useEnd tells which end should
// remain in the view
{
	UINT32
		startPosition,
		endPosition;

	GetSelectionEndPositions(theView->parentBuffer->selectionUniverse,&startPosition,&endPosition);
	EditorHomeViewToRange(theView,startPosition,endPosition,useEnd,horizontalMovement,verticalMovement);
}

void EditorHomeViewToSelectionEdge(EDITORVIEW *theView,bool useEnd,int horizontalMovement,int verticalMovement)
// move theView so that the start (or end) of the selection is displayed
{
	UINT32
		startPosition,
		endPosition;

	GetSelectionEndPositions(theView->parentBuffer->selectionUniverse,&startPosition,&endPosition);
	if(!useEnd)
	{
		endPosition=startPosition;
	}
	else
	{
		startPosition=endPosition;
	}
	EditorHomeViewToRange(theView,startPosition,endPosition,false,horizontalMovement,verticalMovement);
}

void EditorVerticalScroll(EDITORVIEW *theView,INT32 amountToScroll)
// scroll the view vertically by amountToScroll
// amountToScroll<0, then scroll towards lower line numbers
// the scroll will pin to the edges of the view
{
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	if(amountToScroll>=0)
	{
		if((topLine+amountToScroll)<theView->parentBuffer->textUniverse->totalLines)
		{
			topLine+=amountToScroll;
		}
		else
		{
			topLine=theView->parentBuffer->textUniverse->totalLines;
		}
	}
	else
	{
		amountToScroll=-amountToScroll;
		if((int)topLine>amountToScroll)
		{
			topLine-=amountToScroll;
		}
		else
		{
			topLine=0;
		}
	}
	SetViewTopLeft(theView,topLine,leftPixel);
}

void EditorVerticalScrollByPages(EDITORVIEW *theView,INT32 pagesToScroll)
// page theView vertically by pagesToScroll
// if pagesToScroll<0, scroll towards lower line numbers in the view
// the scroll will pin to the edges of the view
{
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	EditorVerticalScroll(theView,pagesToScroll*(INT32)numLines);
}

void EditorHorizontalScroll(EDITORVIEW *theView,INT32 amountToScroll)
// scroll the view horizontally by amountToScroll pixels
// amountToScroll<0, then scroll towards the left
// the scroll will pin to the edges of the view
{
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	if(amountToScroll>=0)
	{
		leftPixel+=amountToScroll;
	}
	else
	{
		if(leftPixel+amountToScroll>0)
		{
			leftPixel+=amountToScroll;
		}
		else
		{
			leftPixel=0;
		}
	}
	SetViewTopLeft(theView,topLine,leftPixel);
}

void EditorHorizontalScrollByPages(EDITORVIEW *theView,INT32 pagesToScroll)
// page theView horizontally by pagesToScroll
// if pagesToScroll<0, scroll towards the left
// the scroll will pin to the edges of the view
{
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	EditorHorizontalScroll(theView,pagesToScroll*(INT32)numPixels);
}

void EditorStartSelectionChange(EDITORBUFFER *theBuffer)
// the selection in theBuffer is about to be changed in some way, do whatever is needed
// NOTE: a change in the selection could mean something as simple as a cursor position change
// NOTE also: selection changes cannot be nested, and nothing except the selection is allowed
// to be altered during a selection change
{
	ViewsStartSelectionChange(theBuffer);			// tell all the views of this buffer that the selection is changing
}

void EditorEndSelectionChange(EDITORBUFFER *theBuffer)
// the selection in theBuffer has finished being changed
// do whatever needs to be done
{
	ViewsEndSelectionChange(theBuffer);			// tell all the views of this buffer that the selection is changed
	theBuffer->haveStartX=theBuffer->haveEndX=false;	// after selection changed, X offsets are no longer valid
	theBuffer->haveCurrentEnd=false;				// lose track of current end when selection changes
}

void EditorStartStyleChange(EDITORBUFFER *theBuffer)
// the style in theBuffer is about to be changed in some way, do whatever is needed
// NOTE also: style changes cannot be nested, and nothing except the style is allowed
// to be altered during a style change
{
	ViewsStartStyleChange(theBuffer);				// tell all the views of this buffer that the style is changing
}

void EditorEndStyleChange(EDITORBUFFER *theBuffer)
// the style in theBuffer has finished being changed
// do whatever needs to be done
{
	ViewsEndStyleChange(theBuffer);				// tell all the views of this buffer that the style is changed
}

void EditorStartTextChange(EDITORBUFFER *theBuffer)
// the text in theBuffer is about to be changed in some way, do whatever is needed
// NOTE: when the text changes, it is possible that the selection and style information
// will also change, so that updates will be handled correctly
{
	ViewsStartTextChange(theBuffer);
}

void EditorEndTextChange(EDITORBUFFER *theBuffer)
// the text in theBuffer has finished being changed, and the views have been
// updated, do whatever needs to be done
{
	ViewsEndTextChange(theBuffer);
	theBuffer->haveStartX=theBuffer->haveEndX=false;	// after text has been replaced, X offsets are no longer valid
	theBuffer->haveCurrentEnd=false;						// lose track of current end when text changes
}

void EditorInvalidateViews(EDITORBUFFER *theBuffer)
// when something happens that requires an update of the entire buffer, call this, and
// all the views will be invalidated, and homed to line 0
{
	EDITORVIEW
		*currentView;
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;

	currentView=theBuffer->firstView;
	while(currentView)									// walk through all views, invalidating each one
	{
		GetEditorViewTextInfo(currentView,&topLine,&numLines,&leftPixel,&numPixels);
		InvalidateViewPortion(currentView,0,numLines,0,numPixels);
		SetEditorViewTopLine(currentView,0);
		currentView=currentView->nextBufferView;		// locate next view of this buffer
	}
}

static void AdjustViewForChange(EDITORVIEW *theView,UINT32 startLine,UINT32 endLine1,UINT32 endLine2,INT32 updateStartX)
// See what is changing in theView, and update it accordingly
// NOTE: this attempts to leave the view pointed to the same lines of
// text, even if those lines may have changed absolute position because of the
// insertion, or deletion of lines above them
// NOTE: This does not issue a call to scroll the view for subtle reasons:
// The view has already been modified by the time this is called. Because the
// scroll needs to make sure the views are up-to-date before scrolling them (otherwise
// it could be scrolling invalidated screen areas), it calls to get the views updated
// before scrolling. This will cause problems, since the view data has changed
// by this time, and the attempt to update will use the new (incorrect data)....
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	if(startLine<topLine+numLines)										// make sure all the changes did not happen completely after the view
	{
		if(endLine1>=topLine)
		{
			if(startLine<topLine)										// if start is before the view, then move the view to the start line
			{
				SetEditorViewTopLine(theView,startLine);				// actually adjust the top line of the view to the new start line
				InvalidateViewPortion(theView,0,numLines,0,numPixels);	// invalidate the whole thing
			}
			else
			{
				if((updateStartX-=leftPixel)<0)							// make X view relative
				{
					updateStartX=0;										// if before left edge of view, place at left edge of view
				}
				InvalidateViewPortion(theView,startLine-topLine,startLine-topLine+1,updateStartX,numPixels);	// invalidate to the end of the start line
				if((endLine1==endLine2)&&(endLine1<topLine+numLines))
				{
					if(endLine1>startLine)								// make sure start and end not on same line
					{
						InvalidateViewPortion(theView,startLine-topLine+1,endLine1-topLine+1,0,numPixels);	// invalidate all lines below up to and including the end line
					}
				}
				else
				{
					if(startLine-topLine+1<numLines)
					{
						InvalidateViewPortion(theView,startLine-topLine+1,numLines,0,numPixels);	// invalidate to the end of the view
					}
				}
			}
		}
		else
		{
			SetEditorViewTopLine(theView,topLine+endLine2-endLine1);	// adjust what we believe the top line is (change does not affect view)
		}
		LineToChunkPosition(theView->parentBuffer->textUniverse,topLine,&theChunk,&theOffset,&(theView->startPosition));			// adjust these after the view changes
		LineToChunkPosition(theView->parentBuffer->textUniverse,topLine+numLines,&theChunk,&theOffset,&(theView->endPosition));
	}
}

bool ReplaceEditorChunks(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,CHUNKHEADER *textChunk,UINT32 textOffset,UINT32 numBytes)
// replace the text between startOffset and endOffset with numBytes of text at textChunk/textOffset
// if there is a problem, set the error, and return false
{
	CHUNKHEADER
		*startLineChunk,
		*unusedChunk;
	UINT32
		startLineOffset,
		unusedOffset,
		slopLeft,
		slopRight;
	EDITORVIEW
		*currentView;
	UINT32
		unused,
		startLine,
		endLine,
		startCharOffset,
		oldNumLines;
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;
	bool
		haveViewNeedingUpdate,
		fail;

	fail=false;
	haveViewNeedingUpdate=false;
	currentView=theBuffer->firstView;
	while(currentView)									// walk through all views, see if any need attention
	{
		if(endOffset>=currentView->startPosition)		// make sure change is not completely before the start, if it is, we can ignore it
		{
			if(startOffset<=currentView->endPosition)	// make sure change is not completely after the end, if it is, we can ignore it
			{
				if(!haveViewNeedingUpdate)
				{
					PositionToLinePosition(theBuffer->textUniverse,startOffset,&startLine,&startCharOffset,&startLineChunk,&startLineOffset);
					PositionToLinePosition(theBuffer->textUniverse,endOffset,&endLine,&unused,&unusedChunk,&unusedOffset);
					haveViewNeedingUpdate=true;			// remember this stuff
				}
				GetEditorViewTextInfo(currentView,&topLine,&numLines,&leftPixel,&numPixels);
				if(startLine>=topLine)
				{
					currentView->updateStartX=leftPixel+numPixels;
					GetEditorViewTextToGraphicPosition(currentView,startOffset,&(currentView->updateStartX),true,&slopLeft,&slopRight);
					currentView->updateStartX-=slopLeft;
				}
				else
				{
					currentView->updateStartX=0;		// change begins before start line, so X is 0
				}
			}
		}
		currentView=currentView->nextBufferView;		// locate next view of this buffer
	}

	if(RegisterUndoDelete(theBuffer,startOffset,endOffset-startOffset))	// remember what we are about to delete
	{
		oldNumLines=theBuffer->textUniverse->totalLines;				// use this to find out how many lines were added/removed
		if(DeleteUniverseText(theBuffer->textUniverse,startOffset,endOffset-startOffset))
		{
			if(InsertUniverseChunks(theBuffer->textUniverse,startOffset,textChunk,textOffset,numBytes))
			{
				if(!RegisterUndoInsert(theBuffer,startOffset,numBytes))
				{
					fail=true;											// we do not cope well if this fails, just keep trying
				}
			}
			else
			{
				fail=true;
				numBytes=0;												// we have failed to insert any bytes
			}

			currentView=theBuffer->firstView;
			while(currentView)											// walk through all views, see if any need attention
			{
				if(endOffset>=currentView->startPosition)				// make sure change is not completely before the start, if it is, we just have to update the topline, and the start and end positions
				{
					if(haveViewNeedingUpdate&&(startOffset<=currentView->endPosition))			// make sure change is not completely after the end, if it is, we can ignore it
					{
						AdjustViewForChange(currentView,startLine,endLine,(endLine+theBuffer->textUniverse->totalLines)-oldNumLines,currentView->updateStartX);
					}
				}
				else
				{
					currentView->startPosition-=(endOffset-startOffset);	// subtract for bytes deleted
					currentView->startPosition+=numBytes;					// add for bytes inserted
					currentView->endPosition-=(endOffset-startOffset);		// subtract for bytes deleted
					currentView->endPosition+=numBytes;						// add for bytes inserted
					GetEditorViewTextInfo(currentView,&topLine,&numLines,&leftPixel,&numPixels);
					SetEditorViewTopLine(currentView,topLine+theBuffer->textUniverse->totalLines-oldNumLines);	// adjust what we believe the top line is (change does not affect view)
				}
				currentView=currentView->nextBufferView;			// locate next view of this buffer
			}

			FixBufferSelections(theBuffer,startOffset,endOffset,startOffset+numBytes);

			if(!theBuffer->replaceHaveRange)						// if no changes yet, just log first change
			{
				theBuffer->replaceStartOffset=startOffset;
				theBuffer->replaceNumBytes=numBytes-(endOffset-startOffset);
				theBuffer->replaceEndOffset=endOffset+theBuffer->replaceNumBytes;
				theBuffer->replaceHaveRange=true;					// remember that we saw a replacement
			}
			else
			{
				if(startOffset<theBuffer->replaceStartOffset)		// expand the range of changes
				{
					theBuffer->replaceStartOffset=startOffset;
				}
				theBuffer->replaceNumBytes+=numBytes-(endOffset-startOffset);		// adjust for total number of bytes added/subtracted
				if(endOffset>theBuffer->replaceEndOffset)			// see if we need to push to a new end
				{
					theBuffer->replaceEndOffset=endOffset+numBytes-(endOffset-startOffset);	// move to new end
				}
				else
				{
					theBuffer->replaceEndOffset+=numBytes-(endOffset-startOffset);	// just adjust old end for new changes
				}
			}
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

bool ReplaceEditorText(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,UINT8 *theText,UINT32 numBytes)
// replace the text between startOffset and endOffset with numBytes of theText
// if there is a problem, set the error, and return false
{
	CHUNKHEADER
		psuedoChunk;														// used to create a chunk that points to theText

	psuedoChunk.previousHeader=psuedoChunk.nextHeader=NULL;					// set up fake chunk header, so we can call replace chunk routine
	psuedoChunk.data=theText;
	psuedoChunk.totalBytes=numBytes;
	psuedoChunk.totalLines=0;												// this does not have to be set accurately
	return(ReplaceEditorChunks(theBuffer,startOffset,endOffset,&psuedoChunk,0,numBytes));
}

bool ReplaceEditorFile(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,char *thePath)
// replace the text between startOffset and endOffset with the text at thePath
// if there is a problem, set the error, and return false
{
	CHUNKHEADER
		psuedoChunk;														// used to create a chunk that points to theText
	UINT8
		*dataBuffer;
	EDITORFILE
		*theFile;
	UINT32
		numRead;
	bool
		done,
		fail;

	fail=false;
	if((theFile=OpenEditorReadFile(thePath)))							// open the file to load from
	{
		if((dataBuffer=(UINT8 *)MNewPtr(LOADSAVEBUFFERSIZE)))			// create a buffer to read into
		{
			psuedoChunk.previousHeader=psuedoChunk.nextHeader=NULL;		// set up fake chunk header, so we can call replace chunk routine
			psuedoChunk.data=dataBuffer;
			psuedoChunk.totalLines=0;									// this does not have to be set accurately
			done=false;
			while(!done&&!fail)											// read until we reach the end of the file
			{
				if(ReadEditorFile(theFile,dataBuffer,LOADSAVEBUFFERSIZE,&numRead))	// inhale as many bytes as we can fit into the buffer
				{
					psuedoChunk.totalBytes=numRead;
					if(ReplaceEditorChunks(theBuffer,startOffset,endOffset,&psuedoChunk,0,numRead))	// take the bytes we got, and insert them into the buffer
					{
						startOffset+=numRead;							// move these for next time
						endOffset=startOffset;
						if(!numRead)									// if no bytes were read, but we did not fail, then we are at the end of the file
						{
							done=true;
						}
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					fail=true;
				}
			}
			MDisposePtr(dataBuffer);
		}
		else
		{
			fail=true;
		}
		CloseEditorFile(theFile);
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

void EditorStartReplace(EDITORBUFFER *theBuffer)
// call this when about to begin replacing editor text
// it will begin the process of remembering what has changed, so that the updates
// can happen at the end
// The styles and selection of the replaced text is allowed to be altered
// during this process, however all other styles and selections must remain
// untouched
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;
	EDITORVIEW
		*currentView;

	UpdateEditorWindows();								// update the windows, making sure to update any view that has been invalidated
	EditorStartTextChange(theBuffer);					// we are going to change text in a buffer (turn off cursors, and selections, and the like)

	currentView=theBuffer->firstView;
	while(currentView)									// walk through all views, and collect information about them that will help speed the replace
	{
		GetEditorViewTextInfo(currentView,&topLine,&numLines,&leftPixel,&numPixels);
		LineToChunkPosition(theBuffer->textUniverse,topLine,&theChunk,&theOffset,&(currentView->startPosition));	// get the text position of the first character in the view
		LineToChunkPosition(theBuffer->textUniverse,topLine+numLines,&theChunk,&theOffset,&(currentView->endPosition));	// get the text position of the first character just off the view
		currentView=currentView->nextBufferView;		// locate next view of this buffer
	}
	theBuffer->replaceHaveRange=false;
	theBuffer->replaceStartOffset=0;
	theBuffer->replaceEndOffset=0;
	theBuffer->replaceNumBytes=0;
}

void EditorEndReplace(EDITORBUFFER *theBuffer)
// when a replacement has been completed, call this, and it will
// update all the views as needed
{
	UINT32
		endOffset,
		numBytes;

	if(theBuffer->replaceHaveRange)					// did a replacement actually occur?
	{
		endOffset=theBuffer->replaceEndOffset-theBuffer->replaceNumBytes;	// convert current end offset to end offset as it would have looked before the text changed
		numBytes=theBuffer->replaceEndOffset-theBuffer->replaceStartOffset;	// convert this to a number of bytes of replacement of original text
		UpdateSyntaxInformation(theBuffer,theBuffer->replaceStartOffset,endOffset,numBytes);
	}

	UpdateEditorWindows();								// update the windows, making sure to update any view that has been invalidated
	EditorEndTextChange(theBuffer);
}

// --------------------------------------------------------------------------------------------------------------------------

void DeleteAllSelectedText(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse)
// delete all the selected text from the passed buffer, place the
// cursor at the start of the first selection deleted
// EditorStartReplace and EditorEndReplace should be called externally
{
	UINT32
		finalCursorPosition,
		endOffset;
	UINT32
		startPosition,
		selectionLength;
	bool
		fail;

	fail=false;
	if(!IsSelectionEmpty(theSelectionUniverse))
	{
		GetSelectionEndPositions(theSelectionUniverse,&finalCursorPosition,&endOffset);
		startPosition=0;
		while(!fail&&GetSelectionAtOrAfterPosition(theSelectionUniverse,0,&startPosition,&selectionLength))
		{
			fail=!ReplaceEditorText(theBuffer,startPosition,startPosition+selectionLength,NULL,0);
		}
		SetSelectionCursorPosition(theSelectionUniverse,finalCursorPosition);
	}

	if(fail)
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("Failed to delete: %.256s\n",errorDescription);
	}
}

void EditorGetSelectionInfo(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,UINT32 *startPosition,UINT32 *endPosition,UINT32 *startLine,UINT32 *endLine,UINT32 *startLinePosition,UINT32 *endLinePosition,UINT32 *totalSegments,UINT32 *totalSpan)
// get useful information about the given selection
{
	CHUNKHEADER
		*textChunk;
	UINT32
		textOffset;
	UINT32
		currentPosition,
		currentLength;

	(*totalSpan)=0;
	(*totalSegments)=0;
	if(!IsSelectionEmpty(theSelectionUniverse))
	{
		GetSelectionEndPositions(theSelectionUniverse,startPosition,endPosition);
		currentPosition=0;
		while(GetSelectionAtOrAfterPosition(theSelectionUniverse,currentPosition,&currentPosition,&currentLength))
		{
			(*totalSegments)++;
			(*totalSpan)+=currentLength;
			currentPosition+=currentLength;
		}
	}
	else
	{
		(*startPosition)=(*endPosition)=GetSelectionCursorPosition(theSelectionUniverse);
	}
	PositionToLinePosition(theBuffer->textUniverse,*startPosition,startLine,startLinePosition,&textChunk,&textOffset);
	PositionToLinePosition(theBuffer->textUniverse,*endPosition,endLine,endLinePosition,&textChunk,&textOffset);
}

UINT8 *EditorNextSelectionToBuffer(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,UINT32 *currentPosition,UINT32 additionalLength,UINT32 *actualLength,bool *atEnd)
// create a buffer the size of the current selection in theBuffer + additionalLength
// then read the bytes of the selection into the buffer, returning a pointer
// to the buffer. The buffer must be disposed of by the caller using MDisposePtr.
// If there is a problem, set the error, and return NULL
// If atEnd is true, there are no more selections in the list
// If there are no selections, return NULL, and set atEnd to true (do not SetError)
// NOTE: if NULL comes back, and atEnd is false, it indicates an error
{
	CHUNKHEADER
		*sourceChunk;
	UINT32
		sourceChunkOffset;
	UINT32
		numElements;
	UINT8
		*dataBuffer;

	(*atEnd)=false;
	if(GetSelectionAtOrAfterPosition(theSelectionUniverse,*currentPosition,currentPosition,&numElements))	// see if there are selections
	{
		PositionToChunkPosition(theBuffer->textUniverse,*currentPosition,&sourceChunk,&sourceChunkOffset);	// point to the start of the selection in the source data
		(*actualLength)=numElements+additionalLength;
		if((dataBuffer=(UINT8 *)MNewPtr(*actualLength)))
		{
			if(ExtractUniverseText(theBuffer->textUniverse,sourceChunk,sourceChunkOffset,dataBuffer,numElements,&sourceChunk,&sourceChunkOffset))
			{
				(*currentPosition)+=numElements;			// update position
				return(dataBuffer);
			}
			MDisposePtr(dataBuffer);
		}
	}
	else
	{
		(*atEnd)=true;
	}
	return(NULL);
}

// cursor position/movement routines

void EditorDeleteSelection(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse)
// delete all the selected text in theBuffer specified by theSelectionUniverse
{
	EditorStartReplace(theBuffer);
	BeginUndoGroup(theBuffer);
	DeleteAllSelectedText(theBuffer,theSelectionUniverse);
	StrictEndUndoGroup(theBuffer);
	EditorEndReplace(theBuffer);
}

static bool IsWordSpace(UINT8 theChar)
// returns true if theChar is considered to be the space between words
{
	return((bool)wordSpaceTable[theChar]);
}

static UINT32 NextCharLeft(EDITORVIEW *theView,UINT32 thePosition)
// return the position one character to the left of thePosition
{
	if(thePosition)
	{
		return(thePosition-1);
	}
	else
	{
		return(thePosition);
	}
}

static UINT32 NextCharRight(EDITORVIEW *theView,UINT32 thePosition)
// return the position one character to the right of thePosition
{
	if(thePosition<theView->parentBuffer->textUniverse->totalBytes)
	{
		return(thePosition+1);
	}
	else
	{
		return(thePosition);
	}
}

static UINT32 NextWordLeft(EDITORVIEW *theView,UINT32 thePosition)
// return the position one word to the left of thePosition
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		found;

	if(thePosition)
	{
		PositionToChunkPosition(theView->parentBuffer->textUniverse,thePosition-1,&theChunk,&theOffset);		// look at the character in question
		found=false;
		if(theChunk)
		{
			thePosition--;
			if(IsWordSpace(theChunk->data[theOffset]))
			{
				found=true;
			}
		}
		while(theChunk&&!found)
		{
			if(theOffset)
			{
				theOffset--;
			}
			else
			{
				if((theChunk=theChunk->previousHeader))
				{
					theOffset=theChunk->totalBytes-1;
				}
			}
			if(theChunk)
			{
				if(IsWordSpace(theChunk->data[theOffset]))
				{
					found=true;
				}
				else
				{
					thePosition--;
				}
			}
		}
	}
	return(thePosition);
}

static UINT32 NextWordRight(EDITORVIEW *theView,UINT32 thePosition)
// return the position one word to the right of thePosition
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		found;

	PositionToChunkPosition(theView->parentBuffer->textUniverse,thePosition,&theChunk,&theOffset);
	found=false;
	if(theChunk)
	{
		thePosition++;
		if(IsWordSpace(theChunk->data[theOffset]))
		{
			found=true;
		}
		else
		{
			theOffset++;
			if(theOffset>=theChunk->totalBytes)
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
	}
	while(theChunk&&!found)
	{
		if(IsWordSpace(theChunk->data[theOffset]))
		{
			found=true;
		}
		else
		{
			thePosition++;
			theOffset++;
			if(theOffset>=theChunk->totalBytes)
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
	}
	return(thePosition);
}

static UINT32 NextLineUp(EDITORVIEW *theView,UINT32 thePosition,bool *haveX,INT32 *desiredX)
// return the position above thePosition
{
	UINT32
		newPosition;
	UINT32
		theLine,
		theLineOffset,
		temp;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;

	PositionToLinePosition(theView->parentBuffer->textUniverse,thePosition,&theLine,&theLineOffset,&theChunk,&theOffset);
	if(theLine)
	{
		if(!(*haveX))
		{
			GetEditorViewTextToGraphicPosition(theView,thePosition,desiredX,false,&temp,&temp);
			*haveX=true;
		}

		LineToChunkPosition(theView->parentBuffer->textUniverse,theLine-1,&theChunk,&theOffset,&newPosition);
		GetEditorViewGraphicToTextPosition(theView,newPosition,*desiredX,&theLineOffset,&temp);

		return(newPosition+theLineOffset);
	}
	else
	{
		return(thePosition);			// there is no line above this one
	}
}

static UINT32 NextLineDown(EDITORVIEW *theView,UINT32 thePosition,bool *haveX,INT32 *desiredX)
// return the position below thePosition
{
	UINT32
		newPosition;
	UINT32
		theLine,
		theLineOffset,
		temp;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;

	PositionToLinePosition(theView->parentBuffer->textUniverse,thePosition,&theLine,&theLineOffset,&theChunk,&theOffset);
	if(theLine<theView->parentBuffer->textUniverse->totalLines)
	{
		if(!(*haveX))
		{
			GetEditorViewTextToGraphicPosition(theView,thePosition,desiredX,false,&temp,&temp);
			*haveX=true;
		}
		LineToChunkPosition(theView->parentBuffer->textUniverse,theLine+1,&theChunk,&theOffset,&newPosition);
		GetEditorViewGraphicToTextPosition(theView,newPosition,*desiredX,&theLineOffset,&temp);
		return(newPosition+theLineOffset);
	}
	else
	{
		return(thePosition);			// there is no line below this one
	}
}

static UINT32 NextLineLeft(EDITORVIEW *theView,UINT32 thePosition)
// return the position of the start of the line of thePosition
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		found;

	if(thePosition)
	{
		PositionToChunkPosition(theView->parentBuffer->textUniverse,thePosition-1,&theChunk,&theOffset);		// ignore starting character
		found=false;
		while(theChunk&&!found)
		{
			if(theChunk->data[theOffset]=='\n')
			{
				found=true;
			}
			else
			{
				thePosition--;
				if(theOffset)
				{
					theOffset--;
				}
				else
				{
					if((theChunk=theChunk->previousHeader))
					{
						theOffset=theChunk->totalBytes-1;
					}
				}
			}
		}
	}
	return(thePosition);
}

static UINT32 NextLineRight(EDITORVIEW *theView,UINT32 thePosition)
// return the position of the end of the line of thePosition
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		found;

	PositionToChunkPosition(theView->parentBuffer->textUniverse,thePosition,&theChunk,&theOffset);
	found=false;
	while(theChunk&&!found)
	{
		if(theChunk->data[theOffset]=='\n')
		{
			found=true;
		}
		else
		{
			thePosition++;
			theOffset++;
			if(theOffset>=theChunk->totalBytes)
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
	}
	return(thePosition);
}

static UINT32 NextParagraphLeft(EDITORVIEW *theView,UINT32 thePosition)
// return the position of the start of the paragraph of thePosition
// NOTE: a paragraph is considered to be the text between
// empty lines
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		locatedPosition;
	bool
		foundPrevious,
		found;

	if((locatedPosition=thePosition))
	{
		found=foundPrevious=false;
		if(thePosition>=theView->parentBuffer->textUniverse->totalBytes)	// if searching back from the end, then step back
		{
			thePosition=theView->parentBuffer->textUniverse->totalBytes-1;
			foundPrevious=true;
		}
		PositionToChunkPosition(theView->parentBuffer->textUniverse,thePosition,&theChunk,&theOffset);
		while(theChunk&&!found)
		{
			if(theChunk->data[theOffset]=='\n'&&foundPrevious)
			{
				found=true;
			}
			else
			{
				if(!(foundPrevious=(theChunk->data[theOffset]=='\n')))
				{
					locatedPosition=thePosition;		// move to this place
				}
				thePosition--;
				if(theOffset)
				{
					theOffset--;
				}
				else
				{
					if((theChunk=theChunk->previousHeader))
					{
						theOffset=theChunk->totalBytes-1;
					}
				}
			}
		}
	}
	return(locatedPosition);
}

static UINT32 NextParagraphRight(EDITORVIEW *theView,UINT32 thePosition)
// return the position of the end of the paragraph of thePosition
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		locatedPosition;
	bool
		foundPrevious,
		found;

	found=foundPrevious=false;
	locatedPosition=thePosition;
	if(thePosition)
	{
		thePosition--;								// step back one
	}
	else
	{
		foundPrevious=true;							// if at start, then it is like having a newline previously
	}
	PositionToChunkPosition(theView->parentBuffer->textUniverse,thePosition,&theChunk,&theOffset);
	while(theChunk&&!found)
	{
		if(theChunk->data[theOffset]=='\n'&&foundPrevious)
		{
			found=true;
		}
		else
		{
			foundPrevious=(theChunk->data[theOffset]=='\n');
			thePosition++;
			if(!foundPrevious)
			{
				locatedPosition=thePosition;		// move to this place
			}
			theOffset++;
			if(theOffset>=theChunk->totalBytes)
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
	}
	return(locatedPosition);
}

static UINT32 NextPageUp(EDITORVIEW *theView,UINT32 thePosition,bool *haveX,INT32 *desiredX)
// return the position that is one page up from thePosition
{
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;
	UINT32
		newPosition;
	UINT32
		theLine,
		theLineOffset,
		temp;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	PositionToLinePosition(theView->parentBuffer->textUniverse,thePosition,&theLine,&theLineOffset,&theChunk,&theOffset);
	if(numLines)
	{
		if(theLine>numLines-1)
		{
			theLine-=numLines-1;
		}
		else
		{
			theLine=0;
		}
	}
	if(!(*haveX))
	{
		GetEditorViewTextToGraphicPosition(theView,thePosition,desiredX,false,&temp,&temp);
		*haveX=true;
	}

	LineToChunkPosition(theView->parentBuffer->textUniverse,theLine,&theChunk,&theOffset,&newPosition);
	GetEditorViewGraphicToTextPosition(theView,newPosition,*desiredX,&theLineOffset,&temp);

	return(newPosition+theLineOffset);
}

static UINT32 NextPageDown(EDITORVIEW *theView,UINT32 thePosition,bool *haveX,INT32 *desiredX)
// return the position that is one page down from thePosition
{
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;
	UINT32
		newPosition;
	UINT32
		theLine,
		theLineOffset,
		temp;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	PositionToLinePosition(theView->parentBuffer->textUniverse,thePosition,&theLine,&theLineOffset,&theChunk,&theOffset);
	if(numLines)
	{
		theLine+=numLines-1;
	}
	if(theLine>theView->parentBuffer->textUniverse->totalLines)
	{
		theLine=theView->parentBuffer->textUniverse->totalLines;
	}
	if(!(*haveX))
	{
		GetEditorViewTextToGraphicPosition(theView,thePosition,desiredX,false,&temp,&temp);
		*haveX=true;
	}

	LineToChunkPosition(theView->parentBuffer->textUniverse,theLine,&theChunk,&theOffset,&newPosition);
	GetEditorViewGraphicToTextPosition(theView,newPosition,*desiredX,&theLineOffset,&temp);
	return(newPosition+theLineOffset);
}

static UINT32 GetRelativePosition(EDITORVIEW *theView,UINT32 thePosition,UINT16 relativeMode,bool *haveX,INT32 *desiredX)
// return the position that relates to thePosition in theView by relativeMode
// if haveX is true, desiredX is valid for moving up or down
// this will return an updated haveX/desiredX
{
	if(relativeMode&RPM_mBACKWARD)
	{
		switch(relativeMode&RPM_mMODE)
		{
			case RPM_CHAR:
				*haveX=false;
				return(NextCharLeft(theView,thePosition));
				break;
			case RPM_WORD:
				*haveX=false;
				return(NextWordLeft(theView,thePosition));
				break;
			case RPM_LINE:
				return(NextLineUp(theView,thePosition,haveX,desiredX));
				break;
			case RPM_LINEEDGE:
				*haveX=false;
				return(NextLineLeft(theView,thePosition));
				break;
			case RPM_PARAGRAPHEDGE:
				*haveX=false;
				return(NextParagraphLeft(theView,thePosition));
				break;
			case RPM_PAGE:
				return(NextPageUp(theView,thePosition,haveX,desiredX));
				break;
			case RPM_DOCEDGE:
				*haveX=false;
				return(0);
				break;
			default:
				*haveX=false;
				return(thePosition);
				break;
		}
	}
	else
	{
		switch(relativeMode&RPM_mMODE)
		{
			case RPM_CHAR:
				*haveX=false;
				return(NextCharRight(theView,thePosition));
				break;
			case RPM_WORD:
				*haveX=false;
				return(NextWordRight(theView,thePosition));
				break;
			case RPM_LINE:
				return(NextLineDown(theView,thePosition,haveX,desiredX));
				break;
			case RPM_LINEEDGE:
				*haveX=false;
				return(NextLineRight(theView,thePosition));
				break;
			case RPM_PARAGRAPHEDGE:
				*haveX=false;
				return(NextParagraphRight(theView,thePosition));
				break;
			case RPM_PAGE:
				return(NextPageDown(theView,thePosition,haveX,desiredX));
				break;
			case RPM_DOCEDGE:
				*haveX=false;
				return(theView->parentBuffer->textUniverse->totalBytes);
				break;
			default:
				*haveX=false;
				return(thePosition);
				break;
		}
	}
}

// selection boundary location routines

static UINT32 NoMove(TEXTUNIVERSE *theUniverse,UINT32 betweenPosition,UINT32 charPosition,UINT16 trackMode)
// used to not move on the given side of the selection
{
	return(betweenPosition);
}

static void ValueLeft(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT8 leftValue,UINT8 rightValue,UINT32 *newPosition)
// search to the left for the first unmatched leftValue
// adjust newPosition
// if leftValue and rightValue are the same, this will stop at the first occurrence
{
	UINT8
		theChar;
	UINT32
		matchCount;
	bool
		found;
	UINT32
		skippedBytes;

	matchCount=0;
	skippedBytes=0;
	found=false;
	while(theChunk&&!found)
	{
		if(theOffset)
		{
			theOffset--;
		}
		else
		{
			if((theChunk=theChunk->previousHeader))
			{
				theOffset=theChunk->totalBytes-1;
			}
		}
		if(theChunk)
		{
			theChar=theChunk->data[theOffset];
			if(theChar==leftValue)
			{
				if(matchCount)
				{
					matchCount--;
					skippedBytes++;
				}
				else
				{
					found=true;
				}
			}
			else
			{
				if(theChar==rightValue)
				{
					matchCount++;
				}
				skippedBytes++;
			}
		}
	}
	if(found)
	{
		(*newPosition)-=skippedBytes;
	}
}

static void ValueRight(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT8 leftValue,UINT8 rightValue,UINT32 *newPosition)
// search to the right for the first unmatched rightValue
// adjust newPosition
// if leftValue and rightValue are the same, this will stop at the first occurrence
{
	UINT8
		theChar;
	UINT32
		matchCount;
	bool
		found;
	UINT32
		skippedBytes;

	matchCount=0;
	skippedBytes=0;
	found=false;
	if(theChunk)				// always skip over the start
	{
		(*newPosition)++;
		theOffset++;
		if(theOffset>=theChunk->totalBytes)
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	while(theChunk&&!found)
	{
		theChar=theChunk->data[theOffset];
		if(theChar==rightValue)
		{
			if(matchCount)
			{
				matchCount--;
				skippedBytes++;
			}
			else
			{
				found=true;
			}
		}
		else
		{
			if(theChar==leftValue)
			{
				matchCount++;
			}
			skippedBytes++;
		}
		theOffset++;
		if(theOffset>=theChunk->totalBytes)
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	if(found)
	{
		(*newPosition)+=skippedBytes;
	}
}

static bool LocateSpecialWordMatchLeft(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 *newPosition)
// see if the character being selected is special, if so, do special matching on it
// this is used to match parenthesis, and brackets, and quotes, etc during the initial word
// select, subsequent tracking is done normally
{
	bool
		found;

	found=false;
	switch(theChunk->data[theOffset])
	{
		case '{':
			(*newPosition)++;
			found=true;
			break;
		case '}':
			ValueLeft(theUniverse,theChunk,theOffset,'{','}',newPosition);
			found=true;
			break;
		case '(':
			(*newPosition)++;
			found=true;
			break;
		case ')':
			ValueLeft(theUniverse,theChunk,theOffset,'(',')',newPosition);
			found=true;
			break;
		case '[':
			(*newPosition)++;
			found=true;
			break;
		case ']':
			ValueLeft(theUniverse,theChunk,theOffset,'[',']',newPosition);
			found=true;
			break;
		case '"':
			(*newPosition)++;
			found=true;
			break;
		case '/':																	// c style comment
			(*newPosition)++;
			found=true;
			break;
		case '\'':
			(*newPosition)++;
			found=true;
			break;
	}
	return(found);
}

static bool LocateSpecialWordMatchRight(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 *newPosition)
// see if the character being selected is special, if so, do special matching on it
// this is used to match parenthesis, and brackets, and quotes, etc during the initial word
// select, subsequent tracking is done normally
{
	bool
		found;

	found=false;
	switch(theChunk->data[theOffset])
	{
		case '}':
			found=true;
			break;
		case '{':
			ValueRight(theUniverse,theChunk,theOffset,'{','}',newPosition);
			found=true;
			break;
		case ')':
			found=true;
			break;
		case '(':
			ValueRight(theUniverse,theChunk,theOffset,'(',')',newPosition);
			found=true;
			break;
		case ']':
			found=true;
			break;
		case '[':
			ValueRight(theUniverse,theChunk,theOffset,'[',']',newPosition);
			found=true;
			break;
		case '"':
			ValueRight(theUniverse,theChunk,theOffset,'"','"',newPosition);
			found=true;
			break;
		case '/':																	// c style comment
			ValueRight(theUniverse,theChunk,theOffset,'/','/',newPosition);
			found=true;
			break;
		case '\'':
			ValueRight(theUniverse,theChunk,theOffset,'\'','\'',newPosition);
			found=true;
			break;
	}
	return(found);
}

static UINT32 WordLeft(TEXTUNIVERSE *theUniverse,UINT32 betweenPosition,UINT32 charPosition,UINT16 trackMode)
// locate next word boundary to the left
// return the new between position
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		found;

	PositionToChunkPosition(theUniverse,charPosition,&theChunk,&theOffset);		// look at the character in question
	found=false;
	if(theChunk)
	{
		if(!(trackMode&TM_mREPEAT))									// if not in repeat, then see if clicked on something we want to match
		{
			found=LocateSpecialWordMatchLeft(theUniverse,theChunk,theOffset,&charPosition);
		}
		if(!found)
		{
			if(IsWordSpace(theChunk->data[theOffset]))
			{
				found=true;
			}
			else
			{
				if(theOffset)
				{
					theOffset--;
				}
				else
				{
					if((theChunk=theChunk->previousHeader))
					{
						theOffset=theChunk->totalBytes-1;
					}
				}
			}
		}
	}
	while(theChunk&&!found)
	{
		if(IsWordSpace(theChunk->data[theOffset]))
		{
			found=true;
		}
		else
		{
			charPosition--;
			if(theOffset)
			{
				theOffset--;
			}
			else
			{
				if((theChunk=theChunk->previousHeader))
				{
					theOffset=theChunk->totalBytes-1;
				}
			}
		}
	}
	return(charPosition);
}

static UINT32 WordRight(TEXTUNIVERSE *theUniverse,UINT32 betweenPosition,UINT32 charPosition,UINT16 trackMode)
// locate next word boundary to the right
// return the new between position
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		found;

	PositionToChunkPosition(theUniverse,charPosition,&theChunk,&theOffset);
	found=false;
	if(theChunk)
	{
		if(!(trackMode&TM_mREPEAT))									// if not in repeat, then see if clicked on something we want to match
		{
			found=LocateSpecialWordMatchRight(theUniverse,theChunk,theOffset,&charPosition);
		}
		if(!found)
		{
			charPosition++;
			if(IsWordSpace(theChunk->data[theOffset]))
			{
				found=true;
			}
			else
			{
				theOffset++;
				if(theOffset>=theChunk->totalBytes)
				{
					theChunk=theChunk->nextHeader;
					theOffset=0;
				}
			}
		}
	}
	while(theChunk&&!found)
	{
		if(IsWordSpace(theChunk->data[theOffset]))
		{
			found=true;
		}
		else
		{
			charPosition++;
			theOffset++;
			if(theOffset>=theChunk->totalBytes)
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
	}
	return(charPosition);
}

static UINT32 LineLeft(TEXTUNIVERSE *theUniverse,UINT32 betweenPosition,UINT32 charPosition,UINT16 trackMode)
// locate next line boundary to the left, and including charPosition
// return the new between position
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		found;

	if(charPosition)
	{
		PositionToChunkPosition(theUniverse,charPosition-1,&theChunk,&theOffset);		// ignore starting character
		found=false;
		while(theChunk&&!found)
		{
			if(theChunk->data[theOffset]=='\n')
			{
				found=true;
			}
			else
			{
				charPosition--;
				if(theOffset)
				{
					theOffset--;
				}
				else
				{
					if((theChunk=theChunk->previousHeader))
					{
						theOffset=theChunk->totalBytes-1;
					}
				}
			}
		}
	}
	return(charPosition);
}

static UINT32 LineRight(TEXTUNIVERSE *theUniverse,UINT32 betweenPosition,UINT32 charPosition,UINT16 trackMode)
// locate next line boundary to the right, and including charPosition
// return the new between position
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		found;

	PositionToChunkPosition(theUniverse,charPosition,&theChunk,&theOffset);
	found=false;
	while(theChunk&&!found)
	{
		charPosition++;
		if(theChunk->data[theOffset]=='\n')
		{
			found=true;
		}
		else
		{
			theOffset++;
			if(theOffset>=theChunk->totalBytes)
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
	}
	return(charPosition);
}

static UINT32 AllLeft(TEXTUNIVERSE *theUniverse,UINT32 betweenPosition,UINT32 charPosition,UINT16 trackMode)
// move back to the start
// return the new between position
{
	return(0);
}

static UINT32 AllRight(TEXTUNIVERSE *theUniverse,UINT32 betweenPosition,UINT32 charPosition,UINT16 trackMode)
// locate the end
// return the new between position
{
	return(theUniverse->totalBytes);
}

// columnar selection functions
// NOTE: these are different from the functions above which return absolute positions in the
// text universe. These return offsets relative to the start of the line in question.

static UINT32 CNoMove(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 betweenPosition,UINT32 charPosition)
// used to not move on the given side of the selection
{
	return(betweenPosition);
}

static UINT32 ColumnarWordLeft(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 betweenPosition,UINT32 charPosition)
// locate next word boundary to the left
// return the new between position
{
	bool
		found;

	AddToChunkPosition(theUniverse,theChunk,theOffset,&theChunk,&theOffset,charPosition);		// look at the character in question
	found=false;
	if(theChunk&&charPosition)
	{
		if(IsWordSpace(theChunk->data[theOffset]))
		{
			found=true;
		}
		else
		{
			if(theOffset)
			{
				theOffset--;
			}
			else
			{
				if((theChunk=theChunk->previousHeader))
				{
					theOffset=theChunk->totalBytes-1;
				}
			}
		}
	}
	while(theChunk&&!found&&charPosition)
	{
		if(IsWordSpace(theChunk->data[theOffset]))
		{
			found=true;
		}
		else
		{
			charPosition--;
			if(theOffset)
			{
				theOffset--;
			}
			else
			{
				if((theChunk=theChunk->previousHeader))
				{
					theOffset=theChunk->totalBytes-1;
				}
			}
		}
	}
	return(charPosition);
}

static UINT32 ColumnarWordRight(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 betweenPosition,UINT32 charPosition)
// locate next word boundary to the right
// return the new between position
{
	bool
		found;

	AddToChunkPosition(theUniverse,theChunk,theOffset,&theChunk,&theOffset,charPosition);		// look at the character in question
	found=false;
	if(theChunk)
	{
		if(theChunk->data[theOffset]!='\n')					// if over the CR, do not select it in columnar mode
		{
			charPosition++;
			if(IsWordSpace(theChunk->data[theOffset]))
			{
				found=true;
			}
			else
			{
				theOffset++;
				if(theOffset>=theChunk->totalBytes)
				{
					theChunk=theChunk->nextHeader;
					theOffset=0;
				}
			}
		}
		else
		{
			found=true;
		}
	}
	while(theChunk&&!found)
	{
		if(IsWordSpace(theChunk->data[theOffset])||theChunk->data[theOffset]=='\n')
		{
			found=true;
		}
		else
		{
			charPosition++;
			theOffset++;
			if(theOffset>=theChunk->totalBytes)
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
	}
	return(charPosition);
}

static UINT32 ColumnarLineLeft(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 betweenPosition,UINT32 charPosition)
// locate next line boundary to the left, and including charPosition
// return the new between position
{
	return(0);
}

static UINT32 ColumnarLineRight(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 betweenPosition,UINT32 charPosition)
// locate next line boundary to the right, and including charPosition
// return the new between position
{
	bool
		found;

	AddToChunkPosition(theUniverse,theChunk,theOffset,&theChunk,&theOffset,charPosition);		// look at the character in question
	found=false;
	while(theChunk&&!found)
	{
		if(theChunk->data[theOffset]=='\n')
		{
			found=true;
		}
		else
		{
			charPosition++;
			theOffset++;
			if(theOffset>=theChunk->totalBytes)
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
	}
	return(charPosition);
}

static void AdjustColumnarRect(EDITORBUFFER *theBuffer,UINT32 theLine,INT32 theXPosition,UINT16 trackMode)
// adjust the columnar selection rectangle bounds based on the new position, and trackMode
{
	if(!(trackMode&TM_mREPEAT))				// during repeat, nothing is ever done to the anchor position
	{
		if(trackMode&TM_mCONTINUE)			// to continue find out which side of the rect is closest to the point, and set the anchor to opposite end
		{
			if(theLine<theBuffer->columnarTopLine)
			{
				theBuffer->anchorLine=theBuffer->columnarBottomLine;
			}
			else
			{
				if(theLine>theBuffer->columnarBottomLine)
				{
					theBuffer->anchorLine=theBuffer->columnarTopLine;
				}
				else
				{
					if(theBuffer->columnarBottomLine-theLine>=theLine-theBuffer->columnarTopLine)
					{
						theBuffer->anchorLine=theBuffer->columnarBottomLine;
					}
					else
					{
						theBuffer->anchorLine=theBuffer->columnarTopLine;
					}
				}
			}
			if(theXPosition<theBuffer->columnarLeftX)
			{
				theBuffer->anchorX=theBuffer->columnarRightX;
			}
			else
			{
				if(theXPosition>theBuffer->columnarRightX)
				{
					theBuffer->anchorX=theBuffer->columnarLeftX;
				}
				else
				{
					if(theBuffer->columnarRightX-theXPosition>=theXPosition-theBuffer->columnarLeftX)
					{
						theBuffer->anchorX=theBuffer->columnarRightX;
					}
					else
					{
						theBuffer->anchorX=theBuffer->columnarLeftX;
					}
				}
			}
		}
		else
		{
			theBuffer->anchorLine=theLine;		// set the columnar rectangle to empty rect centered at the given point, anchor tracking to this point
			theBuffer->anchorX=theXPosition;
		}
	}
	// now, use the anchor and the point to determine the top, left, bottom, and right
	if(theBuffer->anchorLine>=theLine)
	{
		theBuffer->columnarTopLine=theLine;
		theBuffer->columnarBottomLine=theBuffer->anchorLine;
	}
	else
	{
		theBuffer->columnarTopLine=theBuffer->anchorLine;
		theBuffer->columnarBottomLine=theLine;
	}
	if(theBuffer->anchorX>=theXPosition)
	{
		theBuffer->columnarLeftX=theXPosition;
		theBuffer->columnarRightX=theBuffer->anchorX;
	}
	else
	{
		theBuffer->columnarLeftX=theBuffer->anchorX;
		theBuffer->columnarRightX=theXPosition;
	}
}

void EditorSetNormalSelection(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,UINT32 startPosition,UINT32 endPosition)
// clear the current selection(s) if any, and set a new one
{
	bool
		fail;

	fail=false;

	EditorStartSelectionChange(theBuffer);
	DeleteAllSelections(theSelectionUniverse);	// remove all old selections
	if(startPosition<endPosition)			// if selection contains at least one character, then create a new array element
	{
		fail=!SetSelectionRange(theSelectionUniverse,startPosition,endPosition-startPosition);
	}
	else
	{
		SetSelectionCursorPosition(theSelectionUniverse,startPosition);		// set the cursor here
	}
	EditorEndSelectionChange(theBuffer);

	if(fail)
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("Failed to set selection: %.256s\n",errorDescription);
	}
}

void EditorSetColumnarSelection(EDITORVIEW *theView,UINT32 startLine,UINT32 endLine,INT32 leftX,INT32 rightX,UINT16 trackMode)
// clear the current selection(s) if any, and set a new columnar one
{
	EDITORBUFFER
		*theBuffer;
	SELECTIONUNIVERSE
		*theSelectionUniverse;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		distance,
		basePosition;
	UINT32
		currentPosition,
		startPosition,
		endPosition,
		startBetween,
		startChar,
		endBetween,
		endChar;
	UINT32
		currentLine;
	UINT32
		(*leftFunction)(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 betweenPosition,UINT32 charPosition),
		(*rightFunction)(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 betweenPosition,UINT32 charPosition);
	bool
		fail;

	theBuffer=theView->parentBuffer;
	theSelectionUniverse=theBuffer->selectionUniverse;

	switch(trackMode&TM_mMODE)
	{
		case TM_CHAR:
			leftFunction=rightFunction=CNoMove;
			break;
		case TM_WORD:
			leftFunction=ColumnarWordLeft;
			rightFunction=ColumnarWordRight;
			break;
		case TM_LINE:
			leftFunction=ColumnarLineLeft;
			rightFunction=ColumnarLineRight;
			break;
		case TM_ALL:
			leftFunction=ColumnarLineLeft;
			rightFunction=ColumnarLineRight;
			startLine=0;
			endLine=theBuffer->textUniverse->totalLines;
			break;
		default:
			leftFunction=rightFunction=CNoMove;
			break;
	}

	EditorStartSelectionChange(theBuffer);

	DeleteAllSelections(theSelectionUniverse);	// remove all old selections

	currentPosition=0;
	LineToChunkPosition(theBuffer->textUniverse,startLine,&theChunk,&theOffset,&basePosition);	// find start of top line

	fail=false;
	for(currentLine=startLine;!fail&&(currentLine<=endLine);currentLine++)
	{
		GetEditorViewGraphicToTextPosition(theView,basePosition,leftX,&startBetween,&startChar);
		GetEditorViewGraphicToTextPosition(theView,basePosition,rightX,&endBetween,&endChar);

		startPosition=basePosition+leftFunction(theBuffer->textUniverse,theChunk,theOffset,startBetween,startChar);
		endPosition=basePosition+rightFunction(theBuffer->textUniverse,theChunk,theOffset,endBetween,endChar);

		if(currentLine==startLine)
		{
			SetSelectionCursorPosition(theSelectionUniverse,startPosition);
		}

		if(startPosition<currentPosition)		// if this extends back before a previous selection, then start immediately following it
		{
			startPosition=currentPosition;
		}
		if(endPosition<currentPosition)			// if this is too far back, then pull forward
		{
			endPosition=currentPosition;
		}
		if(startPosition<endPosition)			// make sure there is something to select
		{
			fail=!SetSelectionRange(theSelectionUniverse,startPosition,endPosition-startPosition);
			currentPosition=endPosition;
		}
		ChunkPositionToNextLine(theBuffer->textUniverse,theChunk,theOffset,&theChunk,&theOffset,&distance);
		basePosition+=distance;
	}
	EditorEndSelectionChange(theBuffer);

	if(fail)
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("Failed to set selection: %.256s\n",errorDescription);
	}
}

static void EditorAdjustNormalSelection(EDITORVIEW *theView,UINT32 theLine,INT32 theXPosition,UINT16 trackMode)
// adjust/create the selection based on theLine, theXPosition, and trackMode
// if nothing would be selected, then place the cursor at the correct position
{
	EDITORBUFFER
		*theBuffer;
	SELECTIONUNIVERSE
		*theSelectionUniverse;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		base,
		betweenPosition,
		charPosition,
		startPosition,
		endPosition;
	UINT32
		(*leftFunction)(TEXTUNIVERSE *theUniverse,UINT32 betweenPosition,UINT32 charPosition,UINT16 trackMode),
		(*rightFunction)(TEXTUNIVERSE *theUniverse,UINT32 betweenPosition,UINT32 charPosition,UINT16 trackMode);

	theBuffer=theView->parentBuffer;
	theSelectionUniverse=theBuffer->selectionUniverse;

	switch(trackMode&TM_mMODE)
	{
		case TM_CHAR:
			leftFunction=rightFunction=NoMove;
			break;
		case TM_WORD:
			leftFunction=WordLeft;
			rightFunction=WordRight;
			break;
		case TM_LINE:
			leftFunction=LineLeft;
			rightFunction=LineRight;
			break;
		case TM_ALL:
			leftFunction=AllLeft;
			rightFunction=AllRight;
			break;
		default:
			leftFunction=rightFunction=NoMove;
			break;
	}

	AdjustColumnarRect(theBuffer,theLine,theXPosition,trackMode);	// keep columnar rect up to date (so if we switch modes, it is doing the right thing)

	LineToChunkPosition(theBuffer->textUniverse,theLine,&theChunk,&theOffset,&base);
	GetEditorViewGraphicToTextPosition(theView,base,theXPosition,&betweenPosition,&charPosition);

	betweenPosition+=base;
	charPosition+=base;

	// adjust the anchor
	if(!(trackMode&TM_mREPEAT))				// during repeat, nothing is ever done to the anchor position
	{
		if(trackMode&TM_mCONTINUE)
		{
			GetSelectionEndPositions(theSelectionUniverse,&startPosition,&endPosition);		// find ends of old selection
			if(betweenPosition<startPosition)
			{
				startPosition=leftFunction(theBuffer->textUniverse,betweenPosition,charPosition,trackMode);
				theBuffer->anchorStartPosition=theBuffer->anchorEndPosition=endPosition;
			}
			else
			{
				if(betweenPosition>endPosition)
				{
					endPosition=rightFunction(theBuffer->textUniverse,betweenPosition,charPosition,trackMode);
					theBuffer->anchorStartPosition=theBuffer->anchorEndPosition=startPosition;
				}
				else
				{
					if(endPosition-betweenPosition>=betweenPosition-startPosition)
					{
						startPosition=leftFunction(theBuffer->textUniverse,betweenPosition,charPosition,trackMode);
						theBuffer->anchorStartPosition=theBuffer->anchorEndPosition=endPosition;
					}
					else
					{
						endPosition=rightFunction(theBuffer->textUniverse,betweenPosition,charPosition,trackMode);
						theBuffer->anchorStartPosition=theBuffer->anchorEndPosition=startPosition;
					}
				}
			}
		}
		else		// selection is beginning here
		{
			startPosition=leftFunction(theBuffer->textUniverse,betweenPosition,charPosition,trackMode);
			endPosition=rightFunction(theBuffer->textUniverse,betweenPosition,charPosition,trackMode);
			theBuffer->anchorStartPosition=startPosition;	// if starting a new selection, and not continuing from an old one, then drop anchor on the selection
			theBuffer->anchorEndPosition=endPosition;
		}
	}
	else
	{
		startPosition=leftFunction(theBuffer->textUniverse,betweenPosition,charPosition,trackMode);
		endPosition=rightFunction(theBuffer->textUniverse,betweenPosition,charPosition,trackMode);
		if(startPosition>theBuffer->anchorStartPosition)
		{
			startPosition=theBuffer->anchorStartPosition;
		}
		if(endPosition<theBuffer->anchorEndPosition)
		{
			endPosition=theBuffer->anchorEndPosition;
		}
	}
	EditorSetNormalSelection(theBuffer,theSelectionUniverse,startPosition,endPosition);
}

static void EditorAdjustColumnarSelection(EDITORVIEW *theView,UINT32 theLine,INT32 theXPosition,UINT16 trackMode)
// adjust the columnar selection in the view based on the given global coordinates, and
// trackMode
{
	EDITORBUFFER
		*theBuffer;

	theBuffer=theView->parentBuffer;

	AdjustColumnarRect(theBuffer,theLine,theXPosition,trackMode);
	EditorSetColumnarSelection(theView,theBuffer->columnarTopLine,theBuffer->columnarBottomLine,theBuffer->columnarLeftX,theBuffer->columnarRightX,trackMode);
}

static void ScrollViewToFollowPointer(EDITORVIEW *theView,INT32 viewXPosition,INT32 viewLine,UINT16 trackMode)
// given a view relative line and X offset and the current tracking mode, scroll the view
{
	UINT32
		topLine,
		numLines,
		numPixels,
		newTopLine;
	INT32
		leftPixel,
		newLeftPixel;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	if(viewLine<0)
	{
		if(topLine)
		{
			newTopLine=topLine-1;
		}
		else
		{
			newTopLine=0;
		}
	}
	else
	{
		if(viewLine>=(int)numLines)
		{
			if(topLine<theView->parentBuffer->textUniverse->totalLines)
			{
				newTopLine=topLine+1;
			}
			else
			{
				newTopLine=theView->parentBuffer->textUniverse->totalLines;
			}
		}
		else
		{
			newTopLine=topLine;
		}
	}
	if(viewXPosition<0)
	{
		if(leftPixel-HORIZONTALSCROLLTHRESHOLD>0)
		{
			newLeftPixel=leftPixel-HORIZONTALSCROLLTHRESHOLD;
		}
		else
		{
			newLeftPixel=0;
		}
	}
	else
	{
		if(viewXPosition>=(int)numPixels)
		{
			newLeftPixel=leftPixel+HORIZONTALSCROLLTHRESHOLD;
		}
		else
		{
			newLeftPixel=leftPixel;
		}
	}
	if((newTopLine!=topLine)||(newLeftPixel!=leftPixel))
	{
		SetViewTopLeft(theView,newTopLine,newLeftPixel);
	}
}

static void EditorTrackViewPointerNormal(EDITORVIEW *theView,INT32 viewXPosition,INT32 viewLine,UINT16 trackMode)
// track the pointer in non-columnar mode
{
	UINT32
		topLine,
		numLines,
		numPixels;
	UINT32
		newLine;
	INT32
		leftPixel,
		newLeftPixel;
	static UINT32
		lastLine;
	static INT32
		lastLeftPixel;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	newLeftPixel=leftPixel+viewXPosition;
	if(viewLine>=0)
	{
		newLine=topLine+viewLine;
	}
	else
	{
		newLine=-viewLine;
		if(newLine<topLine)
		{
			newLine=topLine-newLine;
		}
		else
		{
			newLine=0;
			newLeftPixel=0;
		}
	}

	if((!(trackMode&TM_mREPEAT))||(newLine!=lastLine)||(newLeftPixel!=lastLeftPixel))	// if first time, or point has moved, then track it
	{
		lastLine=newLine;
		lastLeftPixel=newLeftPixel;
		EditorAdjustNormalSelection(theView,newLine,newLeftPixel,trackMode);
	}
	ScrollViewToFollowPointer(theView,viewXPosition,viewLine,trackMode);
}

static void EditorTrackViewPointerColumnar(EDITORVIEW *theView,INT32 viewXPosition,INT32 viewLine,UINT16 trackMode)
// track the pointer in columnar mode
{
	UINT32
		topLine,
		numLines,
		numPixels;
	UINT32
		newLine;
	INT32
		leftPixel,
		newLeftPixel;
	static UINT32
		lastLine;
	static INT32
		lastLeftPixel;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);
	newLeftPixel=leftPixel+viewXPosition;
	if(viewLine>=0)
	{
		newLine=topLine+viewLine;
	}
	else
	{
		newLine=-viewLine;
		if(newLine<topLine)
		{
			newLine=topLine-newLine;
		}
		else
		{
			newLine=0;
		}
	}

	if((!(trackMode&TM_mREPEAT))||(newLine!=lastLine)||(newLeftPixel!=lastLeftPixel))	// if first time, or point has moved, then track it
	{
		lastLine=newLine;
		lastLeftPixel=newLeftPixel;
		EditorAdjustColumnarSelection(theView,newLine,newLeftPixel,trackMode);
	}
	ScrollViewToFollowPointer(theView,viewXPosition,viewLine,trackMode);
}

static void EditorTrackViewPointerHand(EDITORVIEW *theView,INT32 viewXPosition,INT32 viewLine,UINT16 trackMode)
// track the pointer in hand mode
{
	static INT32
		baseViewXPosition,
		baseViewTopLine,
		baseXPosition,
		baseLine;
	UINT32
		topLine,
		numLines,
		numPixels,
		newTopLine;
	INT32
		leftPixel,
		newLeftPixel;

	GetEditorViewTextInfo(theView,&topLine,&numLines,&leftPixel,&numPixels);

	if(!(trackMode&TM_mREPEAT))		// if first time through, get and remember the passed position
	{
		baseViewXPosition=leftPixel;	// remember where the view was when this started
		baseViewTopLine=topLine;
		baseXPosition=viewXPosition;	// remember where the pointer was when this started
		baseLine=viewLine;
	}

	if(baseViewTopLine-(viewLine-baseLine)<0)	// do the check this way, since newTopLine is unsigned
	{
		newTopLine=0;
	}
	else
	{
		newTopLine=baseViewTopLine-(viewLine-baseLine);
	}

	newLeftPixel=baseViewXPosition-(viewXPosition-baseXPosition);
	if(newLeftPixel<0)
	{
		newLeftPixel=0;
	}

	if((newTopLine!=topLine)||(newLeftPixel!=leftPixel))
	{
		SetViewTopLeft(theView,newTopLine,newLeftPixel);
	}
}

void EditorTrackViewPointer(EDITORVIEW *theView,INT32 viewXPosition,INT32 viewLine,UINT16 trackMode)
// track the pointer in theView, according to the current cursor/selection in theView
// and trackMode
{
	if(trackMode&TM_mHAND)
	{
		EditorTrackViewPointerHand(theView,viewXPosition,viewLine,trackMode);
	}
	else if(trackMode&TM_mCOLUMNAR)
	{
		EditorTrackViewPointerColumnar(theView,viewXPosition,viewLine,trackMode);
	}
	else
	{
		EditorTrackViewPointerNormal(theView,viewXPosition,viewLine,trackMode);
	}
}

void EditorMoveCursor(EDITORVIEW *theView,UINT16 relativeMode)
// move the cursor according to relativeMode, unless there is a selection
// if there is a selection, send the cursor to the top if relativeMode is backward,
// send the cursor to the bottom of the selection if relativeMode is forward
{
	UINT32
		startPosition,
		endPosition;
	bool
		haveX;
	INT32
		desiredX;

	if(!IsSelectionEmpty(theView->parentBuffer->selectionUniverse))
	{
		GetSelectionEndPositions(theView->parentBuffer->selectionUniverse,&startPosition,&endPosition);		// find ends of old selection
		if(relativeMode&RPM_mBACKWARD)
		{
			EditorSetNormalSelection(theView->parentBuffer,theView->parentBuffer->selectionUniverse,startPosition,startPosition);
		}
		else
		{
			EditorSetNormalSelection(theView->parentBuffer,theView->parentBuffer->selectionUniverse,endPosition,endPosition);
		}
	}
	else
	{
		haveX=theView->parentBuffer->haveStartX;			// pass these to relative position routine
		desiredX=theView->parentBuffer->desiredStartX;
		startPosition=GetRelativePosition(theView,GetSelectionCursorPosition(theView->parentBuffer->selectionUniverse),relativeMode,&haveX,&desiredX);
		EditorSetNormalSelection(theView->parentBuffer,theView->parentBuffer->selectionUniverse,startPosition,startPosition);
		theView->parentBuffer->haveStartX=haveX;			// update these
		theView->parentBuffer->desiredStartX=desiredX;
	}
}

void EditorMoveSelection(EDITORVIEW *theView,UINT16 relativeMode)
// move the current end of the selection according to relativeMode
// NOTE: if there is no current end, then use relative mode to determine
// what the current end should be
{
	UINT32
		tempPosition,
		startPosition,
		endPosition;
	bool
		tempBool,
		haveStartX,
		haveEndX,
		haveCurrentEnd;
	INT32
		tempX,
		desiredStartX,
		desiredEndX;

	GetSelectionEndPositions(theView->parentBuffer->selectionUniverse,&startPosition,&endPosition);		// find ends of old selection

	haveStartX=theView->parentBuffer->haveStartX;
	desiredStartX=theView->parentBuffer->desiredStartX;
	haveEndX=theView->parentBuffer->haveEndX;
	desiredEndX=theView->parentBuffer->desiredEndX;

	haveCurrentEnd=theView->parentBuffer->haveCurrentEnd;

	if(!haveCurrentEnd)			// if there is no current end, take a cue from the desired direction
	{
		theView->parentBuffer->currentIsStart=false;
		if(relativeMode&RPM_mBACKWARD)
		{
			theView->parentBuffer->currentIsStart=true;
		}
		haveCurrentEnd=true;
	}

	if(theView->parentBuffer->currentIsStart)
	{
		startPosition=GetRelativePosition(theView,startPosition,relativeMode,&haveStartX,&desiredStartX);
	}
	else
	{
		endPosition=GetRelativePosition(theView,endPosition,relativeMode,&haveEndX,&desiredEndX);
	}

	if(startPosition>endPosition)			// have the ends swapped?
	{
		tempPosition=startPosition;			// swap them here
		startPosition=endPosition;
		endPosition=tempPosition;

		tempBool=haveStartX;
		haveStartX=haveEndX;
		haveEndX=tempBool;

		tempX=desiredStartX;
		desiredStartX=desiredEndX;
		desiredEndX=tempX;

		theView->parentBuffer->currentIsStart=!theView->parentBuffer->currentIsStart;	// swap the current end
	}

	EditorSetNormalSelection(theView->parentBuffer,theView->parentBuffer->selectionUniverse,startPosition,endPosition);

	theView->parentBuffer->haveStartX=haveStartX;				// update these
	theView->parentBuffer->desiredStartX=desiredStartX;
	theView->parentBuffer->haveEndX=haveEndX;
	theView->parentBuffer->desiredEndX=desiredEndX;

	theView->parentBuffer->haveCurrentEnd=haveCurrentEnd;
}

void EditorExpandNormalSelection(EDITORVIEW *theView,UINT16 relativeMode)
// expand the selection using relativeMode
{
	UINT32
		startPosition,
		endPosition;
	bool
		haveStartX,
		haveEndX;
	INT32
		desiredStartX,
		desiredEndX;

	GetSelectionEndPositions(theView->parentBuffer->selectionUniverse,&startPosition,&endPosition);		// find ends of old selection

	haveStartX=theView->parentBuffer->haveStartX;
	desiredStartX=theView->parentBuffer->desiredStartX;
	haveEndX=theView->parentBuffer->haveEndX;
	desiredEndX=theView->parentBuffer->desiredEndX;

	if(relativeMode&RPM_mBACKWARD)
	{
		startPosition=GetRelativePosition(theView,startPosition,relativeMode,&haveStartX,&desiredStartX);
		theView->parentBuffer->currentIsStart=true;
	}
	else
	{
		endPosition=GetRelativePosition(theView,endPosition,relativeMode,&haveEndX,&desiredEndX);
		theView->parentBuffer->currentIsStart=false;
	}
	EditorSetNormalSelection(theView->parentBuffer,theView->parentBuffer->selectionUniverse,startPosition,endPosition);

	theView->parentBuffer->haveStartX=haveStartX;				// update these
	theView->parentBuffer->desiredStartX=desiredStartX;
	theView->parentBuffer->haveEndX=haveEndX;
	theView->parentBuffer->desiredEndX=desiredEndX;

	theView->parentBuffer->haveCurrentEnd=true;
}

void EditorReduceNormalSelection(EDITORVIEW *theView,UINT16 relativeMode)
// reduce the selection using relativeMode
{
	UINT32
		startPosition,
		endPosition;
	bool
		haveStartX,
		haveEndX;
	INT32
		desiredStartX,
		desiredEndX;

	GetSelectionEndPositions(theView->parentBuffer->selectionUniverse,&startPosition,&endPosition);		// find ends of old selection

	haveStartX=theView->parentBuffer->haveStartX;
	desiredStartX=theView->parentBuffer->desiredStartX;
	haveEndX=theView->parentBuffer->haveEndX;
	desiredEndX=theView->parentBuffer->desiredEndX;

	if(relativeMode&RPM_mBACKWARD)
	{
		endPosition=GetRelativePosition(theView,endPosition,relativeMode,&haveEndX,&desiredEndX);
		if(endPosition<=startPosition)
		{
			haveStartX=haveEndX=false;
			endPosition=startPosition;
		}
		theView->parentBuffer->currentIsStart=false;
	}
	else
	{
		startPosition=GetRelativePosition(theView,startPosition,relativeMode,&haveStartX,&desiredStartX);
		if(startPosition>=endPosition)
		{
			haveStartX=haveEndX=false;
			startPosition=endPosition;
		}
		theView->parentBuffer->currentIsStart=true;
	}
	EditorSetNormalSelection(theView->parentBuffer,theView->parentBuffer->selectionUniverse,startPosition,endPosition);

	theView->parentBuffer->haveStartX=haveStartX;				// update these
	theView->parentBuffer->desiredStartX=desiredStartX;
	theView->parentBuffer->haveEndX=haveEndX;
	theView->parentBuffer->desiredEndX=desiredEndX;

	theView->parentBuffer->haveCurrentEnd=true;
}

void EditorLocateLine(EDITORBUFFER *theBuffer,UINT32 theLine)
// find the given line, and select it
// if the line is past the end of the text, select the end of the text
// NOTE: this thinks of lines as starting at 0, not 1
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		startPosition,
		endPosition;

	LineToChunkPosition(theBuffer->textUniverse,theLine,&theChunk,&theOffset,&startPosition);	// find start of given line line
	ChunkPositionToNextLine(theBuffer->textUniverse,theChunk,theOffset,&theChunk,&theOffset,&endPosition);
	endPosition+=startPosition;
	EditorSetNormalSelection(theBuffer,theBuffer->selectionUniverse,startPosition,endPosition);
}

void EditorSelectAll(EDITORBUFFER *theBuffer)
// Select all of the text in theBuffer
{
	EditorSetNormalSelection(theBuffer,theBuffer->selectionUniverse,0,theBuffer->textUniverse->totalBytes);
}

void EditorDelete(EDITORVIEW *theView,UINT16 relativeMode)
// delete the character(s) between the current cursor position, and relativeMode away from it
{
	EDITORBUFFER
		*theBuffer;
	SELECTIONUNIVERSE
		*theSelectionUniverse;
	UINT32
		relativePosition,
		startDelete,
		endDelete;
	bool
		haveX;
	INT32
		desiredX;
	bool
		fail;

	fail=false;
	theBuffer=theView->parentBuffer;
	theSelectionUniverse=theBuffer->selectionUniverse;
	EditorStartReplace(theBuffer);

	if(!IsSelectionEmpty(theSelectionUniverse))
	{
		BeginUndoGroup(theBuffer);
		DeleteAllSelectedText(theBuffer,theSelectionUniverse);
		StrictEndUndoGroup(theBuffer);
		haveX=false;
	}
	else
	{
		haveX=theBuffer->haveStartX;			// pass these to relative position routine
		desiredX=theBuffer->desiredStartX;
		relativePosition=GetRelativePosition(theView,GetSelectionCursorPosition(theSelectionUniverse),relativeMode,&haveX,&desiredX);

		if(relativePosition<GetSelectionCursorPosition(theSelectionUniverse))
		{
			startDelete=relativePosition;
			endDelete=GetSelectionCursorPosition(theSelectionUniverse);
		}
		else
		{
			startDelete=GetSelectionCursorPosition(theSelectionUniverse);
			endDelete=relativePosition;
		}
		if(startDelete<endDelete)
		{
			if(!ReplaceEditorText(theBuffer,startDelete,endDelete,NULL,0))
			{
				fail=true;
			}
		}
	}
	EditorEndReplace(theBuffer);
	theBuffer->haveStartX=haveX;			// update these
	theBuffer->desiredStartX=desiredX;

	if(fail)
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("Failed to delete: %.256s\n",errorDescription);
	}
}

static void EditorInsertAtSelection(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,UINT8 *theText,UINT32 textLength)
// insert text after the cursor or over the selection in the given selection universe
// if there is a problem, it will be reported here
{
	bool
		fail;

	fail=false;
	EditorStartReplace(theBuffer);
	if(!IsSelectionEmpty(theSelectionUniverse))
	{
		BeginUndoGroup(theBuffer);
		DeleteAllSelectedText(theBuffer,theSelectionUniverse);						// get rid of selections
		fail=!ReplaceEditorText(theBuffer,GetSelectionCursorPosition(theSelectionUniverse),GetSelectionCursorPosition(theSelectionUniverse),theText,textLength);
		EndUndoGroup(theBuffer);
	}
	else	// if not deleting a selection, then let undo group continue from where it left off
	{
		fail=!ReplaceEditorText(theBuffer,GetSelectionCursorPosition(theSelectionUniverse),GetSelectionCursorPosition(theSelectionUniverse),theText,textLength);
	}
	EditorEndReplace(theBuffer);

	if(fail)
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("Failed to insert: %.256s\n",errorDescription);
	}
}

void EditorInsert(EDITORBUFFER *theBuffer,UINT8 *theText,UINT32 textLength)
// insert the text after the cursor, or over the selection if there is one
// if there is a problem, it will be reported here
{
	EditorInsertAtSelection(theBuffer,theBuffer->selectionUniverse,theText,textLength);
}

void EditorAuxInsert(EDITORBUFFER *theBuffer,UINT8 *theText,UINT32 textLength)
// insert the text after the aux cursor, or over the aux selection if there is one
// if there is a problem, it will be reported here
{
	EditorInsertAtSelection(theBuffer,theBuffer->auxSelectionUniverse,theText,textLength);
}

bool EditorInsertFile(EDITORBUFFER *theBuffer,char *thePath)
// insert the data in the file at thePath after the cursor or over the selection
// in the selection universe
// if there is a SetError, and return false, it will be reported here
{
	SELECTIONUNIVERSE
		*theSelectionUniverse;
	bool
		fail;
	UINT32
		replacePosition;

	fail=false;
	theSelectionUniverse=theBuffer->selectionUniverse;
	EditorStartReplace(theBuffer);
	BeginUndoGroup(theBuffer);
	if(!IsSelectionEmpty(theSelectionUniverse))
	{
		DeleteAllSelectedText(theBuffer,theSelectionUniverse);	// NOTE: we do not end the group here, so that the replace is in the same group, otherwise, it may or may not be depending on if the selection had more than one element
	}
	replacePosition=GetSelectionCursorPosition(theSelectionUniverse);
	fail=!ReplaceEditorFile(theBuffer,replacePosition,replacePosition,thePath);
	StrictEndUndoGroup(theBuffer);
	EditorEndReplace(theBuffer);
	return(!fail);
}

void EditorAutoIndent(EDITORBUFFER *theBuffer)
// drop a return into the text, and auto indent the next line
// if there is a problem, it will be reported here
{
	SELECTIONUNIVERSE
		*theSelectionUniverse;
	UINT32
		startReplace;
	CHUNKHEADER
		*startChunk,
		*theChunk;
	UINT32
		startOffset,
		theOffset;
	UINT32
		theLine,
		lineOffset;
	UINT32
		numWhite;
	bool
		fail,
		done;
	UINT8
		*insertBuffer;
	bool
		hadGroup;

	fail=hadGroup=false;
	theSelectionUniverse=theBuffer->selectionUniverse;

	EditorStartReplace(theBuffer);
	if(!IsSelectionEmpty(theSelectionUniverse))
	{
		BeginUndoGroup(theBuffer);
		DeleteAllSelectedText(theBuffer,theSelectionUniverse);
		hadGroup=true;
	}
	startReplace=GetSelectionCursorPosition(theSelectionUniverse);
	PositionToLinePosition(theBuffer->textUniverse,startReplace,&theLine,&lineOffset,&theChunk,&theOffset);	// find the line that we are replacing in
	startChunk=theChunk;
	startOffset=theOffset;
	numWhite=0;
	done=false;
	while(numWhite<lineOffset&&!done&&theChunk)							// run through counting the number of white space characters in this line, until the first non-white is seen, or until we reach the place where we will insert the return
	{
		switch(theChunk->data[theOffset])
		{
			case ' ':
			case '\t':
				numWhite++;
				break;
			default:
				done=true;
				break;
		}
		theOffset++;
		if(theOffset>=theChunk->totalBytes)								// push through end of chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	if((insertBuffer=(UINT8 *)MNewPtr(numWhite+1)))						// make room for all the white space, and the CR
	{
		insertBuffer[0]='\n';											// jam in the CR
		if(ExtractUniverseText(theBuffer->textUniverse,startChunk,startOffset,&(insertBuffer[1]),numWhite,&theChunk,&theOffset))	// pull the white space from the line
		{
			if(ReplaceEditorText(theBuffer,startReplace,startReplace,insertBuffer,numWhite+1))
			{
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
		MDisposePtr(insertBuffer);
	}
	else
	{
		fail=true;
	}
	if(hadGroup)
	{
		EndUndoGroup(theBuffer);
	}
	EditorEndReplace(theBuffer);

	if(fail)
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("Failed to auto indent: %.256s\n",errorDescription);
	}
}

// --------------------------------------------------------------------------------------------------------------------------
// clear/load/save functions

bool ClearBuffer(EDITORBUFFER *theBuffer)
// clear the buffer, wiping out any text that was previously there
// also kill any selection, and reset the cursor
// and set the "clean" point
// if there is a problem, set the error, and return false
{
	bool
		fail;

	fail=false;
	EditorStartReplace(theBuffer);									// we are going to change text in a buffer
	fail=!ReplaceEditorText(theBuffer,0,theBuffer->textUniverse->totalBytes,NULL,0);	// remove all that was there
	EditorClearUndo(theBuffer);
	SetUndoCleanPoint(theBuffer);
	EditorEndReplace(theBuffer);										// text change is complete
	return(!fail);
}

bool LoadBuffer(EDITORBUFFER *theBuffer,char *thePath)
// load the buffer from thePath, wiping out
// any text that was previously there
// also kill any selection, and reset the cursor
// and set the buffer clean
// if there is a problem, set the error, and return false
{
	bool
		fail;

	fail=false;
	EditorStartReplace(theBuffer);									// we are going to change text in a buffer
	fail=!ReplaceEditorFile(theBuffer,0,theBuffer->textUniverse->totalBytes,thePath);	// replace all that was there
	EditorSetNormalSelection(theBuffer,theBuffer->selectionUniverse,0,0);				// move the cursor to the top after the change
	EditorClearUndo(theBuffer);
	SetUndoCleanPoint(theBuffer);
	EditorEndReplace(theBuffer);									// text change is complete
	return(!fail);
}

bool SaveBuffer(EDITORBUFFER *theBuffer,char *thePath,bool setClean)
// save theBuffer to thePath
// if setClean is true, then set the buffer as clean
// if there is a problem, set the error, and return false
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT8
		*dataBuffer;
	EDITORFILE
		*theFile;
	UINT32
		numWritten;
	UINT32
		bytesLeft,
		bytesToWrite;
	bool
		fail;

	fail=false;
	EditorStartTextChange(theBuffer);									// start text change, so that status bars can update if needed
	if((theFile=OpenEditorWriteFile(thePath)))							// open the file to write to
	{
		if((dataBuffer=(UINT8 *)MNewPtr(LOADSAVEBUFFERSIZE)))			// create an intermediate buffer to speed the save
		{
			bytesLeft=theBuffer->textUniverse->totalBytes;
			theChunk=theBuffer->textUniverse->firstChunkHeader;
			theOffset=0;
			while(bytesLeft&&!fail)										// read until we reach the end of the file
			{
				if(bytesLeft>LOADSAVEBUFFERSIZE)						// see how many bytes to extract this time
				{
					bytesToWrite=LOADSAVEBUFFERSIZE;
				}
				else
				{
					bytesToWrite=bytesLeft;
				}
				if(ExtractUniverseText(theBuffer->textUniverse,theChunk,theOffset,dataBuffer,bytesToWrite,&theChunk,&theOffset))	// read data from universe into buffer
				{
					if(WriteEditorFile(theFile,dataBuffer,bytesToWrite,&numWritten))	// write the buffer contents to the file
					{
						bytesLeft-=bytesToWrite;						// decrement bytes remaining to be written
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					fail=true;
				}
			}
			MDisposePtr(dataBuffer);
		}
		else
		{
			fail=true;
		}
		CloseEditorFile(theFile);
	}
	else
	{
		fail=true;
	}
	if(!fail&&setClean)
	{
		SetUndoCleanPoint(theBuffer);			// make it clean again if asked to
	}
	EditorEndTextChange(theBuffer);				// text change is complete
	return(!fail);
}

void LinkViewToBuffer(EDITORVIEW *theView,EDITORBUFFER *theBuffer)
// connect theView to theBuffer (at the start)
{
	theView->parentBuffer=theBuffer;
	theView->nextBufferView=theBuffer->firstView;
	theBuffer->firstView=theView;
}

void UnlinkViewFromBuffer(EDITORVIEW *theView)
// disconnect theView from its buffer
{
	EDITORVIEW
		*previousView,
		*currentView;

	previousView=NULL;
	currentView=theView->parentBuffer->firstView;
	while(currentView&&currentView!=theView)
	{
		previousView=currentView;
		currentView=currentView->nextBufferView;
	}
	if(currentView)
	{
		if(previousView)
		{
			previousView->nextBufferView=theView->nextBufferView;
		}
		else
		{
			theView->parentBuffer->firstView=theView->nextBufferView;
		}
	}
}

bool GotoEditorMark(EDITORBUFFER *theBuffer,MARKLIST *theMark)
// set the editor's selection to be the same as theMark
// if there is a problem here, SetError, return false, and leave the selection
// unchanged
{
	SELECTIONUNIVERSE
		*newUniverse;

	if((newUniverse=CreateSelectionUniverseCopy(theMark->selectionUniverse)))
	{
		EditorStartSelectionChange(theBuffer);
		CloseSelectionUniverse(theBuffer->selectionUniverse);
		theBuffer->selectionUniverse=newUniverse;
		EditorEndSelectionChange(theBuffer);
	}
	return(false);
}

// --------------------------------------------------------------------------------------------------------------------------
// creation/deletion functions

MARKLIST *LocateEditorMark(EDITORBUFFER *theBuffer,char *markName)
// attempt to locate a mark with the given name on theBuffer
// if one is found, return it, else, return NULL
{
	bool
		found;
	MARKLIST
		*currentMark;

	currentMark=theBuffer->theMarks;
	found=false;
	while(currentMark&&!found)
	{
		if(currentMark->markName&&(strcmp(currentMark->markName,markName)==0))
		{
			found=true;
		}
		else
		{
			currentMark=currentMark->nextMark;			// locate the mark to delete
		}
	}
	return(currentMark);
}

static void UnlinkEditorMark(EDITORBUFFER *theBuffer,MARKLIST *theMark)
// unlink a mark from the buffer
{
	MARKLIST
		*previousMark,
		*currentMark;

	previousMark=NULL;
	currentMark=theBuffer->theMarks;
	while(currentMark&&(currentMark!=theMark))
	{
		previousMark=currentMark;
		currentMark=currentMark->nextMark;			// locate the mark to delete
	}
	if(currentMark)									// make sure we located it
	{
		if(previousMark)							// see if it was at the top
		{
			previousMark->nextMark=currentMark->nextMark;	// link across
		}
		else
		{
			theBuffer->theMarks=currentMark->nextMark;	// set new top of list
		}
	}
}

static void LinkEditorMark(EDITORBUFFER *theBuffer,MARKLIST *theMark)
// link a mark to the buffer
{
	theMark->nextMark=theBuffer->theMarks;				// link onto start of list
	theBuffer->theMarks=theMark;
}

MARKLIST *SetEditorMark(EDITORBUFFER *theBuffer,char *markName)
// add a mark to the buffer with markName
// if a mark already exists with the given name, reset it to the current
// selection
// if there is a problem, SetError, and return NULL (if the mark existed, and we have
// a problem, it gets deleted)
{
	MARKLIST
		*theMark;

	if((theMark=LocateEditorMark(theBuffer,markName)))
	{
		CloseSelectionUniverse(theMark->selectionUniverse);	// get rid of the old universe
		if((theMark->selectionUniverse=CreateSelectionUniverseCopy(theBuffer->selectionUniverse)))
		{
			return(theMark);
		}
		else
		{
			UnlinkEditorMark(theBuffer,theMark);
			MDisposePtr(theMark->markName);
			MDisposePtr(theMark);
		}
	}
	else
	{
		if((theMark=(MARKLIST *)MNewPtr(sizeof(MARKLIST))))
		{
			if((theMark->selectionUniverse=CreateSelectionUniverseCopy(theBuffer->selectionUniverse)))	// copy the current selection as the new mark
			{
				if((theMark->markName=(char *)MNewPtr(strlen(markName)+1)))
				{
					strcpy(theMark->markName,markName);			// copy the name
					LinkEditorMark(theBuffer,theMark);
					return(theMark);
				}
				CloseSelectionUniverse(theMark->selectionUniverse);
			}
			MDisposePtr(theMark);
		}
	}
	return(NULL);
}

void ClearEditorMark(EDITORBUFFER *theBuffer,MARKLIST *theMark)
// remove a mark to the buffer
{
	UnlinkEditorMark(theBuffer,theMark);
	MDisposePtr(theMark->markName);
	CloseSelectionUniverse(theMark->selectionUniverse);		// close the selections
	MDisposePtr(theMark);									// get rid of the mark
}
