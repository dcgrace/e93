// Undo/redo functions
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

static void DoClearUndo(UNDOGROUP *theGroup)
// clear the text, style, and undo arrays from theGroup
{
	ARRAYCHUNKHEADER
		*undoChunk;
	UINT32
		undoOffset;

	DeleteUniverseArray(&(theGroup->undoArray),theGroup->undoArray.firstChunkHeader,0,theGroup->undoArray.totalElements,&undoChunk,&undoOffset);
	DeleteStyleRange(theGroup->undoStyles,0,theGroup->undoText->totalBytes);
	DeleteUniverseText(theGroup->undoText,0,theGroup->undoText->totalBytes);
}

void BeginUndoGroup(EDITORBUFFER *theBuffer)
// all of the things that are done between now, and the matching EndUndoGroup
// will be remembered as a group in the undo buffer
{
	if(!theBuffer->undoUniverse->groupingDepth++)			// increment the depth, if it was 0, then start a new group
	{
		theBuffer->undoUniverse->currentFrame++;			// bump the frame number
	}
}

void EndUndoGroup(EDITORBUFFER *theBuffer)
// call this to balance a call to BeginUndoGroup
// it ends the undo group, but will allow subsequent undos to be joined
{
	if(theBuffer->undoUniverse->groupingDepth)
	{
		--theBuffer->undoUniverse->groupingDepth;			// decrement the group nesting count
	}
}

void StrictEndUndoGroup(EDITORBUFFER *theBuffer)
// call this to balance a call to BeginUndoGroup
// it ends the undo group, and will NOT allow subsequent undos to be joined
{
	if(theBuffer->undoUniverse->groupingDepth)
	{
		if(!(--theBuffer->undoUniverse->groupingDepth))		// decrement the group nesting count, if it went to 0, force new group
		{
			theBuffer->undoUniverse->currentFrame++;		// bump the frame number
		}
	}
}

static bool CreateUndoDeleteElement(EDITORBUFFER *theBuffer,UINT32 startPosition,UINT32 numBytes,UNDOGROUP *theGroup)
// create a new element at the end of the undo array
// if there is a problem, set the error, and return false
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	ARRAYCHUNKHEADER
		*undoChunk;
	UINT32
		undoOffset;
	UNDOELEMENT
		*undoElement;

	if(InsertUniverseArray(&(theGroup->undoArray),NULL,0,1,&undoChunk,&undoOffset))		// create the element
	{
		undoElement=&(((UNDOELEMENT *)undoChunk->data)[undoOffset]);
		undoElement->frameNumber=theBuffer->undoUniverse->currentFrame;		// set the frame number to the current undo frame
		undoElement->textStartPosition=startPosition;						// when the text is blasted back into place, this is where it goes
		undoElement->textLength=0;											// number of bytes to overwrite (none, we are deleting)
		undoElement->undoStartPosition=theGroup->undoText->totalBytes;		// start at what is now the current end of the undo text
		undoElement->undoLength=numBytes;									// number of bytes to put back
		PositionToChunkPosition(theBuffer->textUniverse,startPosition,&theChunk,&theOffset);	// point to bytes about to be deleted
		if(CopyStyle(theBuffer->styleUniverse,startPosition,theGroup->undoStyles,theGroup->undoText->totalBytes,numBytes))
		{
			if(InsertUniverseChunks(theGroup->undoText,theGroup->undoText->totalBytes,theChunk,theOffset,numBytes))	// copy into undo holding area
			{
				return(true);												// if we got them, then we are done
			}
			DeleteStyleRange(theGroup->undoStyles,theGroup->undoText->totalBytes,numBytes);	// kill the styles we copied
		}
		DeleteUniverseArray(&(theGroup->undoArray),undoChunk,undoOffset,1,&undoChunk,&undoOffset);	// we failed, so get rid of the last element
	}
	return(false);
}

