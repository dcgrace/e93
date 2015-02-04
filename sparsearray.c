// Sparse array management functions
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

// Sparse arrays are arrays of arbitrary items of which most
// elements are usually unoccupied.
// These are handy for describing selections, styles, and other
// things within e93.

// To create a sparse array, define a structure of which the first
// element is a SPARSEARRAYHEADER.

#include	"includes.h"

#define	HEADEROFFSET(theSparseArray,theChunk,theOffset) ((SPARSEARRAYHEADER *)(&theChunk->data[(theOffset)*theSparseArray->elementSize]))->offset
#define	HEADERLENGTH(theSparseArray,theChunk,theOffset) ((SPARSEARRAYHEADER *)(&theChunk->data[(theOffset)*theSparseArray->elementSize]))->length

/* This should not be needed anymore ---------------------------------------------------------
static void SanityTest(SPARSEARRAY *theSparseArray)
// Look at the cached information about theSparseArray, and verify that
// it is correct
// If there is a problem, send it to the console
{
	ARRAYCHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		sumOffset;
	bool
		sawCacheChunk;


	theChunk=theSparseArray->elementChunks.firstChunkHeader;
	sumOffset=0;
	theOffset=0;
	sawCacheChunk=false;
	while(theChunk)
	{
		if(theChunk==theSparseArray->cacheChunkHeader&&theOffset==theSparseArray->cacheOffset)
		{
			sawCacheChunk=true;
			if(sumOffset!=theSparseArray->cacheCurrentElement)
			{
				fprintf(stderr,"BAD current element: CE:%d ACE:%d%c\n",theSparseArray->cacheCurrentElement,sumOffset,7);
			}
		}
		if(HEADEROFFSET(theSparseArray,theChunk,theOffset)>2000000)
		{
			fprintf(stderr,"Offset looks a little large:%d%c\n",HEADEROFFSET(theSparseArray,theChunk,theOffset),7);
		}
		if(HEADERLENGTH(theSparseArray,theChunk,theOffset)>2000000)
		{
			fprintf(stderr,"Length looks a little large:%d%c\n",HEADEROFFSET(theSparseArray,theChunk,theOffset),7);
		}
		sumOffset+=HEADEROFFSET(theSparseArray,theChunk,theOffset);
		sumOffset+=HEADERLENGTH(theSparseArray,theChunk,theOffset);
		theOffset++;
		if(theOffset>=theChunk->totalElements)	// move to the next chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
	if(sumOffset!=theSparseArray->totalElements)
	{
		fprintf(stderr,"BAD total elements: TE:%d ATE:%d%c\n",theSparseArray->totalElements,sumOffset,7);
	}
	if(theSparseArray->cacheChunkHeader&&!sawCacheChunk)
	{
		fprintf(stderr,"BAD cache chunk/offset %lX/%d (not seen in array!)%c\n",theSparseArray->cacheChunkHeader,theSparseArray->cacheOffset,7);
	}
}

void DumpSATable(SPARSEARRAY *theSparseArray)
// spew out the sparse array table passed (for debugging)
{
	ARRAYCHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		sumOffset;

	fprintf(stderr,"offset=%d cachecount=%d, totalE=%d\n",theSparseArray->cacheOffset,theSparseArray->cacheCurrentElement,theSparseArray->totalElements);

	fprintf(stderr,"\n");
	theChunk=theSparseArray->elementChunks.firstChunkHeader;
	sumOffset=0;
	theOffset=0;
	while(theChunk)
	{
		sumOffset+=HEADEROFFSET(theSparseArray,theChunk,theOffset);
		fprintf(stderr,"offset:%d (%d) length:%d\n",HEADEROFFSET(theSparseArray,theChunk,theOffset),sumOffset,HEADERLENGTH(theSparseArray,theChunk,theOffset));
		sumOffset+=HEADERLENGTH(theSparseArray,theChunk,theOffset);
		theOffset++;
		if(theOffset>=theChunk->totalElements)	// move to the next chunk
		{
			theChunk=theChunk->nextHeader;
			theOffset=0;
		}
	}
}
*/

