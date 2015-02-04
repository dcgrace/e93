// Regular expression handling
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

#define BITTABLELENGTH				(256/8)			// number of bytes in a bit table
#define MAXSTRINGPERNODE			BITTABLELENGTH	// maximum number of characters in a string within a node (unioned with bit table)
#define	MAXRECURSIONDEPTH			8000			// maximum amount of recursion we deem allowable

typedef enum
{
	// NOTE: the order of these IS important, there are tables below that match this order
	PT_STRING=0,				// matching constant string
	PT_LIST,					// matching one character to see if it is in a list
	PT_PREVIOUSMATCH,			// matching against data found in previous match
	PT_SUBEXPRESSIONSTART,		// matching against data found in a sub expression
	PT_SUBEXPRESSIONEND,		// end of sub expression match
	PT_LEFTEDGE,				// left edge must be one of the given characters (or edge of search)
	PT_RIGHTEDGE,				// right edge must be one of the given characters (or edge of search)
} PATTERNTYPE;

typedef struct
{
	CHUNKHEADER
		*theChunk;				// chunk and offset that start this register (if NULL, register not yet matched)
	UINT32
		theOffset;
	UINT32
		matchIndex;				// index into matched data where this register was matched
	UINT32
		numBytes;				// number of bytes matched by this register
	UINT32
		minimumMatchLength,		// used only during analysis, tells the minimum length of a pattern that could be matched by this register (0 means it can match nothing)
		maximumMatchLength;		// used only during analysis, tells the maximum length of a pattern that could be matched by this register
	bool
		infiniteMatchLength;	// used only during analysis, tells if the maximum length of a pattern could be infinite (in which case maximumMatchLength should be ignored)
} REGISTER;

typedef struct expressionNode
{
	PATTERNTYPE
		theType;				// type of pattern this element represents
	UINT8
		dataLength;				// length of string data if it exists
	union
	{
		UINT8
			theData[MAXSTRINGPERNODE];		// characters of the string to compare against (if string type)
		UINT8
			theBits[BITTABLELENGTH];		// the bits that tell if a character is or is not to be accepted (in a group) (also used in edge searches)
		REGISTER
			*theRegister;		// points to the register which holds the start and end of the last match we are to compare against (for register type)
		struct
		{
			struct expressionListHeader
				*subExpressionList;	// pointer to expression list for this subexpression
			UINT32
				currentCount;	// current number of successful iterations of the subexpression
			REGISTER
				*theRegister;	// points to the register to remember match of subexpression in (NULL if none remembered)
			CHUNKHEADER
				*lastChunk;		// the chunk pointer that was active last time
			UINT32
				lastOffset;		// the offset that was active last time
			UINT32
				lastNumToSearch;	// number of bytes we were asked to search last time
			bool
				justEntered;	// tells if just entered this subexpression
		} se;
	} pt;
	bool
		matchSpecified;			// user has specified a match limit for this node
	UINT32
		matchMin,				// minimum number of times to match pattern (0 means we dont even have to match this)
		matchMax;				// maximum number of times to match pattern (0 means infinite)
	struct expressionNode
		*nextExpression,		// next expression along the line at this level
		*parentExpression;		// parent expression, or NULL if at top
} EXPRESSIONNODE;

typedef struct expressionListHeader
{
	EXPRESSIONNODE
		*parentExpression;		// parent that got us here, or NULL if at top
	EXPRESSIONNODE
		*firstExpression;		// first expression in the list for this level
	struct expressionListHeader
		*nextAlternate;			// next alternate expression list for this level (NULL for no more)
} EXPRESSIONLISTHEADER;

typedef struct compiledExpression
{
	EXPRESSIONLISTHEADER
		*firstList;				// points to the first top-level expression list
	UINT32
		numRegisters;			// incremented every time a register is defined during compile
	bool
		registerDefined[MAXREGISTERS];	// used during compilation to remember which registers have been defined already
	REGISTER
		theRegisters[MAXREGISTERS];		// the registers that keep track of various match points
	bool
		charsMatched[256];		// contains list of all characters that can possibly be matched by this expression
	bool
		startCharsMatched[256];	// contains list of all characters that can possibly be matched by this expression at the start
	UINT32
		minimumMatchLength,		// tells the minimum length of a pattern that could be matched by this expression (0 means it can match nothing)
		maximumMatchLength;		// tells the maximum length of a pattern that could be matched by this expression
	bool
		infiniteMatchLength;	// tells if the maximum length of a pattern could be infinite (in which case maximumMatchLength should be ignored)
} COMPILEDEXPRESSION;

typedef struct
{
	bool
		leftEdge,				// tells if search is starting on logical left edge (used for edge match operations < and ^)
		rightEdge;				// tells if search is starting on logical right edge (used for edge match operations > and $)
	bool
		checkAbort;
	UINT32
		initialNumToSearch;		// tells how many bytes we were asked to search through at the start
	CHUNKHEADER
		*endChunk;				// these are passed in everywhere
	UINT32
		endOffset;
} RECURSEGLOBALS;

static char localErrorFamily[]="regex";

enum
{
	UNBALANCEDPAREN,
	UNBALANCEDSQUAREBRACKET,
	BADRANGEEXPRESSION,
	BADREPETITION,
	INVALIDREGISTER,
	DANGLINGQUOTE,
	INVALIDDIGIT,
	MISSINGDIGIT,
	STACKOVERFLOW
};

static char *errorMembers[]=
{
	"UnbalancedParen",
	"UnbalancedSquareBracket",
	"BadRangeExpression",
	"BadRepetition",
	"InvalidRegister",
	"DanglingQuote",
	"InvalidDigit",
	"MissingDigit",
	"StackOverflow"
};

static char *errorDescriptions[]=
{
	"Unbalanced ()",
	"Unbalanced [ or [^",
	"Invalid {} expression",
	"Repetition operator *, +, ?, or {} applied to nothing",
	"Invalid register specification",
	"Incomplete \\ expression",
	"Invalid digit encountered",
	"Digit expected, but not found",
	"Expression stack overflow"
};

static EXPRESSIONNODE *REAllocateNode()
// allocate, and return an expression node
// if there is a problem, SetError, and return NULL
{
	return((EXPRESSIONNODE *)MNewPtr(sizeof(EXPRESSIONNODE)));
}

static void REFreeNode(EXPRESSIONNODE *theExpressionNode)
// free the node given by theExpressionNode
{
	MDisposePtr(theExpressionNode);
}

static EXPRESSIONLISTHEADER *REAllocateList()
// allocate, and return an expression list header
// if there is a problem, SetError, and return NULL
{
	return((EXPRESSIONLISTHEADER *)MNewPtr(sizeof(EXPRESSIONLISTHEADER)));
}

static void REFreeList(EXPRESSIONLISTHEADER *theExpressionList)
// free the list given by theExpressionList
{
	MDisposePtr(theExpressionList);
}

static EXPRESSIONNODE *REAddString(EXPRESSIONNODE *theParent,EXPRESSIONNODE *currentNode,UINT8 theChar,const UINT8 *translateTable)
// add theChar to currentNode. If currentNode is NULL, or full, or not of the correct type
// then create a new node, and make theChar the first of the string
// NOTE: if currentNode was full, the new node is linked to it's nextExpression
// return the node that theChar was added to.
// if there is a problem, SetError, and return NULL
{
	EXPRESSIONNODE
		*theNode;

	if(currentNode&&(currentNode->theType==PT_STRING)&&(!currentNode->matchSpecified)&&(currentNode->dataLength<MAXSTRINGPERNODE))
	{
		theNode=currentNode;							// there is space in the current node, so add this in
		if(translateTable)
		{
			theNode->pt.theData[theNode->dataLength++]=translateTable[theChar];	// drop in the data, bump the amount
		}
		else
		{
			theNode->pt.theData[theNode->dataLength++]=theChar;	// drop in the data, bump the amount
		}
	}
	else
	{
		if((theNode=REAllocateNode()))
		{
			theNode->theType=PT_STRING;					// create a node, and apply the correct type to it
			theNode->parentExpression=theParent;		// set the parent
			theNode->dataLength=1;						// so far, it contains one byte of string data
			theNode->pt.theData[0]=theChar;				// drop in the data
			theNode->matchSpecified=false;				// no match count specified for this node yet
			theNode->matchMin=1;						// until further notice, this must match exactly once
			theNode->matchMax=1;
			theNode->nextExpression=NULL;				// this is now the last expression
			if(currentNode)
			{
				currentNode->nextExpression=theNode;	// link to previous node if one existed
			}
		}
	}
	return(theNode);
}

#define	ADDCHARBIT(theArray,theChar)	((theArray)[(theChar)>>3]|=(1<<((theChar)&0x07)))		// set bit in field
#define	CLRCHARBIT(theArray,theChar)	((theArray)[(theChar)>>3]&=~(1<<((theChar)&0x07)))		// clear bit in field
#define	GETCHARBIT(theArray,theChar)	((theArray)[(theChar)>>3]&(1<<((theChar)&0x07)))		// get bit from field

static bool REGetHexDigit(UINT8 *theExpression,UINT32 *index,UINT32 numBytes,UINT8 *theNibble)
// get a hex digit from the input stream, and convert it to a nibble
// if there is a problem, set the error, and return false
{
	bool
		fail;
	UINT8
		theChar;

	fail=false;
	theChar='\0';
	if(*index<numBytes)
	{
		theChar=theExpression[(*index)++];
		if(theChar>='a')
		{
			theChar-='a'-'A';			// make lower case into upper case
		}
		if(theChar>='A')
		{
			theChar-='A'-10;			// make letter into number
			if(theChar>15)
			{
				SetError(localErrorFamily,errorMembers[INVALIDDIGIT],errorDescriptions[INVALIDDIGIT]);
				fail=true;
			}
		}
		else
		{
			if(theChar>='0')
			{
				theChar-='0';
				if(theChar>9)
				{
					SetError(localErrorFamily,errorMembers[INVALIDDIGIT],errorDescriptions[INVALIDDIGIT]);
					fail=true;
				}
			}
			else
			{
				SetError(localErrorFamily,errorMembers[INVALIDDIGIT],errorDescriptions[INVALIDDIGIT]);
				fail=true;
			}
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[MISSINGDIGIT],errorDescriptions[MISSINGDIGIT]);
		fail=true;
	}
	*theNibble=theChar;
	return(!fail);
}

