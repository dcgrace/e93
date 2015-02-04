// Chunk array management functions
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

// NOTE: To manage the array data efficiently, and to keep memory
// from fragmenting, variable arrays of structures are kept in "Chunks"
// Each chunk allocates space from the free memory pool
// and the elements of the array are split among them. The chunks are linked
// to each other in order. When an element is inserted, additional chunks are allocated
// as necessary, and linked into the list at the appropriate location.
// Chunks keep track of how many elements are currently used within them.
// Chunks are not allowed to contain 0 elements

static void DisposeArrayChunk(ARRAYCHUNKHEADER *theChunk)
// deallocate the memory used by theChunk
{
	MDisposePtr(theChunk->data);
	MDisposePtr(theChunk);
}

static ARRAYCHUNKHEADER *CreateArrayChunk(UINT32 maxElements,UINT32 elementSize)
// create a chunk header, and its corresponding data
// if there is a problem, set the error, and return NULL
{
	ARRAYCHUNKHEADER
		*theChunk;

	if((theChunk=(ARRAYCHUNKHEADER *)MNewPtrClr(sizeof(ARRAYCHUNKHEADER))))
	{
		if((theChunk->data=(UINT8 *)MNewPtr(maxElements*elementSize)))
		{
			return(theChunk);
		}
		MDisposePtr(theChunk);
	}
	return((ARRAYCHUNKHEADER *)NULL);
}

static void DisposeArrayChunkList(ARRAYCHUNKHEADER *startChunk)
// dispose of a linked list of chunks
{
	ARRAYCHUNKHEADER
		*nextChunk;

	while(startChunk)
	{
		nextChunk=startChunk->nextHeader;
		DisposeArrayChunk(startChunk);
		startChunk=nextChunk;
	}
}

static bool CreateArrayChunkList(ARRAYCHUNKHEADER **startChunk,ARRAYCHUNKHEADER **endChunk,UINT32 numChunks,UINT32 maxElements,UINT32 elementSize)
// create a linked list of empty chunks
// if there is a problem, set the error, free anything allocated, and return false
{
	bool
		fail;
	ARRAYCHUNKHEADER
		*theChunk;

	(*startChunk)=(*endChunk)=NULL;
	fail=false;
	while(numChunks&&!fail)
	{
		if((theChunk=CreateArrayChunk(maxElements,elementSize)))
		{
			if(*endChunk)
			{
				(*endChunk)->nextHeader=theChunk;
				theChunk->previousHeader=(*endChunk);
				(*endChunk)=theChunk;
			}
			else
			{
				(*startChunk)=(*endChunk)=theChunk;
			}
			numChunks--;
		}
		else
		{
			DisposeArrayChunkList(*startChunk);					// remove the partially created chunk list
			fail=true;
		}
	}
	return(!fail);
}

static void UnlinkArrayChunkList(ARRAYUNIVERSE *theUniverse,ARRAYCHUNKHEADER *startChunk,ARRAYCHUNKHEADER *endChunk)
// unlink the chunks from startChunk to endChunk from theUniverse
{
	if(startChunk->previousHeader)
	{
		startChunk->previousHeader->nextHeader=endChunk->nextHeader;
	}
	else
	{
		theUniverse->firstChunkHeader=endChunk->nextHeader;
	}

	if(endChunk->nextHeader)
	{
		endChunk->nextHeader->previousHeader=startChunk->previousHeader;
	}
	else
	{
		theUniverse->lastChunkHeader=startChunk->previousHeader;
	}
	startChunk->previousHeader=endChunk->nextHeader=NULL;			// terminate the list
}

static void DeleteWithinArrayChunk(ARRAYUNIVERSE *theUniverse,ARRAYCHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numElements,ARRAYCHUNKHEADER **nextChunk,UINT32 *nextOffset)
// data within one chunk is being deleted
{
	if(numElements<theChunk->totalElements)							// see if something will remain after the deletion
	{
		MMoveMem(&(theChunk->data[(theOffset+numElements)*theUniverse->elementSize]),&(theChunk->data[theOffset*theUniverse->elementSize]),(theChunk->totalElements-(theOffset+numElements))*theUniverse->elementSize);	// move back any bytes needed
		theChunk->totalElements-=numElements;
		theUniverse->totalElements-=numElements;
		if(theOffset>=theChunk->totalElements)
		{
			(*nextChunk)=theChunk->nextHeader;
			(*nextOffset)=0;
		}
		else
		{
			(*nextChunk)=theChunk;
			(*nextOffset)=theOffset;
		}
	}
	else
	{
		(*nextChunk)=theChunk->nextHeader;
		(*nextOffset)=0;
		UnlinkArrayChunkList(theUniverse,theChunk,theChunk);
		theUniverse->totalElements-=theChunk->totalElements;
		DisposeArrayChunkList(theChunk);
	}
}

