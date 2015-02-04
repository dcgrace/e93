// Menu handling
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

#define MENUBORDERSIZE				2			// number of pixels of border around menus (not items, but whole menus)

#define	MENUMINITEMCOMMANDSPACING	7			// minimum number of pixels to leave between menu item, and right justified command string
#define	MENUADDITIONALWIDTH			6			// additional pixel width added to each menu item to make it look better when drawn
#define	MENUADDITIONALHEIGHT		6			// additional pixel height added to each menu item to make it look better when drawn

static EDITORMENU
	*firstMenu;

static char localErrorFamily[]="Xguimenus";

enum
{
	BADRELATIONSHIP,
	BADPATH
};

static char *errorMembers[]=
{
	"BadRelationship",
	"BadPath"
};

static char *errorDescriptions[]=
{
	"Invalid menu relationship",
	"Invalid menu path"
};

EDITORMENU *GetRelatedEditorMenu(EDITORMENU *theMenu,int menuRelationship)
// return the menu with menuRelationship to theMenu
// return NULL if there is no menu with menuRelationship
{
	EDITORMENU
		*outputMenu;

	outputMenu=NULL;
	switch(menuRelationship)
	{
		case MR_NEXTSIBLING:
			if(theMenu->parent)
			{
				if(theMenu->nextSibling!=theMenu->parent->firstChild)
				{
					outputMenu=theMenu->nextSibling;
				}
			}
			else
			{
				if(theMenu->nextSibling!=firstMenu)
				{
					outputMenu=theMenu->nextSibling;
				}
			}
			break;
		case MR_PREVIOUSSIBLING:
			if(theMenu->parent)
			{
				if(theMenu!=theMenu->parent->firstChild)
				{
					outputMenu=theMenu->previousSibling;
				}
			}
			else
			{
				if(theMenu!=firstMenu)
				{
					if(theMenu->previousSibling!=firstMenu)
					{
						outputMenu=theMenu->previousSibling;
					}
				}
				else
				{
					outputMenu=firstMenu;
				}
			}
			break;
		case MR_FIRSTCHILD:
			if(theMenu)
			{
				outputMenu=theMenu->firstChild;
			}
			else
			{
				outputMenu=firstMenu;
			}
			break;
		case MR_LASTCHILD:
			if(theMenu)
			{
				if(theMenu->firstChild)
				{
					outputMenu=theMenu->firstChild->previousSibling;
				}
			}
			else
			{
				if(firstMenu)
				{
					outputMenu=firstMenu->previousSibling;
				}
			}
			break;
		default:
			break;
	}
	return(outputMenu);
}

static EDITORWINDOW *GetMenuWindow(EDITORMENU *theMenu)
// return the window that belongs to theMenu if it has one, or NULL
// if it does not
{
	if(theMenu)						// see if menu, or root
	{
		return(theMenu->theWindow);
	}
	return(rootMenuWindow);
}

static void CalculateMenuItemSize(EDITORMENU *theMenu,UINT32 *theWidth,UINT32 *theHeight)
// get the ideal width and height for theMenu
{
	if(theMenu->separator)
	{
		(*theHeight)=2;											// use exact height for separators
		(*theWidth)=MENUADDITIONALWIDTH;
	}
	else
	{
		(*theHeight)=theMenu->theFont->ascent+theMenu->theFont->descent+MENUADDITIONALHEIGHT;	// get the height of the menu item (use total height of the font, plus some slop to make it look better)
		(*theWidth)=XTextWidth(theMenu->theFont->theXFont,theMenu->itemText,strlen(theMenu->itemText))+MENUADDITIONALWIDTH;

		if(theMenu->firstChild)							// if this menu has children, we need to draw the hierarchical right arrow, otherwise, we can use the command string
		{
			(*theWidth)+=MENUMINITEMCOMMANDSPACING;				// add spacing between item and command string
			(*theWidth)+=SUBMENUARROWWIDTH;
			if((*theHeight)<SUBMENUARROWHEIGHT)						// if the font is tiny, then adjust height
			{
				(*theHeight)=SUBMENUARROWHEIGHT;
			}
		}
		else
		{
			if(theMenu->commandString)
			{
				(*theWidth)+=MENUMINITEMCOMMANDSPACING;			// add spacing between item and command string
				(*theWidth)+=XTextWidth(theMenu->theFont->theXFont,theMenu->commandString,strlen(theMenu->commandString));
			}
		}
	}
}

static void PinMenuRect(EDITORRECT *theRect)
// make sure theRect does not go off the screen (as best as possible)
{
	UINT32
		screenWidth,
		screenHeight;

	GetEditorScreenDimensions(&screenWidth,&screenHeight);
	if(theRect->x+theRect->w>screenWidth)
	{
		theRect->x=screenWidth-theRect->w;
	}
	if(theRect->y+theRect->h>screenHeight)
	{
		theRect->y=screenHeight-theRect->h;
	}
	if(theRect->x<0)
	{
		theRect->x=0;
	}
	if(theRect->y<0)
	{
		theRect->y=0;
	}
}

static void CalculateMenuItemRect(EDITORMENU *theMenu,EDITORRECT *theRect)
// get the rect of theItem as it will be drawn into its parent menu
{
	EDITORMENU
		*currentMenu,
		*parentMenu;
	UINT32
		tempWidth,
		tempHeight;
	INT32
		currentY;

	theRect->x=theRect->y=MENUBORDERSIZE;									// offset by border
	theRect->w=theRect->h=0;
	currentY=0;
	parentMenu=theMenu->parent;
	currentMenu=GetRelatedEditorMenu(parentMenu,MR_FIRSTCHILD);
	while(currentMenu)
	{
		CalculateMenuItemSize(currentMenu,&tempWidth,&tempHeight);
		if(tempWidth>theRect->w)
		{
			theRect->w=tempWidth;											// find the maximum width of all the items
		}
		if(currentMenu==theMenu)											// when we get to our menu remember its Y position
		{
			theRect->y+=currentY;
			theRect->h=tempHeight;
		}
		currentY+=tempHeight;
		currentMenu=GetRelatedEditorMenu(currentMenu,MR_NEXTSIBLING);
	}
}

static void CalculateMenuWindowSize(EDITORMENU *theMenu,UINT32 *theWidth,UINT32 *theHeight)
// run though the list of children of theMenu, determining the size of each, use these
// sizes to get the composite size for theMenu
{
	EDITORMENU
		*currentMenu;
	UINT32
		tempWidth,
		tempHeight;

	(*theWidth)=0;
	(*theHeight)=0;
	currentMenu=GetRelatedEditorMenu(theMenu,MR_FIRSTCHILD);
	while(currentMenu)
	{
		CalculateMenuItemSize(currentMenu,&tempWidth,&tempHeight);
		if(tempWidth>(*theWidth))
		{
			(*theWidth)=tempWidth;											// find the maximum width of all the items
		}
		(*theHeight)+=tempHeight;											// and the total height of all items
		currentMenu=GetRelatedEditorMenu(currentMenu,MR_NEXTSIBLING);
	}
	*theWidth+=2*MENUBORDERSIZE;												// fancy border drawn around entire menu
	*theHeight+=2*MENUBORDERSIZE;
}

