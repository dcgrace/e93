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

// DCG 2015-02-01: port to t8.5+ window interface
#include "../../tk8.6.3/win/tkWin.h"

#define DONT_REDEFINE // Console app needs native types
#define WBUFLEN 512

#define	CARET_TIMER_ID			1
#define	SYSTEM_TIMER_ID			2
#define	SYSTEM_TIMER_INTERVAL	50		// system timer goes off 20 times a second
#define	EDITOR_TIMER_ID			3
#define	EDITOR_TIMER_INTERVAL	1000	// editor timer goes off 1 time a second

extern int main(int argCount,char *argList[]);

static bool Anothere93Running(int argCount,char *argList[])
// determine if another e93 application is running on this system
{
	HWND
		hwndTmp;
	WINDOWPLACEMENT
		wp;
	HANDLE
		hDropHandle;
	char
		*fileDropBase,
		*fileDropPtr;
	SECURITY_ATTRIBUTES
		sa;
	int
		i,
		size;
	SYSTEM_INFO
		si;
	DWORD
		dwResult;

	if(hwndTmp=FindWindow(frameClass,NULL))
	{
		wp.length=sizeof(wp);
		GetWindowPlacement(hwndTmp,&wp);
		SetForegroundWindow(hwndTmp);
		if(wp.showCmd==SW_SHOWMINIMIZED)
		{
			ShowWindow(hwndTmp,SW_SHOW);
		}
		if(argCount>1)
		{
			GetSystemInfo(&si);
			for(i=1,size=0;i<argCount;i++)
			{
				size+=strlen(argList[i])+1;
			}
			size=((size+si.dwPageSize)/si.dwPageSize)*si.dwPageSize;
			sa.nLength=sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor=NULL;
			sa.bInheritHandle=true;
			if(hDropHandle=CreateFileMapping((HANDLE)0xFFFFFFFF,&sa,PAGE_READWRITE,0,size,frameClass))
			{
				if(fileDropBase=fileDropPtr=(char *)MapViewOfFile(hDropHandle,FILE_MAP_ALL_ACCESS,0,0,size))
				{
					for(i=1;i<argCount;i++)
					{
						strcpy(fileDropPtr,argList[i]);
						fileDropPtr=&(fileDropPtr[strlen(fileDropPtr)+1]);
					}
					fileDropPtr[0]='\0';
					UnmapViewOfFile(fileDropBase);
					SendMessageTimeout(hwndTmp,WM_USER,size,0,SMTO_BLOCK,5000,&dwResult);
				}
				CloseHandle(hDropHandle);
			}
		}
		return true;
	}
	return false;
}

static void TranlateASCIIToViewVirtualKey(EDITORKEY *theKeyData)
//	Translate ASCII keys to the editors virtual keys that the editor knows
{
	theKeyData->isVirtual = true;
	switch(theKeyData->keyCode)
	{
		case '\b':		// backspace
		{
			theKeyData->keyCode=VVK_BACKSPACE;
			break;
		}
		case '\t':		// tab
		{
			theKeyData->keyCode=VVK_TAB;
			break;
		}
		case '\r':		// return
		{
			theKeyData->keyCode=VVK_RETURN;
			break;
		}
		case 0x1B:		// escape
		{
			theKeyData->keyCode=VVK_ESC;
			break;
		}
		default:
		{
			theKeyData->isVirtual = false;
			break;
		}
	}
}

static void TranlateWinVirtualKeyToViewVirtualKey(EDITORKEY *theKeyData)
//	Translate Windows virtual keys to the editors virtual keys that the editor has.
{

	theKeyData->isVirtual = true;
	switch(theKeyData->keyCode)
	{
		case VK_PRIOR:
		{
			theKeyData->keyCode=VVK_PAGEUP;
			break;
		}
		case VK_NEXT:
		{
			theKeyData->keyCode=VVK_PAGEDOWN;
			break;
		}
		case VK_END:
		{
			theKeyData->keyCode=VVK_END;
			break;
		}
		case VK_HOME:
		{
			theKeyData->keyCode=VVK_HOME;
			break;
		}
		case VK_LEFT:
		{
			theKeyData->keyCode=VVK_LEFTARROW;
			break;
		}
		case VK_UP:
		{
			theKeyData->keyCode=VVK_UPARROW;
			break;
		}
		case VK_RIGHT:
		{
			theKeyData->keyCode=VVK_RIGHTARROW;
			break;
		}
		case VK_DOWN:
		{
			theKeyData->keyCode=VVK_DOWNARROW;
			break;
		}
		case VK_INSERT:
		{
			theKeyData->keyCode=VVK_INSERT;
			break;
		}
		case VK_DELETE:
		{
			theKeyData->keyCode=VVK_DELETE;
			break;
		}
		case VK_HELP:
		{
			theKeyData->keyCode=VVK_HELP;
			break;
		}
		case VK_F1:
		case VK_F2:
		case VK_F3:
		case VK_F4:
		case VK_F5:
		case VK_F6:
		case VK_F7:
		case VK_F8:
		case VK_F9:
		case VK_F10:
		case VK_F11:
		case VK_F12:
		case VK_F13:
		case VK_F14:
		case VK_F15:
		case VK_F16:
		case VK_F17:
		case VK_F18:
		case VK_F19:
		case VK_F20:
		{
			theKeyData->keyCode = (theKeyData->keyCode-VK_F1) + VVK_F1;
			break;
		}
		default:
		{
			theKeyData->isVirtual = false;
			break;
		}
	}
}

static UINT32
	lastClickTime=0,
	numClicks=0,
	lastButtonClick=0;
static INT32
	lastClickX=0,
	lastClickY=0;