static bool MergeUndoDeleteElement(EDITORBUFFER *theBuffer,UNDOELEMENT *lastElement,UINT32 startPosition,UINT32 numBytes,UNDOGROUP *theGroup)
// a delete is happening, and the last element of the undo list is in the current
// frame. See if the delete can be combined with the last element of the undo list,
// or if another entirely new element needs to be created
// if there is a problem, set the error, and return false
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	ARRAYCHUNKHEADER
		*undoChunk;
	UINT32
		undoOffset;
	UINT32
		bytesToEdge;

	if((lastElement->textStartPosition==startPosition)||(lastElement->textStartPosition+lastElement->textLength==startPosition+numBytes))	// for a delete to be grouped, it must start exactly in line with last element, or end exactly in line with it
	{
		if(startPosition>=lastElement->textStartPosition)
		{
			bytesToEdge=lastElement->textStartPosition+lastElement->textLength-startPosition;
			if(bytesToEdge>=numBytes)
			{
				lastElement->textLength-=numBytes;
				if(!lastElement->textLength&&!lastElement->undoLength)	// if this element has become empty, then get rid of it
				{
					DeleteUniverseArray(&(theGroup->undoArray),theGroup->undoArray.lastChunkHeader,theGroup->undoArray.lastChunkHeader->totalElements-1,1,&undoChunk,&undoOffset);
				}
				return(true);											// all that was needed was to remove bytes
			}
			else
			{
				startPosition+=bytesToEdge;
				numBytes-=bytesToEdge;
				PositionToChunkPosition(theBuffer->textUniverse,startPosition,&theChunk,&theOffset);	// point to bytes about to be deleted

				if(CopyStyle(theBuffer->styleUniverse,startPosition,theGroup->undoStyles,theGroup->undoText->totalBytes,numBytes))
				{
					if(InsertUniverseChunks(theGroup->undoText,theGroup->undoText->totalBytes,theChunk,theOffset,numBytes))		// copy into undo holding area
					{
						lastElement->textLength-=bytesToEdge;
						lastElement->undoLength+=numBytes;
						return(true);																	// if we got them, then we are done
					}
					DeleteStyleRange(theGroup->undoStyles,theGroup->undoText->totalBytes,numBytes);		// kill the styles we copied
				}
				return(false);
			}
		}
		else
		{
			if((startPosition+numBytes)<=(lastElement->textStartPosition+lastElement->textLength))		// make sure it does not span entire element
			{
				bytesToEdge=startPosition+numBytes-lastElement->textStartPosition;
				numBytes-=bytesToEdge;
				PositionToChunkPosition(theBuffer->textUniverse,startPosition,&theChunk,&theOffset);	// point to bytes about to be deleted
				if(InsertStyleRange(theGroup->undoStyles,lastElement->undoStartPosition,numBytes))		// move old styles over to account for new ones
				{
					if(CopyStyle(theBuffer->styleUniverse,startPosition,theGroup->undoStyles,lastElement->undoStartPosition,numBytes))
					{
						if(InsertUniverseChunks(theGroup->undoText,lastElement->undoStartPosition,theChunk,theOffset,numBytes))	// copy into undo holding area
						{
							lastElement->textStartPosition=startPosition;
							lastElement->textLength-=bytesToEdge;
							lastElement->undoLength+=numBytes;
							return(true);																	// if we got them, then we are done
						}
					}
					DeleteStyleRange(theGroup->undoStyles,lastElement->undoStartPosition,numBytes);		// kill the styles we inserted
				}
				return(false);
			}
			else
			{
				return(CreateUndoDeleteElement(theBuffer,startPosition,numBytes,theGroup));	// this delete spans entire previous element, so make new one
			}
		}
	}
// if we cannot merge, then create a new element
	if(!theBuffer->undoUniverse->groupingDepth)						// if not currently in a group, then make this a new frame
	{
		theBuffer->undoUniverse->currentFrame++;						// bump the frame number
	}
	return(CreateUndoDeleteElement(theBuffer,startPosition,numBytes,theGroup));	// delete in a different place
}

static bool RegisterDelete(EDITORBUFFER *theBuffer,UINT32 startPosition,UINT32 numBytes,UNDOGROUP *theGroup)
// register the delete into the given text buffer, and chunk array
{
	ARRAYCHUNKHEADER
		*lastUndoChunk;
	UNDOELEMENT
		*lastElement;

	if((lastUndoChunk=theGroup->undoArray.lastChunkHeader))							// is there a current item in the array?
	{
		lastElement=&(((UNDOELEMENT *)lastUndoChunk->data)[lastUndoChunk->totalElements-1]);	// point to last element of undo array
		if(theBuffer->undoUniverse->currentFrame==lastElement->frameNumber)			// is it in the current frame?
		{
			return(MergeUndoDeleteElement(theBuffer,lastElement,startPosition,numBytes,theGroup));
		}
		else
		{
			return(CreateUndoDeleteElement(theBuffer,startPosition,numBytes,theGroup));
		}
	}
	else
	{
		return(CreateUndoDeleteElement(theBuffer,startPosition,numBytes,theGroup));
	}
}