void DeleteUniverseArray(ARRAYUNIVERSE *theUniverse,ARRAYCHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numElements,ARRAYCHUNKHEADER **nextChunk,UINT32 *nextOffset)
// delete numElements from theUniverse starting at theChunk, theOffset
// return nextChunk/nextOffset as the chunk and offset just past the end of the
// delete, or NULL if none exists past the end
{
	UINT32
		numToDelete;

	while(numElements&&theChunk)
	{
		if((theChunk->totalElements-theOffset)<numElements)
		{
			numToDelete=theChunk->totalElements-theOffset;
		}
		else
		{
			numToDelete=numElements;
		}
		DeleteWithinArrayChunk(theUniverse,theChunk,theOffset,numToDelete,&theChunk,&theOffset);
		numElements-=numToDelete;
	}
	(*nextChunk)=theChunk;
	(*nextOffset)=theOffset;
}

static bool InsertWithRoomInChunk(ARRAYUNIVERSE *theUniverse,ARRAYCHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numElements,ARRAYCHUNKHEADER **nextChunk,UINT32 *nextOffset)
// the chunk that the elements are going to be inserted into
// contains enough free space to accept the entire inserted list
{
	MMoveMem(&(theChunk->data[theOffset*theUniverse->elementSize]),&(theChunk->data[(theOffset+numElements)*theUniverse->elementSize]),(theChunk->totalElements-theOffset)*theUniverse->elementSize);	// move data within the chunk to make room
	theChunk->totalElements+=numElements;					// bump number of elements in this chunk
	theUniverse->totalElements+=numElements;				// adjust universal understanding
	(*nextChunk)=theChunk;
	(*nextOffset)=theOffset;
	return(true);												// this cannot fail
}

static bool InsertWithRoomInTwoChunks(ARRAYUNIVERSE *theUniverse,ARRAYCHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numElements,ARRAYCHUNKHEADER **nextChunk,UINT32 *nextOffset)
// the chunk where the element insertion starts, and the next chunk contain enough free space
// to allow the elements to be inserted
{
	ARRAYCHUNKHEADER
		*secondChunk;
	UINT32
		freeInSecond,
		amountToMove,
		distanceToMove;

	secondChunk=theChunk->nextHeader;
	if(numElements+theOffset<=theUniverse->maxElements)				// see if the added elements can be contained completely within the first chunk
	{
		freeInSecond=theUniverse->maxElements-secondChunk->totalElements;		// get free space in second chunk
		distanceToMove=theChunk->totalElements-theOffset;			// would like to move as much as possible to second chunk
		if(distanceToMove>freeInSecond)
		{
			distanceToMove=freeInSecond;
		}

		MMoveMem(secondChunk->data,&(secondChunk->data[(distanceToMove)*theUniverse->elementSize]),secondChunk->totalElements*theUniverse->elementSize);	// move second chunk's data out of the way for the add
		MMoveMem(&(theChunk->data[(theChunk->totalElements-distanceToMove)*theUniverse->elementSize]),secondChunk->data,distanceToMove*theUniverse->elementSize);	// move data from first chunk to second chunk

		theChunk->totalElements-=distanceToMove;
		secondChunk->totalElements+=distanceToMove;					// this many more bytes now in use in second chunk

		MMoveMem(&(theChunk->data[theOffset*theUniverse->elementSize]),&(theChunk->data[(theOffset+numElements)*theUniverse->elementSize]),(theChunk->totalElements-theOffset)*theUniverse->elementSize);	// move data within the chunk to make room

		theChunk->totalElements+=numElements;						// bump number of bytes in this chunk
		theUniverse->totalElements+=numElements;					// adjust universal understanding
	}
	else
	{
		distanceToMove=theChunk->totalElements+numElements-theUniverse->maxElements;		// need to push second chunk data forward this distance
		MMoveMem(secondChunk->data,&(secondChunk->data[distanceToMove*theUniverse->elementSize]),secondChunk->totalElements*theUniverse->elementSize);	// move second chunk's data out of the way for the add
		secondChunk->totalElements+=distanceToMove;					// this many more elements now in use in second chunk
		amountToMove=theChunk->totalElements-theOffset;				// number of elements to move from first to second

		MMoveMem(&(theChunk->data[theOffset*theUniverse->elementSize]),&(secondChunk->data[(theOffset+numElements-theUniverse->maxElements)*theUniverse->elementSize]),amountToMove*theUniverse->elementSize);	// move data from first chunk to second chunk

		theChunk->totalElements=theUniverse->maxElements;
		theUniverse->totalElements+=numElements;				// adjust universal understanding
	}
	(*nextChunk)=theChunk;
	(*nextOffset)=theOffset;
	return(true);												// this cannot fail
}

