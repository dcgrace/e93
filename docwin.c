// High level document window stuff
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

EDITORWINDOW *LocateEditorDocumentWindow(char *bufferName)
// attempt to locate a preexisting document window that
// is open to absolutePath, if one is found, return it
// if none is located, return NULL
{
	EDITORBUFFER
		*theBuffer;

	if((theBuffer=LocateBuffer(bufferName)))
	{
		return(theBuffer->theWindow);
	}
	return(NULL);
}

void EditorReassignDocumentWindowTitle(EDITORBUFFER *theBuffer)
// if theBuffer has a window associated with it, assign the
// window's title based on theBuffer's contentName
// if there is no window, do nothing
{
	char
		*windowTitle;

	if(theBuffer->theWindow)
	{
		if(theBuffer->fromFile)
		{
			if((windowTitle=CreateWindowTitleFromPath(theBuffer->contentName)))
			{
				SetDocumentWindowTitle(theBuffer->theWindow,windowTitle);		// change the window's title
				MDisposePtr(windowTitle);										// get rid of memory
			}
			else
			{
				SetDocumentWindowTitle(theBuffer->theWindow,theBuffer->contentName);		// had problem getting new name, use path
			}
		}
		else
		{
			SetDocumentWindowTitle(theBuffer->theWindow,theBuffer->contentName);			// change the window's title
		}
	}
}

EDITORWINDOW *EditorOpenDocumentWindow(EDITORBUFFER *theBuffer,EDITORRECT *theRect,char *fontName,UINT32 tabSize,EDITORCOLOR foreground,EDITORCOLOR background)
// create a new document edit window for theBuffer, place it as the front-most
// NOTE: if there is already a document window for theBuffer
// place it front-most, and return it
// if there is a problem, SetError, return NULL
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	char
		*titleToDelete,
		*windowTitle;

	if(theBuffer->theWindow)
	{
		theView=GetDocumentWindowCurrentView(theBuffer->theWindow);
		SetEditorDocumentWindowRect(theBuffer->theWindow,theRect);		// adjust the rectangle
		SetEditorViewStyleFont(theView,0,fontName);						// and the font
		SetEditorViewTabSize(theView,tabSize);							// and the tabsize
		SetEditorViewStyleForegroundColor(theView,0,foreground);		// set colors for style 0
		SetEditorViewStyleBackgroundColor(theView,0,background);
		SetEditorDocumentWindowForegroundColor(theBuffer->theWindow,foreground);	// set the new colors
		SetEditorDocumentWindowBackgroundColor(theBuffer->theWindow,background);
		SetTopDocumentWindow(theBuffer->theWindow);						// place it on top
	}
	else
	{
		titleToDelete=NULL;
		if(theBuffer->fromFile)
		{
			titleToDelete=CreateWindowTitleFromPath(theBuffer->contentName);
		}
		if(titleToDelete)
		{
			windowTitle=titleToDelete;
		}
		else
		{
			windowTitle=theBuffer->contentName;
		}
		if((theWindow=OpenDocumentWindow(theBuffer,theRect,windowTitle,fontName,tabSize,foreground,background)))
		{
			theBuffer->theWindow=theWindow;
		}
		if(titleToDelete)
		{
			MDisposePtr(titleToDelete);
		}
	}
	return(theBuffer->theWindow);
}

void EditorCloseDocumentWindow(EDITORWINDOW *theWindow)
// close the given edit window (do not destroy the buffer it is tied to)
// this is not allowed to fail
{
	theWindow->theBuffer->theWindow=NULL;					// point the buffer that had this window to NULL
	CloseDocumentWindow(theWindow);							// get rid of the window
}
