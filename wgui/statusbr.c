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

#define	RUNNING_TASK_STR	"Running"
#define	IDLE_TASK_STR		"Idle"

#define	DEFAULT_FONT	"-*-courier-medium-r-normal-*-13-*-*-*-*-*"

#define	MAXSTATUSFIELDS	64

enum
{
	STATUSFIELDTYPE_TEXT,
	STATUSFIELDTYPE_BUTTON
};

typedef struct
{
	UINT32
		type,
		width;
	char
		*cmdString,
		*string;
} STATUSFIELD;

static STATUSFIELD
	theStatusField[MAXSTATUSFIELDS];

static char localErrorFamily[]="Wgui";

enum
{
	STATUSFIELDOUTOFRANGE
};

static char *errorMembers[]=
{
	"StatusFieldOutOfRange"
};

static char *errorDescriptions[]=
{
	"Field number for SetStatusField is out of range"
};


static UINT32 GetStringWidthInPixels(HDC hdc,char *string)
/*	Passed a HDC that has a font selected, and a string, return the number of pixels
	the string is.
 */
{
	SIZE
		size;

	GetTextExtentPoint32(hdc,string,strlen(string),&size);
	return((UINT32)size.cx);
}

static void DrawStatusBarButtonRect(HWND statusHwnd,HDC hdc,RECT *theTextRect,STATUSPAINT *StatbarPntData)
/*	Draw the 3D button area in the status bar
 */
{
	RECT
		rcTemp;

	rcTemp=*theTextRect;

//	top shadow line
	rcTemp.top=rcTemp.top+StatbarPntData->dyBorderx3;
	rcTemp.bottom=rcTemp.top+StatbarPntData->dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DHILIGHT));
//	left side shadow line
	rcTemp.bottom=theTextRect->bottom-StatbarPntData->dyBorderx3;
	rcTemp.right=rcTemp.left+StatbarPntData->dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DHILIGHT));

	rcTemp=*theTextRect;

//	bottom highlight line
	rcTemp.top=rcTemp.bottom-StatbarPntData->dyBorderx2;
	rcTemp.bottom=rcTemp.top+StatbarPntData->dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DSHADOW));
//	right side highlight line
	rcTemp.top=theTextRect->top+StatbarPntData->dyBorderx3;
	rcTemp.bottom=theTextRect->bottom-StatbarPntData->dyBorder;
	rcTemp.left=theTextRect->right;
	rcTemp.right=rcTemp.left+StatbarPntData->dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DSHADOW));
}


static void DrawStatusBarTextRect(HWND statusHwnd,HDC hdc,RECT *theTextRect,STATUSPAINT *StatbarPntData)
/*	Draw the 3D text area in the status bar
 */
{
	RECT
		rcTemp;

	rcTemp=*theTextRect;
//
//	top shadow line
//

	rcTemp.top=rcTemp.top+StatbarPntData->dyBorderx3;
	rcTemp.bottom=rcTemp.top+StatbarPntData->dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DSHADOW));

//
//	left side shadow line
//

	rcTemp.bottom=theTextRect->bottom-StatbarPntData->dyBorderx3;
	rcTemp.right=rcTemp.left+StatbarPntData->dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DSHADOW));

	rcTemp=*theTextRect;

//
//	bottom highlight line
//

	rcTemp.top=rcTemp.bottom-StatbarPntData->dyBorderx2;
	rcTemp.bottom=rcTemp.top+StatbarPntData->dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DHILIGHT));
//
//	right side highlight line
//

	rcTemp.top=theTextRect->top+StatbarPntData->dyBorderx3;
	rcTemp.bottom=theTextRect->bottom-StatbarPntData->dyBorder;
	rcTemp.left=theTextRect->right;
	rcTemp.right=rcTemp.left+StatbarPntData->dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DHILIGHT));
}

