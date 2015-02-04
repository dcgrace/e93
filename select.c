// Selection list management functions
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

typedef struct selectionElement
{
	SPARSEARRAYHEADER
		theSAHeader;		// only need the header
} SELECTIONELEMENT;

typedef struct selectionUniverse
{
	UINT32
		cursorPosition;				// if there is no selection, this is the cursor's position
	SPARSEARRAY
		selectionArray;
} SELECTIONUNIVERSE;

void DeleteAllSelections(SELECTIONUNIVERSE *theUniverse)
// Remove all selections from theUniverse
{
	DeleteSARange(&theUniverse->selectionArray,0,theUniverse->selectionArray.totalElements);
}

void DeleteSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements)
// Remove selection array elements from startPosition for numElements
// NOTE: this does not mind being called for positions outside of the current array
{
	DeleteSARange(&theUniverse->selectionArray,startPosition,numElements);
}

bool InsertSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements)
// insert numElements into the selection array at startPosition
// the elements inserted will be selected if the insertion was inside a current selection
{
	return(InsertSARange(&theUniverse->selectionArray,startPosition,numElements));
}

bool GetSelectionAtOrAfterPosition(SELECTIONUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements)
// Return the index of the selection which occupies position,
// or the first one after the position if the position is not occupied.
// If the passed position is past the end of the array, return false
// NOTE: this uses a cache so it is efficient to call
// repeatedly for positions which are nearby each other
{
	SELECTIONELEMENT
		*theElement;

	if(GetSARecordAtOrAfterPosition(&theUniverse->selectionArray,position,startPosition,numElements,(void **)&theElement))
	{
		return(true);
	}
	return(false);
}

bool GetSelectionAtOrBeforePosition(SELECTIONUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements)
// Return the index of the selection which occupies position,
// or the first one before the position if the position is not occupied.
// If the passed position is before the first element of the array, return false.
// NOTE: this uses a cache so it is efficient to call
// repeatedly for positions which are nearby each other
{
	SELECTIONELEMENT
		*theElement;

	if(GetSARecordAtOrBeforePosition(&theUniverse->selectionArray,position,startPosition,numElements,(void **)&theElement))
	{
		return(true);
	}
	return(false);
}

bool GetSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements,bool *isActive)
// return the range of positions in the array which include position
// and are selected (or not selected) the same as position
// NOTE: this uses a cache so it is efficient to call
// repeatedly for positions which are nearby each other
// NOTE: if position is off the end of the array, isActive will be returned as false,
// startPosition will contain the position where the array ended and
// this will return false (numElements will not be set)
{
	SELECTIONELEMENT
		*theElement;

	*isActive=false;
	if(GetSARecordForPosition(&theUniverse->selectionArray,position,startPosition,numElements,(void **)&theElement))
	{
		if(theElement)
		{
			*isActive=true;
		}
		return(true);
	}
	return(false);
}

bool GetSelectionAtPosition(SELECTIONUNIVERSE *theUniverse,UINT32 position)
// call this to return the selection at the given position of
// the given selection universe.
// NOTE: if position is off the end of the array, this will return false.
// NOTE: this uses a cache so it is efficient to call
// repeatedly for positions which are nearby each other
{
	UINT32
		startPosition,
		numElements;
	bool
		theSelection;

	GetSelectionRange(theUniverse,position,&startPosition,&numElements,&theSelection);
	return(theSelection);
}

bool SetSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements)
// set theSelection at startPosition for numElements of theUniverse
// if there is a problem, return false
// the selection will overwrite anything currently in the array at
// the given positions
{
	SELECTIONELEMENT
		theElement;

	return(SetSARange(&theUniverse->selectionArray,startPosition,numElements,&theElement));
}

bool IsSelectionEmpty(SELECTIONUNIVERSE *theUniverse)
// return TRUE if theUniverse contains no selection elements
{
	return(IsSAEmpty(&theUniverse->selectionArray));
}

void GetSelectionEndPositions(SELECTIONUNIVERSE *theUniverse,UINT32 *startPosition,UINT32 *endPosition)
// return the start and end absolute text positions of the passed selection universe
// if there are no selections, then the cursor position is returned as both the start and the end
{
	if(!GetSAElementEnds(&theUniverse->selectionArray,startPosition,endPosition))
	{
		(*startPosition)=(*endPosition)=theUniverse->cursorPosition;
	}
}

