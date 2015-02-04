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

#define	DEFAULT_FONT		"-*-courier-medium-r-normal-*-10-*-*-*-*-*"
#define	DEFAULT_FONT_SIZE	10

static GUIFONT
	*topFont = NULL;		// holds top font in linked list of gui fonts

/*
 * The following defines specify the meaning of the fields in a fully
 * qualified XLFD.
 */

#define XLFD_FOUNDRY	    0
#define XLFD_FAMILY	    	1
#define XLFD_WEIGHT	    	2
#define XLFD_SLANT	    	3
#define XLFD_SETWIDTH	    4
#define XLFD_ADD_STYLE	    5
#define XLFD_PIXEL_SIZE	    6
#define XLFD_POINT_SIZE	    7
#define XLFD_RESOLUTION_X   8
#define XLFD_RESOLUTION_Y   9
#define XLFD_SPACING	    10
#define XLFD_AVERAGE_WIDTH  11
#define XLFD_CHARSET	    12
#define XLFD_NUMFIELDS	    13	/* Number of fields in XLFD. */


/*
 * The following structure is used as a two way map between integers
 * and strings.
 */

typedef struct Str2NumTable
{
    int
    	numKey;			/* Integer representation of a value. */
    char
    	*strKey;		/* String representation of a value. */
} Str2NumTable;


static Str2NumTable xlfdWeightMap[] =
{
    {FW_MEDIUM,		"medium"},
    {FW_NORMAL,		"book"},
    {FW_LIGHT,		"light"},
    {FW_BOLD,		"bold"},
    {FW_SEMIBOLD,	"demi"},
    {FW_DEMIBOLD,	"demibold"},
    {FW_NORMAL,		"normal"},
    {FW_NORMAL,		NULL}
};

static Str2NumTable xlfdSlantMap[] =
{
    {0,				"r"},		// Roman (no slant)
    {1,				"i"},		// Italic (slant left)
    {0,				"o"},		// Oblique (slant left)				*unsupported*
    {0,				"ri"},		// Reverse italic (slant right)		*unsupported*
    {0,				"ro"},		// Reverse oblique (slant right)	*unsupported*
    {0,				NULL}
};

static Str2NumTable xlfdSetwidthMap[] =
{
    {00,			"normal"},
    {90,			"narrow"},
    {75,			"semicondensed"},
    {50,			"condensed"},
    {00,			NULL}
};

/*
 *---------------------------------------------------------------------------
 *
 * Str2Num --
 *
 *	Given a lookup table, map a string to a number in the table.
 *
 * Results:
 *	If strKey was equal to the string keys of one of the elements
 *	in the table, returns the numeric key of that element.
 *	Returns the numKey associated with the last element (the NULL
 *	string one) in the table if strKey was not equal to any of the
 *	string keys in the table.
 *
 * Side effects.
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int Str2Num(const Str2NumTable *mapPtr, const char *strKey)
{
    const Str2NumTable
    	*mPtr;

    for (mPtr = mapPtr; mPtr->strKey != NULL; mPtr++)
    {
		if (strcmp(strKey, mPtr->strKey) == 0)
		{
		    return mPtr->numKey;
		}
    }
    
    return mPtr->numKey;
}

/*
 *---------------------------------------------------------------------------
 *
 * Num2Str --
 *
 *	Given a lookup table, map a number to a string in the table.
 *
  * Results:
 *   Returns "*" if it doesn't find a match
 *
 * Side effects.
 *	None.
 *
 *---------------------------------------------------------------------------
 */

const char *Num2Str(const Str2NumTable *mapPtr, int numKey)
{
    const Str2NumTable
    	*mPtr;

    for (mPtr = mapPtr; mPtr->strKey != NULL; mPtr++)
    {
		if (mPtr->numKey == numKey)
		{
		    return mPtr->strKey;
		}
    }
    
    return "*";
}

