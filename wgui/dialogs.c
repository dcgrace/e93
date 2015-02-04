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

static char
	*listTitle,
	**listArray;
static UINT32
	listNumItems;
static bool
	*selectedItems;
static int
	*selectedArray;

// Specifying NULL for hwndParent centers hwndChild relative to the screen.
static BOOL CenterWindow(HWND hwndChild,HWND hwndParent)
{
	RECT
		rcChild,
		rcParent;
	int
		cxChild,
		cyChild,
		cxParent,
		cyParent;
	int
		cxScreen,
		cyScreen,
		xNew,
		yNew;
	HDC
		hdc;

	// Get the Height and Width of the child window
	GetWindowRect(hwndChild,&rcChild);
	cxChild=rcChild.right-rcChild.left;
	cyChild=rcChild.bottom-rcChild.top;


	if(hwndParent)
	{
		// Get the Height and Width of the parent window
		GetWindowRect(hwndParent,&rcParent);
		cxParent=rcParent.right-rcParent.left;
		cyParent=rcParent.bottom-rcParent.top;
	}

	else
	{
		cxParent=GetSystemMetrics(SM_CXSCREEN);
		cyParent=GetSystemMetrics(SM_CYSCREEN);
		rcParent.left=0;
		rcParent.top =0;
		rcParent.right=cxParent;
		rcParent.bottom= cyParent;
	}

	// Get the display limits
	hdc=GetDC(hwndChild);
	cxScreen=GetDeviceCaps(hdc,HORZRES);
	cyScreen=GetDeviceCaps(hdc,VERTRES);
	ReleaseDC(hwndChild,hdc);

	// Calculate new X position,then adjust for screen
	xNew=rcParent.left+((cxParent-cxChild)/2);
	if(xNew<0)
	{
		xNew=0;
	}
	else if((xNew+cxChild)>cxScreen)
	{
		xNew=cxScreen-cxChild;
	}

	// Calculate new Y position,then adjust for screen
	yNew=rcParent.top  +((cyParent-cyChild)/2);
	if(yNew<0)
	{
		yNew=0;
	}
	else if((yNew+cyChild)>cyScreen)
	{
		yNew=cyScreen-cyChild;
	}

	// Set it, and return
	return(SetWindowPos(hwndChild,NULL,xNew,yNew,0,0,SWP_NOSIZE|SWP_NOZORDER));
}

BOOL CALLBACK ListBoxHandler(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
/*	Message handler for the ListDialog box.
 */
{
	BOOL
		result;
	UINT32
		itemNum,
		itemsSelected,
		i;
    HWND
    	hwndList;

	result=FALSE;

    switch(msg)
	{
		case WM_INITDIALOG:
		{
			SetWindowText(hwnd,listTitle);		/* set the title of the dialog box */
			hwndList=GetDlgItem(hwnd,IDC_LIST);	/* get the window handle of the list box */
			for(i=0;i<listNumItems;i++)			/* add all the items to the box */
            {
				itemNum=SendMessage(hwndList,LB_ADDSTRING,0,(LPARAM)listArray[i]);
				SendMessage(hwndList,LB_SETITEMDATA,itemNum,(LPARAM)i);
				if(selectedItems[i])			/* see if the item should be selected */
				{
					SendMessage(hwndList,LB_SETSEL,TRUE,(LPARAM)itemNum);	/* yes,mark it selected */
				}
			}
			result=TRUE;
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wparam))
			{
				case IDC_LIST:
				{
					if(HIWORD(wparam)!=LBN_DBLCLK)
					{
						break;		/* if the user didn't double click,break out of the case statement */
					}
				}				/* else, fall through as if the user click the Ok button, THESE TWO CASE ITEM MUST STAY TOGETHER */
				case IDOK:
				{
		            for(i=0;i<listNumItems;i++)	/* first mark every item not selected */
		            {
 		          		selectedItems[i]=false;
					}
					hwndList=GetDlgItem(hwnd,IDC_LIST);
					/* get the number of items selected,and an array of those items */
					itemsSelected=SendMessage(hwndList,LB_GETSELITEMS,listNumItems,(LPARAM)(LPINT)selectedArray);
		            for(i=0;i<itemsSelected;i++)	/* for each item Windows said were selected,get our item number */
		            {
						itemNum=SendMessage(hwndList,LB_GETITEMDATA,selectedArray[i],0);
 		          		selectedItems[itemNum]=true;	/* and mark it selected in our array */
					}
					EndDialog(hwnd,IDOK);
					result=TRUE;
					break;
				}
				case IDCANCEL:
				{
					EndDialog(hwnd,IDCANCEL);
					result=TRUE;
					break;
				}
			}
	    	break;
		}
   	}
    return result;
}

