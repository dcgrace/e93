// Dialog window handling
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

static void ChangeFocus(DIALOG *theDialog,DIALOGITEM *newItem)
// change the focus of theDialog to newItem
// if the focus is already at new item, do nothing
// NOTE: this is DIALOG ITEM focus, not X window focus
{
	DIALOGITEM
		*oldItem;

	oldItem=theDialog->focusItem;
	if(oldItem!=newItem)							// make sure they are not the same, or both NULL
	{
		theDialog->focusItem=newItem;
		if(oldItem)
		{
			if(oldItem->focusChangeProc)
			{
				oldItem->focusChangeProc(oldItem);	// tell him about it
			}
		}
		if(newItem)
		{
			if(newItem->focusChangeProc)
			{
				newItem->focusChangeProc(newItem);	// tell him about it
			}
		}
	}
}

static DIALOGITEM *LocateForwardFocusItem(DIALOG *theDialog)
// look forward from the dialog item which currently has the focus
// and return the next item which should get focus
// if no items require focus, return NULL
{
	DIALOGITEM
		*currentItem,
		*nextFocusItem;

	if((currentItem=theDialog->focusItem))
	{
		currentItem=currentItem->nextItem;			// begin looking one beyond the old focus item (if there was an old one)
	}
	nextFocusItem=NULL;
	while(currentItem&&!nextFocusItem)
	{
		if(currentItem->wantFocus)
		{
			nextFocusItem=currentItem;				// this is the next one that wants the focus
		}
		currentItem=currentItem->nextItem;
	}
	currentItem=theDialog->firstItem;				// start over at the beginning
	while(currentItem&&!nextFocusItem)
	{
		if(currentItem->wantFocus)
		{
			nextFocusItem=currentItem;				// this is the next one that wants the focus
		}
		currentItem=currentItem->nextItem;
	}
	return(nextFocusItem);
}

static void ForwardFocus(DIALOG *theDialog)
// push the focus forward one notch, and alert the involved parties
// NOTE: this is DIALOG ITEM focus, not X window focus
{
	DIALOGITEM
		*nextFocusItem;

	nextFocusItem=LocateForwardFocusItem(theDialog);	// this can return NULL, which will be passed to ChangeFocus to select nothing
	ChangeFocus(theDialog,nextFocusItem);
}

static void LinkItemToDialog(DIALOG *theDialog,DIALOGITEM *newItem)
// link newItem to the end of the linked list of dialog items in theDialog
{
	newItem->theDialog=theDialog;				// point back at the dialog
	newItem->nextItem=NULL;
	if((newItem->previousItem=theDialog->lastItem))
	{
		theDialog->lastItem->nextItem=newItem;
		theDialog->lastItem=newItem;
	}
	else
	{
		theDialog->firstItem=theDialog->lastItem=newItem;
	}
}

static void UnlinkItemFromDialog(DIALOGITEM *theDialogItem)
// unlink theDialogItem from the dialog list
{
	if(theDialogItem->previousItem)
	{
		theDialogItem->previousItem->nextItem=theDialogItem->nextItem;
	}
	else
	{
		theDialogItem->theDialog->firstItem=theDialogItem->nextItem;
	}
	if(theDialogItem->nextItem)
	{
		theDialogItem->nextItem->previousItem=theDialogItem->previousItem;
	}
	else
	{
		theDialogItem->theDialog->lastItem=theDialogItem->previousItem;
	}
}

DIALOGITEM *NewDialogItem(DIALOG *theDialog,bool (*createProc)(DIALOGITEM *theItem,void *itemDescriptor),void *itemDescriptor)
// create a new dialog item, link it to theDialog, call the item's creation procedure
// createProc must not be NULL and must return true for this to succeed
// if there is a problem, SetError, return NULL
{
	DIALOGITEM
		*theDialogItem;

	if((theDialogItem=(DIALOGITEM *)MNewPtrClr(sizeof(DIALOGITEM))))	// make new item, clear any procedure pointers to NULL
	{
		LinkItemToDialog(theDialog,theDialogItem);
		if(createProc(theDialogItem,itemDescriptor))
		{
			return(theDialogItem);
		}
		UnlinkItemFromDialog(theDialogItem);
		MDisposePtr(theDialogItem);
	}
	return(NULL);
}

void DisposeDialogItem(DIALOGITEM *theDialogItem)
// unlink, and dispose of a dialog item, created by NewDialogItem
{
	if(theDialogItem->disposeProc)
	{
		theDialogItem->disposeProc(theDialogItem);
	}
	UnlinkItemFromDialog(theDialogItem);
	MDisposePtr(theDialogItem);
}

static DIALOG *CreateDialog(EDITORRECT *theRect,char *theTitle,EDITORCOLOR background)
// open a new empty dialog of the given size, with the given title
// it should not be updated until all the items have been added to it
// if there is a problem, SetError, return NULL
{
	DIALOG
		*theDialog;
	EDITORWINDOW
		*theWindow;
	WINDOWLISTELEMENT
		*theWindowElement;
	XSetWindowAttributes
		theAttributes;
	UINT32
		valueMask;
	Atom
		protocols[2];																	// number of protocols
	XWMHints
		theHints;																		// hints to tell window manager how to treat these windows
	XSizeHints
		theSizeHints;																	// window size info (used to REALLY convince X that I know where I want my own window)


	if((theDialog=(DIALOG *)MNewPtr(sizeof(DIALOG))))
	{
		theDialog->backgroundColor=AllocColor(background);
		theDialog->dialogComplete=false;												// not done with this yet
		theDialog->firstItem=theDialog->lastItem=theDialog->focusItem=NULL;
		if((theWindow=(EDITORWINDOW *)MNewPtr(sizeof(EDITORWINDOW))))					// create editor window
		{
			if((theWindowElement=(WINDOWLISTELEMENT *)MNewPtr(sizeof(WINDOWLISTELEMENT))))	// create a window list element
			{
				LinkWindowElement(theWindowElement);									// link it to the global window list that we maintain
				theWindowElement->theEditorWindow=theWindow;							// point to our high level concept of window
				theWindow->windowType=EWT_DIALOG;										// set the editor window type to "dialog"
				theWindow->userData=theWindowElement;									// point back to the window element

				theAttributes.background_pixmap=None;
				theAttributes.background_pixel=theDialog->backgroundColor->theXPixel;
				theAttributes.border_pixmap=None;
				theAttributes.border_pixel=black->theXPixel;
				theAttributes.override_redirect=False;
				theAttributes.colormap=xColormap;
				valueMask=CWBackPixmap|CWBackPixel|CWBorderPixmap|CWBorderPixel|CWColormap;
				// Due to X's nasty error handling, we assume this create does not fail!!!

				theWindowElement->realTopWindow=theWindowElement->xWindow=XCreateWindow(xDisplay,RootWindow(xDisplay,xScreenNum),theRect->x,theRect->y,theRect->w,theRect->h,2,CopyFromParent,InputOutput,DefaultVisual(xDisplay,xScreenNum),valueMask,&theAttributes);

				theSizeHints.flags=USSize|USPosition|PMinSize|PMaxSize;					// ### LIE to X, tell it the user requested the size and position so it will honor them
				theSizeHints.x=theRect->x;
				theSizeHints.y=theRect->y;
				theSizeHints.width=theRect->w;
				theSizeHints.height=theRect->h;
				theSizeHints.min_width=theRect->w;
				theSizeHints.min_height=theRect->h;
				theSizeHints.max_width=theRect->w;
				theSizeHints.max_height=theRect->h;
				XSetWMNormalHints(xDisplay,theWindowElement->xWindow,&theSizeHints);

				XStoreName(xDisplay,theWindowElement->xWindow,theTitle);
				XSelectInput(xDisplay,theWindowElement->xWindow,ExposureMask|ButtonPressMask|ButtonReleaseMask|FocusChangeMask|KeyPressMask|StructureNotifyMask);	// want to see these events

				protocols[0]=takeFocusAtom;
				protocols[1]=deleteWindowAtom;
				XSetWMProtocols(xDisplay,theWindowElement->xWindow,&(protocols[0]),2);	// tell X we would like to participate in these protocols

				theHints.flags=InputHint|StateHint|WindowGroupHint;
				theHints.input=True;
				theHints.initial_state=NormalState;
				theHints.window_group=((WINDOWLISTELEMENT *)rootMenuWindow->userData)->xWindow;	// all dialogs belong to the group headed by the main menu window
				XSetWMHints(xDisplay,theWindowElement->xWindow,&theHints);

// NOTE: when setting transient below, It would be nice if the window manager would allow it to be transient for the GROUP, so that
// it would stay on top of all of the editor windows, but so far, I have not been able to convince it to do that :{
// Note that it is being set transient to itself, this provides behavior which is better than anything else I could get the WM
// to do!
//
// NOTE 10/13/2000 the new Gnome does not like this, so it was removed
//				XSetTransientForHint(xDisplay,theWindowElement->xWindow,theWindowElement->xWindow);	// tell WM this is transient (would like it to stay on top, but window manager will not oblige)

				theWindowElement->graphicsContext=XCreateGC(xDisplay,theWindowElement->xWindow,0,NULL);	// create context in which to draw

				theWindowElement->invalidRegion=XCreateRegion();					// create new empty invalid region

				theWindowElement->currentIcon=(Pixmap)NULL;							// no icon associated with this window
				theDialog->parentWindow=theWindow;									// link the dialog to the window just created
				theWindow->windowInfo=theDialog;									// link the window to the dialog just created
				XMapRaised(xDisplay,theWindowElement->xWindow);						// map window to the top of the display
				WaitUntilMapped(theWindow);											// wait until this window is actually mapped before continuing (olwm likes to mess with the temporal ordering of our windows, this attempts to prevent its inane meddling
				return(theDialog);
			}
			MDisposePtr(theWindow);
		}
		FreeColor(theDialog->backgroundColor);
		MDisposePtr(theDialog);
	}
	return(NULL);
}