static UINT32 CalcDrawRects(RECT *statusRect,RECT *areaRect,RECT *textRect,UINT32 xOffset,UINT32 width,STATUSPAINT *StatbarPntData)
/*	Pass the status bar rect, a width of the size of an area in the status bar and a horz offset,
	calculate 3D area rect and text rect, and return the horz offset for the next status area.
 */
{
	*areaRect=*statusRect;
	areaRect->bottom=areaRect->top+StatbarPntData->dyInner;
	areaRect->left+=xOffset;
	areaRect->right=areaRect->left+width+StatbarPntData->dyBorderx3*2;

	*textRect=*areaRect;

    textRect->top=textRect->top+(StatbarPntData->dyBorder*4);
    textRect->bottom=textRect->bottom-StatbarPntData->dyBorderx3;
    textRect->left=textRect->left+StatbarPntData->dyBorderx3;
    textRect->right=textRect->left+width;
	return(areaRect->right+StatbarPntData->dyBorderx3*2);
}

void InvalidateWindowStatusBar(HWND hwnd)
/*	Invalidate the local status bar area
 */
{
	RECT
		rc;

    GetClientRect(hwnd,&rc);
    rc.bottom=rc.top+windowStatbarPntData.dyStatbar;
	InvalidateRect(hwnd,&rc,TRUE);
}

void FillWindowStatusBar(HWND statusHwnd,HDC hdc)
/*	Clear the status bar
 */
{
	RECT
		rc,
		rcTemp;

    GetClientRect(statusHwnd,&rc);

	rc.bottom=rc.top+windowStatbarPntData.dyStatbar;
	/* Border color */

	/* Fill the center */
	rcTemp=rc;
	rcTemp.top+=windowStatbarPntData.dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DFACE));


	/* Highlight color */

	/* Across the top */
	rcTemp=rc;
	rcTemp.top+=windowStatbarPntData.dyBorder;

	rcTemp.bottom=rcTemp.top+windowStatbarPntData.dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DHILIGHT));


	/* solid black line across top and bottom*/

	rcTemp=rc;
	rcTemp.bottom=rcTemp.top+windowStatbarPntData.dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DDKSHADOW));

	rcTemp=rc;
	rcTemp.top=rcTemp.bottom-windowStatbarPntData.dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DDKSHADOW));
}

void DrawWindowStatusBar(HWND statusHwnd,HDC hdc)
/*	Draw the editor status bar info
 */
{
	RECT
		statusRect,
		areaRect,
		textRect;
	char
		statusStr[512];
	EDITORWINDOW
		*theWindow;
	UINT32
		xOffset;
	EDITORBUFFER
		*theUniverse;
	UINT32
		width,
		startPosition,						/* selection information */
		endPosition,
		startLine,
		endLine,
		startLinePosition,
		endLinePosition,
		totalSegments,
		totalSpan;
	INT32
		widthLeft;

	theWindow=(EDITORWINDOW *)GetWindowLong(statusHwnd,EDITORWINDOW_DATA_OFFSET);
	if(theWindow->theBuffer)
	{
		theUniverse=theWindow->theBuffer;
	    SetTextColor(hdc,GetSysColor(COLOR_BTNTEXT));
	    SetBkColor(hdc,GetSysColor(COLOR_3DFACE));
	    windowStatbarPntData.hFontStatbar=(HFONT)SelectObject(hdc,(HGDIOBJ)windowStatbarPntData.hFontStatbar);
		SetMapMode(hdc,MM_TEXT);
	    GetClientRect(statusHwnd,&statusRect);

	    statusRect.bottom=statusRect.top+windowStatbarPntData.dyStatbar;
		xOffset=windowStatbarPntData.dyBorderx3*2;

	/* draw task info */
		width=GetStringWidthInPixels(hdc,RUNNING_TASK_STR);
		xOffset=CalcDrawRects(&statusRect,&areaRect,&textRect,xOffset,width,&windowStatbarPntData);
		DrawStatusBarTextRect(statusHwnd,hdc,&areaRect,&windowStatbarPntData);
		if(theWindow->theBuffer->theTask)						/* see if there is a task active in this window */
		{
			ExtTextOut(hdc,textRect.left,textRect.top,ETO_OPAQUE|ETO_CLIPPED,&textRect,RUNNING_TASK_STR,sizeof(RUNNING_TASK_STR)-1,NULL);
		}
		else
		{
			ExtTextOut(hdc,textRect.left,textRect.top,ETO_OPAQUE|ETO_CLIPPED,&textRect,IDLE_TASK_STR,sizeof(IDLE_TASK_STR)-1,NULL);
		}


	/* draw dirty info */
		width=GetStringWidthInPixels(hdc,"*");
		xOffset=CalcDrawRects(&statusRect,&areaRect,&textRect,xOffset,width,&windowStatbarPntData);
		DrawStatusBarTextRect(statusHwnd,hdc,&areaRect,&windowStatbarPntData);
		
		if(!AtUndoCleanPoint(theUniverse))
		{
			ExtTextOut(hdc,textRect.left,textRect.top,ETO_OPAQUE | ETO_CLIPPED, &textRect,"*",1,NULL);
		}
		else
		{
			ExtTextOut(hdc,textRect.left,textRect.top,ETO_OPAQUE | ETO_CLIPPED, &textRect," ",1,NULL);
		}


	/* draw line info */
		EditorGetSelectionInfo(theUniverse,theUniverse->selectionUniverse,&startPosition,&endPosition,&startLine,&endLine,&startLinePosition,&endLinePosition,&totalSegments,&totalSpan);
		if(totalSegments)
		{
			sprintf(statusStr,"<Selections> Number:%d TotalChars:%d Start(%d:%d %d) End(%d:%d %d)",
					totalSegments,
					totalSpan,
					startLine+1,startLinePosition,startPosition,
					endLine+1,endLinePosition,endPosition
					);

		}
		else
		{
			sprintf(statusStr,"Lines:%d Chars:%d Cursor(%d:%d %d)",
					theUniverse->textUniverse->totalLines,
					theUniverse->textUniverse->totalBytes,
					startLine+1,startLinePosition,startPosition
					);
		}
		widthLeft=(statusRect.right-xOffset)-windowStatbarPntData.dyBorderx3*4;
		if(widthLeft>0)
		{
			xOffset=CalcDrawRects(&statusRect,&areaRect,&textRect,xOffset,widthLeft,&windowStatbarPntData);
			DrawStatusBarTextRect(statusHwnd,hdc,&areaRect,&windowStatbarPntData);
			ExtTextOut(hdc,textRect.left,textRect.top,ETO_OPAQUE | ETO_CLIPPED, &textRect,statusStr,strlen(statusStr),NULL);
		}
	    windowStatbarPntData.hFontStatbar=(HFONT)SelectObject(hdc,(HGDIOBJ)windowStatbarPntData.hFontStatbar);
	}
}