bool SimpleListBoxDialog(char *theTitle,UINT32 numElements,char **listElements,bool *selectedElements,bool *cancel)
/* open a list selection dialog box with two buttons(ok,cancel)
 * The list dialog will be used to modify the selectionListArray, to tell
 * which items were selected,and which were not
 * if there is a problem, SetError, return FALSE
 * otherwise, if cancel is TRUE, the user cancelled
 * otherwise, selectedElements is modified to reflect which items
 * have been selected
 */
{
	bool
		result;
	int
		buttonType;

	(*cancel)=false;
	listTitle=theTitle;
	listNumItems=numElements;
	listArray=listElements;
	selectedItems=selectedElements;
	if(selectedArray=(int *)MNewPtr(sizeof(int)*numElements))
	{
		if((buttonType=DialogBox(programInstanceHandle,"LISTBOXDIALOG",frameWindowHwnd,(DLGPROC)ListBoxHandler))!=-1)
		{
			(*cancel)=(buttonType==IDCANCEL);
			result=true;
		}
		else
		{
			SetWindowsError();
		}
		MDisposePtr(selectedArray);
	}
	return result;
}

void ReportMessage(char *format,...)
/* bring up an ok dialog, with theText
 * wait for the user to click on ok
 */
{
	va_list
		args;
	char
		tempString[2048];						/* this must be large in case the string is huge!(sprintf does not allow us to limit it) */

	va_start(args,format);
	vsprintf(tempString,format,args);			/* make the string */
	va_end(args);

	MessageBox(NULL,tempString,_pgmptr,MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND|MB_TASKMODAL);
}

void PrintMessage(FILE *stream, char *format,...)
{
	va_list
		args;
	char
		tempString[2048];						/* this must be large in case the string is huge!(sprintf does not allow us to limit it) */

	va_start(args,format);
	vsprintf(tempString,format,args);			/* make the string */
	va_end(args);

	MessageBox(NULL,tempString,_pgmptr,MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND|MB_TASKMODAL);
}

bool OkDialog(char *theText)
/* bring up an ok dialog, with theText
 * wait for the user to click on ok,then return
 * if there is a problem, SetError, and return FALSE
 */
{
	if(MessageBox(frameWindowHwnd,theText,_pgmptr,MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND|MB_APPLMODAL))
	{
		return(true);
	}
	SetWindowsError();
	return(false);
}

bool YesNoCancelDialog(char *theText,bool *yes,bool *cancel)
/* bring up a yes, no, or cancel dialog, with theText
 * if there is a problem, SetError, return FALSE
 * otherwise, if cancel is TRUE the user cancelled,
 * otherwise, if yes is TRUE, yes was hit,
 * otherwise no was hit.
 */
{
	int
		buttonType;

	(*yes)=false;
	(*cancel)=false;

	if(buttonType=MessageBox(frameWindowHwnd,theText,_pgmptr,MB_YESNOCANCEL|MB_ICONQUESTION|MB_SETFOREGROUND|MB_APPLMODAL))
	{
		switch(buttonType)
		{
			case IDYES:
			{
				(*yes)=true;
				break;
			}
			case IDNO:
			{
				break;
			}
			case IDCANCEL:
			{
				(*cancel)=true;
				break;
			}
		}
		return(true);
	}
	SetWindowsError();
	return(false);
}