static void DrawMenuItem(Window xWindow,GC graphicsContext,EDITORMENU *theMenu)
// use this to draw theMenu's item into xWindow
{
	EDITORRECT
		itemRect;
	int
		stringLength,
		stringWidth;
	int
		itemX,
		itemY;
	GUICOLOR
		*background;
	INT32
		xLeft,
		xRight,
		yTop,
		yBottom;

	CalculateMenuItemRect(theMenu,&itemRect);										// get the rectangle for this item
	if(itemRect.w&&itemRect.h)
	{
		background=gray2;

		XSetForeground(xDisplay,graphicsContext,background->theXPixel);
		XFillRectangle(xDisplay,xWindow,graphicsContext,itemRect.x,itemRect.y,itemRect.w,itemRect.h);

		if(theMenu->separator)
		{
			xLeft=itemRect.x;
			xRight=itemRect.x+itemRect.w-1;
			yTop=itemRect.y;
			yBottom=yTop+1;

			XSetForeground(xDisplay,graphicsContext,black->theXPixel);
			XDrawLine(xDisplay,xWindow,graphicsContext,xLeft,yTop,xRight,yTop);
			XSetForeground(xDisplay,graphicsContext,white->theXPixel);
			XDrawLine(xDisplay,xWindow,graphicsContext,xLeft,yBottom,xRight,yBottom);
		}
		else
		{
			stringLength=strlen(theMenu->itemText);
			stringWidth=XTextWidth(theMenu->theFont->theXFont,theMenu->itemText,stringLength);
			itemX=itemRect.x+(MENUADDITIONALWIDTH/2);
			itemY=itemRect.y+(itemRect.h/2)-(theMenu->theFont->ascent+theMenu->theFont->descent)/2+theMenu->theFont->ascent;

			XSetFont(xDisplay,graphicsContext,theMenu->theFont->theXFont->fid);						// point to the current font for this item

			XSetForeground(xDisplay,graphicsContext,black->theXPixel);
			XDrawString(xDisplay,xWindow,graphicsContext,itemX,itemY,theMenu->itemText,stringLength);

			if(theMenu->firstChild)
			{
				itemX=itemRect.x+(itemRect.w-SUBMENUARROWWIDTH-(MENUADDITIONALWIDTH/2));
				itemY=itemRect.y+(itemRect.h-SUBMENUARROWHEIGHT)/2;
				XSetForeground(xDisplay,graphicsContext,gray3->theXPixel);
				XSetBackground(xDisplay,graphicsContext,gray2->theXPixel);
				XCopyPlane(xDisplay,subMenuPixmap,xWindow,graphicsContext,0,0,SUBMENUARROWWIDTH,SUBMENUARROWHEIGHT,itemX,itemY,1);
			}
			else
			{
				if(theMenu->commandString)
				{
					stringLength=strlen(theMenu->commandString);
					stringWidth=XTextWidth(theMenu->theFont->theXFont,theMenu->commandString,stringLength);
					itemX=itemRect.x+(itemRect.w-stringWidth-(MENUADDITIONALWIDTH/2));
					itemY=itemRect.y+(itemRect.h/2)-(theMenu->theFont->ascent+theMenu->theFont->descent)/2+theMenu->theFont->ascent;

					XSetFont(xDisplay,graphicsContext,theMenu->theFont->theXFont->fid);						// point to the current font for this item

					XSetForeground(xDisplay,graphicsContext,gray0->theXPixel);
					XDrawString(xDisplay,xWindow,graphicsContext,itemX+1,itemY+1,theMenu->commandString,stringLength);

					XSetForeground(xDisplay,graphicsContext,white->theXPixel);
					XDrawString(xDisplay,xWindow,graphicsContext,itemX,itemY,theMenu->commandString,stringLength);
				}
			}
			if(theMenu->highlight)
			{
				OutlineShadowRectangle(xWindow,graphicsContext,&itemRect,white,gray0,2);
			}
		}
	}
}

static void DrawMenuWindow(EDITORWINDOW *theWindow)
// draw all of the buttons into theWindow
// also draw the menu border
{
	EDITORMENU
		*theOwner,							// the edit menu that owns this window
		*currentMenu;
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;
	GC
		graphicsContext;
	XWindowAttributes
		theAttributes;
	EDITORRECT
		windowRect;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;				// get the x window information associated with this window
	xWindow=theWindowElement->xWindow;										// get x window for this window
	graphicsContext=theWindowElement->graphicsContext;						// get graphics context for this window
	XSetRegion(xDisplay,graphicsContext,theWindowElement->invalidRegion);	// set clipping to what's invalid

	XGetWindowAttributes(xDisplay,xWindow,&theAttributes);
	windowRect.x=0;
	windowRect.y=0;
	windowRect.w=theAttributes.width;
	windowRect.h=theAttributes.height;
	OutlineShadowRectangle(xWindow,graphicsContext,&windowRect,gray3,gray1,MENUBORDERSIZE);

	theOwner=((GUIMENUWINDOWINFO *)theWindow->windowInfo)->ownerMenu;
	currentMenu=GetRelatedEditorMenu(theOwner,MR_FIRSTCHILD);
	while(currentMenu)
	{
		DrawMenuItem(xWindow,graphicsContext,currentMenu);					// draw the items into the menu
		currentMenu=GetRelatedEditorMenu(currentMenu,MR_NEXTSIBLING);
	}
	XSetClipMask(xDisplay,graphicsContext,None);							// get rid of clip mask
}

void UpdateMenuWindow(EDITORWINDOW *theWindow)
// make sure theWindow is up to date
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;				// get the x window information associated with this menu window
	if(!XEmptyRegion(theWindowElement->invalidRegion))						// see if it has an invalid area
	{
		DrawMenuWindow(theWindow);
		XDestroyRegion(theWindowElement->invalidRegion);					// clear out the invalid region now that the window has been drawn
		theWindowElement->invalidRegion=XCreateRegion();
	}
}

static void ResizeMenuWindow(EDITORMENU *theMenu)
// if theMenu has a window, call this to size and invalidate the window when a child of theMenu
// has changed in some way, or when the menu is created
// if NULL is passed for theMenu, it indicates the root menu window
{
	EDITORWINDOW
		*theWindow;
	WINDOWLISTELEMENT
		*theWindowElement;
	EDITORRECT
		theRect;
	XSizeHints
		theSizeHints;

	theRect.x=theRect.y=0;

	if((theWindow=GetMenuWindow(theMenu)))
	{
		theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
		CalculateMenuWindowSize(theMenu,&theRect.w,&theRect.h);					// get the ideal dimensions of this window (based on its children)
		if(theRect.w==0)														// ### this is because X will not allow me to open windows of 0 width or height!
		{
			theRect.w=1;
		}
		if(theRect.h==0)
		{
			theRect.h=1;
		}

		theSizeHints.flags=PMinSize|PMaxSize;
		theSizeHints.min_width=theRect.w;
		theSizeHints.min_height=theRect.h;
		theSizeHints.max_width=theRect.w;
		theSizeHints.max_height=theRect.h;
		XSetWMNormalHints(xDisplay,theWindowElement->xWindow,&theSizeHints);

		XResizeWindow(xDisplay,theWindowElement->xWindow,theRect.w,theRect.h);	// set the window to the new size, now that child has changed

		InvalidateWindowRect(theWindow,&theRect);								// invalidate the entire contents of the window so it will be redrawn
	}
}

static EDITORMENU *LocateMenuWithKey(EDITORMENU *topLevel,KeySym theKeySym)
// attempt to locate an active menu below topLevel, that has theKeySym defined
// if one can be located, return it, else return NULL
{
	EDITORMENU
		*currentMenu,
		*locatedMenu;

	currentMenu=GetRelatedEditorMenu(topLevel,MR_FIRSTCHILD);
	locatedMenu=NULL;
	while(currentMenu&&!locatedMenu)
	{
		if(!(locatedMenu=LocateMenuWithKey(currentMenu,theKeySym)))			// recurse to try to find child that has this key
		{
			if(currentMenu->hasKey&&currentMenu->active)
			{
				if(currentMenu->theKeySym==theKeySym)					// see if this item has the key
				{
					locatedMenu=currentMenu;
				}
			}
		}
		currentMenu=GetRelatedEditorMenu(currentMenu,MR_NEXTSIBLING);
	}
	return(locatedMenu);
}

