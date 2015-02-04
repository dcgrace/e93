// Handle high level buffer stuff
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

// Some handy notes about buffers:
// shellBusy
//   This is used exclusively by the the shell. It indicates to the
//   shell that it is already doing something with the buffer, and that it is
//   not allowed to affect it in any way (useful when doing search and replace,
//   where replace is a shell script... keeps script from being able to modify
//   the buffer that is in the middle of being modified by replace command)

static EDITORBUFFER
	*startBuffer;								// pointer to start of editor buffer list

static char localErrorFamily[]="buffer";

enum
{
	NOTFROMFILE,
	DUPLICATENAME
};

static char *errorMembers[]=
{
	"NotFromFile",
	"DuplicateName"
};

static char *errorDescriptions[]=
{
	"Buffer is not linked to a file",
	"Duplicate buffer names are not allowed"
};

static void DisposeBuffer(EDITORBUFFER *theBuffer)
// dispose of a buffer
{
	EDITORBUFFER
		*previousBuffer,
		*testBuffer;
	EDITORBUFFERHANDLE
		*theHandle;

	theHandle=theBuffer->bufferHandles;
	while(theHandle)							// for each handle to this buffer, unlink and clear it
	{
		theHandle->theBuffer=NULL;				// this buffer is gone...
		theHandle=theHandle->nextHandle;
	}

	UnInitVariableBindingTable(&(theBuffer->variableTable));	// remove variables hanging off this buffer
	if(theBuffer->theWindow)
	{
		EditorCloseDocumentWindow(theBuffer->theWindow);		// close down the window if one exists
	}
	if(theBuffer->theTask)
	{
		KillBufferTask(theBuffer);								// kill any task the buffer may hold
		// DisconnectBufferTask(theBuffer);						// close down the connection to any task that may be attached to this buffer
	}
	if(theBuffer->syntaxInstance)								// if syntax instance has been assigned, remove it
	{
		CloseSyntaxInstance(theBuffer->syntaxInstance);
	}
	CloseStyleUniverse(theBuffer->styleUniverse);
	while(theBuffer->theMarks)
	{
		ClearEditorMark(theBuffer,theBuffer->theMarks);			// get rid of all marks on this buffer
	}
	CloseSelectionUniverse(theBuffer->auxSelectionUniverse);
	CloseSelectionUniverse(theBuffer->selectionUniverse);
	CloseUndoUniverse(theBuffer->undoUniverse);
	CloseTextUniverse(theBuffer->textUniverse);
	MDisposePtr(theBuffer->contentName);

	previousBuffer=NULL;
	testBuffer=startBuffer;
	while(testBuffer&&testBuffer!=theBuffer)
	{
		previousBuffer=testBuffer;
		testBuffer=testBuffer->nextBuffer;
	}
	if(testBuffer==theBuffer)									// make sure we found it before trying to unlink
	{
		if(previousBuffer)
		{
			previousBuffer->nextBuffer=theBuffer->nextBuffer;
		}
		else
		{
			startBuffer=theBuffer->nextBuffer;
		}
	}

	MDisposePtr(theBuffer);
}

static EDITORBUFFER *CreateBuffer(char *bufferName)
// create a buffer (buffers contain both text and selection
// universes (which are created here))
// if there is a problem, set the error, and return NULL
{
	EDITORBUFFER
		*theBuffer;

	if((theBuffer=(EDITORBUFFER *)MNewPtr(sizeof(EDITORBUFFER))))
	{
		theBuffer->bufferHandles=NULL;									// not being held
		theBuffer->shellBusy=false;										// set this to a known value
		theBuffer->fromFile=false;

		if((theBuffer->contentName=(char *)MNewPtr(strlen(bufferName)+1)))
		{
			strcpy(theBuffer->contentName,bufferName);
			if((theBuffer->textUniverse=OpenTextUniverse()))			// create the text universe
			{
				if((theBuffer->undoUniverse=OpenUndoUniverse()))		// create the undo universe
				{
					if((theBuffer->selectionUniverse=OpenSelectionUniverse()))			// create the selection universe
					{
						if((theBuffer->auxSelectionUniverse=OpenSelectionUniverse()))	// create the aux selection universe (used for tasks, so they can write data into a place other than at the cursor)
						{
							theBuffer->theMarks=NULL;						// no marks on this buffer yet
							if((theBuffer->styleUniverse=OpenStyleUniverse()))			// create the style universe
							{
								theBuffer->syntaxInstance=NULL;				// no syntax highlight on this buffer yet
								theBuffer->theTask=NULL;
								theBuffer->taskBytes=0;
								theBuffer->theWindow=NULL;
								theBuffer->firstView=NULL;					// no views onto this buffer yet
								InitVariableBindingTable(&(theBuffer->variableTable));	// initialize the variable table

								theBuffer->replaceHaveRange=false;
								theBuffer->replaceStartOffset=0;
								theBuffer->replaceEndOffset=0;
								theBuffer->replaceNumBytes=0;

								theBuffer->anchorStartPosition=0;			// clear selection handling variables
								theBuffer->anchorEndPosition=0;
								theBuffer->anchorLine=0;
								theBuffer->anchorX=0;
								theBuffer->columnarTopLine=0;
								theBuffer->columnarBottomLine=0;
								theBuffer->columnarLeftX=0;
								theBuffer->columnarRightX=0;
								theBuffer->haveStartX=false;
								theBuffer->haveEndX=false;
								theBuffer->desiredStartX=0;
								theBuffer->desiredEndX=0;
								theBuffer->haveCurrentEnd=false;
								theBuffer->currentIsStart=false;

								theBuffer->nextBuffer=startBuffer;
								startBuffer=theBuffer;
								return(theBuffer);
							}
							CloseSelectionUniverse(theBuffer->auxSelectionUniverse);
						}
						CloseSelectionUniverse(theBuffer->selectionUniverse);
					}
					CloseUndoUniverse(theBuffer->undoUniverse);
				}
				CloseTextUniverse(theBuffer->textUniverse);
			}
			MDisposePtr(theBuffer->contentName);
		}
		MDisposePtr(theBuffer);
	}
	return(NULL);
}

