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

#define	TRANSLATEBUFFERSIZE		8192

struct editorFile
{
	HANDLE
		fileHandle;
	bool
		lastCharWasCR;
};

char *CreateWindowTitleFromPath(char *absolutePath)
/* use absolute path to generate a string (which is returned) that
 * should be placed in the title of a window
 * the string should be disposed of using MDisposePtr
 * if there is a problem, SetError, and return NULL
 */
{
	int
		pathLength;
	int
		nameLength;
	char
		*returnString;

	pathLength=strlen(absolutePath);
	if(returnString=(char *)MNewPtr(pathLength+6))
	{
		nameLength=0;
		while(pathLength&&(absolutePath[pathLength-1]!='\\' && absolutePath[pathLength-1]!='/'))	/* move backwards until the first part of the path is encountered */
		{
			nameLength++;
			pathLength--;
		}
		sprintf(returnString,"%.*s -- %.*s",nameLength,&(absolutePath[pathLength]),pathLength,&(absolutePath[0]));
		return(returnString);
	}
	return(NULL);
}

char *CreateAbsolutePath(char *relativePath)
/* given a relative path, attempt to make it an absolute path
 * NOTE: if the relativePath does not point to anything valid, or the absolute path buffer
 * could not be allocated, SetError, and return NULL
 * the returned buffer will be 0 terminated, and must be freed by calling
 * MDisposePtr
 */
{
	char
		resolvedPath[_MAX_PATH];
	char
		*returnPath;
	int
		i;

	if(relativePath[0]=='/' && relativePath[1]=='/')
	{
		strcpy(&(resolvedPath[0]),relativePath);				/* copy it into the buffer */
	}
	else
	{
		GetFullPathName(relativePath,_MAX_PATH,&(resolvedPath[0]),NULL);			/* get real path if possible */
	}
	if(returnPath=(char *)MNewPtr(strlen(&(resolvedPath[0]))+1))	/* create buffer to pass it back in */
	{
		strcpy(returnPath,&(resolvedPath[0]));				/* copy it into the buffer */
		i=0;
		while(returnPath[i])
		{
			if(returnPath[i]=='\\')
			{
				returnPath[i]='/';
			}
			// returnPath[i]=tolower(returnPath[i]);
			i++;
		}
		return(returnPath);
	}
	return(NULL);
}

static UINT32 CopyBufferRemovingCR(EDITORFILE *theFile,UINT8 *srcPtr,UINT32 length)
/*	Passed a source buffer and destination buffer, copy 'length' bytes, removing carriage returns
	that are followed by linefeeds as it copies, and return the number of bytes moved. This handles
	CRLF spanning buffer boundries.
 */
{
	UINT32
		srcIndex,
		dstIndex;

	srcIndex=0;
	dstIndex=0;
	if(theFile->lastCharWasCR)
	{
		if(length)
		{
			if(srcPtr[0]!='\n')
			{
				MMoveMem(&(srcPtr[0]),&(srcPtr[1]),length);	/* make room for the previous CR */
				srcPtr[0]='\r';
				length++;								/* add previous CR to buffer */
			}
		}
		else
		{
			srcPtr[0]='\r';
			length++;								/* add previous CR to buffer */
		}
		srcIndex++;
		dstIndex++;
	}
	theFile->lastCharWasCR=FALSE;					/* clear our flag */
	while(srcIndex<length)
	{
		if(srcPtr[srcIndex]=='\r')					/* if it is a CR */
		{
			if(srcIndex==(length-1))				/* and it is the last character in the buffer */
			{
				theFile->lastCharWasCR=TRUE;		/* set our flag for next time */
			}
			else
			{
				if(srcPtr[srcIndex+1]=='\n')		/* see if the next character is a LF */
				{
					srcIndex++;						/* skip past the CR */
				}
				srcPtr[dstIndex++]=srcPtr[srcIndex];	/* copy the LF */
			}
		}
		else
		{
			srcPtr[dstIndex++]=srcPtr[srcIndex];
		}
		srcIndex++;
	}
	return(dstIndex);
}

static void CopyBufferAddingCRtoLF(UINT8 *srcPtr,UINT8 *dstPtr,UINT32 length,UINT32 *numFromSrc,UINT32 *numToDst)
/*	Copy 'length' bytes from the srcPtr to the dstPtr, adding carriage returns before all linefeeds and return
	the number of bytes copied. THE dstPtr BUFFER MUST BE 'length*2+1' BYTES BIG.
 */
{
	UINT32
		inIndex,
		outIndex;

	for(inIndex=0,outIndex=0;outIndex<length;inIndex++)
	{
		if(srcPtr[inIndex]=='\n')
		{
			dstPtr[outIndex++]='\r';
		}
		dstPtr[outIndex++]=srcPtr[inIndex];
	}
	(*numFromSrc)=inIndex;
	(*numToDst)=outIndex;
}

