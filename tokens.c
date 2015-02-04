// Token matching
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

static bool StrCaseCmp(char *stringA,char *stringB)
// compare stringA to stringB in a case-insensitive way
{
	while((*stringA)&&(*stringB)&&(toupper(*stringA)==toupper(*stringB)))
	{
		stringA++;
		stringB++;
	}
	return((*stringA)=='\0'&&(*stringB)=='\0');	// make sure we have reached the end of both strings
}

bool MatchToken(char *theString,TOKENLIST *theList,int *theToken)
// given a string, see if it matches any of the tokens in the token list, if so, return
// its token number in theToken, otherwise, return false
// This should not be case sensitive.
{
	int
		i;
	bool
		found,reachedEnd;

	found=reachedEnd=false;
	i=0;
	while(!found&&!reachedEnd)
	{
		if(theList[i].token[0]!='\0')
		{
			if(StrCaseCmp(theString,theList[i].token))
			{
				found=true;
				*theToken=theList[i].tokenNum;
			}
			i++;
		}
		else
		{
			reachedEnd=true;
		}
	}
	return(found);
}
