//	Search/replace functions
//	Copyright (C) 2000 Core Technologies.

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

static char localErrorFamily[]="search";

enum
{
	MATCHEDNOTHING,
	TCLERROR
};

static char *errorMembers[]=
{
	"MatchedNothing",
	"TclError"
};

static char *errorDescriptions[]=
{
	"Find matched nothing -- could not continue",
	"Tcl error"
};

typedef struct
{
	bool
		ignoreCase;					// true if we should ignore case, false if not
	TEXTUNIVERSE
		*searchForText;				// pointer to the search for text universe
	TEXTUNIVERSE
		*replaceWithText;			// pointer to the replace with text universe
	COMPILEDEXPRESSION
		*expression;				// if type is regex, this points to the compiled expression, otherwise it is NULL
	char
		*procedure;					// if non NULL, this points to a procedure to use instead of the replace text
} SEARCHRECORD;

bool LiteralMatch(CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER *forTextChunk,UINT32 forTextOffset,UINT32 numToSearch,CHUNKHEADER **endChunk,UINT32 *endOffset)
// see if the text at theChunk/theOffset matches the text in forTextChunk/forTextOffset
// if there is a match, return true with endChunk, and endOffset filled in to point to one
// past the last place looked in inText
// NOTE: there must be at least numToSearch bytes remaining in both theChunk/theOffset and forTextChunk/forTextOffset
{
	UINT8
		*inPointer,
		*forPointer;
	UINT32
		inTotal,
		forTotal;

	if(numToSearch)
	{
		if(forTextChunk->data[forTextOffset]==theChunk->data[theOffset])		// for speed, make initial check to see if this can match
		{
			inTotal=theChunk->totalBytes;										// cache total bytes, and pointer
			inPointer=&(theChunk->data[theOffset+1]);
			forTotal=forTextChunk->totalBytes;
			forPointer=&(forTextChunk->data[forTextOffset+1]);
			if((++theOffset)>=inTotal)											// move past bytes just checked
			{
				theOffset=0;
				if((theChunk=theChunk->nextHeader))
				{
					inTotal=theChunk->totalBytes;
					inPointer=theChunk->data;
				}
			}
			if((++forTextOffset)>=forTotal)
			{
				forTextOffset=0;
				if((forTextChunk=forTextChunk->nextHeader))
				{
					forTotal=forTextChunk->totalBytes;
					forPointer=forTextChunk->data;
				}
			}
			numToSearch--;
			while(numToSearch&&(*inPointer++==*forPointer++))					// go until mismatch, or until we run out of for data
			{
				if((++theOffset)>=inTotal)
				{
					theOffset=0;
					if((theChunk=theChunk->nextHeader))
					{
						inTotal=theChunk->totalBytes;
						inPointer=theChunk->data;
					}
				}
				if((++forTextOffset)>=forTotal)
				{
					forTextOffset=0;
					if((forTextChunk=forTextChunk->nextHeader))
					{
						forTotal=forTextChunk->totalBytes;
						forPointer=forTextChunk->data;
					}
				}
				numToSearch--;
			}
			if(!numToSearch)
			{
				*endChunk=theChunk;
				*endOffset=theOffset;
				return(true);
			}
		}
	}
	else
	{
		*endChunk=theChunk;
		*endOffset=theOffset;
		return(true);
	}
	return(false);
}

bool LiteralMatchTT(CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER *forTextChunk,UINT32 forTextOffset,UINT32 numToSearch,const UINT8 *translateTable,CHUNKHEADER **endChunk,UINT32 *endOffset)
// see if the text at theChunk/theOffset matches the text in forTextChunk/forTextOffset
// if there is a match, return true with endChunk, and endOffset filled in to point to one
// past the last place looked in inText
// NOTE: there must be at least numToSearch bytes remaining in both theChunk/theOffset and forTextChunk/forTextOffset
// NOTE: translate characters when matching
{
	UINT8
		*inPointer,
		*forPointer;
	UINT32
		inTotal,
		forTotal;

	if(numToSearch)
	{
		if(translateTable[forTextChunk->data[forTextOffset]]==translateTable[theChunk->data[theOffset]])		// for speed, make initial check to see if this can match
		{
			inTotal=theChunk->totalBytes;										// cache total bytes, and pointer
			inPointer=&(theChunk->data[theOffset+1]);
			forTotal=forTextChunk->totalBytes;
			forPointer=&(forTextChunk->data[forTextOffset+1]);
			if((++theOffset)>=inTotal)											// move past bytes just checked
			{
				theOffset=0;
				if((theChunk=theChunk->nextHeader))
				{
					inTotal=theChunk->totalBytes;
					inPointer=theChunk->data;
				}
			}
			if((++forTextOffset)>=forTotal)
			{
				forTextOffset=0;
				if((forTextChunk=forTextChunk->nextHeader))
				{
					forTotal=forTextChunk->totalBytes;
					forPointer=forTextChunk->data;
				}
			}
			numToSearch--;
			while(numToSearch&&(translateTable[*inPointer++]==translateTable[*forPointer++]))	// go until mismatch, or until we run out of for data
			{
				if((++theOffset)>=inTotal)
				{
					theOffset=0;
					if((theChunk=theChunk->nextHeader))
					{
						inTotal=theChunk->totalBytes;
						inPointer=theChunk->data;
					}
				}
				if((++forTextOffset)>=forTotal)
				{
					forTextOffset=0;
					if((forTextChunk=forTextChunk->nextHeader))
					{
						forTotal=forTextChunk->totalBytes;
						forPointer=forTextChunk->data;
					}
				}
				numToSearch--;
			}
			if(!numToSearch)
			{
				*endChunk=theChunk;
				*endOffset=theOffset;
				return(true);
			}
		}
	}
	else
	{
		*endChunk=theChunk;
		*endOffset=theOffset;
		return(true);
	}
	return(false);
}

