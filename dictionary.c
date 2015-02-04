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


// a generic Hastable
#include	"includes.h"

// a key/value pair structure
typedef struct KeyValuePair
	{
 	char *key;
 	void *value;
	}
	KVPair;

static void DeleteKVPair(void *kvPtr)
//
{
	KVPair
		*thePair=(KVPair *)kvPtr;

	if(thePair->key)
	{
		MDisposePtr(thePair->key);					// free its key
	}
	MDisposePtr(thePair);							// delete the node
}

static int IndexOf(Dictionary *theTable,const char *key)
// 
{
	int
		size=SizeOfList(theTable),
		position=-1;
	KVPair
		*thePair=NULL;
		
	for(int index=0;(index<size)&&(position==-1);index++)
	{
		thePair=(KVPair *)ValueAt(theTable,index);
		if(strcmp(thePair->key, key)==0)
		{
			position=index;
		}
	}

	return position;
}

static KVPair *GetKVPair(Dictionary *theTable,const char *key)
// 
{
	int
		size=SizeOfList(theTable);
	KVPair
		*thePair=NULL;
		
	for(int index=0;(index<size);index++)
	{
		thePair=(KVPair *)ValueAt(theTable,index);
		if(strcmp(thePair->key, key)==0)
		{
			return thePair;
		}
	}

	return NULL;
}

bool HasKey(Dictionary *theTable,const char *key)
// 
{
	bool
		result=false;
	KVPair
		*thePair=GetKVPair(theTable,key);
		
	if(thePair!=NULL)
	{
		result=true;
	}

	return result;
}

void *GetFromDictionary(Dictionary *theTable,const char *key)
// Returns the value to which the specified key is mapped in Dictionary.
{
	void
		*value=NULL;
	KVPair
		*thePair=GetKVPair(theTable,key);
		
	if(thePair!=NULL)
	{
		value=thePair->value;
	}

	return value;
}

List *GetKeysFromDictionary(Dictionary *theTable)
// Returns a List of the keys in this Dictionary.
{
	List
		*keys=NULL;
	int
		size=SizeOfList(theTable);
	KVPair
		*thePair;
		
	if((keys=CreateList())!=NULL)
	{
		for(int index=0;index<size;index++)
		{
			thePair=(KVPair *)ValueAt(theTable,index);
			Append(keys,thePair->key);
		}
	}
	
	return keys;
}

KVPair *CreateKVPair(const char *key, void *value)
//
{
	KVPair
		*thePair=NULL;
		
	if((thePair=(KVPair *)MNewPtr(sizeof(thePair)))!=NULL)		// allocate memory to hold the KVPair
	{
		if((thePair->key=(char *)MNewPtr(strlen(key)+1))!=NULL)	// allocate memory to hold its key
		{
			strcpy(thePair->key,key);							// copy the key
			thePair->value=value;								// set the value
			return thePair;										// return here
		}
		MDisposePtr(thePair);
	}
	
	return thePair;
}

bool AddToDictionary(Dictionary *theTable,const char *key,void *value)
// Maps the specified key to the specified value in Dictionary.
// Neither the key nor the value can be null. 
{
	bool
		result=true;
	KVPair
		*thePair=NULL;
		
	if ((thePair=GetKVPair(theTable,key))!=NULL)			// this key is already in used
	{
		thePair->value=value;								// just reset the value in the KVPair
	}
	else
	{
		if ((thePair=CreateKVPair(key,value))!=NULL)		// make a new pair
		{
			Append(theTable,thePair);						// stuff it in the list
		}
		else
		{
			result=false;
		}
	}
	
	return result;
}

void RemoveFromDictionary(Dictionary *theTable,const char *key)
// Removes the node associated with key from the list
{
	int
		index;

	if((index=IndexOf(theTable,key))!=-1)					// get the index of the KVPair with this key
	{
		DeleteKVPair(ValueAt(theTable,index));				// delete this KVPair
		RemoveFromList(theTable, index);					// remove it from the list
	}
}

void DeleteDictionary(Dictionary *theTable)
// Frees the Dictionary list and all its members
{
	int
		size=SizeOfList(theTable);
		
	for(int index=0;index<size;index++)
	{
		DeleteKVPair(ValueAt(theTable,index));					// deleted all the KVPairs
	}
	DeleteList(theTable);
}

Dictionary *CreateDictionary()
// A Dictionary is a List of KVPairs
{
	Dictionary
		*theTable=CreateList();
	
		return theTable;
}