static void DrawTrackingButton(UINT32 xOffset,UINT32 field,bool hilited)
/*	Passed a buttom field, offset over in the status bar and whether it's hilited or not, draw it.
 */
{
	HDC
		hdc;
	RECT
		statusRect,
		areaRect,
		textRect;

	hdc=GetDC(frameWindowHwnd);		/* then we can draw it directly */
    SetTextColor(hdc,GetSysColor(COLOR_BTNTEXT));
    SetBkColor(hdc,GetSysColor(COLOR_3DFACE));
    SelectObject(hdc,(HGDIOBJ)globalStatbarPntData.hFontStatbar);
    GetClientRect(frameWindowHwnd,&statusRect);

    statusRect.top=statusRect.bottom-globalStatbarPntData.dyStatbar;
    if(hilited)
    {
		xOffset=CalcDrawRects(&statusRect,&areaRect,&textRect,xOffset,theStatusField[field].width,&globalStatbarPntData);
		DrawStatusBarTextRect(frameWindowHwnd,hdc,&areaRect,&globalStatbarPntData);
		ExtTextOut(hdc,textRect.left,textRect.top,ETO_OPAQUE | ETO_CLIPPED, &textRect,theStatusField[field].string,strlen(theStatusField[field].string),NULL);
    }
    else
    {
		xOffset=CalcDrawRects(&statusRect,&areaRect,&textRect,xOffset,theStatusField[field].width,&globalStatbarPntData);
		DrawStatusBarButtonRect(frameWindowHwnd,hdc,&areaRect,&globalStatbarPntData);
		ExtTextOut(hdc,textRect.left,textRect.top,ETO_OPAQUE | ETO_CLIPPED, &textRect,theStatusField[field].string,strlen(theStatusField[field].string),NULL);
    }
	ReleaseDC(frameWindowHwnd,hdc);
}

