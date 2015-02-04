// Syntax highlighting functions
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

// Overview of how e93 performs syntax highlighting:
//
// Syntax highlighting is the process of looking at the text, searching
// for patterns, and highlighting those patterns by changing color,
// font, or style. Most editors which perform syntax highlighting have
// built-in rules for what to search for. By having built in rules,
// an editor can search out the syntax efficiently, and
// reduce the complexity of the process, but it limits the user from
// modifying rules or adding new ones. A problem occurs when one tries to
// generalize the rule definition process so that users can define their
// own -- things slow down.
//
// e93 has always been meant as a generalized text editor, not a 'C' editor.
// Because of this, syntax highlighting must be generalized so that
// users can create their own rules for whatever they might be editing.
//
// Syntax rules are given in the form of 1 or more regular expressions.
// The first regular expression is called the "start" expression, and the
// second (possibly empty) set of expressions are called the "end"
// expressions.

// If only a start expression is defined, e93 matches it, and applies
// a defined style to the text wherever it matches.
// If both start and end expressions are defined, e93 matches the
// start, then matches the nearest end. The start match has a style associated
// with it, the area between the start and end has a style associated
// with it, and the end expression has a style associated with it.
// So far, so good...
//
// Of course this method of highlighting has some limitations.
// For instance, one cannot define a set of keywords which are
// highlighted only inside parenthesis. To overcome this limitation,
// e93 allows for rules to be defined which are only active during
// the match of other rules (nested rules).
// So, you could define a rule with a start expression of
// open parenthesis, an end expression of close parenthesis. Then
// you could define a rule to match certain keywords, and tell e93
// that this rule only applies between the start and end parenthesis.
// This nesting of rules is allowed to occur to an arbitrary depth.

// Now, the only problem is getting e93 to do all of this without
// bringing the editor to a complete crawl on a 600MHz machine.
// Here's how that works:
// For every expression that is defined for a given syntax
// highlighting ruleset, e93 matches ALL occurrences of that expression
// and keeps them cached.
// When the text is changed, e93 uses information it has about the
// expression to limit how much searching it needs to perform to update
// the caches.
// The syntax highlighting rules use these expression-match caches
// to work out which rule is in effect, and when. These rules also
// cache their state, and attempt to update as little as possible with
// each change.

// Because of all the caching, and complexity caused by this solution,
// the number of data structures is getting out of hand.
// This might help clear things up:

// A SYNTAXMAP is a data structure that contains all the information needed
// to look at the syntax of a document, and convert it to style information
// which will be applied to the document. It holds a set of compiled regular
// expressions, and a mapping which tells how these expressions are converted
// into style information.

// A SYNTAXEXPRESSION data structure holds a compiled regular expression
// which is going to be matched against the text of the document.

// A SYNTAXSTYLEMAPPING data structure describes the relationship between
// a set of matched regular expressions, and which styles should
// be applied to the text when they match. Each SYNTAXSTYLEMAPPING points
// to a regular expression which needs to be matched at the start,
// and an optional regular expression which needs to be matched at the
// end. It then describes which styles should be applied to the text
// for the starting match, the text between the matches, and the text
// of the ending match.
// SYNTAXSTYLEMAPPINGs also descibe which nested SYNTAXSTYLEMAPPINGs should
// be active between matches. This allows for recursive mappings (eg. inside
// a set of parenthesis, one could search for a set of keywords).

// A SYNTAXSTYLEMAPPINGLIST structure is nothing more than a container for a list
// of SYNTAXSTYLEMAPPINGs

// This is really not all that confusing, but the terminology could use
// some work.

#include	"includes.h"

typedef struct expressionPiece
{
	struct syntaxExpression
		*parent;					// pointer to the expression that this is a piece of
	UINT32
		pieceIndex;					// index of this piece within the entire list of pieces used in the mapping (used to index array of EXPRESSIONPIECEINSTANCEs defined below)
	bool
		useRegisterMatch;			// tells if entire expression matches, or a register matches
	UINT32
		registerIndex;				// if useRegisterMatch is true, this gives the register index of the match
	struct expressionPiece
		*prev,
		*next;						// used to link the list of pieces together
} EXPRESSIONPIECE;

typedef struct expressionPieceList	// used to create arbitrary lists of expression pieces
{
	EXPRESSIONPIECE
		*thePiece;					// pointer to the piece given by this list entry
	struct expressionPieceList
		*prev,
		*next;						// used to link the list of pieces together
} EXPRESSIONPIECELIST;

typedef struct syntaxExpression		// keeps named expression
{
	struct syntaxMap
		*parent;					// pointer to the map this is part of
	COMPILEDEXPRESSION
		*compiledExpression;		// compiled regular expression
	struct syntaxExpression
		*prev,
		*next;						// pointer to next expression
	EXPRESSIONPIECE
		*firstPiece;				// pointer to list of pieces of this expression which are being mapped
	char
		expressionName[1];			// variable length array, name of this expression
} SYNTAXEXPRESSION;

typedef struct syntaxStyleMapping	// describes relationship between expressions and styles
{
	struct syntaxMap
		*parent;					// pointer to the map this is part of
	EXPRESSIONPIECE
		*startExpressionPiece;		// expression piece which starts this style
	EXPRESSIONPIECELIST
		*endExpressionPieces;		// expression pieces which may end this style (NULL if style is begun and ended with startExpressionPiece)
	UINT32
		startStyle,					// style index used when matching startExpressionPiece
		betweenStyle,				// style index to use between start and end expressions (used only if endExpressionPieces are defined)
		endStyle;					// style index used when matching one of the endExpressionPieces (used only if endExpressionPieces are defined)
	struct syntaxStyleMappingList
		*betweenList;				// list of style maps which are active between the match of startExpressionPiece and the match of endExpressionPieces (used only if endExpressionPieces are defined)

	struct syntaxStyleMapping		// links together all the syntax style mappings for the given syntax map
		*prev,
		*next;
	char
		mappingName[1];				// variable length array, name of this mapping
} SYNTAXSTYLEMAPPING;

typedef struct syntaxStyleMappingList	// these are used to link together lists of pointers to SYNTAXSTYLEMAPs which are active at any given time
{
	SYNTAXSTYLEMAPPING
		*mappingEntry;				// pointer to map entry for this list element
	struct syntaxStyleMappingList
		*prev,
		*next;
} SYNTAXSTYLEMAPPINGLIST;

