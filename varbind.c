// Buffer variable binding interface
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

typedef struct variableBinding
{
	struct variableBinding
		*previousBinding,					// pointer to previous binding in the global binding list
		*nextBinding,						// pointer to next binding in the global binding list (in user defined order)
		*nextHashBinding;					// pointer to next binding in local hash list
	char
		*variableName,						// name of this variable
		*variableText;						// data contained in this variable
} VARIABLEBINDING;

static VARIABLEBINDING **GetBindingHashListHead(VARIABLETABLE *theVariableTable,char *variableName)
// perform hashing function on variableName, return pointer to list head
// for bindings
{
	UINT32
		count,
		length,
		sum;

	length=strlen(variableName);
	sum=0;
	for(count=0;count<length;count++)			// very crude hash function
	{
		sum+=variableName[count];
	}
	return(&(theVariableTable->variableBindingTable[sum&0xFF]));
}

char *GetVariableBindingName(VARIABLEBINDING *theBinding)
// return a pointer to the variable binding name for theBinding
{
	return(theBinding->variableName);
}

char *GetVariableBindingText(VARIABLEBINDING *theBinding)
// return a pointer to the variable binding text for theBinding
{
	return(theBinding->variableText);
}

static bool LocateVariableBindingLocation(VARIABLETABLE *theVariableTable,char *variableName,VARIABLEBINDING ***hashListHead,VARIABLEBINDING **previousHashEntry,VARIABLEBINDING **theEntry)
// locate a variable binding in the binding table, return pointers to important entries
{
	bool
		found;
	VARIABLEBINDING
		*currentEntry;

	currentEntry=*(*hashListHead=GetBindingHashListHead(theVariableTable,variableName));	// point to the head of the linked list at this hash entry
	*previousHashEntry=NULL;
	found=false;
	while(currentEntry&&!found)
	{
		if(strcmp(variableName,currentEntry->variableName)==0)
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

VARIABLEBINDING *LocateNextVariableBinding(VARIABLEBINDING *currentBinding)
// return the next variable binding from the current one
// if there is none, return NULL
{
	return(currentBinding->nextBinding);
}

VARIABLEBINDING *LocateVariableBinding(VARIABLETABLE *theVariableTable,char *variableName)
// find a key binding that matches the variableName, and return it
// if there is none, return NULL
{
	bool
		found;
	VARIABLEBINDING
		*currentEntry;

	currentEntry=*GetBindingHashListHead(theVariableTable,variableName);	// point to the head of the linked list at this hash entry
	found=false;
	while(currentEntry&&!found)
	{
		if(strcmp(variableName,currentEntry->variableName)==0)
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

bool DeleteVariableBinding(VARIABLETABLE *theVariableTable,char *variableName)
// attempt to locate the given binding in theVariableTable, and
// remove it if it is found
// if it is not located, return false
{
	VARIABLEBINDING
		**hashListHead,
		*previousHashEntry,
		*theEntry;

	if(LocateVariableBindingLocation(theVariableTable,variableName,&hashListHead,&previousHashEntry,&theEntry))
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
			if((theVariableTable->variableBindingListHead=theEntry->nextBinding))
			{
				theVariableTable->variableBindingListHead->previousBinding=NULL;
			}
		}
		MDisposePtr(theEntry->variableName);
		MDisposePtr(theEntry->variableText);
		MDisposePtr(theEntry);
		return(true);
	}
	return(false);
}

bool CreateVariableBinding(VARIABLETABLE *theVariableTable,char *variableName,char *variableText)
// search the variable binding table for an entry that matches the one given, and delete it if
// it exists. Then add this entry to the table
// if there is a problem, SetError, and return false
{
	VARIABLEBINDING
		**hashListHead,
		*newEntry;

	DeleteVariableBinding(theVariableTable,variableName);			// try to delete one if it exists
	hashListHead=GetBindingHashListHead(theVariableTable,variableName);	// point to head of list in hash table where this binding will go
	if((newEntry=(VARIABLEBINDING *)MNewPtr(sizeof(VARIABLEBINDING))))
	{
		if((newEntry->variableName=(char *)MNewPtr(strlen(variableName)+1)))
		{
			if((newEntry->variableText=(char *)MNewPtr(strlen(variableText)+1)))
			{
				strcpy(newEntry->variableName,variableName);			// copy in name
				strcpy(newEntry->variableText,variableText);			// copy in text
				newEntry->nextHashBinding=*hashListHead;
				*hashListHead=newEntry;									// link to head of hash list
				newEntry->previousBinding=NULL;
				if((newEntry->nextBinding=theVariableTable->variableBindingListHead))	// point to the next entry
				{
					theVariableTable->variableBindingListHead->previousBinding=newEntry;
				}
				theVariableTable->variableBindingListHead=newEntry;
				return(true);
			}
			MDisposePtr(newEntry->variableName);
		}
		MDisposePtr(newEntry);
	}
	return(false);
}

bool InitVariableBindingTable(VARIABLETABLE *theVariableTable)
// clear the variable binding table so that there are no bindings present
// if there is a problem, SetError, and return false
{
	int
		i;

	for(i=0;i<256;i++)
	{
		theVariableTable->variableBindingTable[i]=NULL;
	}
	theVariableTable->variableBindingListHead=NULL;
	return(true);
}

void UnInitVariableBindingTable(VARIABLETABLE *theVariableTable)
// remove all variable bindings from the bindings table
{
	int
		i;
	VARIABLEBINDING
		*currentEntry,
		*nextEntry;

	for(i=0;i<256;i++)
	{
		currentEntry=theVariableTable->variableBindingTable[i];
		while(currentEntry)
		{
			nextEntry=currentEntry->nextHashBinding;
			MDisposePtr(currentEntry->variableName);
			MDisposePtr(currentEntry->variableText);
			MDisposePtr(currentEntry);
			currentEntry=nextEntry;
		}
		theVariableTable->variableBindingTable[i]=NULL;
	}
	theVariableTable->variableBindingListHead=NULL;
}
