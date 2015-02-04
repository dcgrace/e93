// Key binding interface
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

typedef struct keyBinding
{
	UINT32
		keyCode,
		modifierMask,
		modifierValue;
	struct keyBinding
		*previousBinding,					// pointer to previous binding in the global binding list
		*nextBinding,						// pointer to next binding in the global binding list (in user defined order)
		*nextHashBinding;					// pointer to next binding in local hash list
	char
		dataText[1];						// variable length array of characters of data (would like to declare as dataText[0] but some compilers complain)
} EDITORKEYBINDING;

static EDITORKEYBINDING **GetBindingHashListHead(UINT32 keyCode)
// perform hashing function on keyCode, return pointer to list head
// for bindings
{
	return(&(keyBindingTable[keyCode&0xFF]));
}

char *GetKeyBindingText(EDITORKEYBINDING *theBinding)
// return a pointer to the key binding text for theBinding
{
	return(theBinding->dataText);
}

void GetKeyBindingCodeAndModifiers(EDITORKEYBINDING *theBinding,UINT32 *keyCode,UINT32 *modifierMask,UINT32 *modifierValue)
// return the key binding information
{
	*keyCode=theBinding->keyCode;
	*modifierMask=theBinding->modifierMask;
	*modifierValue=theBinding->modifierValue;
}

static bool LocateKeyBinding(UINT32 keyCode,UINT32 modifierMask,UINT32 modifierValue,EDITORKEYBINDING ***hashListHead,EDITORKEYBINDING **previousHashEntry,EDITORKEYBINDING **theEntry)
// locate a key binding in the binding table, return pointers to important entries
{
	bool
		found;
	EDITORKEYBINDING
		*currentEntry;

	currentEntry=*(*hashListHead=GetBindingHashListHead(keyCode));	// point to the head of the linked list at this hash entry
	*previousHashEntry=NULL;
	found=false;
	while(currentEntry&&!found)
	{
		if((currentEntry->keyCode==keyCode)&&(currentEntry->modifierMask==modifierMask)&&(currentEntry->modifierValue==modifierValue))
		{
			found=true;
			*theEntry=currentEntry;
		}
		else
		{
			*previousHashEntry=currentEntry;
			currentEntry=currentEntry->nextHashBinding;
		}
	}
	return(found);
}

EDITORKEYBINDING *LocateNextKeyBinding(EDITORKEYBINDING *currentBinding)
// return the next key binding from the current one
// if there is none, return NULL
{
	return(currentBinding->nextBinding);
}

EDITORKEYBINDING *LocateKeyBindingMatch(UINT32 keyCode,UINT32 modifierValue)
// find a key binding that matches the keyCode, and modifierValue, and return it
// if there is none, return NULL
{
	bool
		found;
	EDITORKEYBINDING
		*currentEntry;

	currentEntry=*GetBindingHashListHead(keyCode);	// point to the head of the linked list at this hash entry
	found=false;
	while(currentEntry&&!found)
	{
		if((currentEntry->keyCode==keyCode)&&((currentEntry->modifierMask&modifierValue)==currentEntry->modifierValue))
		{
			found=true;
		}
		else
		{
			currentEntry=currentEntry->nextHashBinding;
		}
	}
	return(currentEntry);
}

bool DeleteEditorKeyBinding(UINT32 keyCode,UINT32 modifierMask,UINT32 modifierValue)
// attempt to locate the given binding in the key bindings table, and
// remove it if it is found
// if it is not located, return false
{
	EDITORKEYBINDING
		**hashListHead,
		*previousHashEntry,
		*theEntry;

	if(LocateKeyBinding(keyCode,modifierMask,modifierValue,&hashListHead,&previousHashEntry,&theEntry))
	{
		if(previousHashEntry)
		{
			previousHashEntry->nextHashBinding=theEntry->nextHashBinding;		// unlink the located entry
		}
		else
		{
			*hashListHead=theEntry->nextHashBinding;
		}
		if(theEntry->previousBinding)
		{
			if((theEntry->previousBinding->nextBinding=theEntry->nextBinding))
			{
				theEntry->nextBinding->previousBinding=theEntry->previousBinding;
			}
		}
		else
		{
			if((keyBindingListHead=theEntry->nextBinding))
			{
				keyBindingListHead->previousBinding=NULL;
			}
		}
		MDisposePtr(theEntry);
		return(true);
	}
	return(false);
}

bool CreateEditorKeyBinding(UINT32 keyCode,UINT32 modifierMask,UINT32 modifierValue,char *dataText)
// search the key binding table for an entry that matches the one given, and delete it if
// it exists. Then add this entry to the table
// if there is a problem, SetError, and return false
{
	EDITORKEYBINDING
		**hashListHead,
		*newEntry;

	DeleteEditorKeyBinding(keyCode,modifierMask,modifierValue);	// try to delete one if it exists
	hashListHead=GetBindingHashListHead(keyCode);						// point to head of list in hash table where this binding will go
	if((newEntry=(EDITORKEYBINDING *)MNewPtr(sizeof(EDITORKEYBINDING)+strlen(dataText)+1)))
	{
		newEntry->keyCode=keyCode;
		newEntry->modifierMask=modifierMask;
		newEntry->modifierValue=modifierValue;
		newEntry->nextHashBinding=*hashListHead;
		strcpy(&(newEntry->dataText[0]),dataText);				// copy in the data
		*hashListHead=newEntry;									// link to head of hash list
		newEntry->previousBinding=NULL;
		if((newEntry->nextBinding=keyBindingListHead))			// point to the next entry
		{
			keyBindingListHead->previousBinding=newEntry;
		}
		keyBindingListHead=newEntry;
		return(true);
	}
	return(false);
}

bool InitKeyBindingTable()
// clear the key binding table so that there are no bindings present
// if there is a problem, SetError, and return false
{
	int
		i;

	for(i=0;i<256;i++)
	{
		keyBindingTable[i]=NULL;
	}
	keyBindingListHead=NULL;
	return(true);
}

void UnInitKeyBindingTable()
// remove all key bindings from the bindings table
{
	int
		i;
	EDITORKEYBINDING
		*currentEntry,
		*nextEntry;

	for(i=0;i<256;i++)
	{
		currentEntry=keyBindingTable[i];
		while(currentEntry)
		{
			nextEntry=currentEntry->nextHashBinding;
			MDisposePtr(currentEntry);
			currentEntry=nextEntry;
		}
		keyBindingTable[i]=NULL;
	}
	keyBindingListHead=NULL;
}