static UINT32 HandleMultipleClicks(UINT32 buttonNum,INT32 xPos,INT32 yPos)
/* updates and returns numClicks which tells how many clicks were in roughly the same area;
 * where each click followed the previous by no more than GetDoubleClickTime() milliseconds
 * This is how double,triple,etc clicks are handled
 * This should only be called from a WM_BUTTONDOWN message, because it gets the time that message was
 * posted for the last message retrieved by GetMessage() (ie. WM_BUTTONDOWN).
 */
{
	UINT32
		curTime;
	int
		dblWidth,
		dblHeight;

	dblWidth=GetSystemMetrics(SM_CXDOUBLECLK);
	dblHeight=GetSystemMetrics(SM_CYDOUBLECLK);
	curTime=GetMessageTime();											// get the time this message was posted at

	if((lastClickTime + GetDoubleClickTime()) > curTime &&
		(lastClickX>=(xPos-dblWidth))&&(lastClickX<=(xPos+dblWidth)) &&
		(lastClickY>=(yPos-dblHeight))&&(lastClickY<=(yPos+dblHeight)) &&
		buttonNum == lastButtonClick)
	{
		numClicks++;
	}
	else
	{
		numClicks=0;
	}
	lastClickTime=curTime;
	lastClickX=xPos;
	lastClickY=yPos;
	lastButtonClick=buttonNum;
	
	return numClicks;
}

static bool StillDown(UINT32 ticksToStopAt,UINT button)
{
	MSG
		msg;
	bool
		hadEvent,
		mouseStillDown,
		timesUp;

	mouseStillDown=true;
	timesUp=false;
	while(mouseStillDown && !timesUp)
	{
		if(hadEvent=(PeekMessage(&msg,NULL,0,0,PM_REMOVE)!=0))
		{
			switch(msg.message)
			{
				case WM_KEYUP:
				case WM_SYSKEYUP:
				case WM_SYSKEYDOWN:
				case WM_KEYDOWN:
				{
					break;
				}
				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
				case WM_MBUTTONUP:
				{
					if(button==WM_RBUTTONDOWN && msg.message==WM_RBUTTONUP)
					{
						mouseStillDown=false;
					}
					else if(button==WM_LBUTTONDOWN && msg.message==WM_LBUTTONUP)
					{
						mouseStillDown=false;
					}
					else if(button==WM_MBUTTONDOWN && msg.message==WM_MBUTTONUP)
					{
						mouseStillDown=false;
					}
					break;
				}
				case WM_MOUSEWHEEL:
				{
					if(button==WM_MBUTTONDOWN)
					{
						mouseStillDown=false;
					}
					break;
				}
				case WM_MOUSEMOVE:									// don't pass mouse moves on
				{
					break;
				}
				default:
				{
	    		    DispatchMessage(&msg);							// send the message
					break;
				}
			}
		}
		if(mouseStillDown)											// as long as the mouse is down
		{
			if(GetPerformanceCounter()<ticksToStopAt)				// see if we need to wait more
			{
				if(!hadEvent)
				{
					Sleep(0);										// yield time back to other apps
				}
			}
			else
			{
				timesUp=true;
			}

		}
	}
	return mouseStillDown;
}

static void WinClickPosToViewClickPos(EDITORVIEW *theView,INT32 xPos,INT32 yPos,VIEWCLICKEVENTDATA *theClickData)
/*	Convert a windows click position relative to the current client area, to a click position relative to the current view.
	This routine clips the click position so it's always inside the view.
 */
{
	GUIVIEWINFO
		*theViewInfo;

	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
	xPos-=theViewInfo->textBounds.left;
	yPos-=theViewInfo->textBounds.top;
	if(yPos<0)
	{
		yPos-=theViewInfo->lineHeight;
	}
	yPos=yPos/((INT32)(theViewInfo->lineHeight));
	theClickData->xClick=xPos;
	theClickData->yClick=yPos;
}

static void WinClickModToEditorMod(UINT32 numClicks,VIEWCLICKEVENTDATA *theClickData)
/*	Set the clickData modifier field to the correct value based on the state of
	numClicks and the state of the modifier keys on the keyboard. This gets the current state
	of the modifiers.
 */
{

	theClickData->modifiers=0;
	if(GetAsyncKeyState(VK_SHIFT)<0)		/* see if the shift key is pressed */
	{
		theClickData->modifiers|=EEM_SHIFT;
	}
	if(GetAsyncKeyState(VK_CONTROL)<0)		/* see if the control key is pressed */
	{
		theClickData->modifiers|=EEM_CTL;
	}
	if(GetAsyncKeyState(VK_MENU)<0)			/* see if the alt key is pressed */
	{
		theClickData->modifiers|=EEM_MOD0;
	}
	numClicks<<=EES_STATE0;
	numClicks&=EEM_STATE0;
	theClickData->modifiers|=numClicks;		/* set the number of clicks */
}