bool OkCancelDialog(char *theText,bool *cancel)
/* bring up an ok/cancel dialog,with theText
 * if there is a problem, SetError, return FALSE
 * otherwise, return TRUE in cancel if the user
 * cancelled, FALSE if not
 */
{
	int
		buttonType;

	(*cancel)=false;
	if(buttonType=MessageBox(frameWindowHwnd,theText,_pgmptr,MB_OKCANCEL|MB_ICONQUESTION|MB_SETFOREGROUND|MB_APPLMODAL))
	{
		(*cancel)=(buttonType==IDCANCEL);
		return(true);
	}
	SetWindowsError();
	return(false);
}


static char
	*simpleTitle,
	*simpleText;
static UINT32
	simpleTextMaxLen;

BOOL PASCAL SimpleTextHandler(HWND hwnd,unsigned msg,WPARAM wparam,LPARAM lparam)
/*	Message handler for the SimpleTextDialog box.
 */
{
	BOOL
		result;

	result=FALSE;

    switch(msg)
	{
		case WM_INITDIALOG:
		{
			SetWindowText(hwnd,simpleTitle);
			SetDlgItemText(hwnd,IDC_SIMPLETEXT_EDIT,simpleText);
			result=true;
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wparam))
			{
				case IDOK:
				{
					GetDlgItemText(hwnd,IDC_SIMPLETEXT_EDIT,simpleText,simpleTextMaxLen);
					EndDialog(hwnd,IDOK);
					result=true;
					break;
				}
				case IDCANCEL:
				{
					EndDialog(hwnd,IDCANCEL);
					result=true;
					break;
				}
			}
	    	break;
		}
   	}
    return result;
}

bool GetSimpleTextDialog(char *theTitle,char *enteredText,UINT32 stringBytes,bool *cancel)
/* open a string input dialog with theTitle,
 * if there is a problem,SetError, return FALSE
 * otherwise, if cancel is TRUE, the user cancelled
 * otherwise, enteredText will be set to the text that the user entered in the box
 * if enteredText is set to something on entry, it will be placed in the box
 */
{
	int
		buttonType;

	(*cancel)=false;
	simpleTitle=theTitle;
	simpleText=enteredText;
	simpleTextMaxLen=stringBytes;
	if((buttonType=DialogBox(programInstanceHandle,"TEXTDIALOG",frameWindowHwnd,(DLGPROC)SimpleTextHandler))!=-1)
	{
		(*cancel)=(buttonType==IDCANCEL);
		RemoveCRFromString(simpleText);
		return(true);
	}
	SetWindowsError();
	return(false);
}

#define	MAXFRTEXTLEN	4096

static bool
	*FRbackwards,
	*FRwrapAround,
	*FRselectionExpr,
	*FRlimitScope,
	*FRreplaceProc,
	*FRignoreCase;
static char
	*FRFindText,
	*FRReplaceText;
static UINT32
	FRMaxTextLen;

