// File name globbing
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

static char localErrorFamily[]="glob";

enum
{
	NOUSER,
	MATCHFAILED,
	PATHTOOLONG,
	UNBALANCEDSQUAREBRACKET,
	DANGLINGQUOTE
};

static char *errorMembers[]=
{
	"NoUser"
	"MatchFailed",
	"PathTooLong",
	"UnbalancedSquareBracket",
	"DanglingQuote"
};

static char *errorDescriptions[]=
{
	"Failed to locate user",
	"Nothing matched",
	"Path grew too large",
	"Unbalanced [ or [^",
	"Incomplete \\ expression"
};

typedef struct
{
	bool
		dontGlobLast,	// if true, the last element of the pattern will not be checked for existence if it contains no meta characters
		passNoMeta;		// if true, the entire pattern will not be checked for existence if it contains no meta characters
	bool
		hadMeta;		// tells if we have seen a meta character in the path so far
	UINT32
		numPaths;		// count of total paths so far
	char
		**thePaths;		// list of paths matching pattern
} GLOBDATA;

static void ExtractFileName(char *fullPath,char *filePart)
// given fillPath which is considered to be a complete path and file name,
// truncate fullPath to just the path part, and copy the file part
// to filePart
// NOTE: filePart must be at least as large an array as fullPath
{
	int
		theLength;

	theLength=strlen(fullPath);
	while(theLength>=0&&fullPath[theLength]!=PATHSEP)
	{
		theLength--;
	}
	if(theLength>0)
	{
		strcpy(filePart,&(fullPath[theLength+1]));
		fullPath[theLength]='\0';
	}
	else
	{
		if(fullPath[0]==PATHSEP)			// / at 0 is special
		{
			strcpy(filePart,&(fullPath[1]));
			fullPath[1]='\0';
		}
		else
		{
			strcpy(filePart,fullPath);
			fullPath[0]='\0';
		}
	}
}

void SplitPathAndFile(char *fullPath,char *pathPart,char *filePart)
// split fullPath into pathPart, and filePart
// NOTE: pathPart and filePart must be at least MAXPATHLEN+1
// bytes long
// if fullPath is longer than MAXPATHLEN, parts will get truncated when copied
// NOTE: if fullPath is not a valid path, return the whole thing as filePart,
// and the current directory as the pathPart
{
	struct stat
		theStat;
	char
		tempPath[MAXPATHLEN+1];

	if(realpath2(fullPath,pathPart))
	{
		if((stat(pathPart,&theStat)!=-1)&&S_ISDIR(theStat.st_mode))		// point to a file, or a directory?
		{
			filePart[0]='\0';			// points to a directory, so no file part, this is done
		}
		else
		{
			ExtractFileName(pathPart,filePart);	// pointed to a file, so separate them
		}
	}
	else
	{
		strncpy(tempPath,fullPath,MAXPATHLEN);	// copy this to separate it
		tempPath[MAXPATHLEN]='\0';
		ExtractFileName(tempPath,filePart);		// separate the pieces
		if(!realpath2(tempPath,pathPart))		// try to get the path to the unknown file part
		{
			if(!realpath2(".",pathPart))		// get the current directory
			{
				pathPart[0]='\0';				// just return empty string
			}
			strncpy(filePart,fullPath,MAXPATHLEN);
			filePart[MAXPATHLEN]='\0';
		}
	}
}

