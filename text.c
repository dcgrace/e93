// Lowest level Text handling functions
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

// NOTE: text is managed using a "all in memory" model
// so, as far as most functions are concerned, the entire
// text data structure is kept in memory while it is being edited
// The OS swapping functions are charged with mapping the data in and out
//
// To manage the text data efficiently, and to keep memory
// from fragmenting, all text is kept in "Chunks"
// Each chunk allocates CHUNKSIZE bytes of space from the free memory pool
// and the text of the document is split among them. The chunks are linked
// to each other in order. When text is inserted, additional chunks are allocated
// as necessary, and linked into the list at the appropriate location.
// Chunks keep track of how many bytes are currently used within them.
// Lines are allowed to span chunks.
// Chunks are not allowed to contain 0 bytes.


static void DisposeChunk(CHUNKHEADER *theChunk)
// deallocate the memory used by theChunk
{
	MDisposePtr(theChunk->data);
	MDisposePtr(theChunk);
}

static CHUNKHEADER *CreateChunk()
// create a chunk header, and its corresponding data
// if there is a problem, SetError, and return NULL
{
	CHUNKHEADER
		*theChunk;

	if((theChunk=(CHUNKHEADER *)MNewPtrClr(sizeof(CHUNKHEADER))))
	{
		if((theChunk->data=(UINT8 *)MNewPtr(CHUNKSIZE)))
		{
			return(theChunk);
		}
		MDisposePtr(theChunk);
	}
	return((CHUNKHEADER *)NULL);
}

bool UniverseSanityCheck(TEXTUNIVERSE *theUniverse)
// check the sanity of the text universe, report result
// if there is something wrong, crash or report it, and return false
{
	CHUNKHEADER
		*previousChunk,
		*currentChunk;
	UINT32
		totalChunks,
		actualTotalLines,
		chunkTotalBytes,
		chunkTotalLines;
	UINT32
		i;
	bool
		sawCachedChunk,
		fail;

	fail=sawCachedChunk=false;
	chunkTotalBytes=0;
	chunkTotalLines=0;
	totalChunks=0;
	previousChunk=NULL;
	currentChunk=theUniverse->firstChunkHeader;
	while(currentChunk)
	{
		totalChunks++;
		if(currentChunk==theUniverse->cacheChunkHeader)		// see if this is the cached chunk
		{
			sawCachedChunk=true;
			if(chunkTotalLines!=theUniverse->cacheLines)
			{
				ReportMessage("Cache lines %d does not agree with %d\n",theUniverse->cacheLines,chunkTotalLines);
				fail=true;
			}
			if(chunkTotalBytes!=theUniverse->cacheBytes)
			{
				ReportMessage("Cache bytes %d does not agree with actual %d\n",theUniverse->cacheBytes,chunkTotalBytes);
				fail=true;
			}
		}
		if(currentChunk->previousHeader==previousChunk)
		{
			if(currentChunk->totalBytes)
			{
				actualTotalLines=0;
				for(i=0;i<currentChunk->totalBytes;i++)
				{
					if(currentChunk->data[i]=='\n')
					{
						actualTotalLines++;
					}
				}
				if(actualTotalLines==currentChunk->totalLines)
				{
					chunkTotalBytes+=currentChunk->totalBytes;
					chunkTotalLines+=currentChunk->totalLines;
					previousChunk=currentChunk;
					currentChunk=currentChunk->nextHeader;
				}
				else
				{
					ReportMessage("Incorrect number of lines @ %X (he said there were %d, I saw %d)\n",currentChunk,currentChunk->totalLines,actualTotalLines);
					fail=true;
				}
			}
			else
			{
				ReportMessage("Empty chunk @ %X\n",currentChunk);
				fail=true;
			}
		}
		else
		{
			ReportMessage("Incorrect previous header %X @ %X\n",currentChunk->previousHeader,currentChunk);
			fail=true;
		}
	}
	if(!fail)
	{
		if(theUniverse->lastChunkHeader==previousChunk)
		{
			if(chunkTotalBytes==theUniverse->totalBytes)
			{
				if(chunkTotalLines==theUniverse->totalLines)
				{
					if(!theUniverse->cacheChunkHeader||sawCachedChunk)
					{
						if(totalChunks)
						{
							ReportMessage("Total chunks %d, efficiency: %d%%\n",totalChunks,(theUniverse->totalBytes*100)/(totalChunks*CHUNKSIZE));
						}
						else
						{
							ReportMessage("Total chunks %d\n",totalChunks);
						}
					}
					else
					{
						ReportMessage("Invalid cache chunk header: %X\n",theUniverse->cacheChunkHeader);
						fail=true;
					}
				}
				else
				{
					ReportMessage("Total lines mismatch: chunks=%d, universe=%d\n",chunkTotalLines,theUniverse->totalLines);
					fail=true;
				}
			}
			else
			{
				ReportMessage("Total bytes mismatch: chunks=%d, universe=%d\n",chunkTotalBytes,theUniverse->totalBytes);
				fail=true;
			}
		}
		else
		{
			ReportMessage("The universe's last chunk did not match last one found\n");
			fail=true;
		}
	}
	return(!fail);
}

static void GetPosStartChunkAndDirection(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER **theChunk,UINT32 *currentBytes,UINT32 *currentLines,bool *backwards)
// return a chunk, and a direction to start searching,
// given a position that is desired
// NOTE: this uses the current cache information to help speed things up
// NOTE ALSO: thePosition must not be out of range
{
	if(theUniverse->cacheChunkHeader)				// if there is a cached chunk, see which direction we should go from it
	{
		*backwards=(thePosition<theUniverse->cacheBytes);	// see which direction from the cache we need to go
		*currentBytes=theUniverse->cacheBytes;
		*currentLines=theUniverse->cacheLines;
		*theChunk=theUniverse->cacheChunkHeader;
	}
	else
	{
		*backwards=false;							// work from the start
		*currentBytes=0;
		*currentLines=0;
		*theChunk=theUniverse->firstChunkHeader;
	}
}