static void LocateSARecord(SPARSEARRAY *theSparseArray,UINT32 position,ARRAYCHUNKHEADER **outChunk,UINT32 *outOffset,UINT32 *startPosition)
// Use the cache to quickly locate the record in theSparseArray which contains position
// return an offset to the chunk of the array universe which contains the
// record in question, and the offset to the record
// also return the actual starting position of the record (the position where its offset begins)
// NOTE: if the passed position is after the last entry in the array, return
// outChunk as NULL, outOffset at 0, and startPosition as the total
// number of elements in the array
// NOTE: except for the cases when this returns NULL, the caller can always
// guarantee that the cache is left pointing to the values returned
{
	ARRAYCHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		currentIndex;
	UINT32
		recordLength;

	if(theSparseArray->elementChunks.firstChunkHeader&&(position<theSparseArray->totalElements))
	{
		if(!theSparseArray->cacheChunkHeader)	// if no current cache header, see if position is closer to start or end
		{
			if(position<=(theSparseArray->totalElements/2))		// see if search should start from beginning or end
			{
				theSparseArray->cacheChunkHeader=theSparseArray->elementChunks.firstChunkHeader;
				theSparseArray->cacheOffset=0;
				theSparseArray->cacheCurrentElement=0;
			}
			else
			{
				theSparseArray->cacheChunkHeader=theSparseArray->elementChunks.lastChunkHeader;
				theSparseArray->cacheOffset=theSparseArray->elementChunks.lastChunkHeader->totalElements;
				theSparseArray->cacheCurrentElement=theSparseArray->totalElements;
			}
		}

		theChunk=theSparseArray->cacheChunkHeader;
		theOffset=theSparseArray->cacheOffset;
		currentIndex=theSparseArray->cacheCurrentElement;

		if(position>=currentIndex)						// hunt forwards for the element
		{
			while(position>=currentIndex+(recordLength=HEADEROFFSET(theSparseArray,theChunk,theOffset)+HEADERLENGTH(theSparseArray,theChunk,theOffset)))	// this should never run off the end of the list because of the test above to make sure position is not too large
			{
				currentIndex+=recordLength;
				theOffset++;
				if(theOffset>=theChunk->totalElements)
				{
					theChunk=theChunk->nextHeader;
					theOffset=0;
				}
			}
			*outChunk=theSparseArray->cacheChunkHeader=theChunk;		// write back to the cache
			*outOffset=theSparseArray->cacheOffset=theOffset;
			*startPosition=theSparseArray->cacheCurrentElement=currentIndex;
		}
		else	// hunting backwards
		{
			if(theOffset)								// step back one
			{
				theOffset--;
			}
			else
			{
				theChunk=theChunk->previousHeader;		// we know this will succeed because of previous tests
				theOffset=theChunk->totalElements-1;
			}
			currentIndex-=(HEADEROFFSET(theSparseArray,theChunk,theOffset)+HEADERLENGTH(theSparseArray,theChunk,theOffset));
			while(position<currentIndex)
			{
				if(theOffset)							// step back one
				{
					theOffset--;
				}
				else
				{
					theChunk=theChunk->previousHeader;	// we know this will succeed because of previous tests
					theOffset=theChunk->totalElements-1;
				}
				currentIndex-=(HEADEROFFSET(theSparseArray,theChunk,theOffset)+HEADERLENGTH(theSparseArray,theChunk,theOffset));
			}
			*outChunk=theSparseArray->cacheChunkHeader=theChunk;		// write back to the cache
			*outOffset=theSparseArray->cacheOffset=theOffset;
			*startPosition=theSparseArray->cacheCurrentElement=currentIndex;
		}
	}
	else	// position beyond end of array, or no array
	{
		*outChunk=NULL;
		*outOffset=0;
		*startPosition=theSparseArray->totalElements;
	}
}