bool SearchForwardLiteral(CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER *forTextChunk,UINT32 forTextOffset,UINT32 forTextLength,UINT32 numToSearch,bool ignoreCase,UINT32 *matchOffset,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset)
// search forward from theChunk/theOffset for forTextChunk/forTextOffset
// look at up to numToSearch bytes of source data
// if a match is located, return true, with matchOffset set to the number
// of bytes in from theChunk/theOffset where the match was found, and
// startChunk/startOffset set to the start of the match, and
// endChunk/endOffset set to one past the last byte of source that was matched
{
	UINT32
		numSearched;
	bool
		found;

	numSearched=0;
	found=false;
	if(ignoreCase)
	{
		while(!found&&((forTextLength+numSearched)<=numToSearch))
		{
			if(LiteralMatchTT(theChunk,theOffset,forTextChunk,forTextOffset,forTextLength,upperCaseTable,endChunk,endOffset))
			{
				*matchOffset=numSearched;
				*startChunk=theChunk;
				*startOffset=theOffset;
				found=true;
			}
			else
			{
				if((++theOffset)>=theChunk->totalBytes)			// move forward in the input
				{
					theOffset=0;
					theChunk=theChunk->nextHeader;
				}
				numSearched++;
			}
		}
	}
	else
	{
		while(!found&&((forTextLength+numSearched)<=numToSearch))
		{
			if(LiteralMatch(theChunk,theOffset,forTextChunk,forTextOffset,forTextLength,endChunk,endOffset))
			{
				*matchOffset=numSearched;
				*startChunk=theChunk;
				*startOffset=theOffset;
				found=true;
			}
			else
			{
				if((++theOffset)>=theChunk->totalBytes)			// move forward in the input
				{
					theOffset=0;
					theChunk=theChunk->nextHeader;
				}
				numSearched++;
			}
		}
	}
	return(found);
}

bool SearchBackwardLiteral(CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER *forTextChunk,UINT32 forTextOffset,UINT32 forTextLength,UINT32 numToSearch,bool ignoreCase,UINT32 *matchOffset,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset)
// search backward from theChunk/theOffset for forTextChunk/forTextOffset
// look at up to numToSearch bytes of source data
// if a match is located, return true, with matchOffset set to the number
// of bytes back from theChunk/theOffset where the match was found, and
// startChunk/startOffset set to theChunk/theOffset at the point of the match
// endChunk/endOffset set to position just after the last match
{
	UINT32
		numSearched;
	UINT32
		amountToBackUp;
	bool
		found;

	numSearched=0;
	found=false;
	if(forTextLength<=numToSearch)
	{
		amountToBackUp=forTextLength;				// back up in the source by forTextLength
		while(amountToBackUp)
		{
			if(theOffset>=amountToBackUp)
			{
				theOffset-=amountToBackUp;
				amountToBackUp=0;
			}
			else
			{
				amountToBackUp-=(theOffset+1);
				if((theChunk=theChunk->previousHeader))
				{
					theOffset=theChunk->totalBytes-1;
				}
			}
		}
		numSearched=forTextLength;
		if(ignoreCase)
		{
			while(!found&&(numSearched<=numToSearch))
			{
				if(LiteralMatchTT(theChunk,theOffset,forTextChunk,forTextOffset,forTextLength,upperCaseTable,endChunk,endOffset))
				{
					*matchOffset=numSearched;
					*startChunk=theChunk;
					*startOffset=theOffset;
					found=true;
				}
				else
				{
					if(!theOffset)					// move backward in the input
					{
						if((theChunk=theChunk->previousHeader))
						{
							theOffset=theChunk->totalBytes;
						}
					}
					theOffset--;
					numSearched++;
				}
			}
		}
		else
		{
			while(!found&&(numSearched<=numToSearch))
			{
				if(LiteralMatch(theChunk,theOffset,forTextChunk,forTextOffset,forTextLength,endChunk,endOffset))
				{
					*matchOffset=numSearched;
					*startChunk=theChunk;
					*startOffset=theOffset;
					found=true;
				}
				else
				{
					if(!theOffset)					// move backward in the input
					{
						if((theChunk=theChunk->previousHeader))
						{
							theOffset=theChunk->totalBytes;
						}
					}
					theOffset--;
					numSearched++;
				}
			}
		}
	}
	return(found);
}

static bool SearchForward(CHUNKHEADER *theChunk,UINT32 theOffset,SEARCHRECORD *theRecord,bool leftEdge,bool rightEdge,UINT32 numToSearch,bool *foundMatch,UINT32 *matchOffset,UINT32 *numMatched,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset)
// search forward given theRecord
// return foundMatch true if a match was found
// return false if there was some sort of hard failure
// if a match is found, startChunk/startOffset point to the start of the match
// matchOffset is the number of bytes searched
// endChunk/endOffset point to one past the end of the matched text
{
	bool
		fail;

	fail=false;
	if(theRecord->expression)
	{
		fail=!SearchForwardRE(theChunk,theOffset,theRecord->expression,leftEdge,rightEdge,numToSearch,numToSearch,theRecord->ignoreCase,foundMatch,matchOffset,numMatched,startChunk,startOffset,endChunk,endOffset,true);
	}
	else
	{
		*foundMatch=SearchForwardLiteral(theChunk,theOffset,theRecord->searchForText->firstChunkHeader,0,theRecord->searchForText->totalBytes,numToSearch,theRecord->ignoreCase,matchOffset,startChunk,startOffset,endChunk,endOffset);
		*numMatched=theRecord->searchForText->totalBytes;
	}
	return(!fail);
}

static bool SearchBackward(CHUNKHEADER *theChunk,UINT32 theOffset,SEARCHRECORD *theRecord,bool leftEdge,bool rightEdge,UINT32 numToSearch,bool *foundMatch,UINT32 *matchOffset,UINT32 *numMatched,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset)
// search backward given theRecord
// return foundMatch true if a match was found
// return false if there was some sort of hard failure
// if a match is found, startChunk/startOffset point to the start of the match
// matchOffset is the number of bytes searched
// endChunk/endOffset point to one past the end of the matched text
{
	bool
		fail;

	fail=false;
	if(theRecord->expression)
	{
		fail=!SearchBackwardRE(theChunk,theOffset,theRecord->expression,leftEdge,rightEdge,numToSearch,numToSearch,theRecord->ignoreCase,foundMatch,matchOffset,numMatched,startChunk,startOffset,endChunk,endOffset,true);
	}
	else
	{
		*foundMatch=SearchBackwardLiteral(theChunk,theOffset,theRecord->searchForText->firstChunkHeader,0,theRecord->searchForText->totalBytes,numToSearch,theRecord->ignoreCase,matchOffset,startChunk,startOffset,endChunk,endOffset);
		*numMatched=theRecord->searchForText->totalBytes;
	}
	return(!fail);
}

static void DisposeSearchRecord(SEARCHRECORD *theRecord)
// get rid of theRecord, and anything allocated under it
{
	if(theRecord->expression)			// if there is a selection expression, get rid of it
	{
		REFree(theRecord->expression);
	}
	if(theRecord->procedure)			// if there is a procedure, get rid of it
	{
		MDisposePtr(theRecord->procedure);
	}
	MDisposePtr(theRecord);
}