static bool HandleMouseDownEvent(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
/*	This is called when there is a mouseDown event in the client area of a window. If the click was in a view,
	we will stay here, getting mouse events until we get a mouse up event or we lose focus.
 */
{
	UINT32
		mouseNumClicks;
	VIEWCLICKEVENTDATA
		mouseClickData;
	VIEWEVENT
		mouseViewEvent;
	EDITORWINDOW
		*theWindow;
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	POINT
		pt,
		mousePt;
	bool
		handled;
	INT32
		ticks;

	handled=false;
	theWindow=(EDITORWINDOW *)GetWindowLong(hwnd,EDITORWINDOW_DATA_OFFSET);	/* get a pointer to the editors window */
	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
	mousePt.x=(INT32)LOSIGNEDWORD(lParam);
	mousePt.y=(INT32)HISIGNEDWORD(lParam);
	mouseViewEvent.eventType=VET_CLICK;
	mouseViewEvent.theView=theView;
	mouseViewEvent.eventData=&mouseClickData;
	switch(msg)
	{
		case WM_RBUTTONDOWN:
		{
			mouseClickData.keyCode=2;
			break;
		}
		case WM_MBUTTONDOWN:
		{
			mouseClickData.keyCode=1;
			break;
		}
		case WM_LBUTTONDOWN:
		{
			mouseClickData.keyCode=0;
			break;
		}
	}
	mouseNumClicks=HandleMultipleClicks(mouseClickData.keyCode,(INT32)LOSIGNEDWORD(lParam),(INT32)HISIGNEDWORD(lParam));
	WinClickModToEditorMod(mouseNumClicks,&mouseClickData);
	WinClickPosToViewClickPos(theView,(INT32)mousePt.x,(INT32)mousePt.y,&mouseClickData);
	if((mouseClickData.xClick>=0)&&(mouseClickData.xClick<(theViewInfo->textBounds.right-theViewInfo->textBounds.left)))	/* make sure initial click is in the text part of the view */
	{
		if((mouseClickData.yClick>=0)&&(mouseClickData.yClick<(INT32)theViewInfo->numLines))	/* make sure initial click is in the view */
		{
			ticks=GetPerformanceCounter();
			HandleViewEvent(&mouseViewEvent);
			WaitForDraw(hwnd);
			SetCapture(hwnd);								/* capture mouse clicks */
			if(msg==WM_MBUTTONDOWN)
			{
				SetCursor(LoadCursor(programInstanceHandle,"Hand"));
			}
			while(StillDown(ticks,msg) && theViewInfo->viewActive)
			{
				ticks=GetPerformanceCounter();
				GetCursorPos(&pt);
				ScreenToClient(hwnd,&pt);	/* convert x,y coords to the window */
				WinClickPosToViewClickPos(theView,(INT32)pt.x,(INT32)pt.y,&mouseClickData);
				mouseViewEvent.eventType=VET_CLICKHOLD;
				mouseViewEvent.theView=theView;
				mouseViewEvent.eventData=&mouseClickData;
				HandleViewEvent(&mouseViewEvent);
				WaitForDraw(hwnd);
			}
			if(msg==WM_MBUTTONDOWN)
			{
				SetCursor(LoadCursor(NULL,IDC_IBEAM));
			}
			ReleaseCapture();					/* release the mouse */
    	}
    }
	return handled;
}

static bool HandleMouseWheel(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	int
		num,
		i;
	UINT
		linePerRoll;

	if(SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0,&linePerRoll,0)==0)
	{
		linePerRoll=4;
	}
	theWindow=(EDITORWINDOW *)GetWindowLong(hwnd,EDITORWINDOW_DATA_OFFSET);
	theView=(EDITORVIEW *)theWindow->windowInfo;

	if(HISIGNEDWORD(wParam)>0)
	{
		if(linePerRoll==65535)			// if it's set to a page at a time
		{
			ScrollEditViewEvent(theView,VVK_DOCUMENTPAGEUP,0);	// then do a page
			WaitForDraw(hwnd);
		}
		else
		{
			num=abs(HISIGNEDWORD(wParam)*(INT32)linePerRoll)/WHEEL_DELTA;
			for(i=0;i<num;i++)
			{
				ScrollEditViewEvent(theView,VVK_SCROLLUP,0);
				WaitForDraw(hwnd);
			}
		}
	}
	else
	{
		if(linePerRoll==65535)
		{
			ScrollEditViewEvent(theView,VVK_DOCUMENTPAGEDOWN,0);
			WaitForDraw(hwnd);
		}
		else
		{
			num=abs(HISIGNEDWORD(wParam)*(INT32)linePerRoll)/WHEEL_DELTA;
			for(i=0;i<num;i++)
			{
				ScrollEditViewEvent(theView,VVK_SCROLLDOWN,0);
				WaitForDraw(hwnd);
			}
		}
	}
	
	return true;
}

static bool IsItAKey(UINT32 keyCode)
/*
 *  Passed a Windows virtual key,
 *  return false if it's a windows modifier key,
 *  return true if it's not a modifier key.
 */
{
	bool
		itIsAKey=TRUE;
	UINT32
		i;

	for (i = 0; (ModifierTable[i].windowsKeyCode != 0) && itIsAKey; i++)
	{
		itIsAKey = (keyCode != ModifierTable[i].windowsKeyCode);
	}

	return itIsAKey;
}

/*
	Depending on the keyboard the user has, Windows behaves in different
	ways. On non-US keyboards it will send a VK_LMENU or a VK_RMENU
	depending on the alt key pressed, left or right. On US keyboards it
	just sends VK_MENU in either case.

	Our table only contains VK_LMENU which will match the
	VK_RMENU effectively stealing it from Windows.

	Therefore,  we check explicitly check for a VK_RMENU and never
	use it as a modifier. This should never happen on a US keyboard.
*/

static void WindowsToEditorModifiers(UINT32 *editorModifiers)
/*
 * Convert Windows modifiers to the editor's
 * gets the modifiers for the current message, not the current state.
 */
{
	UINT32
		i;

	(*editorModifiers)=0;

	if (!(GetKeyState(VK_RMENU) & 0XFF80))		// e93 may not use VK_RMENU as a modifier, but it will match VK_MENU from the table, so don't even check
	{
		for (i = 0; (ModifierTable[i].windowsKeyCode != 0); i++)
		{
			if (GetKeyState(ModifierTable[i].windowsKeyCode) & ModifierTable[i].stateTest) // it is pressed
			{
				(*editorModifiers)|=ModifierTable[i].editorKeyCode;
			}
		}
	}
}

