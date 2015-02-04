// Font handling
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

// fallback fonts, (if we dont get the requested font, try these in succession)
#define	FALLBACKFONT0				"-*-lucidatypewriter-medium-r-normal-sans-12-*-*-*-*-*-*-*"
#define	FALLBACKFONT1				"fixed"
#define	FALLBACKFONT2				"-adobe-courier-medium-r-normal--12-120-75-75-*-70-*-*"
#define	FALLBACKFONT3				"7x13"

static GUIFONT
	*topFont;			// holds top font in linked list of gui fonts

static GUIFONT *LocateLoadedFont(char *theName)
// see if an already loaded font can be located which has theName
// if so, return a pointer to it, otherwise return NULL
// ### this should use a hash table
{
	GUIFONT
		*testFont;

	testFont=topFont;
	while(testFont&&(strcasecmp(testFont->theName,theName)!=0))
	{
		testFont=testFont->next;
	}
	return(testFont);
}

static XFontStruct *GetClosestFont(char *theName)
// return the X font which is the best we could find
{
	XFontStruct
		*theFont;

	if((theFont=XLoadQueryFont(xDisplay,theName)))
	{
		return(theFont);
	}
	if((theFont=XLoadQueryFont(xDisplay,FALLBACKFONT0)))
	{
		return(theFont);
	}
	if((theFont=XLoadQueryFont(xDisplay,FALLBACKFONT1)))
	{
		return(theFont);
	}
	if((theFont=XLoadQueryFont(xDisplay,FALLBACKFONT2)))
	{
		return(theFont);
	}
	if((theFont=XLoadQueryFont(xDisplay,FALLBACKFONT3)))
	{
		return(theFont);
	}
	SetError("font","NoFont","Could not locate font");
	return(NULL);
}

void FreeFont(GUIFONT *theFont)
// Free a font allocated by LoadFont
{
	if(!(--theFont->usageCount))
	{
		XFreeFont(xDisplay,theFont->theXFont);
		if(theFont->previous)
		{
			theFont->previous->next=theFont->next;
		}
		else
		{
			topFont=theFont->next;
		}
		if(theFont->next)
		{
			theFont->next->previous=theFont->previous;
		}
		MDisposePtr(theFont);
	}
}

GUIFONT *LoadFont(char *theName)
// attempt to load the font given by theName
// if there is a problem, SetError, return NULL
{
	GUIFONT
		*theResult;
	int
		overhang;
	int
		i,j;

	if((theResult=LocateLoadedFont(theName)))	// see if we can find this one already loaded
	{
		theResult->usageCount++;				// add another user to this font structure
		return(theResult);
	}
	else
	{
		if((theResult=(GUIFONT *)MNewPtr(sizeof(GUIFONT))))	// allocate one
		{
			if((theResult->theXFont=GetClosestFont(theName)))
			{
				strncpy(theResult->theName,theName,sizeof(theResult->theName));
				theResult->theName[sizeof(theResult->theName)-1]='\0';	// terminate string if copy was too long

				theResult->usageCount=1;

				theResult->previous=NULL;		// link to font list
				if((theResult->next=topFont))
				{
					topFont->previous=theResult;
				}
				topFont=theResult;

				theResult->ascent=theResult->theXFont->ascent;
				theResult->descent=theResult->theXFont->descent;

				theResult->maxLeftOverhang=0;			// initialize the overhang amounts
				theResult->maxRightOverhang=0;

				for(i=0;i<256;i++)
				{
					theResult->charWidths[i]=0;						// clear widths for non-existent characters
				}
				if(theResult->theXFont->per_char)					// see if there is information for individual characters
				{
					j=0;
					for(i=theResult->theXFont->min_char_or_byte2;i<=(int)theResult->theXFont->max_char_or_byte2;i++)
					{
						// handle left overhang
						overhang=-theResult->theXFont->per_char[j].lbearing;
						if(overhang>(INT32)theResult->maxLeftOverhang)
						{
							theResult->maxLeftOverhang=(UINT32)overhang;
						}

						// handle right overhang
						overhang=theResult->theXFont->per_char[j].rbearing-theResult->theXFont->per_char[j].width;
						if(overhang>(INT32)theResult->maxRightOverhang)
						{
							theResult->maxRightOverhang=(UINT32)overhang;
						}

						theResult->charWidths[i]=theResult->theXFont->per_char[j].width;	// set widths for existing characters
						j++;
					}
				}
				else
				{
					for(i=theResult->theXFont->min_char_or_byte2;i<=(int)theResult->theXFont->max_char_or_byte2;i++)
					{
						theResult->charWidths[i]=theResult->theXFont->max_bounds.width;		// set widths for existing characters
					}
				}
				return(theResult);
			}
			MDisposePtr(theResult);
		}
	}
	return(NULL);
}

void UnInitFonts()
// Undo what InitFonts did
{
}

bool InitFonts()
// set up font handling stuff
{
	topFont=NULL;					// no top font loaded in yet
	return(true);
}