void DeleteSARange(SPARSEARRAY *theSparseArray,UINT32 startPosition,UINT32 numElements)
// Remove sparse array elements from startPosition for numElements
// NOTE: this does not mind being called for positions outside of the current array
// NOTE: This was a complete pain to write and is hard to think about.
{
	ARRAYCHUNKHEADER
		*theChunk,
		*firstChunk;
	UINT32
		theOffset,
		firstOffset;
	UINT32
		actualStart;
	UINT32
		numRecords;
	UINT32
		startRecordIndex,
		endRecordIndex,
		elementsLeftInRecord;
	UINT32
		offset,
		startOffset,
		length,
		startLength;

	if(numElements&&(startPosition<theSparseArray->totalElements))	// make sure there's work to be done
	{
		if(numElements>=(theSparseArray->totalElements-startPosition))
		{
			numElements=theSparseArray->totalElements-startPosition;	// truncate down the number of elements to the number we have
		}

		LocateSARecord(theSparseArray,startPosition,&theChunk,&theOffset,&actualStart);

		theSparseArray->totalElements-=numElements;			// removing at least this many elements from the array

		startOffset=offset=HEADEROFFSET(theSparseArray,theChunk,theOffset);
		startLength=length=HEADERLENGTH(theSparseArray,theChunk,theOffset);

		startRecordIndex=startPosition-actualStart;			// find out how far into the starting record we are

		elementsLeftInRecord=offset+length-startRecordIndex;

		firstChunk=theChunk;								// hold onto these
		firstOffset=theOffset;

		numRecords=0;										// keep track of the number of records stepped over here

		while(numElements>elementsLeftInRecord)
		{
			numElements-=elementsLeftInRecord;
			numRecords++;
			theOffset++;
			if(theOffset>=theChunk->totalElements)			// move to the next chunk
			{
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
			offset=HEADEROFFSET(theSparseArray,theChunk,theOffset);
			length=HEADERLENGTH(theSparseArray,theChunk,theOffset);
			elementsLeftInRecord=offset+length;
		}
		elementsLeftInRecord-=numElements;

		endRecordIndex=offset+length-elementsLeftInRecord;

		if(numRecords)										// see if we stepped to a different record than we started in
		{
			if(startRecordIndex>startOffset)				// see if first record needs truncation or deletion
			{
				HEADERLENGTH(theSparseArray,firstChunk,firstOffset)=startRecordIndex-startOffset;
				firstOffset++;
				if(firstOffset>=firstChunk->totalElements)	// move to the next chunk to start deletion
				{
					firstChunk=firstChunk->nextHeader;
					firstOffset=0;
				}
				numRecords--;
			}
			if(endRecordIndex==offset+length)				// does last record need deletion?
			{
				numRecords++;
			}
			if(numRecords)									// anything needs deletion?
			{
				DeleteUniverseArray(&theSparseArray->elementChunks,firstChunk,firstOffset,numRecords,&theChunk,&theOffset);
			}
			if(theChunk)									// anything remaining after deletion?
			{
				if(endRecordIndex<offset+length)			// see if last record was completely deleted, or needs to be cropped
				{
					if(endRecordIndex<=offset)
					{
						if(startRecordIndex<=startOffset)	// see if the start record was deleted, and there is some offset which needs to be propogated to this record
						{
							HEADEROFFSET(theSparseArray,theChunk,theOffset)+=startRecordIndex-endRecordIndex;	// add in the offset left over from the start
						}
						else
						{
							HEADEROFFSET(theSparseArray,theChunk,theOffset)-=endRecordIndex;
							theSparseArray->cacheCurrentElement=startPosition;	// start record was not deleted, so new record begins at start position
						}
					}
					else
					{
						if(startRecordIndex<=startOffset)	// see if the start record was deleted, and there is some offset which needs to be propogated to this record
						{
							HEADEROFFSET(theSparseArray,theChunk,theOffset)=startRecordIndex;
						}
						else
						{
							HEADEROFFSET(theSparseArray,theChunk,theOffset)=0;
							theSparseArray->cacheCurrentElement=startPosition;	// start record was not deleted, so new record begins at start position
						}
						HEADERLENGTH(theSparseArray,theChunk,theOffset)=offset+length-endRecordIndex;
					}
				}
				else										// last record was deleted
				{
					if(startRecordIndex<=startOffset)		// see if the start record was deleted, and there is some offset which needs to be propogated to this record
					{
						HEADEROFFSET(theSparseArray,theChunk,theOffset)+=startRecordIndex;	// add in the offset left over from the start
					}
					else
					{
						theSparseArray->cacheCurrentElement=startPosition;	// start record was not deleted, so new record begins at start position
					}
				}
			}
			else
			{
				if(startRecordIndex<=startOffset)			// see if first record needed truncation or deletion
				{
					theSparseArray->totalElements-=startRecordIndex;		// get rid of offset at end of array
				}
			}
			theSparseArray->cacheChunkHeader=theChunk;
			theSparseArray->cacheOffset=theOffset;
		}
		else												// deletion began an ended in one record
		{
			if(endRecordIndex<offset+length)				// ended within the record
			{
				if(endRecordIndex>offset)					// ended within the length
				{
					if(startRecordIndex>=offset)			// started within the length, so just reduce the length
					{
						HEADERLENGTH(theSparseArray,theChunk,theOffset)-=endRecordIndex-startRecordIndex;
					}
					else									// started in the offset, ended in the length
					{
						HEADEROFFSET(theSparseArray,theChunk,theOffset)-=offset-startRecordIndex;
						HEADERLENGTH(theSparseArray,theChunk,theOffset)-=endRecordIndex-offset;
					}
				}
				else										// ended within the offset (so it started within the offset, so just reduce the offset)
				{
					HEADEROFFSET(theSparseArray,theChunk,theOffset)-=endRecordIndex-startRecordIndex;
				}
			}
			else	// ended at the end of the record
			{
				if(startRecordIndex>offset)					// ended at the end, but started within the length, so just truncate
				{
					HEADERLENGTH(theSparseArray,theChunk,theOffset)=startRecordIndex-offset;
				}
				else				// ended at the end, and started before the length, so kill this record, and apply remaining offset to next (if there is one)
				{
					DeleteUniverseArray(&theSparseArray->elementChunks,theChunk,theOffset,1,&theChunk,&theOffset);
					if(theChunk)
					{
						HEADEROFFSET(theSparseArray,theChunk,theOffset)+=startRecordIndex;
					}
					else
					{
						theSparseArray->totalElements-=startRecordIndex;		// get rid of offset at end of array
					}
					theSparseArray->cacheChunkHeader=theChunk;
					theSparseArray->cacheOffset=theOffset;
				}
			}
		}
	}
}

bool InsertSARange(SPARSEARRAY *theSparseArray,UINT32 startPosition,UINT32 numElements)
// insert numElements into the array at startPosition
// This will expand the array as needed, pushing out either the offset or length
// of a record (or doing nothing if the insertion is past the end)
{
	ARRAYCHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		actualStart;
	bool
		fail;

	fail=false;
	if(numElements)			// if no elements to insert, then do nothing
	{
		LocateSARecord(theSparseArray,startPosition,&theChunk,&theOffset,&actualStart);
		if(theChunk)		// did something already exist in the array at this point? If not, then do nothing
		{
			theSparseArray->totalElements+=numElements;		// this many elements going in
			if((startPosition-actualStart)>HEADEROFFSET(theSparseArray,theChunk,theOffset))
			{
				HEADERLENGTH(theSparseArray,theChunk,theOffset)+=numElements;
			}
			else
			{
				HEADEROFFSET(theSparseArray,theChunk,theOffset)+=numElements;
			}
		}
	}
	return(!fail);
}

bool GetSARecordForPosition(SPARSEARRAY *theSparseArray,UINT32 position,UINT32 *startPosition,UINT32 *numElements,void **theRecord)
// Return the pointer to the record of theSparseArray which occupies position.
// If the passed position is past the end of the array, return false.
// If the passed position is in an empty part of the array, return theRecord as NULL, with
// startPosition and numElements describing the empty part of the array.
// NOTE: this uses a cache so it is efficient to call
// repeatedly for positions which are nearby each other
// NOTE: the pointer is only valid until the next call to any sparse array routine
{
	ARRAYCHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		headerOffset;

	LocateSARecord(theSparseArray,position,&theChunk,&theOffset,startPosition);
	if(theChunk)
	{
		headerOffset=HEADEROFFSET(theSparseArray,theChunk,theOffset);
		if(position>=(*startPosition+headerOffset))
		{
			*startPosition+=headerOffset;
			*numElements=HEADERLENGTH(theSparseArray,theChunk,theOffset);
			*theRecord=&theChunk->data[theOffset*theSparseArray->elementSize];
		}
		else
		{
			*numElements=headerOffset;
			*theRecord=NULL;
		}
		return(true);
	}
	return(false);
}

bool GetSARecordAtOrAfterPosition(SPARSEARRAY *theSparseArray,UINT32 position,UINT32 *startPosition,UINT32 *numElements,void **theRecord)
// Return the pointer to the record of theSparseArray which occupies position,
// or the first one after the position if the position is not occupied.
// If the passed position is past the end of the array, return false
// NOTE: this uses a cache so it is efficient to call
// repeatedly for positions which are nearby each other
// NOTE: the pointer is only valid until the next call to any sparse array routine
{
	ARRAYCHUNKHEADER
		*theChunk;
	UINT32
		theOffset;

	LocateSARecord(theSparseArray,position,&theChunk,&theOffset,startPosition);
	if(theChunk)
	{
		*startPosition+=HEADEROFFSET(theSparseArray,theChunk,theOffset);
		*numElements=HEADERLENGTH(theSparseArray,theChunk,theOffset);
		*theRecord=&theChunk->data[theOffset*theSparseArray->elementSize];
		return(true);
	}
	return(false);
}

bool GetSARecordAtOrBeforePosition(SPARSEARRAY *theSparseArray,UINT32 position,UINT32 *startPosition,UINT32 *numElements,void **theRecord)
// Return the pointer to the record of theSparseArray which occupies position,
// or the first one before the position if the position is not occupied.
// If the passed position is before the first element of the array, return false.
// NOTE: this uses a cache so it is efficient to call
// repeatedly for positions which are nearby each other
// NOTE: the pointer is only valid until the next call to any sparse array routine
{
	ARRAYCHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		headerOffset;

	LocateSARecord(theSparseArray,position,&theChunk,&theOffset,startPosition);
	if(theChunk)
	{
		headerOffset=HEADEROFFSET(theSparseArray,theChunk,theOffset);
		if(position>=*startPosition+headerOffset)
		{
			*startPosition+=headerOffset;
			*numElements=HEADERLENGTH(theSparseArray,theChunk,theOffset);
			*theRecord=&theChunk->data[theOffset*theSparseArray->elementSize];
			return(true);
		}
		else	// get element that precedes this one
		{
			if(theOffset)							// step back one
			{
				theOffset--;
			}
			else
			{
				if((theChunk=theChunk->previousHeader))
				{
					theOffset=theChunk->totalElements-1;
				}
			}
			if(theChunk)
			{
				*startPosition-=HEADERLENGTH(theSparseArray,theChunk,theOffset);
				*numElements=HEADERLENGTH(theSparseArray,theChunk,theOffset);
				*theRecord=&theChunk->data[theOffset*theSparseArray->elementSize];
				return(true);
			}
		}
	}
	else
	{
		if((theChunk=theSparseArray->elementChunks.lastChunkHeader))
		{
			theOffset=theSparseArray->elementChunks.lastChunkHeader->totalElements;
			*startPosition=theSparseArray->totalElements-HEADERLENGTH(theSparseArray,theChunk,theOffset);
			*numElements=HEADERLENGTH(theSparseArray,theChunk,theOffset);
			*theRecord=&theChunk->data[theOffset*theSparseArray->elementSize];
			return(true);
		}
	}
	return(false);
}

bool SetSARange(SPARSEARRAY *theSparseArray,UINT32 startPosition,UINT32 numElements,void *theValue)
// Set elements of theSparseArray from startPosition for numElements
// to theValue
// if there is a problem, set the error and return false
// the theValue will overwrite anything currently in the array at
// the given positions, and will cause items to be split
// if it bisects them.
// NOTE: if the range goes beyond the current end of the array,
// the array will be extended.
{
	ARRAYCHUNKHEADER
		*theChunk,
		*newChunk,
		*splitChunk;
	UINT32
		theOffset,
		newOffset,
		splitOffset;
	UINT32
		actualStart;
	bool
		fail;

	fail=false;
	if(numElements)
	{
		DeleteSARange(theSparseArray,startPosition,numElements);	// get rid of any elements that were in that spot
		LocateSARecord(theSparseArray,startPosition,&theChunk,&theOffset,&actualStart);	// find the record containing the start
		if(theChunk)												// if there is a record there, see how we need to add
		{
			if(actualStart+HEADEROFFSET(theSparseArray,theChunk,theOffset)<startPosition)	// need to split?
			{
				if(InsertUniverseArray(&theSparseArray->elementChunks,theChunk,theOffset,2,&theChunk,&theOffset))	// add 2 elements, one for the new stuff, and one for the half of the split we need
				{
					theSparseArray->cacheChunkHeader=theChunk;		// adjust cache for new chunks just added
					theSparseArray->cacheOffset=theOffset;

					newOffset=theOffset;
					newChunk=theChunk;
					newOffset++;
					if(newOffset>=newChunk->totalElements)			// move to the next chunk
					{
						newChunk=newChunk->nextHeader;
						newOffset=0;
					}

					splitOffset=newOffset;
					splitChunk=newChunk;
					splitOffset++;
					if(splitOffset>=splitChunk->totalElements)		// move to the next chunk (which is the original we need to split)
					{
						splitChunk=splitChunk->nextHeader;
						splitOffset=0;
					}

					memcpy(&newChunk->data[newOffset*theSparseArray->elementSize],theValue,theSparseArray->elementSize);	// blast in new value
					memcpy(&theChunk->data[theOffset*theSparseArray->elementSize],&splitChunk->data[splitOffset*theSparseArray->elementSize],theSparseArray->elementSize);	// blast in split value

					// finally, adjust offsets and lengths
					HEADERLENGTH(theSparseArray,theChunk,theOffset)=startPosition-actualStart-HEADEROFFSET(theSparseArray,theChunk,theOffset);	// knock off some length on the first item
					HEADEROFFSET(theSparseArray,newChunk,newOffset)=0;
					HEADERLENGTH(theSparseArray,newChunk,newOffset)=numElements;
					HEADEROFFSET(theSparseArray,splitChunk,splitOffset)=0;
					HEADERLENGTH(theSparseArray,splitChunk,splitOffset)-=startPosition-actualStart-HEADEROFFSET(theSparseArray,theChunk,theOffset);

					theSparseArray->totalElements+=numElements;
				}
				else
				{
					fail=true;
				}
			}
			else	// no splitting, just push other record over, and add this one
			{
				HEADEROFFSET(theSparseArray,theChunk,theOffset)-=startPosition-actualStart;	// reduce the offset of this element
				if(InsertUniverseArray(&theSparseArray->elementChunks,theChunk,theOffset,1,&theChunk,&theOffset))	// add an element
				{
					memcpy(&theChunk->data[theOffset*theSparseArray->elementSize],theValue,theSparseArray->elementSize);	// blast in new value
					HEADEROFFSET(theSparseArray,theChunk,theOffset)=startPosition-actualStart;
					HEADERLENGTH(theSparseArray,theChunk,theOffset)=numElements;
					theSparseArray->cacheCurrentElement=actualStart;
					theSparseArray->cacheChunkHeader=theChunk;
					theSparseArray->cacheOffset=theOffset;

					theSparseArray->totalElements+=numElements;
				}
				else
				{
					fail=true;	// things will not be pretty if this fails, as it leaves the array in an inconsistent state
				}
			}
		}
		else	// no record, so must be at the end, so add a new record there, and set the needed offset at the start (leave cache set to null)
		{
			if(InsertUniverseArray(&theSparseArray->elementChunks,NULL,0,1,&theChunk,&theOffset))	// add an element
			{
				memcpy(&theChunk->data[theOffset*theSparseArray->elementSize],theValue,theSparseArray->elementSize);	// blast in new value
				HEADEROFFSET(theSparseArray,theChunk,theOffset)=startPosition-theSparseArray->totalElements;
				HEADERLENGTH(theSparseArray,theChunk,theOffset)=numElements;

				theSparseArray->totalElements=startPosition+numElements;
			}
			else
			{
				fail=true;
			}
		}
	}
	return(!fail);
}

bool ClearSARange(SPARSEARRAY *theSparseArray,UINT32 startPosition,UINT32 numElements)
// Clear out elements of theSparseArray from startPosition for numElements
// if there is a problem, set the error and return false
// this will overwrite anything currently in the array at
// the given positions, and will cause items to be split
// if it bisects them.
{
	ARRAYCHUNKHEADER
		*theChunk,
		*splitChunk;
	UINT32
		theOffset,
		splitOffset;
	UINT32
		actualStart;
	bool
		fail;

	fail=false;
	if(numElements)
	{
		DeleteSARange(theSparseArray,startPosition,numElements);	// get rid of any elements that were in that spot
		LocateSARecord(theSparseArray,startPosition,&theChunk,&theOffset,&actualStart);	// find the record containing the start
		if(theChunk)												// if there is a record there, see how we need to clear
		{
			if(actualStart+HEADEROFFSET(theSparseArray,theChunk,theOffset)<startPosition)	// need to split?
			{
				if(InsertUniverseArray(&theSparseArray->elementChunks,theChunk,theOffset,1,&theChunk,&theOffset))	// add 1 element for the split we need
				{
					theSparseArray->cacheChunkHeader=theChunk;		// adjust cache for new chunk just added
					theSparseArray->cacheOffset=theOffset;

					splitOffset=theOffset;
					splitChunk=theChunk;
					splitOffset++;
					if(splitOffset>=splitChunk->totalElements)		// move to the next chunk (which is the original we need to split)
					{
						splitChunk=splitChunk->nextHeader;
						splitOffset=0;
					}

					memcpy(&theChunk->data[theOffset*theSparseArray->elementSize],&splitChunk->data[splitOffset*theSparseArray->elementSize],theSparseArray->elementSize);	// blast in split value

					// finally, adjust offsets and lengths
					HEADERLENGTH(theSparseArray,theChunk,theOffset)=startPosition-actualStart-HEADEROFFSET(theSparseArray,theChunk,theOffset);	// knock off some length on the first item
					HEADEROFFSET(theSparseArray,splitChunk,splitOffset)=numElements;
					HEADERLENGTH(theSparseArray,splitChunk,splitOffset)-=startPosition-actualStart-HEADEROFFSET(theSparseArray,theChunk,theOffset);

					theSparseArray->totalElements+=numElements;
				}
				else
				{
					fail=true;
				}
			}
			else	// no splitting, just push other record over by the amount to clear
			{
				HEADEROFFSET(theSparseArray,theChunk,theOffset)+=numElements;
				theSparseArray->totalElements+=numElements;
			}
		}
	}
	return(!fail);
}

bool IsSAEmpty(SPARSEARRAY *theSparseArray)
// return true if the given sparse array contains no elements, false otherwise
{
	if(theSparseArray->totalElements)
	{
		return(false);
	}
	return(true);
}

bool GetSAElementEnds(SPARSEARRAY *theSparseArray,UINT32 *startElementOffset,UINT32 *endElementOffset)
// if the given sparse array contains elements, fill in startElementOffset as the
// index to the first used element in the array, and endElementOffset as the offset
// of the last used element in the array (+1), then return true
// otherwise, return false.
{
	if(theSparseArray->totalElements)
	{
		(*startElementOffset)=HEADEROFFSET(theSparseArray,theSparseArray->elementChunks.firstChunkHeader,0);
		(*endElementOffset)=theSparseArray->totalElements;
		return(true);
	}
	return(false);
}

void UnInitSA(SPARSEARRAY *theSparseArray)
// uninitialize a sparse array
{
	UnInitArrayUniverse(&(theSparseArray->elementChunks));
}

bool InitSA(SPARSEARRAY *theSparseArray,UINT32 elementSize)
// initialize a sparse array
// if there is a problem, set the error, and return false
{
	theSparseArray->elementSize=elementSize;	// remember this as we need it when moving through the array
	theSparseArray->totalElements=0;

	theSparseArray->cacheChunkHeader=NULL;		// initialize the cache handling
	theSparseArray->cacheOffset=0;
	theSparseArray->cacheCurrentElement=0;

	if(InitArrayUniverse(&(theSparseArray->elementChunks),256,elementSize))
	{
		return(true);
	}
	return(false);
}
