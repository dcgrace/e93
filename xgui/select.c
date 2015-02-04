// X windows selection handling
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

#define	SELECTIONRECEIVETIMEOUT		20*60		// amount of time to wait before deciding that the selection owner is not cooperating
#define	SELECTIONSENDTIMEOUT		5*60		// amount of time to wait before deciding that the selection requestor is not cooperating
#define	SELECTIONINCRTHRESHOLD		65536		// if the selection is larger than this amount, it is sent incrementally

static Bool NotifyPredicate(Display *theDisplay,XEvent *theEvent,char *arg)
// use this to wait for the notification that the selection has been converted
{
	if(theEvent->type==SelectionNotify&&theEvent->xselection.requestor==*((Window *)arg))
	{
		return(True);
	}
	return(False);
}

static bool GetSelectionNotify(Window theXWindow,XEvent *theEvent)
// wait here until either a time out, or until we get notification of
// selection data placed on a property of the given window
// return theEvent, or false if there is a timeout
{
	UINT32
		selectionTimer;
	bool
		gotEvent;

	selectionTimer=timer;							// reset to current time

	gotEvent=false;
	while(timer<(selectionTimer+SELECTIONRECEIVETIMEOUT)&&!gotEvent)	// wait here until the notification event arrives, or we time out
	{
		if(!(gotEvent=XCheckIfEvent(xDisplay,theEvent,NotifyPredicate,(char *)&theXWindow)))
		{
			pause();								// sleep a little while waiting, so as not to bog the machine
		}
	}
	return(gotEvent);
}

static Bool PropertyPredicate(Display *theDisplay,XEvent *theEvent,char *arg)
// use this to wait for the our property to change, so we know more data
// has arrived during incremental selection update
{
	if(theEvent->type==PropertyNotify&&theEvent->xproperty.window==*((Window *)arg))
	{
		return(True);
	}
	return(False);
}

static bool GetPropertyChangeNotify(Window theXWindow,XEvent *theEvent)
// wait here until either a time out, or until we get notification of
// property change in the given window (NOTE: property delete messages
// are read, and ignored here)
// return theEvent, or false if there is a timeout
{
	UINT32
		selectionTimer;
	bool
		gotEvent;

	selectionTimer=timer;							// reset to current time

	gotEvent=false;
	while(timer<(selectionTimer+SELECTIONRECEIVETIMEOUT)&&!gotEvent)	// wait here until the notification event arrives, or we time out
	{
		if(XCheckIfEvent(xDisplay,theEvent,PropertyPredicate,(char *)&theXWindow))
		{
			if(theEvent->xproperty.state==PropertyNewValue)
			{
				gotEvent=true;						// only have the event when it is a new value
			}
		}
		else
		{
			pause();								// sleep a little while waiting, so as not to bog the machine
		}
	}
	return(gotEvent);
}

static void FlushPropertyNotify(Display *theDisplay,Window theXWindow)
// remove all property notify events that are in the queue
// for theXWindow on theDisplay
{
	XEvent
		theEvent;

//	XSync(theDisplay,False);
	while(XCheckIfEvent(theDisplay,&theEvent,PropertyPredicate,(char *)&theXWindow))
		;
}

static bool GetPropertyDeleteNotify(Display *theDisplay,Window theXWindow,XEvent *theEvent)
// wait here until either a time out, or until we get notification of
// property delete in the given window (NOTE: property change messages
// are read, and ignored here)
// return theEvent, or false if there is a timeout
{
	UINT32
		selectionTimer;
	bool
		gotEvent;

	selectionTimer=timer;							// reset to current time

	gotEvent=false;
	while(timer<(selectionTimer+SELECTIONSENDTIMEOUT)&&!gotEvent)	// wait here until the notification event arrives, or we time out
	{
		if(XCheckIfEvent(theDisplay,theEvent,PropertyPredicate,(char *)&theXWindow))
		{
			if(theEvent->xproperty.state==PropertyDelete)
			{
				gotEvent=true;						// only have the event when it is a new value
			}
		}
		else
		{
			pause();								// sleep a little while waiting, so as not to bog the machine
		}
	}
	return(gotEvent);
}

