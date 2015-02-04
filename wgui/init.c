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

void UnInitEnvironment()
/*	Uninitialize colors, and menus
 */
{
	UnInitFonts();
	UninitMenuManager();
	UninitColors();
}

bool InitEnvironment()
/*	Initialize globals, colors and menus
 */
{
	bool
		result;

	result=false;
	windowListHead=NULL;
	if(InitColors())
	{
		if(InitMenuManager())
		{
    		if(InitFonts())
    		{
				result=true;
			}
		}
	}
	return(result);
}

static bool FirstInstance(HINSTANCE inst)
/*
 * FirstInstance - register window class for the application,
 *		   and do any other application initialization
 */
{
    WNDCLASS
		wc;
    wc.hInstance=inst;
    wc.hIcon=LoadIcon(inst,"E93Icon");

    /* Setup frame class*/
    wc.style=CS_HREDRAW|CS_VREDRAW|CS_BYTEALIGNCLIENT|CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc=MPWindowProc;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground=(HBRUSH)(COLOR_APPWORKSPACE+1);
    wc.lpszMenuName=NULL;
    wc.lpszClassName=frameClass;
    if(!RegisterClass(&wc))
	{
		SetWindowsError();
		return(false);
	}

	/* set up and register the MDI child window class */
	wc.style=CS_HREDRAW|CS_VREDRAW|CS_BYTEALIGNCLIENT|CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc=MdiWindowProc;
    wc.cbClsExtra=0;
    wc.cbWndExtra=WINDOW_DATA_EXTRA_SIZE;
	wc.hCursor=LoadCursor(NULL,IDC_IBEAM);
	wc.hbrBackground=NULL;
    wc.lpszMenuName=NULL;
    wc.lpszClassName=childClass;
    if(!RegisterClass(&wc))
	{
		SetWindowsError();
		return(false);
	}

	/* set up and register the MDI child window class */
    wc.style=CS_HREDRAW|CS_VREDRAW|CS_BYTEALIGNCLIENT|CS_BYTEALIGNWINDOW;
    wc.lpfnWndProc=TkWindowProc;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=NULL;
    wc.lpszMenuName=NULL;
    wc.lpszClassName=tkClass;
    if(!RegisterClass(&wc))
	{
		SetWindowsError();
		return(false);
	}

	return(true);
}

static bool CreateFrameWindow(HINSTANCE inst,int cmdshow)
/*
 * CreateFrameWindow - do work required for every instance of the application:
 *		  create the window,initialize data
 */
{
    bool
		result;
	INT32
		xPos,
		yPos,
		width,
		height;
	bool
		dataInReg,
		maximized,
		maximizedMDI;

	result=false;


	if(!ReadRegistrySettings(&xPos,&yPos,&width,&height,&maximized,&maximizedMDI,&openAnotherE93,&dataInReg) || !dataInReg)
	{
		xPos=CW_USEDEFAULT;		/* if there was no registry entry */
		yPos=CW_USEDEFAULT;		/* use defaults */
		width=CW_USEDEFAULT;
		height=CW_USEDEFAULT;
		maximized=FALSE;
	}

	InitMDIShouldZoom(maximizedMDI);	// set to true if MDI windows should be maximized when created
	clientWindowHwnd=0;

	frameWindowHwnd=CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW|WS_EX_ACCEPTFILES,
		frameClass,				/* class */
		_pgmptr,				/* caption */
		WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,	/* style */
		xPos,			/* init. x pos */
		yPos,			/* init. y pos */
		width,			/* init. x size */
		height,			/* init. y size */
		NULL,			/* parent window */
		NULL,			/* menu handle */
		inst,			/* program handle */
		NULL   			/* create parms */
		);

   	if(frameWindowHwnd)
	{
		if(maximized)
		{
			ShowWindow(frameWindowHwnd,SW_MAXIMIZE);
		}
		else
		{
			ShowWindow(frameWindowHwnd,cmdshow);
		}
		UpdateWindow(frameWindowHwnd);
		result=true;
	}
	else
	{
		SetWindowsError();
	}
    return(result);
}

static void DestroyFrameWindow()
/*	Get rid of the frame window
 */
{
	WINDOWPLACEMENT
		wp;

	wp.length=sizeof(wp);
	GetWindowPlacement(frameWindowHwnd,&wp);	// before destroying the window, get its size and maximize state and the maximize state of the MDI windows
	WriteRegistrySettings(wp.rcNormalPosition.left,wp.rcNormalPosition.top,wp.rcNormalPosition.right-wp.rcNormalPosition.left,wp.rcNormalPosition.bottom-wp.rcNormalPosition.top,(IsZoomed(frameWindowHwnd)!=0),MDIShouldZoom(),openAnotherE93);
	DestroyWindow(frameWindowHwnd);
}

void EarlyUnInit()
/*	Undo everything EarlyInit did
 */
{
	DestroyFrameWindow();
	UninitStatusBar();
}

bool EarlyInit()
/*	Create classes and the frame window
 */
{
	if(InitStatusBar())
	{
		if(FirstInstance(programInstanceHandle))
		{
		    if(CreateFrameWindow(programInstanceHandle,theShowCommand))
			{
				return(true);
			}
		}
		UninitStatusBar();
	}
	return(false);
}