static void DisposeDialog(DIALOG *theDialog)
// close a dialog window, kill any items it may have linked to it
{
	WINDOWLISTELEMENT
		*theWindowElement;

	while(theDialog->firstItem)
	{
		DisposeDialogItem(theDialog->firstItem);							// remove any items
	}
	FocusAwayFrom(theDialog->parentWindow);									// trick X by moving focus so window manager will not try to when this window dies
	theWindowElement=(WINDOWLISTELEMENT *)theDialog->parentWindow->userData;	// get the x window associated with this document window
	XDestroyRegion(theWindowElement->invalidRegion);						// destroy invalid region
	XFreeGC(xDisplay,theWindowElement->graphicsContext);					// get rid of the graphics context for this window
	XDestroyWindow(xDisplay,theWindowElement->xWindow);						// tell x to make the window go away
	WaitUntilDestroyed(theDialog->parentWindow);
	UnlinkWindowElement(theWindowElement);									// unlink it from our list
	MDisposePtr(theWindowElement);
	MDisposePtr(theDialog->parentWindow);
	FreeColor(theDialog->backgroundColor);
	MDisposePtr(theDialog);
	UpdateEditorWindows();													// update the display
}

static DIALOG *CreateCenteredDialog(UINT32 width,UINT32 height,char *theTitle,EDITORCOLOR background)
// open a new dialog window of width, height centered
// nicely on the screen
// if there is a problem, SetError, and return NULL
{
	EDITORRECT
		dialogRect;
	UINT32
		screenWidth,
		screenHeight;

	GetEditorScreenDimensions(&screenWidth,&screenHeight);
	dialogRect.x=screenWidth/2-width/2;						// half the way across
	dialogRect.y=screenHeight/3-height/2;					// 1/3 the way down
	dialogRect.w=width;
	dialogRect.h=height;
	return(CreateDialog(&dialogRect,theTitle,background));
}

static void DrawDialog(DIALOG *theDialog)
// Draw the dialog, and all items that request it
// drawing focus is handled from here
{
	DIALOGITEM
		*currentItem;
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	GC
		graphicsContext;
	XWindowAttributes
		theAttributes;
	EDITORRECT
		theRect;

	theWindowElement=(WINDOWLISTELEMENT *)theDialog->parentWindow->userData;		// get the x window information associated with this window
	xWindow=theWindowElement->xWindow;												// get x window for this window
	graphicsContext=theWindowElement->graphicsContext;								// get graphics context for this window

	XGetWindowAttributes(xDisplay,xWindow,&theAttributes);
	theRect.x=0;
	theRect.y=0;
	theRect.w=theAttributes.width;
	theRect.h=theAttributes.height;
	XSetRegion(xDisplay,graphicsContext,theWindowElement->invalidRegion);			// set clipping to what's invalid
	OutlineShadowRectangle(xWindow,graphicsContext,&theRect,gray3,gray1,2);			// drop in border
	XSetClipMask(xDisplay,graphicsContext,None);									// get rid of clip mask

	currentItem=theDialog->firstItem;
	while(currentItem)
	{
		if(currentItem->drawProc)
		{
			currentItem->drawProc(currentItem);
		}
		currentItem=currentItem->nextItem;
	}
}

void UpdateDialogWindow(EDITORWINDOW *theWindow)
// make sure theWindow is up to date
{
	WINDOWLISTELEMENT
		*theWindowElement;
	DIALOG
		*theDialog;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;				// get the X window information associated with this document window
	if(!XEmptyRegion(theWindowElement->invalidRegion))						// see if it has an invalid area
	{
		theDialog=(DIALOG *)theWindow->windowInfo;
		DrawDialog(theDialog);
		XDestroyRegion(theWindowElement->invalidRegion);					// clear out the invalid region now that the window has been drawn
		theWindowElement->invalidRegion=XCreateRegion();
	}
}

static void DialogButtonPress(DIALOG *theDialog,XEvent *theEvent)
// handle button press events for a dialog (see what the user is clicking
// on, and attempt to deal with it)
{
	DIALOGITEM
		*currentItem;
	bool
		handled;

	handled=false;
	currentItem=theDialog->firstItem;
	while(currentItem&&!handled)
	{
		if(currentItem->trackProc)
		{
			handled=currentItem->trackProc(currentItem,theEvent);
		}
		currentItem=currentItem->nextItem;
	}
}

static void DialogKeyPress(DIALOG *theDialog,XEvent *theEvent)
// a keyboard event has arrived for theWindow
// decide who it should go to, and handle it
// if a focus change key comes in, and it is not grabbed early, it will
// be used to change the focus, and not sent to the focus item
{
	DIALOGITEM
		*currentItem;
	bool
		handled;
	KeySym
		theKeySym;

	handled=false;
	currentItem=theDialog->firstItem;
	while(currentItem&&!handled)
	{
		if(currentItem->earlyKeyProc)
		{
			handled=currentItem->earlyKeyProc(currentItem,theEvent);
		}
		currentItem=currentItem->nextItem;
	}
	if(!handled)
	{
		ComposeXLookupString((XKeyEvent *)theEvent,NULL,0,&theKeySym);
		if(theKeySym!=XK_Tab)
		{
			if(theDialog->focusItem)
			{
				if(theDialog->focusItem->focusKeyProc)
				{
					theDialog->focusItem->focusKeyProc(theDialog->focusItem,theEvent);
				}
			}
		}
		else
		{
			ForwardFocus(theDialog);
		}
	}
}

void DialogPeriodicProc(EDITORWINDOW *theWindow)
// this is called roughly twice a second to flash cursors, or do whatever in the dialog
// when the dialog has focus
{
	DIALOG
		*theDialog;

	theDialog=(DIALOG *)theWindow->windowInfo;
	if(theDialog->focusItem)
	{
		if(theDialog->focusItem->focusPeriodicProc)
		{
			theDialog->focusItem->focusPeriodicProc(theDialog->focusItem);
		}
	}
}

void DialogTakeFocus(DIALOGITEM *theItem)
// an item in the dialog window has decided that it wants the focus, so change the focus to it
{
	ChangeFocus(theItem->theDialog,theItem);
}

void DialogWindowEvent(EDITORWINDOW *theWindow,XEvent *theEvent)
// handle an event arriving for a dialog window
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	switch(theEvent->type)
	{
		case ButtonPress:
			XRaiseWindow(xDisplay,theWindowElement->xWindow);								// make sure dialog is on top
			XSetInputFocus(xDisplay,theWindowElement->xWindow,RevertToNone,CurrentTime);	// give me the focus
			DialogButtonPress((DIALOG *)theWindow->windowInfo,theEvent);					// go handle the button press
			break;
		case KeyPress:
			DialogKeyPress((DIALOG *)theWindow->windowInfo,theEvent);						// go handle the key press
			break;
		case ClientMessage:
			break;
		default:
			break;
	}
}

static void HandleDialog(DIALOG *theDialog)
// Handle events for theDialog
{
	ForwardFocus(theDialog);								// place focus on first item that wants it
	DialogEventLoop(theDialog->parentWindow,&(theDialog->dialogComplete));
}

static void ExitPress(BUTTON *theButton,void *parameters)
// button was pressed, say that dialog is complete
{
	((DIALOG *)parameters)->dialogComplete=true;			// set completion flag to true
}

