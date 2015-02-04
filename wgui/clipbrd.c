// Routines to move data to and from the editors clipboard and Windows clipboard
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

bool ImportWindowsClipboard()
/*	Call this to get data from the Windows clipboard into the editors clipboard
 */
{
	UINT8
		*clipMemPtr;
	EDITORBUFFER
		*clipUniverse;
	bool
		result;
	HGLOBAL
		clipMemHandle;
	UINT8
		*localMemPtr;
	UINT32
		strLength;

	result=false;
	if(IsClipboardFormatAvailable(CF_TEXT))
	{
		if(clipUniverse=EditorStartImportClipboard())
		{
			if(OpenClipboard(frameWindowHwnd))		/* open the clipboard */
			{
				if(clipMemHandle=GetClipboardData(CF_TEXT))
				{
					clipMemPtr=(UINT8 *)GlobalLock(clipMemHandle);		/* this pointer is a 32 bit FAR pointer and as such is useless */
					strLength=strlen((char *)clipMemPtr);
					if(localMemPtr=(UINT8 *)MNewPtr(strLength+1))
					{
						strcpy((char *)localMemPtr,(char *)clipMemPtr);
						strLength=RemoveCRFromString((char *)localMemPtr);
						EditorStartReplace(clipUniverse);
						result=ReplaceEditorText(clipUniverse,0,clipUniverse->textUniverse->totalBytes,localMemPtr,strLength);
						EditorEndReplace(clipUniverse);
						MDisposePtr(localMemPtr);
					}
					GlobalUnlock(clipMemHandle);
				}
				CloseClipboard();
			}
			EditorEndImportClipboard();
		}
	}
	else
	{
		result=true;
	}
	return(result);
}

bool ExportClipboard()
/*	This is called from the editor any time something is moved to its
	clipboard. We get a copy of it and put it into Windows clipboard.
	Since the editor only deals with LF, we convert the LF to CRLF before
	passing it to windows.
 */
{
	CHUNKHEADER
		*theChunk;
	UINT32
		bytesToExtract,
		theOffset;
	HGLOBAL
		clipMemHandle;
	UINT8
		*theBuffer;
	bool
		result;
	EDITORBUFFER
		*theUniverse;
	UINT8
		*clipMemPtr;

	result=false;


	if(OpenClipboard(frameWindowHwnd))		/* open the clipboard */
	{
		if(theUniverse=EditorStartExportClipboard())
		{
			EmptyClipboard();				/* get rid of its current contents */

			if(theBuffer=(UINT8 *)MNewPtr(theUniverse->textUniverse->totalBytes+1))	/* create a buffer to give to X (inefficient, but X forces us to do it like this) */
			{
				theChunk=theUniverse->textUniverse->firstChunkHeader;
				theOffset=0;
				bytesToExtract=theUniverse->textUniverse->totalBytes;
				if(ExtractUniverseText(theUniverse->textUniverse,theChunk,theOffset,theBuffer,bytesToExtract,&theChunk,&theOffset))	/* read data from universe into buffer */
				{
					theBuffer[bytesToExtract]='\0';
					bytesToExtract=CountNumberOfLineFeeds((char *)theBuffer)+bytesToExtract+1;
					if(clipMemHandle=GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,bytesToExtract))		/* get global memory to pass the converted clipboard data to Windows in */
					{
						clipMemPtr=(UINT8 *)GlobalLock(clipMemHandle);		/* this pointer is a 32 bit FAR pointer and as such is useless */
						CopyStringAndAddCRToLF((char *)theBuffer,(char *)clipMemPtr,bytesToExtract);
						GlobalUnlock(clipMemHandle);				/* unlock the global memory */
						SetClipboardData(CF_TEXT,clipMemHandle);	/* set Windows clipboard data */
						importWindowsClipboardData=false;			/* should not import from windows, if we switch out and back in */
						result=true;
					}
				}
				MDisposePtr(theBuffer);
			}
			EditorEndExportClipboard();
		}
		CloseClipboard();
	}
	return(result);
}

bool ImportClipboard()
/* When the editor is about to do a paste, or something else
 * that requires it to look at the contents of the clipboard
 * it will call this to possibly import outside contents into
 * the current clipboard
 * NOTE: this should return TRUE if it succeeds, FALSE if it fails
 * NOTE: if no importing is done, this should still return TRUE
 */
{
	/* this is not handled at paste time, it is handled when the clipboard changes */
	return(true);
}
