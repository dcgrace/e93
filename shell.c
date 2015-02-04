// Shell interface for e93 (currently using John Ousterhout's Tcl)
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

static Tcl_AsyncHandler
	abortHandler;				// this allows us to check if the user has aborted during a Tcl script

void InsertBufferTaskData(EDITORBUFFER *theBuffer,UINT8 *theData,UINT32 numBytes)
// insert numBytes of theData into theBuffer, update any
// views of theBuffer that may need it
// if there is a problem, report it here
{
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;
	EDITORVIEW
		*currentView;
	UINT32
		tempPosition,
		startPosition,
		endPosition;
	UINT32
		startLine;
	CHUNKHEADER
		*startLineChunk;
	UINT32
		startLineOffset;

// run through the views, see which ones will need to be homed after the insert:
// The ones to home, are those that had the cursor in view at the time of the insert

	GetSelectionEndPositions(theBuffer->selectionUniverse,&startPosition,&endPosition);
	currentView=theBuffer->firstView;					// get first view of this buffer (if any)
	while(currentView)									// walk through all views, update each one as needed
	{
		GetEditorViewTextInfo(currentView,&topLine,&numLines,&leftPixel,&numPixels);
		PositionToLinePosition(theBuffer->textUniverse,startPosition,&startLine,&tempPosition,&startLineChunk,&startLineOffset);
		currentView->wantHome=(startLine>=topLine&&startLine<topLine+numLines);	// see if the cursor is on the view
		currentView=currentView->nextBufferView;		// locate next view of this buffer
	}

	EditorAuxInsert(theBuffer,theData,numBytes);		// drop the text into theBuffer

	GetSelectionEndPositions(theBuffer->selectionUniverse,&startPosition,&endPosition);	// get selection position after the insertion
// now home all of those views that need it
	currentView=theBuffer->firstView;					// get first view of this buffer (if any)
	while(currentView)									// walk through all views, update each one as needed
	{
		if(currentView->wantHome)
		{
			EditorHomeViewToRange(currentView,startPosition,endPosition,false,HT_NONE,HT_LENIENT);
		}
		currentView=currentView->nextBufferView;		// locate next view of this buffer
	}
}

bool tkHasGrab()
// determine if Tk has a window that has been "grabbed" for events
// It would be nice if this could be replaced with something which
// could call Tk directly.
{
	bool
		grab;
	int
		tclResult;
	const char
		*stringResult;
	Tcl_Obj
		*objPtr,
		*oldResult;		// keep result here while we do other things

	grab=false;

	oldResult=Tcl_GetObjResult(theTclInterpreter);				// hold the current result so it can be replaced when done
	Tcl_IncrRefCount(oldResult);								// don't let this get deleted
	objPtr=Tcl_NewStringObj("::override::grab current",-1);		// get the list of currently grabbed windows
	tclResult=Tcl_EvalObjEx(theTclInterpreter,objPtr,0);
	stringResult=Tcl_GetStringResult(theTclInterpreter);		// the list is returned here
	grab=((tclResult==TCL_OK)&&(strlen(stringResult)>0));		// there's a grab if the command succeeded, and the list is not empty
	Tcl_SetObjResult(theTclInterpreter,oldResult);				// put the old Tcl result back to cover our tacks and leave things unmolested
	Tcl_DecrRefCount(oldResult);								// let this get deleted when it needs to

	return(grab);
}

static bool Safe_Tcl_Eval(Tcl_Interp *interp, char *cmd, int *tclResult)
// Some Tcl commands can delete the text of the script which
// is running. If this happens, it is possible for things to get confused.
// For example: a menu command could try to delete itself, which would cause
// the script of the command to be deleted mid-execution.
// To solve this problem, the script is copied into a safe temporary place
// then executed, and finally deleted
// If there is a problem copying the script, this will return false, and not
// execute it. If this returns true, the result from the Tcl_Eval command will
// be placed in tclResult
{
	char
		*scriptCopy;

	if((scriptCopy=(char *)MNewPtr(strlen(cmd)+1)))
	{
		strcpy(scriptCopy,cmd);								// copy the command
		*tclResult=Tcl_Eval(theTclInterpreter,scriptCopy);	// execute it
		MDisposePtr(scriptCopy);							// get rid of the copy
		return(true);
	}
	return(false);
}

