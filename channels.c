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

// This code was contributed by Michael Manning

// This could use some more work to support some of the things Tcl might
// want to do with its channels. Like blocking and setting watches on
// them.


#include	"includes.h"

static Dictionary
	*bufferHandles;

static int BlockModeProc(ClientData instanceData,int mode)
// Set blocking or non-blocking mode
{
    return(-1);			// can't set blocking, return non-zero
}

static void WatchProc(ClientData instanceData,int mask)
// Called by the notifier to set up to watch for events on this
// channel.
{
}

static int GetHandleProc(ClientData instanceData,int direction,ClientData *handlePtr)
// Called from Tcl_GetChannelHandle to retrieve OS handles from
// inside a command console line based channel.
{
	*handlePtr=NULL;

	return(TCL_ERROR);
}

static int CloseProc(ClientData instanceData,Tcl_Interp *interp)
// Closes a console based IO channel.
{
	return(TCL_OK);		// nothing for us to do, but this proc must exist
}

static void ActiveWindowOutputProc(char *buf,int charsToWrite)
// get the output buffer if there is one, make sure it is not otherwise busy
{
	EDITORBUFFERHANDLE
		activeWindowBufferHandle;
	EDITORWINDOW
		*theWindow;

	if((theWindow=GetActiveDocumentWindow()) != NULL)							// get the top window if there is one
	{
		EditorInitBufferHandle(&activeWindowBufferHandle);
		EditorGrabBufferHandle(&activeWindowBufferHandle, theWindow->theBuffer); // lock the buffer

		if(!BufferBusy(theWindow->theBuffer))									//  make sure the buffer is not otherwise busy
		{
			EditorInsert(theWindow->theBuffer,(UINT8 *)buf,charsToWrite);		// insert the text
		}

		EditorReleaseBufferHandle(&activeWindowBufferHandle);					// release our grip on any buffer we held
	}
}

static int OutputProc(ClientData instanceData, CONST char *buf, int toWrite, int *errorCodePtr)
// Writes the given output on the IO channel. Returns count of how
// many characters were actually written, and an error indication.
{
	int
		charsWritten=0;
	EDITORBUFFER
		*theBuffer;

	if (instanceData == NULL)													// if there is not a bufferhandle for this channel
	{
		ActiveWindowOutputProc((char *)buf, toWrite);									// send it to the active window, if there is one
	}
	else
	{
		theBuffer=EditorGetBufferFromHandle((EDITORBUFFERHANDLE *)instanceData);
		if(theBuffer&&!BufferBusy(theBuffer))									// get the output buffer if there is one, make sure it is not otherwise busy
		{
			EditorInsert(theBuffer,(UINT8 *)buf,toWrite);
			charsWritten=toWrite;												// tell tcl how many characters we wrote
		}
	}

	if (charsWritten != toWrite)
	{
		fwrite(buf, toWrite, sizeof(char), stderr);	   			 				// send it to C's stderr
		charsWritten=toWrite;
	}

	return(charsWritten);
}

static void DeleteChannel(Tcl_Interp *localInterpreter,char *channelId)
// unregister the channel from Tcl, unlock the buffer (if there is one)
{
	Tcl_Channel
		channel;
	EDITORBUFFERHANDLE
		*theBufferHandle = NULL;
		
	if ((channel=Tcl_GetChannel(localInterpreter, channelId, NULL)) != NULL)
	{
		Tcl_UnregisterChannel(localInterpreter, channel);
	}
	Tcl_ResetResult(localInterpreter);										// do not leave error messages hanging around


	theBufferHandle=(EDITORBUFFERHANDLE *)GetFromDictionary(bufferHandles,channelId);	// get any bufferhandle we may already be using
	if(theBufferHandle!=NULL)
	{
		EditorReleaseBufferHandle(theBufferHandle);							// release our grip on any buffer we held
		RemoveFromDictionary(bufferHandles,channelId);					// drop it from the list
		MDisposePtr(theBufferHandle);										// free the bufferHandle
	}
}

static void DeleteChannelBuffers(Tcl_Interp *localInterpreter)
// release our hold on all bufferhandles we may have.
// delete the bufferhandle structures
{
	List
		*keys;																	// get a list of keys from the Dictionary
	int
		size;																	// how many keys are there?
	
	if((keys=GetKeysFromDictionary(bufferHandles))!=NULL)
	{
		size=SizeOfList(keys);
		for(int index=0;index<size;index++)
		{
			DeleteChannel(localInterpreter, (char *)(ValueAt(keys,index)));		// deleted all channels (this will also release their bufferhandles)
		}
		DeleteList(keys);
	}
}

