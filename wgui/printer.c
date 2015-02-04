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

static volatile BOOL wasAborted;
static HWND DlgWnd;
static char *printTitle;
static UINT32 currentPageNumber;

HFONT CreateFontForPrinterDC(HDC hdc,char *theFont)
/*	Passed in the DC for a printer and a pointer to one of our fonts, create a Windows font
	for the printer DC
 */
{
	LOGFONT
		lf;
	HFONT
		winFont;
	INT32
		ptSize;
	HDC
		sHdc;

	GetFontInfo(theFont,&lf);
	sHdc=GetDC(frameWindowHwnd);
	ptSize=(-(LONG)lf.lfHeight)*72L/(LONG)GetDeviceCaps(sHdc,LOGPIXELSY);
	ReleaseDC(frameWindowHwnd,sHdc);
	lf.lfHeight=-(LONG)(ptSize*(LONG)GetDeviceCaps(hdc,LOGPIXELSY)/72L);
	if(winFont=CreateFontIndirect(&lf))
	{
		return(winFont);
	}
	else
	{
		SetWindowsError();
	}
	return((HFONT)NULL);
}

BOOL CALLBACK Abort(HDC hdc,int w)
/*	Abort - procedure to abort a print job
 */
{
	MSG
		msg;

	while(!wasAborted&&PeekMessage(&msg,NULL,0,0,PM_REMOVE))
	{
		if(!IsDialogMessage(DlgWnd,&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return(!wasAborted);
}

BOOL CALLBACK AbortDialog(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
/*	AbortDialog - wait for the user to press cancel
 */
{
	int
		handled;
	char
		temp[32];

	handled=FALSE;
	switch(msg)
	{
		case WM_COMMAND:
		{
			wasAborted=TRUE;
			handled=TRUE;
			break;
		}
		case WM_INITDIALOG:
		{
			sprintf(temp,"Printing page:%d",currentPageNumber);
			SetDlgItemText(hwnd,CURRENT_PAGE,(LPSTR)temp);
			SetDlgItemText(hwnd,PRINT_TITLE,(LPSTR)printTitle);
			handled=TRUE;
			break;
		}
	}
	return(handled);
}

void PrintPageTitle(HDC pHDC,char *pageTitle,INT16 yPos,UINT16 pageWidth)
/*	Print the string pageTitle, centered at the yPos
 */
{
	SIZE
		textSize;

	GetTextExtentPoint32(pHDC,pageTitle,strlen(pageTitle),&textSize);
	TextOut(pHDC,(pageWidth-textSize.cx)/2,yPos,pageTitle,strlen(pageTitle));
}

void PrintPageNumber(HDC pHDC,UINT32 pageNum,INT16 yPos,UINT16 pageWidth)
/*	Print the pageNum, centered at the yPos
 */
{
	char
		pageNumStr[32];
		SIZE
		textSize;

	sprintf(pageNumStr,"- %d -",pageNum);
	GetTextExtentPoint32(pHDC,pageNumStr,strlen(pageNumStr),&textSize);
	TextOut(pHDC,(pageWidth-textSize.cx)/2,yPos,pageNumStr,strlen(pageNumStr));
}

#define	JOBNAMEMAXLEN	12

typedef	struct
{
	int
		size;
	void
		*data;
} MEMBUFFER;

// given a MEMBUFFER, remove the memory it holds
void EmptyBuffer(MEMBUFFER *buffer)
{
	if (buffer->data != NULL)
	{
		MDisposePtr(buffer->data);
		buffer->data = NULL;
		buffer->size = 0;
	}
}

// if the buffer is not large enough to hold size worth of bytes (or the data is NULL), 
// delete the data pointer and make a new one that is large enough
void ResizeBuffer(int size, MEMBUFFER *buffer)
{
	if (buffer->data != NULL && buffer->size < size)
	{
		EmptyBuffer(buffer);
	}

	if (buffer->data == NULL)
	{
		buffer->data = (char *)MNewPtr(size);
		buffer->size = size;
	}
}

// count the number of tabs that appear in string s
static int countTabs(const char *s)
{
	int
		count = 0;

	while(*s)
	{
		if (*s == '\t')
		{
			count++;
		}
		s++;
	}

	return count;
}

// given a string, replace the tab character with tabString
static char *replaceTabs(const char *s, const char *tabString, MEMBUFFER *buffer)
{
	int
		tabSize = strlen(tabString),
		numTabs = countTabs(s),
		count = 0;

	ResizeBuffer(strlen(s) + 1 + ((numTabs * tabSize) - numTabs), buffer);

	char
		*outputString = (char *)buffer->data;

	while(*s)
	{
		if (*s == '\t')
		{
			memcpy(&(outputString[count]), tabString, tabSize);
			count += tabSize;
		}
		else
		{
			outputString[count++] = *s;
		}
		s++;
	}
	outputString[count] = NULL;

	return outputString;
}

// build a NULL terminated C string full of whitespace (spaces) of the requested size
static char *makeWhiteSpace(int size, MEMBUFFER *buffer)
{
	ResizeBuffer(size + 1, buffer);

	int
		count = 0;
	char
		*outputString = (char *)buffer->data;

	while(count < size)
	{
		outputString[count++] = ' ';
	}

	outputString[count] = NULL;

	return outputString;
}


// Extract one line of data from the TEXTUNIVERSE starting at theOffset
static char *ExtractNextLine(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,MEMBUFFER *theBuffer,CHUNKHEADER **nextChunk,UINT32 *nextOffset)
{
	char
		*theLine = NULL;
	UINT32
		tmpOffset;
	CHUNKHEADER
		*tmpChunk;
	UINT32
		bytesToRead;

	// determine how many bytes do we need to extract to read one line from the textUniverse?
	ChunkPositionToNextLine(theUniverse, theChunk, theOffset, &tmpChunk, &tmpOffset, &bytesToRead);

	// make sure that our buffer is big enough to hold the line we want to read from the textUniverse
	ResizeBuffer(bytesToRead + 1, theBuffer);

	// point to the buffer's data, that's where we'll put our line
	theLine = (char *)theBuffer->data;

	// read the data into it
	if (ExtractUniverseText(theUniverse, theChunk, theOffset, (unsigned char *)theLine, bytesToRead, nextChunk, nextOffset))
	{
		char
			*newline;

		// terminate at the first CR or LF newline character
		theLine[bytesToRead] = NULL;
		if (newline = strpbrk(theLine, "\n\r"))
		{
			*newline = NULL;
		}
	}
	
	return theLine;
}

static bool PrintTextToPrinter(EDITORWINDOW *theWindow,HDC pHDC,UINT32 startLine,UINT32 numLines,HWND theDlgHwnd)
/*	Print the editorwindows text, starting at startLine, and do numLines
 */
{
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	UINT32
		titleYPos,
		pageNumYPos,
		pageWidth,
		pageheight,
		lineheight;
	UINT32
		startingHeight,
		height;
	bool
		error;
	CHUNKHEADER
		*theChunk;
	UINT32
		theOffset;
	UINT32
		curPageNum;
	DOCINFO
		dInfo;
	char
		tempStr[32],
		jobTitle[JOBNAMEMAXLEN];
	TEXTMETRIC
		fontInfo;
	int
		charWidths[256];	/* cache of widths of all the characters in the current font */

	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */

	SetTextAlign(pHDC,TA_LEFT|TA_TOP);
	SetMapMode(pHDC,MM_TEXT);
	SetBkMode(pHDC,TRANSPARENT);
	pageheight=GetDeviceCaps(pHDC,VERTRES);			/* get the height of a page */
	pageWidth=GetDeviceCaps(pHDC,HORZRES);			/* get the width of a page */

	GetTextMetrics(pHDC,&fontInfo);
	lineheight=fontInfo.tmHeight+fontInfo.tmInternalLeading+fontInfo.tmExternalLeading;		/* get the height of a line */
	lineheight += lineheight/4;
	
	titleYPos=TOPPRINTMARGIN;
	pageNumYPos=pageheight-lineheight;
	pageheight=pageheight-(BOTTOMPRINTMARGIN+TOPPRINTMARGIN+lineheight*3);					/* bring the bottom end up by this much */
	startingHeight=TOPPRINTMARGIN+lineheight+lineheight/2;

	if(GetWindowsVersion()==VER_PLATFORM_WIN32_NT)
	{
		GetCharWidth32(pHDC,0,255,(int *)&(charWidths[0]));
	}
	else
	{
		GetCharWidth(pHDC,0,255,(int *)&(charWidths[0]));
	}


	strncpy(jobTitle,theViewInfo->titleString,JOBNAMEMAXLEN);	/* job title can only be JOBNAMEMAXLEN characters max */
	jobTitle[JOBNAMEMAXLEN-1]='\0';

	wasAborted=FALSE;											/* the print job has not been aborted yet */

	if(SetAbortProc(pHDC,(ABORTPROC)Abort))
	{
		dInfo.cbSize=sizeof(dInfo);
		dInfo.lpszDocName=jobTitle;
		dInfo.lpszOutput=NULL;
		dInfo.lpszDatatype=NULL;
		dInfo.fwType=0;

		curPageNum=currentPageNumber=1;

		error=FALSE;
		if(StartDoc(pHDC,&dInfo)>0)
		{
			UINT32
				bytesToRead;

			height = startingHeight;																				// start printing this many lines down
			LineToChunkPosition(theView->parentBuffer->textUniverse,startLine,&theChunk,&theOffset,&bytesToRead);	// point to the data for first line we want to print
			if(StartPage(pHDC)>0)
			{
				static MEMBUFFER
					outputLineBuffer,
					chunkDataBuffer,
					tabBuffer;
				char
					*tabString = makeWhiteSpace(theViewInfo->tabSize, &tabBuffer);

				PrintPageTitle(pHDC,printTitle,(INT16)titleYPos,(INT16)pageWidth);
				
				while(!error&&!wasAborted&&numLines)
				{
					if((height+lineheight)>pageheight)					/* do we need to start a new page? */
					{
						currentPageNumber=curPageNum;
						sprintf(tempStr,"Printing page:%d",curPageNum);
						SetDlgItemText(theDlgHwnd,CURRENT_PAGE,(LPSTR)tempStr);

						PrintPageNumber(pHDC,curPageNum++,(INT16)pageNumYPos,(INT16)pageWidth);
						height=startingHeight;							/* yes, start printing this many lines down */
						if(!((EndPage(pHDC)>0)&&(StartPage(pHDC)>0)))	/* end the current page and start a new one */
						{
							error=TRUE;
						}
						else
						{
							PrintPageTitle(pHDC,printTitle,(INT16)titleYPos,(INT16)pageWidth);
						}
					}

					if(!error)
					{
						if(theChunk)
						{
							char
								*inputLine;
								
							if (inputLine = ExtractNextLine(theView->parentBuffer->textUniverse, theChunk, theOffset, &chunkDataBuffer, &theChunk, &theOffset))
							{
								// convert tabs to spaces
								char
									*outputLine = replaceTabs(inputLine, tabString, &outputLineBuffer);
								
								TextOut(pHDC, LEFTPRINTMARGIN, height, outputLine, strlen(outputLine));
							}
						}
						numLines--;
						height+=lineheight;
					}
				}
				currentPageNumber=curPageNum;
				sprintf(tempStr,"Printing page:%d",curPageNum);
				SetDlgItemText(theDlgHwnd,CURRENT_PAGE,(LPSTR)tempStr);
				PrintPageNumber(pHDC,curPageNum++,(INT16)pageNumYPos,(INT16)pageWidth);

				EmptyBuffer(&outputLineBuffer);
				EmptyBuffer(&chunkDataBuffer);
				EmptyBuffer(&tabBuffer);
				
				if(!error&&(EndPage(pHDC)<0))
				{
					error=TRUE;
				}
			}
			if(!error)
			{
				EndDoc(pHDC);
			}
			else
			{
				AbortDoc(pHDC);
			}
		}
		else
		{
			SetWindowsError();
			error=TRUE;
		}
	}
	else
	{
		SetWindowsError();
		error=TRUE;
	}
	return(!error);
}

bool PrintEditorWindow(EDITORWINDOW *theWindow)
/*	Passed in a window to print, bring up a print dialog box allowing the user to change printer options and
	start print. If there is a selection in the view allow the user to print the selection or the whole
	document. If the user prints the selection, we print everything from the line the selection starts on to
	the line the selection ends.
 */
{
	PRINTDLG
		pd;
	bool
		result;
	GUIVIEWINFO
		*theViewInfo;
	EDITORVIEW
		*theView;
	UINT32
		startLine,
		numLines;
	HFONT
		printFont;
	UINT32
		startPosition,						/* selection information */
		endPosition,
		endLine,
		startLinePosition,
		endLinePosition,
		totalSegments,
		totalSpan;

	result=FALSE;
	theView=(EDITORVIEW *)theWindow->windowInfo;
	theViewInfo=(GUIVIEWINFO *)theView->viewInfo;	/* point at the information record for this view */

	pd.lStructSize=sizeof(PRINTDLG);
	pd.hwndOwner=frameWindowHwnd;
	pd.hDevMode=(HANDLE)NULL;
	pd.hDevNames=(HANDLE)NULL;
	pd.Flags=PD_RETURNDC|PD_HIDEPRINTTOFILE|PD_NOPAGENUMS|PD_ALLPAGES;
	pd.hDC=NULL;
	pd.nFromPage=1;
	pd.nToPage=1;
	pd.nMinPage=0;
	pd.nMaxPage=0;
	pd.nCopies=1;
	pd.hInstance=NULL;
	pd.lCustData=0L;
	pd.lpfnPrintHook=NULL;
	pd.lpfnSetupHook=NULL;
	pd.lpPrintTemplateName=NULL;
	pd.lpSetupTemplateName=NULL;
	pd.hPrintTemplate=NULL;
	pd.hSetupTemplate=NULL;

	EditorGetSelectionInfo(theView->parentBuffer, theView->parentBuffer->selectionUniverse,&startPosition,&endPosition,&startLine,&endLine,&startLinePosition,&endLinePosition,&totalSegments,&totalSpan);

	if(totalSegments)
	{
		pd.Flags|=PD_SELECTION;		/* default to printing only the selection */
	}
	else
	{
		pd.Flags|=PD_NOSELECTION;	/* else, don't even give the user the option */
	}

	if(PrintDlg(&pd))
	{
		if(pd.Flags&PD_SELECTION)	/* if the user wants to print the selection */
		{
			numLines=endLine-startLine;
		}
		else
		{
			startLine=0;			/* else, print the whole thing */
			numLines=theView->parentBuffer->textUniverse->totalLines;
		}
		
		numLines = max(numLines, 1);

		if(printFont=CreateFontForPrinterDC(pd.hDC,theViewInfo->viewStyles[0].theFont->theName))
 		{
			printFont=(HFONT)SelectObject(pd.hDC,(HGDIOBJ)printFont);

			printTitle=theViewInfo->titleString;
			currentPageNumber=1;

			if(DlgWnd=CreateDialog(programInstanceHandle,"ABORTDIALOG",frameWindowHwnd,(DLGPROC)AbortDialog))
			{
				ShowWindow(DlgWnd,SW_SHOW);
				UpdateWindow(DlgWnd);

				result=PrintTextToPrinter(theWindow,pd.hDC,startLine,numLines,DlgWnd);
				DestroyWindow(DlgWnd);
			}
			else
			{
				SetWindowsError();
			}
			printFont=(HFONT)SelectObject(pd.hDC,(HGDIOBJ)printFont);
			DeleteObject(printFont);
		}
		DeleteDC(pd.hDC);
	}
	else
	{
		if(CommDlgExtendedError()==0)
		{
			result=TRUE;
		}
		else
		{
			SetCommDlgError();
		}
	}

	if(pd.hDevMode)
	{
		GlobalFree(pd.hDevMode);
	}
	if(pd.hDevNames)
	{
		GlobalFree(pd.hDevNames);
	}

	return(result);
}
