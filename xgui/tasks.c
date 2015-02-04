// Routines to allow tasks to run under the editor
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

static char localErrorFamily[]="Xguitasks";

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
// create pipes that will connect us to a task
// if there is a problem, SetError, return false
{
	int
		flags;

	if(pipe(&(theTask->fdin[0]))==0)
	{
		if(pipe(&(theTask->fdout[0]))==0)
		{
			flags=fcntl(theTask->fdout[0],F_GETFL,0);
			flags|=O_NDELAY;						// turn off blocking on read
			fcntl(theTask->fdout[0],F_SETFL,flags);
			theTask->closedStdin=false;				// have not closed the input to the task
			return(true);
		}
		else
		{
			SetStdCLibError();
		}
		close(theTask->fdin[0]);
		close(theTask->fdin[1]);
	}
	else
	{
		SetStdCLibError();
	}
	return(false);
}

static void CloseTaskPipes(EDITORTASK *theTask)
// close pipes opened by OpenTaskPipes
{
	close(theTask->fdin[0]);
	if(!theTask->closedStdin)						// this part of the pipe can be closed before we get here, if an EOF has been sent to the task
	{
		close(theTask->fdin[1]);
	}
	close(theTask->fdout[0]);
	close(theTask->fdout[1]);
}

static bool SpawnTask(EDITORBUFFER *theBuffer,UINT8 *theData)
// spawn off a task return true if it spawned
// SetError, and return false if there was a problem
{
	int
		forkResult;

	if((theBuffer->theTask=(EDITORTASK *)MNewPtr(sizeof(EDITORTASK))))	// create task record
	{
		theBuffer->taskBytes=0;
		if(OpenTaskPipes(theBuffer->theTask))			// open pipes to the task
		{
			if((forkResult=fork()))						// parent process gets child ID back
			{
				if(forkResult>0)
				{
				    char
				    	*tclInit = "set tcl_interactive 1;\n",
				    	*tclExit = ";\nexit\n";
						
					theBuffer->theTask->pid=forkResult;	// set the process ID
					write(theBuffer->theTask->fdin[1],tclInit, strlen(tclInit));
					write(theBuffer->theTask->fdin[1],theData, strlen((char *)theData));
					write(theBuffer->theTask->fdin[1],tclExit, strlen(tclExit));				
					return(true);
				}
				else
				{
					SetStdCLibError();
				}
			}
			else										// child process gets 0 back
			{
				setsid();								// make a process group with this as the leader
				close(0);								// redirect stdin, stdout, and stderr through the pipe
				dup(theBuffer->theTask->fdin[0]);		// create stdin
				close(1);
				dup(theBuffer->theTask->fdout[1]);		// create stdout
				close(2);
				dup(theBuffer->theTask->fdout[1]);		// create stderr
				
				execl("/usr/bin/tclsh","tclsh",NULL);	// start the shell running in the child fork (if this works, we're done)
				_exit(-1);								// our task did not start, so bail out (use _exit to avoid flushing twice)
			}
			CloseTaskPipes(theBuffer->theTask);
		}
		MDisposePtr(theBuffer->theTask);
	}
	theBuffer->theTask=NULL;
	return(false);
}

bool WriteBufferTaskData(EDITORBUFFER *theBuffer,UINT8 *theData,UINT32 numBytes)
// send numBytes to the standard input of the task running in theBuffer
// if no task is running in theBuffer, send theData to execl, starting
// a task
// if there is a problem writing data, SetError, return false
{
	if(theBuffer->theTask)								// task??
	{
		if(!theBuffer->theTask->closedStdin)			// make sure it has not been closed
		{
			write(theBuffer->theTask->fdin[1],theData,numBytes);	// write data to "stdin" of task
			return(true);
		}
		else
		{
			SetError(localErrorFamily,errorMembers[STDINCLOSED],errorDescriptions[STDINCLOSED]);
		}
	}
	else
	{
		if(SpawnTask(theBuffer,theData))				// start the task
		{
			if(theBuffer->theWindow)
			{
				AdjustDocumentWindowStatus(theBuffer->theWindow);	// status bar needs updating now
			}
			return(true);
		}
	}
	return(false);
}