static EDITORMENU *LocateKeyPressMenu(XEvent *theEvent)
// return the menu item associated with theEvent, or NULL
// if none
{
	KeySym
		theKeySym;
	EDITORMENU
		*theMenu;
	XKeyEvent
		ourEvent;

	if(theEvent->xkey.state&Mod1Mask)
	{
		ourEvent=*((XKeyEvent *)theEvent);
		ourEvent.state&=(~(Mod1Mask|LockMask));						// remove these from consideration
		ComposeXLookupString(&ourEvent,NULL,0,&theKeySym);
		if(!IsModifierKey(theKeySym))								// do not pass lone modifiers
		{
			if((theMenu=LocateMenuWithKey(NULL,theKeySym)))
			{
				if(!theMenu->firstChild)							// must be leaf on the tree
				{
					return(theMenu);
				}
			}
		}
	}
	return(NULL);
}

static void UnAssignFont(EDITORMENU *theMenu)
// Get rid of font assigned to a menu item
{
	if(theMenu->theFont)
	{
		FreeFont(theMenu->theFont);
		theMenu->theFont=NULL;				// use the default font
	}
}

static void AssignFont(EDITORMENU *theMenu,char *theFontName)
// given a font name, apply it to theMenu
// if the menu already has a font assigned to it, get rid of it
// if there is a problem getting theFontName, use the DEFAULTMENUFONT
{
	if(theMenu->theFont)
	{
		FreeFont(theMenu->theFont);
	}
	if(!(theMenu->theFont=LoadFont(theFontName)))
	{
		if(!(theMenu->theFont=LoadFont(DEFAULTMENUFONT)))
		{
			ReportMessage("Failed to load default menu font!\n");
		}
	}
}

static void UnAssignKeyEquivalent(EDITORMENU *theMenu)
// remove key code assigned to theMenu
{
	if(theMenu->hasKey)
	{
		if(theMenu->commandString)
		{
			MDisposePtr(theMenu->commandString);
			theMenu->commandString=NULL;
		}
		theMenu->hasKey=false;
	}
}

static void AssignKeyEquivalent(EDITORMENU *theMenu,char *theKeyName)
// given a key name, assign that key to theMenu
// if the another menu already has the key assigned to it, get rid of it
// if theKeyName does not map to a valid key, then the menu will have no key equivalent
{
	KeySym
		theKeySym;
	EDITORMENU
		*oldMenu;

	UnAssignKeyEquivalent(theMenu);
	if((theKeySym=XStringToKeysym(theKeyName))!=NoSymbol)
	{
		if((oldMenu=LocateMenuWithKey(NULL,theKeySym)))		// see if another menu item had this key, if so, take it
		{
			UnAssignKeyEquivalent(oldMenu);
			ResizeMenuWindow(oldMenu->parent);				// update the old menu
		}
		theMenu->theKeySym=theKeySym;
		if((theMenu->commandString=(char *)MNewPtr(strlen(theKeyName)+3)))	// make room for <,>, and 0 terminator
		{
			sprintf(theMenu->commandString,"<%s>",theKeyName);			// copy in the command key string
		}
		theMenu->hasKey=true;
	}
}

static void ExtractAttributeParameter(char *parameterInput,UINT32 *parameterIndex,char *parameterBuffer,UINT32 parameterBufferSize)
// extract the next parameter from parameterInput[parameterIndex] into parameter buffer
// if the parameter would be too long, just truncate it
// leave paramterIndex updated to point to the end of the given parameter, no matter
// how long it is
{
	UINT32
		outIndex;
	char
		theChar;
	bool
		done;

	outIndex=0;
	done=false;
	while((theChar=parameterInput[*parameterIndex])&&!done)		// make sure not end of string, or end of parameter
	{
		if(theChar!='\\')
		{
			(*parameterIndex)++;
			if(outIndex<parameterBufferSize)
			{
				parameterBuffer[outIndex++]=theChar;
			}
		}
		else
		{
			if(parameterInput[*parameterIndex+1]=='\\')			// see if quoted separator (not a separator at all then)
			{
				(*parameterIndex)+=2;
				if(outIndex<parameterBufferSize)
				{
					parameterBuffer[outIndex++]='\\';			// place separator into buffer
				}
			}
			else
			{
				done=true;										// ran into end of parameter list
			}
		}
	}
	if(outIndex<parameterBufferSize)
	{
		parameterBuffer[outIndex]='\0';							// terminate buffer
	}
	else
	{
		if(parameterBufferSize)
		{
			parameterBuffer[outIndex-1]='\0';					// terminate buffer by writing over last character
		}
	}
}

static bool RealizeAttributes(EDITORMENU *theMenu)
// Attempt to fill the parts of theMenu's record that are described in its string
// Currently supports:
// \Ffontname		specify the font to use when drawing the item
// \S				specify that this item is a menu separation line (text is not drawn)
// \Kkey			specify that mod1 <key> could be used to access the given menu
// if there is a problem, return false
{
	char
		parameterBuffer[256];							// max size for parameter string, anything larger gets truncated!
	UINT32
		attributeIndex;
	char
		theChar;

	theMenu->separator=false;
	theMenu->hasKey=false;
	theMenu->theKeySym=NoSymbol;
	theMenu->commandString=NULL;						// no command string yet
	theMenu->theFont=NULL;								// no font yet

	attributeIndex=0;
	while((theChar=theMenu->attributes[attributeIndex]))
	{
		if(theChar!='\\')								// eat separators
		{
			ExtractAttributeParameter(theMenu->attributes,&attributeIndex,&(parameterBuffer[0]),256);
			switch(parameterBuffer[0])					// check command part
			{
				case 'F':
					AssignFont(theMenu,&(parameterBuffer[1]));
					break;
				case 'S':
					theMenu->separator=true;
					break;
				case 'K':
					AssignKeyEquivalent(theMenu,&(parameterBuffer[1]));
					break;
				default:
					break;
			}
		}
		else
		{
			attributeIndex++;
		}
	}

	if(!theMenu->theFont)							// if not font specified, use the default
	{
		AssignFont(theMenu,DEFAULTMENUFONT);
	}
	return(true);
}

static void UnRealizeAttributes(EDITORMENU *theMenu)
// undo anything that RealizeAttributes did
{
	UnAssignFont(theMenu);
	UnAssignKeyEquivalent(theMenu);
}

static void SetMenuEnableState(EDITORMENU *theMenu)
// set the active state for theMenu
{
}

