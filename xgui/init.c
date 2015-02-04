// Low level stuff needed to init machine
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

static char localErrorFamily[]="Xguiinit";

enum
{
	BADOPEN
};

static char *errorMembers[]=
{
	"BadOpen"
};

static char *errorDescriptions[]=
{
	"Could not open display"
};

bool InitEnvironment()
// Initialize the machine, software libraries, etc...
// if there is a problem, set the error and return false
{
	windowListHead=NULL;						// no windows are open on the display yet
	XSetErrorHandler(XErrorEventHandler);		// set error handling routine
	if((xDisplay=XOpenDisplay(NULL)))			// connect to the default X server
	{
		xScreenNum=DefaultScreen(xDisplay);		// keep this around for handy future reference
		if(InitColors())
		{
			selectionAtom=XInternAtom(xDisplay,"SELECTED_DATA",False);			// define atoms needed by top-level windows
			incrAtom=XInternAtom(xDisplay,"INCR",False);						// incremental selection passing atom
			takeFocusAtom=XInternAtom(xDisplay,"WM_TAKE_FOCUS",False);
			deleteWindowAtom=XInternAtom(xDisplay,"WM_DELETE_WINDOW",False);

			statusBackgroundColor=gray3;										// set up colors
			statusForegroundColor=black;

			if(InitPixmaps())
			{
				if(InitFonts())
				{
					if(InitEditorMenus())
					{
						if(InitEvents())
						{
							timer=0;								// timer ticks
							ResetCursorBlinkTime();					// reset the timeout value of the cursor blink timer
							StartAlarmTimer();						// start the ualarm timer running (it wakes us to check events, and also updates timers)
							showingBusy=0;							// reset cursor busy depth
							verticalScrollBarOnLeft=false;			// by default keep the scroll bar on the right
							caretCursor=XCreateFontCursor(xDisplay,XC_xterm);	// create the pointer cursor to use when inside view
							arrowCursor=XCreateFontCursor(xDisplay,XC_arrow);	// create the arrow cursor to use for other things
							timeToQuit=false;						// clear the quit request flag
							return(true);
						}
						UnInitEditorMenus();
					}
					UnInitFonts();
				}
				UnInitPixmaps();
			}
			UnInitColors();
		}
		XCloseDisplay(xDisplay);
	}
	else
	{
		SetError(localErrorFamily,errorMembers[BADOPEN],errorDescriptions[BADOPEN]);
	}
	return(false);
}

void UnInitEnvironment()
// undo what Init did
{
	XFreeCursor(xDisplay,arrowCursor);
	XFreeCursor(xDisplay,caretCursor);
	UnInitEvents();
	UnInitEditorMenus();
	UnInitFonts();
	UnInitPixmaps();
	UnInitColors();
	XCloseDisplay(xDisplay);
}

bool EarlyInit()
// This is called before anything else in the editor, it can be used to fork off the
// editor as a separate process, or do anything which may need to be done very early
// if this returns false, the editor will exit without complaint
{
	return(true);
}

void EarlyUnInit()
// This is the very last thing that the editor calls before exiting
{
}
