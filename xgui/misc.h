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


char *GetEditorLocalVersion();
bool PointInRECT(INT32 x,INT32 y,EDITORRECT *theRect);
void GlobalToLocal(Window theXWindow,INT32 x,INT32 y,INT32 *outX,INT32 *outY);
void LocalToGlobal(Window theXWindow,INT32 x,INT32 y,INT32 *outX,INT32 *outY);
void EditorBeep();
void ReportMessage(char *format,...);
void ShowBusy();
void ShowNotBusy();
char *CreateWindowTitleFromPath(char *absolutePath);
char *realpath2(char *path,char *outputPath);
char *CreateAbsolutePath(char *relativePath);
bool LocateStartupScript(char *scriptPath);
EDITORFILE *OpenEditorReadFile(char *thePath);
EDITORFILE *OpenEditorWriteFile(char *thePath);
bool ReadEditorFile(EDITORFILE *theFile,UINT8 *theBuffer,UINT32 bytesToRead,UINT32 *bytesRead);
bool WriteEditorFile(EDITORFILE *theFile,UINT8 *theBuffer,UINT32 bytesToWrite,UINT32 *bytesWritten);
void CloseEditorFile(EDITORFILE *theFile);
void OutlineShadowRectangle(Window xWindow,GC graphicsContext,EDITORRECT *theRect,GUICOLOR *upperLeftColor,GUICOLOR *lowerRightColor,UINT32 thickness);