bool OkDialog(char *theText)
// bring up an ok dialog, with theText
// wait for the user to click on ok, then return
// if there is a problem, SetError, and return false
{
	DIALOG
		*theDialog;
	STATICTEXTDESCRIPTOR
		textDescriptor;
	SEPARATORDESCRIPTOR
		separatorDescriptor;
	BUTTONDESCRIPTOR
		buttonDescriptor;
	bool
		result;

	result=false;
	if((theDialog=CreateCenteredDialog(500,200," ",DIALOGBACKGROUNDCOLOR)))			// make new empty dialog
	{
		textDescriptor.theRect.x=10;
		textDescriptor.theRect.y=10;
		textDescriptor.theRect.w=480;
		textDescriptor.theRect.h=150;
		textDescriptor.backgroundColor=DIALOGBACKGROUNDCOLOR;
		textDescriptor.foregroundColor=BLACK;
		textDescriptor.theString=theText;
		textDescriptor.fontName=DEFAULTDIALOGSTATICFONT;

		if(NewDialogItem(theDialog,CreateStaticTextItem,&textDescriptor))
		{
			separatorDescriptor.horizontal=true;
			separatorDescriptor.index=155;
			separatorDescriptor.start=10;
			separatorDescriptor.end=490;

			if(NewDialogItem(theDialog,CreateSeparatorItem,&separatorDescriptor))
			{
				buttonDescriptor.theRect.x=225;
				buttonDescriptor.theRect.y=165;
				buttonDescriptor.theRect.w=50;
				buttonDescriptor.theRect.h=25;
				buttonDescriptor.buttonName="Ok";
				buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
				buttonDescriptor.hasKey=true;
				buttonDescriptor.theKeySym=XK_Return;
				buttonDescriptor.pressedProc=ExitPress;
				buttonDescriptor.pressedProcParameters=theDialog;	// pass pointer to the dialog
				buttonDescriptor.pressedState=NULL;

				if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
				{
					HandleDialog(theDialog);					// deal with this dialog
					result=true;
				}
			}
		}
		DisposeDialog(theDialog);
	}
	return(result);
}

bool OkCancelDialog(char *theText,bool *cancel)
// bring up an ok/cancel dialog, with theText
// if there is a problem, SetError, return false
// otherwise, return true in cancel if the user
// cancelled, false if not
{
	DIALOG
		*theDialog;
	STATICTEXTDESCRIPTOR
		textDescriptor;
	SEPARATORDESCRIPTOR
		separatorDescriptor;
	BUTTONDESCRIPTOR
		buttonDescriptor;
	bool
		result;

	result=false;
	*cancel=false;
	if((theDialog=CreateCenteredDialog(500,200," ",DIALOGBACKGROUNDCOLOR)))			// make new empty dialog
	{
		textDescriptor.theRect.x=10;
		textDescriptor.theRect.y=10;
		textDescriptor.theRect.w=480;
		textDescriptor.theRect.h=150;
		textDescriptor.backgroundColor=DIALOGBACKGROUNDCOLOR;
		textDescriptor.foregroundColor=BLACK;
		textDescriptor.theString=theText;
		textDescriptor.fontName=DEFAULTDIALOGSTATICFONT;

		if(NewDialogItem(theDialog,CreateStaticTextItem,&textDescriptor))
		{
			separatorDescriptor.horizontal=true;
			separatorDescriptor.index=155;
			separatorDescriptor.start=10;
			separatorDescriptor.end=490;

			if(NewDialogItem(theDialog,CreateSeparatorItem,&separatorDescriptor))
			{
				buttonDescriptor.theRect.x=55;
				buttonDescriptor.theRect.y=165;
				buttonDescriptor.theRect.w=50;
				buttonDescriptor.theRect.h=25;
				buttonDescriptor.buttonName="Ok";
				buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
				buttonDescriptor.hasKey=true;
				buttonDescriptor.theKeySym=XK_Return;
				buttonDescriptor.pressedProc=ExitPress;
				buttonDescriptor.pressedProcParameters=theDialog;
				buttonDescriptor.pressedState=NULL;

				if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
				{
					buttonDescriptor.theRect.x=375;
					buttonDescriptor.theRect.y=165;
					buttonDescriptor.theRect.w=70;
					buttonDescriptor.theRect.h=25;
					buttonDescriptor.buttonName="Cancel";
					buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
					buttonDescriptor.hasKey=true;
					buttonDescriptor.theKeySym=XK_Escape;
					buttonDescriptor.pressedProc=ExitPress;
					buttonDescriptor.pressedProcParameters=theDialog;
					buttonDescriptor.pressedState=cancel;

					if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
					{
						HandleDialog(theDialog);					// deal with this dialog
						result=true;
					}
				}
			}
		}
		DisposeDialog(theDialog);
	}
	return(result);
}

bool YesNoCancelDialog(char *theText,bool *yes,bool *cancel)
// bring up a yes, no, or cancel dialog, with theText
// if there is a problem, SetError, return false
// otherwise, if cancel is true the user cancelled,
// otherwise, if yes is true, yes was hit,
// otherwise no was hit.
{
	DIALOG
		*theDialog;
	STATICTEXTDESCRIPTOR
		textDescriptor;
	SEPARATORDESCRIPTOR
		separatorDescriptor;
	BUTTONDESCRIPTOR
		buttonDescriptor;
	bool
		result;

	result=false;
	*yes=*cancel=false;
	if((theDialog=CreateCenteredDialog(500,200," ",DIALOGBACKGROUNDCOLOR)))			// make new empty dialog
	{
		textDescriptor.theRect.x=10;
		textDescriptor.theRect.y=10;
		textDescriptor.theRect.w=480;
		textDescriptor.theRect.h=150;
		textDescriptor.backgroundColor=DIALOGBACKGROUNDCOLOR;
		textDescriptor.foregroundColor=BLACK;
		textDescriptor.theString=theText;
		textDescriptor.fontName=DEFAULTDIALOGSTATICFONT;

		if(NewDialogItem(theDialog,CreateStaticTextItem,&textDescriptor))
		{
			separatorDescriptor.horizontal=true;
			separatorDescriptor.index=155;
			separatorDescriptor.start=10;
			separatorDescriptor.end=490;

			if(NewDialogItem(theDialog,CreateSeparatorItem,&separatorDescriptor))
			{
				buttonDescriptor.theRect.x=55;
				buttonDescriptor.theRect.y=165;
				buttonDescriptor.theRect.w=50;
				buttonDescriptor.theRect.h=25;
				buttonDescriptor.buttonName="Yes";
				buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
				buttonDescriptor.hasKey=true;
				buttonDescriptor.theKeySym=XK_y;
				buttonDescriptor.pressedProc=ExitPress;
				buttonDescriptor.pressedProcParameters=theDialog;
				buttonDescriptor.pressedState=yes;

				if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
				{
					buttonDescriptor.theRect.x=145;
					buttonDescriptor.theRect.y=165;
					buttonDescriptor.theRect.w=50;
					buttonDescriptor.theRect.h=25;
					buttonDescriptor.buttonName="No";
					buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
					buttonDescriptor.hasKey=true;
					buttonDescriptor.theKeySym=XK_n;
					buttonDescriptor.pressedProc=ExitPress;
					buttonDescriptor.pressedProcParameters=theDialog;
					buttonDescriptor.pressedState=NULL;

					if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
					{
						buttonDescriptor.theRect.x=375;
						buttonDescriptor.theRect.y=165;
						buttonDescriptor.theRect.w=70;
						buttonDescriptor.theRect.h=25;
						buttonDescriptor.buttonName="Cancel";
						buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
						buttonDescriptor.hasKey=true;
						buttonDescriptor.theKeySym=XK_Escape;
						buttonDescriptor.pressedProc=ExitPress;
						buttonDescriptor.pressedProcParameters=theDialog;
						buttonDescriptor.pressedState=cancel;

						if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
						{
							HandleDialog(theDialog);					// deal with this dialog
							result=true;
						}
					}
				}
			}
		}
		DisposeDialog(theDialog);
	}
	return(result);
}