typedef struct syntaxMap
{
	UINT32
		numExpressionPieces;		// number of pieces contained in the map
	SYNTAXEXPRESSION
		*firstExpression;			// pointer to head of list of expressions for this map
	SYNTAXSTYLEMAPPING
		*firstStyleMapping;			// pointer to the entire list of syntax style maps for this map
	struct syntaxInstance
		*firstInstance;				// first instance of this map
	SYNTAXSTYLEMAPPINGLIST
		*rootMapList;				// list of syntax style maps which are active at the root
	struct syntaxMap
		*prev,						// pointer to the previous syntax map in the global list
		*next;						// pointer to the next syntax map in the global list
	char
		mapName[1];					// variable length array, name of this map
} SYNTAXMAP;


// Each editor universe which performs syntax highlighting, is assigned a
// SYNTAXINSTANCE. The SYNTAXINSTANCE is used to maintain syntax
// highlighting cache information which is specific to the editor
// universe.
// This information includes the cached array of which expressions
// matched where in the text, and the cached state of the syntax coloring
// state machine. This is kept separate from the SYNTAXMAP, since that
// definition is used across all editor universes which require it.

typedef struct styleElement
{
	SPARSEARRAYHEADER
		theSAHeader;
	UINT32
		offset,						// offset to start of piece within match
		length;						// number of characters that were matched by the piece
} EXPRESSIONMATCHELEMENT;

typedef struct expressionPieceInstance	// used to keep track of matches to an expression for a given editor universe
{
	EXPRESSIONPIECE
		*theExpressionPiece;
	UINT32
		maximumStartDistance;		// tells maximum distance from start we have ever witnessed, use to tell how far back searches for this expression must begin
	SPARSEARRAY
		matchArray;
} EXPRESSIONPIECEINSTANCE;

// For the outermost level of syntax highlighting rules, there is a table which is maintained
// by the syntax highlighting state machine that tells which mapping is in effect
// for which section of text.
// This table is used to keep from having to search all the expression matches, and
// rebuild the entire style table with each change to the text.
// When a change to the text occurs, this table is consulted to determine the
// range of style changes which will be required to get back in sync with what
// existed before.
typedef struct mappingElement
{
	SPARSEARRAYHEADER
		theSAHeader;
	SYNTAXSTYLEMAPPING
		*theMapping;				// pointer to the mapping which matches at the root level
} MAPPINGELEMENT;

typedef struct syntaxInstance
{
	SYNTAXMAP
		*parentMap;					// map this instance is based on
	EDITORBUFFER
		*theBuffer;					// pointer to the buffer that this instance belongs to
	struct syntaxInstance
		*prev,						// doubly linked list of all instances on the parent map (so parent can traverse all instances linked to it)
		*next;
	SPARSEARRAY
		mappingArray;				// used to keep track of mappings at the root level
	EXPRESSIONPIECEINSTANCE
		theExpressionPieces[1];		// variable length array of expression piece instance structures (one for each piece of an expression in the syntax map)
} SYNTAXINSTANCE;


static SYNTAXMAP
	*syntaxMapHead,					// head of global list of syntax maps
	*syntaxMapTail;

static char localErrorFamily[]="syntax";

enum
{
	MAPEXISTS,
	EXPRESSIONEXISTS,
	STYLEMAPPINGEXISTS,
	LISTMAPPINGEXISTS,
	EXPRESSIONPIECEEXISTS,
};

static char *errorMembers[]=
{
	"MapExists",
	"ExpressionExists",
	"StyleMappingExists",
	"ListMappingExists",
	"ExpressionPieceExists",
};

static char *errorDescriptions[]=
{
	"Syntax map already exists",
	"Expression already exists",
	"Style mapping already exists",
	"List element already exists",
	"Expression piece already exists",
};


static bool DeleteOverlappingMatches(EXPRESSIONPIECEINSTANCE *thePieceInstance,UINT32 startOffset,UINT32 endOffset,UINT32 numBytes,UINT32 numBack)
// The text has changed between startOffset and endOffset for numBytes
// Call this to delete any matches that overlapped this area,
// and to adjust all those beyond this area
// numBack tells how far back we need to go when re-evaluating expressions
// this routine deletes all previous matches which begin that far back off the startOffset
{
	DeleteSARange(&thePieceInstance->matchArray,startOffset-numBack,endOffset-startOffset+numBack);	// this cannot fail
	return(InsertSARange(&thePieceInstance->matchArray,startOffset-numBack,numBytes+numBack));
}