static EDITORWINDOW *CreateSubMenuWindow(EDITORMENU *theOwner,EDITORRECT *theRect,char *theTitle)
// this is called to create a sub menu window for theOwner
// sub menu windows do not have normal window decoration
// NOTE: this checks to see if the submenu will go off the screen, and pins it
// into range
{
	EDITORWINDOW
		*theWindow;
	WINDOWLISTELEMENT
		*theWindowElement;
	GUIMENUWINDOWINFO
		*theMenuInfo;
	XSetWindowAttributes
		theAttributes;
	UINT32
		valueMask;
	Atom
		protocols[2];																	// number of protocols

	if((theWindow=(EDITORWINDOW *)MNewPtr(sizeof(EDITORWINDOW))))						// create editor window
	{
		if((theMenuInfo=(GUIMENUWINDOWINFO *)MNewPtr(sizeof(GUIMENUWINDOWINFO))))		// make structure to contain menu window info
		{
			if((theWindowElement=(WINDOWLISTELEMENT *)MNewPtr(sizeof(WINDOWLISTELEMENT))))	// create a window list element
			{
				LinkWindowElement(theWindowElement);									// link it to the global window list that we maintain
				theWindowElement->theEditorWindow=theWindow;							// point to our high level concept of window
				theWindow->windowType=EWT_MENU;											// set the editor window type to "menu"
				theWindow->userData=theWindowElement;									// point back to the window element
				theWindow->windowInfo=theMenuInfo;										// menu window info pointer
				theMenuInfo->ownerMenu=theOwner;										// point at the owner of this window

				theAttributes.save_under=True;
				theAttributes.background_pixmap=None;
				theAttributes.background_pixel=gray2->theXPixel;
				theAttributes.border_pixmap=None;
				theAttributes.border_pixel=black->theXPixel;
				theAttributes.override_redirect=True;
				theAttributes.colormap=xColormap;
				valueMask=CWSaveUnder|CWBackPixmap|CWBackPixel|CWBorderPixmap|CWBorderPixel|CWOverrideRedirect|CWColormap;

				CalculateMenuWindowSize(theOwner,&theRect->w,&theRect->h);

				if(theRect->w==0)														// ### this is because X will not allow me to open windows of 0 width or height!
				{
					theRect->w=1;
				}
				if(theRect->h==0)														// ### this is because X will not allow me to open windows of 0 width or height!
				{
					theRect->h=1;
				}
				PinMenuRect(theRect);													// pin the rectangle to the screen
				// Due to X's nasty error handling, we assume this create does not fail!!!
				theWindowElement->realTopWindow=theWindowElement->xWindow=XCreateWindow(xDisplay,RootWindow(xDisplay,xScreenNum),theRect->x,theRect->y,theRect->w,theRect->h,0,CopyFromParent,InputOutput,DefaultVisual(xDisplay,xScreenNum),valueMask,&theAttributes);

				XStoreName(xDisplay,theWindowElement->xWindow,theTitle);
				XSetIconName(xDisplay,theWindowElement->xWindow,theTitle);
				XSelectInput(xDisplay,theWindowElement->xWindow,ExposureMask|ButtonPressMask|ButtonReleaseMask|KeyPressMask|StructureNotifyMask);	// want to see these events

				protocols[0]=deleteWindowAtom;
				XSetWMProtocols(xDisplay,theWindowElement->xWindow,&(protocols[0]),1);	// tell X we would like to participate in these protocols

				theWindowElement->graphicsContext=XCreateGC(xDisplay,theWindowElement->xWindow,0,NULL);	// create context in which to draw

				theWindowElement->invalidRegion=XCreateRegion();						// create new empty invalid region

				theWindowElement->currentIcon=(Pixmap)NULL;								// no icon for sub-menus
				theOwner->theWindow=theWindow;											// point this menu at its window
				XDefineCursor(xDisplay,theWindowElement->xWindow,arrowCursor);			// this is the cursor to use in the submenu
				XMapRaised(xDisplay,theWindowElement->xWindow);							// map window to the top of the display
				WaitUntilMappedNoFocus(theWindow);										// wait until this window is actually mapped before continuing (olwm likes to mess with the temporal ordering of our windows, this attempts to prevent its inane meddling
				return(theWindow);
			}
			MDisposePtr(theMenuInfo);
		}
		MDisposePtr(theWindow);
	}
	ReportMessage("Failed to create sub menu window\n");
	return((EDITORWINDOW *)NULL);
}

static void DisposeSubMenuWindow(EDITORMENU *theMenu)
// remove the window associated with theMenu
{
	WINDOWLISTELEMENT
		*theWindowElement;

	if(theMenu->theWindow)
	{
		theWindowElement=(WINDOWLISTELEMENT *)theMenu->theWindow->userData;
		XDestroyRegion(theWindowElement->invalidRegion);					// destroy invalid region
		XFreeGC(xDisplay,theWindowElement->graphicsContext);				// get rid of the graphics context for this window
		XDestroyWindow(xDisplay,theWindowElement->xWindow);					// tell x to make the window go away
		WaitUntilDestroyed(theMenu->theWindow);								// wait for it to die
		UnlinkWindowElement(theWindowElement);								// unlink it from our list
		MDisposePtr(theWindowElement);
		MDisposePtr(theMenu->theWindow->windowInfo);						// get rid of any information hanging on this window
		MDisposePtr(theMenu->theWindow);
		theMenu->theWindow=NULL;											// point to no window
	}
}

static EDITORWINDOW *CreateRootMenuWindow(EDITORRECT *theRect,char *theTitle)
// create a window which will be used as the root of a menu tree
// if there is a problem, SetError, return NULL
{
	EDITORWINDOW
		*theWindow;
	WINDOWLISTELEMENT
		*theWindowElement;
	GUIMENUWINDOWINFO
		*theMenuInfo;
	XSetWindowAttributes
		theAttributes;
	UINT32
		valueMask;
	Atom
		protocols[2];																	// number of protocols
	XWMHints
		theHints;																		// hints to tell window manager how to treat these windows
	XSizeHints
		theSizeHints;																	// window size info (used to REALLY convince X that I know where I want my own window)

	if((theWindow=(EDITORWINDOW *)MNewPtr(sizeof(EDITORWINDOW))))						// create editor window
	{
		if((theMenuInfo=(GUIMENUWINDOWINFO *)MNewPtr(sizeof(GUIMENUWINDOWINFO))))		// make structure to contain menu window info
		{
			if((theWindowElement=(WINDOWLISTELEMENT *)MNewPtr(sizeof(WINDOWLISTELEMENT))))	// create a window list element
			{
				LinkWindowElement(theWindowElement);									// link it to the global window list that we maintain
				theWindowElement->theEditorWindow=theWindow;							// point to our high level concept of window
				theWindow->windowType=EWT_MENU;											// set the editor window type to "menu"
				theWindow->userData=theWindowElement;									// point back to the window element
				theWindow->windowInfo=theMenuInfo;										// point to the menu info
				theMenuInfo->ownerMenu=NULL;											// this window is the root, so no menu owns it

				theAttributes.background_pixmap=None;
				theAttributes.background_pixel=gray2->theXPixel;
				theAttributes.border_pixmap=None;
				theAttributes.border_pixel=black->theXPixel;
				theAttributes.colormap=xColormap;
				valueMask=CWBackPixmap|CWBackPixel|CWBorderPixmap|CWBorderPixel|CWColormap;

				CalculateMenuWindowSize(NULL,&theRect->w,&theRect->h);
				if(theRect->w==0)														// ### this is because X will not allow me to open windows of 0 width or height!
				{
					theRect->w=1;
				}
				if(theRect->h==0)														// ### this is because X will not allow me to open windows of 0 width or height!
				{
					theRect->h=1;
				}
				// Due to X's nasty error handling, we assume this create does not fail!!!

				theWindowElement->realTopWindow=theWindowElement->xWindow=XCreateWindow(xDisplay,RootWindow(xDisplay,xScreenNum),theRect->x,theRect->y,theRect->w,theRect->h,0,CopyFromParent,InputOutput,DefaultVisual(xDisplay,xScreenNum),valueMask,&theAttributes);

//				theWindowElement->realTopWindow=theWindowElement->xWindow=XCreateSimpleWindow(xDisplay,RootWindow(xDisplay,xScreenNum),theRect->x,theRect->y,theRect->w,theRect->h,XWINDOWBORDERWIDTH,black->theXPixel,gray2->theXPixel);

				theSizeHints.flags=USSize|USPosition|PMinSize|PMaxSize;					// ### LIE to X, tell it the user requested the size and position so it will honor them
				theSizeHints.x=theRect->x;
				theSizeHints.y=theRect->y;
				theSizeHints.width=theRect->w;
				theSizeHints.height=theRect->h;
				theSizeHints.min_width=1;
				theSizeHints.min_height=1;
				theSizeHints.max_width=1;
				theSizeHints.max_height=1;
				XSetWMNormalHints(xDisplay,theWindowElement->xWindow,&theSizeHints);

				XStoreName(xDisplay,theWindowElement->xWindow,theTitle);
				XSetIconName(xDisplay,theWindowElement->xWindow,theTitle);
				XSelectInput(xDisplay,theWindowElement->xWindow,ExposureMask|ButtonPressMask|ButtonReleaseMask|FocusChangeMask|KeyPressMask|StructureNotifyMask|PropertyChangeMask);	// want to see these events

				protocols[0]=takeFocusAtom;
				protocols[1]=deleteWindowAtom;
				XSetWMProtocols(xDisplay,theWindowElement->xWindow,&(protocols[0]),2);	// tell X we would like to participate in these protocols

				theHints.flags=InputHint|StateHint|WindowGroupHint|IconPixmapHint;
				theHints.input=True;
				theHints.initial_state=NormalState;
				theHints.window_group=theWindowElement->xWindow;
				theHints.icon_pixmap=editorIconPixmap;						// point at application icon
				XSetWMHints(xDisplay,theWindowElement->xWindow,&theHints);

				theWindowElement->currentIcon=theHints.icon_pixmap;			// remember the icon in use

				theWindowElement->graphicsContext=XCreateGC(xDisplay,theWindowElement->xWindow,0,NULL);	// create context in which to draw the text (0 for mask) use default

				theWindowElement->invalidRegion=XCreateRegion();						// create new empty invalid region

				XMapRaised(xDisplay,theWindowElement->xWindow);				// map window to the top of the display
				WaitUntilMapped(theWindow);									// wait until this window is actually mapped before continuing (olwm likes to mess with the temporal ordering of our windows, this attempts to prevent its inane meddling
				return(theWindow);
			}
			MDisposePtr(theMenuInfo);
		}
		MDisposePtr(theWindow);
	}
	ReportMessage("Failed to create menu window\n");
	return((EDITORWINDOW *)NULL);
}