bool TrackStatusButton(INT32 x,INT32 y,char **cmdString)
/*	Passed an x,y position, return TRUE if a status bar button was clicked on, else FALSE.
	If a button was clicked on cmdString points to the string for that button
 */
{
	RECT
		statusRect,
		areaRect,
		textRect;
	UINT32
		field,
		fXOffset,
		xOffset,
		i;
	POINT
		pt;
	bool
		noMouseUp,
		hilited,
		found;
	MSG
		msg;

	pt.x=x;
	pt.y=y;
	found=false;
    GetClientRect(frameWindowHwnd,&statusRect);
    statusRect.top=statusRect.bottom-globalStatbarPntData.dyStatbar;
    xOffset=globalStatbarPntData.dyBorderx3*2;
	for(i=0;!found&&(i<MAXSTATUSFIELDS);i++)
	{
		switch(theStatusField[i].type)
		{
			case STATUSFIELDTYPE_TEXT:
			{
				if(theStatusField[i].width)
				{
					fXOffset=xOffset;
					xOffset=CalcDrawRects(&statusRect,&areaRect,&textRect,xOffset,theStatusField[i].width,&globalStatbarPntData);
				}
				break;
			}
			case STATUSFIELDTYPE_BUTTON:
			{
				if(theStatusField[i].width)
				{
					fXOffset=xOffset;
					xOffset=CalcDrawRects(&statusRect,&areaRect,&textRect,xOffset,theStatusField[i].width,&globalStatbarPntData);
					if(PtInRect(&areaRect,pt))
					{
						found=true;
						field=i;
						(*cmdString)=theStatusField[i].cmdString;
					}

				}
				break;
			}
		}
	}
	if(hilited=found)
	{
		DrawTrackingButton(fXOffset,field,true);
		SetCapture(frameWindowHwnd);								/* capture mouse clicks */
		noMouseUp=true;
		while(noMouseUp)
		{
			if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
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
					case WM_RBUTTONDOWN:
					case WM_MBUTTONDOWN:
					case WM_LBUTTONDOWN:
					{
						noMouseUp=false;
						break;
					}
					case WM_MOUSEMOVE:
					{
						pt.x=(INT32)LOSIGNEDWORD(msg.lParam);
						pt.y=(INT32)HISIGNEDWORD(msg.lParam);
						if(PtInRect(&areaRect,pt))
						{
							if(!hilited)
							{
								DrawTrackingButton(fXOffset,field,true);
							}
							hilited=true;
						}
						else
						{
							if(hilited)
							{
								DrawTrackingButton(fXOffset,field,FALSE);
							}
							hilited=false;
						}

						break;
					}
					case WM_LBUTTONUP:
					case WM_MBUTTONUP:
					case WM_RBUTTONUP:
					{
						noMouseUp=false;
						DrawTrackingButton(fXOffset,field,FALSE);
						break;
					}
					default:
					{
		    		    DispatchMessage(&msg);				/* send the message */
						break;
					}
				}
			}
			else
			{
				WaitMessage();	/* go to sleep if there was no message until one comes in */
			}
			if(GetActiveWindow()!=frameWindowHwnd)
			{
				noMouseUp=false;
				DrawTrackingButton(fXOffset,field,FALSE);
				hilited=false;
			}
		}
		ReleaseCapture();					/* release the mouse */
   	}
	return(hilited);
}

void InvalidateGlobalStatusBar()
/*	Invalidate the global status bar area
 */
{
	RECT
		rc;

    GetClientRect(frameWindowHwnd,&rc);
    rc.top=rc.bottom-globalStatbarPntData.dyStatbar;
	InvalidateRect(frameWindowHwnd,&rc,TRUE);
}

void FillGlobalStatusBar(HWND statusHwnd,HDC hdc)
/*	Clear the status bar
 */
{
	RECT
		rc,
		rcTemp;

    GetClientRect(statusHwnd,&rc);

	rc.top=rc.bottom-globalStatbarPntData.dyStatbar;
	/* draw the frame */

	/* Border color */

	rcTemp=rc;
	rcTemp.top=rcTemp.bottom-globalStatbarPntData.dyBorderx2;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DFACE));
	/* Fill the center */
	rcTemp=rc;
	rcTemp.top+=globalStatbarPntData.dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DFACE));


	/* Highlight color */

	/* Across the top */
	rcTemp=rc;
	rcTemp.top+=globalStatbarPntData.dyBorder;

	rcTemp.bottom=rcTemp.top+globalStatbarPntData.dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DHILIGHT));


	/* solid black line across top*/

	rcTemp=rc;
	rcTemp.bottom=rcTemp.top+globalStatbarPntData.dyBorder;
	FillRect(hdc,&rcTemp,(HBRUSH)GetSysColorBrush(COLOR_3DDKSHADOW));
}