bool GetSimpleTextDialog(char *theTitle,char *enteredText,UINT32 stringBytes,bool *cancel)
// open a string input dialog with theTitle,
// if there is a problem, SetError, return false
// otherwise, if cancel is true, the user cancelled
// otherwise, enteredText will be set to the text that the user entered in the box
// if enteredText is set to something on entry, it will be placed in the box
{
	DIALOG
		*theDialog;
	EDITORBUFFER
		*theBuffer;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	TEXTBOXDESCRIPTOR
		textBoxDescriptor;
	STATICTEXTDESCRIPTOR
		textDescriptor;
	BUTTONDESCRIPTOR
		buttonDescriptor;
	bool
		result;

	result=false;
	*cancel=false;
	if((theBuffer=EditorNewBuffer("simple text")))		// create a buffer in which to collect the text
	{
		if(InsertUniverseText(theBuffer->textUniverse,theBuffer->textUniverse->totalBytes,(UINT8 *)enteredText,strlen(enteredText)))
		{
			EditorSelectAll(theBuffer);					// select it all
			if((theDialog=CreateCenteredDialog(600,300," ",DIALOGBACKGROUNDCOLOR)))			// make new empty dialog
			{
				textDescriptor.theRect.x=10;
				textDescriptor.theRect.y=10;
				textDescriptor.theRect.w=580;
				textDescriptor.theRect.h=20;
				textDescriptor.backgroundColor=DIALOGBACKGROUNDCOLOR;
				textDescriptor.foregroundColor=BLACK;
				textDescriptor.theString=theTitle;
				textDescriptor.fontName=DEFAULTDIALOGSTATICFONT;

				if(NewDialogItem(theDialog,CreateStaticTextItem,&textDescriptor))
				{
					textBoxDescriptor.theBuffer=theBuffer;
					textBoxDescriptor.theRect.x=10;
					textBoxDescriptor.theRect.y=30;
					textBoxDescriptor.theRect.w=580;
					textBoxDescriptor.theRect.h=220;
					textBoxDescriptor.topLine=0;
					textBoxDescriptor.leftPixel=0;
					textBoxDescriptor.tabSize=4;
					textBoxDescriptor.focusBackgroundColor=GRAY3;
					textBoxDescriptor.focusForegroundColor=BLACK;
					textBoxDescriptor.nofocusBackgroundColor=GRAY3;
					textBoxDescriptor.nofocusForegroundColor=GRAY1;
					textBoxDescriptor.fontName=DEFAULTVIEWFONT;

					if(NewDialogItem(theDialog,CreateTextBoxItem,&textBoxDescriptor))
					{
						buttonDescriptor.theRect.x=55;
						buttonDescriptor.theRect.y=260;
						buttonDescriptor.theRect.w=50;
						buttonDescriptor.theRect.h=25;
						buttonDescriptor.buttonName="Ok";
						buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
						buttonDescriptor.hasKey=true;
						buttonDescriptor.theKeySym=XK_Return;
						buttonDescriptor.pressedProc=ExitPress;
						buttonDescriptor.pressedProcParameters=theDialog;
						buttonDescriptor.pressedState=NULL;

						if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
						{
							buttonDescriptor.theRect.x=475;
							buttonDescriptor.theRect.y=260;
							buttonDescriptor.theRect.w=70;
							buttonDescriptor.theRect.h=25;
							buttonDescriptor.buttonName="Cancel";
							buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
							buttonDescriptor.hasKey=true;
							buttonDescriptor.theKeySym=XK_Escape;
							buttonDescriptor.pressedProc=ExitPress;
							buttonDescriptor.pressedProcParameters=theDialog;
							buttonDescriptor.pressedState=cancel;

							if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
							{
								HandleDialog(theDialog);					// deal with this dialog
								result=true;
							}
						}
					}
				}
				DisposeDialog(theDialog);
			}
		}
		if(!(*cancel))
		{
			if(ExtractUniverseText(theBuffer->textUniverse,theBuffer->textUniverse->firstChunkHeader,0,(UINT8 *)enteredText,stringBytes,&theChunk,&theOffset))
			{
				if(theBuffer->textUniverse->totalBytes<stringBytes)
				{
					enteredText[theBuffer->textUniverse->totalBytes]='\0';
				}
				else
				{
					if(stringBytes)
					{
						enteredText[stringBytes-1]='\0';
					}
				}
			}
			else
			{
				result=false;
			}
		}
		EditorCloseBuffer(theBuffer);
	}
	return(result);
}