/*
 *---------------------------------------------------------------------------
 *
 * FieldSpecified --
 *
 *	Helper function for GetFontInfo().  Determines if a field in the
 *	XLFD was set to a non-null, non-don't-care value.
 *
 * Results:
 *	The return value is 0 if the field in the XLFD was not set and
 *	should be ignored, non-zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int FieldSpecified(const char *field)
{
    char
    	ch;

    if (field == NULL)
    {
		return 0;
    }
    
    ch = field[0];
    
    return (ch != '*' && ch != '?');
}

 
static void InitLogicalFont(LOGFONT *lf)
{
    // these are the fields we might convert from XLFD to a Windows LOGFONT, but these are the defaults if we can't
	strcpy(lf->lfFaceName, "courier");				// Family
    lf->lfWeight				= FW_MEDIUM;		// Weight
    lf->lfItalic				= 0;				// Slant
    lf->lfWidth					= 0;				// Setwidth
    												// Addional Style (e.g. Sans Serif)
    lf->lfHeight				= -MulDiv(DEFAULT_FONT_SIZE, GetDeviceCaps(GetDC(frameWindowHwnd), LOGPIXELSY), 72);
    												// Size

    // these are the fields in a Windows LOGFONT we do not set from XLFD, we only use these defaults
    lf->lfUnderline			= false;
    lf->lfStrikeOut			= false;
    lf->lfCharSet			= DEFAULT_CHARSET;
    lf->lfOutPrecision		= OUT_TT_PRECIS;
    lf->lfClipPrecision		= CLIP_DEFAULT_PRECIS;
    lf->lfQuality			= PROOF_QUALITY;
    lf->lfPitchAndFamily	= DEFAULT_PITCH | FF_MODERN;
    lf->lfEscapement		= 0;
    lf->lfOrientation		= 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetFontInfo --
 *
 *	Break up a fully specified XLFD into a Windows logical font.
 *
 *---------------------------------------------------------------------------
 *  -*-Family-Weight-Slant-SetWidth-*-Size-*-*-*-*-*-*-*
 *  -adobie-courier-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1
 */
