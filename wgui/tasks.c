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


#include "includes.h"

#define	PIPEBUFSIZE	65536*2

static char
	localErrorFamily[]="Wguitasks";

enum
{
	NOTASK,
	STDINCLOSED
};

static char *errorMembers[]=
{
	"NoTask",
	"stdinClosed"
};

static char *errorDescriptions[]=
{
	"No current task",
	"Task's stdin is closed"
};

                                 
static bool OpenTaskPipes(EDITORTASK *theTask)
/* create pipes that will connect us to a task
 * if there is a problem, SetError and return FALSE
 */
{
	SECURITY_ATTRIBUTES
		saAttr;
	HANDLE
		taskStdoutRdTmp,
		taskStdinWrTmp;
	unsigned long
		mode;

	theTask->closedStdin=false;
	saAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle=TRUE;
	saAttr.lpSecurityDescriptor=NULL;

	/* Create a pipe for the child's STDOUT */

	if(CreatePipe(&taskStdoutRdTmp,&(theTask->taskStdoutWr),&saAttr,PIPEBUFSIZE))
	{
		if(DuplicateHandle(GetCurrentProcess(),taskStdoutRdTmp,GetCurrentProcess(),&(theTask->taskStdoutRd),GENERIC_READ,FALSE,DUPLICATE_SAME_ACCESS))
		{
			CloseHandle(taskStdoutRdTmp);
			/* Create a pipe for the child's STDIN */
			saAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
			saAttr.bInheritHandle=TRUE;
			saAttr.lpSecurityDescriptor=NULL;
			if(CreatePipe(&(theTask->taskStdinRd),&taskStdinWrTmp,&saAttr,PIPEBUFSIZE))
			{
				if(DuplicateHandle(GetCurrentProcess(),taskStdinWrTmp,GetCurrentProcess(),&(theTask->taskStdinWr),GENERIC_WRITE,FALSE,DUPLICATE_SAME_ACCESS))
				{
					CloseHandle(taskStdinWrTmp);

					mode=PIPE_TYPE_BYTE|PIPE_NOWAIT|PIPE_READMODE_BYTE;
					SetNamedPipeHandleState(theTask->taskStdoutRd,&mode,NULL,NULL);	//must not block, or else I lock up

					mode=PIPE_TYPE_BYTE|PIPE_WAIT|PIPE_READMODE_BYTE;
					
					SetNamedPipeHandleState(theTask->taskStdoutWr,&mode,NULL,NULL);	// must block or we lose data
					SetNamedPipeHandleState(theTask->taskStdinRd,&mode,NULL,NULL);	// ftp shows us this should block

					SetNamedPipeHandleState(theTask->taskStdinWr,&mode,NULL,NULL);
					return(true);
				}
				CloseHandle(theTask->taskStdinRd);
				CloseHandle(taskStdinWrTmp);
			}
			CloseHandle(theTask->taskStdoutRd);
		}
		CloseHandle(taskStdoutRdTmp);
		CloseHandle(theTask->taskStdoutWr);
	}
	SetWindowsError();
	return(false);
}

static void CloseTaskPipes(EDITORTASK *theTask)
/* close pipes opened by OpenTaskPipes
 */
{
	CloseHandle(theTask->taskStdoutRd);
	if(!theTask->closedStdin)
	{
		CloseHandle(theTask->taskStdinWr);
	}
	CloseHandle(theTask->taskInfo.hProcess);	/* close the tasks process */
}

