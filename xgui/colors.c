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

static GUICOLOR
	*topColor;			// holds top font in linked list of gui colors

static bool
	internalColormap;	// tells if e93 is using its own color map, or the system default

void EditorColorToXColor(EDITORCOLOR inColor,XColor *outColor)
// convert an editor color into an X color
{
	UINT32
		red,
		green,
		blue;

	red=(inColor>>16)&0xFF;
	green=(inColor>>8)&0xFF;
	blue=(inColor)&0xFF;
	outColor->red=(red<<8)+red;
	outColor->green=(green<<8)+green;
	outColor->blue=(blue<<8)+blue;
}

void XColorToEditorColor(XColor *inColor,EDITORCOLOR *outColor)
// convert an X color into an editor color
{
	*outColor=((inColor->red>>8)<<16)+((inColor->green>>8)<<8)+((inColor->blue>>8));
}

bool EditorColorNameToColor(char *colorName,EDITORCOLOR *theColor)
// convert a machine specific color name into an editor color
// if the name does not map to a color, return false
// NOTE: it is reasonable for this routine to always return
// false on machines which do not assign names to colors
{
	XColor
		exactResult,
		screenResult;

	if(XLookupColor(xDisplay,xColormap,colorName,&exactResult,&screenResult))
	{
		XColorToEditorColor(&screenResult,theColor);
		return(true);
	}
	return(false);
}

static GUICOLOR *LocateAllocatedColor(EDITORCOLOR theColor)
// see if an already allocated color can be located
// if so, return a pointer to it, otherwise return NULL
// ### this should use a hash table
{
	GUICOLOR
		*testColor;

	testColor=topColor;
	while(testColor&&(testColor->theColor!=theColor))
	{
		testColor=testColor->next;
	}
	return(testColor);
}

static bool GetClosestColor(EDITORCOLOR inColor,Pixel *theXPixel)
// Given an editor color, attempt to allocate the
// closest proximity to it from X
// if there is a problem, SetError, and return false
{
	XColor
		theColor;
	int
		colorSum;
	XColor
		newColor;

	EditorColorToXColor(inColor,&theColor);

	if(XAllocColor(xDisplay,xColormap,&theColor))
	{
		*theXPixel=theColor.pixel;
		return(true);
	}

	// Could not get the requested color, so now find something close :-)
	colorSum=0;
	if(theColor.red>=32768)
	{
		colorSum++;
	}
	if(theColor.green>=32768)
	{
		colorSum++;
	}
	if(theColor.blue>=32768)
	{
		colorSum++;
	}
	if(colorSum>=2)
	{
		newColor.red=newColor.green=newColor.blue=65535;	// get white
	}
	else
	{
		newColor.red=newColor.green=newColor.blue=0;		// get black
	}

	if(XAllocColor(xDisplay,xColormap,&newColor))
	{
		*theXPixel=theColor.pixel;
		return(true);
	}
	return(false);
}

void FreeColor(GUICOLOR *theColor)
// This will attempt to free the color that was allocated using AllocateColor
{
	if(!(--theColor->usageCount))
	{
		XFreeColors(xDisplay,xColormap,&theColor->theXPixel,1,0);
		if(theColor->previous)
		{
			theColor->previous->next=theColor->next;
		}
		else
		{
			topColor=theColor->next;
		}
		if(theColor->next)
		{
			theColor->next->previous=theColor->previous;
		}
		MDisposePtr(theColor);
	}
}

GUICOLOR *AllocColor(EDITORCOLOR inColor)
// Loose indirection of XAllocColor
// This will attempt to get the requested color, and if it cannot be
// allocated, this will return the closest color it can come up with
// (at the moment, this is black, or white!)
{
	GUICOLOR
		*theResult;

	if((theResult=LocateAllocatedColor(inColor)))	// see if we can find this one already loaded
	{
		theResult->usageCount++;				// add another user to this font structure
		return(theResult);
	}
	else
	{
		if((theResult=(GUICOLOR *)MNewPtr(sizeof(GUICOLOR))))	// allocate one
		{
			if(GetClosestColor(inColor,&theResult->theXPixel))
			{
				theResult->theColor=inColor;	// remember what was asked for

				theResult->usageCount=1;

				theResult->previous=NULL;		// link to font list
				if((theResult->next=topColor))
				{
					topColor->previous=theResult;
				}
				topColor=theResult;
				return(theResult);
			}
			MDisposePtr(theResult);
		}
	}
	return(NULL);
}

void UnInitColors()
// undo what InitColors did
{
	FreeColor(gray3);
	FreeColor(gray2);
	FreeColor(gray1);
	FreeColor(gray0);
	FreeColor(white);
	FreeColor(black);

	if(internalColormap)
	{
		XUninstallColormap(xDisplay,xColormap);
		XFreeColormap(xDisplay,xColormap);
	}
}

bool InitColors()
// get colors needed to run the editor
// if there is a fatal problem, SetError, return false
{
	internalColormap=false;
	xColormap=DefaultColormap(xDisplay,xScreenNum);

	if(getenv("E93_COLORMAP"))	// see if we should use our own color map, normally we do not
	{
		xColormap=XCreateColormap(xDisplay,RootWindow(xDisplay,xScreenNum),DefaultVisual(xDisplay,xScreenNum),AllocNone);
		XInstallColormap(xDisplay,xColormap);
		internalColormap=true;
	}

	topColor=NULL;

	if((black=AllocColor(BLACK)))
	{
		if((white=AllocColor(WHITE)))
		{
			if((gray0=AllocColor(GRAY0)))
			{
				if((gray1=AllocColor(GRAY1)))
				{
					if((gray2=AllocColor(GRAY2)))
					{
						if((gray3=AllocColor(GRAY3)))
						{
							return(true);
						}
						FreeColor(gray2);
					}
					FreeColor(gray1);
				}
				FreeColor(gray0);
			}
			FreeColor(white);
		}
		FreeColor(black);
	}
	SetError("Colors","init","Failed to allocate colors");
	return(false);
}

