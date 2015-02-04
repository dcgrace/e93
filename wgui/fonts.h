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


typedef struct guiFont
{
	UINT32
		usageCount;								// tells how many references there are to this font
	char
		theName[256];							// name of this font
	HFONT
		font;				/* current font for this view */
	UINT32
		minCharWidth,
		ascent,
		descent;
	INT
		charWidths[256];						// cache of widths of all the characters in the font
	UINT32
		maxLeftOverhang,
		maxRightOverhang;
	struct guiFont
		*next,									// used to link the fonts to the global list
		*previous;
} GUIFONT;


void FreeFont(GUIFONT *theFont);
GUIFONT *LoadFont(char *theName);
void PutFontInfo(LOGFONT *lf,char *theFont);
void GetFontInfo(char *theFont,LOGFONT *lf);
void UnInitFonts();
bool InitFonts();