static bool SpawnTask(EDITORBUFFER *theBuffer,UINT8 *theData, char *completionProc)
// spawn off a task return true if it spawned
// SetError, and return false if there was a problem
//
// run tclsh, send the command to it for processing, tell tclsh to exit.
//
{
	bool
		result=false;

	if(theBuffer->theTask=(EDITORTASK *)MNewPtr(sizeof(EDITORTASK)))
	{
		UINT32
			createFlags=NORMAL_PRIORITY_CLASS;

		if(OpenTaskPipes(theBuffer->theTask))
		{
			char
				tclShellName[32];
			int
				major,
				minor;
			STARTUPINFO
				startInfo;
				
			Tcl_GetVersion(&major, &minor, NULL, NULL);
			sprintf(tclShellName,"tclsh%d%dt.exe", major, minor);

			startInfo.cb=sizeof(STARTUPINFO);
			startInfo.lpReserved=NULL;
			startInfo.cbReserved2=0;
			startInfo.lpReserved2=NULL;
			startInfo.lpDesktop=NULL;
			startInfo.lpTitle=NULL;
			startInfo.wShowWindow=SW_HIDE;
			startInfo.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
			startInfo.hStdInput=theBuffer->theTask->taskStdinRd;	/* causes some tools to not work */
			startInfo.hStdOutput=theBuffer->theTask->taskStdoutWr;
			startInfo.hStdError=theBuffer->theTask->taskStdoutWr;

			if(CreateProcess(NULL,tclShellName,NULL,NULL,TRUE,createFlags,NULL,NULL,&startInfo,&theBuffer->theTask->taskInfo))
			{
			    /* 
			     * "When an application spawns a process repeatedly, a new thread 
			     * instance will be created for each process but the previous 
			     * instances may not be cleaned up.  This results in a significant 
			     * virtual memory loss each time the process is spawned.  If there 
			     * is a WaitForInputIdle() call between CreateProcess() and
			     * CloseHandle(), the problem does not occur."
			     */

			    WaitForInputIdle(theBuffer->theTask->taskInfo.hProcess, 5000);
			    CloseHandle(theBuffer->theTask->taskInfo.hThread);
			    unsigned long
			    	numWritten;
			    char
			    	*tclInit = "proc promptproc {} {};set tcl_prompt1 promptproc;set tcl_interactive 1;",
			    	*tclExit = ";\nif {[lindex [split $errorCode] 0]==\"CHILDSTATUS\"} {exit [lindex [split $errorCode] 2]} else {exit 0}\n";

				WriteFile(theBuffer->theTask->taskStdinWr,(void *)tclInit,strlen(tclInit),&numWritten,NULL);
				WriteFile(theBuffer->theTask->taskStdinWr,(void *)theData,strlen((char *)theData),&numWritten,NULL);
				WriteFile(theBuffer->theTask->taskStdinWr,(void *)tclExit,strlen(tclExit),&numWritten,NULL);
				
				theBuffer->theTask->completionProc = completionProc;
				
				result=true;
			}
			
			CloseHandle(theBuffer->theTask->taskStdoutWr);	/* close our end to the pipes that we have */
			CloseHandle(theBuffer->theTask->taskStdinRd);	/* if the process got created, their end is still open, and will close when the process ends */
			
			if(!result)
			{
				CloseHandle(theBuffer->theTask->taskStdoutRd);
				CloseHandle(theBuffer->theTask->taskStdinWr);
			}
		}
		if(!result)
		{
			SetWindowsError();
			MDisposePtr(theBuffer->theTask);
			theBuffer->theTask=NULL;
		}
	}

	return(result);
}

bool SendEOFToBufferTask(EDITORBUFFER *theBuffer)
/* send an EOF to the input of the task running in theBuffer
 * if no task is running in theBuffer, or there
 * is some other problem, SetError, return false
 */
{
	if(theBuffer->theTask)
	{
		if(!theBuffer->theTask->closedStdin)			/* if closed already, complain */
		{
			CloseHandle(theBuffer->theTask->taskStdinWr);
			theBuffer->theTask->closedStdin=true;
			return(true);
		}
		else
		{
			SetError(localErrorFamily,errorMembers[STDINCLOSED],errorDescriptions[STDINCLOSED]);
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[NOTASK],errorDescriptions[NOTASK]);
	}
	return(false);
}


bool WriteBufferTaskData(EDITORBUFFER *theBuffer,UINT8 *theData,UINT32 numBytes, char *completionProc)
 // send numBytes to the standard input of the task running in theBuffer
 // if no task is running in theBuffer assume theData is a command and execute it, thereby starting a new task
 // 
 // if there is a problem writing data, SetError, return false
{
	bool
		result=false;
		
	if(theBuffer->theTask)								// do we already have a task?
	{
		if(!theBuffer->theTask->closedStdin)			// make sure it has not been closed
		{
			unsigned long
				numWritten;
				
			WriteFile(theBuffer->theTask->taskStdinWr,(void *)theData,numBytes,&numWritten,NULL);	// write theData to "stdin" of task
			result=true;
		}
		else
		{
			SetError(localErrorFamily,errorMembers[STDINCLOSED],errorDescriptions[STDINCLOSED]);
		}
	}
	else
	{
		if(SpawnTask(theBuffer,theData, completionProc))			// start the task
		{
			if(theBuffer->theWindow)
			{
				AdjustDocumentWindowStatus(theBuffer->theWindow);	// status bar needs updating now
			}
			result=true;
		}
	}
	return(result);
}

