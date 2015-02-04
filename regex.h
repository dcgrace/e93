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

#define MAXREGISTERS				10				// maximum number of registers that will be remembered

typedef struct compiledExpression COMPILEDEXPRESSION;

void REFree(COMPILEDEXPRESSION *theExpression);
void REPrintCompiledExpression(COMPILEDEXPRESSION *theExpression);
COMPILEDEXPRESSION *RECompile(UINT8 *theExpression,UINT32 numBytes,const UINT8 *translateTable);
bool GetRERegisterMatch(COMPILEDEXPRESSION *theCompiledExpression,UINT32 theRegisterIndex,UINT32 *matchOffset,UINT32 *matchLength);
bool GetRERegisterMatchChunkAndOffset(COMPILEDEXPRESSION *theCompiledExpression,UINT32 theRegisterIndex,CHUNKHEADER **theChunk,UINT32 *theOffset,UINT32 *matchLength);
bool SearchForwardRE(CHUNKHEADER *theChunk,UINT32 theOffset,COMPILEDEXPRESSION *theCompiledExpression,bool leftEdge,bool rightEdge,UINT32 numToSearchStart,UINT32 numToSearch,bool ignoreCase,bool *foundMatch,UINT32 *matchOffset,UINT32 *numMatched,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset,bool allowAbort);
bool SearchBackwardRE(CHUNKHEADER *theChunk,UINT32 theOffset,COMPILEDEXPRESSION *theCompiledExpression,bool leftEdge,bool rightEdge,UINT32 numToSearchStart,UINT32 numToSearch,bool ignoreCase,bool *foundMatch,UINT32 *matchOffset,UINT32 *numMatched,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset,bool allowAbort);
void REHuntBackToFarthestPossibleStart(CHUNKHEADER *theChunk,UINT32 theOffset,COMPILEDEXPRESSION *theCompiledExpression,UINT32 *numBack,CHUNKHEADER **endChunk,UINT32 *endOffset);