static bool InsertWithNoRoom(ARRAYUNIVERSE *theUniverse,ARRAYCHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numElements,ARRAYCHUNKHEADER **nextChunk,UINT32 *nextOffset)
// the elements to be inserted cannot fit within the chunk passed
// so, chunks will have to be created
// NOTE: this must handle the case where theOffset is theUniverse->maxElements
{
	UINT32
		numChunks,
		maxToFill,
		numToFill;
	ARRAYCHUNKHEADER
		*startChunk,
		*endChunk;

	(*nextChunk)=theChunk;
	(*nextOffset)=theOffset;
	numChunks=(numElements-(theUniverse->maxElements-(theChunk->totalElements))+theUniverse->maxElements-1)/theUniverse->maxElements;
	if(CreateArrayChunkList(&startChunk,&endChunk,numChunks,theUniverse->maxElements,theUniverse->elementSize))	// create just enough chunks to hold the elements
	{
		if((endChunk->nextHeader=theChunk->nextHeader))						// link new chunks onto the list
		{
			theChunk->nextHeader->previousHeader=endChunk;
		}
		else
		{
			theUniverse->lastChunkHeader=endChunk;							// update the universe
		}
		theChunk->nextHeader=startChunk;
		startChunk->previousHeader=theChunk;
		theUniverse->totalElements+=numElements;							// more elements in the universe now

		if((numElements+theOffset)/theUniverse->maxElements<numChunks)
		{
			MMoveMem(&(theChunk->data[theOffset*theUniverse->elementSize]),&(endChunk->data[0]),(theChunk->totalElements-theOffset)*theUniverse->elementSize);	// move data from old last chunk to new last chunk
		}
		else
		{
			MMoveMem(&(theChunk->data[theOffset*theUniverse->elementSize]),&(endChunk->data[((numElements+theOffset)%theUniverse->maxElements)*theUniverse->elementSize]),(theChunk->totalElements-theOffset)*theUniverse->elementSize);	// move data from old last chunk to new last chunk
		}
		endChunk->totalElements=theChunk->totalElements-theOffset;			// this many elements in end chunk at the moment
		theChunk->totalElements=theOffset;									// this many elements in here
		while(numElements&&theChunk)
		{
			maxToFill=theUniverse->maxElements-theOffset;
			if(numElements>maxToFill)
			{
				numToFill=maxToFill;
			}
			else
			{
				numToFill=numElements;
			}
			theChunk->totalElements+=numToFill;								// bump number of elements in this chunk
			numElements-=numToFill;											// this many less elements now
			theChunk=theChunk->nextHeader;									// point to next chunk to fill
			theOffset=0;
		}
		if((*nextOffset)>=(*nextChunk)->totalElements)						// see if pointing past the end, if so, move to the next
		{
			(*nextChunk)=(*nextChunk)->nextHeader;
			(*nextOffset)=0;
		}
		return(true);														// done
	}
	return(false);
}

