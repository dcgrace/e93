// Misc low level stuff
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

// routines callable by the editor

char *GetEditorLocalVersion()
// return a pointer to a constant string, that tells something about this local
// version of the editor (used by the "version" command)
// it is ok to return NULL
{
	return("X Windows");
}

bool PointInRECT(INT32 x,INT32 y,EDITORRECT *theRect)
// return true if x, and y fall within theRect
{
	if((x>=theRect->x)&&(y>=theRect->y))
	{
		if((x<(theRect->x+(int)theRect->w))&&(y<(theRect->y+(int)theRect->h)))
		{
			return(true);
		}
	}
	return(false);
}

static void GlobalOffset(Window theXWindow,INT32 *outX,INT32 *outY)
// return the global offset of theXWindow
// NOTE: although this should always work, it is nasty
// X should provide a better way to do this
{
	XWindowAttributes
		theAttributes;
	Window
		current,
		root,
		parent,
		*children;
	unsigned int
		numChildren;

	(*outX)=0;
	(*outY)=0;
	current=theXWindow;
	root=RootWindow(xDisplay,xScreenNum);									// get the root
	while(current!=root)
	{
		XGetWindowAttributes(xDisplay,current,&theAttributes);
		(*outX)+=theAttributes.x;											// offset by parent
		(*outY)+=theAttributes.y;
		XQueryTree(xDisplay,current,&root,&parent,&children,&numChildren);	// this is an annoying way to get the parent, but it is all X allows
		XFree((char *)children);
		current=parent;														// step back to the parent
	}
}

void GlobalToLocal(Window theXWindow,INT32 x,INT32 y,INT32 *outX,INT32 *outY)
// passed an x and y at the root level of the screen that theXWindow is on,
// convert the coordinates to those local to theXWindow, and return them
{
	GlobalOffset(theXWindow,outX,outY);
	(*outX)=x-(*outX);
	(*outY)=y-(*outY);
}

void LocalToGlobal(Window theXWindow,INT32 x,INT32 y,INT32 *outX,INT32 *outY)
// passed an x and y within theXWindow, return the root equivalent
{
	GlobalOffset(theXWindow,outX,outY);
	(*outX)+=x;
	(*outY)+=y;
}

void EditorBeep()
// make a noise so we can tell what is going on
{
	XBell(xDisplay,0);
}

void ReportMessage(char *format,...)
// report messages to the console
{
	va_list
		args;
	char
		tempString[4096];						// this must be large in case the string is huge! (sprintf does not allow us to limit it)

	va_start(args,format);
	vsprintf(tempString,format,args);			// make the string
	va_end(args);

	OkDialog(tempString);
}

void ShowBusy()
// the editor calls this when it is doing something that may take some
// time, it should show the user (by changing the pointer or something)
// this call is nestable
{
	if(!showingBusy)
	{
//		XRecolorCursor(xDisplay,caretCursor,&gray2,&gray2);
	}
	showingBusy++;
}

void ShowNotBusy()
// when the editor is again ready to accept user input, it will call this
// it should undo anything ShowBusy did
// every ShowBusy will be exactly balanced by a call to ShowNotBusy
{
	if(showingBusy)
	{
		showingBusy--;
	}
	if(!showingBusy)
	{
//		XRecolorCursor(xDisplay,caretCursor,&gray0,&gray3);
	}
}

char *CreateWindowTitleFromPath(char *absolutePath)
// use absolute path to generate a string (which is returned) that
// should be placed in the title of a window
// the string should be disposed of using MDisposePtr
// if there is a problem, SetError, and return NULL
{
	int
		pathLength;
	int
		nameLength;
	char
		*returnString;

	pathLength=strlen(absolutePath);
	if((returnString=(char *)MNewPtr(pathLength+5)))
	{
		nameLength=0;
		while(pathLength&&absolutePath[pathLength-1]!=PATHSEP)	// move backwards until the first part of the path is encountered
		{
			nameLength++;
			pathLength--;
		}
		sprintf(returnString,"%.*s -- %.*s",nameLength,&(absolutePath[pathLength]),pathLength,&(absolutePath[0]));
		return(returnString);
	}
	return(NULL);
}