static bool InhaleSelectionProperty(Window theXWindow,EDITORBUFFER *theClipboard,UINT32 *numBytesInhaled)
// Read all the bytes of the selection property that are arriving, and append them to
// theClipboard. If there is a problem, report it here, return false
{
	Atom
		returnType;
	unsigned long
		numItems,
		numItemsLeft,
		theOffset;
	int
		formatReturn;
	unsigned char
		*theData;
	bool
		done,
		fail;

	done=fail=false;
	theOffset=0;
	*numBytesInhaled=0;
	while(!done&&!fail)
	{
		if(XGetWindowProperty(xDisplay,theXWindow,selectionAtom,theOffset,16384,False,XA_STRING,&returnType,&formatReturn,&numItems,&numItemsLeft,&theData)==Success)
		{
			if(returnType==XA_STRING&&formatReturn==8)
			{
				(*numBytesInhaled)+=numItems;
				theOffset+=numItems>>2;
				ReplaceEditorText(theClipboard,theClipboard->textUniverse->totalBytes,theClipboard->textUniverse->totalBytes,(UINT8 *)theData,numItems);		// take the bytes we got, and insert them into the clipboard at the end
				done=!numItemsLeft;
			}
			else
			{
				ReportMessage("Got unexpected atom from selection owner: %d\n",returnType);
				fail=true;
			}
			XFree(theData);
		}
		else
		{
			ReportMessage("Failed to get selection property\n");
			fail=true;
		}
	}
	return(!fail);
}

static bool ImportINCRSelection(EDITORBUFFER *theClipboard)
// The selection that is being passed to us is so large that the server
// cannot handle it, so fall back to the INCR solution. This works by
// passing the selection incrementally between the clients, making sure
// never to overflow the server with data.
// If there is a problem getting the selection, report it here, and return false.
{
	XEvent
		theEvent;
	Atom
		returnType;
	unsigned long
		numItems,
		numItemsLeft;
	int
		formatReturn;
	UINT32
		totalBytes;
	unsigned char
		*incrData;
	bool
		done,
		fail;
	Window
		rootXWindow;
	UINT32
		numBytesInhaled;

	rootXWindow=((WINDOWLISTELEMENT *)rootMenuWindow->userData)->xWindow;
	fail=false;
	// read to get INCR value which tells how much data will be transferred
	if(XGetWindowProperty(xDisplay,rootXWindow,selectionAtom,0,1,False,incrAtom,&returnType,&formatReturn,&numItems,&numItemsLeft,&incrData)==Success)
	{
		FlushPropertyNotify(xDisplay,rootXWindow);						// get rid of any pending notify events
		XDeleteProperty(xDisplay,rootXWindow,selectionAtom);			// once the property is read, delete it (this tells sender that we are ready for more)

		if(returnType==incrAtom&&formatReturn==32)
		{
			totalBytes=*((UINT32 *)incrData);
			EditorStartReplace(theClipboard);
			ReplaceEditorText(theClipboard,0,theClipboard->textUniverse->totalBytes,NULL,0);	// clear the clipboard
			done=fail=false;
			while(!done&&!fail)
			{
				if(GetPropertyChangeNotify(rootXWindow,&theEvent))			// wait for it to change, to see if there is more data
				{
					if(InhaleSelectionProperty(rootXWindow,theClipboard,&numBytesInhaled))
					{
						if(numBytesInhaled==0)
						{
							done=true;									// done when property of 0 size is encountered
						}
					}
					else
					{
						fail=true;
					}
					XDeleteProperty(xDisplay,rootXWindow,selectionAtom);	// delete this, so we can go again
				}
				else
				{
					ReportMessage("Timed out waiting for reply from selection owner\n");
					fail=true;
				}
			}
			EditorEndReplace(theClipboard);
		}
		else
		{
			ReportMessage("Got unexpected atom from selection owner: %d\n",returnType);
			fail=true;
		}
		XFree(incrData);
	}
	else
	{
		ReportMessage("Failed to get selection property\n");
		fail=true;
	}
	return(!fail);
}

static bool ImportNormalSelection(EDITORBUFFER *theClipboard)
// The selection owner has decided to pass the selection to us in the
// normal way (used when the selection is small enough to be held by the server).
// If there is a problem getting the selection, report it here, and return false.
{
	Window
		rootXWindow;
	UINT32
		numBytesInhaled;
	bool
		fail;

	rootXWindow=((WINDOWLISTELEMENT *)rootMenuWindow->userData)->xWindow;
	EditorStartReplace(theClipboard);
	ReplaceEditorText(theClipboard,0,theClipboard->textUniverse->totalBytes,NULL,0);	// clear the clipboard
	fail=!InhaleSelectionProperty(rootXWindow,theClipboard,&numBytesInhaled);
	XDeleteProperty(xDisplay,rootXWindow,selectionAtom);								// get rid of the property
	EditorEndReplace(theClipboard);
	return(!fail);
}