bool RegisterUndoDelete(EDITORBUFFER *theBuffer,UINT32 startPosition,UINT32 numBytes)
// call this BEFORE deleting numBytes of text from startPosition of theBuffer
// if there is a problem, return false
// NOTE: for speed, it is the caller's responsibility to determine theChunk and theOffset
// which point to startPosition of theBuffer
{
	UNDOUNIVERSE
		*theUndoUniverse;

	if(numBytes)
	{
		theUndoUniverse=theBuffer->undoUniverse;

		if(!theUndoUniverse->undoActive)
		{
			if(theUndoUniverse->cleanElements>theUndoUniverse->undoGroup.undoArray.totalElements)	// if position which is considered "clean" is beyond where we are now, it becomes meaningless
			{
				theUndoUniverse->cleanKnown=false;
			}
			DoClearUndo(&(theUndoUniverse->redoGroup));	// get rid of redo information
		}
		if(theUndoUniverse->undoing)					// if undoing, then place undo info into redo buffers
		{
			return(RegisterDelete(theBuffer,startPosition,numBytes,&(theUndoUniverse->redoGroup)));
		}
		else
		{
			return(RegisterDelete(theBuffer,startPosition,numBytes,&(theUndoUniverse->undoGroup)));
		}
	}
	else
	{
		return(true);
	}
}

static bool CreateUndoInsertElement(EDITORBUFFER *theBuffer,UINT32 startPosition,UINT32 numBytes,UNDOGROUP *theGroup)
// create a new element at the end of the undo array
// if there is a problem, set the error, and return false
{
	ARRAYCHUNKHEADER
		*undoChunk;
	UINT32
		undoOffset;
	UNDOELEMENT
		*undoElement;

	if(InsertUniverseArray(&(theGroup->undoArray),NULL,0,1,&undoChunk,&undoOffset))	// create the element
	{
		undoElement=&(((UNDOELEMENT *)undoChunk->data)[undoOffset]);
		undoElement->frameNumber=theBuffer->undoUniverse->currentFrame;		// set the frame number to the current undo frame
		undoElement->textStartPosition=startPosition;						// when the text is blasted back into place, this is where it goes
		undoElement->textLength=numBytes;									// number of bytes to overwrite (all that is being inserted)
		undoElement->undoStartPosition=theGroup->undoText->totalBytes;		// start at what is now the current end of the undo text
		undoElement->undoLength=0;											// number of bytes to put back (none, we are inserting)
		return(true);
	}
	return(false);
}

static bool MergeUndoInsertElement(EDITORBUFFER *theBuffer,UNDOELEMENT *lastElement,UINT32 startPosition,UINT32 numBytes,UNDOGROUP *theGroup)
// an insert is happening, and the last element of the undo list is in the current
// frame. See if the insert can be combined with the last element of the undo list,
// or if another entirely new element needs to be created
// if there is a problem, set the error, and return false
{
	if(startPosition==lastElement->textStartPosition+lastElement->textLength)	// see if insert just after end of current start
	{
		lastElement->textLength+=numBytes;									// add these bytes to what should be deleted during undo
		return(true);
	}
// if we cannot merge, then create a new element
	if(!theBuffer->undoUniverse->groupingDepth)								// if not currently in a group, then make this a new frame
	{
		theBuffer->undoUniverse->currentFrame++;							// bump the frame number
	}
	return(CreateUndoInsertElement(theBuffer,startPosition,numBytes,theGroup));	// insert in a different place
}

