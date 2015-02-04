// Style list management functions
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

// This code handles keeping track of style in the text
// it uses the sparse-array concept to hold the style information
// NOTE: styles are coalesced in the array to make them work
// in a more intuitive manner

#include	"includes.h"

typedef struct styleElement
{
	SPARSEARRAYHEADER
		theSAHeader;
	UINT32
		theStyle;				// style index
} STYLEELEMENT;

typedef struct styleUniverse
{
	SPARSEARRAY
		styleArray;
} STYLEUNIVERSE;

static void CoalescePosition(STYLEUNIVERSE *theUniverse,UINT32 thePosition)
// look at the style at thePosition, and thePosition-1
// if they are the same, coalesce the array
{
	STYLEELEMENT
		*leftElement,
		*rightElement;
	STYLEELEMENT
		newElement;
	UINT32
		leftStartPosition,
		leftElements,
		rightStartPosition,
		rightElements;

	if(thePosition>0)
	{
		if(GetSARecordForPosition(&theUniverse->styleArray,thePosition-1,&leftStartPosition,&leftElements,(void **)&leftElement))
		{
			if(leftElement&&(leftStartPosition+leftElements==thePosition))	// make sure there's something to the left, and it ends just before position
			{
				if(GetSARecordForPosition(&theUniverse->styleArray,thePosition,&rightStartPosition,&rightElements,(void **)&rightElement))
				{
					if(rightElement&&(leftElement->theStyle==rightElement->theStyle))	// make sure there's something to the right, and it matches
					{
						newElement.theStyle=leftElement->theStyle;
						SetSARange(&theUniverse->styleArray,leftStartPosition,leftElements+rightElements,&newElement);
						// @@@ this might fail, but the routine should never fail
						// Look into changing this to call "MergeSARange"
					}
				}
			}
		}
	}
}

void DeleteStyleRange(STYLEUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements)
// Remove style array elements from startPosition for numElements
// NOTE: this does not mind being called for positions outside of the current array
{
	DeleteSARange(&theUniverse->styleArray,startPosition,numElements);
	CoalescePosition(theUniverse,startPosition);	// if two of the same style ended up next to each other, merge them
}

bool InsertStyleRange(STYLEUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements)
// insert numElements into the style array at startPosition
{
	return(InsertSARange(&theUniverse->styleArray,startPosition,numElements));
}

bool GetStyleRange(STYLEUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements,UINT32 *theStyle)
// return the range of positions in the array which include position
// and have the same style as position
// NOTE: this uses a cache so it is efficient to call
// repeatedly for positions which are nearby each other
// NOTE: if position is off the end of the array, theStyle will be returned as 0, and
// this will return false
{
	STYLEELEMENT
		*theElement;

	*theStyle=0;
	if(GetSARecordForPosition(&theUniverse->styleArray,position,startPosition,numElements,(void **)&theElement))
	{
		if(theElement)
		{
			*theStyle=theElement->theStyle;
		}
		return(true);
	}
	return(false);
}

UINT32 GetStyleAtPosition(STYLEUNIVERSE *theUniverse,UINT32 position)
// call this to return the style at the given position of
// the given style universe.
// NOTE: if position is off the end of the array, this will return style 0.
// NOTE: this uses a cache so it is efficient to call
// repeatedly for positions which are nearby each other
{
	UINT32
		startPosition,
		numElements;
	UINT32
		theStyle;

	if(!GetStyleRange(theUniverse,position,&startPosition,&numElements,&theStyle))
	{
		theStyle=0;			// default to style 0
	}
	return(theStyle);
}

bool SetStyleRange(STYLEUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements,UINT32 theStyle)
// set theStyle at startPosition for numElements of theUniverse
// if there is a problem, set the error and return false
// the style will overwrite anything currently in the array at
// the given positions
// NOTE: if the range goes beyond the current end of the array,
// the array will be extended by setting style 0 from the end of
// the array until startPosition, and then placing numElements of
// the given style into the array at startPosition
{
	STYLEELEMENT
		theElement;

	if(theStyle)
	{
		theElement.theStyle=theStyle;
		if(SetSARange(&theUniverse->styleArray,startPosition,numElements,&theElement))
		{
			CoalescePosition(theUniverse,startPosition);
			CoalescePosition(theUniverse,startPosition+numElements);
			return(true);
		}
		return(false);
	}
	else
	{
		return(ClearSARange(&theUniverse->styleArray,startPosition,numElements));
	}
}

bool SetStyleAtPosition(STYLEUNIVERSE *theUniverse,UINT32 position,UINT32 theStyle)
// set theStyle at position of theUniverse
// if there is a problem, set the error and return false
{
	return(SetStyleRange(theUniverse,position,1,theStyle));
}

void AdjustStylesForChange(STYLEUNIVERSE *theUniverse,UINT32 startOffset,UINT32 oldEndOffset,UINT32 newEndOffset)
// given a change at startOffset, run through the given style list, and fix it up
{
	DeleteStyleRange(theUniverse,startOffset,oldEndOffset-startOffset);
	InsertStyleRange(theUniverse,startOffset,newEndOffset-startOffset);
}

bool CopyStyle(STYLEUNIVERSE *sourceUniverse,UINT32 sourceOffset,STYLEUNIVERSE *destUniverse,UINT32 destOffset,UINT32 numElements)
// copy numElements of style information from sourceUniverse starting at sourceOffset
// to destUniverse at destOffset
// If there is a problem, set the error, and return false
{
	bool
		fail;
	UINT32
		startPosition;
	UINT32
		elementCount;
	UINT32
		theStyle;
	UINT32
		elementsToSet;

	fail=false;
	while(!fail&&numElements)
	{
		if(GetStyleRange(sourceUniverse,sourceOffset,&startPosition,&elementCount,&theStyle))
		{
			elementsToSet=elementCount-(sourceOffset-startPosition);
			if(elementsToSet>numElements)
			{
				elementsToSet=numElements;
			}
			if(SetStyleRange(destUniverse,destOffset,elementsToSet,theStyle))
			{
				sourceOffset+=elementsToSet;
				destOffset+=elementsToSet;
				numElements-=elementsToSet;
			}
			else
			{
				fail=true;
			}
		}
		else	// ran off the end of the source universe, so treat the rest as style 0
		{
			fail=!SetStyleRange(destUniverse,destOffset,numElements,0);
			numElements=0;	// terminate the loop
		}
	}
	return(!fail);
}

void CloseStyleUniverse(STYLEUNIVERSE *theUniverse)
// dispose of a style universe
{
	UnInitSA(&(theUniverse->styleArray));
	MDisposePtr(theUniverse);
}

STYLEUNIVERSE *OpenStyleUniverse()
// open a style universe (with no elements)
// if there is a problem, set the error, and return NULL
{
	STYLEUNIVERSE
		*theUniverse;

	if((theUniverse=(STYLEUNIVERSE *)MNewPtr(sizeof(STYLEUNIVERSE))))
	{
		if(InitSA(&(theUniverse->styleArray),sizeof(STYLEELEMENT)))
		{
			return(theUniverse);
		}
		MDisposePtr(theUniverse);
	}
	return(NULL);
}
