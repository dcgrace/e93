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



bool CreatePopupMenuAndReturnItem(UINT32 argc,char *argv[],UINT32 *item,bool *itemSelected)
/*
 */
{
	HMENU
		newMenu;
	UINT32
		flags,
		i;
	bool
		fail;
	POINT
		pt;
	MSG
		msg;

	(*itemSelected)=false;
	fail=false;
	if(argc)
	{
		if(newMenu=CreatePopupMenu())
		{
			for(i=0;!fail&&(i<argc);i++)
			{
				flags=MF_BYPOSITION|MF_STRING|MF_ENABLED;
				if((i%32)==0)						// do a menu break every 32
				{
					flags=flags|MF_MENUBREAK;
				}
				if(fail=!InsertMenu(newMenu,0xFFFFFFFF,flags,i+1,argv[i]))
				{
					SetWindowsError();
				}
			}
			if(!fail)
			{
				GetCursorPos(&pt);
				if(TrackPopupMenu(newMenu,TPM_LEFTBUTTON|TPM_LEFTALIGN,pt.x,pt.y,0,frameWindowHwnd,NULL))
				{
				//
				//	TrackPopupMenu posted a WM_COMMAND message if the user selected one of the menu items.
				//
					if(PeekMessage(&msg,frameWindowHwnd,WM_COMMAND,WM_COMMAND,PM_REMOVE))
					{
						(*itemSelected)=true;
						(*item)=LOWORD(msg.wParam);
					}
				}
			}
			DestroyMenu(newMenu);
		}
		else
		{
			fail=true;
			SetWindowsError();
		}
	}
	return(!fail);
}