bool ImportClipboard()
// When the editor is about to do a paste, or something else
// that requires it to look at the contents of the clipboard
// it will call this to possibly import outside contents into
// the current clipboard
// NOTE: this should return true if it succeeds, false if it fails
// NOTE: if no importing is done, this should still return true
{
	XEvent
		theEvent;
	Window
		theXWindow,
		rootXWindow;
	bool
		fail;
	Atom
		returnType;
	unsigned long
		numItems,
		numItemsLeft;
	int
		formatReturn;
	unsigned char
		*theData;
	EDITORBUFFER
		*theClipboard;

	fail=false;
	theXWindow=XGetSelectionOwner(xDisplay,XA_PRIMARY);		// see if we own the selection, if so, do not import it
	rootXWindow=((WINDOWLISTELEMENT *)rootMenuWindow->userData)->xWindow;
	if(theXWindow!=None&&theXWindow!=rootXWindow)			// someone else owns it, so import it
	{
		if((theClipboard=EditorStartImportClipboard()))
		{
			XConvertSelection(xDisplay,XA_PRIMARY,XA_STRING,selectionAtom,rootXWindow,CurrentTime);

			if(GetSelectionNotify(rootXWindow,&theEvent))			// wait until notified of selection placed on our window
			{
				if(theEvent.xselection.property==selectionAtom)		// if the one with the selection could convert it, then we have it now
				{
					if(XGetWindowProperty(xDisplay,rootXWindow,selectionAtom,0,0,False,AnyPropertyType,&returnType,&formatReturn,&numItems,&numItemsLeft,&theData)==Success)
					{
						if(returnType==XA_STRING&&formatReturn==8)
						{
							ImportNormalSelection(theClipboard);			// attempt to import the selection the traditional way
						}
						else
						{
							if(returnType==incrAtom&&formatReturn==32)		// see if other client wants to do this incrementally
							{
								ImportINCRSelection(theClipboard);			// attempt to import the selection the incremental way
							}
							else
							{
								ReportMessage("Unknown selection format (Atom=%d)\n",returnType);
								fail=true;
							}
						}
						XFree(theData);
					}
					else
					{
						ReportMessage("Failed to get selection property\n");
						fail=true;
					}
				}
				else
				{
					ReportMessage("Selection owner couldn't convert to string\n");
					fail=true;
				}
			}
			else
			{
				ReportMessage("Timed out waiting for reply from selection owner\n");
				fail=true;
			}
			EditorEndImportClipboard();
		}
	}
	return(!fail);
}

bool ExportClipboard()
// When the editor changes the contents of a clipboard, this is called
// so the GUI can export the clipboard into its current environment
// if there is a problem, return false
{
	XSetSelectionOwner(xDisplay,XA_PRIMARY,((WINDOWLISTELEMENT *)rootMenuWindow->userData)->xWindow,CurrentTime);		// let the world know that the selection is mine (yes, CurrentTime should not be used, but there is no good alternative, since scripts can change the clipboard at any time)
	return(true);
}

static void RefuseSelectionRequest(XEvent *theEvent)
// Complain to the requestor that we do not wish to fulfill his pitiful
// request for data
{
	XEvent
		notifyEvent;

	notifyEvent.xselection.type=SelectionNotify;
	notifyEvent.xselection.serial=0;
	notifyEvent.xselection.send_event=False;
	notifyEvent.xselection.display=theEvent->xselectionrequest.display;
	notifyEvent.xselection.requestor=theEvent->xselectionrequest.requestor;
	notifyEvent.xselection.selection=theEvent->xselectionrequest.selection;
	notifyEvent.xselection.target=theEvent->xselectionrequest.target;
	notifyEvent.xselection.property=None;
	notifyEvent.xselection.time=theEvent->xselectionrequest.time;
	XSendEvent(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,False,0,&notifyEvent);
}

static void HandleNormalSelectionRequest(XEvent *theEvent,EDITORBUFFER *theClipboard)
// handle sending the contents of the selection in the traditional way
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT8
		*theBuffer;
	XEvent
		notifyEvent;
	bool
		sentOk;

	sentOk=false;
	if((theBuffer=(UINT8 *)MNewPtr(theClipboard->textUniverse->totalBytes)))	// create a buffer to give to X
	{
		theChunk=theClipboard->textUniverse->firstChunkHeader;
		theOffset=0;
		if(ExtractUniverseText(theClipboard->textUniverse,theChunk,theOffset,theBuffer,theClipboard->textUniverse->totalBytes,&theChunk,&theOffset))	// read data from universe into buffer
		{
			XChangeProperty(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,theEvent->xselectionrequest.property,XA_STRING,8,PropModeReplace,(unsigned char *)theBuffer,theClipboard->textUniverse->totalBytes);

			notifyEvent.xselection.type=SelectionNotify;
			notifyEvent.xselection.serial=0;
			notifyEvent.xselection.send_event=False;
			notifyEvent.xselection.display=theEvent->xselectionrequest.display;
			notifyEvent.xselection.requestor=theEvent->xselectionrequest.requestor;
			notifyEvent.xselection.selection=theEvent->xselectionrequest.selection;
			notifyEvent.xselection.target=theEvent->xselectionrequest.target;
			notifyEvent.xselection.property=theEvent->xselectionrequest.property;
			notifyEvent.xselection.time=theEvent->xselectionrequest.time;
			XSendEvent(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,False,0,&notifyEvent);
			sentOk=true;
		}
		MDisposePtr(theBuffer);
	}
	if(!sentOk)
	{
		RefuseSelectionRequest(theEvent);
	}
}