static bool UpdateExpressionCaches(CHUNKHEADER *theChunk,UINT32 theOffset,SYNTAXINSTANCE *theInstance,SYNTAXEXPRESSION *theExpression,UINT32 startOffset,UINT32 endOffset,UINT32 numBytes,UINT32 totalBytes,UINT32 *earliestExpressonChange)
// text in theTextUniverse has changed between startOffset and endOffset for numBytes
// run through and search the text to update the cached set of matches
// NOTE: theChunk could be NULL if there is no text in the universe
{
	CHUNKHEADER
		*newChunk,
		*outChunk;
	UINT32
		newOffset,
		outOffset;
	UINT32
		numBack;
	bool
		fail;
	bool
		found;
	UINT32
		count;
	UINT32
		i;
	UINT32
		matchOffset;
	EXPRESSIONMATCHELEMENT
		newElement;
	EXPRESSIONPIECE
		*thePiece;

	fail=false;
	if(theExpression->firstPiece)	// do not bother to do anything here if there are no pieces
	{
		REHuntBackToFarthestPossibleStart(theChunk,theOffset,theExpression->compiledExpression,&numBack,&newChunk,&newOffset);

		if(startOffset-numBack<*earliestExpressonChange)
		{
			*earliestExpressonChange=(startOffset-numBack);
		}

		thePiece=theExpression->firstPiece;
		while(thePiece&&!fail)
		{
			fail=!DeleteOverlappingMatches(&theInstance->theExpressionPieces[thePiece->pieceIndex],startOffset,endOffset,numBytes,numBack);	// adjust pieces for the change
			thePiece=thePiece->next;
		}

		i=startOffset-numBack;
		found=true;
		while(!fail&&found&&(i<=startOffset+numBytes))
		{
			if(SearchForwardRE(newChunk,newOffset,theExpression->compiledExpression,i==0,true,startOffset+numBytes-i,totalBytes-i,false,&found,&matchOffset,&count,&newChunk,&newOffset,&outChunk,&outOffset,false))
			{
				if(found)
				{
					thePiece=theExpression->firstPiece;				// update all the pieces
					while(thePiece&&!fail)
					{
						if(!thePiece->useRegisterMatch)
						{
							newElement.offset=0;
							newElement.length=count;
							fail=!SetSARange(&theInstance->theExpressionPieces[thePiece->pieceIndex].matchArray,i+matchOffset,1,&newElement);	// add this in
						}
						else
						{
							if(GetRERegisterMatch(theExpression->compiledExpression,thePiece->registerIndex,&newElement.offset,&newElement.length))
							{
								fail=!SetSARange(&theInstance->theExpressionPieces[thePiece->pieceIndex].matchArray,i+matchOffset,1,&newElement);	// add this in

								if(newElement.offset>theInstance->theExpressionPieces[thePiece->pieceIndex].maximumStartDistance)
								{
//									fprintf(stderr,"%s: %d\n",theInstance->theExpressionPieces[thePiece->pieceIndex].theExpressionPiece->parent->expressionName,newElement.offset);
									theInstance->theExpressionPieces[thePiece->pieceIndex].maximumStartDistance=newElement.offset;	// keep track of largest offset here
								}
							}
						}
						thePiece=thePiece->next;
					}

					i+=matchOffset+1;								// skip to spot just after this one
					if(i<=startOffset+numBytes)						// see if we should adjust the chunk
					{
						if((++newOffset)>=newChunk->totalBytes)		// move forward in the input 1 step
						{
							newChunk=newChunk->nextHeader;
							newOffset=0;
						}
					}
				}
			}
			else
			{
				fail=true;
			}
		}
	}
	return(!fail);
}

static bool AdjustMappingTableForChange(SYNTAXINSTANCE *theInstance,UINT32 startOffset,UINT32 endOffset,UINT32 numBytes)
// run through the mapping table, and reduce or expand as needed to account
// for the text change
{
	DeleteSARange(&theInstance->mappingArray,startOffset,endOffset-startOffset);	// this cannot fail
	return(InsertSARange(&theInstance->mappingArray,startOffset,numBytes));
}

static bool LocatePieceWithOffset(EXPRESSIONPIECEINSTANCE *theInstance,UINT32 position,UINT32 *startPosition,UINT32 *length)
// Locate the nearest expression match which is >= position for the given instance
// If there is none, return false
{
	UINT32
		currentPosition;
	UINT32
		numElements;
	EXPRESSIONMATCHELEMENT
		*theMatchElement;
	bool
		done,
		found;

	found=false;
	if(theInstance->maximumStartDistance)			// see if this piece uses start offsets at all (if not, the search can go faster)
	{
		if(position>theInstance->maximumStartDistance)
		{
			currentPosition=position-theInstance->maximumStartDistance;
		}
		else
		{
			currentPosition=0;
		}
		done=false;
		while((currentPosition<=position)&&!done)
		{
			if(GetSARecordAtOrAfterPosition(&theInstance->matchArray,currentPosition,startPosition,&numElements,(void **)(&theMatchElement)))
			{
				*startPosition+=theMatchElement->offset;
				if(*startPosition>=position)
				{
					*length=theMatchElement->length;
					done=found=true;
				}
				else
				{
					currentPosition++;
				}
			}
			else
			{
				done=true;							// no more matches
			}
		}
	}
	else											// search the fast way
	{
		if(GetSARecordAtOrAfterPosition(&theInstance->matchArray,position,startPosition,&numElements,(void **)(&theMatchElement)))
		{
			*length=theMatchElement->length;
			found=true;
		}
	}
	return(found);
}

static SYNTAXSTYLEMAPPING *FindStartPossiblesOrEnd(SYNTAXINSTANCE *theInstance,SYNTAXSTYLEMAPPINGLIST *thePossibles,SYNTAXSTYLEMAPPING *endMapping,UINT32 position,UINT32 *matchPosition,UINT32 *matchLength,bool *wasEnd)
// given a position in the text, and a list of rules to match against, see if we can find a match
// for any starting expression of any of the rules
// if so, return the matched rule, position and length
// if nothing matches, return NULL
// NOTE: if none of the starting expressions match at or before the
// next end expression match, return that
{
	UINT32
		thePieceIndex;
	UINT32
		startPosition,
		length;
	UINT32
		bestStartPosition,
		bestLength;
	SYNTAXSTYLEMAPPING
		*bestPossible;
	EXPRESSIONPIECELIST
		*thePieceListElement;

	bestStartPosition=bestLength=0;
	bestPossible=NULL;
	*wasEnd=false;

	while(thePossibles)
	{
		thePieceIndex=thePossibles->mappingEntry->startExpressionPiece->pieceIndex;

		if(LocatePieceWithOffset(&theInstance->theExpressionPieces[thePieceIndex],position,&startPosition,&length))
		{
			if(!bestPossible||startPosition<bestStartPosition)
			{
				bestPossible=thePossibles->mappingEntry;
				bestStartPosition=startPosition;
				bestLength=length;
			}
		}
		thePossibles=thePossibles->next;
	}

	if(endMapping&&endMapping->endExpressionPieces)			// see if there is a possible end to search for
	{
		thePieceListElement=endMapping->endExpressionPieces;
		while(thePieceListElement)
		{
			thePieceIndex=thePieceListElement->thePiece->pieceIndex;
			if(LocatePieceWithOffset(&theInstance->theExpressionPieces[thePieceIndex],position,&startPosition,&length))
			{
				if(!bestPossible||startPosition<bestStartPosition)
				{
					bestPossible=endMapping;
					bestStartPosition=startPosition;
					bestLength=length;
					*wasEnd=true;
				}
			}
			thePieceListElement=thePieceListElement->next;
		}
	}

	if(bestPossible)		// found a match!
	{
		*matchPosition=bestStartPosition;
		*matchLength=bestLength;

//		fprintf(stderr,"Best was %s: %d:%d\n",&bestPossible->mappingName[0],bestStartPosition,bestLength);

		return(bestPossible);
	}
	return(NULL);
}