char *realpath2(char *path,char *outputPath)
// This routine replaces the system call "realpath" since there
// still exist machines that do not implement it
// This function directly replaces "realpath"
// NOTE: if path is passed as a string of length 0, this
// will return the current directory (which is what the real
// realpath does)
{

#define	MAXLINKDEPTH	256						// maximum number of links we will follow before giving up

	int
		currentLinkPath;						// tells which linkBuffer to read into next time we readlink
	char
		linkBuffer[2][MAXPATHLEN];				// two buffers to read links into
	char
		*linkPath;								// pointer to current link buffer
	int
		linkDepth,								// number of times links have been traversed
		outputLength,							// current length of output buffer
		linkLength;								// length of link returned by readlink
	bool
		hadRelativeComponent;					// boolean to tell if ., or .. was seen

	currentLinkPath=0;							// use buffer 0 next time
	linkDepth=0;								// so far, no links encountered
	outputLength=0;								// current length of output path

	if(*path!=PATHSEP)							// if path does not start with PATHSEP it is relative to current directory (even if path[0]=='\0')
	{
		if(getcwd(outputPath,MAXPATHLEN))		// get absolute path up to this point
		{
			outputLength=strlen(outputPath);	// get the length of the path returned
			if((!outputLength)||outputPath[outputLength-1]!=PATHSEP)	// make sure the end of the path has a PATHSEP on it
			{
				outputPath[outputLength++]=PATHSEP;	// drop it in
			}
		}
		else
		{
			errno=ENOENT;						// something went amiss with getcwd, so make up an error code, and leave
			return(NULL);
		}
	}
	else
	{
		outputPath[outputLength++]=PATHSEP;		// path is absolute, so start it
		path++;									// eat the PATHSEP in the input
	}

	while(*path)								// get each component of the path, and expand
	{
		if(*path!=PATHSEP)						// gobble up separators
		{
			hadRelativeComponent=false;			// assume no ., or ..
			if(path[0]=='.')					// see if possible relative path component
			{
				if(path[1]=='.')
				{
					if(!path[2]||path[2]==PATHSEP)	// just got '..', so move back over it, and previous path component, if any
					{
						path+=2;				// skip over '..' of input
						if(outputLength>1)		// if at root, do not backup
						{
							outputLength--;		// step back over trailing PATHSEP
							while(outputLength&&outputPath[outputLength-1]!=PATHSEP)
							{
								outputLength--;
							}
						}
						hadRelativeComponent=true;	// tell rest of loop this was handled
					}
				}
				else
				{
					if(!path[1]||path[1]==PATHSEP)	// just saw '.', which is redundant, so remove it
					{
						path++;					// skip over '.'
						hadRelativeComponent=true;	// tell rest of loop this was handled
					}
				}
			}

			if(!hadRelativeComponent)			// if the component was not relative, copy it to the output, and check for links
			{
				while(*path&&*path!=PATHSEP)		// copy the next pathname component
				{
					if(outputLength<MAXPATHLEN-1)	// check for overflow, leave room for 0 termination
					{
						outputPath[outputLength++]=*path++;	// copy the character
					}
					else
					{
						errno=ENAMETOOLONG;		// ran out of space during the copy, complain
						return(NULL);
					}
				}

				outputPath[outputLength]='\0';	// terminate the path that is being created
				linkPath=linkBuffer[currentLinkPath];	// point to the spare link buffer
				if((linkLength=readlink(outputPath,linkPath,MAXPATHLEN-1))>=0)	// attempt to see what this is linked to, MAXPATHLEN-1 is to leave room for 0 terminator
				{
					if(linkDepth++<=MAXLINKDEPTH)	// make sure we have not exhausted max number of links
					{
						linkPath[linkLength]='\0';	// terminate the path returned by readlink
						if(linkPath[0]==PATHSEP)	// see if the link is absolute or relative
						{
							outputLength=0;			// if the link is absolute, then the output is completely re-written, and scan continues from this point
						}
						else
						{
							while(outputLength&&outputPath[outputLength-1]!=PATHSEP)	// the link is relative, so remove the link component from the output
							{
								outputLength--;
							}
							if(outputLength)		// step back over the path separator too
							{
								outputLength--;
							}
						}
						if(linkLength+strlen(path)<MAXPATHLEN)	// make sure the remainder of the path will fit into the buffer
						{
							strcat(linkPath,path);	// append what's left of the path to the output of readlink
							path=linkPath;			// call this the new path
							currentLinkPath^=1;		// swap link buffers
						}
						else
						{
							errno=ENAMETOOLONG;
							return(NULL);
						}
					}
					else
					{
						errno=ELOOP;				// too many links traversed, complain
						return(NULL);
					}
				}
				else
				{
					if(errno!=EINVAL)		// anything but EINVAL is fatal, EINVAL is ok, the file is just not a symbolic link
					{
						return(NULL);
					}
				}
				outputPath[outputLength++]=PATHSEP;	// add separator to the output
			}
		}
		else
		{
			path++;								// skip extraneous path separators
		}
	}

	if((outputLength>1)&&outputPath[outputLength-1]==PATHSEP)	// realpath does not return the last PATHSEP unless we're at the root
	{
		outputLength--;							// strip off final PATHSEP
	}
	outputPath[outputLength]='\0';				// terminate the result
	return(outputPath);							// return it
}