BOOL PASCAL SearchReplaceHandler(HWND hwnd,unsigned msg,WPARAM wparam,LPARAM lparam)
{
	BOOL
		result;

	result=FALSE;

    switch(msg)
	{
		case WM_INITDIALOG:
		{
			SetDlgItemText(hwnd,IDC_EDITFIND,FRFindText);
			SetDlgItemText(hwnd,IDC_EDITREPLACE,FRReplaceText);
			CheckDlgButton(hwnd,IDC_CBBACKWARD,(*FRbackwards));
			CheckDlgButton(hwnd,IDC_CBWRAP,(*FRwrapAround));
			CheckDlgButton(hwnd,IDC_CBREGEXP,(*FRselectionExpr));
			CheckDlgButton(hwnd,IDC_CBCASE,(*FRignoreCase));
			CheckDlgButton(hwnd,IDC_CBSCOPE,(*FRlimitScope));
			CheckDlgButton(hwnd,IDC_CBTCL,(*FRreplaceProc));
			result=TRUE;
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wparam))
			{
				case IDC_FINDBUTTON:
				case IDC_FINDALLBUTTON:
				case IDC_REPLACEBUTTON:
				case IDC_REPLACEALLBUTTON:
				{
					GetDlgItemText(hwnd,IDC_EDITFIND,FRFindText,FRMaxTextLen);
					GetDlgItemText(hwnd,IDC_EDITREPLACE,FRReplaceText,FRMaxTextLen);
					EndDialog(hwnd,wparam);
					result=TRUE;
					break;
				}
				case IDCANCEL:
				{
					EndDialog(hwnd,LOWORD(wparam));
					result=TRUE;
					break;
				}
				case IDC_CBBACKWARD:
				{
					(*FRbackwards)=(IsDlgButtonChecked(hwnd,IDC_CBBACKWARD)!=0);
					result=TRUE;
					break;
				}
				case IDC_CBWRAP:
				{
					(*FRwrapAround)=(IsDlgButtonChecked(hwnd,IDC_CBWRAP)!=0);
					result=TRUE;
					break;
				}
				case IDC_CBREGEXP:
				{
					(*FRselectionExpr)=(IsDlgButtonChecked(hwnd,IDC_CBREGEXP)!=0);
					result=TRUE;
					break;
				}
				case IDC_CBCASE:
				{
					(*FRignoreCase)=(IsDlgButtonChecked(hwnd,IDC_CBCASE)!=0);
					result=TRUE;
					break;
				}
				case IDC_CBSCOPE:
				{
					(*FRlimitScope)=(IsDlgButtonChecked(hwnd,IDC_CBSCOPE)!=0);
					result=TRUE;
					break;
				}
				case IDC_CBTCL:
				{
					(*FRreplaceProc)=(IsDlgButtonChecked(hwnd,IDC_CBTCL)!=0);
					result=TRUE;
					break;
				}
			}
	    	break;
		}
   	}
    return result;
}