static SEARCHRECORD *CreateSearchRecord(TEXTUNIVERSE *searchForText,TEXTUNIVERSE *replaceWithText,bool selectionExpr,bool ignoreCase,bool replaceProc)
// create a search record that describes the search, and remembers some stuff about it
// if there is a problem, SetError, and return NULL
{
	SEARCHRECORD
		*theRecord;
	COMPILEDEXPRESSION
		*theCompiledExpression;
	UINT8
		*theRE;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		fail;

	fail=false;
	if((theRecord=(SEARCHRECORD *)MNewPtr(sizeof(SEARCHRECORD))))
	{
		theRecord->searchForText=searchForText;			// remember these for later
		theRecord->replaceWithText=replaceWithText;
		theRecord->ignoreCase=ignoreCase;
		theRecord->expression=NULL;						// assume no selection expression yet
		theRecord->procedure=NULL;						// assume no procedure yet
		if(replaceProc)									// if treating replace as procedure, we need to grab the text from the replace buffer, and place it into memory
		{
			if((theRecord->procedure=(char *)MNewPtr(replaceWithText->totalBytes+1)))	// make a buffer
			{
				fail=!ExtractUniverseText(replaceWithText,replaceWithText->firstChunkHeader,0,(UINT8 *)theRecord->procedure,replaceWithText->totalBytes,&theChunk,&theOffset);
				theRecord->procedure[replaceWithText->totalBytes]='\0';				// terminate the string
			}
			else
			{
				fail=true;
			}
		}
		if(!fail)
		{
			if(selectionExpr)
			{
				if((theRE=(UINT8 *)MNewPtr(searchForText->totalBytes)))	// make a buffer to temporarily hold the expression text
				{
					if(ExtractUniverseText(searchForText,searchForText->firstChunkHeader,0,theRE,searchForText->totalBytes,&theChunk,&theOffset))
					{
						if((theCompiledExpression=RECompile(&(theRE[0]),searchForText->totalBytes,ignoreCase?upperCaseTable:NULL)))
						{
							theRecord->expression=theCompiledExpression;
							MDisposePtr(theRE);				// get rid of temporary buffer
							return(theRecord);
						}
					}
					MDisposePtr(theRE);
				}
			}
			else
			{
				return(theRecord);
			}
		}
		if(theRecord->procedure)			// if there is a procedure, get rid of it
		{
			MDisposePtr(theRecord->procedure);
		}
		MDisposePtr(theRecord);
	}
	return(NULL);
}