/*
typedef struct Tcl_ChannelType1 {
	char *typeName;
	Tcl_DriverBlockModeProc *blockModeProc;	
	Tcl_DriverCloseProc *closeProc;
	Tcl_DriverInputProc *inputProc;
	Tcl_DriverOutputProc *outputProc;
	Tcl_DriverSeekProc *seekProc;
	Tcl_DriverSetOptionProc *setOptionProc;
	Tcl_DriverGetOptionProc *getOptionProc;
	Tcl_DriverWatchProc *watchProc;
	Tcl_DriverGetHandleProc *getHandleProc;
	Tcl_DriverClose2Proc *close2Proc;
} Tcl_ChannelType1;

typedef struct Tcl_ChannelType {
    char *typeName;
    Tcl_ChannelTypeVersion version;
    Tcl_DriverCloseProc *closeProc;
    Tcl_DriverInputProc *inputProc;
    Tcl_DriverOutputProc *outputProc;
    Tcl_DriverSeekProc *seekProc;
    Tcl_DriverSetOptionProc *setOptionProc;
    Tcl_DriverGetOptionProc *getOptionProc;
    Tcl_DriverWatchProc *watchProc;
    Tcl_DriverGetHandleProc *getHandleProc;
    Tcl_DriverClose2Proc *close2Proc;
    Tcl_DriverBlockModeProc *blockModeProc;  
    Tcl_DriverFlushProc *flushProc;  
    Tcl_DriverHandlerProc *handlerProc;  
    Tcl_DriverWideSeekProc *wideSeekProc;
} Tcl_ChannelType;

*/

static Tcl_ChannelType
	E93_ChannelType=
	{
		"buffer",				// Type name
		TCL_CHANNEL_VERSION_2,	// Version
		CloseProc,				// Close proc
		NULL,					// Input proc (we don't do input)
		OutputProc,				// Output proc
		NULL,					// Seek proc
		NULL,					// Set option proc
		NULL,					// Get option proc
		WatchProc,				// Set up notifier to watch the channel
		GetHandleProc,			// Get an OS handle from channel
		NULL,					// Close2 proc
		BlockModeProc,			// Set blocking or non-blocking mode
		NULL,					// flushProc;  
		NULL,					// handlerProc;  
		NULL					// wideSeekProc;
	};

EDITORBUFFER *GetChannelBuffer(Tcl_Interp *localInterpreter, char *name)
// get the buffer being used for stdout
{
	EDITORBUFFERHANDLE
		*thebuffHndl=(EDITORBUFFERHANDLE *)GetFromDictionary(bufferHandles,name);
	EDITORBUFFER
		*theBuffer=NULL;

	if(thebuffHndl!=NULL)
	{
		theBuffer=EditorGetBufferFromHandle(thebuffHndl);
	}

	return theBuffer;
}

bool SetChannelBuffer(Tcl_Interp *localInterpreter,char *channelId,EDITORBUFFER *newBuffer)
// set the buffer to be used for channelId
{
	Tcl_Channel
		channel;
	EDITORBUFFERHANDLE
		*theBufferHandle = NULL;
	bool
		result=false;

	DeleteChannel(localInterpreter, channelId);								// remove any previous buffer attached to this channel

	if(newBuffer)
	{
		if((theBufferHandle=(EDITORBUFFERHANDLE *)MNewPtr(sizeof(EDITORBUFFERHANDLE))))
		{
			EditorGrabBufferHandle(theBufferHandle,newBuffer);					// hang onto this one
		}
	}

	if(AddToDictionary(bufferHandles,channelId,theBufferHandle))				// associate the buffer with the channel name; a NULL buffer will send to the active Window
	{
		if((channel=Tcl_CreateChannel((Tcl_ChannelType *)&E93_ChannelType,channelId,theBufferHandle,TCL_WRITABLE))!=NULL)
		{
			Tcl_RegisterChannel(localInterpreter,channel);
			Tcl_SetChannelOption(localInterpreter,channel,"-buffering","none");	// don't buffer, write as soon as you get data
			Tcl_SetChannelOption(localInterpreter,channel,"-translation","lf");	// we only want to see linefeeds in our output
			result=true;
		}
	}
	
	return result;
}

void UnInitChannels(Tcl_Interp *localInterpreter)
// undo what InitChannels did
{
	DeleteChannelBuffers(localInterpreter);									// release our hold on all bufferhandles we may have
	DeleteDictionary(bufferHandles);										// delete the bufferhandles table, and all nodes
}

bool InitChannels(Tcl_Interp *localInterpreter)
{
	bool
		result=false;
	Tcl_Channel
		channel;
		
	bufferHandles=CreateDictionary();

	// take the stderr channel away from Tcl, this will prevent Tcl from closing the C stderr
	if ((channel=Tcl_GetChannel(localInterpreter, "stderr", NULL)) != NULL)
	{
		Tcl_UnregisterChannel(localInterpreter, channel);
	}
	Tcl_SetStdChannel(NULL, TCL_STDERR);
	
	// take the stdout channel away from Tcl, this will prevent Tcl from closing the C stdout
	if ((channel=Tcl_GetChannel(localInterpreter, "stdout", NULL)) != NULL)
	{
		Tcl_UnregisterChannel(localInterpreter, channel);
	}
	Tcl_SetStdChannel(NULL, TCL_STDOUT);
	
	// start without the stderr and stdout channels attached to any buffer, output will go to current window
	// the order matters for some reason known only to Tcl. Sharing channels for stderr and stdout?
	
	if(result=SetChannelBuffer(localInterpreter,"stdout",NULL))
	{
		result=SetChannelBuffer(localInterpreter,"stderr",NULL);
	}
	
	return result;
}