void DrawGlobalStatusFields(HWND statusHwnd,HDC hdc)
/*	Run through the list of global statuses, drawing each one that has a width greater than zero
 */
{
	RECT
		statusRect,
		areaRect,
		textRect;
	UINT32
		xOffset,
		i;

    SetTextColor(hdc,GetSysColor(COLOR_BTNTEXT));
    SetBkColor(hdc,GetSysColor(COLOR_3DFACE));
    SelectObject(hdc,(HGDIOBJ)globalStatbarPntData.hFontStatbar);
    GetClientRect(statusHwnd,&statusRect);

    statusRect.top=statusRect.bottom-globalStatbarPntData.dyStatbar;
    xOffset=globalStatbarPntData.dyBorderx3*2;
	for(i=0;i<MAXSTATUSFIELDS;i++)
	{
		switch(theStatusField[i].type)
		{
			case STATUSFIELDTYPE_TEXT:
			{
				if(theStatusField[i].width)
				{
					xOffset=CalcDrawRects(&statusRect,&areaRect,&textRect,xOffset,theStatusField[i].width,&globalStatbarPntData);
					DrawStatusBarTextRect(statusHwnd,hdc,&areaRect,&globalStatbarPntData);
					ExtTextOut(hdc,textRect.left,textRect.top,ETO_OPAQUE | ETO_CLIPPED, &textRect,theStatusField[i].string,strlen(theStatusField[i].string),NULL);
				}
				break;
			}
			case STATUSFIELDTYPE_BUTTON:
			{
				if(theStatusField[i].width)
				{
					xOffset=CalcDrawRects(&statusRect,&areaRect,&textRect,xOffset,theStatusField[i].width,&globalStatbarPntData);
					DrawStatusBarButtonRect(statusHwnd,hdc,&areaRect,&globalStatbarPntData);
					ExtTextOut(hdc,textRect.left,textRect.top,ETO_OPAQUE | ETO_CLIPPED, &textRect,theStatusField[i].string,strlen(theStatusField[i].string),NULL);
				}
				break;
			}
		}
	}
}

UINT32 GetGlobalStatusBarStringWidth(char *string)
/*	Passed a string, return the width of the string in pixels for the current global
	status bar font.
 */
{
	UINT32
		width;
	HDC
		hdc;

	hdc=GetDC(NULL);
	globalStatbarPntData.hFontStatbar=(HFONT)SelectObject(hdc,(HGDIOBJ)globalStatbarPntData.hFontStatbar);
	width=GetStringWidthInPixels(hdc,string);
	globalStatbarPntData.hFontStatbar=(HFONT)SelectObject(hdc,(HGDIOBJ)globalStatbarPntData.hFontStatbar);
	ReleaseDC(NULL,hdc);
	return(width);
}

bool SetGlobalStatusField(UINT32 field,UINT32 width,char *theString)
/*	Set a global status field
 */
{
	bool
		result;
	char
		*strPtr;
	HDC
		hdc;

	result=false;
	if(field<MAXSTATUSFIELDS)			/* make sure it is in range */
	{
		theStatusField[field].type=STATUSFIELDTYPE_TEXT;
		if(width!=0)					/* see if the field if going away */
		{
			if(strPtr=(char *)MNewPtr(strlen(theString)+1))
			{
				strcpy(strPtr,theString);
				if(theStatusField[field].string)
				{
					MDisposePtr(theStatusField[field].string);
				}
				theStatusField[field].string=strPtr;
				if(theStatusField[field].width==width)	/* if you are changing the width */
				{
					hdc=GetDC(frameWindowHwnd);		/* then we can draw it directly */
					DrawGlobalStatusFields(frameWindowHwnd,hdc);
					ReleaseDC(frameWindowHwnd,hdc);
				}
				else
				{
					theStatusField[field].width=width;
					InvalidateGlobalStatusBar();		/* else, we need to update the whole thing */
				}
				result=true;
			}
		}
		else
		{
			theStatusField[field].width=0;	/* set width to zero */
			if(theStatusField[field].string)	/* did we have memory for a string? */
			{
				MDisposePtr(theStatusField[field].string);	/* yes, get rid of it */
				theStatusField[field].string=NULL;
			}
			InvalidateGlobalStatusBar();	/* make sure the status bar gets redrawn */
			result=true;
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STATUSFIELDOUTOFRANGE],errorDescriptions[STATUSFIELDOUTOFRANGE]);
	}
	return(result);
}