bool HandleBoundKeyEvent(UINT32 keyCode,UINT32 modifierValue)
// see if there is a bound key that matches the given code, and modifiers
// if there is, perform the script given by the bound key, and return true
// if there is not, return false
{
	EDITORKEYBINDING
		*theEntry;
	int
		tclResult;
	const char
		*stringResult;

	if((theEntry=LocateKeyBindingMatch(keyCode,modifierValue)))
	{
		ClearAbort();
		if(Safe_Tcl_Eval(theTclInterpreter,GetKeyBindingText(theEntry),&tclResult))
		{
			stringResult=Tcl_GetStringResult(theTclInterpreter);
			if((tclResult!=TCL_OK)&&stringResult[0])
			{
				ReportMessage("Bound key execution error:\n%.256s\n",stringResult);
			}
		}
		else
		{
			ReportMessage("Bound key execution error:\nOut of memory\n");
		}
		return(true);
	}
	return(false);
}

void HandleMenuEvent(EDITORMENU *theMenu)
// when a menu event arrives, this is called (by the menu manager of the GUI) to handle the menu
{
	int
		tclResult;
	const char
		*stringResult;

	ClearAbort();
	if(Safe_Tcl_Eval(theTclInterpreter,GetEditorMenuDataText(theMenu),&tclResult))
	{
		stringResult=Tcl_GetStringResult(theTclInterpreter);
		if((tclResult!=TCL_OK)&&stringResult[0])
		{
			ReportMessage("Menu execution error:\n%.256s\n",stringResult);
		}
	}
	else
	{
		ReportMessage("Menu execution error:\nOut of memory\n");
	}
}

void HandleNonStandardControlEvent(char *theString)
// when the GUI needs to implement some GUI specific control, it can call this to execute shell
// commands for that control
{
	int
		tclResult;
	const char
		*stringResult;

	ClearAbort();
	tclResult=Tcl_Eval(theTclInterpreter,theString);
	stringResult=Tcl_GetStringResult(theTclInterpreter);
	if((tclResult!=TCL_OK)&&stringResult[0])
	{
		ReportMessage("Execution error:\n%.256s\n",stringResult);
	}
}

#define MAXSCROLLSPEED	2

