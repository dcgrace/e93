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


bool LiteralMatch(CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER *forTextChunk,UINT32 forTextOffset,UINT32 numToSearch,CHUNKHEADER **endChunk,UINT32 *endOffset);
bool LiteralMatchTT(CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER *forTextChunk,UINT32 forTextOffset,UINT32 numToSearch,const UINT8 *translateTable,CHUNKHEADER **endChunk,UINT32 *endOffset);
bool SearchForwardLiteral(CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER *forTextChunk,UINT32 forTextOffset,UINT32 forTextLength,UINT32 numToSearch,bool ignoreCase,UINT32 *matchOffset,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset);
bool SearchBackwardLiteral(CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER *forTextChunk,UINT32 forTextOffset,UINT32 forTextLength,UINT32 numToSearch,bool ignoreCase,UINT32 *matchOffset,CHUNKHEADER **startChunk,UINT32 *startOffset,CHUNKHEADER **endChunk,UINT32 *endOffset);
bool EditorFind(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,TEXTUNIVERSE *searchForText,bool backward,bool wrapAround,bool selectionExpr,bool ignoreCase,bool *foundMatch,SELECTIONUNIVERSE *resultSelectionUniverse);
bool EditorFindAll(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,TEXTUNIVERSE *searchForText,bool backward,bool wrapAround,bool selectionExpr,bool ignoreCase,bool limitScope,bool *foundMatch,SELECTIONUNIVERSE *resultSelectionUniverse);
bool EditorReplace(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,TEXTUNIVERSE *searchForText,TEXTUNIVERSE *replaceWithText,bool backward,bool wrapAround,bool selectionExpr,bool ignoreCase,bool replaceProc,bool *foundMatch,SELECTIONUNIVERSE *resultSelectionUniverse);
bool EditorReplaceAll(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,TEXTUNIVERSE *searchForText,TEXTUNIVERSE *replaceWithText,bool backward,bool wrapAround,bool selectionExpr,bool ignoreCase,bool limitScope,bool replaceProc,bool *foundMatch,SELECTIONUNIVERSE *resultSelectionUniverse);