void ConcatPathAndFile(char *thePath,char *theFile,char *theResult)
// merge thePath and theFile to form a fully qualified file name
// theResult must be at least MAXPATHLEN+1 characters long
// NOTE: if thePath and theFile combine into a string longer than MAXPATHLEN,
// the combined string will be truncated
// NOTE ALSO, if theFile specifies a complete Path (starts with a PATHSEP) then
// it alone is added to theResult
{
	int
		pathLength,
		fileLength;

	if(theFile[0]!=PATHSEP)
	{
		pathLength=strlen(thePath);
		if(pathLength>MAXPATHLEN)
		{
			pathLength=MAXPATHLEN;
		}
		strncpy(theResult,thePath,pathLength);
		if(pathLength&&(thePath[pathLength-1]!=PATHSEP)&&(pathLength<MAXPATHLEN))
		{
			theResult[pathLength++]=PATHSEP;
		}
	}
	else
	{
		pathLength=0;
	}
	fileLength=strlen(theFile);
	if(pathLength+fileLength>MAXPATHLEN)
	{
		fileLength=(MAXPATHLEN-pathLength);
	}
	strncpy(&(theResult[pathLength]),theFile,fileLength);
	theResult[pathLength+fileLength]='\0';
}

// string list functions

void FreeStringList(char **theList)
// free a list of strings created by calling NewStringList
{
	UINT32
		theIndex;

	theIndex=0;
	while(theList&&theList[theIndex])
	{
		MDisposePtr(theList[theIndex]);
		theIndex++;
	}
	MDisposePtr(theList);
}

static int CompareStrings(const void *i,const void *j)
// compare two elements of the list
{
	char
		*elementA,
		*elementB;

	elementA=*((char **)i);
	elementB=*((char **)j);

	return(strcmp(elementA,elementB));
}

void SortStringList(char **theList,UINT32 numElements)
// run through any elements of theList, and sort them
// by calling qsort
{
	if(theList)
	{
		qsort((char *)theList,numElements,sizeof(char **),CompareStrings);
	}
}

bool ReplaceStringInList(char *theString,char **theList,UINT32 theElement)
// place theString into theList at the given position, freeing
// any string that was there
// NOTE: theElement must be in range
// if there is a problem leave the entry untouched, SetError, and return false
{
	char
		*newElement;

	if((newElement=(char *)MNewPtr(strlen(theString)+1)))
	{
		strcpy(newElement,theString);
		MDisposePtr(theList[theElement]);
		theList[theElement]=newElement;
		return(true);
	}
	return(false);
}

bool AddStringToList(char *theString,char ***theList,UINT32 *numElements)
// add theString to theList
// at any point, theList can be deleted by calling FreeStringList
// if there is a problem, do not add the element, SetError, and return false
{
	char
		**newList;
	UINT32
		newLength;

	newLength=sizeof(char *)*((*numElements)+2);
	if((newList=(char **)MResizePtr(*theList,newLength)))
	{
		(*theList)=newList;
		if(((*theList)[*numElements]=(char *)MNewPtr(strlen(theString)+1)))
		{
			strcpy((*theList)[(*numElements)++],theString);
			(*theList)[*numElements]=NULL;									// keep the list terminated with a NULL
			return(true);
		}
	}
	return(false);
}

char **NewStringList()
// Create an empty string list
// if there is a problem, SetError, return NULL
{
	char
		**theList;

	if((theList=(char **)MNewPtr(sizeof(char *))))
	{
		theList[0]=NULL;
	}
	return(theList);
}

// tilde expansion routines