bool SearchReplaceDialog(EDITORBUFFER *searchUniverse,EDITORBUFFER *replaceUniverse,bool *backwards,bool *wrapAround,bool *selectionExpr,bool *ignoreCase,bool *limitScope,bool *replaceProc,UINT16 *searchType,bool *cancel)
/* open the search/replace dialog
 * if there is a problem, SetError, return FALSE
 * otherwise, if cancel is TRUE, the user cancelled
 * otherwise,
 * return the enumerated mode that the user chose, as well as the states of
 * backwards, wrapAround, selectionExpr, ignoreCase, limitScope, replaceProc
 */
{
	bool
		result;
	CHUNKHEADER
		*theChunk;
	UINT32
		bytesToExtract,
		theOffset;
	INT16
		dlgButton;
	char
		tmpText[MAXFRTEXTLEN+1],
		findText[MAXFRTEXTLEN*2+1],
		replaceText[MAXFRTEXTLEN*2+1];

	result=false;
	(*cancel)=false;
	FRbackwards=backwards;
	FRwrapAround=wrapAround;
	FRselectionExpr=selectionExpr;
	FRignoreCase=ignoreCase;
	FRlimitScope=limitScope;
	FRreplaceProc=replaceProc;
	FRFindText=findText;
	FRReplaceText=replaceText;
	FRMaxTextLen=MAXFRTEXTLEN;

	theChunk=searchUniverse->textUniverse->firstChunkHeader;
	theOffset=0;
	bytesToExtract=Min(MAXFRTEXTLEN,searchUniverse->textUniverse->totalBytes);
	if(ExtractUniverseText(searchUniverse->textUniverse,theChunk,theOffset,(UINT8 *)tmpText,bytesToExtract,&theChunk,&theOffset))	/* read data from universe into buffer */
	{
		tmpText[bytesToExtract]='\0';
		CopyStringAndAddCRToLF(tmpText,findText,MAXFRTEXTLEN*2);

		theChunk=replaceUniverse->textUniverse->firstChunkHeader;
		theOffset=0;
		bytesToExtract=Min(MAXFRTEXTLEN,replaceUniverse->textUniverse->totalBytes);
		if(ExtractUniverseText(replaceUniverse->textUniverse,theChunk,theOffset,(UINT8 *)tmpText,bytesToExtract,&theChunk,&theOffset))	/* read data from universe into buffer */
		{
			tmpText[bytesToExtract]='\0';
			CopyStringAndAddCRToLF(tmpText,replaceText,MAXFRTEXTLEN*2);

			if((dlgButton=DialogBox(programInstanceHandle,"FINDREPLACEDIALOG",frameWindowHwnd,(DLGPROC)SearchReplaceHandler))!=-1)
			{
				switch(dlgButton)
				{
					case IDC_FINDBUTTON:
					{
						(*searchType)=ST_FIND;
						RemoveCRFromString(findText);
						RemoveCRFromString(replaceText);
						EditorStartReplace(searchUniverse);
						result=ReplaceEditorText(searchUniverse,0,searchUniverse->textUniverse->totalBytes,(UINT8 *)findText,strlen(findText));
						EditorEndReplace(searchUniverse);
						if(result)
						{
							EditorStartReplace(replaceUniverse);
							result=ReplaceEditorText(replaceUniverse,0,replaceUniverse->textUniverse->totalBytes,(UINT8 *)replaceText,strlen(replaceText));
							EditorEndReplace(replaceUniverse);
						}
						break;
					}
					case IDC_FINDALLBUTTON:
					{
						(*searchType)=ST_FINDALL;
						RemoveCRFromString(findText);
						RemoveCRFromString(replaceText);
						EditorStartReplace(searchUniverse);
						result=ReplaceEditorText(searchUniverse,0,searchUniverse->textUniverse->totalBytes,(UINT8 *)findText,strlen(findText));
						EditorEndReplace(searchUniverse);
						if(result)
						{
							EditorStartReplace(replaceUniverse);
							result=ReplaceEditorText(replaceUniverse,0,replaceUniverse->textUniverse->totalBytes,(UINT8 *)replaceText,strlen(replaceText));
							EditorEndReplace(replaceUniverse);
						}
						break;
					}
					case IDC_REPLACEBUTTON:
					{
						(*searchType)=ST_REPLACE;
						RemoveCRFromString(findText);
						RemoveCRFromString(replaceText);
						EditorStartReplace(searchUniverse);
						result=ReplaceEditorText(searchUniverse,0,searchUniverse->textUniverse->totalBytes,(UINT8 *)findText,strlen(findText));
						EditorEndReplace(searchUniverse);
						if(result)
						{
							EditorStartReplace(replaceUniverse);
							result=ReplaceEditorText(replaceUniverse,0,replaceUniverse->textUniverse->totalBytes,(UINT8 *)replaceText,strlen(replaceText));
							EditorEndReplace(replaceUniverse);
						}
						break;
					}
					case IDC_REPLACEALLBUTTON:
					{
						(*searchType)=ST_REPLACEALL;
						RemoveCRFromString(findText);
						RemoveCRFromString(replaceText);
						EditorStartReplace(searchUniverse);
						result=ReplaceEditorText(searchUniverse,0,searchUniverse->textUniverse->totalBytes,(UINT8 *)findText,strlen(findText));
						EditorEndReplace(searchUniverse);
						if(result)
						{
							EditorStartReplace(replaceUniverse);
							result=ReplaceEditorText(replaceUniverse,0,replaceUniverse->textUniverse->totalBytes,(UINT8 *)replaceText,strlen(replaceText));
							EditorEndReplace(replaceUniverse);
						}
						break;
					}
					case IDCANCEL:
					{
						result=true;
						(*cancel)=true;
						RemoveCRFromString(findText);
						RemoveCRFromString(replaceText);
						EditorStartReplace(searchUniverse);
						result=ReplaceEditorText(searchUniverse,0,searchUniverse->textUniverse->totalBytes,(UINT8 *)findText,strlen(findText));
						EditorEndReplace(searchUniverse);
						if(result)
						{
							EditorStartReplace(replaceUniverse);
							result=ReplaceEditorText(replaceUniverse,0,replaceUniverse->textUniverse->totalBytes,(UINT8 *)replaceText,strlen(replaceText));
							EditorEndReplace(replaceUniverse);
						}
						break;
					}
				}
			}
			else
			{
				SetWindowsError();
			}
		}
	}
	return result;
}

void FreeOpenFileDialogPaths(char **thePaths)
{
	FreeStringList(thePaths);
}