static void GetLineStartChunkAndDirection(TEXTUNIVERSE *theUniverse,UINT32 theLine,CHUNKHEADER **theChunk,UINT32 *currentBytes,UINT32 *currentLines,bool *backwards)
// return a chunk, and a direction to start searching,
// given a line number that is desired
// NOTE: this uses the current cache information to help speed things up
// NOTE ALSO: theLine must not be out of range
{
	if(theUniverse->cacheChunkHeader)				// if there is a cached chunk, see which direction we should go from it
	{
		*backwards=(theLine<=theUniverse->cacheLines);	// see which direction from the cache we need to go
		*currentBytes=theUniverse->cacheBytes;
		*currentLines=theUniverse->cacheLines;
		*theChunk=theUniverse->cacheChunkHeader;
	}
	else
	{
		*backwards=false;							// work from the start
		*currentBytes=0;
		*currentLines=0;
		*theChunk=theUniverse->firstChunkHeader;
	}
}

void PositionToChunkPositionPastEnd(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER **theChunk,UINT32 *theOffset)
// locate the chunk which contains the byte at thePosition within theUniverse
// if there are no chunks in the universe, return theChunk set to NULL, theOffset set to 0
// if the given position is past the end of the universe, return with theChunk pointed to the
// last chunk of the universe, and theOffset as the last chunk's total-bytes
// NOTE: this will update the cache position to the chunk which is returned
{
	CHUNKHEADER
		*currentChunk;
	UINT32
		currentPosition,
		currentLine;
	bool
		backwards;

	if(thePosition>=theUniverse->totalBytes)						// quick test to get us to the end faster (and avoid some checks in the loop below)
	{
		if((theUniverse->cacheChunkHeader=(*theChunk)=theUniverse->lastChunkHeader))	// point to the last chunk (if there is one)
		{
			theUniverse->cacheBytes=theUniverse->totalBytes-theUniverse->lastChunkHeader->totalBytes;
			theUniverse->cacheLines=theUniverse->totalLines-theUniverse->lastChunkHeader->totalLines;
			(*theOffset)=theUniverse->lastChunkHeader->totalBytes;	// pass back max offset
		}
		else
		{
			(*theOffset)=0;
		}
	}
	else
	{
		GetPosStartChunkAndDirection(theUniverse,thePosition,&currentChunk,&currentPosition,&currentLine,&backwards);
		if(backwards)
		{
			while(currentPosition>thePosition)
			{
				currentChunk=currentChunk->previousHeader;
				currentPosition-=currentChunk->totalBytes;
				currentLine-=currentChunk->totalLines;
			}
		}
		else
		{
			while((currentPosition+=currentChunk->totalBytes)<=thePosition)
			{
				currentLine+=currentChunk->totalLines;
				currentChunk=currentChunk->nextHeader;
			}
			currentPosition-=currentChunk->totalBytes;
		}
		theUniverse->cacheChunkHeader=(*theChunk)=currentChunk;
		theUniverse->cacheBytes=currentPosition;
		theUniverse->cacheLines=currentLine;
		(*theOffset)=thePosition-currentPosition;
	}
}

void PositionToChunkPosition(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER **theChunk,UINT32 *theOffset)
// locate the chunk which contains the byte at thePosition within theUniverse
// if there is no chunk in theUniverse at thePosition, return NULL/0
// NOTE: this will update the cache position to the chunk which is returned
{
	PositionToChunkPositionPastEnd(theUniverse,thePosition,theChunk,theOffset);
	if((*theChunk)&&((*theOffset)>=(*theChunk)->totalBytes))			// if past end, then return NULL/0
	{
		*theChunk=NULL;
		*theOffset=0;
	}
}