void GetFontInfo(char *string, LOGFONT *lf)
{
    char
    	*src;
    const char
   		*str;
    int
    	i,
    	j,
    	pixelHeight = DEFAULT_FONT_SIZE;
    char
    	*field[XLFD_NUMFIELDS + 2];
    Tcl_DString
    	ds;

    memset(field, '\0', sizeof(field));

 	InitLogicalFont(lf);

	str = string;

    if (*str == '-')
    {
		str++;

	    Tcl_DStringInit(&ds);
	    Tcl_DStringAppend(&ds, (char *) str, -1);
	    src = Tcl_DStringValue(&ds);

	    field[0] = src;
	    for (i = 0; *src != '\0'; src++)
	    {
			if (!(*src & 0x80) && Tcl_UniCharIsUpper(UCHAR(*src)))
			{
			    *src = (char) Tcl_UniCharToLower(UCHAR(*src));
			}

			if (*src == '-')
			{
			    i++;
			    if (i == XLFD_NUMFIELDS)
			    {
					continue;
			    }

			    *src = '\0';
			    field[i] = src + 1;
			    if (i > XLFD_NUMFIELDS)
			    {
					break;
			    }
			}
	    }

	    /*
	     * An XLFD of the form -adobe-times-medium-r-*-12-*-* is pretty common,
	     * but it is (strictly) malformed, because the first * is eliding both
	     * the Setwidth and the Addstyle fields.  If the Addstyle field is a
	     * number, then assume the above incorrect form was used and shift all
	     * the rest of the fields right by one, so the number gets interpreted
	     * as a pixelsize.  This fix is so that we don't get a million reports
	     * that "it works under X (as a native font name), but gives a syntax
	     * error under Windows (as a parsed set of attributes)".
	     */

	    if ((i > XLFD_ADD_STYLE) && (FieldSpecified(field[XLFD_ADD_STYLE])))
	    {
			if (atoi(field[XLFD_ADD_STYLE]) != 0)
			{
			    for (j = XLFD_NUMFIELDS - 1; j >= XLFD_ADD_STYLE; j--)
			    {
					field[j + 1] = field[j];
			    }
			    field[XLFD_ADD_STYLE] = NULL;
			    i++;
			}
	    }

	    /* XLFD_FOUNDRY ignored. */

	    if (FieldSpecified(field[XLFD_FAMILY]))
	    {
			strcpy((char *)lf->lfFaceName, field[XLFD_FAMILY]);
	    }
	    
	    if (FieldSpecified(field[XLFD_WEIGHT]))
	    {
	    	lf->lfWeight = Str2Num(xlfdWeightMap, field[XLFD_WEIGHT]);
	    }
	    
	    if (FieldSpecified(field[XLFD_SLANT]))
	    {
			lf->lfItalic = Str2Num(xlfdSlantMap, field[XLFD_SLANT]);
	    }
	    
	    /* XLFD_ADD_STYLE ignored. */

	    /*
	     * Pointsize in tenths of a point, but treat it as tenths of a pixel
	     * for historical compatibility.
	     */

	    if (FieldSpecified(field[XLFD_POINT_SIZE]))
	    {
			if (field[XLFD_POINT_SIZE][0] == '[')
			{
			    /*
			     * Some X fonts have the point size specified as follows:
			     *
			     *	    [ N1 N2 N3 N4 ]
			     *
			     * where N1 is the point size (in points, not decipoints!), and
			     * N2, N3, and N4 are some additional numbers that I don't know
			     * the purpose of, so I ignore them.
			     */

			    pixelHeight = atoi(field[XLFD_POINT_SIZE] + 1);
			}
			else
			{
				int iTmp = strtol(field[XLFD_POINT_SIZE], NULL, 10);
				if (iTmp != 0)
				    pixelHeight = iTmp / 10;
			}
	    }

	    /*
	     * Pixel height of font.  If specified, overrides pointsize.
	     */

	    if (FieldSpecified(field[XLFD_PIXEL_SIZE]))
	    {
			if (field[XLFD_PIXEL_SIZE][0] == '[')
			{
			    /*
			     * Some X fonts have the pixel size specified as follows:
			     *
			     *	    [ N1 N2 N3 N4 ]
			     *
			     * where N1 is the pixel size, and where N2, N3, and N4
			     * are some additional numbers that I don't know
			     * the purpose of, so I ignore them.
			     */

			    pixelHeight = atoi(field[XLFD_PIXEL_SIZE] + 1);
			}
			else
			{
				int iTmp = strtol(field[XLFD_PIXEL_SIZE], NULL, 10);
				if (iTmp != 0)
				    pixelHeight = iTmp;
				else
				    pixelHeight = DEFAULT_FONT_SIZE;
			}
		}

		lf->lfHeight = -MulDiv(pixelHeight, GetDeviceCaps(GetDC(frameWindowHwnd), LOGPIXELSY), 72);

	    if (FieldSpecified(field[XLFD_SETWIDTH]))
	    {
			lf->lfWidth = lf->lfHeight * (Str2Num(xlfdSetwidthMap, field[XLFD_SETWIDTH])/100);
	    }

		/* XLFD_RESOLUTION_X ignored. */

		/* XLFD_RESOLUTION_Y ignored. */

		/* XLFD_SPACING ignored. */

		/* XLFD_AVERAGE_WIDTH ignored. */

		/* XLFD_CHARSET ignored. */

		Tcl_DStringFree(&ds);
	}
}