static bool ParseWindowsMultiFileNames(char *filesString,char ***pathArray)
{
	UINT32
		numElements;
	char
		tmp[MAX_PATH*2],
		*path;
	bool
		fail;


	fail=false;
	if((*pathArray)=NewStringList())
	{
		numElements=0;

		path=filesString;
		if(!NextStringArgument(&path))
		{
			fail=!AddStringToList(filesString,pathArray,&numElements);
		}
		else
		{
			path=filesString;
			while(!fail&&NextStringArgument(&filesString))
			{
				strcpy(tmp,path);
				if(path[strlen(path)-1]!='/')
				{
					strcat(tmp,"/");
				}
				strcat(tmp,filesString);
				fail=!AddStringToList(tmp,pathArray,&numElements);
			}
		}
		if(fail)
		{
			FreeStringList((*pathArray));
		}
		return(!fail);
	}
	return(false);
}

static BOOL APIENTRY CommonDialogHookProc(HWND hDlg,UINT message,UINT wParam,INT32 lParam)
{
    switch(message)
	{
		case WM_NOTIFY:
		{
			switch(((NMHDR *) lParam)->code)
			{
				case CDN_INITDONE:
				{
					CenterWindow(GetParent(hDlg),NULL);
					break;
				}
			}
			break;
		}
	}
	return(FALSE);
}

static BOOL APIENTRY OpenDialogHookProc(HWND hDlg,UINT message,UINT wParam,INT32 lParam)
{
	HWND
		ctrl,
		parent;

    switch(message)
	{
		case WM_INITDIALOG:
		{
			parent=GetParent(hDlg);
			ctrl=GetDlgItem(parent,lst1);
			EnableWindow(ctrl,TRUE);
			break;
		}
		case WM_NOTIFY:
		{
			switch(((NMHDR *) lParam)->code)
			{
				case CDN_INITDONE:
				{
					CenterWindow(GetParent(hDlg),NULL);
					parent=GetParent(hDlg);
					ctrl=GetDlgItem(parent,lst1);
					EnableWindow(ctrl,TRUE);
					break;
				}
			}
			break;
		}
	}
	return(FALSE);
}

static char
	szFile[65535],			// a buffer to hold file names TODO: malloc this
	initDir[MAX_PATH + 1],
	szFilter[MAX_PATH * 2];	// TODO: nothing magic about MAX_PATH*2

static OPENFILENAME
	ofn;

static bool
	inited = false;
	
static bool InitOpenFileStruct(const char *theTitle)
{
	szFile[0]='\0';
	ofn.Flags=OFN_ENABLEHOOK|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_EXPLORER|OFN_LONGNAMES;

	if (!inited)
	{
		char
			chReplace;
		int
			i,
			cbString;
			
		cbString=LoadString(programInstanceHandle,IDS_FILTERSTRING,szFilter,sizeof(szFilter));
		chReplace=szFilter[cbString-1];
		for(i=0;szFilter[i]!='\0';i++)
		{
			if(szFilter[i]==chReplace)
			{
				szFilter[i]='\0';
			}
		}
		
		ofn.lStructSize=sizeof(OPENFILENAME);
		ofn.hwndOwner=frameWindowHwnd;
		ofn.hInstance=programInstanceHandle;
		ofn.lpstrFilter=szFilter;
		ofn.nFilterIndex=1;
		ofn.lpstrFile=szFile;
		ofn.nMaxFile=sizeof(szFile)-1;
		ofn.lpstrFileTitle=NULL;
		ofn.nMaxFileTitle=0;
		ofn.lpfnHook=(LPOFNHOOKPROC)CommonDialogHookProc;
		
		inited=true;
	}
	
	_getcwd(initDir,_MAX_PATH);
	ofn.lpstrInitialDir=initDir;
	ofn.lpstrTitle=theTitle;
	
	return true;
}

static void UninitOpenFileStruct()
{
}