void PositionToLinePosition(TEXTUNIVERSE *theUniverse,UINT32 thePosition,UINT32 *theLine,UINT32 *theLineOffset,CHUNKHEADER **theChunk,UINT32 *theChunkOffset)
// locate the line which contains the byte at thePosition within theUniverse
// also return theChunk, and theChunkOffset that point to the START of theLine
// if thePosition is past the end of theUniverse, theLine will be the last line
// and theLineOffset will be one past the last character of theLine
// theChunk/theChunkOffset will point to the start of the last line (if there is one, NULL/0 if not)
// NOTE: this will update the cache position to the chunk which contained thePosition
{
	CHUNKHEADER
		*currentChunk;
	UINT32
		currentOffset,
		currentPosition,
		currentLine,
		lineStartOffset,
		numNewLines;
	bool
		backwards;

	currentOffset=0;
	if(thePosition>=theUniverse->totalBytes)						// quick test to get us to the end faster (and avoid some checks in the loop below)
	{
		if((theUniverse->cacheChunkHeader=currentChunk=theUniverse->lastChunkHeader))	// point to the last chunk (if there is one)
		{
			theUniverse->cacheBytes=theUniverse->totalBytes-theUniverse->lastChunkHeader->totalBytes;
			theUniverse->cacheLines=currentLine=theUniverse->totalLines-theUniverse->lastChunkHeader->totalLines;
			currentOffset=theUniverse->lastChunkHeader->totalBytes;	// max offset to begin searching backwards from
		}
	}
	else
	{
		GetPosStartChunkAndDirection(theUniverse,thePosition,&currentChunk,&currentPosition,&currentLine,&backwards);
		if(backwards)
		{
			while(currentPosition>thePosition)
			{
				currentChunk=currentChunk->previousHeader;
				currentPosition-=currentChunk->totalBytes;
				currentLine-=currentChunk->totalLines;
			}
		}
		else
		{
			while((currentPosition+=currentChunk->totalBytes)<=thePosition)
			{
				currentLine+=currentChunk->totalLines;
				currentChunk=currentChunk->nextHeader;
			}
			currentPosition-=currentChunk->totalBytes;
		}
		theUniverse->cacheChunkHeader=currentChunk;
		theUniverse->cacheBytes=currentPosition;
		theUniverse->cacheLines=currentLine;
		currentOffset=thePosition-currentPosition;					// offset to the given position
	}

	*theLineOffset=0;												// at the moment, no offset
	if(currentChunk)												// if this is valid, then there were some bytes in the universe
	{
		// walk backwards from the current position until the beginning of the current chunk, and then until the first newline encountered (if there are any)
		numNewLines=0;												// tells how many new lines seen in this chunk backwards from current position
		lineStartOffset=0;
		while(currentOffset)
		{
			if(currentChunk->data[--currentOffset]=='\n')
			{
				if(!numNewLines)
				{
					lineStartOffset=currentOffset+1;				// offset into the chunk that points to the start of the line
				}
				numNewLines++;
			}
			if(!numNewLines)										// increment length into the line until a newline is seen
			{
				(*theLineOffset)++;
			}
		}
		*theLine=currentLine+numNewLines;							// this tells which line the position fell into
		if(!numNewLines)
		{
			while(currentChunk->previousHeader&&!(currentChunk->previousHeader->totalLines))	// push back through all chunks which do not contain new lines
			{
				currentChunk=currentChunk->previousHeader;
				(*theLineOffset)+=currentChunk->totalBytes;			// this many more bytes into the line
			}
			if(currentChunk->previousHeader)						// step back into one which contains newlines
			{
				currentChunk=currentChunk->previousHeader;
				currentOffset=lineStartOffset=currentChunk->totalBytes;
			}
			while(currentOffset&&currentChunk->data[--currentOffset]!='\n')
			{
				(*theLineOffset)++;									// keep moving back
				lineStartOffset=currentOffset;						// offset in chunk to start of line
			}
		}
		if(lineStartOffset>=currentChunk->totalBytes)
		{
			*theChunk=currentChunk->nextHeader;						// move one past (possibly off the end)
			*theChunkOffset=0;
		}
		else
		{
			*theChunk=currentChunk;
			*theChunkOffset=lineStartOffset;
		}
	}
	else
	{
		*theChunk=NULL;
		*theChunkOffset=0;
		*theLine=theUniverse->totalLines;							// since it is past the end, the line is this, work backwards to get the offset
	}
}

void LineToChunkPosition(TEXTUNIVERSE *theUniverse,UINT32 theLine,CHUNKHEADER **theChunk,UINT32 *theOffset,UINT32 *thePosition)
// locate the chunk which contains the byte at the start of theLine within theUniverse
// thePosition is the absolute offset within the text to the start of theLine
// if no chunk contains a byte that starts theLine, theChunk will be NULL, theOffset will be 0
// thePosition will be the number of bytes in the text buffer
// NOTE: this will update the cache position to the chunk which is returned
{
	CHUNKHEADER
		*currentChunk;
	UINT32
		currentByte,
		currentLine;
	bool
		backwards;

	if(theLine>theUniverse->totalLines||!(theUniverse->firstChunkHeader))
	{
		if((theUniverse->cacheChunkHeader=theUniverse->lastChunkHeader))	// point to the last chunk (if there is one)
		{
			theUniverse->cacheBytes=theUniverse->totalBytes-theUniverse->lastChunkHeader->totalBytes;
			theUniverse->cacheLines=currentLine=theUniverse->totalLines-theUniverse->lastChunkHeader->totalLines;
		}
		(*theChunk)=NULL;
		(*theOffset)=0;
		(*thePosition)=theUniverse->totalBytes;
	}
	else
	{
		GetLineStartChunkAndDirection(theUniverse,theLine,&currentChunk,&currentByte,&currentLine,&backwards);	// find out where to start searching from
		if(backwards)
		{
			while(currentChunk->previousHeader&&currentLine>=theLine)		// run through the chunks until we get to the one BEFORE or during which this line starts
			{
				currentChunk=currentChunk->previousHeader;
				currentLine-=currentChunk->totalLines;
				currentByte-=currentChunk->totalBytes;
			}
		}
		else
		{
			while((currentLine+=currentChunk->totalLines)<theLine)			// run through the chunks looking for the start of the given line
			{
				currentByte+=currentChunk->totalBytes;
				currentChunk=currentChunk->nextHeader;						// keep going
			}
			currentLine-=currentChunk->totalLines;							// back up, to start of chunk which contains the requested line
		}
		*theOffset=0;
		while(currentLine<theLine)											// move through until enough lines have been counted
		{
			if(currentChunk->data[(*theOffset)++]=='\n')
			{
				currentLine++;
			}
			currentByte++;
		}
		(*thePosition)=currentByte;
		(*theChunk)=currentChunk;
		if((*theOffset)>=currentChunk->totalBytes)							// if needed, move to the next chunk
		{
			(*theChunk)=currentChunk->nextHeader;
			(*theOffset)=0;
		}
	}
}