static bool RegisterInsert(EDITORBUFFER *theBuffer,UINT32 startPosition,UINT32 numBytes,UNDOGROUP *theGroup)
// register the insert into the given text buffer, and chunk array
// if there is a problem, set the error, and return false
{
	ARRAYCHUNKHEADER
		*lastUndoChunk;
	UNDOELEMENT
		*lastElement;

	if((lastUndoChunk=theGroup->undoArray.lastChunkHeader))							// is there a current item in the array?
	{
		lastElement=&(((UNDOELEMENT *)lastUndoChunk->data)[lastUndoChunk->totalElements-1]);	// point to last element of undo array
		if(theBuffer->undoUniverse->currentFrame==lastElement->frameNumber)			// is it in the current frame?
		{
			return(MergeUndoInsertElement(theBuffer,lastElement,startPosition,numBytes,theGroup));
		}
		else
		{
			return(CreateUndoInsertElement(theBuffer,startPosition,numBytes,theGroup));
		}
	}
	else
	{
		return(CreateUndoInsertElement(theBuffer,startPosition,numBytes,theGroup));
	}
}

bool RegisterUndoInsert(EDITORBUFFER *theBuffer,UINT32 startPosition,UINT32 numBytes)
// call this AFTER inserting numBytes of text into theBuffer at startPosition
// if there is a problem, set the error, return false
{
	UNDOUNIVERSE
		*theUndoUniverse;

	if(numBytes)
	{
		theUndoUniverse=theBuffer->undoUniverse;

		if(!theUndoUniverse->undoActive)
		{
			if(theUndoUniverse->cleanElements>theUndoUniverse->undoGroup.undoArray.totalElements)	// if position which is considered "clean" is beyond where we are now, it becomes meaningless
			{
				theUndoUniverse->cleanKnown=false;
			}
			DoClearUndo(&(theUndoUniverse->redoGroup));	// get rid of redo information
		}
		if(theUndoUniverse->undoing)					// if undoing, then place undo info into redo buffers
		{
			return(RegisterInsert(theBuffer,startPosition,numBytes,&(theUndoUniverse->redoGroup)));
		}
		else
		{
			return(RegisterInsert(theBuffer,startPosition,numBytes,&(theUndoUniverse->undoGroup)));
		}
	}
	else
	{
		return(true);
	}
}

static bool UndoSelectionChange(SELECTIONUNIVERSE *theSelectionUniverse,UINT32 thePosition,UINT32 numBytesOut,UINT32 numBytesIn)
// Handle updating the selection, an creating new selections during undo
// NOTE: if something goes wrong, attempt to leave the selection universe in a sane state
{
	AdjustSelectionsForChange(theSelectionUniverse,thePosition,thePosition+numBytesOut,thePosition+numBytesIn);
	if(SetSelectionRange(theSelectionUniverse,thePosition,numBytesIn))
	{
		return(true);
	}
	return(false);
}