bool OpenFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,char ***listElements,bool *cancel)
/* bring up a file open dialog
 * if there is a problem, SetError, return FALSE
 * otherwise, cancel is TRUE if the user cancelled the dialog
 *
 * otherwise, listElements contains the pointer to the start of an array of
 * pointers to char which are the selections
 * it must be disposed of at some later time with a call to FreeOpenFileDialogPaths
 * NOTE: the array is terminated with a pointer to NULL
 *
 */
{
	char
		**pathArray;
	bool
		result = false;

	(*cancel)=false;

	if (InitOpenFileStruct(theTitle))
	{
		ofn.Flags |= OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_ALLOWMULTISELECT;

		if(strlen(fullPath) && (strlen(fullPath) <= _MAX_PATH))
		{
			int
				i = 0;
			
			while (fullPath[i]!='\0')
			{
				initDir[i] = fullPath[i];
				if (initDir[i] == '/')
				{
					initDir[i] = '\\';
				}
				i++;
			}
			initDir[i] = '\0';
		}

		if(GetOpenFileName(&ofn))
		{
			ConvertBSlashToFSlash(ofn.lpstrFile);
			if(ParseWindowsMultiFileNames(ofn.lpstrFile,&pathArray))
			{
				(*listElements)=pathArray;
				result=true;
			}
		}
		else
		{
			if(CommDlgExtendedError()==0)
			{
				result=true;
				(*cancel)=true;
			}
			else
			{
				SetCommDlgError();
			}
		}
		UninitOpenFileStruct();
	}
	
	return result;
}

void ExtractFilenameFromFullpath(const char *fullPath,char *filename,char *path)
/*	Passed in a full DOS pathname, extract the filename out of it
 */
{
	char
		drive[_MAX_DRIVE],
		dir[_MAX_DIR],
		ext[_MAX_EXT];

	_splitpath(fullPath, drive, dir, filename, ext);
	strcat(filename, ext);
	
	if (path != NULL)
	{
		strcpy(path, drive);
		strcat(path, dir);
	}
}

bool SaveFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel)
/* bring up a file save dialog
 * if there is a problem,SetError,return false
 * otherwise, if cancel is true,the user cancelled
 * otherwise, return the full path name of the chosen file
 * NOTE: if the fullPath(including terminator) would be larger than stringBytes,
 * then it will be truncated to fit
 * NOTE ALSO: fullPath may be sent to this routine with a pathname, and the save
 * dialog will open to that path(as best as it can)
 */
{
	bool
		result = false;

	(*cancel)=false;

	
	if (InitOpenFileStruct(theTitle))
	{
		ExtractFilenameFromFullpath(fullPath, ofn.lpstrFile, NULL);

		if (GetSaveFileName(&ofn))
		{
			if ((strlen(ofn.lpstrFile) + 1) <= stringBytes)	// make sure the filename can be copied
			{
				strcpy(fullPath, ofn.lpstrFile);
				ConvertBSlashToFSlash(fullPath);
				result = true;
			}
		}
		else
		{
			if (CommDlgExtendedError()==0)
			{
				result = true;
				(*cancel) = true;
			}
			else
			{
				SetCommDlgError();
			}
		}
		UninitOpenFileStruct();
	}
	
	return result;
}

BOOL APIENTRY ChoosePathDialogHookProc(HWND hDlg,UINT message,UINT wParam,INT32 lParam)
{
	HWND
		parent;

    switch(message)
	{
		case WM_INITDIALOG:
		{
			parent=GetParent(hDlg);
			CommDlg_OpenSave_HideControl(parent,edt1);
			CommDlg_OpenSave_HideControl(parent,cmb1);
			CommDlg_OpenSave_HideControl(parent,stc2);
			CommDlg_OpenSave_HideControl(parent,stc3);
			CommDlg_OpenSave_SetControlText(parent,stc4,"Path:");
			CommDlg_OpenSave_SetControlText(parent,IDOK,"OK");
			break;
		}
		case WM_NOTIFY:
		{
			switch(((NMHDR *) lParam)->code)
			{
				case CDN_INITDONE:
				{
					CenterWindow(GetParent(hDlg),NULL);
					break;
				}
			}
			break;
		}
	}
	return(false);
}