void AddToChunkPosition(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **newChunk,UINT32 *newOffset,UINT32 distanceToMove)
// move forward from theChunk/theOffset by distanceToMove
// if distanceToMove pushes us off the end, return NULL, 0
// It is ok to pass newChunk as a pointer to theChunk
// NOTE: this does not update the cache position
{
	while(theChunk&&(distanceToMove>=(theChunk->totalBytes-theOffset)))
	{
		distanceToMove-=(theChunk->totalBytes-theOffset);
		theChunk=theChunk->nextHeader;
		theOffset=0;
	}
	if((*newChunk=theChunk))
	{
		*newOffset=theOffset+distanceToMove;
	}
	else
	{
		*newOffset=0;
	}
}

void ChunkPositionToNextLine(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **newChunk,UINT32 *newOffset,UINT32 *distanceMoved)
// locate the line that begins after theChunk, and theOffset
// if theChunk is NULL, return NULL, and distanceMoved=0, otherwise
// attempt to locate the start of the next line, returning theChunk, and theOffset.
// if there is no next line start, return NULL, and distance moved as the amount we traversed before
// discovering there was no next line
// It is ok to pass newChunk as a pointer to theChunk
// NOTE: this does not update the cache position
{
	bool
		found;

	(*distanceMoved)=0;

	if(theChunk)
	{
		found=false;
		if(theChunk->totalLines)									// see if there are any lines in this chunk (if not, dont bother looking for next new line)
		{
			while(!found&&theOffset<theChunk->totalBytes)
			{
				if(theChunk->data[theOffset]=='\n')
				{
					found=true;
				}
				else
				{
					(*distanceMoved)++;
					theOffset++;
				}
			}
			if(!found)
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
		else
		{
			(*distanceMoved)=theChunk->totalBytes-theOffset;		// if no lines, skip the rest of the first chunk
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}

		if(!found)
		{
			while(theChunk&&!theChunk->totalLines)					// run through the chunks looking for one that contains a line
			{
				(*distanceMoved)+=theChunk->totalBytes;
				theChunk=theChunk->nextHeader;						// skip all chunks with no lines in them
			}
			if(theChunk)											// this is the chunk that contains the new line we were looking for, so locate it
			{
				while(theChunk->data[theOffset]!='\n')
				{
					theOffset++;
				}
				(*distanceMoved)+=theOffset;
				found=true;
			}
		}
		if(found)													// at this point, we have found the new line, theChunk, and theOffset point to the new line character, so skip over it, and return what's next
		{
			(*distanceMoved)++;
			theOffset++;
			if(theOffset>=theChunk->totalBytes)						// push through to the next chunk
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
	}
	(*newChunk)=theChunk;
	(*newOffset)=theOffset;
}

static void CopyTextToChunk(CHUNKHEADER *theChunk,UINT32 theOffset,UINT8 *theText,UINT32 numBytes,UINT32 *numLines)
// copy theText into theChunk at theOffset for numBytes
// also count up how many new-lines were seen during the copy
// return numLines as the total number of new-line characters that were copied
{
	UINT8
		*dst;
	UINT32
		i;

	dst=&(theChunk->data[theOffset]);
	(*numLines)=0;
	for(i=0;i<numBytes;i++)
	{
		if((dst[i]=theText[i])=='\n')
		{
			(*numLines)++;
		}
	}
}

static void CopyChunkToChunk(CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **sourceChunk,UINT32 *sourceOffset,UINT32 numBytes,UINT32 *numLines)
// copy the text starting at sourceChunk/sourceOffset into theChunk at theOffset for numBytes
// also count up how many new-lines were seen during the copy
// return numLines as the total number of new-line characters that were copied
// also return with sourceChunk/sourceOffset pointing to the byte just after the last one copied
{
	CHUNKHEADER
		*sChunk;
	UINT8
		*src,
		*dst;
	UINT32
		i,
		sOffset,
		sourceLimit;

	dst=&(theChunk->data[theOffset]);
	(*numLines)=0;
	if((sChunk=(*sourceChunk)))
	{
		src=sChunk->data;
		sOffset=*sourceOffset;
		sourceLimit=sChunk->totalBytes;
		for(i=0;i<numBytes&&sChunk;i++)
		{
			if((dst[i]=src[sOffset])=='\n')
			{
				(*numLines)++;
			}
			if(++sOffset>=sourceLimit)
			{
				sOffset=0;
				if((sChunk=sChunk->nextHeader))
				{
					src=sChunk->data;
					sourceLimit=sChunk->totalBytes;
				}
			}
		}
		(*sourceChunk)=sChunk;
		(*sourceOffset)=sOffset;
	}
}

static void DetermineNewLines(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numBytes,UINT32 *numLines)
// count the number of new-line characters at theOffset of theChunk, for numBytes
// return numLines as the total number of new-line characters that were counted
{
	UINT8
		*dst;
	UINT32
		i;

	dst=&(theChunk->data[theOffset]);
	(*numLines)=0;
	for(i=0;i<numBytes;i++)
	{
		if((dst[i])=='\n')
		{
			(*numLines)++;
		}
	}
}

static void DisposeChunkList(CHUNKHEADER *startChunk)
// dispose of a linked list of chunks
{
	CHUNKHEADER
		*nextChunk;

	while(startChunk)
	{
		nextChunk=startChunk->nextHeader;
		DisposeChunk(startChunk);
		startChunk=nextChunk;
	}
}

static bool CreateChunkList(CHUNKHEADER **startChunk,CHUNKHEADER **endChunk,UINT32 numChunks)
// create a linked list of empty chunks
// if there is a problem, SetError, free anything allocated, and return false
{
	bool
		fail;
	CHUNKHEADER
		*theChunk;

	(*startChunk)=(*endChunk)=NULL;
	fail=false;
	while(numChunks&&!fail)
	{
		if((theChunk=CreateChunk()))
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
			DisposeChunkList(*startChunk);					// remove the partially created chunk list
			fail=true;
		}
	}
	return(!fail);
}

// Text insertion cases
// these routines handle all of the possible cases that can occur while
// inserting text

static bool InsertWithRoomInChunk(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **textChunk,UINT32 *textOffset,UINT32 numBytes)
// the chunk that the text is going to be inserted into
// contains enough free space to accept the entire inserted text
{
	UINT32
		linesInserted;

	MMoveMem(&(theChunk->data[theOffset]),&(theChunk->data[theOffset+numBytes]),theChunk->totalBytes-theOffset);	// move data within the chunk to make room
	CopyChunkToChunk(theChunk,theOffset,textChunk,textOffset,numBytes,&linesInserted);
	theChunk->totalBytes+=numBytes;								// bump number of bytes in this chunk
	theChunk->totalLines+=linesInserted;						// bump number of lines in this chunk
	theUniverse->totalBytes+=numBytes;							// adjust universal understanding
	theUniverse->totalLines+=linesInserted;
	return(true);												// this cannot fail
}

static bool InsertWithRoomInTwoChunks(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **textChunk,UINT32 *textOffset,UINT32 numBytes)
// the chunk where the text insertion starts, and the next chunk contain enough free space
// to allow the text to be inserted
{
	CHUNKHEADER
		*secondChunk;
	UINT32
		linesInserted,
		tempInserted;
	UINT32
		amountToMove,
		distanceToMove;

	secondChunk=theChunk->nextHeader;
	if(numBytes+theOffset<=CHUNKSIZE)								// see if the added text can be contained completely within the first chunk
	{
		distanceToMove=numBytes-(CHUNKSIZE-theChunk->totalBytes);	// for speed, we would like to move as much as possible to the second chunk, but for memory efficiency, we would like to move as little as possible! (memory efficiency turns out to be better)

		MMoveMem(secondChunk->data,&(secondChunk->data[distanceToMove]),secondChunk->totalBytes);	// move second chunk's data out of the way for the add
		CopyTextToChunk(secondChunk,0,&(theChunk->data[theChunk->totalBytes-distanceToMove]),distanceToMove,&tempInserted);	// move data from first chunk to second chunk
		theChunk->totalBytes-=distanceToMove;
		secondChunk->totalBytes+=distanceToMove;					// this many more bytes now in use in second chunk
		theChunk->totalLines-=tempInserted;
		secondChunk->totalLines+=tempInserted;

		MMoveMem(&(theChunk->data[theOffset]),&(theChunk->data[theOffset+numBytes]),theChunk->totalBytes-theOffset);	// move data within the chunk to make room
		CopyChunkToChunk(theChunk,theOffset,textChunk,textOffset,numBytes,&linesInserted);	// copy the data into the chunk
		theChunk->totalBytes+=numBytes;								// bump number of bytes in this chunk
		theChunk->totalLines+=linesInserted;						// bump number of lines in this chunk
		theUniverse->totalBytes+=numBytes;							// adjust universal understanding
		theUniverse->totalLines+=linesInserted;
	}
	else
	{
		distanceToMove=numBytes-(CHUNKSIZE-theChunk->totalBytes);	// need to push second chunk data forward this distance
		MMoveMem(secondChunk->data,&(secondChunk->data[distanceToMove]),secondChunk->totalBytes);	// move second chunk's data out of the way for the add
		secondChunk->totalBytes+=distanceToMove;					// this many more bytes now in use in second chunk
		amountToMove=theChunk->totalBytes-theOffset;				// number of bytes to move from first to second

		CopyTextToChunk(secondChunk,theOffset+numBytes-CHUNKSIZE,&(theChunk->data[theOffset]),amountToMove,&tempInserted);	// move data from first chunk to second chunk
		theChunk->totalLines-=tempInserted;
		secondChunk->totalLines+=tempInserted;

		CopyChunkToChunk(theChunk,theOffset,textChunk,textOffset,CHUNKSIZE-theOffset,&tempInserted);	// copy source text into first chunk
		theChunk->totalBytes=CHUNKSIZE;
		theChunk->totalLines+=tempInserted;

		CopyChunkToChunk(secondChunk,0,textChunk,textOffset,numBytes-(CHUNKSIZE-theOffset),&linesInserted);	// copy remaining text into second chunk
		secondChunk->totalLines+=linesInserted;
		theUniverse->totalLines+=linesInserted+tempInserted;		// add number of lines added in first chunk

		theUniverse->totalBytes+=numBytes;							// adjust universal understanding
	}
	return(true);													// this cannot fail
}

static bool InsertWithNoRoom(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **textChunk,UINT32 *textOffset,UINT32 numBytes)
// the text to be inserted cannot fit within its two adjacent chunks
// so, chunks will have to be created, and the text put into them
{
	UINT32
		linesInserted,
		tempInserted;
	UINT32
		numChunks,
		maxToMove,
		bytesToMove;
	CHUNKHEADER
		*startChunk,
		*endChunk;

	numChunks=(numBytes-(CHUNKSIZE-(theChunk->totalBytes))+CHUNKSIZE-1)/CHUNKSIZE;
	if(CreateChunkList(&startChunk,&endChunk,numChunks))						// create just enough chunks to hold the data
	{
		theChunk->nextHeader->previousHeader=endChunk;							// link new chunks onto the list
		endChunk->nextHeader=theChunk->nextHeader;
		theChunk->nextHeader=startChunk;
		startChunk->previousHeader=theChunk;
		theUniverse->totalBytes+=numBytes;										// more bytes in the universe now

		if(numBytes<(CHUNKSIZE-theOffset))										// see if the bytes we are adding can be added into the first chunk
		{
			bytesToMove=theChunk->totalBytes+numBytes-CHUNKSIZE;
			CopyTextToChunk(endChunk,0,&(theChunk->data[theChunk->totalBytes-bytesToMove]),bytesToMove,&tempInserted);	// move data from old last chunk to new last chunk
			endChunk->totalBytes=bytesToMove;									// this many bytes in end chunk
			endChunk->totalLines=tempInserted;
			theChunk->totalLines-=tempInserted;
			MMoveMem(&(theChunk->data[theOffset]),&(theChunk->data[theOffset+numBytes]),theChunk->totalBytes-bytesToMove-theOffset);	// move data within the original chunk to make room

			CopyChunkToChunk(theChunk,theOffset,textChunk,textOffset,numBytes,&tempInserted);	// copy text into this chunk
			theChunk->totalBytes=CHUNKSIZE;										// this is now completely full
			theChunk->totalLines+=tempInserted;									// added this many lines
			theUniverse->totalLines+=tempInserted;								// add to the universe too
		}
		else
		{
			if((numBytes+theOffset)/CHUNKSIZE<numChunks)						// see if we can move the stuff after the given offset all the way into the end chunk
			{
				CopyTextToChunk(endChunk,0,&(theChunk->data[theOffset]),theChunk->totalBytes-theOffset,&tempInserted);	// move data from old last chunk to new last chunk
			}
			else
			{
				if(theChunk->totalBytes==theOffset)								// minor kludge to make sure we are left pointing to the correct spot
				{
					tempInserted=0;
				}
				else
				{
					CopyTextToChunk(endChunk,(numBytes+theOffset)%CHUNKSIZE,&(theChunk->data[theOffset]),theChunk->totalBytes-theOffset,&tempInserted);	// move data from old last chunk to new last chunk
				}
			}
			endChunk->totalBytes=theChunk->totalBytes-theOffset;					// this many bytes in end chunk at the moment
			theChunk->totalBytes=theOffset;											// this many bytes in here
			theChunk->totalLines-=tempInserted;										// removed lines from here
			endChunk->totalLines=tempInserted;										// placed them here
			linesInserted=0;
			while(numBytes&&theChunk)
			{
				maxToMove=CHUNKSIZE-theOffset;										// can move this many bytes
				if(numBytes>maxToMove)
				{
					bytesToMove=maxToMove;
				}
				else
				{
					bytesToMove=numBytes;
				}
				CopyChunkToChunk(theChunk,theOffset,textChunk,textOffset,bytesToMove,&tempInserted);	// copy text into this chunk
				theChunk->totalBytes+=bytesToMove;								// bump number of bytes in this chunk
				theChunk->totalLines+=tempInserted;								// bump number of lines in this chunk
				linesInserted+=tempInserted;									// keep track of total number of lines inserted so far
				numBytes-=bytesToMove;											// this many less bytes to move now
				theChunk=theChunk->nextHeader;									// point to next chunk to fill
				theOffset=0;													// from this point on, the offset is always 0
			}
			theUniverse->totalLines+=linesInserted;								// increment the number of lines in the universe
		}
		return(true);															// done
	}
	return(false);
}

static bool InsertAtLastChunk(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **textChunk,UINT32 *textOffset,UINT32 numBytes)
// text is being inserted into the last chunk of the universe, and
// the chunk does not contain enough free space to hold it
// NOTE: this code must handle the case where theOffset is CHUNKSIZE
{
	UINT32
		linesInserted,
		tempInserted;
	UINT32
		numChunks,
		maxToMove,
		bytesToMove;
	CHUNKHEADER
		*startChunk,
		*endChunk;

	numChunks=(numBytes-(CHUNKSIZE-(theChunk->totalBytes))+CHUNKSIZE-1)/CHUNKSIZE;
	if(CreateChunkList(&startChunk,&endChunk,numChunks))						// create just enough chunks to hold the data
	{
		theChunk->nextHeader=startChunk;										// link new chunks onto the list
		startChunk->previousHeader=theChunk;
		theUniverse->lastChunkHeader=endChunk;									// update the universe
		theUniverse->totalBytes+=numBytes;										// more bytes in the universe now
		if((numBytes+theOffset)/CHUNKSIZE<numChunks)
		{
			CopyTextToChunk(endChunk,0,&(theChunk->data[theOffset]),theChunk->totalBytes-theOffset,&tempInserted);	// move data from old last chunk to new last chunk
		}
		else
		{
			if(theChunk->totalBytes==theOffset)									// minor kludge to make sure we are left pointing to the correct spot
			{
				tempInserted=0;
			}
			else
			{
				CopyTextToChunk(endChunk,(numBytes+theOffset)%CHUNKSIZE,&(theChunk->data[theOffset]),theChunk->totalBytes-theOffset,&tempInserted);	// move data from old last chunk to new last chunk
			}
		}
		endChunk->totalBytes=theChunk->totalBytes-theOffset;					// this many bytes in end chunk at the moment
		theChunk->totalBytes=theOffset;											// this many bytes in here
		theChunk->totalLines-=tempInserted;										// removed lines from here
		endChunk->totalLines=tempInserted;										// placed them here
		linesInserted=0;
		while(numBytes&&theChunk)
		{
			maxToMove=CHUNKSIZE-theOffset;										// can move this many bytes
			if(numBytes>maxToMove)
			{
				bytesToMove=maxToMove;
			}
			else
			{
				bytesToMove=numBytes;
			}
			CopyChunkToChunk(theChunk,theOffset,textChunk,textOffset,bytesToMove,&tempInserted);	// copy text into this chunk
			theChunk->totalBytes+=bytesToMove;								// bump number of bytes in this chunk
			theChunk->totalLines+=tempInserted;								// bump number of lines in this chunk
			linesInserted+=tempInserted;									// keep track of total number of lines inserted so far
			numBytes-=bytesToMove;											// this many less bytes to move now
			theChunk=theChunk->nextHeader;									// point to next chunk to fill
			theOffset=0;													// from this point on, the offset is always 0
		}
		theUniverse->totalLines+=linesInserted;								// increment the number of lines in the universe
		return(true);														// done
	}
	return(false);
}

static bool InsertWithNoChunks(TEXTUNIVERSE *theUniverse,CHUNKHEADER **textChunk,UINT32 *textOffset,UINT32 numBytes)
// text insertion is happening in an empty text universe
{
	UINT32
		linesInserted,
		tempInserted;
	UINT32
		bytesToMove;
	CHUNKHEADER
		*theChunk,
		*startChunk,
		*endChunk;

	if(CreateChunkList(&startChunk,&endChunk,(numBytes+CHUNKSIZE-1)/CHUNKSIZE))	// create just enough chunks to hold the data
	{
		theUniverse->firstChunkHeader=startChunk;
		theUniverse->lastChunkHeader=endChunk;
		theUniverse->totalBytes=numBytes;
		theChunk=startChunk;
		linesInserted=0;
		while(theChunk&&numBytes)
		{
			if(numBytes>CHUNKSIZE)
			{
				bytesToMove=CHUNKSIZE;
			}
			else
			{
				bytesToMove=numBytes;
			}
			CopyChunkToChunk(theChunk,0,textChunk,textOffset,bytesToMove,&tempInserted);	// copy the data into the chunk
			theChunk->totalBytes=bytesToMove;								// set number of bytes in this chunk
			theChunk->totalLines=tempInserted;								// set number of lines in this chunk
			linesInserted+=tempInserted;									// keep track of total number of lines inserted so far
			numBytes-=bytesToMove;											// this many less bytes to move now
			theChunk=theChunk->nextHeader;									// point to next chunk to fill
		}
		theUniverse->totalLines=linesInserted;								// remember the number of lines in the universe
		return(true);														// done
	}
	return(false);
}

bool InsertUniverseChunks(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER *textChunk,UINT32 textOffset,UINT32 numBytes)
// insert numBytes of text from textChunk/textOffset into theUniverse starting at thePosition
// if there is a problem, SetError, and return false
// this routine is allowed to either succeed, or fail completely, under no circumstance
// is it allowed to insert only part of the given text
// NOTE: it is illegal for textChunk/textOffset to point to less data than numBytes
// NOTE ALSO: to accommodate inserting unchunked text, textChunk is allowed to be
// larger than CHUNKSIZE, and textChunk's totalLines field does not have to be accurate
// Finally, this is does not guarantee that it will not alter the chunk list of
// theUniverse before the point of insertion.
// NOTE: if bytes are inserted, this will update the cache to point to the chunk that the insertion started in
{
	bool
		fail;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;

	fail=false;
	if(numBytes)															// only do more work if bytes to insert
	{
		PositionToChunkPosition(theUniverse,thePosition,&theChunk,&theOffset);	// locate the position, move the cache there
		if(theChunk)														// see if inserting within the text
		{
			if(theOffset>theChunk->totalBytes)								// if this is out of range, fudge it back!
			{
				theOffset=theChunk->totalBytes;
			}
			if((theOffset==0)&&(theChunk->previousHeader)&&(theChunk->previousHeader->totalBytes<CHUNKSIZE))	// if inserting at the very start of a chunk, it is more efficient to look one back (if possible)
			{
				theUniverse->cacheChunkHeader=theChunk=theChunk->previousHeader;
				theUniverse->cacheBytes-=theChunk->totalBytes;				// move the cache back one, so it will not be effected by the insert
				theUniverse->cacheLines-=theChunk->totalLines;
				theOffset=theChunk->totalBytes;
			}
			if(numBytes<=CHUNKSIZE-(theChunk->totalBytes))					// see if the bytes can be placed in empty space of this chunk
			{
				fail=!InsertWithRoomInChunk(theUniverse,theChunk,theOffset,&textChunk,&textOffset,numBytes);
			}
			else
			{
				if(theChunk->nextHeader)									// is there another chunk after this one?
				{
					if(numBytes<=(2*CHUNKSIZE)-(theChunk->totalBytes+theChunk->nextHeader->totalBytes))	// is there room in the combined free space of this chunk, and the next one?
					{
						fail=!InsertWithRoomInTwoChunks(theUniverse,theChunk,theOffset,&textChunk,&textOffset,numBytes);
					}
					else
					{
						fail=!InsertWithNoRoom(theUniverse,theChunk,theOffset,&textChunk,&textOffset,numBytes);
					}
				}
				else	// last chunk does not have enough space to hold the text
				{
					fail=!InsertAtLastChunk(theUniverse,theChunk,theOffset,&textChunk,&textOffset,numBytes);
				}
			}
		}
		else
		{
			// inserting text at the very end (cache is set to NULL in this case)
			if((theChunk=theUniverse->lastChunkHeader))
			{
				if(numBytes<=CHUNKSIZE-(theChunk->totalBytes))					// see if the bytes can be placed in empty space of this chunk
				{
					fail=!InsertWithRoomInChunk(theUniverse,theChunk,theChunk->totalBytes,&textChunk,&textOffset,numBytes);
				}
				else
				{
					fail=!InsertAtLastChunk(theUniverse,theChunk,theChunk->totalBytes,&textChunk,&textOffset,numBytes);
				}
			}
			else
			{
				fail=!InsertWithNoChunks(theUniverse,&textChunk,&textOffset,numBytes);
			}
		}
	}
	return(!fail);
}

bool InsertUniverseText(TEXTUNIVERSE *theUniverse,UINT32 thePosition,UINT8 *theText,UINT32 numBytes)
// insert numBytes of theText into theUniverse starting at thePosition
// if there is a problem, SetError, and return false
// this routine is allowed to either succeed, or fail completely, under no circumstance
// is it allowed to insert only part of the given text
// if there is a problem, SetError, and return false
// NOTE: this will update the cache to point to the chunk that the insertion started in
{
	CHUNKHEADER
		psuedoChunk;														// used to create a chunk that points to theText

	psuedoChunk.previousHeader=psuedoChunk.nextHeader=NULL;					// set up fake chunk header, so we can call insert chunk routine
	psuedoChunk.data=theText;
	psuedoChunk.totalBytes=numBytes;
	psuedoChunk.totalLines=0;												// this does not have to be set accurately
	return(InsertUniverseChunks(theUniverse,thePosition,&psuedoChunk,0,numBytes));
}

bool ExtractUniverseText(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT8 *theText,UINT32 numBytes,CHUNKHEADER **nextChunk,UINT32 *nextOffset)
// extract numBytes of theText from theUniverse at theChunk/theOffset
// if there are not numBytes of text in theUniverse at theChunk/theOffset, then
// as many as possible will be returned
// it is acceptable to pass theChunk as NULL, in which case, no bytes will ever be returned
// theText must be large enough to accept numBytes or bad things will happen
// if there is a problem, SetError, and return false
{
	UINT32
		numToExtract;

	while(numBytes&&theChunk)
	{
		if((theChunk->totalBytes-theOffset)<numBytes)
		{
			numToExtract=theChunk->totalBytes-theOffset;
		}
		else
		{
			numToExtract=numBytes;
		}
		MMoveMem(&theChunk->data[theOffset],theText,numToExtract);
		numBytes-=numToExtract;
		theText+=numToExtract;
		theOffset+=numToExtract;
		if(theOffset>=theChunk->totalBytes)
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	(*nextChunk)=theChunk;
	(*nextOffset)=theOffset;
	return(true);
}

static void UnlinkChunkList(TEXTUNIVERSE *theUniverse,CHUNKHEADER *startChunk,CHUNKHEADER *endChunk)
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

static void DeleteWithinChunk(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numBytes,CHUNKHEADER **nextChunk,UINT32 *nextOffset)
// data within one chunk is being deleted
// NOTE: if the cache points to the given chunk, and it is completely deleted,
// the cache will be moved forward to the next chunk in the list
{
	UINT32
		linesDeleted;

	if(numBytes<theChunk->totalBytes)	// see if something will remain after the deletion
	{
		DetermineNewLines(theChunk,theOffset,numBytes,&linesDeleted);
		MMoveMem(&(theChunk->data[theOffset+numBytes]),&(theChunk->data[theOffset]),theChunk->totalBytes-(theOffset+numBytes));	// move back any bytes needed
		theChunk->totalBytes-=numBytes;
		theChunk->totalLines-=linesDeleted;
		theUniverse->totalBytes-=numBytes;
		theUniverse->totalLines-=linesDeleted;
		if(theOffset>=theChunk->totalBytes)
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
		if(theUniverse->cacheChunkHeader==theChunk)		// if cached chunk is going away, then just move it to the next one
		{
			theUniverse->cacheChunkHeader=theChunk->nextHeader;
		}
		(*nextChunk)=theChunk->nextHeader;
		(*nextOffset)=0;
		UnlinkChunkList(theUniverse,theChunk,theChunk);
		theUniverse->totalBytes-=theChunk->totalBytes;
		theUniverse->totalLines-=theChunk->totalLines;
		DisposeChunkList(theChunk);
	}
}

bool DeleteUniverseText(TEXTUNIVERSE *theUniverse,UINT32 thePosition,UINT32 numBytes)
// delete numBytes of text from theUniverse starting at thePosition
// if there is a problem, SetError, and return false
// this routine is allowed to either succeed, or fail completely, under no circumstance
// is it allowed to delete only part of the given text
// NOTE: this will move the cache position to the chunk where the deletion started
{
	UINT32
		numToDelete;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		fail;

	fail=false;
	if(numBytes)
	{
		PositionToChunkPosition(theUniverse,thePosition,&theChunk,&theOffset);	// locate the position, move the cache there
		while(numBytes&&theChunk)
		{
			if((theChunk->totalBytes-theOffset)<numBytes)
			{
				numToDelete=theChunk->totalBytes-theOffset;
			}
			else
			{
				numToDelete=numBytes;
			}
			DeleteWithinChunk(theUniverse,theChunk,theOffset,numToDelete,&theChunk,&theOffset);
			numBytes-=numToDelete;
		}
	}
	return(!fail);
}

TEXTUNIVERSE *OpenTextUniverse()
// create the data structures necessary to manage
// a universe of text
// if there is a problem, SetError, and return NULL
{
	TEXTUNIVERSE
		*theUniverse;

	if((theUniverse=(TEXTUNIVERSE *)MNewPtr(sizeof(TEXTUNIVERSE))))	// create an empty universe
	{
		theUniverse->firstChunkHeader=theUniverse->lastChunkHeader=theUniverse->cacheChunkHeader=(CHUNKHEADER *)NULL;
		theUniverse->totalBytes=0;
		theUniverse->totalLines=0;
		theUniverse->cacheBytes=0;
		theUniverse->cacheLines=0;
		return(theUniverse);
	}
	return((TEXTUNIVERSE *)NULL);
}

void CloseTextUniverse(TEXTUNIVERSE *theUniverse)
// dispose of a text universe
{
	DisposeChunkList(theUniverse->firstChunkHeader);
	MDisposePtr(theUniverse);
}

