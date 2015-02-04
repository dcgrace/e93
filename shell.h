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


void InsertBufferTaskData(EDITORBUFFER *theBuffer,UINT8 *theData,UINT32 numBytes);
bool tkHasGrab();
bool HandleBoundKeyEvent(UINT32 keyCode,UINT32 modifierValue);
void HandleMenuEvent(EDITORMENU *theMenu);
void HandleNonStandardControlEvent(char *theString);
void HandleViewEvent(VIEWEVENT *theEvent);
bool HandleShellCommand(char *theCommand,int argc,char *argv[]);
void ShellDoBackground();
void ShellLoop(int argc,char *argv[]);