void HandleViewEvent(VIEWEVENT *theEvent)
// high level view events are handled here
{
	UINT32
		topLine,
		numLines,
		numPixels;
	INT32
		leftPixel;
	UINT8
		theKey;
	static UINT32
		scrollTimeout;
	static UINT32
		scrollSpeed;
	UINT16
		trackMode;							// tells editor how to place cursor when cursor position keys arrive
	EDITORKEY
		*keyEventData;
	VIEWPOSEVENTDATA
		*posEventData;
	VIEWCLICKEVENTDATA
		*clickEventData;

	keyEventData=NULL;						// keep compiler quiet
	switch(theEvent->eventType)
	{
		case VET_KEYDOWN:
		case VET_KEYREPEAT:
			keyEventData=(EDITORKEY *)theEvent->eventData;
			if(keyEventData->isVirtual)
			{
				switch(keyEventData->keyCode)
				{
					case VVK_SCROLLUP:
						if(theEvent->eventType==VET_KEYDOWN)
						{
							scrollSpeed=1;
							scrollTimeout=0;
						}
						EditorVerticalScroll(theEvent->theView,-((INT32)scrollSpeed));
						if(scrollTimeout++>3)
						{
							if(scrollSpeed<MAXSCROLLSPEED)
							{
								scrollSpeed++;
							}
							scrollTimeout=0;
						}
						break;
					case VVK_SCROLLDOWN:
						if(theEvent->eventType==VET_KEYDOWN)
						{
							scrollSpeed=1;
							scrollTimeout=0;
						}
						EditorVerticalScroll(theEvent->theView,scrollSpeed);
						if(scrollTimeout++>3)
						{
							if(scrollSpeed<MAXSCROLLSPEED)
							{
								scrollSpeed++;
							}
							scrollTimeout=0;
						}
						break;
					case VVK_DOCUMENTPAGEUP:
						EditorVerticalScrollByPages(theEvent->theView,-1);
						break;
					case VVK_DOCUMENTPAGEDOWN:
						EditorVerticalScrollByPages(theEvent->theView,1);
						break;
					case VVK_SCROLLLEFT:
						EditorHorizontalScroll(theEvent->theView,-HORIZONTALSCROLLTHRESHOLD);
						break;
					case VVK_SCROLLRIGHT:
						EditorHorizontalScroll(theEvent->theView,HORIZONTALSCROLLTHRESHOLD);
						break;
					case VVK_DOCUMENTPAGELEFT:
						EditorHorizontalScrollByPages(theEvent->theView,-1);
						break;
					case VVK_DOCUMENTPAGERIGHT:
						EditorHorizontalScrollByPages(theEvent->theView,1);
						break;
					case VVK_RETURN:
						EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
						EditorAutoIndent(theEvent->theView->parentBuffer);
						EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
						break;
					case VVK_TAB:
						theKey='\t';				// insert a tab
						EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
						EditorInsert(theEvent->theView->parentBuffer,&theKey,1);
						EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
						break;
					case VVK_LEFTARROW:
						if(!(keyEventData->modifiers&EEM_CTL))
						{
							if(!(keyEventData->modifiers&EEM_MOD0))
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorMoveCursor(theEvent->theView,RPM_mBACKWARD|RPM_CHAR);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorMoveSelection(theEvent->theView,RPM_mBACKWARD|RPM_CHAR);
									EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_SEMISTRICT,HT_SEMISTRICT);
								}
							}
							else
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorMoveCursor(theEvent->theView,RPM_mBACKWARD|RPM_LINEEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorMoveSelection(theEvent->theView,RPM_mBACKWARD|RPM_LINEEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_SEMISTRICT,HT_SEMISTRICT);
								}
							}
						}
						else
						{
							if(!(keyEventData->modifiers&EEM_SHIFT))
							{
								EditorMoveCursor(theEvent->theView,RPM_mBACKWARD|RPM_WORD);
								EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
							}
							else
							{
								EditorMoveSelection(theEvent->theView,RPM_mBACKWARD|RPM_WORD);
								EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_SEMISTRICT,HT_SEMISTRICT);
							}
						}
						break;
					case VVK_RIGHTARROW:
						if(!(keyEventData->modifiers&EEM_CTL))
						{
							if(!(keyEventData->modifiers&EEM_MOD0))
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorMoveCursor(theEvent->theView,RPM_CHAR);
									EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorMoveSelection(theEvent->theView,RPM_CHAR);
									EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_SEMISTRICT,HT_SEMISTRICT);
								}
							}
							else
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorMoveCursor(theEvent->theView,RPM_LINEEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorMoveSelection(theEvent->theView,RPM_LINEEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_SEMISTRICT,HT_SEMISTRICT);
								}
							}
						}
						else
						{
							if(!(keyEventData->modifiers&EEM_SHIFT))
							{
								EditorMoveCursor(theEvent->theView,RPM_WORD);
								EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
							}
							else
							{
								EditorMoveSelection(theEvent->theView,RPM_WORD);
								EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_SEMISTRICT,HT_SEMISTRICT);
							}
						}
						break;
					case VVK_UPARROW:
						if(!(keyEventData->modifiers&EEM_CTL))
						{
							if(!(keyEventData->modifiers&EEM_MOD0))
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorMoveCursor(theEvent->theView,RPM_mBACKWARD|RPM_LINE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorMoveSelection(theEvent->theView,RPM_mBACKWARD|RPM_LINE);
									EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_SEMISTRICT,HT_SEMISTRICT);
								}
							}
							else
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorMoveCursor(theEvent->theView,RPM_mBACKWARD|RPM_PAGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorMoveSelection(theEvent->theView,RPM_mBACKWARD|RPM_PAGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_LENIENT,HT_LENIENT);
								}
							}
						}
						else
						{
							EditorVerticalScroll(theEvent->theView,-1);
						}
						break;
					case VVK_DOWNARROW:
						if(!(keyEventData->modifiers&EEM_CTL))
						{
							if(!(keyEventData->modifiers&EEM_MOD0))
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorMoveCursor(theEvent->theView,RPM_LINE);
									EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorMoveSelection(theEvent->theView,RPM_LINE);
									EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_SEMISTRICT,HT_SEMISTRICT);
								}
							}
							else
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorMoveCursor(theEvent->theView,RPM_PAGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorMoveSelection(theEvent->theView,RPM_PAGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_LENIENT,HT_LENIENT);
								}
							}
						}
						else
						{
							EditorVerticalScroll(theEvent->theView,1);
						}
						break;
					case VVK_BACKSPACE:
						if(!(keyEventData->modifiers&EEM_CTL)&&!(keyEventData->modifiers&EEM_MOD1))
						{
							if(!(keyEventData->modifiers&EEM_MOD0))
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_mBACKWARD|RPM_CHAR);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_CHAR);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
							}
							else
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_LINEEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_mBACKWARD|RPM_LINEEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
							}
						}
						else
						{
							if(!(keyEventData->modifiers&EEM_MOD0))
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_mBACKWARD|RPM_WORD);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_WORD);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
							}
							else
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_DOCEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_mBACKWARD|RPM_DOCEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
							}
						}
						break;
					case VVK_DELETE:
						if(!(keyEventData->modifiers&EEM_CTL)&&!(keyEventData->modifiers&EEM_MOD1))
						{
							if(!(keyEventData->modifiers&EEM_MOD0))
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_CHAR);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_mBACKWARD|RPM_CHAR);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
							}
							else
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_mBACKWARD|RPM_LINEEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_LINEEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
							}
						}
						else
						{
							if(!(keyEventData->modifiers&EEM_MOD0))
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_WORD);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_mBACKWARD|RPM_WORD);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
							}
							else
							{
								if(!(keyEventData->modifiers&EEM_SHIFT))
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_mBACKWARD|RPM_DOCEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
								else
								{
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
									EditorDelete(theEvent->theView,RPM_DOCEDGE);
									EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
								}
							}
						}
						break;
					case VVK_HOME:
						if(!(keyEventData->modifiers&EEM_MOD0))
						{
							if(!(keyEventData->modifiers&EEM_SHIFT))
							{
								EditorMoveCursor(theEvent->theView,RPM_mBACKWARD|RPM_LINEEDGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
							}
							else
							{
								EditorExpandNormalSelection(theEvent->theView,RPM_mBACKWARD|RPM_LINEEDGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
							}
						}
						else
						{
							if(!(keyEventData->modifiers&EEM_SHIFT))
							{
								EditorMoveCursor(theEvent->theView,RPM_mBACKWARD|RPM_DOCEDGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
							}
							else
							{
								EditorExpandNormalSelection(theEvent->theView,RPM_mBACKWARD|RPM_DOCEDGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
							}
						}
						break;
					case VVK_END:
						if(!(keyEventData->modifiers&EEM_MOD0))
						{
							if(!(keyEventData->modifiers&EEM_SHIFT))
							{
								EditorMoveCursor(theEvent->theView,RPM_LINEEDGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
							}
							else
							{
								EditorExpandNormalSelection(theEvent->theView,RPM_LINEEDGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
							}
						}
						else
						{
							if(!(keyEventData->modifiers&EEM_SHIFT))
							{
								EditorMoveCursor(theEvent->theView,RPM_DOCEDGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
							}
							else
							{
								EditorExpandNormalSelection(theEvent->theView,RPM_DOCEDGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
							}
						}
						break;
					case VVK_PAGEUP:
						if(keyEventData->modifiers&EEM_CTL)
						{
							EditorVerticalScrollByPages(theEvent->theView,-1);
						}
						else
						{
							if(!(keyEventData->modifiers&EEM_SHIFT))
							{
								EditorMoveCursor(theEvent->theView,RPM_mBACKWARD|RPM_PAGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);
							}
							else
							{
								EditorMoveSelection(theEvent->theView,RPM_mBACKWARD|RPM_PAGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_LENIENT,HT_LENIENT);
							}
						}
						break;
					case VVK_PAGEDOWN:
						if(keyEventData->modifiers&EEM_CTL)
						{
							EditorVerticalScrollByPages(theEvent->theView,1);
						}
						else
						{
							if(!(keyEventData->modifiers&EEM_SHIFT))
							{
								EditorMoveCursor(theEvent->theView,RPM_PAGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,true,HT_LENIENT,HT_LENIENT);
							}
							else
							{
								EditorMoveSelection(theEvent->theView,RPM_PAGE);
								EditorHomeViewToSelectionEdge(theEvent->theView,!theEvent->theView->parentBuffer->currentIsStart,HT_LENIENT,HT_LENIENT);
							}
						}
						break;
					case VVK_UNDO:
						if(EditorUndo(theEvent->theView->parentBuffer))
						{
							EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
						}
						else
						{
							EditorBeep();
						}
						break;
					case VVK_REDO:
						if(EditorRedo(theEvent->theView->parentBuffer))
						{
							EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
						}
						else
						{
							EditorBeep();
						}
						break;
					case VVK_UNDOTOGGLE:
						if(EditorToggleUndo(theEvent->theView->parentBuffer))
						{
							EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
						}
						else
						{
							EditorBeep();
						}
						break;
					case VVK_CUT:
						EditorCut(theEvent->theView->parentBuffer,EditorGetCurrentClipboard());
						EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
						break;
					case VVK_COPY:
						EditorCopy(theEvent->theView->parentBuffer,EditorGetCurrentClipboard());
						break;
					case VVK_PASTE:
						EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
						EditorColumnarPaste(theEvent->theView->parentBuffer,theEvent->theView,EditorGetCurrentClipboard());
						EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
						break;
				}
			}
			else
			{
				theKey=(UINT8)keyEventData->keyCode;
				if(keyEventData->modifiers&EEM_MOD0)		// check for bound keys
				{
					switch(theKey)
					{
						case 'j':
							UniverseSanityCheck(theEvent->theView->parentBuffer->textUniverse);
							break;
						case 'a':
							EditorSelectAll(theEvent->theView->parentBuffer);
							break;
						case 'y':
							if(EditorRedo(theEvent->theView->parentBuffer))
							{
								EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
							}
							else
							{
								EditorBeep();
							}
							break;
						case 'u':
							if(EditorUndo(theEvent->theView->parentBuffer))
							{
								EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
							}
							else
							{
								EditorBeep();
							}
							break;
						case 'z':
							if(EditorToggleUndo(theEvent->theView->parentBuffer))
							{
								EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
							}
							else
							{
								EditorBeep();
							}
							break;
						case 'x':
							EditorCut(theEvent->theView->parentBuffer,EditorGetCurrentClipboard());
							EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
							break;
						case 'c':
							EditorCopy(theEvent->theView->parentBuffer,EditorGetCurrentClipboard());
							break;
						case 'v':
							EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
							EditorColumnarPaste(theEvent->theView->parentBuffer,theEvent->theView,EditorGetCurrentClipboard());
							EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);
							break;
					}
				}
				else
				{
					EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_SEMISTRICT,HT_SEMISTRICT);	// if position was off the view, put it well back on
					EditorInsert(theEvent->theView->parentBuffer,&theKey,1);
					EditorHomeViewToSelectionEdge(theEvent->theView,false,HT_LENIENT,HT_LENIENT);	// now scroll into position
				}
			}
			break;
		case VET_POSITIONVERTICAL:
			posEventData=(VIEWPOSEVENTDATA *)theEvent->eventData;
			GetEditorViewTextInfo(theEvent->theView,&topLine,&numLines,&leftPixel,&numPixels);
			SetViewTopLeft(theEvent->theView,posEventData->position,leftPixel);
			break;
		case VET_POSITIONHORIZONTAL:
			posEventData=(VIEWPOSEVENTDATA *)theEvent->eventData;
			GetEditorViewTextInfo(theEvent->theView,&topLine,&numLines,&leftPixel,&numPixels);
			SetViewTopLeft(theEvent->theView,topLine,posEventData->position);
			break;
		case VET_CLICK:
		case VET_CLICKHOLD:
			clickEventData=(VIEWCLICKEVENTDATA *)theEvent->eventData;
			trackMode=0;								// default place mode
			if(theEvent->eventType==VET_CLICKHOLD)		// see if still tracking
			{
				trackMode|=TM_mREPEAT;
			}
			if(clickEventData->modifiers&EEM_SHIFT)		// attempting to add to current selection?
			{
				trackMode|=TM_mCONTINUE;
			}
			if((clickEventData->modifiers&EEM_MOD0)||(clickEventData->keyCode==2))
			{
				trackMode|=TM_mCOLUMNAR;
			}
			if((clickEventData->modifiers&EEM_MOD1)||(clickEventData->keyCode==1))
			{
				trackMode|=TM_mHAND;
			}
			switch((clickEventData->modifiers&EEM_STATE0)>>EES_STATE0)
			{
				case 0:
					trackMode|=TM_CHAR;
					break;
				case 1:
					trackMode|=TM_WORD;
					break;
				case 2:
					trackMode|=TM_LINE;
					break;
				default:
					trackMode|=TM_ALL;
					break;
			}
			EditorTrackViewPointer(theEvent->theView,clickEventData->xClick,clickEventData->yClick,trackMode);
			break;
	}
	ResetEditorViewCursorBlink(theEvent->theView);	// reset cursor blinking (if we even have a cursor that blinks)
}

bool HandleShellCommand(char *theCommand,int argc,char *argv[])
// sometimes things in the gui request that the shell should perform
// certain operations. this is the interface to allow that to happen
// If the operation has some sort of failure (defined by the operation)
// this will report it (if there is a message) and return false
{
	char
		*theList;
	int
		tclResult;
	const char
		*stringResult;

	theList=Tcl_Merge(argc,argv);
	ClearAbort();
	tclResult=Tcl_VarEval(theTclInterpreter,"ShellCommand ",theCommand," ",theList,(char *)NULL);
	Tcl_Free(theList);
	stringResult=Tcl_GetStringResult(theTclInterpreter);
	if(tclResult!=TCL_OK)
	{
		if(stringResult[0])
		{
			ReportMessage("Shell command execution error:\n%.256s\n",stringResult);
		}
		return(false);
	}
	return(true);
}

static bool ExecuteStartupScript(Tcl_Interp *theInterpreter)
// Locate the startup script file, open, and execute it
// if there is any problem, report it to stderr, and return false
{
	Tcl_SourceRCFile(theInterpreter);

	return true;
}

static int AbortProc(ClientData theClientData,Tcl_Interp *theInterpreter,int theCode)
// this procedure will be enabled if we discover that an abort attempt has been made
// while executing Tcl script, it will change the return code of the previously
// executed Tcl command, so that we can abort
{
	Tcl_ResetResult(theInterpreter);
	Tcl_AppendResult(theInterpreter,"User Aborted",NULL);
	return(TCL_ERROR);
}

static void TraceCheckAbortProc(ClientData theClientData,Tcl_Interp *theInterpreter,int theLevel,char *theCommand,Tcl_CmdProc *theProc,ClientData cmdClientData,int argc,char *argv[])
// This is a small trick on Tcl. We tell it we want to trace, but really, we want
// to check to see if the user is trying to abort the execution of a script.
// So, every time we are called, we check to see if the user is aborting, and
// if so, mark the AbortProc so that it will run
{
	if(CheckAbort())
	{
		Tcl_AsyncMark((Tcl_AsyncHandler)theClientData);
	}
}

static void UnSetUpTclAbortHandling(Tcl_Interp *theInterpreter)
// undo what SetUpTclAbortHandling did
{
	Tcl_AsyncDelete(abortHandler);
}

static bool SetUpTclAbortHandling(Tcl_Interp *theInterpreter)
// set up tcl so that it manages to call CheckAbort, and if CheckAbort
// returns true, then we force an error in the Tcl control Flow
// with a result string of "Abort"
{
	if((abortHandler=Tcl_AsyncCreate(AbortProc,NULL)))
	{
		if(true) // Tcl_CreateTrace(theInterpreter,INT_MAX,TraceCheckAbortProc,(ClientData)abortHandler))
		{
			return(true);
		}
		Tcl_AsyncDelete(abortHandler);
	}
	return(false);
}

void ShellDoBackground()
// handle Tcl's event loop
{
	// tell Tcl to check for any events it may care about
	while(Tcl_DoOneEvent(TCL_ALL_EVENTS|TCL_DONT_WAIT));
}

static void UnInitTclAndTk()
// Undo what InitTclAndTk did
{
	UnSetUpTclAbortHandling(theTclInterpreter);
	UnInitChannels(theTclInterpreter);
	Tcl_DeleteInterp(theTclInterpreter);	// have TCL clean up
	// Tcl_Exit(0);
}

static bool InitTclAndTk()
// setup things for Tcl/Tk
{
	char
		scriptPath[MAXPATHNAMELENGTH];

	if(LocateStartupScript(scriptPath))
	{
		ClearAbort();

		Tcl_SetVar(theTclInterpreter,"SCRIPTPATH",scriptPath,TCL_GLOBAL_ONLY);
		Tcl_SetVar(theTclInterpreter,"tcl_rcFileName",scriptPath,TCL_GLOBAL_ONLY);

		if(InitChannels(theTclInterpreter))
		{
			if(Tcl_Init(theTclInterpreter)==TCL_OK)
			{
				char
					cmnd[255];

				sprintf(cmnd, "set argv {-use %s -geometry 0x0+0+0}", GetMainWindowID());
				Tcl_Eval(theTclInterpreter, cmnd);					// tell Tk to use our window as its main window and make TK's window really small

				if(Tk_Init(theTclInterpreter)==TCL_OK)
				{
					Tcl_StaticPackage(theTclInterpreter, "Tk", Tk_Init, Tk_SafeInit);
					Tcl_Eval(theTclInterpreter,"wm withdraw .");	// tell Tk to NOT show its main window (would be better if it never existed, but this will do)

					if(SetUpTclAbortHandling(theTclInterpreter))
					{
						return true;
					}
				}
				else
				{
					fprintf(stderr,"Failed to Tk_Init(): %s\n",Tcl_GetStringResult(theTclInterpreter));
				}
			}
			else
			{
				fprintf(stderr,"Failed to Tcl_Init(): %s\n",Tcl_GetStringResult(theTclInterpreter));
			}
			UnInitChannels(theTclInterpreter);
		}
		else
		{
			fprintf(stderr,"Failed to redirect Tcl's stdout and stderr\n");
		}
		Tcl_DeleteInterp(theTclInterpreter);						// have TCL clean up
		Tcl_Exit(0);
	}

	return false;
}

static int
	g_argc;
static char
	**g_argv;

// called from Tcl's Tcl_Main() because we told it to in Tcl_AppInit()
void Tcl_MainLoop()
// this is where the editor shell gets and handles events
// it actually defers to the gui level to call us back with events
{
	UINT32
		numPointers;

	EditorClearModal();

	if(InitTclAndTk())
	{
		if(InitKeyBindingTable())
		{
			if(InitSyntaxMaps())
			{
				if(InitBuffers())
				{
					if(InitClipboard())
					{
						if(CreateEditorShellCommands(theTclInterpreter))
						{
							if(AddSupplementalShellCommands(theTclInterpreter))
							{
								if(ExecuteStartupScript(theTclInterpreter))			// deal with the start-up script, leave if there is a problem
								{
									EditorEventLoop(g_argc,g_argv);					// pass initial parameters
								}
							}
							else
							{
								fprintf(stderr,"Failed to add supplemental shell commands\n");
							}
						}
						else
						{
							fprintf(stderr,"Failed to create editor shell commands\n");
						}
						UnInitClipboard();
					}
					else
					{
						fprintf(stderr,"Failed to initialize clipboard\n");
					}
					UnInitBuffers();
				}
				else
				{
					fprintf(stderr,"Failed to initialize buffers\n");
				}
				UnInitSyntaxMaps();
			}
			else
			{
				fprintf(stderr,"Failed to syntax maps\n");
			}
			UnInitKeyBindingTable();
		}
		else
		{
			fprintf(stderr,"Failed to initialize key bindings\n");
		}
		UnInitTclAndTk();
	}
	else
	{
		fprintf(stderr,"Failed to initialize Tcl/Tk\n");
	}
	
	UnInitEnvironment();
	UnInitErrors();
	EarlyUnInit();
	if((numPointers=MGetNumAllocatedPointers()))
	{
		fprintf(stderr,"Had %d pointer(s) allocated on exit\n",numPointers);
	}
}

// called from Tcl's Tcl_Main() because we told it to in ShellLoop()
int Tcl_AppInit(Tcl_Interp *interp)
{
	// Tcl_Main() created the interpreter, save it for our use
	theTclInterpreter=interp;

	// tell Tcl to set our Tcl_MainLoop() as it's main loop
	Tcl_SetMainLoop(Tcl_MainLoop);

	return TCL_OK;
}

void ShellLoop(int argc,char *argv[])
// this is where the editor shell gets and handles events
// it actually defers to the gui level to call us back with events
{
	// save argc and argv for later
	g_argc = argc;
	g_argv = argv;

	// don't pass our argc and argv to Tcl, make new empty ones for Tcl
	static char
		*tcl_argv[1];
	static char
		*arg = argv[0];

	tcl_argv[0] = arg;

	// call Tcl's main, telling it to call us back at Tcl_AppInit()
	Tcl_Main(1 , tcl_argv, Tcl_AppInit);
}
