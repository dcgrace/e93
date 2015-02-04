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


#ifndef __WINDOWSGUI__
#define __WINDOWSGUI__

#define	GUI_VERSION			"Windows NT/95/98/2000/ME GUI Version 2.8r2"

#ifdef	LOWORD
#undef	LOWORD
#endif
#define LOWORD(l)			((unsigned short int)(unsigned long int)(l))

#ifdef	HIWORD
#undef	HIWORD
#endif
#define HIWORD(l)			((unsigned short int)((((unsigned long int)(l)) >> 16) & 0xFFFF))

#define LOSIGNEDWORD(l)		((short int)(long int)(l))
#define HISIGNEDWORD(l)		((short int)((((long int)(l)) >> 16) & 0xFFFF))

#define	MAXLINELEN			2048
#define	MAXTASKSEPERATOR	32

#define	EDITMAXWIDTH		32767
#define	EDITMAXHEIGHT		32767

#define IDM_FIRSTCHILD      32768		/* first menu item # for MDI menu item */

#define EDIT_ID				100
#define PRINT_TITLE			200
#define PRINT_REST			201
#define	CURRENT_PAGE		202

#define	IDS_FILTERSTRING	300

#define IDC_SIMPLETEXT_EDIT		101

#define IDC_FINDBUTTON			102
#define IDC_EDITFIND			103
#define IDC_EDITREPLACE			104
#define IDC_CBBACKWARD			105
#define IDC_CBWRAP				106
#define IDC_CBREGEXP			107
#define IDC_CBCASE				108
#define IDC_CBSCOPE				109
#define	IDC_CBTCL				110
#define IDC_FINDALLBUTTON		111
#define IDC_REPLACEBUTTON		112
#define IDC_REPLACEALLBUTTON	113

#define	IDC_LIST				114
#define	IDC_SELECTDIRECTORY		115
#define	IDC_PATH				116

#define EDITORWINDOW_DATA_OFFSET	0
#define STATUSHWND_DATA_OFFSET		4
#define	WIN_TIMER_BOOL_OFFSET		8
#define	WINDOW_DATA_EXTRA_SIZE		12

typedef struct windowListElement			// this structure is used to locate our data based on an x window ID
	{
	HWND
		hwnd;									// the Windows window handle
	EDITORWINDOW
		*theEditorWindow;						// pointer to our editor window structure
	struct windowListElement
		*nextElement,							// next one of our windows, or NULL if none
		*previousElement;						// previous window, or NULL if none
	} WINDOWLISTELEMENT;


/* this definition sets EDITORTASK to the structure needed by the gui */
struct editorTask
	{
	UINT32
		appType;
	HANDLE
		taskStdinRd,
		taskStdinWr,
		taskStdoutRd,
		taskStdoutWr;
	PROCESS_INFORMATION
		taskInfo;
	bool
		closedStdin;
	char
		*completionProc;
	DWORD
		exitCode;
	};

typedef struct
	{
	HMENU
		menuHandle;
	UINT32
		keyCode,
		keyModifiers,
		keyMask,
		menuID;
	bool
		popupMenu;
	EDITORMENU				/* linked list of menus */
		*parent,
		*nextSibling,
		*previousSibling,
		*firstChild;
	char
		*dataText,
		*itemText;			/* name of the item is stored here (semi-redundant with OS's name, but easier to get to) */
	} GUIMENUITEMINFO;

/* editor window types */
enum
	{
	EWT_DOCUMENT,						/* an actual window used to edit text */
	EWT_DIALOG							/* a dialog window */
	};

/* Statbar paint helper structure */
typedef struct
	{
	int         dyBorder;                   /* System Border Width/Height       */
	int         dyBorderx2;                 /* System Border Width/Height * 2   */
	int         dyBorderx3;                 /* System Border Width/Height * 3   */
	int         dyStatbar;                  /* Status Bar height                */
	int			dyInner;					/* Height of the inner part */
	HFONT       hFontStatbar;               /* Font used in status bar */
	} STATUSPAINT;


#define	CreateWindowsRGBCOLOREF(r,g,b)	(COLORREF)(0x02000000 | ((unsigned long)(((unsigned char)(r)|((unsigned short)(g)<<8))|(((unsigned long)(unsigned char)(b)<<16)))))
#define	CreateWindowsINDEXCOLOREF(i)	(COLORREF)(0x01000000 | ((unsigned long)(((unsigned short)(i)))))

#endif