static bool REConvertQuotedCharacter(UINT8 *theExpression,UINT32 *index,UINT32 numBytes,UINT8 *theChar)
// convert character after a \ into what it actually represents
// (for instance \n means newline)
// this can fail if there are no more characters after the \, or
// if the characters do not make sense
{
	bool
		fail;
	UINT8
		highNibble,
		lowNibble;

	fail=false;
	if(*index<numBytes)
	{
		*theChar=theExpression[(*index)++];
		switch(*theChar)
		{
			case 'b':
				*theChar=0x08;
				break;
			case 'n':
				*theChar='\n';
				break;
			case 'r':
				*theChar='\r';
				break;
			case 't':
				*theChar='\t';
				break;
			case 'x':
				if(REGetHexDigit(theExpression,index,numBytes,&highNibble))
				{
					if(REGetHexDigit(theExpression,index,numBytes,&lowNibble))
					{
						*theChar=highNibble*16+lowNibble;			// create actual character
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
				break;
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[DANGLINGQUOTE],errorDescriptions[DANGLINGQUOTE]);
		fail=true;
	}
	return(!fail);
}

static void REAddRangeToList(EXPRESSIONNODE *theNode,UINT8 rangeLow,UINT8 rangeHigh,const UINT8 *translateTable)
// add the bytes between rangeLow, and rangeHigh (inclusive) to the list in theNode
{
	UINT32
		i;

	if(translateTable)
	{
		for(i=rangeLow;i<=(UINT32)rangeHigh;i++)
		{
			ADDCHARBIT(&(theNode->pt.theBits[0]),translateTable[i]);
		}
	}
	else
	{
		for(i=rangeLow;i<=(UINT32)rangeHigh;i++)
		{
			ADDCHARBIT(&(theNode->pt.theBits[0]),i);
		}
	}
}

static EXPRESSIONNODE *REAddList(EXPRESSIONNODE *theParent,EXPRESSIONNODE *currentNode,UINT8 *theExpression,UINT32 *index,UINT32 numBytes,const UINT8 *translateTable)
// list operator has started, parse it, and create a list node in the list
// if there is a problem, SetError, and return NULL
{
	EXPRESSIONNODE
		*theNode;
	UINT8
		theChar,
		lastChar;
	bool
		notSearch,
		done,
		fail;
	UINT32
		i;

	if(*index<numBytes)
	{
		if((theNode=REAllocateNode()))
		{
			theNode->theType=PT_LIST;						// create a node, and apply the correct type to it
			theNode->parentExpression=theParent;			// set the parent
			theNode->matchSpecified=false;					// no match count specified for this node yet
			theNode->matchMin=1;							// until further notice, this must match exactly once
			theNode->matchMax=1;
			theNode->nextExpression=NULL;					// this is now the last expression
			for(i=0;i<BITTABLELENGTH;i++)					// clear the array
			{
				theNode->pt.theBits[i]=0;
			}
			done=fail=false;
			notSearch=false;
			i=*index;										// remember where we started
			lastChar=0;										// start range at 0
			while(!done&&!fail&&(*index<numBytes))
			{
				switch(theChar=theExpression[(*index)++])
				{
					case '\\':
						if(REConvertQuotedCharacter(theExpression,index,numBytes,&theChar))
						{
							if(translateTable)
							{
								ADDCHARBIT(&(theNode->pt.theBits[0]),translateTable[theChar]);
							}
							else
							{
								ADDCHARBIT(&(theNode->pt.theBits[0]),theChar);
							}
						}
						else
						{
							fail=true;
						}
						break;
					case '^':
						if(*index-1==i)						// at start??
						{
							notSearch=true;					// set not flag
							theChar=0;						// keep last char pointed to the start
						}
						else
						{
							if(translateTable)
							{
								ADDCHARBIT(&(theNode->pt.theBits[0]),translateTable[theChar]);
							}
							else
							{
								ADDCHARBIT(&(theNode->pt.theBits[0]),theChar);
							}
						}
						break;
					case '-':
						if(*index<numBytes)					// get the next character for the range (if there is one)
						{
							theChar=theExpression[(*index)++];
							switch(theChar)
							{
								case '\\':
									if(REConvertQuotedCharacter(theExpression,index,numBytes,&theChar))
									{
										REAddRangeToList(theNode,lastChar,theChar,translateTable);
									}
									else
									{
										fail=true;
									}
									break;
								case ']':
									theChar=255;
									REAddRangeToList(theNode,lastChar,theChar,translateTable);		// range pointing into space is assumed max value
									done=true;
									break;
								default:
									REAddRangeToList(theNode,lastChar,theChar,translateTable);		// range pointing into space is assumed max value
									break;
							}
						}
						else
						{
							SetError(localErrorFamily,errorMembers[UNBALANCEDSQUAREBRACKET],errorDescriptions[UNBALANCEDSQUAREBRACKET]);
							fail=true;
						}
						break;
					case ']':
						done=true;							// scanning is done (empty lists are allowed)
						break;
					default:								// otherwise it is normal, and just gets added in
						if(translateTable)
						{
							ADDCHARBIT(&(theNode->pt.theBits[0]),translateTable[theChar]);
						}
						else
						{
							ADDCHARBIT(&(theNode->pt.theBits[0]),theChar);
						}
						break;
				}
				lastChar=theChar;							// remember this for next time around the loop
			}
			if(!done&&!fail)								// ran out of things to scan
			{
				SetError(localErrorFamily,errorMembers[UNBALANCEDSQUAREBRACKET],errorDescriptions[UNBALANCEDSQUAREBRACKET]);
				fail=true;
			}
			if(!fail)
			{
				if(notSearch)								// see if the table needs to be inverted
				{
					for(i=0;i<BITTABLELENGTH;i++)
					{
						theNode->pt.theBits[i]^=0xFF;		// invert all the bits in the table if doing "notSearch" search
					}
				}
				if(currentNode)
				{
					currentNode->nextExpression=theNode;	// link to previous node if one existed
				}
				return(theNode);
			}
			REFreeNode(theNode);
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[UNBALANCEDSQUAREBRACKET],errorDescriptions[UNBALANCEDSQUAREBRACKET]);
	}
	return(NULL);
}

static EXPRESSIONNODE *REAddAny(EXPRESSIONNODE *theParent,EXPRESSIONNODE *currentNode,UINT8 *theExpression,UINT32 *index,UINT32 numBytes)
// create a list that matches any character except \n
// if there is a problem, SetError, and return NULL
{
	UINT32
		i;
	EXPRESSIONNODE
		*theNode;

	if((theNode=REAllocateNode()))
	{
		theNode->theType=PT_LIST;						// create a node, and apply the correct type to it
		theNode->parentExpression=theParent;			// set the parent
		theNode->matchSpecified=false;					// no match count specified for this node yet
		theNode->matchMin=1;							// until further notice, this must match exactly once
		theNode->matchMax=1;
		theNode->nextExpression=NULL;					// this is now the last expression
		for(i=0;i<BITTABLELENGTH;i++)					// set the array
		{
			theNode->pt.theBits[i]=0xFF;				// select ALL characters
		}
		CLRCHARBIT(&(theNode->pt.theBits[0]),'\n');		// remove newline from the list
		if(currentNode)
		{
			currentNode->nextExpression=theNode;		// link to previous node if one existed
		}
		return(theNode);
	}
	return(NULL);
}

static EXPRESSIONNODE *REAddPreviousMatch(EXPRESSIONNODE *theParent,EXPRESSIONNODE *currentNode,REGISTER *theRegister)
// link in a new node that matches previously matched text
// if there is a problem, SetError, and return NULL
{
	EXPRESSIONNODE
		*theNode;

	if((theNode=REAllocateNode()))
	{
		theNode->theType=PT_PREVIOUSMATCH;				// create a node, and apply the correct type to it
		theNode->parentExpression=theParent;			// set the parent
		theNode->matchSpecified=false;					// no match count specified for this node yet
		theNode->matchMin=1;							// until further notice, this must match exactly once
		theNode->matchMax=1;
		theNode->nextExpression=NULL;					// this is now the last expression
		theNode->pt.theRegister=theRegister;			// set pointer to the register that needs to be matched
		if(currentNode)
		{
			currentNode->nextExpression=theNode;		// link to previous node if one existed
		}
		return(theNode);
	}
	return(NULL);
}

static EXPRESSIONNODE *REAddStartOfWordMatch(EXPRESSIONNODE *theParent,EXPRESSIONNODE *currentNode)
// link in a new node that matches the start of a word
// if there is a problem, SetError, and return NULL
{
	UINT32
		i;
	EXPRESSIONNODE
		*theNode;

	if((theNode=REAllocateNode()))
	{
		theNode->theType=PT_LEFTEDGE;					// create a node, and apply the correct type to it
		theNode->parentExpression=theParent;			// set the parent
		theNode->matchSpecified=false;					// no match count specified for this node yet
		theNode->matchMin=1;							// until further notice, this must match exactly once
		theNode->matchMax=1;
		theNode->nextExpression=NULL;					// this is now the last expression

		for(i=0;i<BITTABLELENGTH;i++)					// clear the array
		{
			theNode->pt.theBits[i]=0;
		}
		for(i=0;i<256;i++)								// set the array to the current word list
		{
			if(wordSpaceTable[i])
			{
				ADDCHARBIT(&(theNode->pt.theBits[0]),i);
			}
		}

		if(currentNode)
		{
			currentNode->nextExpression=theNode;		// link to previous node if one existed
		}
		return(theNode);
	}
	return(NULL);
}

static EXPRESSIONNODE *REAddEndOfWordMatch(EXPRESSIONNODE *theParent,EXPRESSIONNODE *currentNode)
// link in a new node that matches the end of a word
// if there is a problem, SetError, and return NULL
{
	UINT32
		i;
	EXPRESSIONNODE
		*theNode;

	if((theNode=REAllocateNode()))
	{
		theNode->theType=PT_RIGHTEDGE;					// create a node, and apply the correct type to it
		theNode->parentExpression=theParent;			// set the parent
		theNode->matchSpecified=false;					// no match count specified for this node yet
		theNode->matchMin=1;							// until further notice, this must match exactly once
		theNode->matchMax=1;
		theNode->nextExpression=NULL;					// this is now the last expression

		for(i=0;i<BITTABLELENGTH;i++)					// clear the array
		{
			theNode->pt.theBits[i]=0;
		}
		for(i=0;i<256;i++)								// set the array to the current word list
		{
			if(wordSpaceTable[i])
			{
				ADDCHARBIT(&(theNode->pt.theBits[0]),i);
			}
		}

		if(currentNode)
		{
			currentNode->nextExpression=theNode;		// link to previous node if one existed
		}
		return(theNode);
	}
	return(NULL);
}

static EXPRESSIONNODE *REAddStartOfLineMatch(EXPRESSIONNODE *theParent,EXPRESSIONNODE *currentNode)
// link in a new node that matches the start of a line
// if there is a problem, SetError, and return NULL
{
	UINT32
		i;
	EXPRESSIONNODE
		*theNode;

	if((theNode=REAllocateNode()))
	{
		theNode->theType=PT_LEFTEDGE;					// create a node, and apply the correct type to it
		theNode->parentExpression=theParent;			// set the parent
		theNode->matchSpecified=false;					// no match count specified for this node yet
		theNode->matchMin=1;							// until further notice, this must match exactly once
		theNode->matchMax=1;
		theNode->nextExpression=NULL;					// this is now the last expression

		for(i=0;i<BITTABLELENGTH;i++)					// clear the array
		{
			theNode->pt.theBits[i]=0;
		}
		ADDCHARBIT(&(theNode->pt.theBits[0]),'\n');

		if(currentNode)
		{
			currentNode->nextExpression=theNode;		// link to previous node if one existed
		}
		return(theNode);
	}
	return(NULL);
}

static EXPRESSIONNODE *REAddEndOfLineMatch(EXPRESSIONNODE *theParent,EXPRESSIONNODE *currentNode)
// link in a new node that matches the end of a line
// if there is a problem, SetError, and return NULL
{
	UINT32
		i;
	EXPRESSIONNODE
		*theNode;

	if((theNode=REAllocateNode()))
	{
		theNode->theType=PT_RIGHTEDGE;					// create a node, and apply the correct type to it
		theNode->parentExpression=theParent;			// set the parent
		theNode->matchSpecified=false;					// no match count specified for this node yet
		theNode->matchMin=1;							// until further notice, this must match exactly once
		theNode->matchMax=1;
		theNode->nextExpression=NULL;					// this is now the last expression

		for(i=0;i<BITTABLELENGTH;i++)					// clear the array
		{
			theNode->pt.theBits[i]=0;
		}
		ADDCHARBIT(&(theNode->pt.theBits[0]),'\n');

		if(currentNode)
		{
			currentNode->nextExpression=theNode;		// link to previous node if one existed
		}
		return(theNode);
	}
	return(NULL);
}

static void REInhaleDigits(UINT8 *theString,UINT32 *index,UINT32 numBytes,UINT32 *theResult)
// inhale as many digits as possible from theString, starting at index, and possibly reading up to
// offset numBytes
// return theResult
// also return with index updated
// NOTE: it is ok for there to be no digits during the read, in this case
// theResult will return as 0, and index will not be changed
{
	UINT8
		theChar;

	*theResult=0;
	while(*index<numBytes&&(theChar=theString[*index])&&theChar>='0'&&theChar<='9')
	{
		*theResult=*theResult*10+(theChar-'0');			// add this digit to the result
		(*index)++;
	}
}

static bool REParseRange(UINT8 *theExpression,UINT32 *index,UINT32 numBytes,UINT32 *rangeLow,UINT32 *rangeHigh)
// repetition range operator has started, read in the ranges, and return them in
// rangeLow, and rangeHigh
// if there is a problem, SetError, and return false
{
	*rangeHigh=0;											// this means infinity
	REInhaleDigits(theExpression,index,numBytes,rangeLow);	// attempt to read the low number in the range
	if(*index<numBytes)										// see if any more characters (looking for , or })
	{
		switch(theExpression[(*index)++])
		{
			case ',':
				REInhaleDigits(theExpression,index,numBytes,rangeHigh);	// attempt to read the high number in the range
				if(*index<numBytes&&theExpression[(*index)++]=='}')		// see if any more characters (looking for })
				{
					if((*rangeHigh==0)||(*rangeHigh>=*rangeLow))			// make sure low is <= high
					{
						return(true);
					}
				}
				SetError(localErrorFamily,errorMembers[BADRANGEEXPRESSION],errorDescriptions[BADRANGEEXPRESSION]);
				break;
			case '}':
				if((*rangeHigh=*rangeLow))					// range with just one number means exactly that many times (0 is invalid)
				{
					return(true);
				}
				else
				{
					SetError(localErrorFamily,errorMembers[BADRANGEEXPRESSION],errorDescriptions[BADRANGEEXPRESSION]);
				}
				break;
			default:
				SetError(localErrorFamily,errorMembers[BADRANGEEXPRESSION],errorDescriptions[BADRANGEEXPRESSION]);
				break;
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[BADRANGEEXPRESSION],errorDescriptions[BADRANGEEXPRESSION]);
	}
	return(false);
}

// this is called recursively from below, so we must prototype it
static EXPRESSIONLISTHEADER *REParseSubExpressionList(COMPILEDEXPRESSION *theCompiledExpression,EXPRESSIONNODE *theParent,UINT8 *theExpression,UINT32 *index,UINT32 numBytes,UINT32 currentRegister,const UINT8 *translateTable);	// prototype this to keep compiler happy

static EXPRESSIONNODE *REAddSubExpression(COMPILEDEXPRESSION *theCompiledExpression,EXPRESSIONNODE *theParent,EXPRESSIONNODE *currentNode,UINT8 *theExpression,UINT32 *index,UINT32 numBytes,const UINT8 *translateTable)
// create a subexpression node, and build the subexpression into it
// if there is a problem, SetError, and return NULL
{
	UINT32
		currentRegister;
	EXPRESSIONNODE
		*startNode,
		*endNode;

	if((startNode=REAllocateNode()))
	{
		startNode->theType=PT_SUBEXPRESSIONSTART;			// create a start node, and apply the correct type to it
		startNode->parentExpression=theParent;
		startNode->matchSpecified=false;					// no match count specified for this node yet (none ever will be)
		startNode->matchMin=1;								// set to some default values (never looked at)
		startNode->matchMax=1;
		if((startNode->nextExpression=endNode=REAllocateNode()))
		{
			endNode->theType=PT_SUBEXPRESSIONEND;			// create a node, and apply the correct type to it
			endNode->parentExpression=theParent;
			endNode->matchSpecified=false;					// no match count specified for this node yet
			endNode->matchMin=1;							// until further notice, this must match exactly once
			endNode->matchMax=1;
			endNode->nextExpression=NULL;					// this is now the last expression

			currentRegister=theCompiledExpression->numRegisters;
			if(currentRegister<MAXREGISTERS)				// see if there is a register
			{
				endNode->pt.se.theRegister=&(theCompiledExpression->theRegisters[currentRegister]);		// remember the register
				theCompiledExpression->numRegisters++;		// increment the number of registers in use
			}
			else
			{
				endNode->pt.se.theRegister=NULL;			// no register for this one
			}

			if((endNode->pt.se.subExpressionList=REParseSubExpressionList(theCompiledExpression,endNode,theExpression,index,numBytes,currentRegister,translateTable)))		// get subexpression
			{
				if(currentNode)
				{
					currentNode->nextExpression=startNode;		// link to previous node if one existed
				}
				return(startNode);
			}
			REFreeNode(endNode);
		}
		REFreeNode(startNode);
	}
	return(NULL);
}

static void REFreeLists(EXPRESSIONLISTHEADER *theList)
// given a pointer to the top of an expression list, free
// all subnodes, and all alternates below
// then free theList
// NOTE: this calls itself recursively for sublists
{
	EXPRESSIONLISTHEADER
		*currentList,
		*nextAlternate;
	EXPRESSIONNODE
		*currentNode,
		*nextNode;

	currentList=theList;
	while(currentList)
	{
		nextAlternate=currentList->nextAlternate;
		currentNode=currentList->firstExpression;
		while(currentNode)
		{
			nextNode=currentNode->nextExpression;
			if(currentNode->theType==PT_SUBEXPRESSIONEND)		// free sub-list
			{
				REFreeLists(currentNode->pt.se.subExpressionList);		// free all the nodes of this subexpression
			}
			REFreeNode(currentNode);
			currentNode=nextNode;								// move to the next horizontally
		}
		REFreeList(currentList);
		currentList=nextAlternate;
	}
}

static EXPRESSIONLISTHEADER *REParseSubExpressionList(COMPILEDEXPRESSION *theCompiledExpression,EXPRESSIONNODE *theParent,UINT8 *theExpression,UINT32 *index,UINT32 numBytes,UINT32 currentRegister,const UINT8 *translateTable)
// parse theExpression, and build lists of nodes for theCompiledExpression
// (if theCompiledExpression is NULL, then it is assumed that we are compiling a sublist)
// return the head of the created list of expression nodes
// if there is a problem, SetError, and return NULL
{
	EXPRESSIONLISTHEADER
		*startList,
		*currentList;						// current list in the list of alternates
	EXPRESSIONNODE
		*startNode,
		*currentNode;
	UINT8
		theChar;
	UINT32
		theRegisterIndex;
	bool
		done,
		fail;

	fail=done=false;
	if((startList=REAllocateList()))
	{
		startList->parentExpression=theParent;
		startList->firstExpression=NULL;
		startList->nextAlternate=NULL;
		currentList=startList;							// point to the current list that is being built
		startNode=currentNode=NULL;
		while(!fail&&!done&&((*index)<numBytes))
		{
			switch(theChar=theExpression[(*index)++])
			{
				case '(':
					fail=!((currentNode=REAddSubExpression(theCompiledExpression,theParent,currentNode,theExpression,index,numBytes,translateTable)));
					break;
				case ')':
					if(theParent)						// if not at the top, then take this as the end of our group
					{
						if(currentRegister<MAXREGISTERS)	// define this register now
						{
							theCompiledExpression->registerDefined[currentRegister]=true;
						}
						done=true;
					}
					else
					{
						SetError(localErrorFamily,errorMembers[UNBALANCEDPAREN],errorDescriptions[UNBALANCEDPAREN]);
						fail=true;
					}
					break;
				case '{':
					if(currentNode&&!currentNode->matchSpecified&&currentNode->theType!=PT_LEFTEDGE&&currentNode->theType!=PT_RIGHTEDGE)
					{
						currentNode->matchSpecified=true;	// match has been specified
						fail=!REParseRange(theExpression,index,numBytes,&currentNode->matchMin,&currentNode->matchMax);
					}
					else
					{
						SetError(localErrorFamily,errorMembers[BADREPETITION],errorDescriptions[BADREPETITION]);
						fail=true;
					}
					break;
				case '*':								// match 0 or more
					if(currentNode&&!currentNode->matchSpecified&&currentNode->theType!=PT_LEFTEDGE&&currentNode->theType!=PT_RIGHTEDGE)
					{
						currentNode->matchSpecified=true;	// match has been specified
						currentNode->matchMin=0;		// no requirement to match any
						currentNode->matchMax=0;		// match as many as possible
					}
					else
					{
						SetError(localErrorFamily,errorMembers[BADREPETITION],errorDescriptions[BADREPETITION]);
						fail=true;
					}
					break;
				case '+':								// match 1 or more
					if(currentNode&&!currentNode->matchSpecified&&currentNode->theType!=PT_LEFTEDGE&&currentNode->theType!=PT_RIGHTEDGE)
					{
						currentNode->matchSpecified=true;	// match has been specified
						currentNode->matchMin=1;		// match at least 1
						currentNode->matchMax=0;		// match as many as possible
					}
					else
					{
						SetError(localErrorFamily,errorMembers[BADREPETITION],errorDescriptions[BADREPETITION]);
						fail=true;
					}
					break;
				case '?':								// match 0 or 1
					if(currentNode&&!currentNode->matchSpecified&&currentNode->theType!=PT_LEFTEDGE&&currentNode->theType!=PT_RIGHTEDGE)
					{
						currentNode->matchSpecified=true;	// match has been specified
						currentNode->matchMin=0;		// no requirement to match any
						currentNode->matchMax=1;		// match at most 1
					}
					else
					{
						SetError(localErrorFamily,errorMembers[BADREPETITION],errorDescriptions[BADREPETITION]);
						fail=true;
					}
					break;
				case '[':
					fail=!(currentNode=REAddList(theParent,currentNode,theExpression,index,numBytes,translateTable));
					break;
				case '|':
					if((currentList->nextAlternate=REAllocateList()))	// create a new list, link it to the old, and make it current
					{
						currentList=currentList->nextAlternate;
						currentList->parentExpression=theParent;
						currentList->nextAlternate=NULL;
						currentList->firstExpression=NULL;
						currentNode=NULL;				// no longer in the middle of a node
					}
					else
					{
						fail=true;
					}
					break;
				case '.':
					fail=!(currentNode=REAddAny(theParent,currentNode,theExpression,index,numBytes));
					break;
				case '<':
					fail=!(currentNode=REAddStartOfWordMatch(theParent,currentNode));
					break;
				case '>':
					fail=!(currentNode=REAddEndOfWordMatch(theParent,currentNode));
					break;
				case '^':
					fail=!(currentNode=REAddStartOfLineMatch(theParent,currentNode));
					break;
				case '$':
					fail=!(currentNode=REAddEndOfLineMatch(theParent,currentNode));
					break;
				case '\\':
					if(*index<=numBytes)
					{
						theChar=theExpression[*index];
						if(theChar>='0'&&theChar<='9')									// see if sub pattern match
						{
							(*index)++;
							theRegisterIndex=theChar-'0';								// make ASCII digit into binary number
							if(theRegisterIndex<MAXREGISTERS&&theCompiledExpression->registerDefined[theRegisterIndex])	// make sure it is in range, and that the given register has already been defined (this will guarantee that it is matched before we need it)
							{
								fail=!(currentNode=REAddPreviousMatch(theParent,currentNode,&theCompiledExpression->theRegisters[theRegisterIndex]));
							}
							else
							{
								SetError(localErrorFamily,errorMembers[INVALIDREGISTER],errorDescriptions[INVALIDREGISTER]);
								fail=true;
							}
						}
						else
						{
							if(REConvertQuotedCharacter(theExpression,index,numBytes,&theChar))
							{
								fail=!(currentNode=REAddString(theParent,currentNode,theChar,translateTable));
							}
							else
							{
								fail=true;
							}
						}
					}
					else
					{
						SetError(localErrorFamily,errorMembers[DANGLINGQUOTE],errorDescriptions[DANGLINGQUOTE]);
						fail=true;
					}
					break;
				default:
					fail=!(currentNode=REAddString(theParent,currentNode,theChar,translateTable));	// just take the character as part of a string
					break;
			}
			if(currentNode)
			{
				if(!currentList->firstExpression)
				{
					currentList->firstExpression=currentNode;
				}
				if(currentNode->theType==PT_SUBEXPRESSIONSTART)							// if just had start of expression, move to the end
				{
					currentNode=currentNode->nextExpression;
				}
			}
		}
		if(!fail&&!done&&theParent)														// reached the end, but in middle of group somewhere
		{
			SetError(localErrorFamily,errorMembers[UNBALANCEDPAREN],errorDescriptions[UNBALANCEDPAREN]);
			fail=true;
		}
		if(!fail)
		{
			return(startList);
		}
		REFreeLists(startList);
	}
	return(NULL);
}

void REFree(COMPILEDEXPRESSION *theExpression)
// free all nodes used by theExpression
// this must be called for every call to RECompile to deallocate the
// memory used by theExpression
{
	REFreeLists(theExpression->firstList);											// free the lists
	MDisposePtr(theExpression);														// remove the expression header
}

static void MoveOver(EXPRESSIONLISTHEADER *theList,UINT32 depth)
// move over by depth spaces
{
	UINT32
		i;

	for(i=0;i<depth;i++)
	{
		printf(" ");
	}
}

static void PrintNode(EXPRESSIONLISTHEADER *theList,EXPRESSIONNODE *theNode,UINT32 depth)
// print formatted node information
{
	UINT32
		i;

	switch(theNode->theType)
	{
		case PT_STRING:
			MoveOver(theList,depth);
			printf("STRING %d/%d '",(int)theNode->matchMin,(int)theNode->matchMax);
			for(i=0;i<theNode->dataLength;i++)
			{
				printf("%c",theNode->pt.theData[i]);
			}
			printf("'\n");
			break;
		case PT_LIST:
			MoveOver(theList,depth);
			printf("LIST %d/%d ",(int)theNode->matchMin,(int)theNode->matchMax);
			for(i=0;i<BITTABLELENGTH;i++)
			{
				printf("%02X ",theNode->pt.theBits[i]);
			}
			printf("\n");
			break;
		case PT_PREVIOUSMATCH:
			MoveOver(theList,depth);
			printf("PREVIOUSMATCH %d/%d\n",(int)theNode->matchMin,(int)theNode->matchMax);
			break;
		case PT_SUBEXPRESSIONSTART:
			MoveOver(theList,depth);
			printf("SUBEXPRESSIONSTART %d/%d\n",(int)theNode->matchMin,(int)theNode->matchMax);
			break;
		case PT_SUBEXPRESSIONEND:
			MoveOver(theList,depth);
			printf("SUBEXPRESSIONEND %d/%d\n",(int)theNode->matchMin,(int)theNode->matchMax);
			break;
		case PT_LEFTEDGE:
			MoveOver(theList,depth);
			printf("LEFTEDGE %d/%d\n",(int)theNode->matchMin,(int)theNode->matchMax);
			break;
		case PT_RIGHTEDGE:
			MoveOver(theList,depth);
			printf("RIGHTEDGE %d/%d\n",(int)theNode->matchMin,(int)theNode->matchMax);
			break;
		default:
			MoveOver(theList,depth);
			printf("????? (%d)\n",(int)theNode->theType);
			break;
	}
}

static void PrintExpressionList(EXPRESSIONLISTHEADER *theList,UINT32 depth)
// dump debugging output of the compiled expression
// NOTE: this calls itself
{
	EXPRESSIONLISTHEADER
		*currentList;
	EXPRESSIONNODE
		*currentNode;

	currentList=theList;
	while(currentList)
	{
		currentNode=currentList->firstExpression;
		while(currentNode)
		{
			if(currentNode->theType==PT_SUBEXPRESSIONEND)
			{
				PrintExpressionList(currentNode->pt.se.subExpressionList,depth+1);		// print the sublist
			}
			PrintNode(currentList,currentNode,depth);
			currentNode=currentNode->nextExpression;
		}
		currentList=currentList->nextAlternate;
	}
}

void REPrintCompiledExpression(COMPILEDEXPRESSION *theExpression)
// dump debugging output of the compiled expression
{
	UINT32
		i;

	PrintExpressionList(theExpression->firstList,0);
	if(theExpression->infiniteMatchLength)
	{
		printf("Max match is infinite\n");
	}
	else
	{
		printf("Max match length=%d\n",theExpression->maximumMatchLength);
	}
	printf("Min match length=%d\n",theExpression->minimumMatchLength);
	printf("Matching start chars=");
	for(i=0;i<256;i++)
	{
		printf("%02X ",theExpression->startCharsMatched[i]);
	}
	printf("\n");
	printf("Matching chars=");
	for(i=0;i<256;i++)
	{
		printf("%02X ",theExpression->charsMatched[i]);
	}
	printf("\n");
}

static void AnalyzeExpressionList(COMPILEDEXPRESSION *theExpression,EXPRESSIONLISTHEADER *theList,UINT32 *minLength,UINT32 *maxLength,bool *infiniteLength,bool haveStartChar)
// Analyze the given expression list, recursively calling this routine as needed
{
	EXPRESSIONLISTHEADER
		*currentList;
	EXPRESSIONNODE
		*currentNode;
	UINT32
		localMinLength,
		localMaxLength,
		alternateMinLength,
		alternateMaxLength,
		subExpressionMinLength,
		subExpressionMaxLength;
	bool
		subExpressionInfiniteLength;
	UINT32
		i;
	bool
		firstAlternate;

	currentList=theList;
	firstAlternate=true;

	localMinLength=0;
	localMaxLength=0;

	while(currentList)
	{
		alternateMinLength=0;
		alternateMaxLength=0;
		currentNode=currentList->firstExpression;
		while(currentNode)
		{
			switch(currentNode->theType)
			{
				case PT_STRING:
					if((!haveStartChar)&&(alternateMinLength==0))	// if at start character of expression, add character to list of possible start characters
					{
						theExpression->startCharsMatched[currentNode->pt.theData[0]]=true;
					}
					alternateMinLength+=(currentNode->dataLength-1)+currentNode->matchMin;	// last character must occur this many times
					if(currentNode->matchMax)
					{
						alternateMaxLength+=currentNode->dataLength+(currentNode->matchMax-1);	// last character gets repeated
					}
					else
					{
						*infiniteLength=true;
					}
					for(i=0;i<currentNode->dataLength;i++)
					{
						theExpression->charsMatched[currentNode->pt.theData[i]]=true;
					}
					break;
				case PT_LIST:
					if((!haveStartChar)&&(alternateMinLength==0))	// if at start character of expression, add character to list of possible start characters
					{
						for(i=0;i<256;i++)
						{
							if(GETCHARBIT(&currentNode->pt.theBits[0],i))
							{
								theExpression->startCharsMatched[i]=true;
							}
						}
					}
					alternateMinLength+=currentNode->matchMin;
					if(currentNode->matchMax)
					{
						alternateMaxLength+=currentNode->matchMax;
					}
					else
					{
						*infiniteLength=true;
					}
					for(i=0;i<256;i++)
					{
						if(GETCHARBIT(&currentNode->pt.theBits[0],i))
						{
							theExpression->charsMatched[i]=true;
						}
					}
					break;
				case PT_PREVIOUSMATCH:
					alternateMinLength+=currentNode->matchMin*currentNode->pt.theRegister->minimumMatchLength;
					if(currentNode->matchMax)
					{
						*infiniteLength|=currentNode->pt.theRegister->infiniteMatchLength;
						alternateMaxLength+=currentNode->matchMax*currentNode->pt.theRegister->maximumMatchLength;
					}
					else
					{
						*infiniteLength=true;
					}
					break;
				case PT_SUBEXPRESSIONSTART:
					break;
				case PT_SUBEXPRESSIONEND:
					subExpressionMinLength=0;
					subExpressionMaxLength=0;
					subExpressionInfiniteLength=false;
					AnalyzeExpressionList(theExpression,currentNode->pt.se.subExpressionList,&subExpressionMinLength,&subExpressionMaxLength,&subExpressionInfiniteLength,(haveStartChar||alternateMinLength>0));	// analyze the sublist
					if(currentNode->pt.se.theRegister)
					{
						currentNode->pt.se.theRegister->minimumMatchLength=subExpressionMinLength;	// remember the min length for this sub-expression
						currentNode->pt.se.theRegister->maximumMatchLength=subExpressionMaxLength;	// remember the max length for this sub-expression
						currentNode->pt.se.theRegister->infiniteMatchLength=subExpressionInfiniteLength;			// remember if this could match infinitely
					}
					alternateMinLength+=currentNode->matchMin*subExpressionMinLength;
					if(currentNode->matchMax&&!subExpressionInfiniteLength)
					{
						alternateMaxLength+=currentNode->matchMax*subExpressionMaxLength;
					}
					else
					{
						*infiniteLength=true;
					}
					break;
				case PT_LEFTEDGE:
					break;
				case PT_RIGHTEDGE:
					break;
				default:
					break;
			}
			currentNode=currentNode->nextExpression;
		}

		if(firstAlternate||alternateMinLength<localMinLength)
		{
			localMinLength=alternateMinLength;
		}
		if(alternateMaxLength>localMaxLength)
		{
			localMaxLength=alternateMaxLength;
		}
		firstAlternate=false;

		currentList=currentList->nextAlternate;
	}

	*minLength+=localMinLength;
	*maxLength+=localMaxLength;
}

static void AnalyzeExpression(COMPILEDEXPRESSION *theExpression)
// Work out which characters can possibly occur in theExpression.
// Also, work out the maximum length of a match of theExpression.
// NOTE: if the maximum match length is infinite, set maximumMatchLength to 0.
{
	int
		i;

	for(i=0;i<256;i++)
	{
		theExpression->charsMatched[i]=false;				// clear the bit table of possible matches
		theExpression->startCharsMatched[i]=false;			// clear the bit table of possible matches at the start
	}
	theExpression->minimumMatchLength=0;
	theExpression->maximumMatchLength=0;
	theExpression->infiniteMatchLength=false;
	AnalyzeExpressionList(theExpression,theExpression->firstList,&theExpression->minimumMatchLength,&theExpression->maximumMatchLength,&theExpression->infiniteMatchLength,false);	// recursively analyze the lists
}

COMPILEDEXPRESSION *RECompile(UINT8 *theExpression,UINT32 numBytes,const UINT8 *translateTable)
// compile numBytes of theExpression, and return a pointer to the
// node of the start of the expression.
// if there is a problem, call SetError, and return NULL
{
	COMPILEDEXPRESSION
		*theCompiledExpression;
	UINT32
		i,
		index;

	if((theCompiledExpression=(COMPILEDEXPRESSION *)MNewPtr(sizeof(COMPILEDEXPRESSION))))
	{
		theCompiledExpression->numRegisters=0;						// no registers encountered yet
		for(i=0;i<MAXREGISTERS;i++)
		{
			theCompiledExpression->registerDefined[i]=false;		// no registers have been defined yet
		}
		index=0;
		if((theCompiledExpression->firstList=REParseSubExpressionList(theCompiledExpression,NULL,theExpression,&index,numBytes,0,translateTable)))
		{
			AnalyzeExpression(theCompiledExpression);				// look at the expression, and work out various pieces of information which can be used to determine how far back one needs to go from a text change to locate an expression which includes the change
//			REPrintCompiledExpression(theCompiledExpression);		// dump out the expression
			return(theCompiledExpression);
		}
		MDisposePtr(theCompiledExpression);
	}
	return(NULL);
}

// MATCHING FUNCTIONS -----------------------------------------------------------------------------------------------------------------

static UINT32
	matchRecursionDepth;					// used to keep track of how far we are recursing during match attempts, allows us to fail without running out of stack

static EXPRESSIONNODE *REGetNextNode(EXPRESSIONNODE *theNode)
// get the next expression node at this level, or move up a level if possible
{
	EXPRESSIONNODE
		*newNode;

	if(!(newNode=theNode->nextExpression))
	{
		newNode=theNode->parentExpression;
	}
	return(newNode);
}

// these are called recursively from below, so they must be prototyped
static bool REMatchAlternatives(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONLISTHEADER *theList,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchStringNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchStringNodeTT(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchListNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchListNodeTT(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchPreviousNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchPreviousNodeTT(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchSubStartNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchSubEndNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchLeftEdgeNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);
static bool REMatchRightEdgeNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals);

// array of pointers to exact matching routines
static bool
	(*exactRoutines[])(CHUNKHEADER *,UINT32,UINT32,EXPRESSIONNODE *,bool *,UINT32 *,RECURSEGLOBALS *)=
	{
		REMatchStringNode,
		REMatchListNode,
		REMatchPreviousNode,
		REMatchSubStartNode,
		REMatchSubEndNode,
		REMatchLeftEdgeNode,
		REMatchRightEdgeNode
	};

// array of pointers to translated matching routines
static bool
	(*translatedRoutines[])(CHUNKHEADER *,UINT32,UINT32,EXPRESSIONNODE *,bool *,UINT32 *,RECURSEGLOBALS *)=
	{
		REMatchStringNodeTT,
		REMatchListNodeTT,
		REMatchPreviousNodeTT,
		REMatchSubStartNode,
		REMatchSubEndNode,
		REMatchLeftEdgeNode,
		REMatchRightEdgeNode
	};

// this points to the array that tells what routines to call for each node type
static bool
	(**matchRoutines)(CHUNKHEADER *,UINT32,UINT32,EXPRESSIONNODE *,bool *,UINT32 *,RECURSEGLOBALS *);

// points to the translation table to use during translated compares
static const UINT8
	*theTranslationTable;

// macro function that branches to next match routine from current one
#define	RECallNextNode(theChunk,theOffset,numToSearch,theNode,foundMatch,numMatched,recurseGlobals)						\
{																														\
	fail=!matchRoutines[theNode->theType](theChunk,theOffset,numToSearch,theNode,foundMatch,numMatched,recurseGlobals);	\
}

static bool REMatchStringNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// attempt to match theNode with the data at theChunk/theOffset
// if the pattern could be matched, return true in foundMatch with numMatched as the number of bytes matched
// if no match was located, return false in foundMatch
// NOTE: do not search through more than numToSearch bytes
// endChunk/endOffset will point to the end of the match if one was located
// if there is some sort of hard failure, this routine will SetError, and return false
{
	CHUNKHEADER
		*lastChunk;												// need to remember this in case we fall off the edge
	UINT32
		i,
		nodeMatched;
	bool
		fail,
		matching;
	UINT8
		theChar;
	EXPRESSIONNODE
		*nextExpression;

	lastChunk=NULL;
	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=false;
		matching=(theNode->dataLength-1<=(int)numToSearch);		// make sure there are enough bytes to even allow a match
		for(i=0;matching&&((int)i<theNode->dataLength-1);i++)	// match the constant part of the string (if one exists)
		{
			matching=(theChunk->data[theOffset]==theNode->pt.theData[i]);	// check for match
			if((++theOffset)>=theChunk->totalBytes)				// move forward in the input
			{
				lastChunk=theChunk;
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
		if(matching)											// make sure we made it through the first part
		{
			numToSearch-=(theNode->dataLength-1);				// matched the constant part of the string, now try to get up to matchMax of last character
			theChar=theNode->pt.theData[theNode->dataLength-1];
			i=0;
			while(matching&&(i<numToSearch)&&((!theNode->matchMax)||(i<theNode->matchMax)))	// read as many as possible until we hit max, or fail
			{
				if((matching=(theChunk->data[theOffset]==theChar)))	// check for match
				{
					if((++theOffset)>=theChunk->totalBytes)			// move forward in the input iff there was a match
					{
						lastChunk=theChunk;
						theChunk=theChunk->nextHeader;
						theOffset=0;
					}
					i++;
				}
			}
			if(i>=theNode->matchMin)							// see if we got the minimum number of matches
			{
				if((nextExpression=REGetNextNode(theNode)))		// see if there is another node after this one (if so, recurse into it)
				{
					RECallNextNode(theChunk,theOffset,numToSearch-i,nextExpression,&matching,&nodeMatched,recurseGlobals);
					while(!fail&&!matching&&i>theNode->matchMin)	// see if possible to backup and try again
					{
						i--;
						if(theChunk)							// back up 1 character
						{
							if(theOffset)
							{
								theOffset--;
							}
							else
							{
								theChunk=theChunk->previousHeader;
								theOffset=theChunk->totalBytes-1;
							}
						}
						else
						{
							theChunk=lastChunk;
							theOffset=theChunk->totalBytes-1;
						}
						RECallNextNode(theChunk,theOffset,numToSearch-i,nextExpression,&matching,&nodeMatched,recurseGlobals);
					}
					if(!fail&&matching)							// match was located!!
					{
						(*numMatched)=theNode->dataLength-1+i+nodeMatched;		// return number of characters that matched
					}
				}
				else											// no other nodes, this was at the end, so it is done
				{
					(*numMatched)=theNode->dataLength-1+i;		// return number of characters that matched
					recurseGlobals->endChunk=theChunk;
					recurseGlobals->endOffset=theOffset;
					matching=true;								// if no more nodes, then this list is complete, and it matched
				}
			}
			else
			{
				matching=false;									// did not find minimum number of matches, so this fails here and now
			}
		}
		(*foundMatch)=matching;
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatchStringNodeTT(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// Exactly like REMatchStringNode except that this one uses a translation table during the search
{
	CHUNKHEADER
		*lastChunk;												// need to remember this in case we fall off the edge
	UINT32
		i,
		nodeMatched;
	bool
		fail,
		matching;
	UINT8
		theChar;
	EXPRESSIONNODE
		*nextExpression;

	lastChunk=NULL;
	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=false;
		matching=(theNode->dataLength-1<=(int)numToSearch);		// make sure there are enough bytes to even allow a match
		for(i=0;matching&&((int)i<theNode->dataLength-1);i++)	// match the constant part of the string (if one exists)
		{
			matching=(theTranslationTable[theChunk->data[theOffset]]==theTranslationTable[theNode->pt.theData[i]]);	// check for match
			if((++theOffset)>=theChunk->totalBytes)				// move forward in the input
			{
				lastChunk=theChunk;
				theChunk=theChunk->nextHeader;
				theOffset=0;
			}
		}
		if(matching)											// make sure we made it through the first part
		{
			numToSearch-=(theNode->dataLength-1);				// matched the constant part of the string, now try to get up to matchMax of last character
			theChar=theTranslationTable[theNode->pt.theData[theNode->dataLength-1]];
			i=0;
			while(matching&&(i<numToSearch)&&((!theNode->matchMax)||(i<theNode->matchMax)))	// read as many as possible until we hit max, or fail
			{
				if((matching=(theTranslationTable[theChunk->data[theOffset]]==theChar)))	// check for match
				{
					if((++theOffset)>=theChunk->totalBytes)			// move forward in the input iff there was a match
					{
						lastChunk=theChunk;
						theChunk=theChunk->nextHeader;
						theOffset=0;
					}
					i++;
				}
			}
			if(i>=theNode->matchMin)							// see if we got the minimum number of matches
			{
				if((nextExpression=REGetNextNode(theNode)))		// see if there is another node after this one (if so, recurse into it)
				{
					RECallNextNode(theChunk,theOffset,numToSearch-i,nextExpression,&matching,&nodeMatched,recurseGlobals);
					while(!fail&&!matching&&i>theNode->matchMin)	// see if possible to backup and try again
					{
						i--;
						if(theChunk)							// back up 1 character
						{
							if(theOffset)
							{
								theOffset--;
							}
							else
							{
								theChunk=theChunk->previousHeader;
								theOffset=theChunk->totalBytes-1;
							}
						}
						else
						{
							theChunk=lastChunk;
							theOffset=theChunk->totalBytes-1;
						}
						RECallNextNode(theChunk,theOffset,numToSearch-i,nextExpression,&matching,&nodeMatched,recurseGlobals);
					}
					if(!fail&&matching)							// match was located!!
					{
						(*numMatched)=theNode->dataLength-1+i+nodeMatched;		// return number of characters that matched
					}
				}
				else											// no other nodes, this was at the end, so it is done
				{
					(*numMatched)=theNode->dataLength-1+i;		// return number of characters that matched
					recurseGlobals->endChunk=theChunk;
					recurseGlobals->endOffset=theOffset;
					matching=true;								// if no more nodes, then this list is complete, and it matched
				}
			}
			else
			{
				matching=false;									// did not find minimum number of matches, so this fails here and now
			}
		}
		(*foundMatch)=matching;
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatchListNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// attempt to match theNode with the data at theChunk/theOffset
// if the pattern could be matched, return true in foundMatch with numMatched as the number of bytes matched
// if no match was located, return false in foundMatch
// NOTE: do not search through more than numToSearch bytes
// endChunk/endOffset will point to the end of the match if one was located
// if there is some sort of hard failure, this routine will SetError, and return false
{
	CHUNKHEADER
		*lastChunk;												// need to remember this in case we fall off the edge
	UINT32
		i,
		nodeMatched;
	bool
		fail,
		matching;
	UINT8
		theChar;
	EXPRESSIONNODE
		*nextExpression;

	lastChunk=NULL;
	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=false;
		matching=true;											// start by assuming a match
		i=0;
		while(matching&&(i<numToSearch)&&((!theNode->matchMax)||(i<theNode->matchMax)))	// read as many as possible until we hit max, or fail
		{
			theChar=theChunk->data[theOffset];
			if((matching=(GETCHARBIT(theNode->pt.theBits,theChar)!=0)))	// check for match
			{
				if((++theOffset)>=theChunk->totalBytes)			// move forward in the input iff there was a match
				{
					lastChunk=theChunk;
					theChunk=theChunk->nextHeader;
					theOffset=0;
				}
				i++;
			}
		}
		if(i>=theNode->matchMin)							// see if we got the minimum number of matches
		{
			if((nextExpression=REGetNextNode(theNode)))		// see if there is another node after this one (if so, recurse into it)
			{
				RECallNextNode(theChunk,theOffset,numToSearch-i,nextExpression,&matching,&nodeMatched,recurseGlobals);
				while(!fail&&!matching&&i>theNode->matchMin)	// see if possible to backup and try again
				{
					i--;
					if(theChunk)							// back up 1 character
					{
						if(theOffset)
						{
							theOffset--;
						}
						else
						{
							theChunk=theChunk->previousHeader;
							theOffset=theChunk->totalBytes-1;
						}
					}
					else
					{
						theChunk=lastChunk;
						theOffset=theChunk->totalBytes-1;
					}
					RECallNextNode(theChunk,theOffset,numToSearch-i,nextExpression,&matching,&nodeMatched,recurseGlobals);
				}
				if(!fail&&matching)							// match was located!!
				{
					(*numMatched)=i+nodeMatched;			// return number of characters that matched
				}
			}
			else											// no other nodes, this was at the end, so it is done
			{
				(*numMatched)=i;							// return number of characters that matched
				recurseGlobals->endChunk=theChunk;
				recurseGlobals->endOffset=theOffset;
				matching=true;								// if no more nodes, then this list is complete, and it matched
			}
		}
		else
		{
			matching=false;									// did not find minimum number of matches, so this fails here and now
		}
		(*foundMatch)=matching;
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatchListNodeTT(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// exactly like REMatchListNode but uses a translation table during the compare
{
	CHUNKHEADER
		*lastChunk;												// need to remember this in case we fall off the edge
	UINT32
		i,
		nodeMatched;
	bool
		fail,
		matching;
	UINT8
		theChar;
	EXPRESSIONNODE
		*nextExpression;

	lastChunk=NULL;
	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=false;
		matching=true;											// start by assuming a match
		i=0;
		while(matching&&(i<numToSearch)&&((!theNode->matchMax)||(i<theNode->matchMax)))	// read as many as possible until we hit max, or fail
		{
			theChar=theTranslationTable[theChunk->data[theOffset]];
			if((matching=(GETCHARBIT(theNode->pt.theBits,theChar)!=0)))	// check for match
			{
				if((++theOffset)>=theChunk->totalBytes)			// move forward in the input iff there was a match
				{
					lastChunk=theChunk;
					theChunk=theChunk->nextHeader;
					theOffset=0;
				}
				i++;
			}
		}
		if(i>=theNode->matchMin)							// see if we got the minimum number of matches
		{
			if((nextExpression=REGetNextNode(theNode)))		// see if there is another node after this one (if so, recurse into it)
			{
				RECallNextNode(theChunk,theOffset,numToSearch-i,nextExpression,&matching,&nodeMatched,recurseGlobals);
				while(!fail&&!matching&&i>theNode->matchMin)	// see if possible to backup and try again
				{
					i--;
					if(theChunk)							// back up 1 character
					{
						if(theOffset)
						{
							theOffset--;
						}
						else
						{
							theChunk=theChunk->previousHeader;
							theOffset=theChunk->totalBytes-1;
						}
					}
					else
					{
						theChunk=lastChunk;
						theOffset=theChunk->totalBytes-1;
					}
					RECallNextNode(theChunk,theOffset,numToSearch-i,nextExpression,&matching,&nodeMatched,recurseGlobals);
				}
				if(!fail&&matching)							// match was located!!
				{
					(*numMatched)=i+nodeMatched;			// return number of characters that matched
				}
			}
			else											// no other nodes, this was at the end, so it is done
			{
				(*numMatched)=i;							// return number of characters that matched
				recurseGlobals->endChunk=theChunk;
				recurseGlobals->endOffset=theOffset;
				matching=true;								// if no more nodes, then this list is complete, and it matched
			}
		}
		else
		{
			matching=false;									// did not find minimum number of matches, so this fails here and now
		}
		(*foundMatch)=matching;
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatchPreviousNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// attempt to match theNode with the data at theChunk/theOffset
// if the pattern could be matched, return true in foundMatch with numMatched as the number of bytes matched
// if no match was located, return false in foundMatch
// NOTE: do not search through more than numToSearch bytes
// endChunk/endOffset will point to the end of the match if one was located
// if there is some sort of hard failure, this routine will SetError, and return false
{
	UINT32
		i,
		totalMatched,
		nodeMatched;
	CHUNKHEADER
		*lastChunk;
	UINT32
		lastOffset;
	bool
		fail,
		matching;
	EXPRESSIONNODE
		*nextExpression;

	lastOffset=0;
	lastChunk=NULL;
	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=false;
		matching=true;											// start by assuming a match
		i=totalMatched=0;
		while(matching&&((!theNode->matchMax)||(i<theNode->matchMax)))	// match as many as possible until we hit max, or fail
		{
			if(theNode->pt.theRegister->theChunk&&numToSearch-totalMatched>=theNode->pt.theRegister->numBytes)	// see if it is defined, and enough bytes to test, if not, matching fails
			{
				if((matching=LiteralMatch(theChunk,theOffset,theNode->pt.theRegister->theChunk,theNode->pt.theRegister->theOffset,theNode->pt.theRegister->numBytes,&(recurseGlobals->endChunk),&(recurseGlobals->endOffset))))
				{
					i++;
					totalMatched+=theNode->pt.theRegister->numBytes;
					lastChunk=theChunk;							// keep these in case we step off the end
					lastOffset=theOffset;
					theChunk=recurseGlobals->endChunk;			// move to new position
					theOffset=recurseGlobals->endOffset;
				}
			}
			else
			{
				matching=false;
			}
		}
		if(i>=theNode->matchMin)							// see if we got the minimum number of matches
		{
			if((nextExpression=REGetNextNode(theNode)))		// see if there is another node after this one (if so, recurse into it)
			{
				RECallNextNode(theChunk,theOffset,numToSearch-totalMatched,nextExpression,&matching,&nodeMatched,recurseGlobals);
				while(!fail&&!matching&&i>theNode->matchMin)	// see if possible to backup and try again
				{
					i--;
					nodeMatched=theNode->pt.theRegister->numBytes;
					totalMatched-=nodeMatched;
					if(theChunk)							// back up nodeMatched characters
					{
						while(nodeMatched>theOffset)		// move back through the chunks until we use up all of the bytes matched
						{
							theChunk=theChunk->previousHeader;	// step back to the previous chunk
							nodeMatched-=theOffset;
							theOffset=theChunk->totalBytes;
						}
						theOffset-=nodeMatched;
					}
					else
					{
						theChunk=lastChunk;
						theOffset=lastOffset;
					}
					RECallNextNode(theChunk,theOffset,numToSearch-totalMatched,nextExpression,&matching,&nodeMatched,recurseGlobals);
				}
				if(!fail&&matching)							// match was located!!
				{
					(*numMatched)=totalMatched+nodeMatched;	// return number of characters that matched
				}
			}
			else											// no other nodes, this was at the end, so it is done
			{
				(*numMatched)=totalMatched;					// return number of characters that matched
				recurseGlobals->endChunk=theChunk;
				recurseGlobals->endOffset=theOffset;
				matching=true;								// if no more nodes, then this list is complete, and it matched
			}
		}
		else
		{
			matching=false;									// did not find minimum number of matches, so this fails here and now
		}
		(*foundMatch)=matching;
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatchPreviousNodeTT(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// exactly like REMatchPreviousNode except that this one uses a translation table
{
	UINT32
		i,
		totalMatched,
		nodeMatched;
	CHUNKHEADER
		*lastChunk;
	UINT32
		lastOffset;
	bool
		fail,
		matching;
	EXPRESSIONNODE
		*nextExpression;

	lastOffset=0;
	lastChunk=NULL;
	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=false;
		matching=true;											// start by assuming a match
		i=totalMatched=0;
		while(matching&&((!theNode->matchMax)||(i<theNode->matchMax)))	// match as many as possible until we hit max, or fail
		{
			if(theNode->pt.theRegister->theChunk&&numToSearch-totalMatched>=theNode->pt.theRegister->numBytes)	// see if it is defined, and enough bytes to test, if not, matching fails
			{
				if((matching=LiteralMatchTT(theChunk,theOffset,theNode->pt.theRegister->theChunk,theNode->pt.theRegister->theOffset,theNode->pt.theRegister->numBytes,theTranslationTable,&(recurseGlobals->endChunk),&(recurseGlobals->endOffset))))
				{
					i++;
					totalMatched+=theNode->pt.theRegister->numBytes;
					lastChunk=theChunk;							// keep these in case we step off the end
					lastOffset=theOffset;
					theChunk=recurseGlobals->endChunk;			// move to new position
					theOffset=recurseGlobals->endOffset;
				}
			}
			else
			{
				matching=false;
			}
		}
		if(i>=theNode->matchMin)							// see if we got the minimum number of matches
		{
			if((nextExpression=REGetNextNode(theNode)))		// see if there is another node after this one (if so, recurse into it)
			{
				RECallNextNode(theChunk,theOffset,numToSearch-totalMatched,nextExpression,&matching,&nodeMatched,recurseGlobals);
				while(!fail&&!matching&&i>theNode->matchMin)	// see if possible to backup and try again
				{
					i--;
					nodeMatched=theNode->pt.theRegister->numBytes;
					totalMatched-=nodeMatched;
					if(theChunk)							// back up nodeMatched characters
					{
						while(nodeMatched>theOffset)		// move back through the chunks until we use up all of the bytes matched
						{
							theChunk=theChunk->previousHeader;	// step back to the previous chunk
							nodeMatched-=theOffset;
							theOffset=theChunk->totalBytes;
						}
						theOffset-=nodeMatched;
					}
					else
					{
						theChunk=lastChunk;
						theOffset=lastOffset;
					}
					RECallNextNode(theChunk,theOffset,numToSearch-totalMatched,nextExpression,&matching,&nodeMatched,recurseGlobals);
				}
				if(!fail&&matching)							// match was located!!
				{
					(*numMatched)=totalMatched+nodeMatched;	// return number of characters that matched
				}
			}
			else											// no other nodes, this was at the end, so it is done
			{
				(*numMatched)=totalMatched;					// return number of characters that matched
				recurseGlobals->endChunk=theChunk;
				recurseGlobals->endOffset=theOffset;
				matching=true;								// if no more nodes, then this list is complete, and it matched
			}
		}
		else
		{
			matching=false;									// did not find minimum number of matches, so this fails here and now
		}
		(*foundMatch)=matching;
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatchSubStartNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// attempt to match theNode with the data at theChunk/theOffset
// if the pattern could be matched, return true in foundMatch with numMatched as the number of bytes matched
// if no match was located, return false in foundMatch
// NOTE: do not search through more than numToSearch bytes
// endChunk/endOffset will point to the end of the match if one was located
// if there is some sort of hard failure, this routine will SetError, and return false
{
	UINT32
		oldCount;
	bool
		fail;

	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=false;
		theNode->nextExpression->pt.se.justEntered=true;	// set up the end of the subexpression so that it knows we just hit the start
		oldCount=theNode->nextExpression->pt.se.currentCount;	// keep track of how many times this expression matched last time
		RECallNextNode(theChunk,theOffset,numToSearch,theNode->nextExpression,foundMatch,numMatched,recurseGlobals);
		theNode->nextExpression->pt.se.currentCount=oldCount;	// restore match count
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatchSubEndNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// attempt to match theNode with the data at theChunk/theOffset
// if the pattern could be matched, return true in foundMatch with numMatched as the number of bytes matched
// if no match was located, return false in foundMatch
// NOTE: do not search through more than numToSearch bytes
// endChunk/endOffset will point to the end of the match if one was located
// if there is some sort of hard failure, this routine will SetError, and return false
{
	bool
		fail,
		matching;
	EXPRESSIONNODE
		*nextExpression;
	REGISTER
		oldRegister;
	CHUNKHEADER
		*oldLastChunk;									// used to keep the contents of the old register in case we back up
	UINT32
		oldLastOffset,
		oldLastNumToSearch;

	oldLastOffset=0;
	oldLastNumToSearch=0;
	oldLastChunk=NULL;
	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=matching=false;
		if(theNode->pt.se.justEntered)
		{
			theNode->pt.se.currentCount=0;					// start off the counter
			theNode->pt.se.lastChunk=theChunk;				// remember where we were, so the register can be set if needed
			theNode->pt.se.lastOffset=theOffset;
			theNode->pt.se.lastNumToSearch=numToSearch;
			theNode->pt.se.justEntered=false;				// next time this is seen, we will know it came from a successful completion!
			if(REMatchAlternatives(theChunk,theOffset,numToSearch,theNode->pt.se.subExpressionList,&matching,numMatched,recurseGlobals))
			{
				if(!matching)								// if could not match, then see if we can move forward from here
				{
					if(!theNode->matchMin)					// if no matches needed, then we have success
					{
						if((nextExpression=REGetNextNode(theNode)))		// see if there is another node after this one (if so, recurse into it)
						{
							RECallNextNode(theChunk,theOffset,numToSearch,nextExpression,&matching,numMatched,recurseGlobals);
						}
						else
						{
							matching=true;
							*numMatched=0;					// this node is done here and now
							recurseGlobals->endChunk=theChunk;
							recurseGlobals->endOffset=theOffset;
						}
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
			theNode->pt.se.currentCount++;						// made another successful match
			if(theNode->pt.se.theRegister)						// see if this is one that is actually remembered
			{
				oldRegister.theChunk=theNode->pt.se.theRegister->theChunk;		// copy old register contents (in case we need to back out)
				oldRegister.theOffset=theNode->pt.se.theRegister->theOffset;
				oldRegister.matchIndex=theNode->pt.se.theRegister->matchIndex;
				oldRegister.numBytes=theNode->pt.se.theRegister->numBytes;

				oldLastChunk=theNode->pt.se.lastChunk;
				oldLastOffset=theNode->pt.se.lastOffset;
				oldLastNumToSearch=theNode->pt.se.lastNumToSearch;

				theNode->pt.se.theRegister->theChunk=theNode->pt.se.lastChunk;
				theNode->pt.se.theRegister->theOffset=theNode->pt.se.lastOffset;
				theNode->pt.se.theRegister->matchIndex=recurseGlobals->initialNumToSearch-theNode->pt.se.lastNumToSearch;
				theNode->pt.se.theRegister->numBytes=theNode->pt.se.lastNumToSearch-numToSearch;

				theNode->pt.se.lastChunk=theChunk;
				theNode->pt.se.lastOffset=theOffset;
				theNode->pt.se.lastNumToSearch=numToSearch;
			}
			if((!theNode->matchMax)||theNode->pt.se.currentCount<theNode->matchMax)
			{
				if(REMatchAlternatives(theChunk,theOffset,numToSearch,theNode->pt.se.subExpressionList,&matching,numMatched,recurseGlobals))
				{
					if(!matching)								// if could not match, then see if we can move forward from here
					{
						if(theNode->pt.se.currentCount>=theNode->matchMin)
						{
							if((nextExpression=REGetNextNode(theNode)))		// see if there is another node after this one (if so, recurse into it)
							{
								RECallNextNode(theChunk,theOffset,numToSearch,nextExpression,&matching,numMatched,recurseGlobals);
							}
							else
							{
								matching=true;
								*numMatched=0;					// this node is done here and now
								recurseGlobals->endChunk=theChunk;
								recurseGlobals->endOffset=theOffset;
							}
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
				if((nextExpression=REGetNextNode(theNode)))	// see if there is another node after this one (if so, recurse into it)
				{
					RECallNextNode(theChunk,theOffset,numToSearch,nextExpression,&matching,numMatched,recurseGlobals);
				}
				else
				{
					matching=true;
					*numMatched=0;
					recurseGlobals->endChunk=theChunk;
					recurseGlobals->endOffset=theOffset;
				}
			}
			if(!matching)
			{
				theNode->pt.se.currentCount--;				// this match failed, so back out

				if(theNode->pt.se.theRegister)				// see if this is one that is actually remembered
				{
					theNode->pt.se.lastChunk=oldLastChunk;
					theNode->pt.se.lastOffset=oldLastOffset;
					theNode->pt.se.lastNumToSearch=oldLastNumToSearch;

					theNode->pt.se.theRegister->theChunk=oldRegister.theChunk;
					theNode->pt.se.theRegister->theOffset=oldRegister.theOffset;
					theNode->pt.se.theRegister->matchIndex=oldRegister.matchIndex;
					theNode->pt.se.theRegister->numBytes=oldRegister.numBytes;
				}
			}
		}
		(*foundMatch)=matching;
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}


static bool REMatchLeftEdgeNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// attempt to match theNode with the data at theChunk/theOffset
// if the pattern could be matched, return true in foundMatch with numMatched as the number of bytes matched
// if no match was located, return false in foundMatch
// NOTE: do not search through more than numToSearch bytes
// endChunk/endOffset will point to the end of the match if one was located
// if there is some sort of hard failure, this routine will SetError, and return false
{
	CHUNKHEADER
		*lastChunk;												// need to remember this in case we fall off the edge
	UINT32
		lastOffset;
	bool
		fail,
		matching;
	UINT8
		theChar;
	EXPRESSIONNODE
		*nextExpression;

	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=false;
		matching=true;										// start by assuming a match
		(*numMatched)=0;									// return number of characters that matched
		recurseGlobals->endChunk=lastChunk=theChunk;		// return position of match
		recurseGlobals->endOffset=lastOffset=theOffset;
		if(lastOffset)										// back up from current position, and look for start character
		{
			lastOffset--;
		}
		else
		{
			if(lastChunk&&(lastChunk=lastChunk->previousHeader))
			{
				lastOffset=lastChunk->totalBytes-1;
			}
		}
		if(lastChunk)										// if no chunk back, then we were at the start (or end) of text
		{
			theChar=lastChunk->data[lastOffset];
			if(!GETCHARBIT(theNode->pt.theBits,theChar))	// see if not left edge character
			{
				if((!recurseGlobals->leftEdge)||(numToSearch<recurseGlobals->initialNumToSearch))	// make sure we have not been told that this is an edge
				{
					matching=false;
				}
			}
		}
		else
		{
			if(!recurseGlobals->leftEdge)					// if this is not an edge, then do not match it
			{
				matching=false;
			}
		}
		if(matching&&(nextExpression=REGetNextNode(theNode)))	// see if there is another node after this one (if so, recurse into it)
		{
			RECallNextNode(theChunk,theOffset,numToSearch,nextExpression,&matching,numMatched,recurseGlobals);
		}
		(*foundMatch)=matching;
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatchRightEdgeNode(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONNODE *theNode,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// attempt to match theNode with the data at theChunk/theOffset
// if the pattern could be matched, return true in foundMatch with numMatched as the number of bytes matched
// if no match was located, return false in foundMatch
// NOTE: do not search through more than numToSearch bytes
// endChunk/endOffset will point to the end of the match if one was located
// if there is some sort of hard failure, this routine will SetError, and return false
{
	bool
		fail,
		matching;
	UINT8
		theChar;
	EXPRESSIONNODE
		*nextExpression;

	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		fail=false;
		matching=true;										// start by assuming a match
		(*numMatched)=0;									// return number of characters that matched
		recurseGlobals->endChunk=theChunk;					// return position of match
		recurseGlobals->endOffset=theOffset;
		if(theChunk)										// if no chunk, then we are at the end of text
		{
			theChar=theChunk->data[theOffset];
			if(!GETCHARBIT(theNode->pt.theBits,theChar))		// see if not right edge character
			{
				if((!recurseGlobals->rightEdge)||numToSearch)	// make sure we have not been told that this is an edge
				{
					matching=false;
				}
			}
		}
		else
		{
			if(!recurseGlobals->rightEdge)					// if this is not an edge, then do not match it
			{
				matching=false;
			}
		}
		if(matching&&(nextExpression=REGetNextNode(theNode)))	// see if there is another node after this one (if so, recurse into it)
		{
			RECallNextNode(theChunk,theOffset,numToSearch,nextExpression,&matching,numMatched,recurseGlobals);
		}
		(*foundMatch)=matching;
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatchAlternatives(CHUNKHEADER *theChunk,UINT32 theOffset,UINT32 numToSearch,EXPRESSIONLISTHEADER *theList,bool *foundMatch,UINT32 *numMatched,RECURSEGLOBALS *recurseGlobals)
// attempt to match alternatives at the given level
// if the pattern could be matched, return true in foundMatch with numMatched as the number of bytes matched
// if no match was located, return false in foundMatch
// NOTE: do not search through more than numToSearch bytes
// endChunk/endOffset will point to the end of the match if one was located
// this tries each alternative in order until a match is located, or no match could be found
// if there is some sort of hard failure, this routine will SetError, and return false
{
	EXPRESSIONLISTHEADER
		*currentList;
	bool
		fail;

	if(++matchRecursionDepth<MAXRECURSIONDEPTH)
	{
		if(!recurseGlobals->checkAbort||!CheckAbort())
		{
			currentList=theList;
			(*foundMatch)=false;
			fail=false;
			while(currentList&&!(*foundMatch)&&!fail)
			{
				if(currentList->firstExpression)	// if non-empty list of expressions, go handle it
				{
					RECallNextNode(theChunk,theOffset,numToSearch,currentList->firstExpression,foundMatch,numMatched,recurseGlobals);
				}
				else	// list is empty, so it matches, and we continue with the parent's next node
				{
					if(currentList->parentExpression)				// go back to the parent
					{
						RECallNextNode(theChunk,theOffset,numToSearch,currentList->parentExpression,foundMatch,numMatched,recurseGlobals);
					}
					else											// no other nodes, this was at the end, so it is done
					{
						*numMatched=0;								// return number of characters that matched
						recurseGlobals->endChunk=theChunk;
						recurseGlobals->endOffset=theOffset;
						*foundMatch=true;							// if list is empty, then it matches
					}
				}
				currentList=currentList->nextAlternate;
			}
		}
		else
		{
			fail=true;								// abort happened (error was set by CheckAbort)
		}
		matchRecursionDepth--;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STACKOVERFLOW],errorDescriptions[STACKOVERFLOW]);
		fail=true;
	}
	return(!fail);
}

static bool REMatch(CHUNKHEADER *theChunk,UINT32 theOffset,COMPILEDEXPRESSION *theCompiledExpression,bool leftEdge,bool rightEdge,UINT32 numToSearch,bool *foundMatch,UINT32 *numMatched,CHUNKHEADER **endChunk,UINT32 *endOffset,bool allowAbort)
// this is the regular expression match routine
// it will return true in foundMatch if a match is found
// if there is a hard failure, SetError will be called, and false returned
// NOTE: this can be called when theChunk is NULL (for instance in the case
// when there is no text to search). It is even possible for this to find
// a match when theChunk is NULL (as in the case when the expression is '$')
// Of course, if theChunk is NULL, numToSearch must be 0
{
	RECURSEGLOBALS
		recurseGlobals;
	UINT32
		i;

	*foundMatch=false;
	for(i=0;i<theCompiledExpression->numRegisters;i++)
	{
		theCompiledExpression->theRegisters[i].theChunk=NULL;			// no registers have been matched yet
	}
	matchRecursionDepth=0;												// clear recursion depth

	// to save time/stack while calling recursively, this structure is used to hold variables that are global to the recursive search routines, and only a pointer to this is passed
	recurseGlobals.leftEdge=leftEdge;
	recurseGlobals.rightEdge=rightEdge;
	recurseGlobals.checkAbort=allowAbort;
	recurseGlobals.initialNumToSearch=numToSearch;
	recurseGlobals.endChunk=NULL;
	recurseGlobals.endOffset=0;

	if(REMatchAlternatives(theChunk,theOffset,numToSearch,theCompiledExpression->firstList,foundMatch,numMatched,&recurseGlobals))
	{
		*endChunk=recurseGlobals.endChunk;
		*endOffset=recurseGlobals.endOffset;

		return(true);
	}
	return(false);
}

bool GetRERegisterMatch(COMPILEDEXPRESSION *theCompiledExpression,UINT32 theRegisterIndex,UINT32 *matchOffset,UINT32 *matchLength)
// After a call to a regular expression search which succeeds, this
// may be called to locate information about the tagged matches
// If the passed tag did not match, set matchOffset to 0, matchLength to 0, and return false
{
	if(theRegisterIndex<theCompiledExpression->numRegisters)
	{
		if(theCompiledExpression->theRegisters[theRegisterIndex].theChunk)
		{
			*matchOffset=theCompiledExpression->theRegisters[theRegisterIndex].matchIndex;
			*matchLength=theCompiledExpression->theRegisters[theRegisterIndex].numBytes;
			return(true);
		}
	}
	*matchOffset=0;
	*matchLength=0;
	return(false);
}

bool GetRERegisterMatchChunkAndOffset(COMPILEDEXPRESSION *theCompiledExpression,UINT32 theRegisterIndex,CHUNKHEADER **theChunk,UINT32 *theOffset,UINT32 *matchLength)
// After a call to a regular expression search which succeeds, this
// may be called to locate information about the tagged matches
// If the passed tag did not match, set theChunk to NULL, theOffset to 0, matchLength to 0, and return false
{
	if(theRegisterIndex<theCompiledExpression->numRegisters)
	{
		if(theCompiledExpression->theRegisters[theRegisterIndex].theChunk)
		{
			*theChunk=theCompiledExpression->theRegisters[theRegisterIndex].theChunk;
			*theOffset=theCompiledExpression->theRegisters[theRegisterIndex].theOffset;
			*matchLength=theCompiledExpression->theRegisters[theRegisterIndex].numBytes;
			return(true);
		}
	}
	*theChunk=NULL;
	*theOffset=0;
	*matchLength=0;
	return(false);
}

bool SearchForwardRE(CHUNKHEADER *theChunk,UINT32 theOffset,COMPILEDEXPRESSION *theCompiledExpression,bool leftEdge,bool rightEdge,UINT32 numToSearchStart,UINT32 numToSearch,bool ignoreCase,bool *foundMatch,UINT32 *matchOffset,UINT32 *numMatched,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset,bool allowAbort)
// search for theCompiledExpression starting at theChunk, theOffset
// return true in foundMatch if it is found
// leftEdge tells if the starting point is considered a left edge for matching purposes
// rightEdge tells if the ending point is considered a right edge for matching purposes
// numToSearchStart gives the maximum number of bytes to look through trying to find the start of a match
// numToSearch gives the maximum number of bytes to look through
// if ignoreCase is true, search in a case-insensitve way
// matchOffset returns the offset from the start of the data where a match was located
// numMatched returns the number of characters that matched
// startChunk, startOffset give the address in the text which matched
// endChunk, endOffset give the address of the end of the match+1
// if allowAbort is true, then user abort will be checked for, and if it occurs, will be passed back as a hard error
// if there is a hard error, SetError will be called, and false returned
{
	bool
		fail;
	UINT32
		localOffset;
	bool
		localFound;

	localFound=false;
	localOffset=0;
	fail=false;
	if(ignoreCase)
	{
		matchRoutines=translatedRoutines;
		theTranslationTable=upperCaseTable;							// point at the table to use

		while(!fail&&!localFound&&localOffset<=numToSearchStart)	// '<=' because regular expressions can match 0 bytes
		{
			if(REMatch(theChunk,theOffset,theCompiledExpression,leftEdge,rightEdge,numToSearch-localOffset,&localFound,numMatched,endChunk,endOffset,allowAbort))
			{
				leftEdge=false;
				if(!localFound)
				{
					localOffset++;
					if(localOffset<=numToSearchStart)
					{
						if((++theOffset)>=theChunk->totalBytes)			// move forward in the input, and try a match there
						{
							theChunk=theChunk->nextHeader;
							theOffset=0;
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
	else		// this is the case which needs to be the fastest, since it is used most often (especially for syntax highlighting)
	{
		matchRoutines=exactRoutines;

		if(theCompiledExpression->minimumMatchLength)	// this case can be sped up
		{
			while(!fail&&!localFound&&localOffset<numToSearchStart)
			{
				if(theCompiledExpression->startCharsMatched[theChunk->data[theOffset]])	// do some quick tests to bail out now if possible
				{
					if(REMatch(theChunk,theOffset,theCompiledExpression,leftEdge,rightEdge,numToSearch-localOffset,&localFound,numMatched,endChunk,endOffset,allowAbort))
					{
						if(!localFound)
						{
							localOffset++;
							if((++theOffset)>=theChunk->totalBytes)			// move forward in the input, and try a match there
							{
								theChunk=theChunk->nextHeader;
								theOffset=0;
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
					localOffset++;
					if((++theOffset)>=theChunk->totalBytes)			// move forward in the input, and try a match there
					{
						theChunk=theChunk->nextHeader;
						theOffset=0;
					}
				}
				leftEdge=false;
			}
		}
		else
		{
			while(!fail&&!localFound&&localOffset<=numToSearchStart)	// '<=' because regular expressions can match 0 bytes
			{
				if(REMatch(theChunk,theOffset,theCompiledExpression,leftEdge,rightEdge,numToSearch-localOffset,&localFound,numMatched,endChunk,endOffset,allowAbort))
				{
					leftEdge=false;
					if(!localFound)
					{
						localOffset++;
						if(localOffset<=numToSearchStart)
						{
							if((++theOffset)>=theChunk->totalBytes)			// move forward in the input, and try a match there
							{
								theChunk=theChunk->nextHeader;
								theOffset=0;
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
	}
	*startChunk=theChunk;
	*startOffset=theOffset;
	*foundMatch=localFound;
	*matchOffset=localOffset;

	return(!fail);
}

bool SearchBackwardRE(CHUNKHEADER *theChunk,UINT32 theOffset,COMPILEDEXPRESSION *theCompiledExpression,bool leftEdge,bool rightEdge,UINT32 numToSearchStart,UINT32 numToSearch,bool ignoreCase,bool *foundMatch,UINT32 *matchOffset,UINT32 *numMatched,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset,bool allowAbort)
// search backward for the expression return true in foundMatch if it is found
// This repeatedly calls REMatch
// if there is a hard error, SetError will be called, and false returned
{
	bool
		fail;
	UINT32
		localOffset;
	bool
		localFound;

	localFound=false;
	localOffset=numToSearch-numToSearchStart;
	fail=false;
	if(ignoreCase)
	{
		matchRoutines=translatedRoutines;
		theTranslationTable=upperCaseTable;									// point at the table to use

		while(!fail&&!localFound&&localOffset<=numToSearch)					// '<=' because regular expressions can match 0 bytes
		{
			if(REMatch(theChunk,theOffset,theCompiledExpression,(localOffset==numToSearch)?leftEdge:false,rightEdge,localOffset,&localFound,numMatched,endChunk,endOffset,allowAbort))
			{
				if(!localFound)
				{
					localOffset++;
					if(localOffset<=numToSearch)
					{
						if(!theOffset)										// move backward in the input
						{
							theChunk=theChunk->previousHeader;
							theOffset=theChunk->totalBytes;
						}
						theOffset--;
					}
				}
			}
			else
			{
				fail=true;
			}
		}
	}
	else
	{
		matchRoutines=exactRoutines;

		while(!fail&&!localFound&&localOffset<=numToSearch)			// '<=' because regular expressions can match 0 bytes
		{
			if(!theCompiledExpression->minimumMatchLength||(localOffset&&theCompiledExpression->startCharsMatched[theChunk->data[theOffset]]))	// do some quick tests to bail out now if possible
			{
				if(REMatch(theChunk,theOffset,theCompiledExpression,(localOffset==numToSearch)?leftEdge:false,rightEdge,localOffset,&localFound,numMatched,endChunk,endOffset,allowAbort))
				{
					if(!localFound)
					{
						localOffset++;
						if(localOffset<=numToSearch)
						{
							if(!theOffset)							// move backward in the input
							{
								theChunk=theChunk->previousHeader;
								theOffset=theChunk->totalBytes;
							}
							theOffset--;
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
				localOffset++;
				if(localOffset<=numToSearch)
				{
					if(!theOffset)									// move backward in the input
					{
						theChunk=theChunk->previousHeader;
						theOffset=theChunk->totalBytes;
					}
					theOffset--;
				}
			}
		}
	}
	*startChunk=theChunk;
	*startOffset=theOffset;
	*foundMatch=localFound;
	*matchOffset=localOffset;

	return(!fail);
}

void REHuntBackToFarthestPossibleStart(CHUNKHEADER *theChunk,UINT32 theOffset,COMPILEDEXPRESSION *theCompiledExpression,UINT32 *numBack,CHUNKHEADER **endChunk,UINT32 *endOffset)
// Given a chunk and offset into some text, this will move backwards through the text to a point which
// is the farthest possible starting point for a regular expression which matches at the passed point
// numBack is the number of characters backwards that we moved, and endChunk/endOffset point to
// the starting point of a search.
// NOTE: this is too slow and ugly
{
	bool
		done;
	UINT32
		currentBack;

	*numBack=0;
	*endChunk=theChunk;
	*endOffset=theOffset;

	if(theChunk)			// if no text, then we cannot move back any further
	{
		done=false;
		currentBack=0;

		while(!done&&(theCompiledExpression->infiniteMatchLength||(currentBack<theCompiledExpression->maximumMatchLength)))
		{
			if(theCompiledExpression->charsMatched[theChunk->data[theOffset]])	// see if this is a matching character
			{
				*endChunk=theChunk;
				*endOffset=theOffset;
				*numBack=currentBack;

				currentBack++;

				if(theOffset)
				{
					theOffset--;
				}
				else
				{
					if(theChunk->previousHeader)
					{
						theChunk=theChunk->previousHeader;
						theOffset=theChunk->totalBytes-1;
					}
					else
					{
						done=true;	// hit the start
					}
				}
			}
			else
			{
				done=true;		// non matching, so this is the spot
			}
		}
	}
}