static bool HandleMatchesForDepth(EDITORBUFFER *theBuffer,SYNTAXSTYLEMAPPINGLIST *startRules,SYNTAXSTYLEMAPPING *endRule,UINT32 *currentPosition,UINT32 betweenStyle)
// Recursively find nested matches, adjust styles, until we find the end match for this depth,
// or run out of data
// NOTE: this will return false when it could not locate the end match for the current depth
// NOTE: even if this returns false, it may have updated currentPosition
{
	UINT32
		matchPosition,
		matchLength;
	SYNTAXSTYLEMAPPING
		*theMapping;
	bool
		wasEnd;

	while(true)
	{
		if((theMapping=FindStartPossiblesOrEnd(theBuffer->syntaxInstance,startRules,endRule,*currentPosition,&matchPosition,&matchLength,&wasEnd)))
		{
			SetStyleRange(theBuffer->styleUniverse,*currentPosition,matchPosition-*currentPosition,betweenStyle);
			*currentPosition=matchPosition+matchLength;		// move to the end of this match

			if(!wasEnd)
			{
				SetStyleRange(theBuffer->styleUniverse,matchPosition,matchLength,theMapping->startStyle);

				if(theMapping->endExpressionPieces)			// if an ending expression was defined, go look for it
				{
					if(!HandleMatchesForDepth(theBuffer,theMapping->betweenList,theMapping,currentPosition,theMapping->betweenStyle))
					{
						return(false);
					}
				}
			}
			else
			{
				SetStyleRange(theBuffer->styleUniverse,matchPosition,matchLength,theMapping->endStyle);
				return(true);
			}
		}
		else	// had an end expression, but unable to locate it, so this mapping goes to the end of the text, and we're done
		{
			SetStyleRange(theBuffer->styleUniverse,*currentPosition,theBuffer->textUniverse->totalBytes-*currentPosition,betweenStyle);			// set style to end of document
			*currentPosition=theBuffer->textUniverse->totalBytes;
			return(false);
		}
	}
}

static void HandleMatchesForDepth0(EDITORBUFFER *theBuffer,SYNTAXSTYLEMAPPINGLIST *startRules,UINT32 currentPosition,UINT32 lastPosition)
// When at depth 0, this attempts to find matches and calls HandleMatchesForDepth for greater depths
// At depth 0, we do some additional work to keep the mapping table up to date
// and to see if it is possible to stop working if we are in sync with the mapping
// table.
{
	UINT32
		matchPosition,
		matchLength;
	SYNTAXSTYLEMAPPING
		*theMapping;
	UINT32
		oldMappingStart,
		oldMappingNumElements;
	MAPPINGELEMENT
		*oldMappingElement,
		newMappingElement;
	bool
		wasEnd,
		done;

	done=false;
	while(!done)
	{
		if((theMapping=FindStartPossiblesOrEnd(theBuffer->syntaxInstance,startRules,NULL,currentPosition,&matchPosition,&matchLength,&wasEnd)))
		{
			SetStyleRange(theBuffer->styleUniverse,currentPosition,matchPosition-currentPosition,0);	// clear up to this point
			ClearSARange(&theBuffer->syntaxInstance->mappingArray,currentPosition,matchPosition-currentPosition);	// clear entries in the mapping table up to this point

			SetStyleRange(theBuffer->styleUniverse,matchPosition,matchLength,theMapping->startStyle);	// set the starting style for the match
			currentPosition=matchPosition+matchLength;

			if(theMapping->endExpressionPieces)	// if an ending expression has been defined, go look for it, but be willing to step deeper if a start match from the between list is located
			{
				done=!HandleMatchesForDepth(theBuffer,theMapping->betweenList,theMapping,&currentPosition,theMapping->betweenStyle);	// hunt after the start for the end
				matchLength=currentPosition-matchPosition;	// update to get length of entire match
			}
			if(currentPosition>lastPosition)	// see if we need to test for being back in sync
			{
				if(GetSARecordAtOrAfterPosition(&theBuffer->syntaxInstance->mappingArray,currentPosition,&oldMappingStart,&oldMappingNumElements,(void **)(&oldMappingElement)))
				{
					if(oldMappingStart>=currentPosition)	// true if in sync with old mapping table now
					{
						done=true;
					}
				}
				else
				{
					done=true;					// if no old record, then also done
				}
			}
			newMappingElement.theMapping=theMapping;
			SetSARange(&theBuffer->syntaxInstance->mappingArray,matchPosition,matchLength,&newMappingElement);	// drop this into the mapping array
		}
		else
		{
			SetStyleRange(theBuffer->styleUniverse,currentPosition,theBuffer->textUniverse->totalBytes-currentPosition,0);	// clear style to end of document
			ClearSARange(&theBuffer->syntaxInstance->mappingArray,currentPosition,theBuffer->textUniverse->totalBytes-currentPosition);	// remove all mapping entries below here
			done=true;
		}
	}
}

static bool UpdateMappingCache(EDITORBUFFER *theBuffer,UINT32 earliestExpressionChange,UINT32 startOffset,UINT32 endOffset,UINT32 numBytes)
// Now that all of the expressions have been updated, back up to the location
// of the first expression cache change, work out which syntax rule was in effect,
// and start running through the rules, updating the styles as we go, until we
// get to the first place after the last expression cache change where the cached
// rules and the current rule set are the same (when we fall back into the pattern
// that existed before) or to the end of the document, whichever comes first
{
	UINT32
		startPosition,
		numElements;
	MAPPINGELEMENT
		*theMappingElement;

	if(AdjustMappingTableForChange(theBuffer->syntaxInstance,startOffset,endOffset,numBytes))
	{
		if(earliestExpressionChange)
		{
			earliestExpressionChange--;		// step back one to get to a record which we know has data that was valid before the change
		}
		if(GetSARecordAtOrAfterPosition(&theBuffer->syntaxInstance->mappingArray,earliestExpressionChange,&startPosition,&numElements,(void **)(&theMappingElement)))	// find the mapping in effect at the time of the earliest expression change
		{
			if(startPosition<earliestExpressionChange)	// if mapping was in effect, back up to the start of it
			{
				earliestExpressionChange=startPosition;
			}
		}
		// now, earliestExpressionChange points to the spot in the text where the last known root level status was "no mapping in effect"
		// start the search from here and run to at least startOffset+numBytes

		EditorStartStyleChange(theBuffer);
		HandleMatchesForDepth0(theBuffer,theBuffer->syntaxInstance->parentMap->rootMapList,earliestExpressionChange,startOffset+numBytes);
		EditorEndStyleChange(theBuffer);
		return(true);
	}
	return(false);
}

