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


void DeleteAllSelections(SELECTIONUNIVERSE *theUniverse);
void DeleteSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements);
bool InsertSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements);
bool GetSelectionAtOrAfterPosition(SELECTIONUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements);
bool GetSelectionAtOrBeforePosition(SELECTIONUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements);
bool GetSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements,bool *isActive);
bool GetSelectionAtPosition(SELECTIONUNIVERSE *theUniverse,UINT32 position);
bool SetSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements);
bool IsSelectionEmpty(SELECTIONUNIVERSE *theUniverse);
void GetSelectionEndPositions(SELECTIONUNIVERSE *theUniverse,UINT32 *startPosition,UINT32 *endPosition);
UINT32 GetSelectionCursorPosition(SELECTIONUNIVERSE *theUniverse);
void SetSelectionCursorPosition(SELECTIONUNIVERSE *theUniverse,UINT32 thePosition);
void AdjustSelectionsForChange(SELECTIONUNIVERSE *theUniverse,UINT32 startOffset,UINT32 oldEndOffset,UINT32 newEndOffset);
bool CopySelectionUniverse(SELECTIONUNIVERSE *theSourceUniverse,SELECTIONUNIVERSE *theDestUniverse);
void CloseSelectionUniverse(SELECTIONUNIVERSE *theUniverse);
SELECTIONUNIVERSE *OpenSelectionUniverse();
SELECTIONUNIVERSE *CreateSelectionUniverseCopy(SELECTIONUNIVERSE *theOldUniverse);