bool SendEOFToBufferTask(EDITORBUFFER *theBuffer)
// send an EOF to the input of the task running in theBuffer
// if no task is running in theBuffer, or there
// is some other problem, SetError, return false
{
	if(theBuffer->theTask)
	{
		if(!theBuffer->theTask->closedStdin)			// if closed already, complain
		{
			close(theBuffer->theTask->fdin[1]);			// close the side of the tasks stdin pipe that we write into, this will send an EOF to the task
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

#define	TASKDATABUFFERSIZE	8192

static char
	taskData[TASKDATABUFFERSIZE];

static bool ReadTaskData(EDITORBUFFER *theBuffer,bool *didUpdate)
// see if the task in theBuffer has any output it wants to send,
// if so, dump it into theBuffer
// return didUpdate=true if output was placed into theBuffer
// if there is a problem, SetError, return false
// NOTE: the test for errno==EWOULDBLOCK is needed, because
// read does not want to return the fact that it read 0 bytes in
// non-blocking mode, If it did return 0, we might think it hit EOF.
{
	int
		numRead;

	*didUpdate=false;
	if(((numRead=read(theBuffer->theTask->fdout[0],&(taskData[0]),TASKDATABUFFERSIZE))>=0)||errno==EWOULDBLOCK)
	{
		if(numRead>0)
		{
			theBuffer->taskBytes+=numRead;
			InsertBufferTaskData(theBuffer,(UINT8 *)&(taskData[0]),numRead);
			*didUpdate=true;
		}
		return(true);
	}
	else
	{
		SetStdCLibError();
	}
	return(false);
}

void DisconnectBufferTask(EDITORBUFFER *theBuffer)
// disconnect ourselves from any task that is running in theBuffer
// if no task is running, do nothing
// NOTE: this does not need to kill the task, and it is usually desirable
// NOT to kill the task
{
	if(theBuffer->theTask)
	{
		CloseTaskPipes(theBuffer->theTask);
		MDisposePtr(theBuffer->theTask);
		theBuffer->theTask=NULL;
		if(theBuffer->theWindow)
		{
			AdjustDocumentWindowStatus(theBuffer->theWindow);	// status bar needs updating now
		}
	}
}

bool UpdateBufferTask(EDITORBUFFER *theBuffer,bool *didUpdate)
// see if there is a task connected to theBuffer
// if so, update the task by reading data from it, and adding
// it to theBuffer
// return didUpdate set true if the task provided some data, false otherwise
// if there is a problem, SetError, and return false
{
	int
		thePid;
	int
		statusp;
	bool
		hadUpdate,
		readOk;

	waitpid(-1,&statusp,WNOHANG);				// get info about any of our children who have died (this will allow children that were started (but since, closed the pipes to) to die, and not lie around <defunct>)

	*didUpdate=false;
	if(theBuffer->theTask)						// see if there is a task active in this buffer if not, complain
	{
		thePid=waitpid(theBuffer->theTask->pid,&statusp,WNOHANG);	// check status of the task
		if(thePid==0)							// if status 0, it is still running
		{
			return(ReadTaskData(theBuffer,didUpdate));	// inhale some data from the task
		}
		else
		{
			while((readOk=ReadTaskData(theBuffer,&hadUpdate))&&hadUpdate)	// inhale any data left in the pipeline
			{
				*didUpdate=true;
			}
			DisconnectBufferTask(theBuffer);	// no more task associated with this buffer
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
// run through all the buffers, see if any has a task that has
// data to be placed into its buffer, if so do it
// if there is a problem, ignore it!
// return true if something was updated, false otherwise
// NOTE: this is never called from the high-level part of the editor
// it is just here as a convenience to event handler.
{
	EDITORBUFFER
		*theBuffer;
	bool
		hadUpdate,
		didUpdate;

	didUpdate=hadUpdate=false;
	theBuffer=EditorGetFirstBuffer();
	while(theBuffer)
	{
		if(theBuffer->theTask)
		{
			UpdateBufferTask(theBuffer,&hadUpdate);
			if(hadUpdate)
			{
				didUpdate=true;
			}
		}
		theBuffer=EditorGetNextBuffer(theBuffer);
	}
	return(didUpdate);
}

bool KillBufferTask(EDITORBUFFER *theBuffer)
// signal the task that is running in theBuffer to quit
// if no task is running, SetError, return false
{
	if(theBuffer->theTask)
	{
		kill(-(theBuffer->theTask->pid),SIGHUP);		// be nice, just send HUP, don't go for the total SIGKILL
		return(true);
	}
	else
	{
		SetError(localErrorFamily,errorMembers[NOTASK],errorDescriptions[NOTASK]);
	}
	return(false);
}