static void HandleINCRSelectionRequest(XEvent *theEvent,EDITORBUFFER *theClipboard)
// handle sending the contents of the selection in the incremental way
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT8
		*theBuffer;
	XEvent
		notifyEvent;
	unsigned long
		bytesThisTime,
		bytesToDo;
	bool
		fail;

	bytesToDo=theClipboard->textUniverse->totalBytes;		// make local to hold total bytes, pass a pointer to ChangeProperty routine
	if((theBuffer=(UINT8 *)MNewPtr(SELECTIONINCRTHRESHOLD)))	// create a buffer to give to X
	{
		XSelectInput(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,PropertyChangeMask);
		FlushPropertyNotify(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor);		// get rid of any pending notify events
		XChangeProperty(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,theEvent->xselectionrequest.property,incrAtom,32,PropModeReplace,(unsigned char *)&bytesToDo,1);	// send INCR to requestor to let him know we will do it the long way

		notifyEvent.xselection.type=SelectionNotify;
		notifyEvent.xselection.serial=0;
		notifyEvent.xselection.send_event=False;
		notifyEvent.xselection.display=theEvent->xselectionrequest.display;
		notifyEvent.xselection.requestor=theEvent->xselectionrequest.requestor;
		notifyEvent.xselection.selection=theEvent->xselectionrequest.selection;
		notifyEvent.xselection.target=theEvent->xselectionrequest.target;
		notifyEvent.xselection.property=theEvent->xselectionrequest.property;
		notifyEvent.xselection.time=theEvent->xselectionrequest.time;
		XSendEvent(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,False,0,&notifyEvent);

		theChunk=theClipboard->textUniverse->firstChunkHeader;		// point to the start
		theOffset=0;
		fail=false;
		while(bytesToDo&&!fail)
		{
			if(bytesToDo>SELECTIONINCRTHRESHOLD)
			{
				bytesThisTime=SELECTIONINCRTHRESHOLD;
			}
			else
			{
				bytesThisTime=bytesToDo;
			}
			if(ExtractUniverseText(theClipboard->textUniverse,theChunk,theOffset,theBuffer,bytesThisTime,&theChunk,&theOffset))	// read data from universe into buffer
			{
				if(GetPropertyDeleteNotify(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,&notifyEvent))
				{
					XChangeProperty(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,theEvent->xselectionrequest.property,XA_STRING,8,PropModeReplace,(unsigned char *)theBuffer,bytesThisTime);
					bytesToDo-=bytesThisTime;
				}
				else
				{
					fail=true;
				}
			}
		}
		if(!fail)
		{
			if(GetPropertyDeleteNotify(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,&notifyEvent))
			{
				XChangeProperty(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,theEvent->xselectionrequest.property,XA_STRING,8,PropModeReplace,(unsigned char *)theBuffer,0);
			}
		}
		XSelectInput(theEvent->xselectionrequest.display,theEvent->xselectionrequest.requestor,0);	// no longer want events for this window
		MDisposePtr(theBuffer);
	}
	else
	{
		RefuseSelectionRequest(theEvent);
	}
}

void HandleSelectionRequest(XEvent *theEvent)
// Someone wants the contents of the selection (which we own)
// so, send the current clipboard contents to them
{
	EDITORBUFFER
		*theClipboard;

	if((theEvent->xselectionrequest.target==XA_STRING)&&(theClipboard=EditorStartExportClipboard()))
	{
		if(theClipboard->textUniverse->totalBytes>SELECTIONINCRTHRESHOLD)		// if too large, do it the incremental way
		{
			HandleINCRSelectionRequest(theEvent,theClipboard);
		}
		else
		{
			HandleNormalSelectionRequest(theEvent,theClipboard);
		}
		EditorEndExportClipboard();
	}
	else
	{
		RefuseSelectionRequest(theEvent);
	}
}