static bool DoUndo(EDITORBUFFER *theBuffer,UNDOGROUP *theGroup,bool *didSomething)
// undo from the given text and array into theBuffer
// NOTE: if there is nothing to undo, return false in *didSomething
// else return true.
// If something bad happens while trying to undo, set the error, and return false
{
	SELECTIONUNIVERSE
		*theSelectionUniverse,
		*newSelectionUniverse;
	ARRAYCHUNKHEADER
		*lastUndoChunk;
	UINT32
		tempOffset;
	UNDOELEMENT
		*lastElement;
	UINT32
		frameToUse;
	bool
		haveMinimumPosition;
	UINT32
		minimumPosition;													// used to set the cursor position after the change
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	bool
		fail;

	fail=*didSomething=false;
	haveMinimumPosition=false;
	minimumPosition=0;
	if((lastUndoChunk=theGroup->undoArray.lastChunkHeader))					// is there a current item in the array?
	{
		lastElement=&(((UNDOELEMENT *)lastUndoChunk->data)[lastUndoChunk->totalElements-1]);	// point to last element of undo array
		frameToUse=lastElement->frameNumber;								// undo all elements of this frame

		if((newSelectionUniverse=OpenSelectionUniverse()))					// create a new selection universe where the output selection will be created
		{
			theSelectionUniverse=theBuffer->selectionUniverse;				// point to current selection universe
			EditorStartSelectionChange(theBuffer);
			DeleteAllSelections(theSelectionUniverse);						// remove all old selections
			EditorEndSelectionChange(theBuffer);

			EditorStartReplace(theBuffer);
			BeginUndoGroup(theBuffer);										// group the redo of this undo
			while(!fail&&lastElement&&lastElement->frameNumber==frameToUse)
			{
				PositionToChunkPosition(theGroup->undoText,lastElement->undoStartPosition,&theChunk,&theOffset);	// point to bytes about to be placed into the text
				if(ReplaceEditorChunks(theBuffer,lastElement->textStartPosition,lastElement->textStartPosition+lastElement->textLength,theChunk,theOffset,lastElement->undoLength))
				{
					if(CopyStyle(theGroup->undoStyles,lastElement->undoStartPosition,theBuffer->styleUniverse,lastElement->textStartPosition,lastElement->undoLength))
					{
						if(UndoSelectionChange(newSelectionUniverse,lastElement->textStartPosition,lastElement->textLength,lastElement->undoLength))
						{
							if(haveMinimumPosition)
							{
								if(lastElement->textStartPosition<minimumPosition)
								{
									minimumPosition=lastElement->textStartPosition;
								}
							}
							else
							{
								minimumPosition=lastElement->textStartPosition;
								haveMinimumPosition=true;
							}

							DeleteStyleRange(theGroup->undoStyles,lastElement->undoStartPosition,lastElement->undoLength);
							DeleteUniverseText(theGroup->undoText,lastElement->undoStartPosition,lastElement->undoLength);
							DeleteUniverseArray(&(theGroup->undoArray),lastUndoChunk,lastUndoChunk->totalElements-1,1,&lastUndoChunk,&tempOffset);
							if((lastUndoChunk=theGroup->undoArray.lastChunkHeader))		// fetch the last element again
							{
								lastElement=&(((UNDOELEMENT *)lastUndoChunk->data)[lastUndoChunk->totalElements-1]);	// point to last element of undo array
							}
							else
							{
								lastElement=NULL;
							}
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
					fail=true;
				}
			}

			EditorEndReplace(theBuffer);
			StrictEndUndoGroup(theBuffer);										// end this group, do not let anything combine with it

			// place new selection into the universe

			EditorStartSelectionChange(theBuffer);
			SetSelectionCursorPosition(newSelectionUniverse,minimumPosition);	// place cursor at the start
			theBuffer->selectionUniverse=newSelectionUniverse;
			CloseSelectionUniverse(theSelectionUniverse);						// close old universe
			EditorEndSelectionChange(theBuffer);

			*didSomething=true;
		}
		else
		{
			fail=true;
		}
	}
	return(!fail);
}

bool EditorUndo(EDITORBUFFER *theBuffer)
// undo whatever was done last (if there is nothing in the undo buffer
// then do not do anything)
// If there is no undo information, return false
// otherwise return true.
// NOTE: if there is a failure, report it here
{
	bool
		didSomething,
		result;

	didSomething=false;
	ShowBusy();
	theBuffer->undoUniverse->undoActive=true;							// changes are coming from undo/redo, not user
	theBuffer->undoUniverse->undoing=true;								// remember that we are undoing so if register calls come in, we can place them in redo buffer
	result=DoUndo(theBuffer,&(theBuffer->undoUniverse->undoGroup),&didSomething);
	theBuffer->undoUniverse->undoing=false;
	theBuffer->undoUniverse->undoActive=false;
	ShowNotBusy();
	if(!result)
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("Failed to undo: %.256s\nNOTE: graceful recovery from this error is not likely.\nThe buffer contents may now be corrupt :-(\n",errorDescription);
		didSomething=true;												// tell the caller that something was done
	}
	return(didSomething);
}

bool EditorRedo(EDITORBUFFER *theBuffer)
// redo whatever was undone last
// If there is no undo information, return false in *didSomething
// otherwise return true.
// NOTE: if there is a failure, report it here
{
	bool
		didSomething,
		result;

	didSomething=false;
	ShowBusy();
	theBuffer->undoUniverse->undoActive=true;							// changes are coming from undo/redo, not user
	result=DoUndo(theBuffer,&(theBuffer->undoUniverse->redoGroup),&didSomething);
	theBuffer->undoUniverse->undoActive=false;
	ShowNotBusy();
	if(!result)
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("Failed to redo: %.256s\nNOTE: graceful recovery from this error is not likely.\nThe buffer contents may now be corrupt :-(\n",errorDescription);
		didSomething=true;												// tell the caller that something was done
	}
	return(didSomething);
}

