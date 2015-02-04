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

typedef struct KeyMap
	{
    	UINT32	windowsKeyCode; // a 0 here is invalid as a key-code and can be used as a terminator
    	UINT32	editorKeyCode;
    	INT		stateTest;      // windows returns special case keystates depending on if the key can be toggled like caps lock
	}
	KeyMap;

extern UINT32
	bitsPerPixel;
extern INT32
	theShowCommand;
extern char
	frameClass[],
	childClass[],
	tkClass[];
extern HWND
	frameWindowHwnd,    /* The classic MDI frame window     */
	clientWindowHwnd;   /* And the client window attached   */
extern HINSTANCE
	programInstanceHandle;       /* Convenient to have handy         */
extern bool
	openAnotherE93,
	windowsHelpActive,
	importWindowsClipboardData;
extern int
	translationModeOn;
extern char
	taskSeperatorString[];
extern WINDOWLISTELEMENT
	*windowListHead;		/* head of the window list */
extern STATUSPAINT
	globalStatbarPntData,
	windowStatbarPntData;
extern HPALETTE
	editorPalette;
extern KeyMap
	ModifierTable[];