static bool ReadTaskData(EDITORBUFFER *theBuffer,bool *didUpdate)
/* see if the task in theBuffer has any output it wants to send,
 * if so, dump it into theBuffer
 * return didUpdate=TRUE if output was placed into theBuffer
 * if there is a problem, SetError, return FALSE
 * NOTE: the test for lastError==ERROR_NO_DATA is needed, because
 * read does not want to return the fact that it read 0 bytes in
 * non-blocking mode, If it did return 0, we might think it hit EOF.
 */
{
	char
		theChars[PIPEBUFSIZE+1];
	UINT32
		lastError;
	unsigned long
		numAval,
		numRead;
	bool
		result;

	result=false;
	(*didUpdate)=false;

	if(theBuffer->theTask !=NULL)
	{
		if(PeekNamedPipe(theBuffer->theTask->taskStdoutRd,(void *)NULL,0,&numRead,&numAval,NULL))
		{
			if(numAval>0)
			{
				numRead=Min(numAval,PIPEBUFSIZE);
				if(ReadFile(theBuffer->theTask->taskStdoutRd,(void *)theChars,numRead,&numRead,NULL))
				{
					if(numRead>0)
					{
						theChars[numRead]='\0';
						numRead=RemoveCRFromString(theChars);
						InsertBufferTaskData(theBuffer,(unsigned char *)&(theChars[0]),numRead);
						(*didUpdate)=true;
					}
					result=true;
				}
				else
				{
					SetWindowsError();
				}
			}
			else
			{
				result=true;
			}
		}
		else
		{
			lastError=GetLastError();
			switch(lastError)
			{
				case ERROR_NO_DATA:
				{
					result=true;
					break;
				}
				case ERROR_BROKEN_PIPE:
				{
					result=true;
					break;
				}
				default:
				{
					SetWindowsError();
					break;
				}
			}
		}
	}
	return(result);									/* the pipe has no data at this point */
}

extern Tcl_Interp
	*theTclInterpreter;							/* pointer to TCL interpreter we are using */

void DisconnectBufferTask(EDITORBUFFER *theBuffer)
/* disconnect ourselves from any task that is running in theBuffer
 * if no task is running, do nothing
 * NOTE: this does not need to kill the task, and it is usually desirable
 * NOT to kill the task
 */
{
	if(theBuffer->theTask)
	{
		char
			completionProcStr[255];
		bool
			callProc=false;
			
		if (theBuffer->theTask->completionProc != NULL)
		{
			sprintf(completionProcStr, "%s {%s} %d", theBuffer->theTask->completionProc, (char *)(theBuffer->contentName), theBuffer->theTask->exitCode);
			callProc=true;
		}
		CloseTaskPipes(theBuffer->theTask);
		MDisposePtr(theBuffer->theTask);
		theBuffer->theTask=NULL;
		if(theBuffer->theWindow)
		{
			AdjustDocumentWindowStatus(theBuffer->theWindow);	/* status bar needs updating now */
		}
		if(callProc)
		{
			ExecuteTclCommand(completionProcStr);
		}
	}
}

bool UpdateBufferTask(EDITORBUFFER *theBuffer,bool *didUpdate)
/* see if there is a task connected to theBuffer
 * if so, update the task by reading data from it, and adding
 * it to theBuffer
 * if there is a problem, SetError, and return false
 */
{
	bool
		result,
		hadUpdate,
		readOk;
	unsigned long
		exitCode;

	*didUpdate=false;
	if(theBuffer->theTask)						/* see if there is a task active in this buffer if not, complain */
	{
		GetExitCodeProcess(theBuffer->theTask->taskInfo.hProcess,&exitCode);
		if(exitCode==STILL_ACTIVE)						/* is the task still active? */
		{
			result=ReadTaskData(theBuffer,&hadUpdate);
			return(result);
		}
		else
		{
			while((readOk=ReadTaskData(theBuffer,&hadUpdate))&&hadUpdate)	/* inhale any data left in the pipeline */
			{
				(*didUpdate)=true;
			}
			GetExitCodeProcess(theBuffer->theTask->taskInfo.hProcess,&(theBuffer->theTask->exitCode));
			DisconnectBufferTask(theBuffer);	/* no more task associated with this buffer */
			return(readOk);
		}
	}
	else
	{
		SetError(localErrorFamily,errorMembers[NOTASK],errorDescriptions[NOTASK]);
	}
	return(false);
}

bool UpdateBufferTasks()
/* run through all the buffers, see if any has a task that has
 * data to be placed into its buffer, if so do it
 * if there is a problem, ignore it!
 * NOTE: this is never called from the high-level part of the editor
 * it is just here as a convenience to event handler.
 */
{
	EDITORBUFFER
		*theBuffer;
	bool
		hadUpdate,
		didUpdates;
		
	didUpdates=hadUpdate=false;
	theBuffer=EditorGetFirstBuffer();
	while(theBuffer)
	{
		if(theBuffer->theTask)
		{
			UpdateBufferTask(theBuffer,&hadUpdate);
			if(hadUpdate)
			{
				didUpdates=true;
			}
		}
		theBuffer=EditorGetNextBuffer(theBuffer);
	}
	return(didUpdates);
}


bool KillBufferTask(EDITORBUFFER *theBuffer)
// signal the task that is running in theBuffer to quit
// if no task is running, SetError, return false
{
	if(theBuffer->theTask)
	{
		kill(theBuffer->theTask->taskInfo.dwProcessId);
		theBuffer->theTask->exitCode=9;		// set the exit code for this as 9 for killed
		DisconnectBufferTask(theBuffer);
		return(true);
	}
	else
	{
		SetError(localErrorFamily,errorMembers[NOTASK],errorDescriptions[NOTASK]);
	}
	return(false);
}