static UINT32 RemapEditorVirtualKeys(UINT32 theKey, UINT32 modifiers, bool extended)
/*	Passed a key, the modifiers for the key and whether it is an extended key (numeric keypad keys, right
	Alt and Ctrl key and so on) return a different virtual key for certain special cases. The only case
	we currently cover is the numeric keypad enter key, for which we return virtual key VK_EXECUTE.
 */
{
	if ((theKey==VK_RETURN) && extended)		// map a extended VK_RETURN to a VK_EXECUTE
	{
		theKey = VK_EXECUTE;
	}
	return theKey;
}

// passed a Windows key and modifiers, fill in the translated Editor values
static void WindowsToEditorKeyData(UINT32 *edKey, UINT32 *edModifiers, UINT32 key, UINT32 modifiers)
{
	WindowsToEditorModifiers(edModifiers);
	*edKey = RemapEditorVirtualKeys(key, *edModifiers, (bool)((HIWORD(modifiers) & KF_EXTENDED) !=0));
}

static bool HandleKeyBindingEvent(EDITORKEY *theKeyData)
/*	Passed in a editor key, process the key for bound keys and menus, returning if they
	handled the key.
 */
{
	bool
		handled;

	if(!(handled=HandleBoundKeyEvent(theKeyData->keyCode,theKeyData->modifiers)))
	{
		handled=HandleMenuKeyEvent(theKeyData->keyCode,theKeyData->modifiers);
	}
	return handled;
}

static void UnInitTimers()
//	Undo everything InitTimers did
{
	KillTimer(frameWindowHwnd,EDITOR_TIMER_ID);
	KillTimer(frameWindowHwnd,SYSTEM_TIMER_ID);
	KillTimer(frameWindowHwnd,EDITOR_TIMER_INTERVAL);
	KillTimer(frameWindowHwnd,CARET_TIMER_ID);
}

static bool InitTimers()
// Initialize the timer events
{
	if(SetCursorBlinkTime())											// start caret blinking timer
	{
		if(SetTimer(frameWindowHwnd,SYSTEM_TIMER_ID,SYSTEM_TIMER_INTERVAL,NULL))
		{
			if(SetTimer(frameWindowHwnd,EDITOR_TIMER_ID,EDITOR_TIMER_INTERVAL,NULL))
			{
				return true;
			}
			KillTimer(frameWindowHwnd,SYSTEM_TIMER_ID);
		}
		KillTimer(frameWindowHwnd,CARET_TIMER_ID);
	}
	return false;
}



// things for Windows

int WINAPI WinMain(HINSTANCE inst,HINSTANCE previnst,LPSTR cmdline,int cmdshow)
// WinMain - initialization
{
    programInstanceHandle=inst;
    sprintf(frameClass,"E93FrameClass");
    sprintf(childClass,"E93ChildClass");
    sprintf(tkClass,"E93TkClass");
    theShowCommand=cmdshow;

	ReadOpenAnotherE93RegistrySetting(&openAnotherE93);
	if(openAnotherE93 || !Anothere93Running(__argc,__argv))
	{
		if(main(__argc,__argv))
		{
			char
				*errorFamily,
				*errorFamilyMember,
				*errorDescription;
				
			GetError(&errorFamily,&errorFamilyMember,&errorDescription);
			ReportMessage("Error:\n%s",errorDescription);
		}
	}
	// F_ApiDisconnectFromSession();
	return 0;
}


static INT32
	prevScrollMsg = SB_ENDSCROLL + 1;

