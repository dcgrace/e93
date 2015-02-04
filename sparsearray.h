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


typedef struct sparseArrayHeader
{
	UINT32
		offset;					// offset to beginning of element from end of previous element
	UINT32
		length;					// number of elements taken by this entry
} SPARSEARRAYHEADER;

typedef struct sparseArray
{
	ARRAYUNIVERSE
		elementChunks;			// array of elements
	UINT32
		elementSize;			// size of an element in the array
	UINT32
		totalElements;			// keeps track of the total number of elements in the array (it is an index to one past the last occupied element)
	ARRAYCHUNKHEADER
		*cacheChunkHeader;		// stuff used for caching
	UINT32
		cacheOffset;
	UINT32
		cacheCurrentElement;	// tells the number of elements used to this point
} SPARSEARRAY;

void DumpSATable(SPARSEARRAY *theSparseArray);
void DeleteSARange(SPARSEARRAY *theSparseArray,UINT32 startPosition,UINT32 numElements);
bool InsertSARange(SPARSEARRAY *theSparseArray,UINT32 startPosition,UINT32 numElements);
bool GetSARecordForPosition(SPARSEARRAY *theSparseArray,UINT32 position,UINT32 *startPosition,UINT32 *numElements,void **theRecord);
bool GetSARecordAtOrAfterPosition(SPARSEARRAY *theSparseArray,UINT32 position,UINT32 *startPosition,UINT32 *numElements,void **theRecord);
bool GetSARecordAtOrBeforePosition(SPARSEARRAY *theSparseArray,UINT32 position,UINT32 *startPosition,UINT32 *numElements,void **theRecord);
bool SetSARange(SPARSEARRAY *theSparseArray,UINT32 startPosition,UINT32 numElements,void *theValue);
bool ClearSARange(SPARSEARRAY *theSparseArray,UINT32 startPosition,UINT32 numElements);
bool IsSAEmpty(SPARSEARRAY *theSparseArray);
bool GetSAElementEnds(SPARSEARRAY *theSparseArray,UINT32 *startElementOffset,UINT32 *endElementOffset);
void UnInitSA(SPARSEARRAY *theSparseArray);
bool InitSA(SPARSEARRAY *theSparseArray,UINT32 elementSize);
