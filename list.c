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

static Node *NodeAt(List *theList,int index)
// get the Node at position index in theList
{
	Node
		*theNode=NULL;
		
	if((index>=0) && (index<theList->size))							// range checking
	{
		theNode=theNode=theList->head;								// start at the head
		for(int count=0;(count<index)&&(theNode!=NULL);count++)		// the node should never become NULL if the list is OK
		{
			theNode=theNode->next;
		}
	}

	return theNode;
}

List *CreateList()
// create and initialize an empty List structure
{
	List
		*theList=NULL;
	
	if((theList=(List *)MNewPtr(sizeof(List)))!=NULL)
	{
		theList->head=NULL;
		theList->size=0;
	}
	
	return theList;
}

void DeleteList(List *theList)
// delete theList structure and all its Nodes
{
	Node
		*theNode=theList->head;
		
	while(theNode!=NULL)
	{
		Node
			*oldNode=theNode;

		theNode=theNode->next;
		MDisposePtr(oldNode);						// delete each Node

	}
	MDisposePtr(theList);							// delete the List	
}

int SizeOfList(List *theList)
{
	return theList->size;
}


void *ValueAt(List *theList,int index)
// get the value at position index in theList
{
	void
		*theValue=NULL;
	Node
		*theNode;
		
	if((theNode=NodeAt(theList, index))!=NULL)
	{
		theValue=theNode->value;
	}

	return theValue;
}

void Append(List *theList,void *value)
// add value to the end of theList
{
	Node
		*theNode=NULL;
	
	if ((theNode=(Node *)MNewPtr(sizeof(Node)))!=NULL)
	{
		theNode->value=value;
		theNode->next=NULL;
	
		if(theList->head==NULL)						// this will be the first Node
		{
			theList->head=theNode;
		}
		else
		{
			Node
				*lastNode=NodeAt(theList,theList->size-1);
			
			lastNode->next=theNode;					// attach it to the last node
		}
		(theList->size)++;
	}		
}

void RemoveFromList(List *theList,int index)
// remove the item at position index in theList
{
	Node
		*theNode;
		
	if((theNode=NodeAt(theList, index))!=NULL)
	{
		if(index==0)									// it was the head
		{
			theList->head=theNode->next;				// point the head at the node following the one we're deleting
		}
		else
		{
			Node
				*prev=NodeAt(theList, index-1);			// it follows another node, find it the one it follows
			
			prev->next=theNode->next;					// point its next at the Nodes next instead of at Node
		}
		
		MDisposePtr(theNode);							// delete the Node
		(theList->size)--;								// the list just got one smaller
	}
}