bool EditorToggleUndo(EDITORBUFFER *theBuffer)
// if there is a redo that can be done, then do it, otherwise, do an undo
// if neither can be done, return false
// NOTE: if there is a failure, report it here
{
	if(!EditorRedo(theBuffer))
	{
		return(EditorUndo(theBuffer));
	}
	return(true);
}

void EditorClearUndo(EDITORBUFFER *theBuffer)
// when, undo/redo must be cleared, call this to do it
{
	if(theBuffer->undoUniverse->cleanKnown)	// if clean position is known, then make sure we are there, otherwise we don't know it anymore
	{
		if(theBuffer->undoUniverse->cleanElements!=theBuffer->undoUniverse->undoGroup.undoArray.totalElements)
		{
			theBuffer->undoUniverse->cleanKnown=false;
		}
	}
	theBuffer->undoUniverse->cleanElements=0;			// clean position moves to 0

	DoClearUndo(&(theBuffer->undoUniverse->undoGroup));	// get rid of undo information
	DoClearUndo(&(theBuffer->undoUniverse->redoGroup));	// get rid of redo information
	theBuffer->undoUniverse->currentFrame=0;			// reset the frame number
}

void SetUndoCleanPoint(EDITORBUFFER *theBuffer)
// mark the current position in the undo data as the buffer "clean" position
// NOTE: this will force a break in the undo flow if needed
{
	theBuffer->undoUniverse->currentFrame++;			// bump the frame number to force a break in the undo flow
	theBuffer->undoUniverse->cleanKnown=true;
	theBuffer->undoUniverse->cleanElements=theBuffer->undoUniverse->undoGroup.undoArray.totalElements;
}

bool AtUndoCleanPoint(EDITORBUFFER *theBuffer)
// tells if the buffer is at the "clean" point in the undo flow
{
	if(theBuffer->undoUniverse->cleanKnown&&theBuffer->undoUniverse->cleanElements==theBuffer->undoUniverse->undoGroup.undoArray.totalElements)
	{
		return(true);
	}
	return(false);
}

static bool InitializeUndoGroup(UNDOGROUP *theGroup)
// initialize an undo group
{
	if((theGroup->undoText=OpenTextUniverse()))
	{
		if((theGroup->undoStyles=OpenStyleUniverse()))
		{
			if(InitArrayUniverse(&(theGroup->undoArray),UNDOCHUNKELEMENTS,sizeof(UNDOELEMENT)))
			{
				return(true);
			}
			CloseStyleUniverse(theGroup->undoStyles);
		}
		CloseTextUniverse(theGroup->undoText);
	}
	return(false);
}

static void UnInitializeUndoGroup(UNDOGROUP *theGroup)
// undo what InitializeUndoGroup did
{
	UnInitArrayUniverse(&(theGroup->undoArray));
	CloseStyleUniverse(theGroup->undoStyles);
	CloseTextUniverse(theGroup->undoText);
}

UNDOUNIVERSE *OpenUndoUniverse()
// open an undo universe
// if there is a problem, set the error, and return NULL
{
	UNDOUNIVERSE
		*theUniverse;

	if((theUniverse=(UNDOUNIVERSE *)MNewPtr(sizeof(UNDOUNIVERSE))))
	{
		theUniverse->currentFrame=0;
		theUniverse->groupingDepth=0;
		theUniverse->cleanElements=0;
		theUniverse->cleanKnown=true;
		theUniverse->undoActive=false;
		theUniverse->undoing=false;
		if(InitializeUndoGroup(&(theUniverse->undoGroup)))
		{
			if(InitializeUndoGroup(&(theUniverse->redoGroup)))
			{
				return(theUniverse);
			}
			UnInitializeUndoGroup(&(theUniverse->undoGroup));
		}
		MDisposePtr(theUniverse);
	}
	return(NULL);
}

void CloseUndoUniverse(UNDOUNIVERSE *theUniverse)
// dispose of an undo universe
{
	UnInitializeUndoGroup(&(theUniverse->redoGroup));
	UnInitializeUndoGroup(&(theUniverse->undoGroup));
	MDisposePtr(theUniverse);
}