void SetBufferBusy(EDITORBUFFER *theBuffer)
// set the buffer's busy state
{
	theBuffer->shellBusy=true;
}

void ClearBufferBusy(EDITORBUFFER *theBuffer)
// clear the buffer's busy state
{
	theBuffer->shellBusy=false;
}

bool BufferBusy(EDITORBUFFER *theBuffer)
// check the given buffer's busy state
// if busy, return true, else false
{
	return(theBuffer->shellBusy);
}

EDITORBUFFER *EditorGetFirstBuffer()
// return the first buffer, or NULL if none
{
	return(startBuffer);
}

EDITORBUFFER *EditorGetNextBuffer(EDITORBUFFER *theBuffer)
// return the next buffer, or NULL if none
{
	return(theBuffer->nextBuffer);
}

EDITORBUFFER *LocateBuffer(char *bufferName)
// search through the list of buffers, and attempt to locate
// one with the given name
// if one is found, return it, else return NULL
{
	bool
		found;
	EDITORBUFFER
		*testBuffer;

	testBuffer=startBuffer;
	found=false;
	while(testBuffer&&!found)
	{
		if(strcmp(testBuffer->contentName,bufferName)==0)
		{
			found=true;
		}
		else
		{
			testBuffer=testBuffer->nextBuffer;
		}
	}
	return(testBuffer);
}

void EditorCloseBuffer(EDITORBUFFER *theBuffer)
// close the given edit buffer
// if the buffer has a task, it will be disconnected
// if the buffer has a window, it will be closed too
// this will close the window without saving, even if the text has been modified
{
	DisposeBuffer(theBuffer);
}

void EditorReleaseBufferHandle(EDITORBUFFERHANDLE *theHandle)
// Remove theHandle from the buffer it was linked to
// NOTE: if theHandle is already released, this does nothing
{
	EDITORBUFFERHANDLE
		*previousHandle,
		*currentHandle;

	if(theHandle->theBuffer)			// make sure this handle is held
	{
		previousHandle=NULL;
		currentHandle=theHandle->theBuffer->bufferHandles;
		while(currentHandle&&currentHandle!=theHandle)
		{
			previousHandle=currentHandle;
			currentHandle=currentHandle->nextHandle;
		}
		if(currentHandle)
		{
			if(previousHandle)
			{
				previousHandle->nextHandle=currentHandle->nextHandle;
			}
			else
			{
				currentHandle->theBuffer->bufferHandles=currentHandle->nextHandle;
			}
		}
		theHandle->theBuffer=NULL;
	}
}

void EditorGrabBufferHandle(EDITORBUFFERHANDLE *theHandle,EDITORBUFFER *theBuffer)
// grab a handle to theBuffer
{
	theHandle->theBuffer=theBuffer;
	theHandle->nextHandle=theBuffer->bufferHandles;
	theBuffer->bufferHandles=theHandle;
}

EDITORBUFFER *EditorGetBufferFromHandle(EDITORBUFFERHANDLE *theHandle)
// return the buffer that theHandle points to
{
	return(theHandle->theBuffer);
}

void EditorInitBufferHandle(EDITORBUFFERHANDLE *theHandle)
// initialize a buffer handle so that it is in the released state
{
	theHandle->theBuffer=NULL;
}

bool EditorRevertBuffer(EDITORBUFFER *theBuffer)
// revert theBuffer, no questions asked
// if there is a problem, SetError, and return false
// NOTE: the window could be left with only part of the reverted text
// if there was a problem
{
	if(theBuffer->fromFile)
	{
		if(LoadBuffer(theBuffer,theBuffer->contentName))
		{
			return(true);
		}
	}
	else
	{
		if(ClearBuffer(theBuffer))
		{
			return(true);
		}
	}
	return(false);
}

