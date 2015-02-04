// Cut, copy, and paste functions
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

static EDITORBUFFERHANDLE
	currentClipboardHandle;							// handle to the current clipboard

EDITORBUFFER *EditorGetCurrentClipboard()
// return the current clipboard, or NULL if there is none
{
	return(EditorGetBufferFromHandle(&currentClipboardHandle));
}

void EditorSetCurrentClipboard(EDITORBUFFER *theClipboard)
// set the current clipboard
{
	EditorReleaseBufferHandle(&currentClipboardHandle);					// release our grip on any buffer we held
	if(theClipboard)
	{
		EditorGrabBufferHandle(&currentClipboardHandle,theClipboard);	// hang onto this one
	}
}

void EditorCopy(EDITORBUFFER *theBuffer,EDITORBUFFER *theClipboard)
// copy the current selection (if there is one) from theBuffer into theClipboard
// a "selection" is made in the clipboard text for every selection that is
// copied from the source text. These selections are used when columnar pasting.
{
	CHUNKHEADER
		*sourceChunk;
	UINT32
		sourceChunkOffset;
	EDITORBUFFER
		*sourceBuffer,
		*destBuffer;
	SELECTIONUNIVERSE
		*sourceSelectionUniverse,
		*destSelectionUniverse;
	UINT32
		sourceStartPosition,
		sourceLength,
		destStartPosition,
		destLength;
	bool
		sawInitialSelection,
		fail;

	fail=false;
	if(theClipboard)
	{
		destBuffer=theClipboard;
		destSelectionUniverse=destBuffer->selectionUniverse;
		sourceBuffer=theBuffer;
		sourceSelectionUniverse=sourceBuffer->selectionUniverse;

		EditorClearUndo(destBuffer);	// kill any undo information that happened to be lying about in the clipboard (if user opened window to clipboard and started typing)

		EditorStartReplace(destBuffer);

		// kill off what was in the clipboard
		fail=!ReplaceEditorChunks(destBuffer,0,destBuffer->textUniverse->totalBytes,NULL,0,0);

		sawInitialSelection=false;
		sourceStartPosition=0;
		while(!fail&&GetSelectionAtOrAfterPosition(sourceSelectionUniverse,sourceStartPosition,&sourceStartPosition,&sourceLength))
		{
			// after every line but the last, place in a newline so that exported columnar selections work as expected
			if(sawInitialSelection)
			{
				fail=!ReplaceEditorText(destBuffer,destBuffer->textUniverse->totalBytes,destBuffer->textUniverse->totalBytes,(UINT8 *)"\n",1);
			}
			if(!fail)
			{
				PositionToChunkPosition(sourceBuffer->textUniverse,sourceStartPosition,&sourceChunk,&sourceChunkOffset);	// point to the start of the selection in the source data
				destStartPosition=destBuffer->textUniverse->totalBytes;
				if(ReplaceEditorChunks(destBuffer,destStartPosition,destStartPosition,sourceChunk,sourceChunkOffset,sourceLength))	// insert the text at the end of the destination
				{
					if(CopyStyle(sourceBuffer->styleUniverse,sourceStartPosition,destBuffer->styleUniverse,destStartPosition,sourceLength))
					{
						destLength=destBuffer->textUniverse->totalBytes-destStartPosition;
						if(destLength>0)
						{
							fail=!SetSelectionRange(destSelectionUniverse,destStartPosition,destLength);	// create new selection
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
			sawInitialSelection=true;
			sourceStartPosition+=sourceLength;
		}
		EditorEndReplace(destBuffer);
	}
	if(!fail)
	{
		if(theClipboard==EditorGetCurrentClipboard())		// only export the clipboard if it is the current one, and we did not fail
		{
			if(!ExportClipboard())
			{
				ReportMessage("Failed to export clipboard\n");
			}
		}
	}
	else
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("Failed to copy: %.256s\n",errorDescription);
	}
}


void EditorCut(EDITORBUFFER *theBuffer,EDITORBUFFER *theClipboard)
// cut the current selection if there is one from theBuffer into theClipboard
{
	EditorCopy(theBuffer,theClipboard);
	EditorStartReplace(theBuffer);
	BeginUndoGroup(theBuffer);
	DeleteAllSelectedText(theBuffer,theBuffer->selectionUniverse);
	StrictEndUndoGroup(theBuffer);
	EditorEndReplace(theBuffer);
}

void EditorColumnarPaste(EDITORBUFFER *theBuffer,EDITORVIEW *theView,EDITORBUFFER *theClipboard)
// paste the contents of theClipboard into theBuffer in columnar format
// this means that each "selection" of the clipboard will be placed into theBuffer at
// the current cursor position, then the position is moved to the next line, and
// the process is repeated until all of the selections have been placed
// NOTE: if there are selections in theBuffer, they are replaced one at a time
// Any selections remaining after the paste is complete are deleted.
// NOTE: if there are no selections in the clipboard, then it is just pasted as one
// large blob
// Finally, if theView is passed as NULL, and the paste requires moving down a line
// at a time, this will just paste the remaining selections at the starts of lines.
{
	CHUNKHEADER
		*sourceChunk,
		*destChunk;
	UINT32
		sourceChunkOffset,
		destChunkOffset;
	EDITORBUFFER
		*sourceBuffer;
	SELECTIONUNIVERSE
		*sourceSelectionUniverse,
		*destSelectionUniverse;
	UINT32
		sourceStartPosition,
		sourceLength,
		destStartPosition,
		destLength,
		lastReplacePosition,
		finalCursorPosition;
	UINT32
		temp,
		lastNumLines,
		numReplaced,
		newPosition,
		theLine,
		theLineOffset;
	INT32
		desiredX;
	bool
		replacedOne,
		fail;

	if(theClipboard)
	{
		sourceBuffer=theClipboard;
		if(ImportClipboard())							// give external world a chance to override clipboard
		{
			fail=false;

			destSelectionUniverse=theBuffer->selectionUniverse;
			sourceSelectionUniverse=sourceBuffer->selectionUniverse;
			EditorStartReplace(theBuffer);
			BeginUndoGroup(theBuffer);

			finalCursorPosition=lastReplacePosition=GetSelectionCursorPosition(destSelectionUniverse);	// initialize these
			if(!IsSelectionEmpty(sourceSelectionUniverse))		// see if selections in the clipboard
			{
				sourceStartPosition=0;							// get absolute position
				destStartPosition=0;
				numReplaced=0;
				replacedOne=false;								// tells if a replace has been done, so we know not to move the cursor position further

				// first, replace selections in the destination with selections from the source
				while(!fail&&GetSelectionAtOrAfterPosition(sourceSelectionUniverse,sourceStartPosition,&sourceStartPosition,&sourceLength)&&GetSelectionAtOrAfterPosition(destSelectionUniverse,0,&destStartPosition,&destLength))
				{
					PositionToChunkPosition(sourceBuffer->textUniverse,sourceStartPosition,&sourceChunk,&sourceChunkOffset);	// point to the start of the selection in the source data

					lastReplacePosition=destStartPosition;		// update this for later
					if(ReplaceEditorChunks(theBuffer,destStartPosition,destStartPosition+destLength,sourceChunk,sourceChunkOffset,sourceLength))
					{
						if(CopyStyle(sourceBuffer->styleUniverse,sourceStartPosition,theBuffer->styleUniverse,destStartPosition,sourceLength))
						{
							if(!replacedOne)
							{
								finalCursorPosition=destStartPosition+sourceLength;	// place cursor after start of first replacement when this procedure is finished
								replacedOne=true;
							}
							numReplaced=sourceLength;				// number of bytes that were added at lastReplacePosition
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
					sourceStartPosition+=sourceLength;
				}

				// now, replace by moving down a line at a time
				PositionToLinePosition(theBuffer->textUniverse,lastReplacePosition,&theLine,&theLineOffset,&destChunk,&destChunkOffset);
				if(theView)
				{
					GetEditorViewTextToGraphicPosition(theView,lastReplacePosition,&desiredX,false,&temp,&temp);	// find the X offset we will attempt to insert at each time
				}

				if(replacedOne)		// if managed to replace, then move to the line BELOW the last replacement
				{
					PositionToLinePosition(theBuffer->textUniverse,lastReplacePosition+numReplaced,&theLine,&theLineOffset,&destChunk,&destChunkOffset);
					theLine++;		// start replacing on this line
				}

				while(!fail&&GetSelectionAtOrAfterPosition(sourceSelectionUniverse,sourceStartPosition,&sourceStartPosition,&sourceLength))
				{
					PositionToChunkPosition(sourceBuffer->textUniverse,sourceStartPosition,&sourceChunk,&sourceChunkOffset);	// point to the start of the selection in the source data

					LineToChunkPosition(theBuffer->textUniverse,theLine,&destChunk,&destChunkOffset,&destStartPosition);
					if(theView)
					{
						GetEditorViewGraphicToTextPosition(theView,destStartPosition,desiredX,&newPosition,&temp);
					}
					else
					{
						newPosition=0;
					}

					destStartPosition+=newPosition;			// this is where the insertion should occur

					if(theLine>theBuffer->textUniverse->totalLines)	// attempting to place on non-existent line?
					{
						if(ReplaceEditorText(theBuffer,destStartPosition,destStartPosition,(UINT8 *)"\n",1))	// stick in a newline at the end
						{
							destStartPosition++;					// then move past it
						}
						else
						{
							fail=true;
						}
					}
					lastNumLines=theBuffer->textUniverse->totalLines;			// remember how many lines there were, so we can see how many were inserted
					if(!fail)
					{
						if(ReplaceEditorChunks(theBuffer,destStartPosition,destStartPosition,sourceChunk,sourceChunkOffset,sourceLength))
						{
							if(CopyStyle(sourceBuffer->styleUniverse,sourceStartPosition,theBuffer->styleUniverse,destStartPosition,sourceLength))
							{
								if(!replacedOne)
								{
									finalCursorPosition=destStartPosition+sourceLength;		// place cursor after start of first replacement
									replacedOne=true;
								}
								theLine+=theBuffer->textUniverse->totalLines-lastNumLines+1;	// skip to next line, plus any inserted
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
					sourceStartPosition+=sourceLength;
				}
			}
			else
			{
				if(!GetSelectionAtOrAfterPosition(destSelectionUniverse,0,&destStartPosition,&destLength))
				{
					destStartPosition=finalCursorPosition;
					destLength=0;
				}
				if(ReplaceEditorChunks(theBuffer,destStartPosition,destStartPosition+destLength,sourceBuffer->textUniverse->firstChunkHeader,0,sourceBuffer->textUniverse->totalBytes))
				{
					fail=!CopyStyle(sourceBuffer->styleUniverse,0,theBuffer->styleUniverse,destStartPosition,sourceBuffer->textUniverse->totalBytes);
				}
				else
				{
					fail=true;
				}
				finalCursorPosition=destStartPosition+sourceBuffer->textUniverse->totalBytes;		// place cursor after start of replacement
			}

			if(!fail)
			{
				DeleteAllSelectedText(theBuffer,destSelectionUniverse);			// kill any remaining selections
				SetSelectionCursorPosition(destSelectionUniverse,finalCursorPosition);	// drop cursor after start of first replacement
			}

			StrictEndUndoGroup(theBuffer);
			EditorEndReplace(theBuffer);

			if(fail)
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				ReportMessage("Failed to paste: %.256s\n",errorDescription);
			}
		}
		else
		{
			ReportMessage("Failed to import clipboard\n");
		}
	}
}

EDITORBUFFER *EditorStartImportClipboard()
// Call this from the gui to get the editor buffer that is
// the current clipboard, when something external to the editor has modified
// the system clipboard, and it is desired to get that data into the editor
// it is legal for this routine to return NULL
// in which case you should not attempt to place anything into the clipboard
// and you should not call EditorEndImportClipboard
{
	return(EditorGetCurrentClipboard());
}

void EditorEndImportClipboard()
// Call this when you are done modifying the clipboard that was returned
// from EditorStartImportClipboard
{
}

EDITORBUFFER *EditorStartExportClipboard()
// Call this from the gui to get the editor buffer that is
// the current clipboard, when you wish to export the current clipboard
// if this returns NULL, you should not call EditorEndExportClipboard
{
	return(EditorGetCurrentClipboard());
}

void EditorEndExportClipboard()
// Call this when you are done exporting the clipboard that was returned
// from EditorStartExportClipboard
{
}

void UnInitClipboard()
// undo whatever InitClipboard did
{
	EditorSetCurrentClipboard(NULL);			// clear the current clipboard, releasing any buffer it held
}

bool InitClipboard()
// initialize clipboard handling
// if there is a problem, SetError, and return false
{
	EditorInitBufferHandle(&currentClipboardHandle);
	return(true);
}
