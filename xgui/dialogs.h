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


#define		DIALOGBACKGROUNDCOLOR	GRAY2			// color to use for dialog backgrounds

typedef struct dialogItem	// item in list of items for given dialog (built during dialog creation)
{
	struct dialog
		*theDialog;			// pointer to the dialog this item is in
	void
		(*disposeProc)(struct dialogItem *theItem);	// routine to call to get rid of item's local storage
	void
		(*drawProc)(struct dialogItem *theItem);	// routine to call when drawing item
	bool
		(*trackProc)(struct dialogItem *theItem,XEvent *theEvent);	// routine to call when attempting to track the item (returns false if initial point not in item)
	bool
		(*earlyKeyProc)(struct dialogItem *theItem,XEvent *theEvent);	// if this item wants certain control keys before the default item can get them, it defines this
	bool
		wantFocus;									// tells the dialog manager that this item desires to be in the focus chain
	void
		(*focusChangeProc)(struct dialogItem *theItem);	// called by dialog manager to tell item that focus is changing
	void
		(*focusKeyProc)(struct dialogItem *theItem,XEvent *theEvent);	// when keys come in for this focussed item, this is called
	void
		(*focusPeriodicProc)(struct dialogItem *theItem);	// if this item wants to flash the cursor or whatever, it can set up this routine
	void
		*itemStruct;		// pointer to any data this item requires (used by create routines if needed)
	struct dialogItem
		*previousItem,
		*nextItem;
} DIALOGITEM;

typedef struct dialog		// list of dialog items
{
	EDITORWINDOW
		*parentWindow;		// the editor window that this dialog is in
	GUICOLOR
		*backgroundColor;	// color to use when drawing background of the dialog
	bool
		dialogComplete;		// set to true by dialog procs to tell when dialog is finished, and should be closed
	DIALOGITEM
		*firstItem;			// pointer to first item in the dialog (or NULL)
	DIALOGITEM
		*lastItem;			// pointer to last item in the dialog (or NULL)
	DIALOGITEM
		*focusItem;			// pointer to item with focus (or NULL)
	void
		*dialogLocal;		// place for local dialog routines to hang any data they need
} DIALOG;


DIALOGITEM *NewDialogItem(DIALOG *theDialog,bool (*createProc)(DIALOGITEM *theItem,void *itemDescriptor),void *itemDescriptor);
void DisposeDialogItem(DIALOGITEM *theDialogItem);
void UpdateDialogWindow(EDITORWINDOW *theWindow);
void DialogPeriodicProc(EDITORWINDOW *theWindow);
void DialogTakeFocus(DIALOGITEM *theItem);
void DialogWindowEvent(EDITORWINDOW *theWindow,XEvent *theEvent);
bool OkDialog(char *theText);
bool OkCancelDialog(char *theText,bool *cancel);
bool YesNoCancelDialog(char *theText,bool *yes,bool *cancel);
bool GetSimpleTextDialog(char *theTitle,char *enteredText,UINT32 stringBytes,bool *cancel);
bool SearchReplaceDialog(EDITORBUFFER *searchUniverse,EDITORBUFFER *replaceUniverse,bool *backwards,bool *wrapAround,bool *selectionExpr,bool *ignoreCase,bool *limitScope,bool *replaceProc,UINT16 *searchType,bool *cancel);
bool SimpleListBoxDialog(char *theTitle,UINT32 numElements,char **listElements,bool *selectedElements,bool *cancel);
bool OpenFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,char ***listElements,bool *cancel);
void FreeOpenFileDialogPaths(char **thePaths);
bool SaveFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel);
bool ChoosePathDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel);
bool ChooseFontDialog(char *theTitle,char *theFont,UINT32 stringBytes,bool *cancel);