LONG PASCAL MdiWindowProc(HWND hwnd,unsigned msg,WPARAM wParam,LPARAM lParam)
//	This is the window proc for an individual MDI child window containing a displayable icon.
{
	bool
		handled = false;
	INT32
		result = 0;
	EDITORWINDOW
		*theWindow;
	EDITORVIEW
		*theView;
	GUIVIEWINFO
		*theViewInfo;
	bool
		repeatFlag;
	VIEWEVENT
		theViewEvent;

	if ((theWindow = (EDITORWINDOW *)GetWindowLong(hwnd, EDITORWINDOW_DATA_OFFSET)) != NULL)	// get a pointer to the editor's window
	{
		if ((theView = (EDITORVIEW *)theWindow->windowInfo) != NULL)
		{
			theViewInfo = (GUIVIEWINFO *)theView->viewInfo;										// point at the information record for this view
		}
	}

   	switch(msg)
	{
    	case WM_CREATE:
		{
			MDICREATESTRUCT
				*lpCreateParams;
				
			// this window has just been created,
			
			// lParam points to the CREATESTRUCT, which contains lpCreateParams,
			lpCreateParams = (MDICREATESTRUCT *)((CREATESTRUCT *)lParam)->lpCreateParams;
			
			// which has a pointer to the editor window in its lParam
			theWindow=(EDITORWINDOW *)lpCreateParams->lParam;
			
			SetWindowLong(hwnd,EDITORWINDOW_DATA_OFFSET,(DWORD)theWindow);		// point the Windows window (hwnd) back to the editor's window structure
			SetWindowLong(hwnd,WIN_TIMER_BOOL_OFFSET,(DWORD)false);
			
			((WINDOWLISTELEMENT *)theWindow->userData)->hwnd = hwnd;			// point the editor's window structure to the Windows window (hwnd)
			theView=(EDITORVIEW *)theWindow->windowInfo;
			theViewInfo = (GUIVIEWINFO *)theView->viewInfo;						// point at the information record for this view
			GetClientRect(hwnd,&(theViewInfo->bounds));
			theViewInfo->bounds.top+=windowStatbarPntData.dyStatbar;
			RecalculateViewFontInfo(theView);
			AdjustDocumentWindowScrollThumbs(theView);
			ValidateRect(hwnd,NULL);
			result=1;
			handled=true;										// we handled this message fully
        	break;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:										// see if this is a virtual key or a bound key, otherwise ignore it
		{
			EDITORKEY
				theKeyData;

			WindowsToEditorKeyData(&theKeyData.keyCode, &theKeyData.modifiers, wParam, lParam);

			handled = HandleKeyBindingEvent(&theKeyData);		// see if this is a bound key
			
			if (!handled)
			{
				TranlateWinVirtualKeyToViewVirtualKey(&theKeyData);
				if  (theKeyData.isVirtual)
				{
					theViewEvent.theView = theView;				// point at the view
					theViewEvent.eventType = VET_KEYDOWN;		// this is a key down event
					theViewEvent.eventData = &theKeyData;

					HandleViewEvent(&theViewEvent);				// pass this off to high level handler
					handled=true;								// we handled this message fully
				}
			}
			break;
		}
		case WM_CHAR:			// this is a printing character, it may have taken several key strokes to generate this (leave that work to Windows)
		{
			EDITORKEY
				theKeyData;

			theKeyData.isVirtual=false;							// we know this is not a virtual key, because it is a character event, not a key event
			theKeyData.modifiers = 0;							// by this point modifiers don't matter, we have an actual ASCII char from the OS, use it
			theKeyData.keyCode = wParam;						// we don't care about lParam (the key code) we only need wParam (the ASCII char)

			theViewEvent.theView = theView;						// point at the view
			theViewEvent.eventType = VET_KEYDOWN;				// this is a key down event
			theViewEvent.eventData = &theKeyData;

			TranlateASCIIToViewVirtualKey(&theKeyData);			// certain ASCII keys map to e93 virtual keys, see if this is one of those
			HandleViewEvent(&theViewEvent);						// pass this off to high level handler
			handled=true;										// we handled this message fully
			break;
		}
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONDOWN:
		{
			handled = HandleMouseDownEvent(hwnd, msg, wParam, lParam);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			handled = HandleMouseWheel(hwnd, msg, wParam, lParam);
			break;
		}
		case WM_SIZE:
		{
			theViewInfo->bounds.left=0;
			theViewInfo->bounds.top=0;
			theViewInfo->bounds.right=LOWORD(lParam);
			theViewInfo->bounds.bottom=HIWORD(lParam);
			theViewInfo->bounds.top+=windowStatbarPntData.dyStatbar;
			RecalculateViewFontInfo(theView);
			AdjustDocumentWindowScrollThumbs(theView);
			// Don't set handled to true. The DefFrameProc() needs to do more work.
			break;
		}
		case WM_MOUSEACTIVATE:
		{
			// we like to eat the mouse click that brought the window forward
			// but the rules to determine that event occured are difficult to ascertain
			// that fact that the active window just changed doesn't tell us anything
			// it could have changed for reasons other than a mouse click
			result = MA_ACTIVATE;
			
			if (false && (LOWORD(lParam) == HTCLIENT))
			{
				result = MA_ACTIVATEANDEAT;
			}
			prevScrollMsg=SB_ENDSCROLL+1;				// set prevScrollMsg to one plus last value, this clears scroll bar repeat for next time */
			handled = true;								// we handled this message fully
			break;
		}
		case WM_SYSCOMMAND:
		{
			SetMDIShouldZoom((0xFFF0 & wParam) == SC_MAXIMIZE);	// save the zoom state of the MDI window
			// Don't set handled to true. The DefFrameProc() needs to do more work.
			break;
		}
		case WM_MDIACTIVATE:
		{
			UnlinkWindowElement((WINDOWLISTELEMENT *)theWindow->userData);	/* remove this window from the list */
			LinkWindowElement((WINDOWLISTELEMENT *)theWindow->userData);	/* put it back on the list as the top one */
			if (wParam == 0)												/* if there is no previous window being deactivated */
			{
				/*	this fixes a problem that the very first window we open
					would not get the focus. So we tell it to get the focus
					if it's the active window. Fixes the problem?
				*/
				SendMessage(clientWindowHwnd, WM_SETFOCUS, (WPARAM)hwnd, 0L);
			}
			InvalidateWindowStatusBar(hwnd);
			handled=true;								// we handled this message fully
			break;
		}
		case WM_SETFOCUS:
		{			
			UpdateWindow(hwnd);
			ViewStartSelectionChange(theView);
			theViewInfo->viewActive=true;
			ViewEndSelectionChange(theView);
			handled=true;								// we handled this message fully
			break;
		}
		case WM_KILLFOCUS:
		{
			ViewStartSelectionChange(theView);
			theViewInfo->viewActive=false;
			ViewEndSelectionChange(theView);
			handled=true;								// we handled this message fully
			break;
		}
		case WM_ERASEBKGND:
		{
			result=1;
			handled=true;
			break;
		}
		case WM_PAINT:
		{
			HRGN
				updateRegion;
			HDC
				hdc;
			PAINTSTRUCT
				ps;
			HPALETTE
				savePalette;

			updateRegion=CreateRectRgn(0,0,0,0);
			if(GetUpdateRgn(hwnd,updateRegion,false)!=NULLREGION)				/* if there is an update region */
			{
				hdc=BeginPaint(hwnd,&ps);										/* update it */
				savePalette=SelectPalette(hdc,editorPalette,false);
				RealizePalette(hdc);
				FillWindowStatusBar(hwnd,hdc);									/* clear out the status bar */
				DrawWindowStatusBar(hwnd,hdc);									/* fill in info about that window in the status bar */
				DrawView(hwnd,hdc,updateRegion);
				SelectPalette(hdc,savePalette,false);
				RealizePalette(hdc);
				EndPaint(hwnd,&ps);
				handled=true;														// we handled this message fully
				result=1;
			}
			DeleteObject(updateRegion);
        	break;
		}
		case WM_NCLBUTTONUP:
		{
			prevScrollMsg=SB_ENDSCROLL+1;		/* set prevScrollMsg to one plus last value, this clears scroll bar repeat for next time */
			handled=true;						// we handled this message fully
			break;
		}
		case WM_HSCROLL:
		{
			if(prevScrollMsg==LOWORD(wParam))	/* if it was the same message as last time, set repeatFlag true */
			{
				repeatFlag=true;
			}
			else
			{
				prevScrollMsg=LOWORD(wParam);	/* else, set the previous scroll message to the current one */
				repeatFlag=false;
			}
			switch(LOWORD(wParam))
			{
				case SB_PAGELEFT:
				{
					ScrollEditViewEvent(theView,VVK_DOCUMENTPAGELEFT,repeatFlag);
					break;
				}
				case SB_PAGERIGHT:
				{
					ScrollEditViewEvent(theView,VVK_DOCUMENTPAGERIGHT,repeatFlag);
					break;
				}
				case SB_LINELEFT:
				{
					ScrollEditViewEvent(theView,VVK_SCROLLLEFT,repeatFlag);
					break;
				}
				case SB_LINERIGHT:
				{
					ScrollEditViewEvent(theView,VVK_SCROLLRIGHT,repeatFlag);
					break;
				}
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
				{
					ScrollEditViewThumbHorizontalEvent(theView,HIWORD(wParam));
					break;
				}
			}
			handled=true;						// we handled this message fully
			break;
		}
		case WM_VSCROLL:
		{
			if(prevScrollMsg==LOWORD(wParam))	/* if the last time we were here it was the same message, set repeatFlag true */
			{
				repeatFlag=true;
			}
			else
			{
				prevScrollMsg=LOWORD(wParam);	/* else, set the previous scroll message to the current one */
				repeatFlag=false;
			}
			switch(LOWORD(wParam))
			{
				case SB_PAGEUP:
				{
					ScrollEditViewEvent(theView,VVK_DOCUMENTPAGEUP,repeatFlag);
					break;
				}
				case SB_PAGEDOWN:
				{
					ScrollEditViewEvent(theView,VVK_DOCUMENTPAGEDOWN,repeatFlag);
					break;
				}
				case SB_LINEUP:
				{
					ScrollEditViewEvent(theView,VVK_SCROLLUP,repeatFlag);
					break;
				}
				case SB_LINEDOWN:
				{
					ScrollEditViewEvent(theView,VVK_SCROLLDOWN,repeatFlag);
					break;
				}
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
				{
					ScrollEditViewThumbVerticalEvent(theView,HIWORD(wParam));
					break;
				}
			}
			handled=true;						// we handled this message fully
			break;
		}
    	case WM_CLOSE:
		{
			result=!HandleShellCommand("close",1,(char **)&(theWindow->theBuffer->contentName));	/* return 0 if we can close */
			handled=true;
			break;
		}

	}
	
	if(!handled)	/* if the message was not handled by me, call the default MDI window handler to handle it */
	{
        result=DefMDIChildProc(hwnd,msg,wParam,lParam);
	}
	
    return result;
}
	
LONG PASCAL MPWindowProc(HWND hwnd,unsigned msg,WPARAM wParam,LPARAM lParam)
//	FrameProc is the window procedure for the Frame window.
{
	INT32
		result = 0;
	BOOL
		handled = false;
 	
 	switch(msg)
	{
		case WM_TIMER:
		{
			switch(wParam)
			{
				case CARET_TIMER_ID:
				{
					if(IsWindow(clientWindowHwnd))						/* see if there is a client window yet */
					{
						HWND
							activeWin=(HWND)LOWORD(SendMessage(clientWindowHwnd,WM_MDIGETACTIVE,0,0L));	/* yes, get the active MDI child */

						if(IsWindow(activeWin))
						{
							EDITORWINDOW
								*theWindow=(EDITORWINDOW *)GetWindowLong(activeWin,  EDITORWINDOW_DATA_OFFSET);

							BlinkViewCursor((EDITORVIEW *)theWindow->windowInfo);
							handled=true;
						}
					}
					break;
				}
				case SYSTEM_TIMER_ID:						// call editor 20 times a second
				{
					UpdateBufferTasks();
					handled=true;
					break;
				}
				case EDITOR_TIMER_ID:						// call TCL once a second
				{
					HandleShellCommand("timer",0,NULL);		// service editor shell timer
					handled=true;
					break;
				}
			}
			break;
		}
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONDOWN:
		{
			char
				*cmdString;

			//	Had a mouse down in the frame window, check and see if the user has clicked on a button
			//	in the status bar. If so pass the buttons command string to the shell.
			if(TrackStatusButton((INT32)LOSIGNEDWORD(lParam),(INT32)HISIGNEDWORD(lParam),&cmdString))
			{
				ExecuteTclCommand(cmdString);
				handled=true;
			}
			break;
		}
    	case WM_CREATE:
		{
			HDC
				hdc;

			hdc=GetDC(hwnd);
			bitsPerPixel=GetDeviceCaps(hdc,BITSPIXEL);
			ReleaseDC(hwnd,hdc);
			handled=true;
        	break;
		}
    	case WM_DESTROY:
		{
			if(windowsHelpActive)					/* if Windows help was active */
			{
				windowsHelpActive=false;			/* mark not active anymore */
				WinHelp(frameWindowHwnd,NULL,HELP_QUIT,0);	/* and tell it to quit */
			}
			UnregisterControlBreakHotKey();
        	PostQuitMessage(0);
			handled=true;
        	break;
		}
    	case WM_COMMAND:
		{
			handled=ProcessWindowsMenuEvent(LOWORD(wParam));
        	break;
		}
    	case WM_QUERYENDSESSION:
    	case WM_CLOSE:
		{
			result=HandleShellCommand("quit",0,NULL);	/* return true if we can quit */
			handled=true;
			break;
        }
		case WM_ACTIVATE:
		{
			if(LOWORD(wParam) != WA_INACTIVE)
			{
				if(importWindowsClipboardData)			/* should we import Windows Clipboard Data */
				{
					result=ImportWindowsClipboard();	/* yes, get windows clipboard data */
				}
			}
			handled=true;
			break;
		}
		case WM_ACTIVATEAPP:
		{
			if(wParam)									/* if the editor is being activated */
			{
    			RegisterControlBreakHotKey();
				if(importWindowsClipboardData)			/* and should we import Windows Clipboard Data */
				{
					result=ImportWindowsClipboard();	/* yes, get windows clipboard data */
				}
			}
			else
			{
				UnregisterControlBreakHotKey();
			}
			handled=true;
			break;
		}
		case WM_DESTROYCLIPBOARD:
		{
			importWindowsClipboardData=true;			/* outside world has something new in the clipboard, allow us to import it when we are activated */
			handled=true;
			break;
		}
		case WM_SIZE:
		{
			RECT
				theRect;
				
			if(IsWindow(clientWindowHwnd))
			{
				GetClientRect(hwnd,&theRect);					/* get the size of the client window */
				theRect.bottom-=globalStatbarPntData.dyStatbar;	/* make it smaller to account for the status bar */
				MoveWindow(clientWindowHwnd,0,theRect.top,theRect.right,theRect.bottom,true);
			}
			handled=true;
			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT
		    	ps;
			HDC
				hdc;
		    	
			hdc=BeginPaint(hwnd,&ps);
			FillGlobalStatusBar(hwnd,hdc);		/* clear out the status bar */
			DrawGlobalStatusFields(hwnd,hdc);	/* draw any Tcl defined status areas */
			EndPaint(hwnd,&ps);
			handled=true;
			break;
		}
        case WM_PALETTECHANGED:
		{
			if((HWND)wParam==hwnd)
			{
				break;
			}
			/* fall through to WM_QUERYNEWPALETTE */
		}
		case WM_QUERYNEWPALETTE:
		{
			InvalidateRect(hwnd,NULL,true);
			result=false;
			handled=true;
			break;
		}
		case WM_DROPFILES:
		{
			int
				i,
				size,
				argc;
			UINT32
				numElements;
			char
				tmpStr[_MAX_PATH+1],
				**theList;
			bool
				fail;

			fail=false;
			argc=DragQueryFile((HDROP)wParam,(UINT32)-1,NULL,0);
			if(theList=NewStringList())
			{
				numElements=0;
				for(i=0;!fail && (i<argc);i++)
				{
				    size=DragQueryFile((HDROP)wParam,i,NULL,0);
				    DragQueryFile((HDROP)wParam,i,tmpStr,_MAX_PATH);
					fail=!AddStringToList(tmpStr,&theList,&numElements);
				}
				if(!fail)
				{
					HandleShellCommand("open",numElements,theList);
				}
				FreeStringList(theList);
			}
			DragFinish((HDROP)wParam);
			handled=true;
			break;
		}
		case WM_USER:
		{
			UINT32
				numElements;
			char
				*fileDropBase,
				*fileDropPtr,
				**theList;
			bool
				fail;
			HANDLE
				dropHandle;

			fail=false;
			if(dropHandle=OpenFileMapping(FILE_MAP_READ,false,frameClass))
			{
				if(fileDropBase=fileDropPtr=(char *)MapViewOfFile(dropHandle,FILE_MAP_READ,0,0,wParam))
				{
					if(theList=NewStringList())
					{
						numElements=0;
						while(!fail && strlen(fileDropPtr)!=0)
						{
							fail=!AddStringToList(fileDropPtr,&theList,&numElements);
							fileDropPtr=&(fileDropPtr[strlen(fileDropPtr)+1]);
						}
						if(!fail)
						{
							HandleShellCommand("open",numElements,theList);
						}
						FreeStringList(theList);
					}
					UnmapViewOfFile(fileDropBase);				// unlock the global memory
				}
				CloseHandle(dropHandle);
			}
			handled=true;
			break;
		}
		case WM_DISPLAYCHANGE:
		{
			WINDOWLISTELEMENT
				*theElement;
			EDITORWINDOW
				*theWindow;
			WINDOWPLACEMENT
				newPlacement,
				oldPlacement;

			oldPlacement.length=sizeof(WINDOWPLACEMENT);	// make sure the window changes size for the new display size
			GetWindowPlacement(hwnd,&oldPlacement);
			newPlacement.length=sizeof(WINDOWPLACEMENT);
			newPlacement=oldPlacement;
			newPlacement.showCmd=SW_SHOWMINIMIZED;
			SetWindowPlacement(hwnd,&newPlacement);
			SetWindowPlacement(hwnd,&oldPlacement);
			bitsPerPixel=wParam;
			theElement=windowListHead;
			while(theElement)								// update each windows color indexes
			{
				theWindow=(EDITORWINDOW *)GetWindowLong(theElement->hwnd,EDITORWINDOW_DATA_OFFSET);
				ReAdjustDocumentColors(theWindow);
				theElement=theElement->nextElement;
			}
			handled=true;
			break;
		}
    }
    
	if(!handled)
	{
       	result=DefFrameProc(hwnd,clientWindowHwnd,msg,wParam,lParam);
	}

    return result;
}

LONG PASCAL TkWindowProc(HWND hwnd,unsigned msg,WPARAM wParam,LPARAM lParam)
//	This is the window procedure for the dummy Tk main window.
{
	INT32
		result = 0;
	BOOL
		handled = false;
 	
 	switch(msg)
	{
		// DCG 2015-02-01: added support for Tk8.5+ container checking
		case TK_INFO:
		{
			switch(LOWORD(wParam))
			{
				case TK_CONTAINER_VERIFY:
				{
					return (LONG)hwnd;
				}
				case TK_CONTAINER_ISAVAILABLE:
				{
					return 1;
				}
			}
			break;
		}
	}
	return DefMDIChildProc(hwnd,msg,wParam,lParam);
}

// things for wgui

bool SetCursorBlinkTime()
{
	KillTimer(frameWindowHwnd,CARET_TIMER_ID);											// stop the caret timer
   	return SetTimer(frameWindowHwnd,CARET_TIMER_ID,GetCaretBlinkTime(),NULL) != 0;		// start caret blinking timer
}

// implementation of things from guidefs.h

void EditorEventLoop(int argc,char *argv[])
//	This is the editors main event loop.
{
	MSG
		msg;
	
	HandleShellCommand("initialargs",argc-1,&(argv[1]));					// pass off initial arguments to shell for processing
	ImportWindowsClipboard();												// get windows clipboard data
	importWindowsClipboardData=true;										// we are not the owner of the clipboard since we just started
	InitTimers();															// start the timed events running
    Tcl_SetServiceMode(TCL_SERVICE_ALL);
    
	while (GetMessage(&msg, NULL, 0, 0))
	{
		bool
			handled = false;
		   	
	   	switch(msg.message)
		{
			case WM_SYSCHAR:												// steal WM_SYSCHARs otherwise Windows puts up the main menu when you press alt
			case WM_SYSKEYUP:												// steal WM_SYSKEYUPs otherwise Windows puts up the main menu when you press alt
			{
				handled=true;												// we ignored this message fully
				break;
			}
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:												// check for bound keys, otherwise let the OS have it
			{
				EDITORKEY
					theKeyData;

				WindowsToEditorKeyData(&theKeyData.keyCode, &theKeyData.modifiers, msg.wParam, msg.lParam);
				
				handled = HandleKeyBindingEvent(&theKeyData);				// see if this is a bound key
				break;
			}
		}

		if (!handled)														// this was not a bound key, let the OS dispatch the event to where it should go
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	Tcl_SetServiceMode(TCL_SERVICE_NONE);
	UnInitTimers();															// the timed events may call Tcl and Tcl is about to go away, so stop the timed events now
}


static char
	windowID[10];

// make a Window that is a child of the main e93 window, 
// return its ID as a string that can be passed as an identifier to TK
char *GetMainWindowID()
{
	CLIENTCREATESTRUCT
		ccs;

	HWND
		tclWindowHwnd = CreateWindow
			(
			tkClass,
			0L,
			WS_CHILD|WS_DISABLED,
			0,
			0,
			0,
			0,
			frameWindowHwnd,
			NULL,
			programInstanceHandle,
			(LPSTR)&ccs
			);

	sprintf(windowID, "0x%08X", (unsigned int)tclWindowHwnd);
	
	return windowID;
}

void ClearAbort()
// call this to clear any pending abort status
{
	MSG
		msg;

	while(PeekMessage(&msg,NULL,WM_HOTKEY,WM_HOTKEY,PM_REMOVE));
}

		
bool CheckAbort()
/* This will return TRUE if the user is requesting an abortion of the
 * current job
 */
{
	MSG
		msg;
	bool
		userWantsToAbort=false;

	if (PeekMessage(&msg,NULL,WM_HOTKEY,WM_HOTKEY,PM_REMOVE))			// get the hot key off the queue
	{
	   	if (msg.message == WM_HOTKEY)									// got a "HotKey"
		{
			SetError("MISC","USERABORT","User Aborted");				// it was, set the error to abort
			userWantsToAbort=true;										// return true
		}
	}

	return userWantsToAbort;
}

void EditorQuit()
// this is called from the editor when it wants to quit
// it needs to inform the event loop that it should stop looping
{
    PostQuitMessage(0);	
}

void EditorSetModal()
// become modal (disallow clicks and keystrokes in editor windows)
{
	EnableWindow(frameWindowHwnd, false);
}

void EditorClearModal()
// return to normal operations
{
	EnableWindow(frameWindowHwnd, true);
}

bool EditorGetKeyPress(UINT32 *keyCode,UINT32 *editorModifiers,bool wait,bool clearBuffered)
/* used by editor when it wants to get a key press
 * if a code is found, it is returned in keyCode/editorModifiers
 * if none is ready, return false
 * if wait is TRUE, wait here until one is ready (TRUE is always returned)
 * if clearBuffered is TRUE, then clear any keypresses that may have been in the
 * buffer before proceeding.
 */
{
	MSG
		msg;
	bool
		done = false,
		gotOne = false;

	if (clearBuffered)
	{
		while(!done && (HIWORD(GetQueueStatus(QS_KEY)) & QS_KEY))
		{
			done =! PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE);
		}
	}

	while (!done && !gotOne)
	{
		if (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
		{
			if (msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYDOWN)
			{
				if (IsItAKey(msg.wParam))
				{
					WindowsToEditorKeyData(keyCode, editorModifiers, msg.wParam, msg.lParam);
					gotOne=true;
				}
			}
			else
			{
				DispatchMessage(&msg);				// send the message
			}
		}
		else
		{
			if (!wait)
			{
				done=true;
			}
		}
	}
	
	return gotOne;
}
