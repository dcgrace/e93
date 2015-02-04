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

UINT32
	bitsPerPixel;
INT32
	theShowCommand;
char
	frameClass[32]="FrameClass",
	childClass[32]="ChildClass",
	tkClass[32]="TkClass";
HWND
	frameWindowHwnd,    		/* The classic MDI frame window */
	clientWindowHwnd;   		/* And the client window attached */
HINSTANCE
	programInstanceHandle;		/* Convenient to have handy */
bool
	openAnotherE93,
	tclInited=false,
	windowsHelpActive,			/* TRUE if we have a help Window active */
	importWindowsClipboardData;	/* TRUE if there is Windows' clipboard data to get */
int
	translationModeOn=0;
char
	taskSeperatorString[MAXTASKSEPERATOR+1];
WINDOWLISTELEMENT
	*windowListHead;			/* head of the window list */
STATUSPAINT
	globalStatbarPntData,
	windowStatbarPntData;
HPALETTE
	editorPalette;
KeyMap						/* map of windows virtual-keys to editor virtual-keys */
	ModifierTable[] =
	{
		{ VK_CONTROL,	EEM_MOD0,		0XFF80 },	// put them in order of most common conventional use
		{ VK_SHIFT,		EEM_SHIFT,		0XFF80 },
		{ VK_MENU,		EEM_CTL,		0XFF80 },
		{ VK_ESCAPE,	EEM_MOD3,		0XFF80 },
		{ VK_APPS,		EEM_MOD1,		0XFF80 },
		{ VK_CAPITAL,	EEM_CAPSLOCK,	0X0001 },
		
		//	no equivalents yet defined for the following
		//		{ VK_??,		EEM_MOD4,		0XFF80 },
		//		{ VK_??,		EEM_MOD5,		0XFF80 },
		//		{ VK_??,		EEM_MOD6,		0XFF80 },
		//		{ VK_??,		EEM_MOD7,		0XFF80 },

		{ 0, 0, 0}
	};