static bool ExpandTildes(char *thePattern,char *newPattern,UINT32 newPatternLength)
// expand tilde sequences in thePattern, write output into newPattern
// if there is a problem, SetError, and return false
{
	bool
		done,
		fail;
	char
		theChar;
	UINT32
		inIndex,
		outIndex;
	char
		*theHome;
	struct passwd
		*passwordEntry;
	UINT32
		theLength;

	fail=false;
	if(thePattern[0]=='~')	// see if a tilde exists at the start of thePattern
	{
		inIndex=1;			// skip the tilde
		outIndex=0;
		done=false;
		while(!done&&!fail&&(theChar=thePattern[inIndex])&&outIndex<(newPatternLength-1))
		{
			switch(theChar)
			{
				case PATHSEP:
					inIndex++;
					done=true;
					break;
				case '\\':
					inIndex++;
					if(!(newPattern[outIndex++]=thePattern[inIndex]))
					{
						SetError(localErrorFamily,errorMembers[DANGLINGQUOTE],errorDescriptions[DANGLINGQUOTE]);
						fail=true;
					}
					break;
				default:
					newPattern[outIndex++]=theChar;
					inIndex++;
					break;
			}
		}
		if(!fail)
		{
			newPattern[outIndex]='\0';		// terminate the user name string
			theHome=NULL;					// no home found yet
			if(outIndex)					// name was specified
			{
				if((passwordEntry=getpwnam(newPattern)))
				{
					theHome=passwordEntry->pw_dir;
				}
			}
			else
			{
				if(!(theHome=getenv("HOME")))	// try to find environment variable
				{
					if((passwordEntry=getpwuid(getuid())))	// if no environment, try to get from password database
					{
						theHome=passwordEntry->pw_dir;
					}
				}
			}
			if(theHome)
			{
				strncpy(newPattern,theHome,newPatternLength-1);
				newPattern[newPatternLength-1]='\0';
				theLength=strlen(newPattern);
				if(theLength&&theLength<newPatternLength-1)
				{
					if(newPattern[theLength-1]!=PATHSEP)	// add path separator if needed
					{
						newPattern[theLength++]=PATHSEP;
						newPattern[theLength]='\0';
					}
				}
				strncat(newPattern,&thePattern[inIndex],newPatternLength-theLength);
			}
			else
			{
				SetError(localErrorFamily,errorMembers[NOUSER],errorDescriptions[NOUSER]);	// user not there
				fail=true;
			}
		}
	}
	else
	{
		strncpy(newPattern,thePattern,newPatternLength-1);
		newPattern[newPatternLength-1]='\0';
	}
	return(!fail);
}

// wildcard expansion routines

static bool MatchRange(char theChar,char *thePattern,UINT32 *patternIndex,bool *haveMatch)
// a range is starting in thePattern, so attempt to match theChar
// against it
{
	bool
		fail,
		done,
		isNot;
	char
		testChar,
		lastChar;

	isNot=done=fail=false;
	if(thePattern[*patternIndex]=='^')				// check for not flag
	{
		isNot=true;
		(*patternIndex)++;
	}
	lastChar='\0';									// start with bottom of range at 0
	*haveMatch=false;
	while(!done&&!fail&&(testChar=thePattern[(*patternIndex)++]))
	{
		switch(testChar)
		{
			case '\\':
				if((testChar=thePattern[(*patternIndex)++]))	// get next character to test
				{
					if(theChar==testChar)					// test it literally
					{
						*haveMatch=true;
					}
				}
				else
				{
					SetError(localErrorFamily,errorMembers[DANGLINGQUOTE],errorDescriptions[DANGLINGQUOTE]);
					fail=true;						// got \ at the end of the pattern
				}
				break;
			case '-':
				if((testChar=thePattern[(*patternIndex)++]))		// get the next character for the range (if there is one)
				{
					switch(testChar)
					{
						case '\\':
							if((testChar=thePattern[(*patternIndex)++]))
							{
								if(theChar>=lastChar&&theChar<=testChar)
								{
									*haveMatch=true;
								}
							}
							else
							{
								SetError(localErrorFamily,errorMembers[DANGLINGQUOTE],errorDescriptions[DANGLINGQUOTE]);
								fail=true;						// got \ at the end of the pattern
							}
							break;
						case ']':
							if(theChar>=lastChar)				// empty range at end, so take as infinite end
							{
								*haveMatch=true;
							}
							done=true;
							break;
						default:
							if(theChar>=lastChar&&theChar<=testChar)
							{
								*haveMatch=true;
							}
							break;
					}
				}
				else
				{
					SetError(localErrorFamily,errorMembers[UNBALANCEDSQUAREBRACKET],errorDescriptions[UNBALANCEDSQUAREBRACKET]);
					fail=true;
				}
				break;
			case ']':
				done=true;							// scanning is done (empty lists are allowed)
				break;
			default:								// otherwise it is normal, and just gets tested
				if(theChar==testChar)
				{
					*haveMatch=true;
				}
				break;
		}
		lastChar=testChar;							// remember this for next time around the loop
	}
	if(!done&&!fail)								// ran out of things to scan
	{
		SetError(localErrorFamily,errorMembers[UNBALANCEDSQUAREBRACKET],errorDescriptions[UNBALANCEDSQUAREBRACKET]);
		fail=true;
	}
	if(!fail)
	{
		if(isNot)
		{
			*haveMatch=!*haveMatch;
		}
	}
	return(!fail);
}

