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


void SetBufferBusy(EDITORBUFFER *theBuffer);
void ClearBufferBusy(EDITORBUFFER *theBuffer);
bool BufferBusy(EDITORBUFFER *theBuffer);
EDITORBUFFER *EditorGetFirstBuffer();
EDITORBUFFER *EditorGetNextBuffer(EDITORBUFFER *theBuffer);
EDITORBUFFER *LocateBuffer(char *bufferName);
void EditorCloseBuffer(EDITORBUFFER *theBuffer);
void EditorReleaseBufferHandle(EDITORBUFFERHANDLE *theHandle);
void EditorGrabBufferHandle(EDITORBUFFERHANDLE *theHandle,EDITORBUFFER *theBuffer);
EDITORBUFFER *EditorGetBufferFromHandle(EDITORBUFFERHANDLE *theHandle);
void EditorInitBufferHandle(EDITORBUFFERHANDLE *theHandle);
bool EditorRevertBuffer(EDITORBUFFER *theBuffer);
bool EditorSaveBufferTo(EDITORBUFFER *theBuffer,char *thePath);
bool EditorSaveBufferAs(EDITORBUFFER *theBuffer,char *thePath);
bool EditorSaveBuffer(EDITORBUFFER *theBuffer);
EDITORBUFFER *EditorNewBuffer(char *bufferName);
EDITORBUFFER *EditorOpenBuffer(char *thePath);
void UnInitBuffers();
bool InitBuffers();
