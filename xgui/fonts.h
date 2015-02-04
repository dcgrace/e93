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
	XFontStruct
		*theXFont;								// info about currently selected font
	UINT32
		ascent,
		descent;
	UINT16
		charWidths[256];						// cache of widths of all the characters in the font
	UINT32
		maxLeftOverhang;						// gives maximum overhang of any characeter in this font to the left of its origin
	UINT32
		maxRightOverhang;						// gives maximum overhang of any characeter in this font to the right of its width
	struct guiFont
		*next,									// used to link the fonts to the global list
		*previous;
} GUIFONT;


void FreeFont(GUIFONT *theFont);
GUIFONT *LoadFont(char *theName);
void UnInitFonts();
bool InitFonts();
