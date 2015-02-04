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


#include "includes.h"

static UINT32
	numAllocated=0;

void *MResizePtr(void *thePointer,UINT32 theSize)
/* Resize thePointer to theSize, and return the resized pointer
 * if there is a problem, do not modify thePointer, SetError and return NULL
 */
{
	void
		*newPointer;

	if(newPointer=(void *)GlobalAlloc(GMEM_FIXED,(UINT32)theSize))
	{
		MoveMemory((void *)newPointer,(void *)thePointer,Min(GlobalSize(thePointer),theSize));
		GlobalFree(thePointer);
		return(newPointer);
	}
	else
	{
		SetWindowsError();
	}
	return(NULL);
}

void *MNewPtr(UINT32 theSize)
/* Call this to get a contiguous, aligned pointer to memory
 * If NULL is returned, the request could not be granted
 * If NULL is returned, GetError may be called to find out what went wrong
 */
{
	void
		*thePointer;

	if(thePointer=(void *)GlobalAlloc(GMEM_FIXED,(UINT32)theSize))
	{
		numAllocated++;
	}
	else
	{
		SetWindowsError();
	}
	return(thePointer);
}

void *MNewPtrClr(UINT32 theSize)
/* Call this to get a contiguous, aligned pointer to cleared memory
 * If NULL is returned, the request could not be granted
 * If NULL is returned, GetError may be called to find out what went wrong
 */
{
	void
		*thePointer;

	if(thePointer=(void *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,(UINT32)theSize))
	{
		numAllocated++;
	}
	else
	{
		SetWindowsError();
	}
	return(thePointer);
}

void MDisposePtr(void *thePointer)
/* Dispose of memory allocated by MNewPtr
 */
{
	numAllocated--;
	GlobalFree(thePointer);
}

void MMoveMem(void *source,void *dest,UINT32 numBytes)
/* Move memory from source to dest for numBytes
 * This must be smart enough to prevent overlapping areas from causing trouble
 */
{
	MoveMemory(dest,source,numBytes);
}

UINT32 MGetNumAllocatedPointers()
/* Return the current number of allocated pointers
 */
{
	return(numAllocated);
}