UINT32 GetSelectionCursorPosition(SELECTIONUNIVERSE *theUniverse)
{
	return(theUniverse->cursorPosition);
}

void SetSelectionCursorPosition(SELECTIONUNIVERSE *theUniverse,UINT32 thePosition)
{
	theUniverse->cursorPosition=thePosition;
}

void AdjustSelectionsForChange(SELECTIONUNIVERSE *theUniverse,UINT32 startOffset,UINT32 oldEndOffset,UINT32 newEndOffset)
// given a change at startOffset, run through the given selection list, and fix it up
// NOTE: this can cause all the selections to disappear
// If it does, then the cursor will be left at the end of the change
{
	UINT32
		startPosition,
		endPosition;

	if(GetSAElementEnds(&theUniverse->selectionArray,&startPosition,&endPosition))	// if there is some selection, then move the cursor to the start so if the selection goes away, the cursor is left in the correct spot
	{
		theUniverse->cursorPosition=startPosition;			// push to the start
	}

	DeleteSelectionRange(theUniverse,startOffset,oldEndOffset-startOffset);
	InsertSelectionRange(theUniverse,startOffset,newEndOffset-startOffset);

	if(theUniverse->cursorPosition>=startOffset)			// if cursor was before the change, then do not move it
	{
		if(theUniverse->cursorPosition>oldEndOffset)		// see if cursor was past changed area (move it relatively)
		{
			theUniverse->cursorPosition=(theUniverse->cursorPosition+newEndOffset)-oldEndOffset;
		}
		else
		{
			theUniverse->cursorPosition=newEndOffset;		// if cursor in middle of change, put it at the end
		}
	}
}

bool CopySelectionUniverse(SELECTIONUNIVERSE *theSourceUniverse,SELECTIONUNIVERSE *theDestUniverse)
// copy theSourceUniverse to theDestUniverse
// if there is a problem, SetError, and return false
{
	bool
		fail;
	UINT32
		startPosition,
		numElements;

	fail=false;
	DeleteAllSelections(theDestUniverse);
	theDestUniverse->cursorPosition=theSourceUniverse->cursorPosition;	// copy the cursor position

	startPosition=0;
	while(!fail&&GetSelectionAtOrAfterPosition(theSourceUniverse,startPosition,&startPosition,&numElements))
	{
		fail=!SetSelectionRange(theDestUniverse,startPosition,numElements);
		startPosition+=numElements;
	}
	return(!fail);
}

void CloseSelectionUniverse(SELECTIONUNIVERSE *theUniverse)
// dispose of a selection universe
{
	UnInitSA(&(theUniverse->selectionArray));
	MDisposePtr(theUniverse);
}

SELECTIONUNIVERSE *OpenSelectionUniverse()
// open a selection universe (with no elements)
// if there is a problem, set the error, and return NULL
{
	SELECTIONUNIVERSE
		*theUniverse;

	if((theUniverse=(SELECTIONUNIVERSE *)MNewPtr(sizeof(SELECTIONUNIVERSE))))
	{
		theUniverse->cursorPosition=0;						// no cursor position yet, so just set it to 0
		if(InitSA(&(theUniverse->selectionArray),sizeof(SELECTIONELEMENT)))
		{
			return(theUniverse);
		}
		MDisposePtr(theUniverse);
	}
	return(NULL);
}

SELECTIONUNIVERSE *CreateSelectionUniverseCopy(SELECTIONUNIVERSE *theOldUniverse)
// make a new selection universe that is a copy of the passed one
// if there is a problem, SetError, and return NULL
{
	SELECTIONUNIVERSE
		*theUniverse;

	if((theUniverse=OpenSelectionUniverse()))				// create a new one
	{
		if(CopySelectionUniverse(theOldUniverse,theUniverse))	// copy the old one into it
		{
			return(theUniverse);							// return the new one
		}
		CloseSelectionUniverse(theUniverse);				// get rid of the new one
	}
	return(NULL);
}
