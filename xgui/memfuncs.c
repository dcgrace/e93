// This takes care of all memory requests, and frees
// It indirects all editor requests for memory
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

static UINT32
	numAllocated=0;

static char localErrorFamily[]="memory";

enum
{
	NOMEMORY
};

static char *errorMembers[]=
{
	"NoMemory"
};

static char *errorDescriptions[]=
{
	"Insufficient memory"
};

void *MResizePtr(void *thePointer,UINT32 theSize)
// Resize thePointer to theSize, and return the resized pointer
// if there is a problem, do not modify thePointer, SetError and return NULL
{
	void
		*newPointer;

	if((newPointer=(void *)realloc(thePointer,(int)theSize)))
	{
		return(newPointer);
	}
	else
	{
		SetError(localErrorFamily,errorMembers[NOMEMORY],errorDescriptions[NOMEMORY]);	// could not allocate memory, so tell user
	}
	return(NULL);
}

void *MNewPtr(UINT32 theSize)
// Call this to get a contiguous, aligned pointer to memory
// If NULL is returned, the request could not be granted
// If NULL is returned, GetError may be called to find out what went wrong
{
	void
		*thePointer;

	if((thePointer=(void *)malloc((int)theSize)))
	{
		numAllocated++;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[NOMEMORY],errorDescriptions[NOMEMORY]);	// could not allocate memory, so tell user
	}
	return(thePointer);
}

void *MNewPtrClr(UINT32 theSize)
// Call this to get a contiguous, aligned pointer to cleared memory
// If NULL is returned, the request could not be granted
// If NULL is returned, GetError may be called to find out what went wrong
{
	void
		*thePointer;

	if((thePointer=(void *)calloc((int)theSize,1)))
	{
		numAllocated++;
	}
	else
	{
		SetError(localErrorFamily,errorMembers[NOMEMORY],errorDescriptions[NOMEMORY]);	// could not allocate memory, so tell user
	}
	return(thePointer);
}

void MDisposePtr(void *thePointer)
// Dispose of memory allocated by MNewPtr
{
	if(thePointer)
	{
		numAllocated--;
		free(thePointer);
	}
}

void MMoveMem(void *source,void *dest,UINT32 numBytes)
// Move memory from source to dest for numBytes
// This must be smart enough to prevent overlapping areas from causing trouble
{
	memmove(dest,source,(int)numBytes);
}

UINT32 MGetNumAllocatedPointers()
// Return the current number of allocated pointers
{
	return(numAllocated);
}