EDITORFILE *OpenEditorReadFile(char *thePath)
/* open a document file for reading into the editor
 * if there is an error, SetError will be called, and NULL returned
 */
{
	EDITORFILE
		*theEditorFile;

	if(theEditorFile=(EDITORFILE *)MNewPtr(sizeof(EDITORFILE)))
	{
		if((theEditorFile->fileHandle=CreateFile(thePath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_SYSTEM|FILE_FLAG_SEQUENTIAL_SCAN,0))!=INVALID_HANDLE_VALUE)
		{
			theEditorFile->lastCharWasCR=FALSE;
			return(theEditorFile);
		}
		else
		{
			SetWindowsError();						/* set high level error information */
		}
		MDisposePtr(theEditorFile);
	}
	return(NULL);
}

EDITORFILE *OpenEditorWriteFile(char *thePath)
/* open a document file for writing into by the editor
 * if there is an error, SetError will be called, and NULL returned
 */
{
	EDITORFILE
		*theEditorFile;

	if(theEditorFile=(EDITORFILE *)MNewPtr(sizeof(EDITORFILE)))
	{
		if((theEditorFile->fileHandle=CreateFile(thePath,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL))!=INVALID_HANDLE_VALUE)
		{
			theEditorFile->lastCharWasCR=FALSE;
			return(theEditorFile);
		}
		else
		{
			SetWindowsError();						/* set high level error information */
		}
		MDisposePtr(theEditorFile);
	}
	return(NULL);
}

bool ReadEditorFile(EDITORFILE *theFile,UINT8 *theBuffer,UINT32 bytesToRead,UINT32 *bytesRead)
/* read at most bytesToRead from theFile into theBuffer
 * return the actual amount read in bytesRead
 * if bytesRead is less than bytesToRead, then EOF is assumed
 * if there is an error, SetError will be called, and FALSE returned
 */
{
	unsigned long
		numJustRead;
	bool
		fail;

	fail=false;
	if(translationModeOn)
	{
		(*bytesRead)=0;							/* no bytes read so far */
		if(theFile->lastCharWasCR)
		{
			if(ReadFile(theFile->fileHandle,(void *)theBuffer,bytesToRead-1,&numJustRead,NULL))
			{
				(*bytesRead)=CopyBufferRemovingCR(theFile,theBuffer,numJustRead);
			}
			else
			{
				SetWindowsError();			/* set high level error information */
				fail=true;
			}
		}
		else
		{
			if(ReadFile(theFile->fileHandle,(void *)theBuffer,bytesToRead,&numJustRead,NULL))
			{
				(*bytesRead)=CopyBufferRemovingCR(theFile,theBuffer,numJustRead);
			}
			else
			{
				SetWindowsError();			/* set high level error information */
				fail=true;
			}
		}
	}
	else
	{
		if(!(fail=!ReadFile(theFile->fileHandle,(void *)theBuffer,bytesToRead,&numJustRead,NULL)))
		{
			SetWindowsError();			/* set high level error information */
		}
		(*bytesRead)=numJustRead;
	}
	return(!fail);
}

bool WriteEditorFile(EDITORFILE *theFile,UINT8 *theBuffer,UINT32 bytesToWrite,UINT32 *bytesWritten)
/* write bytesToWrite from theBuffer into theFile
 * return the actual amount written in bytesWritten
 * bytesWritten should be the same as bytesToWrite, unless there was an error
 * if there is an error, SetError will be called, and FALSE returned
 */
{
	UINT32
		numFromSrc,
		numJustTranslated,
		numToWrite;
	unsigned long
		numJustWritten;
	bool
		fail;
	UINT8
		tBuffer[TRANSLATEBUFFERSIZE*2+1];

	if(translationModeOn)
	{
		(*bytesWritten)=0;							/* no bytes written so far */
		numFromSrc=0;
		fail=false;
		while(!fail&&bytesToWrite)
		{
			numToWrite=Min(bytesToWrite,TRANSLATEBUFFERSIZE);			/* dont pass an int that is too big */
			CopyBufferAddingCRtoLF(&(theBuffer[numFromSrc]),tBuffer,numToWrite,&numJustTranslated,&numToWrite);
			numFromSrc+=numJustTranslated;
			if(!WriteFile(theFile->fileHandle,(void *)tBuffer,numToWrite,&numJustWritten,NULL))
			{
				SetWindowsError();					/* set high level error information */
				fail=true;							/* no matter what, we have failed now */
			}
			(*bytesWritten)+=(UINT32)numJustWritten;	/* add in amount just written */
			bytesToWrite-=(UINT32)numJustTranslated;			/* this many less bytes to go */
		}
	}
	else
	{
		if(!(fail=!WriteFile(theFile->fileHandle,(void *)theBuffer,bytesToWrite,&numJustWritten,NULL)))
		{
			SetWindowsError();			/* set high level error information */
		}
		(*bytesWritten)=numJustWritten;
	}
	return(!fail);
}

void CloseEditorFile(EDITORFILE *theFile)
/* when the editor is finished with a file, it will call this
 */
{
	CloseHandle(theFile->fileHandle);					/* get rid of the file */
	MDisposePtr(theFile);
}