bool SetGlobalStatusButton(UINT32 field,UINT32 width,char *theString,char *cmdString)
/*	Set a global status field
 */
{
	bool
		result;
	char
		*strPtr;
	HDC
		hdc;

	result=false;
	if(field<MAXSTATUSFIELDS)			/* make sure it is in range */
	{
		theStatusField[field].type=STATUSFIELDTYPE_BUTTON;
		if(width!=0)					/* see if the field if going away */
		{
			if(strPtr=(char *)MNewPtr(strlen(theString)+1))
			{
				strcpy(strPtr,theString);
				if(theStatusField[field].string)
				{
					MDisposePtr(theStatusField[field].string);
				}
				theStatusField[field].string=strPtr;
				if(strPtr=(char *)MNewPtr(strlen(cmdString)+1))
				{
					strcpy(strPtr,cmdString);
					if(theStatusField[field].cmdString)
					{
						MDisposePtr(theStatusField[field].cmdString);
					}
					theStatusField[field].cmdString=strPtr;
					if(theStatusField[field].width==width)	/* if you are not changing the width */
					{
						hdc=GetDC(frameWindowHwnd);		/* then we can draw it directly */
						DrawGlobalStatusFields(frameWindowHwnd,hdc);
						ReleaseDC(frameWindowHwnd,hdc);
					}
					else
					{
						theStatusField[field].width=width;
						InvalidateGlobalStatusBar();		/* else, we need to update the whole thing */
					}
					result=true;
				}
			}
		}
		else
		{
			theStatusField[field].width=0;	/* set width to zero */
			if(theStatusField[field].string)	/* did we have memory for a string? */
			{
				MDisposePtr(theStatusField[field].string);	/* yes, get rid of it */
				theStatusField[field].string=NULL;
			}
			InvalidateGlobalStatusBar();	/* make sure the status bar gets redrawn */
			result=true;
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[STATUSFIELDOUTOFRANGE],errorDescriptions[STATUSFIELDOUTOFRANGE]);
	}
	return(result);
}

static bool SetStatusInfoFont(STATUSPAINT *statusInfo,char *fontString,int *fontHeight)
/*	Passed a pointer to a status bar record, set a new font for it, and return the height of that font
 */
{
	HDC
		hDC;
	TEXTMETRIC
		tm;
	LOGFONT
		lf;
	HFONT
		tmpFont;
	bool
		result;

	result=false;
	GetFontInfo(fontString,&lf);
	if(tmpFont=CreateFontIndirect(&lf))
	{
		if(statusInfo->hFontStatbar)
		{
			DeleteObject(statusInfo->hFontStatbar);
		}
		statusInfo->hFontStatbar=tmpFont;
	    hDC=GetDC(NULL);
	    tmpFont=(HFONT)SelectObject(hDC,(HGDIOBJ)statusInfo->hFontStatbar);
	    GetTextMetrics(hDC,&tm);
	    SelectObject(hDC,(HGDIOBJ)tmpFont);
	    ReleaseDC(NULL,hDC);
	    (*fontHeight)=tm.tmHeight+tm.tmExternalLeading;
	    result=true;
	}
	else
	{
		SetWindowsError();
	}
	return(result);
}