bool UpdateSyntaxInformation(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,UINT32 numBytes)
// A change has been made in theBuffer.
// The text between startOffset and endOffset has been replaced with numBytes
// This will recalculate all the syntax highlighting information for the area of the change
// NOTE: if this is called for a universe which has no syntax highlighting, it does
// nothing.
// NOTE: since regular expressions can match ending at boundaries, we need
// to begin checking 1 character before the change, so we can match expressions
// which may now be matching/not matching because of the ending boundary change
// NOTE: since regular expressions can match starting at boundaries, we need to look
// one character past the end for the same reason.
{
	bool
		fail;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		earliestExpressionChange;
	SYNTAXEXPRESSION
		*theExpression;

	fail=false;
	if(theBuffer->syntaxInstance)	// only do work if this universe has a syntax instance
	{
		// fudge start and end parameters out by 1 so that boundary conditions are checked properly
		if(startOffset)
		{
			startOffset--;
			numBytes++;					// account for byte we just moved back
		}
		if((startOffset+numBytes)<theBuffer->textUniverse->totalBytes)
		{
			endOffset++;
			numBytes++;					// account for byte we just moved out
		}

		PositionToChunkPositionPastEnd(theBuffer->textUniverse,startOffset,&theChunk,&theOffset);	// locate the start of the change

		earliestExpressionChange=startOffset;		// this is the max earliest expression change

		theExpression=theBuffer->syntaxInstance->parentMap->firstExpression;
		while(theExpression&&!fail)		// update all the expressions
		{
			fail=!UpdateExpressionCaches(theChunk,theOffset,theBuffer->syntaxInstance,theExpression,startOffset,endOffset,numBytes,theBuffer->textUniverse->totalBytes,&earliestExpressionChange);
			theExpression=theExpression->next;
		}

		if(!fail)
		{
			fail=!UpdateMappingCache(theBuffer,earliestExpressionChange,startOffset,endOffset,numBytes);	// update the mapping of expressions to style
		}
	}
	return(!fail);
}

bool RegenerateAllSyntaxInformation(EDITORBUFFER *theBuffer)
// Run through theBuffer, and recreate its syntaxInstance, and update
// its editor universe's styleUniverse.
// This is done when a syntaxInstance is first added to an editor universe
// If there is a problem, SetError, return false
{
	return(UpdateSyntaxInformation(theBuffer,0,theBuffer->textUniverse->totalBytes,theBuffer->textUniverse->totalBytes));	// pretend as if the whole thing changed
}

SYNTAXMAP *GetInstanceParentMap(SYNTAXINSTANCE *theInstance)
// return the parent syntax map for the given instance
{
	return(theInstance->parentMap);
}

void CloseSyntaxInstance(SYNTAXINSTANCE *theInstance)
// unlink theInstance from its parent map, and destroy it
{
	UINT32
		i;

	if(theInstance->next)
	{
		theInstance->next->prev=theInstance->prev;
	}
	if(theInstance->prev)
	{
		theInstance->prev->next=theInstance->next;
	}
	if(theInstance->parentMap->firstInstance==theInstance)				// if at the head of the list, move the head
	{
		theInstance->parentMap->firstInstance=theInstance->next;
	}

	for(i=0;i<theInstance->parentMap->numExpressionPieces;i++)
	{
		UnInitSA(&theInstance->theExpressionPieces[i].matchArray);
	}

	UnInitSA(&theInstance->mappingArray);

	MDisposePtr(theInstance);
}

SYNTAXINSTANCE *OpenSyntaxInstance(SYNTAXMAP *theMap,EDITORBUFFER *theBuffer)
// Create an instance of a syntax highlight map
// link it to theMap and return it
// If there is a problem, SetError, return NULL
{
	SYNTAXINSTANCE
		*theInstance;
	SYNTAXEXPRESSION
		*theExpression;
	EXPRESSIONPIECE
		*theExpressionPiece;
	bool
		fail;
	UINT32
		i;

	fail=false;
	if((theInstance=(SYNTAXINSTANCE *)MNewPtr(sizeof(SYNTAXINSTANCE)+sizeof(EXPRESSIONPIECEINSTANCE)*theMap->numExpressionPieces)))
	{
		theInstance->theBuffer=theBuffer;
		fail=!InitSA(&theInstance->mappingArray,sizeof(MAPPINGELEMENT));

		for(i=0;(i<theMap->numExpressionPieces)&&!fail;i++)
		{
			theInstance->theExpressionPieces[i].maximumStartDistance=0;	// never saw a match with an offset larger than this yet
			fail=!InitSA(&theInstance->theExpressionPieces[i].matchArray,sizeof(EXPRESSIONMATCHELEMENT));
		}
		if(!fail)
		{
			theExpression=theMap->firstExpression;	// assign all the expression pieces
			while(theExpression)
			{
				theExpressionPiece=theExpression->firstPiece;
				while(theExpressionPiece)
				{
					theInstance->theExpressionPieces[theExpressionPiece->pieceIndex].theExpressionPiece=theExpressionPiece;
					theExpressionPiece=theExpressionPiece->next;
				}
				theExpression=theExpression->next;
			}

			theInstance->parentMap=theMap;			// point at the parent map
			theInstance->prev=NULL;
			theInstance->next=theMap->firstInstance;
			if(theMap->firstInstance)
			{
				theMap->firstInstance->prev=theInstance;
			}
			theMap->firstInstance=theInstance;
			return(theInstance);
		}
		MDisposePtr(theInstance);
	}
	return(NULL);
}

// -----------------------------------------------------------------------------------------------------------

// Code for creating syntax maps