bool SearchReplaceDialog(EDITORBUFFER *searchBuffer,EDITORBUFFER *replaceBuffer,bool *backwards,bool *wrapAround,bool *selectionExpr,bool *ignoreCase,bool *limitScope,bool *replaceProc,UINT16 *searchType,bool *cancel)
// open the search/replace dialog
// if there is a problem, SetError, return false
// otherwise, if cancel is true, the user cancelled
// otherwise,
// return the enumerated mode that the user chose, as well as the states of
// backwards, wrapAround, selectionExpr, ignoreCase, limitScope, replaceProc
{
	DIALOG
		*theDialog;
	TEXTBOXDESCRIPTOR
		textBoxDescriptor;
	STATICTEXTDESCRIPTOR
		textDescriptor;
	BUTTONDESCRIPTOR
		buttonDescriptor;
	CHECKBOXDESCRIPTOR
		checkDescriptor;
	bool
		result;
	bool
		hadFind,
		hadFindAll,
		hadReplace,
		hadReplaceAll;

	result=false;
	*cancel=false;
	hadFind=false;
	hadFindAll=false;
	hadReplace=false;
	hadReplaceAll=false;

	EditorSelectAll(searchBuffer);						// select it all
	EditorSelectAll(replaceBuffer);						// select it all

	if((theDialog=CreateCenteredDialog(600,430," ",DIALOGBACKGROUNDCOLOR)))	// make new empty dialog
	{
		textDescriptor.theRect.x=10;
		textDescriptor.theRect.y=10;
		textDescriptor.theRect.w=580;
		textDescriptor.theRect.h=20;
		textDescriptor.backgroundColor=DIALOGBACKGROUNDCOLOR;
		textDescriptor.foregroundColor=BLACK;
		textDescriptor.theString="Find:";
		textDescriptor.fontName=DEFAULTDIALOGSTATICFONT;

		if(NewDialogItem(theDialog,CreateStaticTextItem,&textDescriptor))
		{
			textBoxDescriptor.theBuffer=searchBuffer;
			textBoxDescriptor.theRect.x=10;
			textBoxDescriptor.theRect.y=30;
			textBoxDescriptor.theRect.w=580;
			textBoxDescriptor.theRect.h=110;
			textBoxDescriptor.topLine=0;
			textBoxDescriptor.leftPixel=0;
			textBoxDescriptor.tabSize=4;
			textBoxDescriptor.focusBackgroundColor=GRAY3;
			textBoxDescriptor.focusForegroundColor=BLACK;
			textBoxDescriptor.nofocusBackgroundColor=GRAY3;
			textBoxDescriptor.nofocusForegroundColor=GRAY1;
			textBoxDescriptor.fontName=DEFAULTVIEWFONT;

			if(NewDialogItem(theDialog,CreateTextBoxItem,&textBoxDescriptor))
			{
				textDescriptor.theRect.x=10;
				textDescriptor.theRect.y=150;
				textDescriptor.theRect.w=580;
				textDescriptor.theRect.h=20;
				textDescriptor.backgroundColor=DIALOGBACKGROUNDCOLOR;
				textDescriptor.foregroundColor=BLACK;
				textDescriptor.theString="Replace with:";
				textDescriptor.fontName=DEFAULTDIALOGSTATICFONT;

				if(NewDialogItem(theDialog,CreateStaticTextItem,&textDescriptor))
				{
					textBoxDescriptor.theBuffer=replaceBuffer;
					textBoxDescriptor.theRect.x=10;
					textBoxDescriptor.theRect.y=170;
					textBoxDescriptor.theRect.w=580;
					textBoxDescriptor.theRect.h=110;
					textBoxDescriptor.topLine=0;
					textBoxDescriptor.leftPixel=0;
					textBoxDescriptor.tabSize=4;
					textBoxDescriptor.focusBackgroundColor=GRAY3;
					textBoxDescriptor.focusForegroundColor=BLACK;
					textBoxDescriptor.nofocusBackgroundColor=GRAY3;
					textBoxDescriptor.nofocusForegroundColor=GRAY1;
					textBoxDescriptor.fontName=DEFAULTVIEWFONT;

					if(NewDialogItem(theDialog,CreateTextBoxItem,&textBoxDescriptor))
					{
						// check boxes
						checkDescriptor.theRect.x=10;
						checkDescriptor.theRect.y=290;
						checkDescriptor.theRect.w=260;
						checkDescriptor.theRect.h=20;
						checkDescriptor.checkBoxName="Search Backwards";
						checkDescriptor.fontName=DEFAULTDIALOGSTATICFONT;
						checkDescriptor.hasKey=false;
						checkDescriptor.pressedProc=NULL;
						checkDescriptor.pressedProcParameters=NULL;
						checkDescriptor.pressedState=backwards;

						if(NewDialogItem(theDialog,CreateCheckBoxItem,&checkDescriptor))
						{
							checkDescriptor.theRect.x=10;
							checkDescriptor.theRect.y=315;
							checkDescriptor.theRect.w=260;
							checkDescriptor.theRect.h=20;
							checkDescriptor.checkBoxName="Wrap Around";
							checkDescriptor.fontName=DEFAULTDIALOGSTATICFONT;
							checkDescriptor.hasKey=false;
							checkDescriptor.pressedProc=NULL;
							checkDescriptor.pressedProcParameters=NULL;
							checkDescriptor.pressedState=wrapAround;

							if(NewDialogItem(theDialog,CreateCheckBoxItem,&checkDescriptor))
							{
								checkDescriptor.theRect.x=330;
								checkDescriptor.theRect.y=290;
								checkDescriptor.theRect.w=260;
								checkDescriptor.theRect.h=20;
								checkDescriptor.checkBoxName="Selection Expression";
								checkDescriptor.fontName=DEFAULTDIALOGSTATICFONT;
								checkDescriptor.hasKey=false;
								checkDescriptor.pressedProc=NULL;
								checkDescriptor.pressedProcParameters=NULL;
								checkDescriptor.pressedState=selectionExpr;

								if(NewDialogItem(theDialog,CreateCheckBoxItem,&checkDescriptor))
								{
									checkDescriptor.theRect.x=330;
									checkDescriptor.theRect.y=315;
									checkDescriptor.theRect.w=260;
									checkDescriptor.theRect.h=20;
									checkDescriptor.checkBoxName="Ignore Case";
									checkDescriptor.fontName=DEFAULTDIALOGSTATICFONT;
									checkDescriptor.hasKey=false;
									checkDescriptor.pressedProc=NULL;
									checkDescriptor.pressedProcParameters=NULL;
									checkDescriptor.pressedState=ignoreCase;

									if(NewDialogItem(theDialog,CreateCheckBoxItem,&checkDescriptor))
									{
										checkDescriptor.theRect.x=10;
										checkDescriptor.theRect.y=340;
										checkDescriptor.theRect.w=580;
										checkDescriptor.theRect.h=20;
										checkDescriptor.checkBoxName="Limit scope of 'Find All'/'Replace All' to current selection";
										checkDescriptor.fontName=DEFAULTDIALOGSTATICFONT;
										checkDescriptor.hasKey=false;
										checkDescriptor.pressedProc=NULL;
										checkDescriptor.pressedProcParameters=NULL;
										checkDescriptor.pressedState=limitScope;

										if(NewDialogItem(theDialog,CreateCheckBoxItem,&checkDescriptor))
										{
											checkDescriptor.theRect.x=10;
											checkDescriptor.theRect.y=365;
											checkDescriptor.theRect.w=580;
											checkDescriptor.theRect.h=20;
											checkDescriptor.checkBoxName="Treat replace text as Tcl procedure -- substitute results";
											checkDescriptor.fontName=DEFAULTDIALOGSTATICFONT;
											checkDescriptor.hasKey=false;
											checkDescriptor.pressedProc=NULL;
											checkDescriptor.pressedProcParameters=NULL;
											checkDescriptor.pressedState=replaceProc;

											if(NewDialogItem(theDialog,CreateCheckBoxItem,&checkDescriptor))
											{

												buttonDescriptor.theRect.x=10;
												buttonDescriptor.theRect.y=395;
												buttonDescriptor.theRect.w=100;
												buttonDescriptor.theRect.h=25;
												buttonDescriptor.buttonName="Find";
												buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
												buttonDescriptor.hasKey=true;
												buttonDescriptor.theKeySym=XK_Return;
												buttonDescriptor.pressedProc=ExitPress;
												buttonDescriptor.pressedProcParameters=theDialog;
												buttonDescriptor.pressedState=&hadFind;

												if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
												{
													buttonDescriptor.theRect.x=120;
													buttonDescriptor.theRect.y=395;
													buttonDescriptor.theRect.w=100;
													buttonDescriptor.theRect.h=25;
													buttonDescriptor.buttonName="Find All";
													buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
													buttonDescriptor.hasKey=false;
													buttonDescriptor.pressedProc=ExitPress;
													buttonDescriptor.pressedProcParameters=theDialog;
													buttonDescriptor.pressedState=&hadFindAll;

													if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
													{
														buttonDescriptor.theRect.x=230;
														buttonDescriptor.theRect.y=395;
														buttonDescriptor.theRect.w=100;
														buttonDescriptor.theRect.h=25;
														buttonDescriptor.buttonName="Replace";
														buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
														buttonDescriptor.hasKey=false;
														buttonDescriptor.pressedProc=ExitPress;
														buttonDescriptor.pressedProcParameters=theDialog;
														buttonDescriptor.pressedState=&hadReplace;

														if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
														{
															buttonDescriptor.theRect.x=340;
															buttonDescriptor.theRect.y=395;
															buttonDescriptor.theRect.w=100;
															buttonDescriptor.theRect.h=25;
															buttonDescriptor.buttonName="Replace All";
															buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
															buttonDescriptor.hasKey=false;
															buttonDescriptor.pressedProc=ExitPress;
															buttonDescriptor.pressedProcParameters=theDialog;
															buttonDescriptor.pressedState=&hadReplaceAll;

															if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
															{
																buttonDescriptor.theRect.x=490;
																buttonDescriptor.theRect.y=395;
																buttonDescriptor.theRect.w=100;
																buttonDescriptor.theRect.h=25;
																buttonDescriptor.buttonName="Cancel";
																buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
																buttonDescriptor.hasKey=true;
																buttonDescriptor.theKeySym=XK_Escape;
																buttonDescriptor.pressedProc=ExitPress;
																buttonDescriptor.pressedProcParameters=theDialog;
																buttonDescriptor.pressedState=cancel;

																if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
																{
																	HandleDialog(theDialog);					// deal with this dialog
																	result=true;
																}
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		DisposeDialog(theDialog);
	}
	if(hadFind)
	{
		(*searchType)=ST_FIND;
	}
	if(hadFindAll)
	{
		(*searchType)=ST_FINDALL;
	}
	if(hadReplace)
	{
		(*searchType)=ST_REPLACE;
	}
	if(hadReplaceAll)
	{
		(*searchType)=ST_REPLACEALL;
	}
	return(result);
}

static void ListExitPress(LISTBOX *theListBox,void *parameters)
// when the simple list is double clicked in, it is just like
// pressing the Ok button, so we do that!
{
	DIALOG
		*theDialog;

	theDialog=(DIALOG *)parameters;
	PressButtonItem((DIALOGITEM *)theDialog->dialogLocal);
}

bool SimpleListBoxDialog(char *theTitle,UINT32 numElements,char **listElements,bool *selectedElements,bool *cancel)
// open a list selection dialog box with two buttons (ok, cancel)
// The list dialog will be used to modify the selectionListArray, to tell
// which items were selected, and which were not
// if there is a problem, SetError, return false
// otherwise, if cancel is true, the user cancelled
// otherwise, selectedElements is modified to reflect which items
// have been selected
{
	DIALOG
		*theDialog;
	LISTBOXDESCRIPTOR
		listBoxDescriptor;
	STATICTEXTDESCRIPTOR
		textDescriptor;
	BUTTONDESCRIPTOR
		buttonDescriptor;
	bool
		done,
		result;
	UINT32
		index;

	result=false;
	*cancel=false;

	if((theDialog=CreateCenteredDialog(600,300," ",DIALOGBACKGROUNDCOLOR)))			// make new empty dialog
	{

		textDescriptor.theRect.x=10;
		textDescriptor.theRect.y=10;
		textDescriptor.theRect.w=580;
		textDescriptor.theRect.h=20;
		textDescriptor.backgroundColor=DIALOGBACKGROUNDCOLOR;
		textDescriptor.foregroundColor=BLACK;
		textDescriptor.theString=theTitle;
		textDescriptor.fontName=DEFAULTDIALOGSTATICFONT;

		if(NewDialogItem(theDialog,CreateStaticTextItem,&textDescriptor))
		{
			listBoxDescriptor.theRect.x=10;
			listBoxDescriptor.theRect.y=30;
			listBoxDescriptor.theRect.w=580;
			listBoxDescriptor.theRect.h=220;
			listBoxDescriptor.topLine=0;						// if no selection found, take top as 0
			done=false;
			for(index=0;!done&&index<numElements;index++)		// make top line the first selection if possible
			{
				if(selectedElements[index])
				{
					listBoxDescriptor.topLine=index;
					done=true;
				}
			}
			listBoxDescriptor.focusBackgroundColor=GRAY3;
			listBoxDescriptor.focusForegroundColor=BLACK;
			listBoxDescriptor.nofocusBackgroundColor=GRAY3;
			listBoxDescriptor.nofocusForegroundColor=BLACK;
			listBoxDescriptor.numElements=numElements;
			listBoxDescriptor.listElements=listElements;
			listBoxDescriptor.selectedElements=selectedElements;
			listBoxDescriptor.fontName=DEFAULTVIEWFONT;
			listBoxDescriptor.pressedProc=ListExitPress;
			listBoxDescriptor.pressedProcParameters=theDialog;

			if(NewDialogItem(theDialog,CreateListBoxItem,&listBoxDescriptor))
			{
				buttonDescriptor.theRect.x=55;
				buttonDescriptor.theRect.y=260;
				buttonDescriptor.theRect.w=50;
				buttonDescriptor.theRect.h=25;
				buttonDescriptor.buttonName="Ok";
				buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
				buttonDescriptor.hasKey=true;
				buttonDescriptor.theKeySym=XK_Return;
				buttonDescriptor.pressedProc=ExitPress;
				buttonDescriptor.pressedProcParameters=theDialog;
				buttonDescriptor.pressedState=NULL;

				if((theDialog->dialogLocal=NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor)))	// remember this button
				{
					buttonDescriptor.theRect.x=475;
					buttonDescriptor.theRect.y=260;
					buttonDescriptor.theRect.w=70;
					buttonDescriptor.theRect.h=25;
					buttonDescriptor.buttonName="Cancel";
					buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
					buttonDescriptor.hasKey=true;
					buttonDescriptor.theKeySym=XK_Escape;
					buttonDescriptor.pressedProc=ExitPress;
					buttonDescriptor.pressedProcParameters=theDialog;
					buttonDescriptor.pressedState=cancel;

					if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
					{
						HandleDialog(theDialog);					// deal with this dialog
						result=true;
					}
				}
			}
		}
		DisposeDialog(theDialog);
	}
	return(result);
}

// stuff dealing with open/save dialogs

// this structure is hung off of the open/save dialogs, it allows the dialog item
// procedures access to some things they need

typedef struct
{
	EDITORBUFFER
		*theBuffer;
	char
		pathString[MAXPATHLEN+1];
	char
		**theList;
	bool
		*selectionList;
	UINT32
		numElements;
	char
		**outputList;
	UINT32
		outputElements;
	bool
		justOne,
		justPaths;
	DIALOGITEM
		*pathText,
		*listBox,
		*textBox,
		*OkButton;
} FILEDIALOGPARAMETERS;

static char **ProcessDirectoryList(char *pathString,char **theList,UINT32 inElements,UINT32 *outElements,bool justPaths)
// given a path string, and a sorted raw list of files at the end of the path
// create a new list which contains the processed versions of each entry
// (place PATHSEP after directories, eliminate files if justPaths is true)
// if there is a problem, SetError, return NULL
// NOTE: this does not dispose of the incoming list
// the output list must be disposed of by calling FreeStringList
{
	char
		**outList;
	char
		tempPath[MAXPATHLEN+1];
	int
		theLength;
	UINT32
		inElement;
	struct stat
		theStat;

	*outElements=0;
	if((outList=NewStringList()))
	{
		for(inElement=0;inElement<inElements;inElement++)
		{
			ConcatPathAndFile(pathString,theList[inElement],tempPath);
			if(stat(tempPath,&theStat)!=-1)				// if this fails, then do not copy the name
			{
				if(S_ISDIR(theStat.st_mode))
				{
					theLength=strlen(theList[inElement]);
					if(theLength<MAXPATHLEN)
					{
						strcpy(tempPath,theList[inElement]);
						tempPath[theLength++]=PATHSEP;	// drop PATHSEP onto the string
						tempPath[theLength]='\0';
						AddStringToList(tempPath,&outList,outElements);	// ignore errors here
					}
				}
				else
				{
					if(!justPaths)						// see if file names should be added
					{
						AddStringToList(theList[inElement],&outList,outElements);	// ignore errors here
					}
				}
			}
		}
	}
	return(outList);
}

static void DisposeSelectionFiles(char **theList,bool *selectionList)
// dispose of the two lists returned by GetPathAndSelectionFiles
{
	MDisposePtr(selectionList);
	FreeStringList(theList);
}

static char **GetSelectionFiles(char *pathString,UINT32 *numElements,bool **selectionList,bool justPaths)
// given pathString, create a list of the files/directories at the end of the path
// DisposeSelectionFiles must be called on theList, and selectionList to dispose of them
// if there is a problem, SetError, return NULL
{
	char
		**rawList,
		**theList;
	UINT32
		rawElements;

	if((rawList=GlobAll(pathString,&rawElements)))
	{
		theList=ProcessDirectoryList(pathString,rawList,rawElements,numElements,justPaths);
		FreeStringList(rawList);					// get rid of the raw list
		if(theList)
		{
			if((*selectionList=(bool *)MNewPtrClr((*numElements)*sizeof(bool))))
			{
				return(theList);
			}
			FreeStringList(theList);
		}
	}

	*selectionList=NULL;
	*numElements=0;

	return(NULL);
}

static void FileListPress(LISTBOX *theListBox,void *parameters)
// when a selection has been made from the file list, this
// will decide what to do
{
	DIALOG
		*theDialog;
	FILEDIALOGPARAMETERS
		*theParameters;

	theDialog=(DIALOG *)parameters;
	theParameters=(FILEDIALOGPARAMETERS *)theDialog->dialogLocal;
	PressButtonItem(theParameters->OkButton);		// behave as if Ok was pressed
}

static bool AddPathAndFileToList(char *thePath,char *theFile,char ***theList,UINT32 *numElements)
// like AddStringToList, but takes a path, and a filename which are
// concatenated into a complete path, and added to the list
// if there is a problem, SetError, and return false
{
	char
		fullPath[MAXPATHLEN+1];

	ConcatPathAndFile(thePath,theFile,fullPath);
	return(AddStringToList(fullPath,theList,numElements));
}

static bool FileOkInText(DIALOG *theDialog,FILEDIALOGPARAMETERS *theParameters)
// Ok was pressed while the text was active
{
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	char
		globPath[MAXPATHLEN+1];
	char
		**globList;
	UINT32
		currentElement,
		globElements;
	bool
		fail;

	fail=false;

	if((theParameters->outputList=NewStringList()))
	{
		theParameters->outputElements=0;
		if(ExtractUniverseText(theParameters->theBuffer->textUniverse,theParameters->theBuffer->textUniverse->firstChunkHeader,0,(UINT8 *)globPath,MAXPATHLEN,&theChunk,&theOffset))
		{
			if(theParameters->theBuffer->textUniverse->totalBytes<MAXPATHLEN)
			{
				globPath[theParameters->theBuffer->textUniverse->totalBytes]='\0';
			}
			else
			{
				globPath[MAXPATHLEN]='\0';
			}
			if((globList=Glob(&(theParameters->pathString[0]),globPath,false,true,&globElements)))
			{
				for(currentElement=0;!fail&&currentElement<globElements;currentElement++)
				{
					fail=!AddPathAndFileToList(&(theParameters->pathString[0]),globList[currentElement],&theParameters->outputList,&theParameters->outputElements);
				}
				FreeStringList(globList);
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
		if(fail)
		{
			FreeStringList(theParameters->outputList);
			theParameters->outputElements=0;
		}
	}
	return(!fail);
}

static bool FileOkInList(DIALOG *theDialog,FILEDIALOGPARAMETERS *theParameters)
// Ok was pressed while the list was active
// Create an output list which contains all the selected items
// if there is a problem, SetError, return false
// NOTE: to make the interface seem more intuitive, if there
// are no selected items in the list, this will revert to making
// a list from the text box
{
	UINT32
		currentElement;
	bool
		noneSelected,
		fail;

	fail=false;
	noneSelected=true;
	if((theParameters->outputList=NewStringList()))
	{
		theParameters->outputElements=0;
		for(currentElement=0;!fail&&currentElement<theParameters->numElements;currentElement++)
		{
			if(theParameters->selectionList[currentElement])	// is this one selected?
			{
				noneSelected=false;
				fail=!AddPathAndFileToList(&(theParameters->pathString[0]),theParameters->theList[currentElement],&theParameters->outputList,&theParameters->outputElements);
			}
		}
		if(fail||noneSelected)
		{
			FreeStringList(theParameters->outputList);
			theParameters->outputElements=0;
		}
	}
	if(!fail)
	{
		if(noneSelected)									// if not failure, but nothing selected, use dialog text
		{
			return(FileOkInText(theDialog,theParameters));
		}
		else
		{
			return(true);
		}
	}
	return(false);
}

static void FileOkPress(BUTTON *theButton,void *parameters)
// The user pressed "Ok" in the dialog (or it was pressed for him)
// take appropriate action
{
	DIR
		*theDirectory;
	DIALOG
		*theDialog;
	FILEDIALOGPARAMETERS
		*theParameters;
	char
		*errorFamily,
		*errorFamilyMember,
		*errorDescription;
	bool
		fail;

	theDialog=(DIALOG *)parameters;
	theParameters=(FILEDIALOGPARAMETERS *)theDialog->dialogLocal;
	fail=false;
	if(theDialog->focusItem==theParameters->listBox)	// if list has focus, then build a list out of what's selected
	{
		fail=!FileOkInList(theDialog,theParameters);
	}
	else
	{
		fail=!FileOkInText(theDialog,theParameters);
	}

	if(!fail)
	{
		if(theParameters->outputElements==1)		// if only one element, then see if directory
		{
			if((theDirectory=opendir(theParameters->outputList[0])))	// see if it is a directory, and we are allowed to open it
			{
				closedir(theDirectory);				// did not want it open, just wanted to see if we could
				if(!(theDialog->focusItem==theParameters->listBox))
				{
					ClearBuffer(theParameters->theBuffer);	// get rid of any text in the view
				}
				if(theParameters->theList)
				{
					DisposeSelectionFiles(theParameters->theList,theParameters->selectionList);
				}
				realpath2(theParameters->outputList[0],&(theParameters->pathString[0]));	// this is the new path
				theParameters->theList=GetSelectionFiles(&(theParameters->pathString[0]),&theParameters->numElements,&theParameters->selectionList,theParameters->justPaths);
				ResetStaticTextItemText(theParameters->pathText,&(theParameters->pathString[0]));
				ResetListBoxItemLists(theParameters->listBox,theParameters->numElements,theParameters->theList,theParameters->selectionList);
				ResetMultipleClicks();				// reset multiple click timers
			}
			else
			{
				if(errno==ENOTDIR||errno==ENOENT)
				{
					if(!theParameters->justPaths)
					{
						theDialog->dialogComplete=true;		// if not a directory, or known file, return it
					}
					else
					{
						ReportMessage("%s\nNot a directory\n",theParameters->outputList[0]);	// let user know what's wrong
					}
				}
				else
				{
					SetStdCLibError();						// report whatever error we got
					GetError(&errorFamily,&errorFamilyMember,&errorDescription);
					ReportMessage("%s\n",errorDescription);	// let user know what's wrong
				}
			}
		}
		else
		{
			if(theParameters->justOne||theParameters->justPaths)
			{
				ReportMessage("Path is ambiguous\n");	// got more elements than asked for, so complain, and keep going
			}
			else
			{
				theDialog->dialogComplete=true;			// if anything other that 1 element, pass it back as the result
			}
		}
		if(!theDialog->dialogComplete)
		{
			FreeStringList(theParameters->outputList);	// not leaving, so kill the list
		}
	}
	else
	{
		GetError(&errorFamily,&errorFamilyMember,&errorDescription);
		ReportMessage("%s\n",errorDescription);			// let user know what's wrong
	}
}

static bool FileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,char ***theList,bool justOne,bool justPaths,bool *cancel)
// bring up a file open/save dialog
// if there is a problem, SetError, return false
// otherwise, cancel is true if the user cancelled the dialog
//
// otherwise, if justPaths is true, return the final path chosen in the
// dialog in fullPath, truncating to stringBytes in length
//
// otherwise, listElements contains the pointer to the start of an array of
// pointers to char which are the selections.
// it must be disposed of at some later time with a call to FreeFileDialogPaths
// NOTE: the array is terminated with a pointer to NULL
{
	DIALOG
		*theDialog;
	TEXTBOXDESCRIPTOR
		textBoxDescriptor;
	LISTBOXDESCRIPTOR
		listBoxDescriptor;
	STATICTEXTDESCRIPTOR
		textDescriptor;
	BUTTONDESCRIPTOR
		buttonDescriptor;
	FILEDIALOGPARAMETERS
		theParameters;
	bool
		result;
	char
		filePart[MAXPATHLEN+1];

	result=false;
	*cancel=false;

	theParameters.justOne=justOne;
	theParameters.justPaths=justPaths;

	if((theParameters.theBuffer=EditorNewBuffer("file dialog")))		// create a buffer in which to collect the file name
	{
		SplitPathAndFile(fullPath,theParameters.pathString,filePart);	// split the incoming name into file and path components
		if(InsertUniverseText(theParameters.theBuffer->textUniverse,theParameters.theBuffer->textUniverse->totalBytes,(UINT8 *)filePart,strlen(filePart)))
		{
			EditorSelectAll(theParameters.theBuffer);		// select it all

			if((theDialog=CreateCenteredDialog(420,380," ",DIALOGBACKGROUNDCOLOR)))		// make new empty dialog
			{
				theDialog->dialogLocal=&theParameters;				// hang parameter list off of the dialog

				theParameters.theList=GetSelectionFiles(&(theParameters.pathString[0]),&theParameters.numElements,&theParameters.selectionList,justPaths);	// ignore failure, since list box will handle a NULL list

				textDescriptor.theRect.x=10;
				textDescriptor.theRect.y=10;
				textDescriptor.theRect.w=400;
				textDescriptor.theRect.h=20;
				textDescriptor.backgroundColor=DIALOGBACKGROUNDCOLOR;
				textDescriptor.foregroundColor=BLACK;
				textDescriptor.theString=theTitle;
				textDescriptor.fontName=DEFAULTDIALOGSTATICFONT;

				if(NewDialogItem(theDialog,CreateStaticTextItem,&textDescriptor))
				{
					textDescriptor.theRect.x=10;
					textDescriptor.theRect.y=30;
					textDescriptor.theRect.w=400;
					textDescriptor.theRect.h=40;
					textDescriptor.backgroundColor=DIALOGBACKGROUNDCOLOR;
					textDescriptor.foregroundColor=BLACK;
					textDescriptor.theString=&(theParameters.pathString[0]);
					textDescriptor.fontName=DEFAULTVIEWFONT;

					if((theParameters.pathText=NewDialogItem(theDialog,CreateStaticTextItem,&textDescriptor)))
					{

						textBoxDescriptor.theBuffer=theParameters.theBuffer;
						textBoxDescriptor.theRect.x=10;
						textBoxDescriptor.theRect.y=66;
						textBoxDescriptor.theRect.w=400;
						textBoxDescriptor.theRect.h=30;
						textBoxDescriptor.topLine=0;
						textBoxDescriptor.leftPixel=0;
						textBoxDescriptor.tabSize=4;
						textBoxDescriptor.focusBackgroundColor=GRAY3;
						textBoxDescriptor.focusForegroundColor=BLACK;
						textBoxDescriptor.nofocusBackgroundColor=GRAY3;
						textBoxDescriptor.nofocusForegroundColor=GRAY1;
						textBoxDescriptor.fontName=DEFAULTVIEWFONT;

						if((theParameters.textBox=NewDialogItem(theDialog,CreateTextBoxItem,&textBoxDescriptor)))
						{

							listBoxDescriptor.theRect.x=10;
							listBoxDescriptor.theRect.y=110;
							listBoxDescriptor.theRect.w=400;
							listBoxDescriptor.theRect.h=220;
							listBoxDescriptor.topLine=0;
							listBoxDescriptor.focusBackgroundColor=GRAY3;
							listBoxDescriptor.focusForegroundColor=BLACK;
							listBoxDescriptor.nofocusBackgroundColor=GRAY3;
							listBoxDescriptor.nofocusForegroundColor=GRAY1;
							listBoxDescriptor.numElements=theParameters.numElements;
							listBoxDescriptor.listElements=theParameters.theList;
							listBoxDescriptor.selectedElements=theParameters.selectionList;
							listBoxDescriptor.fontName=DEFAULTVIEWFONT;
							listBoxDescriptor.pressedProc=FileListPress;
							listBoxDescriptor.pressedProcParameters=theDialog;

							if((theParameters.listBox=NewDialogItem(theDialog,CreateListBoxItem,&listBoxDescriptor)))
							{

								buttonDescriptor.theRect.x=55;
								buttonDescriptor.theRect.y=340;
								buttonDescriptor.theRect.w=50;
								buttonDescriptor.theRect.h=25;
								buttonDescriptor.buttonName="Ok";
								buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
								buttonDescriptor.hasKey=true;
								buttonDescriptor.theKeySym=XK_Return;
								buttonDescriptor.pressedProc=FileOkPress;
								buttonDescriptor.pressedProcParameters=theDialog;
								buttonDescriptor.pressedState=NULL;

								if((theParameters.OkButton=NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor)))	// remember this button
								{
									buttonDescriptor.theRect.x=305;
									buttonDescriptor.theRect.y=340;
									buttonDescriptor.theRect.w=70;
									buttonDescriptor.theRect.h=25;
									buttonDescriptor.buttonName="Cancel";
									buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
									buttonDescriptor.hasKey=true;
									buttonDescriptor.theKeySym=XK_Escape;
									buttonDescriptor.pressedProc=ExitPress;
									buttonDescriptor.pressedProcParameters=theDialog;
									buttonDescriptor.pressedState=cancel;

									if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
									{
										if(justPaths)			// add another button for paths (use enter to select)
										{
											buttonDescriptor.theRect.x=160;
											buttonDescriptor.theRect.y=340;
											buttonDescriptor.theRect.w=100;
											buttonDescriptor.theRect.h=25;
											buttonDescriptor.buttonName="Accept Path";
											buttonDescriptor.fontName=DEFAULTDIALOGBUTTONFONT;
											buttonDescriptor.hasKey=true;
											buttonDescriptor.theKeySym=XK_KP_Enter;
											buttonDescriptor.pressedProc=ExitPress;
											buttonDescriptor.pressedProcParameters=theDialog;
											buttonDescriptor.pressedState=NULL;

											if(NewDialogItem(theDialog,CreateButtonItem,&buttonDescriptor))
											{
												HandleDialog(theDialog);	// deal with this dialog
												result=true;
											}
										}
										else
										{
											HandleDialog(theDialog);	// deal with this dialog
											result=true;
										}
									}
								}
							}
						}
					}
				}
				if(theParameters.theList)							// delete any lists that are hanging around
				{
					DisposeSelectionFiles(theParameters.theList,theParameters.selectionList);
				}
				DisposeDialog(theDialog);
			}
		}
		EditorCloseBuffer(theParameters.theBuffer);
	}
	if(justPaths)
	{
		if(stringBytes)
		{
			strncpy(fullPath,theParameters.pathString,stringBytes);	// return the path
			fullPath[stringBytes-1]='\0';
		}
	}
	if(result&&!(*cancel)&&(!justPaths))
	{
		*theList=theParameters.outputList;
	}
	return(result);
}

static void FreeFileDialogPaths(char **thePaths)
// free an array of paths that was returned from FileDialog
{
	FreeStringList(thePaths);
}

bool OpenFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,char ***listElements,bool *cancel)
// bring up a file open dialog
// if there is a problem, SetError, return false
// otherwise, cancel is true if the user cancelled the dialog
//
// otherwise, listElements contains the pointer to the start of an array of
// pointers to char which are the selections
// it must be disposed of at some later time with a call to FreeOpenFileDialogPaths
// NOTE: the array is terminated with a pointer to NULL
//
// NOTE: this is NOT allowed to change the current directory of the editor
{
	return(FileDialog(theTitle,fullPath,stringBytes,listElements,false,false,cancel));
}

void FreeOpenFileDialogPaths(char **thePaths)
// free an array of paths that was returned from OpenFileDialog
{
	FreeFileDialogPaths(thePaths);
}

bool SaveFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel)
// bring up a file save dialog
// if there is a problem, SetError, return false
// otherwise, if cancel is true, the user cancelled
// otherwise, return the full path name of the chosen file
// NOTE: if the fullPath (including terminator) would be larger than stringBytes,
// then it will be truncated to fit
// NOTE ALSO: fullPath may be sent to this routine with a pathname, and the save
// dialog will open to that path (as best as it can)
//
// NOTE: this is NOT allowed to change the current directory of the editor
{
	char
		**thePaths;
	bool
		result;

	result=false;
	if(FileDialog(theTitle,fullPath,stringBytes,&thePaths,true,false,cancel))
	{
		if(!(*cancel))
		{
			if(stringBytes)
			{
				strncpy(fullPath,thePaths[0],stringBytes);
				fullPath[stringBytes-1]='\0';
			}
			FreeFileDialogPaths(thePaths);
		}
		result=true;
	}
	return(result);
}

bool ChoosePathDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel)
// bring up a path choose dialog
// if there is a problem, SetError, return false
// otherwise, if cancel is true, the user cancelled
// otherwise, return the full path name of the chosen path
// NOTE: if the fullPath (including terminator) would be larger than stringBytes,
// then it will be truncated to fit
// NOTE ALSO: fullPath may be sent to this routine with a pathname, and the
// dialog will open to that path (as best as it can)
//
// NOTE: this is NOT allowed to change the current directory of the editor
{
	return(FileDialog(theTitle,fullPath,stringBytes,NULL,true,true,cancel));
}

static char **GetSortedFontList(UINT32 *numElements)
// return a sorted list of fonts, or SetError, and return NULL if there is a problem
// if a list is returned, it must be freed with FreeStringList
{
	int
		actualCount;
	char
		**theList,
		**newList;
	UINT32
		index;
	bool
		fail;

	newList=NULL;
	if((theList=XListFonts(xDisplay,"*",32767,&actualCount)))
	{
		if((newList=NewStringList()))							// copy the returned list, so it can be sorted (since it is not nice to sort the passed list, because we do not really own it, and should not be able to write to it)
		{
			fail=false;
			*numElements=0;
			for(index=0;!fail&&(int)index<actualCount;index++)
			{
				fail=!AddStringToList(theList[index],&newList,numElements);
			}
			if(!fail)
			{
				SortStringList(newList,*numElements);
			}
			else
			{
				FreeStringList(newList);
				newList=NULL;
			}
		}
		XFreeFontNames(theList);
	}
	else
	{
		SetError("font","NoFonts","No Fonts Located");
	}
	return(newList);
}

static UINT32 MatchLength(char *stringA,char *stringB)
// return the number of matching characters between stringA, and stringB
// (the check is case insensitive)
{
	UINT32
		matchIndex;

	matchIndex=0;
	while(stringA[matchIndex]&&(toupper(stringA[matchIndex])==toupper(stringB[matchIndex])))
	{
		matchIndex++;
	}
	return(matchIndex);
}

static UINT32 GetBestMatchIndex(char **theList,UINT32 numElements,char *theFont)
// try to match theFont against theList, return the index of the
// element of theList which matches best
{
	UINT32
		currentMatch,
		bestMatch,
		bestMatchPosition;
	UINT32
		index;

	bestMatch=0;
	bestMatchPosition=0;
	for(index=0;index<numElements;index++)
	{
		if((currentMatch=MatchLength(theFont,theList[index]))>bestMatch)
		{
			bestMatch=currentMatch;
			bestMatchPosition=index;
		}
	}
	return(bestMatchPosition);
}

bool ChooseFontDialog(char *theTitle,char *theFont,UINT32 stringBytes,bool *cancel)
// bring up a font choose dialog
// if there is a problem, SetError, return false
// otherwise, if cancel is true, the user cancelled
// otherwise, return the name of the chosen font in theFont
// NOTE: if the theFont (including terminator) would be larger than stringBytes,
// then it will be truncated to fit
// NOTE ALSO: theFont may be sent to this routine with a font name, and the
// dialog will open to that font (as best as it can)
{
	char
		**theList;
	UINT32
		actualCount;
	bool
		*selectionList,
		done,
		result;
	int
		index;

	if((theList=GetSortedFontList(&actualCount)))
	{
		result=false;
		if((selectionList=(bool *)MNewPtrClr(actualCount*sizeof(bool))))
		{
			selectionList[GetBestMatchIndex(theList,actualCount,theFont)]=true;	// select the item which is closest to the current font
			if((result=SimpleListBoxDialog(theTitle,actualCount,theList,selectionList,cancel)))
			{
				if(!*cancel)
				{
					done=false;
					for(index=0;!done&&index<(int)actualCount;index++)
					{
						if(selectionList[index])	// find first selected item
						{
							if(stringBytes)
							{
								strncpy(theFont,theList[index],stringBytes);
								theFont[stringBytes-1]='\0';
							}
							done=true;
						}
					}
					*cancel=!done;				// if none highlighted, take it as a cancel request
				}
			}
			MDisposePtr(selectionList);
		}
		FreeStringList(theList);
		return(result);
	}
	return(false);
}
