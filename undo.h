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


void BeginUndoGroup(EDITORBUFFER *theBuffer);
void EndUndoGroup(EDITORBUFFER *theBuffer);
void StrictEndUndoGroup(EDITORBUFFER *theBuffer);
bool RegisterUndoDelete(EDITORBUFFER *theBuffer,UINT32 startPosition,UINT32 numBytes);
bool RegisterUndoInsert(EDITORBUFFER *theBuffer,UINT32 startPosition,UINT32 numBytes);
bool EditorUndo(EDITORBUFFER *theBuffer);
bool EditorRedo(EDITORBUFFER *theBuffer);
bool EditorToggleUndo(EDITORBUFFER *theBuffer);
void EditorClearUndo(EDITORBUFFER *theBuffer);
void SetUndoCleanPoint(EDITORBUFFER *theBuffer);
bool AtUndoCleanPoint(EDITORBUFFER *theBuffer);
UNDOUNIVERSE *OpenUndoUniverse();
void CloseUndoUniverse(UNDOUNIVERSE *theUniverse);