static bool InsertWithNoChunks(ARRAYUNIVERSE *theUniverse,UINT32 numElements,ARRAYCHUNKHEADER **nextChunk,UINT32 *nextOffset)
// insertion is happening in an empty universe
{
	UINT32
		numToFill;
	ARRAYCHUNKHEADER
		*theChunk,
		*startChunk,
		*endChunk;

	if(CreateArrayChunkList(&startChunk,&endChunk,(numElements+theUniverse->maxElements-1)/theUniverse->maxElements,theUniverse->maxElements,theUniverse->elementSize))	// create just enough chunks to hold the data
	{
		(*nextChunk)=startChunk;
		(*nextOffset)=0;
		theUniverse->firstChunkHeader=startChunk;
		theUniverse->lastChunkHeader=endChunk;
		theUniverse->totalElements=numElements;
		theChunk=startChunk;
		while(theChunk&&numElements)
		{
			if(numElements>theUniverse->maxElements)
			{
				numToFill=theUniverse->maxElements;
			}
			else
			{
				numToFill=numElements;
			}
			theChunk->totalElements=numToFill;							// set number of elements in this chunk
			numElements-=numToFill;										// this many less elements now
			theChunk=theChunk->nextHeader;									// point to next chunk to fill
		}
		return(true);														// done
	}
	return(false);
}

bool InsertUniverseArray(ARRAYUNIVERSE *theUniverse,ARRAYCHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numElements,ARRAYCHUNKHEADER **nextChunk,UINT32 *nextOffset)
// make room in the element list for numElements at/before theChunk/theOffset
// return nextChunk and nextOffset as the start of the place where the insertions were made
{
	bool
		fail;

	fail=false;
	(*nextChunk)=NULL;
	(*nextOffset)=0;
	if(numElements)
	{
		if(theChunk)
		{
			if((theOffset==0)&&(theChunk->previousHeader)&&(theChunk->previousHeader->totalElements<theUniverse->maxElements))					// if inserting at the very start of a chunk, it is more efficient to look one back (if possible)
			{
				theChunk=theChunk->previousHeader;
				theOffset=theChunk->totalElements;
			}
			if(numElements<=theUniverse->maxElements-(theChunk->totalElements))			// see if the elements can be placed in empty space of this chunk
			{
				fail=!InsertWithRoomInChunk(theUniverse,theChunk,theOffset,numElements,nextChunk,nextOffset);
			}
			else
			{
				if(theChunk->nextHeader)									// is there another chunk after this one?
				{
					if(numElements<=(2*theUniverse->maxElements)-(theChunk->totalElements+theChunk->nextHeader->totalElements))	// is there room in the combined free space of this chunk, and the next one?
					{
						fail=!InsertWithRoomInTwoChunks(theUniverse,theChunk,theOffset,numElements,nextChunk,nextOffset);
					}
					else
					{
						fail=!InsertWithNoRoom(theUniverse,theChunk,theOffset,numElements,nextChunk,nextOffset);
					}
				}
				else
				{
					fail=!InsertWithNoRoom(theUniverse,theChunk,theOffset,numElements,nextChunk,nextOffset);
				}
			}
		}
		else
		{
			if((theChunk=theUniverse->lastChunkHeader))
			{
				if(numElements<=theUniverse->maxElements-(theChunk->totalElements))	// see if the bytes can be placed in empty space of this chunk
				{
					fail=!InsertWithRoomInChunk(theUniverse,theChunk,theChunk->totalElements,numElements,nextChunk,nextOffset);
				}
				else
				{
					fail=!InsertWithNoRoom(theUniverse,theChunk,theChunk->totalElements,numElements,nextChunk,nextOffset);
				}
			}
			else
			{
				fail=!InsertWithNoChunks(theUniverse,numElements,nextChunk,nextOffset);
			}
		}
	}
	return(!fail);
}

void UnInitArrayUniverse(ARRAYUNIVERSE *theUniverse)
// dispose of an array universe
{
	DisposeArrayChunkList(theUniverse->firstChunkHeader);
}

bool InitArrayUniverse(ARRAYUNIVERSE *theUniverse,UINT32 maxElements,UINT32 elementSize)
// initialize an array universe
// if there is a problem, set the error, and return false
{
	theUniverse->firstChunkHeader=theUniverse->lastChunkHeader=(ARRAYCHUNKHEADER *)NULL;
	theUniverse->totalElements=0;
	theUniverse->maxElements=maxElements;
	theUniverse->elementSize=elementSize;
	return(true);
}