void RemoveMappingList(SYNTAXSTYLEMAPPINGLIST *theList)
// remove all elements of theList
{
	SYNTAXSTYLEMAPPINGLIST
		*next;

	while(theList)
	{
		next=theList->next;
		MDisposePtr(theList);
		theList=next;
	}
}

SYNTAXSTYLEMAPPINGLIST *AddMappingListElement(SYNTAXSTYLEMAPPINGLIST **theListHead,SYNTAXSTYLEMAPPING *theMapping)
// Create a new mapping list element, and link it to the end of the given list
{
	SYNTAXSTYLEMAPPINGLIST
		*previousElement;
	SYNTAXSTYLEMAPPINGLIST
		*theMappingListElement;

	previousElement=*theListHead;
	while(previousElement&&(previousElement->mappingEntry!=theMapping)&&previousElement->next)
	{
		previousElement=previousElement->next;
	}
	if((!previousElement)||(previousElement->mappingEntry!=theMapping))
	{
		if((theMappingListElement=(SYNTAXSTYLEMAPPINGLIST *)MNewPtr(sizeof(SYNTAXSTYLEMAPPINGLIST))))
		{
			theMappingListElement->mappingEntry=theMapping;
			if(previousElement)
			{
				previousElement->next=theMappingListElement;
				theMappingListElement->prev=previousElement;
				theMappingListElement->next=NULL;
			}
			else
			{
				*theListHead=theMappingListElement;
				theMappingListElement->prev=NULL;
				theMappingListElement->next=NULL;
			}
			return(theMappingListElement);
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[LISTMAPPINGEXISTS],errorDescriptions[LISTMAPPINGEXISTS]);
	}
	return(NULL);
}

SYNTAXSTYLEMAPPINGLIST *AddBetweenListElementToMapping(SYNTAXSTYLEMAPPING *theMapping,SYNTAXSTYLEMAPPING *betweenMapping)
// Add betweenMapping to theMapping
// if there is a problem, SetError, and return false
{
	return(AddMappingListElement(&theMapping->betweenList,betweenMapping));
}

SYNTAXSTYLEMAPPINGLIST *AddRootListElementToMap(SYNTAXMAP *theMap,SYNTAXSTYLEMAPPING *rootMapping)
// Add rootMapping to theMap
// if there is a problem, SetError, and return false
{
	return(AddMappingListElement(&theMap->rootMapList,rootMapping));
}

SYNTAXSTYLEMAPPING *LocateSyntaxStyleMapping(SYNTAXMAP *theMap,char *theName)
// locate a syntax style mapping with theName in theMap, and return a pointer to it
// if none can be found, return NULL
// NOTE: this could probably benefit from a hash table
{
	SYNTAXSTYLEMAPPING
		*theMapping;

	theMapping=theMap->firstStyleMapping;
	while(theMapping&&strcmp(theMapping->mappingName,theName)!=0)
	{
		theMapping=theMapping->next;
	}
	return(theMapping);
}

void SetMappingStartExpressionPiece(SYNTAXSTYLEMAPPING *theMapping,EXPRESSIONPIECE *startExpressionPiece)
// Set the pointer to the start expression piece for theMapping
{
	theMapping->startExpressionPiece=startExpressionPiece;
}

void RemoveExpressionList(EXPRESSIONPIECELIST *theList)
// remove all elements of theList
{
	EXPRESSIONPIECELIST
		*next;

	while(theList)
	{
		next=theList->next;
		MDisposePtr(theList);
		theList=next;
	}
}

EXPRESSIONPIECELIST *AddMappingEndExpressionPiece(SYNTAXSTYLEMAPPING *theMapping,EXPRESSIONPIECE *endExpressionPiece)
// Add endExpression piece to the end of the list of ending expression pieces hanging off of theMapping
// NOTE: this considers it an error to have two of the same piece on the list
// if there is a problem, SetError, and return NULL
{
	EXPRESSIONPIECELIST
		*previousElement;
	EXPRESSIONPIECELIST
		*theListElement;

	previousElement=theMapping->endExpressionPieces;
	while(previousElement&&(previousElement->thePiece!=endExpressionPiece)&&previousElement->next)
	{
		previousElement=previousElement->next;
	}
	if((!previousElement)||(previousElement->thePiece!=endExpressionPiece))
	{

		if((theListElement=(EXPRESSIONPIECELIST *)MNewPtr(sizeof(EXPRESSIONPIECELIST))))
		{
			theListElement->thePiece=endExpressionPiece;
			if(previousElement)
			{
				previousElement->next=theListElement;
				theListElement->prev=previousElement;
				theListElement->next=NULL;
			}
			else
			{
				theMapping->endExpressionPieces=theListElement;
				theListElement->prev=NULL;
				theListElement->next=NULL;
			}
		}
		return(theListElement);
	}
	else
	{
		SetError(localErrorFamily,errorMembers[EXPRESSIONPIECEEXISTS],errorDescriptions[EXPRESSIONPIECEEXISTS]);
	}
	return(NULL);
}

void SetMappingStyles(SYNTAXSTYLEMAPPING *theMapping,UINT32 startStyle,UINT32 betweenStyle,UINT32 endStyle)
// Set the pointer to the start expression piece for theMapping
{
	theMapping->startStyle=startStyle;
	theMapping->betweenStyle=betweenStyle;
	theMapping->endStyle=endStyle;
}

void RemoveMappingFromSyntaxMap(SYNTAXMAP *theMap,SYNTAXSTYLEMAPPING *theMapping)
// pull theMapping out of the map and delete it
{
	RemoveExpressionList(theMapping->endExpressionPieces);	// get rid of list of ending expressions

	RemoveMappingList(theMapping->betweenList);				// get rid of sub-lists

	if(theMapping->next)
	{
		theMapping->next->prev=theMapping->prev;
	}
	if(theMapping->prev)
	{
		theMapping->prev->next=theMapping->next;
	}
	if(theMap->firstStyleMapping==theMapping)				// if at the head of the list, move the head
	{
		theMap->firstStyleMapping=theMapping->next;
	}
	MDisposePtr(theMapping);
}