static void DisposeRootMenuWindow(EDITORWINDOW *theWindow)
// close a given root menu window
{
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;			// get the x window associated with this document window
	XDestroyRegion(theWindowElement->invalidRegion);					// destroy invalid region
	XFreeGC(xDisplay,theWindowElement->graphicsContext);				// get rid of the graphics context for this window
	FocusAwayFrom(theWindow);
	XDestroyWindow(xDisplay,theWindowElement->xWindow);					// tell x to make the window go away
	WaitUntilDestroyed(theWindow);										// wait for it to die
	UnlinkWindowElement(theWindowElement);								// unlink it from our list
	MDisposePtr(theWindowElement);
	MDisposePtr(theWindow->windowInfo);
	MDisposePtr(theWindow);
}

void DeactivateEditorMenu(EDITORMENU *theMenu)
// make theMenu inactive
// if theMenu contains sub-Menus, they should be inactive because
// their parent is now inactive, but they should not have their active
// state changed. This way, when the parent becomes active again, the children
// will resume their old states
{
	theMenu->active=false;
	SetMenuEnableState(theMenu);
}

void ActivateEditorMenu(EDITORMENU *theMenu)
// make theMenu active
{
	theMenu->active=true;
	SetMenuEnableState(theMenu);
}

char *GetEditorMenuName(EDITORMENU *theMenu)
// return a pointer to the name of theMenu
{
	return(theMenu->itemText);
}

bool SetEditorMenuName(EDITORMENU *theMenu,char *theName)
// set the name for theMenu
// if this fails, leave the old name, set the error, and return false
{
	char
		*newName;
	if((newName=(char *)MNewPtr(strlen(theName)+1)))
	{
		strcpy(newName,theName);
		MDisposePtr(theMenu->itemText);
		theMenu->itemText=newName;
		return(true);
	}
	return(false);
}

char *GetEditorMenuAttributes(EDITORMENU *theMenu)
// return a pointer to the attributes string for theMenu
{
	return(theMenu->attributes);
}

bool SetEditorMenuAttributes(EDITORMENU *theMenu,char *theAttributes)
// set the attributes for theMenu
// if this fails, set the error, and return false
{
	char
		*newAttributes;

	if((newAttributes=(char *)MNewPtr(strlen(theAttributes)+1)))
	{
		strcpy(newAttributes,theAttributes);
		MDisposePtr(theMenu->attributes);
		theMenu->attributes=newAttributes;
		return(true);
	}
	return(false);
}

char *GetEditorMenuDataText(EDITORMENU *theMenu)
// return a pointer to the data text string for theMenu
{
	return(theMenu->dataText);
}

bool SetEditorMenuDataText(EDITORMENU *theMenu,char *theDataText)
// set the data text for theMenu
// if this fails, leave the data text, set the error, and return false
{
	char
		*newDataText;

	if((newDataText=(char *)MNewPtr(strlen(theDataText)+1)))
	{
		strcpy(newDataText,theDataText);
		MDisposePtr(theMenu->dataText);
		theMenu->dataText=newDataText;
		return(true);
	}
	return(false);
}

bool GetEditorMenu(int argc,char *argv[],EDITORMENU **theMenu)
// traverse the elements of the menu path given by argc,argv
// and attempt to locate the menu that they point to
// if the path is invalid, return false
// otherwise return true with theMenu set to the located item
// NOTE: on entry, theMenu is used as the starting point for
// the search, if it is passed as NULL, it indicates that the search
// should start at the top
{
	EDITORMENU
		*currentMenu,
		*testMenu;
	int
		elementIndex;
	bool
		matching;

	currentMenu=testMenu=*theMenu;
	matching=true;
	for(elementIndex=0;(elementIndex<argc)&&matching;elementIndex++)
	{
		if(currentMenu)							// locate root of current menu
		{
			testMenu=currentMenu->firstChild;
		}
		else
		{
			testMenu=firstMenu;
		}
		matching=false;
		while(testMenu&&!matching)
		{
			if(strcmp(argv[elementIndex],testMenu->itemText)==0)
			{
				matching=true;					// located a match, this is the menu we want
			}
			else
			{
				testMenu=GetRelatedEditorMenu(testMenu,MR_NEXTSIBLING);
			}
		}
		currentMenu=testMenu;
	}
	*theMenu=testMenu;
	return(matching);
}