static bool MatchName(char *theName,char *thePattern,UINT32 *patternIndex,bool *haveMatch)
// match theName against thePattern
// thePattern points to the start of a pattern terminated with a '\0', or a PATHSEP
// update patternIndex to point to the character that stopped the match
// if a match occurs, return true if have match
// if there is a problem SetError, and return false
{
	UINT32
		nameIndex;
	UINT32
		startPatternIndex;
	bool
		fail,
		done,
		matching;
	char
		theChar;

	fail=*haveMatch=done=false;
	matching=true;
	nameIndex=0;
	while(!done&&matching)
	{
		switch(thePattern[*patternIndex])
		{
			case '\0':				// ran out of characters of the pattern
			case PATHSEP:
				matching=(theName[nameIndex]=='\0');
				done=true;
				break;
			case '*':
				(*patternIndex)++;	// move past the *
				matching=false;
				do
				{
					startPatternIndex=*patternIndex;
					fail=!MatchName(&theName[nameIndex],thePattern,&startPatternIndex,&matching);
				}
				while(theName[nameIndex++]&&!fail&&!matching);			// recurse trying to make a match
				if(matching)
				{
					*patternIndex=startPatternIndex;
					done=true;
				}
				break;
			case '?':
				(*patternIndex)++;
				matching=(theName[nameIndex++]!='\0');
				break;
			case '[':
				(*patternIndex)++;
				if((theChar=theName[nameIndex++]))
				{
					fail=!MatchRange(theChar,thePattern,patternIndex,&matching);
				}
				else
				{
					matching=false;
				}
				break;
			case '\\':
				(*patternIndex)++;
				if((theChar=thePattern[(*patternIndex)++]))
				{
					matching=(theName[nameIndex++]==theChar);
				}
				else
				{
					SetError(localErrorFamily,errorMembers[DANGLINGQUOTE],errorDescriptions[DANGLINGQUOTE]);
					fail=true;
				}
				break;
			default:
				matching=(theName[nameIndex++]==thePattern[(*patternIndex)++]);
				break;
		}
	}
	if(!fail)
	{
		*haveMatch=matching;
	}
	return(!fail);
}

// prototype this because it is mutually recursive with MoveFurther
static bool GlobAgainstDirectory(char *thePattern,UINT32 patternIndex,char *pathBuffer,UINT32 pathIndex,GLOBDATA *theData);