SYNTAXSTYLEMAPPING *AddMappingToSyntaxMap(SYNTAXMAP *theMap,char *mappingName)
// Add a mapping of expressions to styles to theMap
// Initially, the mapping is empty, and must be filled in by calls to the above routines
// if there is a problem, SetError, and return NULL
{
	SYNTAXSTYLEMAPPING
		*theMapping;

	if(!LocateSyntaxStyleMapping(theMap,mappingName))
	{
		if((theMapping=(SYNTAXSTYLEMAPPING *)MNewPtr(sizeof(SYNTAXSTYLEMAPPING)+strlen(mappingName)+1)))
		{
			theMapping->parent=theMap;

			strcpy(&theMapping->mappingName[0],mappingName);	// copy over the name

			theMapping->startExpressionPiece=NULL;
			theMapping->endExpressionPieces=NULL;

			theMapping->startStyle=0;
			theMapping->betweenStyle=0;
			theMapping->endStyle=0;

			theMapping->betweenList=NULL;

			theMapping->prev=NULL;
			theMapping->next=theMap->firstStyleMapping;			// link it onto the list
			if(theMap->firstStyleMapping)
			{
				theMap->firstStyleMapping->prev=theMapping;
			}
			theMap->firstStyleMapping=theMapping;
			return(theMapping);
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STYLEMAPPINGEXISTS],errorDescriptions[STYLEMAPPINGEXISTS]);
	}
	return(NULL);
}

EXPRESSIONPIECE *LocateExpressionPiece(SYNTAXEXPRESSION *theExpression,bool useRegisterMatch,UINT32 registerIndex)
// Attempt to find the piece of theExpression which has the given attributes,
// If no piece can be located, return NULL
{
	EXPRESSIONPIECE
		*thePiece;
	bool
		found;

	found=false;
	thePiece=theExpression->firstPiece;
	while(thePiece&&!found)
	{
		if((!useRegisterMatch&&!thePiece->useRegisterMatch)||(useRegisterMatch&&thePiece->useRegisterMatch&&(thePiece->registerIndex==registerIndex)))
		{
			found=true;
		}
		else
		{
			thePiece=thePiece->next;
		}
	}
	return(thePiece);
}

void RemovePieceFromExpression(SYNTAXEXPRESSION *theExpression,EXPRESSIONPIECE *thePiece)
// Remove thePiece from theExpression
{
	if(thePiece->next)
	{
		thePiece->next->prev=thePiece->prev;
	}
	if(thePiece->prev)
	{
		thePiece->prev->next=thePiece->next;
	}
	if(theExpression->firstPiece==thePiece)				// if at the head of the list, move the head
	{
		theExpression->firstPiece=thePiece->next;
	}
	MDisposePtr(thePiece);
}

EXPRESSIONPIECE *AddPieceToExpression(SYNTAXEXPRESSION *theExpression,bool useRegisterMatch,UINT32 registerIndex)
// Add a piece to theExpression
// NOTE: if there is already a piece on theExpression matching the given one, just return it
// if there is a problem, SetError, return NULL
{
	EXPRESSIONPIECE
		*thePiece;

	if(!(thePiece=LocateExpressionPiece(theExpression,useRegisterMatch,registerIndex)))
	{
		if((thePiece=(EXPRESSIONPIECE *)MNewPtr(sizeof(EXPRESSIONPIECE))))
		{
			thePiece->parent=theExpression;
			thePiece->prev=NULL;
			thePiece->next=theExpression->firstPiece;
			if(theExpression->firstPiece)
			{
				theExpression->firstPiece->prev=thePiece;
			}
			theExpression->firstPiece=thePiece;

			thePiece->useRegisterMatch=useRegisterMatch;	// tells if entire expression matches, or a register matches
			thePiece->registerIndex=registerIndex;			// if useRegisterMatch is true, this gives the register index of the match

			thePiece->pieceIndex=theExpression->parent->numExpressionPieces;
			theExpression->parent->numExpressionPieces++;
		}
	}
	return(thePiece);
}

SYNTAXEXPRESSION *LocateSyntaxMapExpression(SYNTAXMAP *theMap,char *theName)
// locate an expression with theName in theMap, and return a pointer to it
// if none can be found, return NULL
{
	SYNTAXEXPRESSION
		*theExpression;

	theExpression=theMap->firstExpression;
	while(theExpression&&strcmp(theExpression->expressionName,theName)!=0)
	{
		theExpression=theExpression->next;
	}
	return(theExpression);
}

void RemoveExpressionFromSyntaxMap(SYNTAXMAP *theMap,SYNTAXEXPRESSION *theExpression)
// pull theExpression out of the map and delete it
// NOTE: it is the caller's responsibility to make sure there are no
// mappings which might be left pointing to the given expression
// after it is deleted
{
	REFree(theExpression->compiledExpression);

	if(theExpression->next)
	{
		theExpression->next->prev=theExpression->prev;
	}
	if(theExpression->prev)
	{
		theExpression->prev->next=theExpression->next;
	}
	if(theMap->firstExpression==theExpression)				// if at the head of the list, move the head
	{
		theMap->firstExpression=theExpression->next;
	}
	MDisposePtr(theExpression);
}

SYNTAXEXPRESSION *AddExpressionToSyntaxMap(SYNTAXMAP *theMap,char *expressionName,char *expressionText)
// Add a new expression to theMap
// If there is a problem, SetError, and return NULL
{
	SYNTAXEXPRESSION
		*theExpression;

	if(!LocateSyntaxMapExpression(theMap,expressionName))
	{
		if((theExpression=(SYNTAXEXPRESSION *)MNewPtr(sizeof(SYNTAXEXPRESSION)+strlen(expressionName)+1)))
		{
			if((theExpression->compiledExpression=RECompile((UINT8 *)expressionText,(UINT32)strlen(expressionText),NULL)))
			{
				strcpy(&theExpression->expressionName[0],expressionName);	// copy over the name
				theExpression->parent=theMap;
				theExpression->prev=NULL;
				theExpression->next=theMap->firstExpression;		// link it onto the list
				if(theMap->firstExpression)
				{
					theMap->firstExpression->prev=theExpression;
				}
				theMap->firstExpression=theExpression;
				theExpression->firstPiece=NULL;						// no pieces defined to match yet
				return(theExpression);
			}
			MDisposePtr(theExpression);
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[EXPRESSIONEXISTS],errorDescriptions[EXPRESSIONEXISTS]);
	}
	return(NULL);
}

