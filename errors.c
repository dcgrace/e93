// Error handling routines, and global structures
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

#define	MAXERRORFAMILY	32						// maximum number of characters in error family and member names
#define	MAXERRORDESCRIPTION	256					// maximum number of characters in error description

static char
	gErrorFamily[MAXERRORFAMILY+1];				// name of the family in which the error occurred
static char
	gFamilyMember[MAXERRORFAMILY+1];			// the member of the error family
static char
	gMemberDescription[MAXERRORDESCRIPTION+1];	// the description of the member

void SetError(char *errorFamily,char *familyMember,char *memberDescription)
// Set the current global error to the given error information.
// It is allowed for anyone to call this routine at any level to send back
// error information.
// errorFamily, and familyMember must be MAXERRORFAMILY characters long at most,
// and description is limited to MAXERRORDESCRIPTION characters in length.
// If the input strings exceed this length, they will be truncated.
{
	strncpy(&gErrorFamily[0],errorFamily,MAXERRORFAMILY);	// leave room for null terminator
	gErrorFamily[MAXERRORFAMILY]='\0';						// terminate if strncpy could not
	strncpy(&gFamilyMember[0],familyMember,MAXERRORFAMILY);	// leave room for null terminator
	gFamilyMember[MAXERRORFAMILY]='\0';						// terminate if strncpy could not
	strncpy(&gMemberDescription[0],memberDescription,MAXERRORDESCRIPTION);	// leave room for null terminator
	gMemberDescription[MAXERRORDESCRIPTION]='\0';			// terminate if strncpy could not
}

void SetStdCLibError()
// When an error occurs within a standard C library function, call this
// and it will turn the standard C library information into an extended library
// error
{
	char
		tempString[256];									// need a place to sprintf info

	sprintf(&tempString[0],"#%d",errno);				// place the error number into the temp space
	SetError("StdCLib",&tempString[0],strerror(errno));
}

void GetError(char **errorFamily,char **familyMember,char **memberDescription)
// Return pointers to the last globally kept error information.
// If no information is present, pointers to empty strings will be returned.
{
	*errorFamily=&gErrorFamily[0];
	*familyMember=&gFamilyMember[0];
	*memberDescription=&gMemberDescription[0];
}

bool InitErrors()
// Initialize the error handling code.
// If there is a problem return false.
// If false is returned, it is NOT legal to call GetError to find out what failed,
// You just have to report that the initialization failed.
{
	gErrorFamily[0]='\0';
	gFamilyMember[0]='\0';
	gMemberDescription[0]='\0';
	return(true);
}

void UnInitErrors()
// Undo whatever InitErrors did.
{
}