char *CreateAbsolutePath(char *relativePath)
// given a relative path, attempt to make it an absolute path
// NOTE: if the relativePath does not point to anything valid, or the absolute path buffer
// could not be allocated, SetError, and return NULL
// the returned buffer will be 0 terminated, and must be freed by calling
// MDisposePtr
// NOTE: the result of calling this for the same file MUST always be
// EXACTLY the same string (case sensitive). The editor may perform
// strcmp style operations on these.
{
	char
		resolvedPath[MAXPATHLEN];
	char
		*returnPath;

	if(realpath2(relativePath,&(resolvedPath[0])))				// get real path if possible
	{
		if((returnPath=(char *)MNewPtr(strlen(&(resolvedPath[0]))+1)))	// create buffer to pass it back in
		{
			strcpy(returnPath,&(resolvedPath[0]));				// copy it into the buffer
			return(returnPath);
		}
	}
	else
	{
		SetStdCLibError();
	}
	return(NULL);
}

#define	STARTUPNAME		"e93lib/e93rc.tcl"						// name of the startup file

static bool ScriptExists(char *scriptPath)
// See if the script at script path exists, and is readable
// If so, return true, if not, return false
{
	int
		fileID;

	if((fileID=open(scriptPath,O_RDONLY))>=0)
	{
		close(fileID);
		return(true);
	}
	return(false);
}

bool LocateStartupScript(char *scriptPath)
// Search for the editor's startup script, return a path
// to the script which is located.
// NOTE: scriptPath must point to at least MAXPATHLEN bytes
// If no script could be located, complain and return false
{
	sprintf(scriptPath,"%s/lib/%s",PREFIX,STARTUPNAME);
	if(ScriptExists(scriptPath))
	{
		return(true);
	}
	fprintf(stderr,"Failed to locate startup script: %s\n",scriptPath);
	return(false);
}

EDITORFILE *OpenEditorReadFile(char *thePath)
// open a document file for reading into the editor
// if there is an error, SetError will be called, and NULL returned
{
	EDITORFILE
		*theEditorFile;

	if((theEditorFile=(EDITORFILE *)MNewPtr(sizeof(EDITORFILE))))
	{
		if((theEditorFile->fileHandle=open(thePath,O_RDONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))!=-1)
		{
			return(theEditorFile);
		}
		else
		{
			SetStdCLibError();						// set high level error information
		}
		MDisposePtr(theEditorFile);
	}
	return(NULL);
}

