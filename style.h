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


void DeleteStyleRange(STYLEUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements);
bool InsertStyleRange(STYLEUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements);
bool GetStyleRange(STYLEUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements,UINT32 *theStyle);
UINT32 GetStyleAtPosition(STYLEUNIVERSE *theUniverse,UINT32 position);
bool SetStyleRange(STYLEUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements,UINT32 theStyle);
bool SetStyleAtPosition(STYLEUNIVERSE *theUniverse,UINT32 position,UINT32 theStyle);
void AdjustStylesForChange(STYLEUNIVERSE *theUniverse,UINT32 startOffset,UINT32 oldEndOffset,UINT32 newEndOffset);
bool CopyStyle(STYLEUNIVERSE *sourceUniverse,UINT32 sourceOffset,STYLEUNIVERSE *destUniverse,UINT32 destOffset,UINT32 numElements);
void CloseStyleUniverse(STYLEUNIVERSE *theUniverse);
STYLEUNIVERSE *OpenStyleUniverse();