static bool MoveFurther(char *thePattern,UINT32 patternIndex,char *pathBuffer,UINT32 pathIndex,GLOBDATA *theData)
// this attempts to add to pathBuffer by reading from thePattern at patternIndex
// pattern data is added to pathBuffer so that all data containing no meta characters is copied
// up to the first PATHSEP before data that does contain meta characters
// if there is a problem, SetError, and return false
{
	bool
		fail;
	bool
		done;
	UINT32
		lastPatternIndex,
		lastPathIndex;
	bool
		nextSpecial;
	bool
		hadMeta;					// tells if locally, we have seen any meta characters yet

	fail=done=nextSpecial=hadMeta=false;
	lastPatternIndex=patternIndex;
	lastPathIndex=pathIndex;
	while(!done&&pathIndex<MAXPATHLEN)
	{
		// copy all of thePattern that contains no meta characters until the next PATHSEP into pathBuffer
		pathBuffer[pathIndex]=thePattern[patternIndex];
		switch(thePattern[patternIndex])
		{
			case '\0':				// ran out of characters of the pattern, so this is not path information
				if((!hadMeta&&theData->dontGlobLast)||(!theData->hadMeta&&theData->passNoMeta))		// if no meta characters, then this IS the path (assuming we dont glob the last one)
				{
					pathBuffer[pathIndex]='\0';		// terminate the current path
					fail=!AddStringToList(pathBuffer,&theData->thePaths,&theData->numPaths);	// add this to the list
				}
				done=true;
				break;
			case '*':				// meta character (this means we stop here)
			case '?':
			case '[':
				if(nextSpecial)
				{
					patternIndex++;
					pathIndex++;
					nextSpecial=false;
				}
				else
				{
					hadMeta=true;				// remember locally that there were meta characters
					theData->hadMeta=true;		// remember it globally as well
					done=true;
				}
				break;
			case PATHSEP:
				patternIndex++;
				pathIndex++;
				lastPatternIndex=patternIndex;			// remember these
				lastPathIndex=pathIndex;
				nextSpecial=false;
				break;
			case '\\':
				if(nextSpecial)
				{
					patternIndex++;
					pathIndex++;
					nextSpecial=false;
				}
				else
				{
					nextSpecial=true;
					patternIndex++;						// skip the \, but do not move forward in path
				}
				break;
			default:
				patternIndex++;
				pathIndex++;
				nextSpecial=false;
				break;
		}
	}
	if(done)											// we got something
	{
		if(!((!hadMeta&&theData->dontGlobLast)||(!theData->hadMeta&&theData->passNoMeta)))	// if we aren't passing this verbatim, then glob against it
		{
			patternIndex=lastPatternIndex;				// move back
			pathIndex=lastPathIndex;
			pathBuffer[pathIndex]='\0';					// terminate the current path
			fail=!GlobAgainstDirectory(thePattern,patternIndex,pathBuffer,pathIndex,theData);	// expand the pattern at the end of the current path
		}
	}
	else											// path was about to overflow output
	{
		SetError(localErrorFamily,errorMembers[PATHTOOLONG],errorDescriptions[PATHTOOLONG]);
		fail=true;
	}
	return(!fail);

}

static bool GlobAgainstDirectory(char *thePattern,UINT32 patternIndex,char *pathBuffer,UINT32 pathIndex,GLOBDATA *theData)
// try thePattern against all files at pathBuffer, adding each that matches completely, and calling
// move further on each one
// if there is some sort of problem, SetError, and return false
{
	DIR
		*theDirectory;
	struct dirent
		*theEntry;
	bool
		fail;
	char
		*thePath;
	UINT32
		theLength;
	UINT32
		newIndex;
	bool
		haveMatch;

	fail=false;
	if(pathBuffer[0])
	{
		thePath=pathBuffer;						// point at the passed path if one given
	}
	else
	{
		thePath="./";							// if passed path is empty, it means current directory
	}
	if((theDirectory=opendir(thePath)))			// if it does not open, assume it is because thePath did not point to something valid
	{
		while(!fail&&(theEntry=readdir(theDirectory)))
		{
			newIndex=patternIndex;
			if(theEntry->d_name[0]!='.'||thePattern[patternIndex]=='.')			// the first . must be matched exactly
			{
				if(MatchName(theEntry->d_name,thePattern,&newIndex,&haveMatch))
				{
					if(haveMatch)					// was there a match?
					{
						theLength=strlen(theEntry->d_name);
						if(pathIndex+theLength<MAXPATHLEN)	// append to path (if there's room)
						{
							strcpy(&(pathBuffer[pathIndex]),theEntry->d_name);
							if(thePattern[newIndex])		// see if there's more pattern left
							{
								fail=!MoveFurther(thePattern,newIndex,pathBuffer,pathIndex+theLength,theData);
							}
							else
							{
								fail=!AddStringToList(pathBuffer,&theData->thePaths,&theData->numPaths);
							}
						}
						else
						{
							SetError(localErrorFamily,errorMembers[PATHTOOLONG],errorDescriptions[PATHTOOLONG]);
							fail=true;
						}
					}
				}
				else
				{
					fail=true;
				}
			}
		}
		closedir(theDirectory);
	}
	return(!fail);
}

