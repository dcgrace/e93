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
		usageCount;								// tells how many references there are to this color
	EDITORCOLOR
		theColor;
	Pixel
		theXPixel;
	struct guiColor
		*next,									// used to link the fonts to the global list
		*previous;
} GUICOLOR;


void EditorColorToXColor(EDITORCOLOR inColor,XColor *outColor);
void XColorToEditorColor(XColor *inColor,EDITORCOLOR *outColor);
bool EditorColorNameToColor(char *colorName,EDITORCOLOR *theColor);
void FreeColor(GUICOLOR *theColor);
GUICOLOR *AllocColor(EDITORCOLOR inColor);
void UnInitColors();
bool InitColors();
