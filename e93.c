// editor '93
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

int main(int argc,char *argv[])
// the e93 entry point
{
	bool
		fail;
	UINT32
		numPointers;

	fail=false;
	programName=argv[0];								// point to the program name forever more
	setlocale(LC_ALL,"");								// adjust to local customs
	if(EarlyInit())										// give process a chance to do whatever it would like (NOTE: if it returns false, exit without complaint (so it can fork if it wants to))
	{
		if(InitErrors())
		{
			if(InitEnvironment())
			{
				ShellLoop(argc,argv);					// start the shell looping
				// @@ TODO: since ShellLoop calls Tcl_MainLoop we never get back to here.
				// Therefore, the cleanup has been moved to the bottom Tcl_MainLoop for now
				UnInitEnvironment();
			}
			else
			{
				GetError(&errorFamily,&errorFamilyMember,&errorDescription);
				fprintf(stderr,"Failed to initialize: %s\n",errorDescription);
				fail=true;
			}
			UnInitErrors();
		}
		else
		{
			fprintf(stderr,"Could not initialize error handlers\n");
			fail=true;
		}
		EarlyUnInit();
	}
	if((numPointers=MGetNumAllocatedPointers()))
	{
		fprintf(stderr,"Had %d pointer(s) allocated on exit\n",numPointers);
		fail=true;
	}
	if(fail)
	{
		return(1);										// tell OS bad things happened
	}
	return(0);
}