static bool LinkEditorMenuElement(EDITORMENU *theMenu,EDITORMENU *baseMenu,int menuRelationship)
// link theMenu to baseMenu with menuRelationship
// if baseMenu is NULL, then menuRelationship must be MR_FIRSTCHILD, or MR_LASTCHILD as it is not possible to
// link menus as siblings of the root
// if there is a problem, Set the error, and return false
{
	bool
		fail;

	fail=false;
	theMenu->firstChild=NULL;
	switch(menuRelationship)
	{
		case MR_NEXTSIBLING:
			if(baseMenu)
			{
				theMenu->nextSibling=baseMenu->nextSibling;
				theMenu->nextSibling->previousSibling=theMenu;
				theMenu->previousSibling=baseMenu;
				baseMenu->nextSibling=theMenu;
				theMenu->parent=baseMenu->parent;
			}
			else
			{
				SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
				fail=true;
			}
			break;
		case MR_PREVIOUSSIBLING:
			if(baseMenu)
			{
				theMenu->previousSibling=baseMenu->previousSibling;
				theMenu->previousSibling->nextSibling=theMenu;
				theMenu->nextSibling=baseMenu;
				baseMenu->previousSibling=theMenu;
				theMenu->parent=baseMenu->parent;
				if(theMenu->parent)
				{
					if(theMenu->parent->firstChild==baseMenu)
					{
						theMenu->parent->firstChild=theMenu;		// make us the first child if linked before old first child
					}
				}
				else
				{
					if(firstMenu==baseMenu)
					{
						firstMenu=theMenu;
					}
				}
			}
			else
			{
				SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
				fail=true;
			}
			break;
		case MR_FIRSTCHILD:
			theMenu->parent=baseMenu;									// set parent
			if(baseMenu)													// see if linking as a child of a current menu
			{
				if((theMenu->nextSibling=baseMenu->firstChild))
				{
					theMenu->previousSibling=theMenu->nextSibling->previousSibling;
					theMenu->nextSibling->previousSibling=theMenu;	// link me in as previous sibling of old first child if there was one
					theMenu->previousSibling->nextSibling=theMenu;	// link me in as next sibling
				}
				else
				{
					theMenu->previousSibling=theMenu;					// loop previous sibling to myself
					theMenu->nextSibling=theMenu;						// loop next sibling to myself
				}
				baseMenu->firstChild=theMenu;							// this is now the first child
			}
			else							// creating a child of the universe
			{
				if(firstMenu)
				{
					theMenu->nextSibling=firstMenu;
					theMenu->previousSibling=firstMenu->previousSibling;
					theMenu->previousSibling->nextSibling=theMenu;
					firstMenu->previousSibling=theMenu;
				}
				else
				{
					theMenu->previousSibling=theMenu;
					theMenu->nextSibling=theMenu;
				}
				firstMenu=theMenu;
			}
			break;
		case MR_LASTCHILD:
			theMenu->parent=baseMenu;									// set parent
			if(baseMenu)													// see if linking as a child of a current menu
			{
				if((theMenu->nextSibling=baseMenu->firstChild))
				{
					theMenu->previousSibling=theMenu->nextSibling->previousSibling;
					theMenu->nextSibling->previousSibling=theMenu;	// link me in as previous sibling of old first child if there was one
					theMenu->previousSibling->nextSibling=theMenu;	// link me in as next sibling
				}
				else
				{
					theMenu->previousSibling=theMenu;					// loop previous sibling to myself
					theMenu->nextSibling=theMenu;						// loop next sibling to myself
					baseMenu->firstChild=theMenu;
				}
			}
			else							// creating a child of the universe
			{
				if(firstMenu)
				{
					theMenu->nextSibling=firstMenu;
					theMenu->previousSibling=firstMenu->previousSibling;

					theMenu->previousSibling->nextSibling=theMenu;

					firstMenu->previousSibling=theMenu;
				}
				else
				{
					theMenu->previousSibling=theMenu;
					theMenu->nextSibling=theMenu;
					firstMenu=theMenu;
				}
			}
			break;
		default:
			SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
			fail=true;
			break;
	}
	return(!fail);
}

static void UnlinkEditorMenuElement(EDITORMENU *theMenu)
// unlink theMenu from the menu tree
{
	if(theMenu->parent)
	{
		if(theMenu->parent->firstChild==theMenu)
		{
			if(theMenu->nextSibling!=theMenu)
			{
				theMenu->parent->firstChild=theMenu->nextSibling;
			}
			else
			{
				theMenu->parent->firstChild=NULL;
			}
		}
	}
	else
	{
		if(firstMenu==theMenu)
		{
			if(theMenu->nextSibling!=theMenu)
			{
				firstMenu=theMenu->nextSibling;
			}
			else
			{
				firstMenu=NULL;
			}
		}
	}
	theMenu->nextSibling->previousSibling=theMenu->previousSibling;
	theMenu->previousSibling->nextSibling=theMenu->nextSibling;
}

static EDITORMENU *CreateEditorMenuElement(char *theName,char *theAttributes,char *dataText)
// create an editor menu structure (not linked to anything, but fill in
// theName, and theAttributes)
// if there is a problem, set the error, and return NULL
{
	EDITORMENU
		*theMenu;

	if((theMenu=(EDITORMENU *)MNewPtr(sizeof(EDITORMENU))))						// create editor menu element
	{
		if((theMenu->itemText=(char *)MNewPtr(strlen(theName)+1)))
		{
			strcpy(theMenu->itemText,theName);									// copy the name
			if((theMenu->attributes=(char *)MNewPtr(strlen(theAttributes)+1)))
			{
				strcpy(theMenu->attributes,theAttributes);						// copy the attributes
				if((theMenu->dataText=(char *)MNewPtr(strlen(dataText)+1)))
				{
					strcpy(theMenu->dataText,dataText);							// copy the data text for this menu
					theMenu->active=theMenu->highlight=false;					// item is not active, and not highlighted by default
					theMenu->theWindow=NULL;									// this menu currently has no window
					if(RealizeAttributes(theMenu))								// parse through the attribute string, set up font, and command string
					{
						return(theMenu);
					}
					MDisposePtr(theMenu->dataText);
				}
				MDisposePtr(theMenu->attributes);
			}
			MDisposePtr(theMenu->itemText);
		}
		MDisposePtr(theMenu);
	}
	return(NULL);
}

static void DisposeEditorMenuElement(EDITORMENU *theMenu)
// dispose of an editor menu structure
{
	DisposeSubMenuWindow(theMenu);
	UnRealizeAttributes(theMenu);
	MDisposePtr(theMenu->dataText);
	MDisposePtr(theMenu->attributes);
	MDisposePtr(theMenu->itemText);
	MDisposePtr(theMenu);
}

static EDITORMENU *CreateEditorMenuAtBase(EDITORMENU *baseMenu,int menuRelationship,char *theName,char *theAttributes,char *dataText,bool active)
// create a new menu with theName, as menuRelationship to baseMenu
// and return a pointer to it
// if baseMenu is NULL it indicates the top of the hierarchy
// therefore, menuRelationship must be MR_FIRSTCHILD, or MR_LASTCHILD as it is not possible to
// create menus which are siblings of the root
// theAttributes is a machine specific string that describes machine specific attributes of the menu
// if active is false, the menu should be created inactive
// if there is a problem, set the error, and return NULL
{
	EDITORMENU
		*theMenu;

	if((theMenu=CreateEditorMenuElement(theName,theAttributes,dataText)))
	{
		theMenu->active=active;										// set the active state
		if(LinkEditorMenuElement(theMenu,baseMenu,menuRelationship))
		{
			ResizeMenuWindow(theMenu->parent);						// if the parent of theMenu has a window, then update the window so that this child item appears
			if(theMenu->parent)
			{
				if((theMenu->previousSibling==theMenu)&&(theMenu->nextSibling==theMenu))	// if this is the only child of the parent, then the parent needs to be updated to show he has children
				{
					ResizeMenuWindow(theMenu->parent->parent);			// creating this item has forced my parent to change (to show he has children)
				}
			}
			return(theMenu);
		}
		DisposeEditorMenuElement(theMenu);
	}
	return(NULL);
}

static void InvalidateMenuRect(EDITORMENU *theMenu,EDITORRECT *theRect)
// invalidate the given rectangle of theMenu's window if theMenu has
// a window
// NULL can be passed as theMenu, and indicates the root menu
{
	EDITORWINDOW
		*theWindow;

	if((theWindow=GetMenuWindow(theMenu)))
	{
		InvalidateWindowRect(theWindow,theRect);		// invalidate the rectangle
	}
}

static void InvalidateMenuItem(EDITORMENU *theMenu)
// if theMenu is within a parent window, invalidate the rectangle of
// the parent that corresponds to theMenu
// it is incorrect to call this with theMenu==NULL
{
	EDITORRECT
		theRect;

	CalculateMenuItemRect(theMenu,&theRect);		// find the rectangle of this item within its parent
	InvalidateMenuRect(theMenu->parent,&theRect);	// invalidate this rectangle of the parent's window (if he has one)
}