char **Glob(char *thePath,char *thePattern,bool dontGlobLast,bool passNoMeta,UINT32 *numElements)
// glob thePattern into an array of paths, and return it
// if there is a problem, SetError, and return NULL
// thePath is the initial path to begin globbing at if thePattern
// does not specify an absolute path
// NOTE: the returned paths must be deleted by a call to
// FreeStringList
// if dontGlobLast is true, the last element of thePattern (if it contains
// no meta characters) will not be checked against its directory,
// but instead will be taken to exist, and will be placed into the output
// this is useful when trying to save to a globbed path, and the last element
// of the path is the name of a file which does not necessarily exist yet.
// if passNoMeta is true, any pattern that contains no wild card
// characters will be placed into the output without being checked
{
	GLOBDATA
		theData;
	char
		newPattern[MAXPATHLEN+1];					// tilde expand the pattern into here
	char
		pathBuffer[MAXPATHLEN+1];					// this is the place to build the output path
	UINT32
		theLength;

	*numElements=0;									// no elements yet
	if((theData.thePaths=NewStringList()))
	{
		theData.numPaths=0;							// no paths so far
		theData.dontGlobLast=dontGlobLast;			// set up flags that tell how to glob
		theData.passNoMeta=passNoMeta;
		theData.hadMeta=false;						// no meta characters seen so far
		if(ExpandTildes(thePattern,&newPattern[0],MAXPATHLEN+1))	// expand tilde sequences in thePattern, write output into newPattern
		{
			if(newPattern[0]==PATHSEP)
			{
				pathBuffer[0]='\0';					// pattern is absolute, so ignore given path
			}
			else
			{
				strncpy(pathBuffer,thePath,MAXPATHLEN);	// use the passed path as a prefix to the glob
			}
			theLength=strlen(pathBuffer);
			if(theLength&&theLength<MAXPATHLEN)			// if we have something, check to see if it needs a PATHSEP
			{
				if(pathBuffer[theLength-1]!=PATHSEP)	// add separator to end of path
				{
					pathBuffer[theLength++]=PATHSEP;
					pathBuffer[theLength]='\0';
				}
			}
			if(MoveFurther(&newPattern[0],0,pathBuffer,theLength,&theData))
			{
				if(theData.numPaths)
				{
					SortStringList(theData.thePaths,theData.numPaths);
					*numElements=theData.numPaths;
					return(theData.thePaths);
				}
				else
				{
					SetError(localErrorFamily,errorMembers[MATCHFAILED],errorDescriptions[MATCHFAILED]);
				}
			}
		}
		FreeStringList(theData.thePaths);
	}
	return(NULL);
}

char **GlobAll(char *thePath,UINT32 *numElements)
// return a sorted list of all of the directories/files at the end of thePath
// thePath is not expanded in any way.
// NOTE: the list will contain ONLY the last component of the path
// if there is a problem, SetError, and return NULL
// NOTE: the returned list must be later deleted by a call to
// FreeStringList
{
	char
		**theFiles;
	DIR
		*theDirectory;
	struct dirent
		*theEntry;
	bool
		fail;

	fail=false;
	*numElements=0;								// no paths so far
	if((theFiles=NewStringList()))
	{
		if((theDirectory=opendir(thePath)))		// if it does not open, assume it is because thePath did not point to something valid, and return NULL
		{
			while(!fail&&(theEntry=readdir(theDirectory)))
			{
				fail=!AddStringToList(theEntry->d_name,&theFiles,numElements);
			}
			closedir(theDirectory);
			if(!fail)
			{
				SortStringList(theFiles,*numElements);
				return(theFiles);
			}
		}
		else
		{
			SetStdCLibError();
		}
		FreeStringList(theFiles);
	}
	*numElements=0;
	return(NULL);
}