bool EditorFind(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,TEXTUNIVERSE *searchForText,bool backward,bool wrapAround,bool selectionExpr,bool ignoreCase,bool *foundMatch,SELECTIONUNIVERSE *resultSelectionUniverse)
// find searchForText in theBuffer, starting at the cursor position of theSelectionUniverse, or the end of
// the current selection
// If anything is found, return it in resultSelectionUniverse, else, do not modify resultSelectionUniverse.
// foundMatch reflects the status of the search.
// NOTE: it is ok to pass theSelectionUniverse and resultSelectionUniverse as the same universe.
// if there is some problem during the search, SetError, return false
{
	CHUNKHEADER
		*inChunk,
		*startChunk,
		*endChunk;
	UINT32
		inOffset,
		startOffset,
		endOffset;
	SEARCHRECORD
		*theRecord;
	UINT32
		startPosition,
		endPosition;
	UINT32
		locatedPosition,
		locatedLength;
	TEXTUNIVERSE
		*searchInText;
	bool
		fail;

	fail=*foundMatch=false;
	if((theRecord=CreateSearchRecord(searchForText,NULL,selectionExpr,ignoreCase,false)))
	{
		ShowBusy();
		searchInText=theBuffer->textUniverse;

		GetSelectionEndPositions(theSelectionUniverse,&startPosition,&endPosition);		// find ends of old selection, or cursor position
		if(!backward)
		{
			PositionToChunkPosition(searchInText,endPosition,&inChunk,&inOffset);		// locate place to start the search
			if(SearchForward(inChunk,inOffset,theRecord,endPosition==0,true,searchInText->totalBytes-endPosition,foundMatch,&locatedPosition,&locatedLength,&startChunk,&startOffset,&endChunk,&endOffset))
			{
				locatedPosition+=endPosition;											// make located position absolute
				if((!(*foundMatch))&&wrapAround)
				{
					if(!SearchForward(searchInText->firstChunkHeader,0,theRecord,true,startPosition==searchInText->totalBytes,startPosition,foundMatch,&locatedPosition,&locatedLength,&startChunk,&startOffset,&endChunk,&endOffset))
					{
						fail=true;
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
			PositionToChunkPositionPastEnd(searchInText,startPosition,&inChunk,&inOffset);	// locate place to start the search
			if(SearchBackward(inChunk,inOffset,theRecord,true,startPosition==searchInText->totalBytes,startPosition,foundMatch,&locatedPosition,&locatedLength,&startChunk,&startOffset,&endChunk,&endOffset))
			{
				locatedPosition=startPosition-locatedPosition;					// make located position absolute
				if((!(*foundMatch))&&wrapAround)
				{
					PositionToChunkPositionPastEnd(searchInText,searchInText->totalBytes,&inChunk,&inOffset);	// locate place to start the search
					if(SearchBackward(inChunk,inOffset,theRecord,endPosition==0,true,searchInText->totalBytes-endPosition,foundMatch,&locatedPosition,&locatedLength,&startChunk,&startOffset,&endChunk,&endOffset))
					{
						locatedPosition=searchInText->totalBytes-locatedPosition;
					}
					else
					{
						fail=true;
					}
				}
			}
			else
			{
				fail=true;
			}
		}
		if((!fail)&&(*foundMatch))
		{
			EditorSetNormalSelection(theBuffer,resultSelectionUniverse,locatedPosition,locatedPosition+locatedLength);
			SetSelectionCursorPosition(resultSelectionUniverse,locatedPosition);
		}
		DisposeSearchRecord(theRecord);
		ShowNotBusy();
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool FindAllForward(TEXTUNIVERSE *searchInText,bool leftEdge,bool rightEdge,UINT32 lowPosition,UINT32 highPosition,SEARCHRECORD *theRecord,SELECTIONUNIVERSE *theSelectionUniverse,bool *foundMatch,UINT32 *firstMatchPosition)
// Find all occurrences of theRecord between lowPosition, and highPosition, adding the selections
// to theSelectionUniverse
// if there is a problem, SetError and return false
// if a matches were found, return true in foundMatch, and the position of the first match found
// in firstMatchPosition
// NOTE: even if false is returned, foundMatch, firstMatchPosition, and theSelectionUniverse are
// valid
{
	bool
		fail;
	CHUNKHEADER
		*inChunk,
		*startChunk;
	UINT32
		inOffset,
		startOffset;
	bool
		matching;
	UINT32
		nextSearchPosition,
		locatedPosition,
		locatedLength;

	fail=*foundMatch=false;
	nextSearchPosition=lowPosition;
	PositionToChunkPosition(searchInText,nextSearchPosition,&inChunk,&inOffset);		// locate place to start the search
	matching=true;
	while(!fail&&matching)
	{
		if(SearchForward(inChunk,inOffset,theRecord,leftEdge,rightEdge,highPosition-nextSearchPosition,&matching,&locatedPosition,&locatedLength,&startChunk,&startOffset,&inChunk,&inOffset))
		{
			if(matching)
			{
				if(locatedLength)											// finding an empty string is fatal to find all
				{
					leftEdge=false;											// no matter what, we just moved off the left edge
					locatedPosition+=nextSearchPosition;					// make located position absolute
					nextSearchPosition=locatedPosition+locatedLength;		// move to next place to search
					if(!(*foundMatch))
					{
						(*firstMatchPosition)=locatedPosition;
						(*foundMatch)=true;
					}
					fail=!SetSelectionRange(theSelectionUniverse,locatedPosition,locatedLength);
				}
				else
				{
					SetError(localErrorFamily,errorMembers[MATCHEDNOTHING],errorDescriptions[MATCHEDNOTHING]);
					fail=true;
				}
			}
		}
		else
		{
			fail=true;
		}
	}
	return(!fail);
}

static bool FindAllBackward(TEXTUNIVERSE *searchInText,bool leftEdge,bool rightEdge,UINT32 lowPosition,UINT32 highPosition,SEARCHRECORD *theRecord,SELECTIONUNIVERSE *theSelectionUniverse,bool *foundMatch,UINT32 *firstMatchPosition)
// Find all occurrences of theRecord between lowPosition, and highPosition, adding the selections
// to theSelectionUniverse
// if there is a problem, SetError and return false
// if a matches were found, return true in foundMatch, and the position of the first match found
// in firstMatchPosition
// NOTE: even if false is returned, foundMatch, firstMatchPosition, and theSelectionUniverse are
// valid
{
	bool
		fail;
	UINT32
		nextSearchPosition;
	CHUNKHEADER
		*inChunk,
		*endChunk;
	UINT32
		inOffset,
		endOffset;
	bool
		matching;
	UINT32
		locatedPosition,
		locatedLength;

	fail=*foundMatch=false;
	nextSearchPosition=highPosition;
	PositionToChunkPositionPastEnd(searchInText,nextSearchPosition,&inChunk,&inOffset);			// locate place to start the search
	matching=true;
	while(!fail&&matching)
	{
		if(SearchBackward(inChunk,inOffset,theRecord,leftEdge,rightEdge,nextSearchPosition-lowPosition,&matching,&locatedPosition,&locatedLength,&inChunk,&inOffset,&endChunk,&endOffset))
		{
			if(matching)
			{
				if(locatedLength)										// finding an empty string is fatal to find all
				{
					rightEdge=false;									// no matter what, just left the right edge
					locatedPosition=nextSearchPosition-locatedPosition;	// make located position absolute
					nextSearchPosition=locatedPosition;
					if(!(*foundMatch))
					{
						(*firstMatchPosition)=locatedPosition;
						(*foundMatch)=true;
					}
					fail=!SetSelectionRange(theSelectionUniverse,locatedPosition,locatedLength);
				}
				else
				{
					SetError(localErrorFamily,errorMembers[MATCHEDNOTHING],errorDescriptions[MATCHEDNOTHING]);
					fail=true;
				}
			}
		}
		else
		{
			fail=true;
		}
	}
	return(!fail);
}

bool EditorFindAll(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,TEXTUNIVERSE *searchForText,bool backward,bool wrapAround,bool selectionExpr,bool ignoreCase,bool limitScope,bool *foundMatch,SELECTIONUNIVERSE *resultSelectionUniverse)
// find all occurrences of searchForText using the given rules, if there is an error,
// or the user aborts, SetError, and return false. If nothing is found, set found to false
// if found returns true, resultSelectionUniverse will be modified to contain the
// list of selections of found text.
// NOTE: it is ok to pass theSelectionUniverse and resultSelectionUniverse as the same universe.
{
	SEARCHRECORD
		*theRecord;
	UINT32
		startPosition,
		endPosition,
		selectionLength;
	UINT32
		firstMatchPosition,
		tempFirstMatchPosition;
	TEXTUNIVERSE
		*searchInText;
	bool
		hadMatch,
		fail;
	SELECTIONUNIVERSE
		*newSelectionUniverse;

	fail=*foundMatch=false;
	if((theRecord=CreateSearchRecord(searchForText,NULL,selectionExpr,ignoreCase,false)))
	{
		ShowBusy();
		if((newSelectionUniverse=OpenSelectionUniverse()))
		{
			searchInText=theBuffer->textUniverse;
			if(!backward)
			{
				if(!limitScope)
				{
					GetSelectionEndPositions(theSelectionUniverse,&startPosition,&endPosition);		// find ends of old selection, or cursor position
					if(FindAllForward(searchInText,endPosition==0,true,endPosition,searchInText->totalBytes,theRecord,newSelectionUniverse,foundMatch,&firstMatchPosition))
					{
						if(wrapAround)
						{
							fail=!FindAllForward(searchInText,true,startPosition==searchInText->totalBytes,0,startPosition,theRecord,newSelectionUniverse,&hadMatch,&tempFirstMatchPosition);
							if(!(*foundMatch))
							{
								*foundMatch=hadMatch;
								firstMatchPosition=tempFirstMatchPosition;
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
					startPosition=0;
					selectionLength=0;
					while(!fail&&GetSelectionAtOrAfterPosition(theSelectionUniverse,startPosition,&startPosition,&selectionLength))
					{
						if(FindAllForward(searchInText,true,true,startPosition,startPosition+selectionLength,theRecord,newSelectionUniverse,&hadMatch,&tempFirstMatchPosition))
						{
							if(!(*foundMatch))
							{
								*foundMatch=hadMatch;
								firstMatchPosition=tempFirstMatchPosition;
							}
						}
						else
						{
							fail=true;
						}
						startPosition+=selectionLength;
					}
				}
			}
			else
			{
				if(!limitScope)
				{
					GetSelectionEndPositions(theSelectionUniverse,&startPosition,&endPosition);		// find ends of old selection, or cursor position
					if(FindAllBackward(searchInText,true,startPosition==searchInText->totalBytes,0,startPosition,theRecord,newSelectionUniverse,foundMatch,&firstMatchPosition))
					{
						if(wrapAround)
						{
							fail=!FindAllBackward(searchInText,endPosition==0,true,endPosition,searchInText->totalBytes,theRecord,newSelectionUniverse,&hadMatch,&tempFirstMatchPosition);
							if(!(*foundMatch))
							{
								*foundMatch=hadMatch;
								firstMatchPosition=tempFirstMatchPosition;
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
					GetSelectionEndPositions(theSelectionUniverse,&startPosition,&endPosition);
					startPosition=endPosition;	// start at the end

					while(!fail&&startPosition&&GetSelectionAtOrBeforePosition(theSelectionUniverse,startPosition-1,&startPosition,&selectionLength))
					{
						if(FindAllBackward(searchInText,true,true,startPosition,startPosition+selectionLength,theRecord,newSelectionUniverse,&hadMatch,&tempFirstMatchPosition))
						{
							if(!(*foundMatch))
							{
								*foundMatch=hadMatch;
								firstMatchPosition=tempFirstMatchPosition;
							}
						}
						else
						{
							fail=true;
						}
					}
				}
			}
			if(*foundMatch)																				// got something, so change result universe
			{
				EditorStartSelectionChange(theBuffer);
				SetSelectionCursorPosition(newSelectionUniverse,firstMatchPosition);
				if(!CopySelectionUniverse(newSelectionUniverse,resultSelectionUniverse))
				{
					fail=true;
				}
				EditorEndSelectionChange(theBuffer);
			}
			CloseSelectionUniverse(newSelectionUniverse);												// close this universe
		}
		else
		{
			fail=true;
		}
		DisposeSearchRecord(theRecord);
		ShowNotBusy();
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool ReplaceRegisterReferences(TEXTUNIVERSE *replaceWithUniverse,SEARCHRECORD *searchedForText)
// run through replaceWithUniverse looking for register specifications, if any are seen,
// replace them with what searchForText found
// if there is a problem, SetError, and return false
// NOTE: registers are specified in replaceWithUniverse using \0 through \9
// ignore any other \ in replaceWithUniverse
{
	CHUNKHEADER
		*foundChunk,
		*currentChunk,
		*nextChunk;
	UINT32
		foundOffset,
		currentOffset,
		currentPosition,
		nextOffset;
	UINT32
		matchLength;
	UINT8
		theChar;
	bool
		fail;

	fail=false;
	currentChunk=replaceWithUniverse->firstChunkHeader;
	currentPosition=currentOffset=0;
	while(currentChunk&&!fail)
	{
		if(currentChunk->data[currentOffset]!='\\')
		{
			currentPosition++;
			currentOffset++;
			if(currentOffset>currentChunk->totalBytes)
			{
				currentChunk=currentChunk->nextHeader;
				currentOffset=0;
			}
		}
		else
		{
			nextChunk=currentChunk;
			nextOffset=currentOffset+1;
			if(nextOffset>nextChunk->totalBytes)
			{
				nextChunk=nextChunk->nextHeader;
				nextOffset=0;
			}
			if(nextChunk)
			{
				theChar=nextChunk->data[nextOffset];
				if(theChar>='0'&&theChar<='9')
				{
					theChar-='0';														// make into an index
					if(DeleteUniverseText(replaceWithUniverse,currentPosition,2))		// remove the register specification
					{
						if(GetRERegisterMatchChunkAndOffset(searchedForText->expression,theChar,&foundChunk,&foundOffset,&matchLength))	// see if anything to insert
						{
							fail=!InsertUniverseChunks(replaceWithUniverse,currentPosition,foundChunk,foundOffset,matchLength);
							currentPosition+=matchLength;								// move past stuff just inserted
						}
						PositionToChunkPosition(replaceWithUniverse,currentPosition,&currentChunk,&currentOffset);		// locate place to continue looking
					}
					else
					{
						fail=true;
					}
				}
				else
				{
					currentChunk=nextChunk;		// push past the quote character
					currentOffset=nextOffset;
					currentPosition++;
				}
			}
			else
			{
				currentChunk=nextChunk;			// \ as last character, just terminate scan
				currentOffset=nextOffset;
			}
		}
	}
	return(!fail);
}

static bool CreateFoundVariable(EDITORBUFFER *replaceInUniverse,UINT32 replaceOffset,UINT32 replaceLength,SEARCHRECORD *searchRecord)
// create the "found" variable, and $n variables which are passed to the Tcl procedure as
// globals.
// If there is a problem, SetError, and return NULL
// NOTE: this routine, and ReplaceSelectedText are the ONLY routines in this file that
// rely on Tcl...
// if there is a problem, SetError, and return false
{
	CHUNKHEADER
		*theChunk,
		*foundChunk;
	UINT32
		theOffset,
		foundOffset;
	UINT32
		matchLength;
	char
		*theLocatedText;
	int
		varCount;
	char
		varName[32];
	bool
		fail;

	fail=false;
	if((theLocatedText=(char *)MNewPtr(replaceLength+1)))	// make pointer to text that was located (this pointer will used to hold all the extracted text)
	{
		PositionToChunkPosition(replaceInUniverse->textUniverse,replaceOffset,&theChunk,&theOffset);		// locate place where match was found
		if(ExtractUniverseText(replaceInUniverse->textUniverse,theChunk,theOffset,(UINT8 *)theLocatedText,replaceLength,&theChunk,&theOffset))
		{
			theLocatedText[replaceLength]='\0';			// terminate the string
			if(!Tcl_SetVar(theTclInterpreter,"found",theLocatedText,TCL_LEAVE_ERR_MSG)!=TCL_OK)
			{
				SetError(localErrorFamily,errorMembers[TCLERROR],(char *)Tcl_GetStringResult(theTclInterpreter));
				fail=true;
			}
		}
		else
		{
			fail=true;
		}
		MDisposePtr(theLocatedText);
	}
	else
	{
		fail=true;
	}

	if(!fail&&searchRecord->expression)			// if it is an expression, make a list of 10 more variables (one for each tagged subexpression)
	{
		for(varCount=0;!fail&&varCount<MAXREGISTERS;varCount++)
		{
			sprintf(varName,"%d",varCount);		// create variable name

			if(GetRERegisterMatchChunkAndOffset(searchRecord->expression,varCount,&foundChunk,&foundOffset,&matchLength))	// see if anything to add
			{
				if((theLocatedText=(char *)MNewPtr(replaceLength+1)))		// make pointer to the text
				{
					if(ExtractUniverseText(replaceInUniverse->textUniverse,foundChunk,foundOffset,(UINT8 *)theLocatedText,matchLength,&theChunk,&theOffset))
					{
						theLocatedText[matchLength]='\0';					// terminate the string
						if(!Tcl_SetVar(theTclInterpreter,varName,theLocatedText,TCL_LEAVE_ERR_MSG)!=TCL_OK)
						{
							SetError(localErrorFamily,errorMembers[TCLERROR],(char *)Tcl_GetStringResult(theTclInterpreter));
							fail=true;
						}
					}
					else
					{
						fail=true;
					}
					MDisposePtr(theLocatedText);
				}
				else
				{
					fail=true;
				}
			}
			else
			{
				if(!Tcl_SetVar(theTclInterpreter,varName,"",TCL_LEAVE_ERR_MSG)!=TCL_OK)
				{
					SetError(localErrorFamily,errorMembers[TCLERROR],(char *)Tcl_GetStringResult(theTclInterpreter));
					fail=true;
				}
			}
		}
	}
	return(!fail);
}

static bool ReplaceSearchedText(EDITORBUFFER *replaceInUniverse,UINT32 replaceOffset,UINT32 replaceLength,CHUNKHEADER **replaceAtChunk,UINT32 *replaceAtChunkOffset,SEARCHRECORD *searchRecord,UINT32 *bytesReplaced)
// given text that was just searched for by searchForText, replace it
// based on the search type
// if there is a problem, SetError, and return false
{
	bool
		fail;
	UINT32
		theLength;
	TEXTUNIVERSE
		*alternateUniverse;
	char
		*stringResult;

	fail=false;
	if(searchRecord->procedure)								// handle replacement by calling Tcl procedure
	{
		if(CreateFoundVariable(replaceInUniverse,replaceOffset,replaceLength,searchRecord))
		{
			if(Tcl_Eval(theTclInterpreter,searchRecord->procedure)==TCL_OK)
			{
				stringResult=(char *)Tcl_GetStringResult(theTclInterpreter);
				theLength=strlen(stringResult);
				fail=!ReplaceEditorText(replaceInUniverse,replaceOffset,replaceOffset+replaceLength,(UINT8 *)stringResult,theLength);
				(*bytesReplaced)=theLength;
			}
			else
			{
				SetError(localErrorFamily,errorMembers[TCLERROR],(char *)Tcl_GetStringResult(theTclInterpreter));
				fail=true;
			}
			Tcl_ResetResult(theTclInterpreter);				// do not leave any Tcl result around (other stuff may append to it)
		}
		else
		{
			fail=true;
		}
	}
	else
	{
		if(searchRecord->expression)
		{
			if((alternateUniverse=OpenTextUniverse()))		// create alternate universe where we make the real text to replace
			{
				if(InsertUniverseChunks(alternateUniverse,alternateUniverse->totalBytes,searchRecord->replaceWithText->firstChunkHeader,0,searchRecord->replaceWithText->totalBytes))	// copy replace with text into alternate universe
				{
					if(ReplaceRegisterReferences(alternateUniverse,searchRecord))
					{
						fail=!ReplaceEditorChunks(replaceInUniverse,replaceOffset,replaceOffset+replaceLength,alternateUniverse->firstChunkHeader,0,alternateUniverse->totalBytes);
						(*bytesReplaced)=alternateUniverse->totalBytes;
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
				CloseTextUniverse(alternateUniverse);
			}
			else
			{
				fail=true;
			}
		}
		else
		{
			fail=!ReplaceEditorChunks(replaceInUniverse,replaceOffset,replaceOffset+replaceLength,searchRecord->replaceWithText->firstChunkHeader,0,searchRecord->replaceWithText->totalBytes);
			(*bytesReplaced)=searchRecord->replaceWithText->totalBytes;
		}
	}
	return(!fail);
}

bool EditorReplace(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,TEXTUNIVERSE *searchForText,TEXTUNIVERSE *replaceWithText,bool backward,bool wrapAround,bool selectionExpr,bool ignoreCase,bool replaceProc,bool *foundMatch,SELECTIONUNIVERSE *resultSelectionUniverse)
// find searchForText in theBuffer, starting at the cursor position, or the _start_ of
// the current selection of theSelectionUniverse
// if searchForText is not located, return false in foundMatch
// if it is located, replace what was found with replaceWithText, modify resultSelectionUniverse to
// be the selection of what was replaced, and return true in foundMatch
// if there is a problem, SetError, and return false
{
	CHUNKHEADER
		*inChunk,
		*startChunk,
		*endChunk;
	UINT32
		inOffset,
		startOffset,
		endOffset;
	SEARCHRECORD
		*theRecord;
	UINT32
		startPosition,
		endPosition,
		bytesReplaced;
	UINT32
		locatedPosition,
		locatedLength;
	TEXTUNIVERSE
		*searchInText;
	bool
		fail;

	fail=*foundMatch=false;
	if((theRecord=CreateSearchRecord(searchForText,replaceWithText,selectionExpr,ignoreCase,replaceProc)))
	{
		ShowBusy();
		searchInText=theBuffer->textUniverse;

		GetSelectionEndPositions(theSelectionUniverse,&startPosition,&endPosition);		// find ends of old selection, or cursor position
		if(!backward)
		{
			PositionToChunkPosition(searchInText,startPosition,&inChunk,&inOffset);		// locate place to start the search
			if(SearchForward(inChunk,inOffset,theRecord,startPosition==0,true,searchInText->totalBytes-startPosition,foundMatch,&locatedPosition,&locatedLength,&startChunk,&startOffset,&endChunk,&endOffset))
			{
				locatedPosition+=startPosition;											// make located position absolute
				if(!(*foundMatch)&&wrapAround)
				{
					if(!SearchForward(searchInText->firstChunkHeader,0,theRecord,true,startPosition==searchInText->totalBytes,startPosition,foundMatch,&locatedPosition,&locatedLength,&startChunk,&startOffset,&endChunk,&endOffset))
					{
						fail=true;
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
			PositionToChunkPositionPastEnd(searchInText,endPosition,&inChunk,&inOffset);	// locate place to start the search
			if(SearchBackward(inChunk,inOffset,theRecord,true,endPosition==searchInText->totalBytes,endPosition,foundMatch,&locatedPosition,&locatedLength,&startChunk,&startOffset,&endChunk,&endOffset))
			{
				locatedPosition=endPosition-locatedPosition;					// make located position absolute
				if(!(*foundMatch)&&wrapAround)
				{
					PositionToChunkPositionPastEnd(searchInText,searchInText->totalBytes,&inChunk,&inOffset);	// locate place to start the search
					if(SearchBackward(inChunk,inOffset,theRecord,endPosition==0,true,searchInText->totalBytes-endPosition,foundMatch,&locatedPosition,&locatedLength,&startChunk,&startOffset,&endChunk,&endOffset))
					{
						locatedPosition=searchInText->totalBytes-locatedPosition;
					}
					else
					{
						fail=true;
					}
				}
			}
			else
			{
				fail=true;
			}
		}
		if(!fail&&(*foundMatch))
		{
			BeginUndoGroup(theBuffer);
			EditorStartReplace(theBuffer);
			fail=!ReplaceSearchedText(theBuffer,locatedPosition,locatedLength,&startChunk,&startOffset,theRecord,&bytesReplaced);
			EditorEndReplace(theBuffer);
			StrictEndUndoGroup(theBuffer);
			EditorSetNormalSelection(theBuffer,resultSelectionUniverse,locatedPosition,locatedPosition+bytesReplaced);
			SetSelectionCursorPosition(resultSelectionUniverse,locatedPosition);
		}
		DisposeSearchRecord(theRecord);
		ShowNotBusy();
	}
	else
	{
		fail=true;
	}
	return(!fail);
}

static bool ReplaceAllForward(EDITORBUFFER *theBuffer,bool leftEdge,bool rightEdge,UINT32 lowPosition,UINT32 *highPosition,SEARCHRECORD *theRecord,SELECTIONUNIVERSE *theSelectionUniverse,bool *foundMatch,UINT32 *firstMatchPosition)
// Replace all occurrences of theRecord between lowPosition, and highPosition, adding the selections
// to theSelectionUniverse before selectionChunk, selectionOffset
// if there is a problem, SetError and return false
// if a matches were found, return true in foundMatch, and the position of the first match found
// in firstMatchPosition
// highPosition is modified to reflect the new highPosition after all replacements have been made
// NOTE: even if false is returned, foundMatch, highPosition, and theSelectionUniverse are
// valid
{
	bool
		fail;
	CHUNKHEADER
		*inChunk,
		*startChunk;
	UINT32
		inOffset,
		startOffset;
	bool
		matching;
	TEXTUNIVERSE
		*searchInText;
	UINT32
		nextSearchPosition,
		locatedPosition,
		locatedLength,
		bytesReplaced;
	UINT32
		newHighPosition;

	fail=*foundMatch=false;
	searchInText=theBuffer->textUniverse;
	nextSearchPosition=lowPosition;
	matching=true;
	newHighPosition=*highPosition;
	while(!fail&&matching)
	{
		PositionToChunkPosition(searchInText,nextSearchPosition,&inChunk,&inOffset);	// locate place to start the search

		if(nextSearchPosition!=lowPosition)
		{
			leftEdge=false;													// no longer at left edge
		}

		if(SearchForward(inChunk,inOffset,theRecord,leftEdge,rightEdge,newHighPosition-nextSearchPosition,&matching,&locatedPosition,&locatedLength,&startChunk,&startOffset,&inChunk,&inOffset))
		{
			if(matching)													// see if a match was found, if not, we are done
			{
				if(locatedLength)											// finding an empty string is fatal to replace all
				{
					locatedPosition+=nextSearchPosition;					// make located position absolute
					if(!(*foundMatch))
					{
						(*firstMatchPosition)=locatedPosition;				// remember the first match position so we can home to it when done
						(*foundMatch)=true;
					}
					if(ReplaceSearchedText(theBuffer,locatedPosition,locatedLength,&startChunk,&startOffset,theRecord,&bytesReplaced))
					{
						inChunk=startChunk;
						inOffset=startOffset;
						nextSearchPosition=locatedPosition+bytesReplaced;	// move to the new position after the replacement
						newHighPosition-=locatedLength;						// subtract off bytes that were replaced
						newHighPosition+=bytesReplaced;						// add back number that replaced them

						DeleteSelectionRange(theSelectionUniverse,locatedPosition,locatedLength);
						if(InsertSelectionRange(theSelectionUniverse,locatedPosition,bytesReplaced))
						{
							fail=!SetSelectionRange(theSelectionUniverse,locatedPosition,bytesReplaced);
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
				else
				{
					SetError(localErrorFamily,errorMembers[MATCHEDNOTHING],errorDescriptions[MATCHEDNOTHING]);
					fail=true;
				}
			}
		}
		else
		{
			fail=true;
		}
	}
	*highPosition=newHighPosition;
	return(!fail);
}

static bool ReplaceAllBackward(EDITORBUFFER *theBuffer,bool leftEdge,bool rightEdge,UINT32 lowPosition,UINT32 *highPosition,SEARCHRECORD *theRecord,SELECTIONUNIVERSE *theSelectionUniverse,bool *foundMatch,UINT32 *firstMatchPosition)
// Replace all occurrences of theRecord between lowPosition, and highPosition, adding the selections
// to theSelectionUniverse
// if there is a problem, SetError and return false
// if a matches were found, return true in foundMatch, and the position of the first match found
// in firstMatchPosition
// highPosition is modified to reflect the new highPosition after all replacements have been made
// NOTE: even if false is returned, foundMatch, highPosition, and theSelectionUniverse are
// valid
{
	bool
		fail;
	CHUNKHEADER
		*inChunk,
		*endChunk;
	UINT32
		inOffset,
		endOffset;
	bool
		matching;
	TEXTUNIVERSE
		*searchInText;
	UINT32
		nextSearchPosition,
		locatedPosition,
		locatedLength,
		bytesReplaced;
	UINT32
		newHighPosition;

	fail=*foundMatch=false;
	searchInText=theBuffer->textUniverse;
	nextSearchPosition=*highPosition;
	matching=true;
	newHighPosition=*highPosition;
	while(!fail&&matching)
	{
		PositionToChunkPositionPastEnd(searchInText,nextSearchPosition,&inChunk,&inOffset);		// locate place to start the search

		if(nextSearchPosition!=newHighPosition)
		{
			rightEdge=false;												// no longer at the right edge
		}

		if(SearchBackward(inChunk,inOffset,theRecord,leftEdge,rightEdge,nextSearchPosition-lowPosition,&matching,&locatedPosition,&locatedLength,&inChunk,&inOffset,&endChunk,&endOffset))
		{
			if(matching)													// see if a match was found, if not, we are done
			{
				if(locatedLength)											// finding an empty string is fatal to replace all
				{
					locatedPosition=nextSearchPosition-locatedPosition;		// make located position absolute
					nextSearchPosition=locatedPosition;
					if(!(*foundMatch))										// if no matches found so far ...
					{
						(*firstMatchPosition)=locatedPosition;				// remember end of selections (sort of, will be adjusted at the end)
						(*foundMatch)=true;
					}
					if(ReplaceSearchedText(theBuffer,locatedPosition,locatedLength,&inChunk,&inOffset,theRecord,&bytesReplaced))
					{
						newHighPosition-=locatedLength;						// subtract off bytes that were replaced
						newHighPosition+=bytesReplaced;						// add back number that replaced them

						DeleteSelectionRange(theSelectionUniverse,locatedPosition,locatedLength);
						if(InsertSelectionRange(theSelectionUniverse,locatedPosition,bytesReplaced))
						{
							fail=!SetSelectionRange(theSelectionUniverse,locatedPosition,bytesReplaced);
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
				else
				{
					SetError(localErrorFamily,errorMembers[MATCHEDNOTHING],errorDescriptions[MATCHEDNOTHING]);
					fail=true;
				}
			}
		}
		else
		{
			fail=true;
		}
	}
	*firstMatchPosition+=newHighPosition-(*highPosition);
	*highPosition=newHighPosition;
	return(!fail);
}

bool EditorReplaceAll(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,TEXTUNIVERSE *searchForText,TEXTUNIVERSE *replaceWithText,bool backward,bool wrapAround,bool selectionExpr,bool ignoreCase,bool limitScope,bool replaceProc,bool *foundMatch,SELECTIONUNIVERSE *resultSelectionUniverse)
// replace all occurrences of searchForText with replaceWithText using the given rules, if none are found,
// return false in foundMatch
// if foundMatch is true, modify resultSelectionUniverse to be the selection of all replaced text
// if there is a problem, SetError, return false
{
	SEARCHRECORD
		*theRecord;
	INT32
		replaceOffset;
	UINT32
		startPosition,
		endPosition,
		selectionLength;
	UINT32
		firstMatchPosition,
		tempFirstMatchPosition,
		highPosition;
	TEXTUNIVERSE
		*searchInText;
	bool
		hadMatch,
		fail;
	SELECTIONUNIVERSE
		*oldSelectionUniverse,
		*newSelectionUniverse;

	fail=*foundMatch=false;
	if((theRecord=CreateSearchRecord(searchForText,replaceWithText,selectionExpr,ignoreCase,replaceProc)))
	{
		ShowBusy();
		if((newSelectionUniverse=OpenSelectionUniverse()))
		{
			if((oldSelectionUniverse=CreateSelectionUniverseCopy(theSelectionUniverse)))				// copy this, since it must not change during the replacement
			{
				searchInText=theBuffer->textUniverse;
				firstMatchPosition=0;
				BeginUndoGroup(theBuffer);
				EditorStartReplace(theBuffer);
				if(!backward)
				{
					if(!limitScope)
					{
						GetSelectionEndPositions(oldSelectionUniverse,&startPosition,&endPosition);		// find ends of old selection, or cursor position
						highPosition=searchInText->totalBytes;
						if(ReplaceAllForward(theBuffer,startPosition==0,true,startPosition,&highPosition,theRecord,newSelectionUniverse,foundMatch,&firstMatchPosition))
						{
							if(wrapAround)
							{
								highPosition=startPosition;
								fail=!ReplaceAllForward(theBuffer,true,highPosition==searchInText->totalBytes,0,&highPosition,theRecord,newSelectionUniverse,&hadMatch,&tempFirstMatchPosition);
								if(!(*foundMatch))
								{
									*foundMatch=hadMatch;
									firstMatchPosition=tempFirstMatchPosition;
								}
								else
								{
									firstMatchPosition+=highPosition-startPosition;	// move home position due to text changes above
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
						replaceOffset=0;
						startPosition=0;
						selectionLength=0;
						while(!fail&&GetSelectionAtOrAfterPosition(oldSelectionUniverse,startPosition,&startPosition,&selectionLength))
						{
							endPosition=startPosition+replaceOffset+selectionLength;
							if(ReplaceAllForward(theBuffer,true,true,startPosition+replaceOffset,&endPosition,theRecord,newSelectionUniverse,&hadMatch,&tempFirstMatchPosition))
							{
								replaceOffset+=endPosition-(startPosition+replaceOffset+selectionLength);
								if(!(*foundMatch))
								{
									*foundMatch=hadMatch;
									firstMatchPosition=tempFirstMatchPosition;
								}
							}
							else
							{
								fail=true;
							}
							startPosition+=selectionLength;
						}
					}
				}
				else
				{
					if(!limitScope)
					{
						GetSelectionEndPositions(oldSelectionUniverse,&startPosition,&endPosition);		// find ends of old selection, or cursor position
						highPosition=endPosition;
						if(ReplaceAllBackward(theBuffer,true,highPosition==searchInText->totalBytes,0,&highPosition,theRecord,newSelectionUniverse,foundMatch,&firstMatchPosition))
						{
							if(wrapAround)
							{
								endPosition=highPosition;
								highPosition=searchInText->totalBytes;
								fail=!ReplaceAllBackward(theBuffer,endPosition==0,true,endPosition,&highPosition,theRecord,newSelectionUniverse,&hadMatch,&tempFirstMatchPosition);
								if(!(*foundMatch))
								{
									*foundMatch=hadMatch;
									firstMatchPosition=tempFirstMatchPosition;
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
						GetSelectionEndPositions(theSelectionUniverse,&startPosition,&endPosition);
						startPosition=endPosition;	// start at the end

						while(!fail&&startPosition&&GetSelectionAtOrBeforePosition(theSelectionUniverse,startPosition-1,&startPosition,&selectionLength))
						{
							endPosition=startPosition+selectionLength;
							highPosition=endPosition;
							if(ReplaceAllBackward(theBuffer,true,true,startPosition,&highPosition,theRecord,newSelectionUniverse,&hadMatch,&tempFirstMatchPosition))
							{
								if(!(*foundMatch))
								{
									*foundMatch=hadMatch;
									firstMatchPosition=tempFirstMatchPosition;
								}
								else
								{
									firstMatchPosition+=highPosition-endPosition;		// adjust for any replacements just made
								}
							}
							else
							{
								fail=true;
							}
						}
					}
				}
				EditorEndReplace(theBuffer);
				StrictEndUndoGroup(theBuffer);
				if(*foundMatch)																	// got something, so change selection universe
				{
					EditorStartSelectionChange(theBuffer);
					SetSelectionCursorPosition(newSelectionUniverse,firstMatchPosition);		// put the cursor at the start
					if(!CopySelectionUniverse(newSelectionUniverse,resultSelectionUniverse))
					{
						fail=true;
					}
					EditorEndSelectionChange(theBuffer);
				}
				CloseSelectionUniverse(oldSelectionUniverse);									// close this universe
			}
			else
			{
				fail=true;
			}
			CloseSelectionUniverse(newSelectionUniverse);										// close this universe
		}
		else
		{
			fail=true;
		}
		DisposeSearchRecord(theRecord);
		ShowNotBusy();
	}
	else
	{
		fail=true;
	}
	return(!fail);
}