bool SetWindowStatusBarFont(char *fontString)
/*	Set the font used for a windows status bar.
 */
{
	WINDOWLISTELEMENT
		*theElement;
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	EDITORWINDOW
		*theWindow;

	if(SetStatusInfoFont(&windowStatbarPntData,fontString,&windowStatbarPntData.dyStatbar))
	{
	    windowStatbarPntData.dyStatbar+=(10*windowStatbarPntData.dyBorder);
		windowStatbarPntData.dyInner=windowStatbarPntData.dyStatbar-2;
		theElement=windowListHead;

		while(theElement)		/* invalidate each window */
		{
			InvalidateRect(theElement->hwnd,NULL,TRUE);
			theWindow=(EDITORWINDOW *)GetWindowLong(theElement->hwnd,EDITORWINDOW_DATA_OFFSET);
			theView=(EDITORVIEW *)theWindow->windowInfo;
			theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */
			GetClientRect(theElement->hwnd,&theViewInfo->bounds);
			theViewInfo->bounds.top+=windowStatbarPntData.dyStatbar;
			RecalculateViewFontInfo(theView);
			AdjustDocumentWindowScrollThumbs(theView);

			theElement=theElement->nextElement;
		}
		return(true);
	}
	return(false);
}

bool SetGlobalStatusBarFont(char *fontString)
/*	Set the font for the global status bar
 */
{
	RECT
		theRect;

	if(SetStatusInfoFont(&globalStatbarPntData,fontString,&globalStatbarPntData.dyStatbar))
	{
	    globalStatbarPntData.dyStatbar+=(8*globalStatbarPntData.dyBorder);
		globalStatbarPntData.dyInner=globalStatbarPntData.dyStatbar;
		GetClientRect(frameWindowHwnd,&theRect);
		SendMessage(frameWindowHwnd,WM_SIZE,SIZE_RESTORED,(theRect.right-theRect.left)+((theRect.bottom-theRect.top)<<16));
		InvalidateRect(frameWindowHwnd,NULL,TRUE);
		return(true);
	}
	return(false);
}

void UninitStatusBar()
/*	Uninitialize status bar stuff
 */
{
	UINT32
		i;

	for(i=0;i<MAXSTATUSFIELDS;i++)
	{
		theStatusField[i].width=0;
		if(theStatusField[i].string)
		{
			MDisposePtr(theStatusField[i].string);
			theStatusField[i].string=NULL;
		}
		if(theStatusField[i].cmdString)
		{
			MDisposePtr(theStatusField[i].cmdString);
			theStatusField[i].cmdString=NULL;
		}
	}
	if(globalStatbarPntData.hFontStatbar)
	{
		DeleteObject(globalStatbarPntData.hFontStatbar);
	}
	if(globalStatbarPntData.hFontStatbar)
	{
		DeleteObject(windowStatbarPntData.hFontStatbar);
	}
}

bool InitStatusBar()
/*	Setup stuff needed for the status bar, ie. a font, and vertical size of the status bar
 */
{
	UINT32
		i;

	for(i=0;i<MAXSTATUSFIELDS;i++)
	{
		theStatusField[i].width=0;
		theStatusField[i].string=NULL;
		theStatusField[i].cmdString=NULL;
	}
    globalStatbarPntData.dyBorder=GetSystemMetrics(SM_CYBORDER);
    globalStatbarPntData.dyBorderx2=globalStatbarPntData.dyBorder*2;
    globalStatbarPntData.dyBorderx3=globalStatbarPntData.dyBorder*3;
	globalStatbarPntData.hFontStatbar=0;

    windowStatbarPntData.dyBorder=GetSystemMetrics(SM_CYBORDER);
    windowStatbarPntData.dyBorderx2=windowStatbarPntData.dyBorder*2;
    windowStatbarPntData.dyBorderx3=windowStatbarPntData.dyBorder*3;
	windowStatbarPntData.hFontStatbar=0;

	if(SetStatusInfoFont(&windowStatbarPntData,DEFAULT_FONT,&windowStatbarPntData.dyStatbar))
	{
	    windowStatbarPntData.dyStatbar+=(10*windowStatbarPntData.dyBorder);
		windowStatbarPntData.dyInner=windowStatbarPntData.dyStatbar-2;
		if(SetStatusInfoFont(&globalStatbarPntData,DEFAULT_FONT,&globalStatbarPntData.dyStatbar))
		{
		    globalStatbarPntData.dyStatbar+=(8*globalStatbarPntData.dyBorder);
			globalStatbarPntData.dyInner=globalStatbarPntData.dyStatbar;
			return(true);
		}
	}
	return(false);
}