bool EditorSaveBufferTo(EDITORBUFFER *theBuffer,char *thePath)
// save the text of theBuffer into thePath
// NOTE: the text will be saved even if it is not dirty
// if there is a problem, SetError, and return false
// NOTE: the buffer "clean" point is not moved by this call
// NOTE: certain problems may cause the text to be only partially saved
// thus corrupting the file
{
	return(SaveBuffer(theBuffer,thePath,false));
}

bool EditorSaveBufferAs(EDITORBUFFER *theBuffer,char *thePath)
// save the text of theBuffer into thePath, change theBuffer's name
// to the absolute version of thePath, and update its window if it has one
// NOTE: the text will be saved even if it is not dirty
// if there is a problem, SetError, and return false
// NOTE: certain problems may cause the text to be only partially saved
// thus corrupting the file
// NOTE: if after a save, the buffer's name becomes the same as that
// of another already existing buffer, the name change will fail, even though
// the contents of the buffer have been written into the file
// this is a little ugly, but all editors have to cope with this problem somehow.
{
	EDITORBUFFER
		*otherBuffer;
	char
		*absolutePath;

	if(SaveBuffer(theBuffer,thePath,true))							// save the file using the regular path, mark it as clean
	{
		if((absolutePath=CreateAbsolutePath(thePath)))				// attempt to make an absolute path out of the one given (this could fail if someone deleted the file we just saved before we get here!)
		{
			if(!(otherBuffer=LocateBuffer(absolutePath))||(otherBuffer==theBuffer))	// see if there is another buffer down this path
			{
				MDisposePtr(theBuffer->contentName);				// wipe out the old content name
				theBuffer->contentName=absolutePath;				// point it at the new path
				theBuffer->fromFile=true;							// linked to file now
				EditorReassignDocumentWindowTitle(theBuffer);		// make new title to reflect new name
				return(true);
			}
			else
			{
				SetError(localErrorFamily,errorMembers[DUPLICATENAME],errorDescriptions[DUPLICATENAME]);
			}
			MDisposePtr(absolutePath);
		}
	}
	return(false);
}

bool EditorSaveBuffer(EDITORBUFFER *theBuffer)
// save the text of theBuffer into its file if it has one
// if it does not have a file, this will fail
{
	if(theBuffer->fromFile)
	{
		return(SaveBuffer(theBuffer,theBuffer->contentName,true));	// save buffer, mark it as clean
	}
	SetError(localErrorFamily,errorMembers[NOTFROMFILE],errorDescriptions[NOTFROMFILE]);
	return(false);
}

EDITORBUFFER *EditorNewBuffer(char *bufferName)
// create a new buffer with the given name
// NOTE: if there is already a buffer with the given name, SetError
// if there is a problem, SetError, return NULL
{
	if(!LocateBuffer(bufferName))
	{
		return(CreateBuffer(bufferName));
	}
	else
	{
		SetError(localErrorFamily,errorMembers[DUPLICATENAME],errorDescriptions[DUPLICATENAME]);
	}
	return(NULL);
}

EDITORBUFFER *EditorOpenBuffer(char *thePath)
// open an editor buffer to the file at thePath
// if possible, open it, and return the buffer
// if there is a problem, set the error, and return NULL
// NOTE: if thePath points to a buffer that is already open,
// then that buffer is simply returned
{
	EDITORBUFFER
		*theBuffer;
	char
		*absolutePath;

	if((absolutePath=CreateAbsolutePath(thePath)))				// attempt to make an absolute path out of the one given
	{
		if((theBuffer=LocateBuffer(absolutePath)))
		{
			MDisposePtr(absolutePath);
			return(theBuffer);
		}
		else
		{
			if((theBuffer=CreateBuffer(absolutePath)))
			{
				if(LoadBuffer(theBuffer,absolutePath))
				{
					theBuffer->fromFile=true;
					MDisposePtr(absolutePath);
					return(theBuffer);
				}
				DisposeBuffer(theBuffer);
			}
		}
		MDisposePtr(absolutePath);
	}
	return(NULL);
}

void UnInitBuffers()
// undo whatever InitBuffers did
{
	while(startBuffer)
	{
		if(startBuffer->bufferHandles)
		{
			fprintf(stderr,"Buffer dying with handles\n");	// this is a bug because no one should have a handle to a buffer at this moment
		}
		EditorCloseBuffer(startBuffer);						// get rid of any buffers left lying about
	}
}

bool InitBuffers()
// initialize buffer handling
// if there is a problem, SetError, and return false
{
	startBuffer=NULL;										// no start buffer as of yet
	return(true);
}
