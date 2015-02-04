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


bool UniverseSanityCheck(TEXTUNIVERSE *theUniverse);
void PositionToChunkPositionPastEnd(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER **theChunk,UINT32 *theOffset);
void PositionToChunkPosition(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER **theChunk,UINT32 *theOffset);
void PositionToLinePosition(TEXTUNIVERSE *theUniverse,UINT32 thePosition,UINT32 *theLine,UINT32 *theLineOffset,CHUNKHEADER **theChunk,UINT32 *theChunkOffset);
void LineToChunkPosition(TEXTUNIVERSE *theUniverse,UINT32 theLine,CHUNKHEADER **theChunk,UINT32 *theOffset,UINT32 *thePosition);
void AddToChunkPosition(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **newChunk,UINT32 *newOffset,UINT32 distanceToMove);
void ChunkPositionToNextLine(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **newChunk,UINT32 *newOffset,UINT32 *distanceMoved);
bool InsertUniverseChunks(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER *textChunk,UINT32 textOffset,UINT32 numBytes);
bool InsertUniverseText(TEXTUNIVERSE *theUniverse,UINT32 thePosition,UINT8 *theText,UINT32 numBytes);
bool ExtractUniverseText(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT8 *theText,UINT32 numBytes,CHUNKHEADER **nextChunk,UINT32 *nextOffset);
bool DeleteUniverseText(TEXTUNIVERSE *theUniverse,UINT32 thePosition,UINT32 numBytes);
TEXTUNIVERSE *OpenTextUniverse();
void CloseTextUniverse(TEXTUNIVERSE *theUniverse);