EDITORFILE *OpenEditorWriteFile(char *thePath)
// open a document file for writing into by the editor
// if there is an error, SetError will be called, and NULL returned
{
	EDITORFILE
		*theEditorFile;

	if((theEditorFile=(EDITORFILE *)MNewPtr(sizeof(EDITORFILE))))
	{
		if((theEditorFile->fileHandle=open(thePath,O_CREAT|O_TRUNC|O_WRONLY,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH))!=-1)
		{
			return(theEditorFile);
		}
		else
		{
			SetStdCLibError();						// set high level error information
		}
		MDisposePtr(theEditorFile);
	}
	return(NULL);
}

bool ReadEditorFile(EDITORFILE *theFile,UINT8 *theBuffer,UINT32 bytesToRead,UINT32 *bytesRead)
// read at most bytesToRead from theFile into theBuffer
// return the actual amount read in bytesRead
// if bytesRead is less than bytesToRead, then EOF is assumed
// if there is an error, SetError will be called, and false returned
{
	int
		bytesJustRead;

	if((bytesJustRead=read(theFile->fileHandle,(char *)theBuffer,(int)bytesToRead))!=(int)bytesToRead)
	{
		if(bytesJustRead==-1)			// if there was an error, set it it
		{
			SetStdCLibError();			// set high level error information
			*bytesRead=0;
			return(false);
		}
	}
	*bytesRead=bytesJustRead;
	return(true);
}

bool WriteEditorFile(EDITORFILE *theFile,UINT8 *theBuffer,UINT32 bytesToWrite,UINT32 *bytesWritten)
// write bytesToWrite from theBuffer into theFile
// return the actual amount written in bytesWritten
// bytesWritten should be the same as bytesToWrite, unless there was an error
// if there is an error, SetError will be called, and false returned
{
	int
		bytesJustWritten;

	if((bytesJustWritten=write(theFile->fileHandle,(char *)theBuffer,(int)bytesToWrite))!=(int)bytesToWrite)
	{
		if(bytesJustWritten==-1)			// if there was an error, set it it
		{
			SetStdCLibError();				// set high level error information
			*bytesWritten=0;
			return(false);
		}
	}
	*bytesWritten=bytesJustWritten;
	return(true);
}

void CloseEditorFile(EDITORFILE *theFile)
// when the editor is finished with a file, it will call this
{
	close(theFile->fileHandle);					// get rid of the file
	MDisposePtr(theFile);
}

void OutlineShadowRectangle(Window xWindow,GC graphicsContext,EDITORRECT *theRect,GUICOLOR *upperLeftColor,GUICOLOR *lowerRightColor,UINT32 thickness)
// outline theRect with a shadow effect in thickness
{
	UINT32
		thicknessCount;

	for(thicknessCount=0;thicknessCount<thickness&&thicknessCount<(theRect->h/2);thicknessCount++)
	{
		XSetForeground(xDisplay,graphicsContext,upperLeftColor->theXPixel);
		XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x,theRect->y+thicknessCount,theRect->x+theRect->w-thicknessCount-1,theRect->y+thicknessCount);	// top
		XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x+thicknessCount,theRect->y,theRect->x+thicknessCount,theRect->y+theRect->h-thicknessCount-1);	// left

		XSetForeground(xDisplay,graphicsContext,lowerRightColor->theXPixel);
		XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x+thicknessCount,theRect->y+theRect->h-thicknessCount-1,theRect->x+theRect->w-thicknessCount-1,theRect->y+theRect->h-thicknessCount-1); // bottom
		XDrawLine(xDisplay,xWindow,graphicsContext,theRect->x+theRect->w-thicknessCount-1,theRect->y+thicknessCount,theRect->x+theRect->w-thicknessCount-1,theRect->y+theRect->h-thicknessCount-1); // right
	}
}