bool ChoosePathDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel)
/* bring up a path choose dialog
 * if there is a problem, SetError,return FALSE
 * otherwise, if cancel is TRUE,the user cancelled
 * otherwise, return the full path name of the chosen path
 * NOTE: if the fullPath(including terminator) would be larger than stringBytes,
 * then it will be truncated to fit
 * NOTE ALSO: fullPath may be sent to this routine with a pathname, and the
 * dialog will open to that path(as best as it can)
 */
{
	OPENFILENAME
		gddOfn;
	bool
		result = false;

	(*cancel)=false;

	strcpy(szFile,"test.\040\040\061\0");				// give open a name so it will let us open
	gddOfn.lStructSize=sizeof(OPENFILENAME);
	gddOfn.hwndOwner=frameWindowHwnd;
	gddOfn.hInstance=programInstanceHandle;
	gddOfn.lpstrFilter="t\0*.\0177\0177\0177\0\0";		// set extension filter to .7f7f7f ie have it show no files, only folders
	gddOfn.lpstrCustomFilter=(LPSTR)NULL;
	gddOfn.nMaxCustFilter=0L;
	gddOfn.nFilterIndex=0;
	gddOfn.lpstrFile=szFile;
	gddOfn.nMaxFile=sizeof(szFile)-1;
	gddOfn.lpstrFileTitle=NULL;
	gddOfn.nMaxFileTitle=0;
	gddOfn.lpstrTitle=theTitle;
	_getcwd(initDir,_MAX_PATH);
	gddOfn.lpstrInitialDir=initDir;
	
	if(strlen(fullPath))
	{
		gddOfn.lpstrInitialDir=fullPath;
	}

	gddOfn.nFileOffset=0;
	gddOfn.nFileExtension=0;
	gddOfn.lpstrDefExt=NULL;
	gddOfn.lCustData=0;

	gddOfn.Flags=OFN_ENABLEHOOK|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_EXPLORER|OFN_LONGNAMES;
	gddOfn.lpfnHook=(LPOFNHOOKPROC)ChoosePathDialogHookProc;
	gddOfn.lpTemplateName=NULL;
	if(GetOpenFileName(&gddOfn))
   	{
		gddOfn.lpstrFile[gddOfn.nFileOffset]='\0';
   		strncpy(fullPath,gddOfn.lpstrFile,stringBytes);
   		fullPath[stringBytes-1]='\0';
   		
		int
			i=0;
			
		while(fullPath[i])
		{
			if(fullPath[i]=='\\')
			{
				fullPath[i]='/';
			}
			i++;
		}
   		result=true;
   	}
   	else
   	{
		if(CommDlgExtendedError()==0)
		{
			result=true;
			(*cancel)=true;
		}
		else
		{
			SetCommDlgError();
		}
   	}
   	return result;
}

bool ChooseFontDialog(char *theTitle,char *theFontName,UINT32 stringBytes,bool *cancel)
/* bring up a font choose dialog
 * if there is a problem, SetError, return FALSE
 * otherwise, if cancel is TRUE, the user cancelled
 * otherwise, return the name of the chosen font in theFontName
 * NOTE: if the theFontName(including terminator) would be larger than stringBytes,
 * then it will be truncated to fit
 * NOTE ALSO: theFontName may be sent to this routine with a font name, and the
 * dialog will open to that font(as best as it can)
 */
{
	CHOOSEFONT
		cf;
	bool
		result;
	LOGFONT
		lf;

	result=false;

	(*cancel)=false;
	cf.lStructSize=sizeof(CHOOSEFONT);
	cf.hwndOwner=frameWindowHwnd;
	cf.lpLogFont=&lf;
	cf.Flags=CF_BOTH|CF_ENABLEHOOK;
	cf.rgbColors=RGB(0,0,0);
	cf.nFontType=0;
	cf.lpfnHook=(LPOFNHOOKPROC)CommonDialogHookProc;
	cf.Flags|=CF_INITTOLOGFONTSTRUCT;

	GetFontInfo(theFontName, &lf);

	if(ChooseFont(&cf))
	{
		PutFontInfo(&lf, theFontName);
		result=true;
	}
	else
	{
		if(CommDlgExtendedError()==0)
		{
			result=true;
			(*cancel)=true;
		}
		else
		{
			SetCommDlgError();
		}
	}

	return result;
}