void PutFontInfo(LOGFONT *lf, char *theFontName)
/*	Passed a Windows logical font record, convert it to a partially specified XLFD
 *  -*-Family-Weight-Slant-Setwidth-*-Size-*-*-*-*-*-*-*
 *  -*-courier-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1

	Values from a Windows logical font that we don't save:
	    lfEscapement;
	    lfOrientation;
	    lfUnderline;
	    lfStrikeOut;
	    lfOutPrecision;
	    lfClipPrecision;
	    lfQuality;
	    lfCharSet;
	    lfPitchAndFamily;
 */
{	
	// sprintf(theFontName,"*"); // TODO: why does this fix the bad font string problem???!!!!!
	sprintf(theFontName,"-*-%s-%s-%s-%s-*-%ld-*-*-*-*-*",
														// * Foundry (unsupported) *
	lf->lfFaceName,										// Family
	Num2Str(xlfdWeightMap, lf->lfWeight),				// Weight
	Num2Str(xlfdSlantMap, lf->lfItalic),				// Slant
	Num2Str(xlfdSetwidthMap, lf->lfWidth),				// Setwidth
	-MulDiv(lf->lfHeight, 72, GetDeviceCaps(GetDC(frameWindowHwnd), LOGPIXELSY)));
}

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

void FreeFont(GUIFONT *theFont)
// Free a font allocated by LoadFont
{
	if(!(--theFont->usageCount))
	{
		DeleteObject(theFont->font);
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
	TEXTMETRIC
		tm;
    HDC
		hdc;
	HFONT
		winFont;
	LOGFONT
		lf;
	char
		*theFontName;
	ABC
		abcData[256];
	int
		i;

	
	theFontName = strlen(theName) ? theName : DEFAULT_FONT;
	
	if((theResult=LocateLoadedFont(theFontName)))	// see if we can find this one already loaded
	{
		theResult->usageCount++;					// add another user to this font structure
		return(theResult);
	}
	else
	{
		if((theResult=(GUIFONT *)MNewPtr(sizeof(GUIFONT))))	// allocate one
		{
			GetFontInfo(theFontName,&lf);
			strncpy(theResult->theName,theFontName,sizeof(theResult->theName));
			theResult->theName[sizeof(theResult->theName)-1]='\0';	// terminate string if copy was too long

			if(winFont=CreateFontIndirect(&lf))
			{
				theResult->font=winFont;
				hdc=GetDC(clientWindowHwnd);
				winFont=(HFONT)SelectObject(hdc,(HGDIOBJ)winFont);
				SetTextAlign(hdc,TA_LEFT|TA_BASELINE);
				SetMapMode(hdc,MM_TEXT);
				theResult->maxLeftOverhang=0;
				theResult->maxRightOverhang=0;

				if(GetWindowsVersion()==VER_PLATFORM_WIN32_NT)
				{
					GetCharWidth32(hdc,0,255,(int *)&(theResult->charWidths[0]));
				}
				else
				{
					GetCharWidth(hdc,0,255,(int *)&(theResult->charWidths[0]));
				}
				GetTextMetrics(hdc,&tm);
				theResult->minCharWidth=tm.tmMaxCharWidth;
				if(GetCharABCWidths(hdc,0,255,&(abcData[0])))		// if it's a truetype font
				{
					for(i=tm.tmFirstChar;i<=tm.tmLastChar;i++)
					{
						if(theResult->minCharWidth>(UINT32)theResult->charWidths[i])
						{
							theResult->minCharWidth=theResult->charWidths[i];
						}
						if(abcData[i].abcA<0)
						{
							if(theResult->maxLeftOverhang<(UINT32)abs(abcData[i].abcA))
							{
								theResult->maxLeftOverhang=abs(abcData[i].abcA);
							}
						}
						if(abcData[i].abcC<0)
						{
							if(theResult->maxRightOverhang<(UINT32)abs(abcData[i].abcC))
							{
								theResult->maxRightOverhang=abs(abcData[i].abcC);
							}
						}
					}
				}
				theResult->ascent=tm.tmAscent;
				theResult->descent=tm.tmDescent;
				SelectObject(hdc,(HGDIOBJ)winFont);
				ReleaseDC(clientWindowHwnd,hdc);
				theResult->usageCount=1;

				theResult->previous=NULL;		// link to font list
				if((theResult->next=topFont))
				{
					topFont->previous=theResult;
				}
				topFont=theResult;
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
	return(true);
}
