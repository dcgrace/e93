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

typedef struct guiColor
{
	UINT32
		colorIndex,
		usageCount;								// tells how many references there are to this color
	EDITORCOLOR
		theColor;
	COLORREF
		colorPaletteIndex;
	HBRUSH
		colorBrush;
	struct guiColor
		*next,									// used to link the fonts to the global list
		*previous;
} GUICOLOR;

bool RegisterNewPaletteEntry(EDITORCOLOR theColor,UINT32 *theIndex);
void UnregisterPaletteEntry(UINT32 index);
void SetColorIndexesForView(EDITORVIEW *theView,UINT32 bitsPerPixel);
void SetViewsColors(EDITORVIEW *theView,EDITORCOLOR foreColor,EDITORCOLOR backColor);
bool EditorColorNameToColor(char *colorName,EDITORCOLOR *theColor);
void FreeColor(GUICOLOR *theColor);
GUICOLOR *AllocColor(EDITORCOLOR inColor);
HBRUSH CreateXORBrush(UINT32 color1,UINT32 color2);
void UninitColors();
bool InitColors();