static void CloseSubMenus(EDITORMENU *theMenu)
// close all of the windows for all inferiors of theMenu
// the close is done depth first
{
	EDITORMENU
		*currentMenu;

	currentMenu=GetRelatedEditorMenu(theMenu,MR_FIRSTCHILD);
	while(currentMenu)
	{
		if(currentMenu->theWindow)
		{
			CloseSubMenus(currentMenu);					// close down any this menu has first
			DisposeSubMenuWindow(currentMenu);
		}
		if(currentMenu->highlight)
		{
			currentMenu->highlight=false;				// unhighlight this menu
			InvalidateMenuItem(currentMenu);
		}
		currentMenu=GetRelatedEditorMenu(currentMenu,MR_NEXTSIBLING);
	}
}

static void OpenSubMenu(EDITORMENU *theMenu)
// if theMenu is hierarchical, call this to open its submenu
// an attempt is made to open the submenu to the right and down of theMenu
// if it does not fit on the screen, some other method should be implemented
{
	EDITORRECT
		theRect;
	EDITORWINDOW
		*theWindow;

	theMenu->highlight=true;								// highlight this menu
	InvalidateMenuItem(theMenu);
	if((theWindow=GetMenuWindow(theMenu->parent))&&theMenu->firstChild)	// find parent's window, make sure this one has children
	{
		CalculateMenuItemRect(theMenu,&theRect);			// get my rectangle within it
		LocalToGlobal(((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,theRect.x+theRect.w,theRect.y,&theRect.x,&theRect.y);	// get global location for new window
		theRect.w=theRect.h=0;								// initialize to zero
		CreateSubMenuWindow(theMenu,&theRect,"submenu");
	}
}

void DisposeEditorMenu(EDITORMENU *theMenu)
// remove theMenu (and any children it may have)
// if NULL is passed, dispose of ALL menus
// NOTE: this calls itself recursively
{
	EDITORMENU
		*theParent,
		*subMenu;

	while((subMenu=GetRelatedEditorMenu(theMenu,MR_LASTCHILD)))	// see if this one has any children
	{
		DisposeEditorMenu(subMenu);
	}
	if(theMenu)
	{
		theParent=theMenu->parent;								// remember the parent, so after this is disposed of, he can be updated
		UnlinkEditorMenuElement(theMenu);						// unlink this item
		DisposeEditorMenuElement(theMenu);						// get rid of it
		if(theParent)											// see if parent is not root
		{
			if(theParent->theWindow&&!theParent->firstChild)	// if the parent has a window, but now has no children, kill the window
			{
				DisposeSubMenuWindow(theParent);				// get rid of the window if it contained the last child
			}
			if(!theParent->firstChild)
			{
				ResizeMenuWindow(theParent->parent);			// tell the parent's parent to resize (to show that there are no children)
			}
		}
		ResizeMenuWindow(theParent);							// if the parent of theMenu has a window, then update the window so that this child item disappears
	}
}

EDITORMENU *CreateEditorMenu(int argc,char *argv[],int menuRelationship,char *theName,char *theAttributes,char *dataText,bool active)
// create a new menu with theName, as menuRelationship to the passed argc,argv path
// the path is absolute within the menu hierarchy
// theAttributes is a machine specific string that describes machine specific attributes of the menu
// if active is false, the menu should be created inactive
// if there is a problem, set the error, and return false
{
	EDITORMENU
		*parentMenu,
		*testMenu;
	bool
		fail;

	parentMenu=NULL;									// start at the top
	if(GetEditorMenu(argc,argv,&parentMenu))			// locate item to link this one to
	{
		testMenu=parentMenu;
		fail=false;
		if(menuRelationship==MR_NEXTSIBLING||menuRelationship==MR_PREVIOUSSIBLING)	// see if hunting for sibling
		{
			if(parentMenu)								// if he wants sibling, better not be at top
			{
				testMenu=testMenu->parent;				// back up one
				if(GetEditorMenu(1,&theName,&testMenu))	// see if an item already exists with this name in the place we are putting it
				{
					if(testMenu!=parentMenu)			// cannot delete the one we want to add to!!
					{
						DisposeEditorMenu(testMenu);	// kill it before adding new item
					}
					else
					{
						SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
						fail=true;
					}
				}
			}
			else
			{
				SetError(localErrorFamily,errorMembers[BADRELATIONSHIP],errorDescriptions[BADRELATIONSHIP]);
				fail=true;
			}
		}
		else
		{
			if(GetEditorMenu(1,&theName,&testMenu))			// see if an item already exists with this name in the place we are putting it
			{
				DisposeEditorMenu(testMenu);				// kill it before adding new item
			}
		}
		if(!fail)
		{
			return(CreateEditorMenuAtBase(parentMenu,menuRelationship,theName,theAttributes,dataText,active));
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[BADPATH],errorDescriptions[BADPATH]);
	}
	return(NULL);
}

static void PressMenuItem(EDITORMENU *theMenu)
// when a menu has been located, do what is needed to highlight it, and also
// if it has a submenu, open it (closing any submenus of any other menus at this level)
{
	if(!theMenu->theWindow)							// if this one does not have a window, close any other windows at this level
	{
		CloseSubMenus(theMenu->parent);				// close any open submenus at this level
		OpenSubMenu(theMenu);						// open, or at least highlight this item
		UpdateEditorWindows();						// force it to update
	}
	else
	{
		if(!theMenu->highlight)						// if it has a window, it may still be unhighlighted, so highlight it
		{
			theMenu->highlight=true;				// highlight this menu
			InvalidateMenuItem(theMenu);
			UpdateEditorWindows();					// force it to update
		}
	}
}

static void UnpressMenuItem(EDITORMENU *theMenu)
// when tracking has left a menu item, call this to unhighlight it
// if an item has submenus open, it is not unpressed
{
	if((!theMenu->theWindow)&&theMenu->highlight)
	{
		theMenu->highlight=false;					// unhighlight this menu
		InvalidateMenuItem(theMenu);
		UpdateEditorWindows();						// force it to update
	}
}

static EDITORMENU *LocateMenuItem(EDITORMENU *theParent,INT32 x,INT32 y)
// locate the item (if any) that X and Y are over in theParent
// X and Y are relative to theParent
{
	EDITORMENU
		*currentMenu;
	EDITORRECT
		theRect;
	bool
		found;

	currentMenu=GetRelatedEditorMenu(theParent,MR_FIRSTCHILD);
	found=false;
	while(currentMenu&&!found)
	{
		CalculateMenuItemRect(currentMenu,&theRect);
		if(currentMenu->active&&!currentMenu->separator&&PointInRECT(x,y,&theRect))
		{
			found=true;
		}
		else
		{
			currentMenu=GetRelatedEditorMenu(currentMenu,MR_NEXTSIBLING);
		}
	}
	return(currentMenu);
}

static bool TrackLocateMenu(EDITORMENU *topLevel,INT32 rootX,INT32 rootY,EDITORMENU **theMenu)
// given an X, and Y that are relative to the X root, run through the open
// menu window list, and return the menu item that the X and Y are over.
// if X,Y are not over any item, return theMenu=NULL
// if false is returned, the given point did not fall in topLevel, or any of its sub windows
{
	EDITORMENU
		*currentMenu;
	INT32
		localX,
		localY;
	bool
		foundHigher;
	EDITORWINDOW
		*theWindow;
	XWindowAttributes
		theAttributes;

	(*theMenu)=NULL;
	foundHigher=false;
	currentMenu=GetRelatedEditorMenu(topLevel,MR_FIRSTCHILD);
	while(currentMenu&&!foundHigher)
	{
		if(currentMenu->theWindow)
		{
			foundHigher=TrackLocateMenu(currentMenu,rootX,rootY,theMenu);
		}
		currentMenu=GetRelatedEditorMenu(currentMenu,MR_NEXTSIBLING);
	}
	if(!foundHigher)
	{
		if((theWindow=GetMenuWindow(topLevel)))
		{
			XGetWindowAttributes(xDisplay,((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,&theAttributes);
			GlobalToLocal(((WINDOWLISTELEMENT *)theWindow->userData)->xWindow,rootX,rootY,&localX,&localY);	// convert global coordinates to those local to this window
			if(localX>=0&&localY>=0&&localX<theAttributes.width&&localY<theAttributes.height)
			{
				(*theMenu)=LocateMenuItem(topLevel,localX,localY);	// see if we can find an item here (either way, we are done because we saw the point in our window)
				foundHigher=true;
			}
		}
	}
	return(foundHigher);
}

static EDITORMENU *MenuWindowButtonPress(EDITORWINDOW *theWindow,XEvent *theEvent)
// handle button press events for menu windows (see what the user is clicking
// on, and return it)
// theWindow is passed in as the root menu window
// NOTE: this does not close submenus when it exits, and it leaves the selected
// menu pressed
{
	WINDOWLISTELEMENT
		*theWindowElement;
	EDITORMENU
		*theMenu,
		*lastMenu;
	Window
		root,
		child;
	int
		rootX,
		rootY,
		windowX,
		windowY,
		lastX,
		lastY;
	unsigned int
		state;
	int
		theButton;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	theButton=theEvent->xbutton.button;
	TrackLocateMenu(NULL,theEvent->xbutton.x_root,theEvent->xbutton.y_root,&theMenu);	// find the menu item (if any) that was clicked on
	lastMenu=theMenu;
	if(theMenu)
	{
		PressMenuItem(theMenu);
	}
	lastX=theEvent->xbutton.x_root;
	lastY=theEvent->xbutton.y_root;
	while(StillDown(theButton,1))
	{
		if(XQueryPointer(xDisplay,theWindowElement->xWindow,&root,&child,&rootX,&rootY,&windowX,&windowY,&state))
		{
			if(lastX!=rootX||lastY!=rootY)							// make sure something has changed
			{
				TrackLocateMenu(NULL,rootX,rootY,&theMenu);
				if(theMenu!=lastMenu)
				{
					if(lastMenu)
					{
						UnpressMenuItem(lastMenu);					// unpress the item that we are leaving
					}
					if(theMenu)
					{
						PressMenuItem(theMenu);						// if a new item was located, press it
					}
					lastMenu=theMenu;
				}
				lastX=rootX;
				lastY=rootY;
			}
		}
		else
		{
			if(lastMenu)
			{
				UnpressMenuItem(lastMenu);						// left the screen, so unpress last
				lastMenu=theMenu=NULL;
			}
		}
	}
	return(theMenu);
}

bool AttemptMenuKeyPress(XEvent *theEvent)
// see if the keypress event can be handled by one of the menu items
// if so, handle it, and return true
// if no menu item wants to handle the key, return false
{
	EDITORMENU
		*theMenu;

	if((theMenu=LocateKeyPressMenu(theEvent)))
	{
		HandleMenuEvent(theMenu);							// call the shell to handle this menu
		return(true);
	}
	return(false);
}

static Bool MenuEventsPredicate(Display *theDisplay,XEvent *theEvent,char *arg)
// match events we want while waiting for menus to be selected
{
	if((theEvent->type==ButtonPress)||(theEvent->type==ButtonRelease)||(theEvent->type==KeyPress))
	{
		return(True);
	}
	return(False);
}

static EDITORMENU *GetFinalMenu(Time theTime)
// user has selected a menu item that has a sub-menu, so, collect and
// process events, attempting to complete the menu selection process
// NOTE: this will not return unless the user cancels (by clicking outside
// the menus), or selects an item which does not have a submenu
{
	EDITORMENU
		*theMenu;
	XEvent
		theEvent;
	bool
		done;

	theMenu=NULL;										// cannot grab pointer, then cancel menu choose
	if(XGrabPointer(xDisplay,RootWindow(xDisplay,xScreenNum),True,ButtonPressMask|ButtonReleaseMask,GrabModeAsync,GrabModeAsync,None,None,theTime)==GrabSuccess)
	{
		done=false;
		while(!done)
		{
			if(XCheckIfEvent(xDisplay,&theEvent,MenuEventsPredicate,NULL))
			{
				switch(theEvent.type)
				{
					case KeyPress:
						if((theMenu=LocateKeyPressMenu(&theEvent)))			// if there is a menu for this key, then get it
						{
							done=(!theMenu->firstChild);
						}
						break;
					case ButtonPress:
						theMenu=MenuWindowButtonPress(rootMenuWindow,&theEvent);	// go handle the button press
						if(!theMenu||!theMenu->firstChild)
						{
							done=true;
						}
						break;
				}
			}
			else
			{
				EditorDoBackground();					// handle background stuff while waiting for menu selection
			}
		}
		XUngrabPointer(xDisplay,CurrentTime);
	}
	return(theMenu);
}

void MenuWindowEvent(EDITORWINDOW *theWindow,XEvent *theEvent)
// handle an event arriving for a menu window
{
	EDITORMENU
		*theMenu;
	WINDOWLISTELEMENT
		*theWindowElement;

	theWindowElement=(WINDOWLISTELEMENT *)theWindow->userData;
	switch(theEvent->type)
	{
		case ButtonPress:
			XRaiseWindow(xDisplay,theWindowElement->xWindow);			// make sure menu is on top
			if((theMenu=MenuWindowButtonPress(theWindow,theEvent)))		// go handle the button press
			{
				if(theMenu->firstChild)									// pressed button that got us to a menu, go try to get final menu press
				{
					theMenu=GetFinalMenu(theEvent->xbutton.time);		// enter event loop waiting for final menu selection, or cancelation
				}
			}
			CloseSubMenus(NULL);										// get rid of sub menus
			if(theMenu)
			{
				UpdateEditorWindows();									// make windows update before handling menu
				HandleMenuEvent(theMenu);								// call the shell to handle this menu
			}
			break;
		case KeyPress:
			// all keys for menus are attempted outside, if we get a keypress, it means we dont want it!
			break;
		case ClientMessage:
			if(theEvent->xclient.data.l[0]==(int)deleteWindowAtom)
			{
				HandleShellCommand("quit",0,NULL);
			}
			break;
		case SelectionRequest:
			HandleSelectionRequest(theEvent);
			break;
		default:
			break;
	}
}

void RaiseRootMenu()
// X specific menu function to pull menus to the top of the window pile
{
	XRaiseWindow(xDisplay,((WINDOWLISTELEMENT *)(rootMenuWindow->userData))->xWindow);	// raise the root menu window to the top
}

bool InitEditorMenus()
// initialize the editor menuing system
// if there is a problem, SetError, return false
{
	EDITORRECT
		theRect;

	firstMenu=NULL;													// no menus exist yet
	theRect.x=10;													// this is the desired place for the main menu window
	theRect.y=20;
	theRect.w=1;													// make tiny window, because we have no menus
	theRect.h=1;
	if((rootMenuWindow=CreateRootMenuWindow(&theRect,programName)))	// attempt to create the root of the menu windows
	{
		return(true);
	}
	return(false);
}

void UnInitEditorMenus()
// uninitialize the editor menuing system
{
	DisposeEditorMenu(NULL);										// get rid of any menus that were created
	DisposeRootMenuWindow(rootMenuWindow);							// get rid of the root of the menu windows
}

static char
	windowID[10];

char *GetMainWindowID()
// get the XWindow ID of the main e93 window,
// in this case the root menu, as a string
// that can be passed as an identifier to TK
{
	WINDOWLISTELEMENT
		*theWindowElement;
	Window
		xWindow;

	theWindowElement = (WINDOWLISTELEMENT *)rootMenuWindow->userData;			// get the x window information associated with this window
	xWindow = theWindowElement->realTopWindow;									// get x window for this window

	sprintf(windowID, "0x%07X", (unsigned int)xWindow);
	return windowID;
}