char *GetSyntaxMapName(SYNTAXMAP *theMap)
// return a pointer to the name of the given syntax map
{
	return(&theMap->mapName[0]);
}

SYNTAXMAP *LocateNextSyntaxMap(SYNTAXMAP *theMap)
// return a pointer to the syntax map defined after the passed one
// if none can be found, return NULL
// if NULL is passed in, return the first one
{
	if(theMap)
	{
		return(theMap->next);
	}
	else
	{
		return(syntaxMapHead);
	}
}

SYNTAXMAP *LocateSyntaxMap(char *theName)
// locate a syntax map with theName, and return a pointer to it
// if none can be found, return NULL
{
	SYNTAXMAP
		*theMap;

	theMap=syntaxMapHead;
	while(theMap&&strcmp(theMap->mapName,theName)!=0)
	{
		theMap=theMap->next;
	}
	return(theMap);
}

void UnlinkSyntaxMap(SYNTAXMAP *theMap)
// unlink theMap from the global list of maps
{
	if(theMap->next)
	{
		theMap->next->prev=theMap->prev;
	}
	else
	{
		syntaxMapTail=theMap->prev;
	}

	if(theMap->prev)
	{
		theMap->prev->next=theMap->next;
	}
	else
	{
		syntaxMapHead=theMap->next;
	}
}

bool LinkSyntaxMap(SYNTAXMAP *theMap)
// link theMap to the global list of maps
// NOTE: this must be called only once for each map
// NOTE: this will fail only if a map already exists in the global list
// which has the same name as theMap.
{
	if(!LocateSyntaxMap(theMap->mapName))
	{
		theMap->prev=syntaxMapTail;
		theMap->next=NULL;
		if(syntaxMapTail)
		{
			syntaxMapTail->next=theMap;
			syntaxMapTail=theMap;
		}
		else
		{
			syntaxMapHead=syntaxMapTail=theMap;
		}
		return(true);
	}
	else
	{
		SetError(localErrorFamily,errorMembers[MAPEXISTS],errorDescriptions[MAPEXISTS]);
	}
	return(false);
}

void CloseSyntaxMap(SYNTAXMAP *theMap)
// dispose of a syntax map
// NOTE: the map must not be in use by any buffer when this is called
{
	RemoveMappingList(theMap->rootMapList);	// get rid of the root mapping list

	while(theMap->firstStyleMapping)
	{
		RemoveMappingFromSyntaxMap(theMap,theMap->firstStyleMapping);
	}

	while(theMap->firstExpression)
	{
		while(theMap->firstExpression->firstPiece)
		{
			RemovePieceFromExpression(theMap->firstExpression,theMap->firstExpression->firstPiece);
		}
		RemoveExpressionFromSyntaxMap(theMap,theMap->firstExpression);
	}

	MDisposePtr(theMap);
}

SYNTAXMAP *OpenSyntaxMap(char *theName)
// create an empty syntax map with the given name, link it to the
// end of the global list
// if there is a problem, set the error, and return NULL
{
	SYNTAXMAP
		*theMap;

	if((theMap=(SYNTAXMAP *)MNewPtr(sizeof(SYNTAXMAP)+strlen(theName)+1)))
	{
		theMap->numExpressionPieces=0;
		theMap->firstExpression=NULL;
		theMap->firstStyleMapping=NULL;
		theMap->firstInstance=NULL;
		theMap->rootMapList=NULL;
		strcpy(&theMap->mapName[0],theName);	// copy over the name
		theMap->prev=NULL;
		theMap->next=NULL;
		return(theMap);
	}
	return(NULL);
}

EDITORBUFFER *LocateNextEditorBufferOnMap(SYNTAXMAP *theMap,EDITORBUFFER *theBuffer)
// Locate the next editor universe linked to theMap
// If theBuffer is passed as NULL, return the first buffer
// If there are no more buffers linked to the map, return NULL
{
	if(theBuffer)
	{
		if(theBuffer->syntaxInstance->next)
		{
			return(theBuffer->syntaxInstance->next->theBuffer);
		}
	}
	else
	{
		if(theMap->firstInstance)
		{
			return(theMap->firstInstance->theBuffer);
		}
	}
	return(NULL);
}

SYNTAXMAP *GetAssignedSyntaxMap(EDITORBUFFER *theBuffer)
// Return the syntax map assigned to theBuffer
// if none has been assigned, return NULL
{
	if(theBuffer->syntaxInstance)					// get rid of anything that may have been there
	{
		return(GetInstanceParentMap(theBuffer->syntaxInstance));
	}
	return(NULL);
}

bool AssignSyntaxMap(EDITORBUFFER *theBuffer,SYNTAXMAP *theMap)
// Assign or remove a SYNTAXMAP from theBuffer
// if theMap is passed as NULL, remove any current map
{
	bool
		fail;

	fail=false;
	if(theBuffer->syntaxInstance)					// get rid of anything that may have been there
	{
		CloseSyntaxInstance(theBuffer->syntaxInstance);
		theBuffer->syntaxInstance=NULL;				// remember this is gone

		if(!theMap)
		{
			// Clear style information for the universe
			EditorStartStyleChange(theBuffer);
			SetStyleRange(theBuffer->styleUniverse,0,theBuffer->textUniverse->totalBytes,0);	// clear out all style info
			EditorEndStyleChange(theBuffer);
		}
	}
	if(theMap)
	{
		if((theBuffer->syntaxInstance=OpenSyntaxInstance(theMap,theBuffer)))
		{
			fail=!RegenerateAllSyntaxInformation(theBuffer);	// update all the information
		}
		else
		{
			fail=true;										// something went wrong trying to get the instance
		}
	}
	return(!fail);
}

void UnInitSyntaxMaps()
// undo what InitSyntaxMaps did
// NOTE: the caller much make sure no maps are in use
// before calling this routine.
{
	SYNTAXMAP
		*theMap;

	while((theMap=syntaxMapHead))
	{
		UnlinkSyntaxMap(theMap);
		CloseSyntaxMap(theMap);
	}
}

bool InitSyntaxMaps()
// initialize the global table of syntax maps
{
	syntaxMapHead=syntaxMapTail=NULL;
	return(true);
}
