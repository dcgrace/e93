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


// view tracking modes

#define	TM_mMODE			0x000F		// masks out selection modes
#define	TM_CHAR				0x0000		// select by character
#define	TM_WORD				0x0001		// select by word
#define	TM_LINE				0x0002		// select by line
#define	TM_ALL				0x0003		// select everything
#define	TM_mHAND			0x1000		// tells if the hand movement tool has been selected
#define	TM_mCOLUMNAR		0x2000		// tells if the selection should be columnar
#define	TM_mREPEAT			0x4000		// tells if still tracking the same selection
#define	TM_mCONTINUE		0x8000		// tells if the selection should be continued from what is currently selected

// relative position modes (used to move cursor, delete, etc...)

#define	RPM_mMODE			0x000F		// masks out position modes
#define	RPM_CHAR			0x0000
#define	RPM_WORD			0x0001
#define	RPM_LINE			0x0002
#define	RPM_LINEEDGE		0x0003
#define	RPM_PARAGRAPHEDGE	0x0004
#define	RPM_PAGE			0x0005
#define	RPM_DOCEDGE			0x0006
#define	RPM_mBACKWARD		0x8000		// if set, move backward (towards start of file), if clear, move forward

void FixBufferSelections(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 oldEndOffset,UINT32 newEndOffset);
void EditorHomeViewToRange(EDITORVIEW *theView,UINT32 startPosition,UINT32 endPosition,bool useEnd,int horizontalMovement,int verticalMovement);
void EditorHomeViewToSelection(EDITORVIEW *theView,bool useEnd,int horizontalMovement,int verticalMovement);
void EditorHomeViewToSelectionEdge(EDITORVIEW *theView,bool useEnd,int horizontalMovement,int verticalMovement);
void EditorVerticalScroll(EDITORVIEW *theView,INT32 amountToScroll);
void EditorVerticalScrollByPages(EDITORVIEW *theView,INT32 pagesToScroll);
void EditorHorizontalScroll(EDITORVIEW *theView,INT32 amountToScroll);
void EditorHorizontalScrollByPages(EDITORVIEW *theView,INT32 pagesToScroll);
void EditorStartSelectionChange(EDITORBUFFER *theBuffer);
void EditorEndSelectionChange(EDITORBUFFER *theBuffer);
void EditorStartStyleChange(EDITORBUFFER *theBuffer);
void EditorEndStyleChange(EDITORBUFFER *theBuffer);
void EditorStartTextChange(EDITORBUFFER *theBuffer);
void EditorEndTextChange(EDITORBUFFER *theBuffer);
void EditorInvalidateViews(EDITORBUFFER *theBuffer);
bool ReplaceEditorChunks(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,CHUNKHEADER *textChunk,UINT32 textOffset,UINT32 numBytes);
bool ReplaceEditorText(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,UINT8 *theText,UINT32 numBytes);
bool ReplaceEditorFile(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,char *thePath);
void EditorStartReplace(EDITORBUFFER *theBuffer);
void EditorEndReplace(EDITORBUFFER *theBuffer);
void DeleteAllSelectedText(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse);
void EditorGetSelectionInfo(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,UINT32 *startPosition,UINT32 *endPosition,UINT32 *startLine,UINT32 *endLine,UINT32 *startLinePosition,UINT32 *endLinePosition,UINT32 *totalSegments,UINT32 *totalSpan);
UINT8 *EditorNextSelectionToBuffer(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,UINT32 *currentPosition,UINT32 additionalLength,UINT32 *actualLength,bool *atEnd);
void EditorDeleteSelection(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse);
void EditorSetNormalSelection(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,UINT32 startPosition,UINT32 endPosition);
void EditorSetColumnarSelection(EDITORVIEW *theView,UINT32 startLine,UINT32 endLine,INT32 leftX,INT32 rightX,UINT16 trackMode);
void EditorTrackViewPointer(EDITORVIEW *theView,INT32 viewXPosition,INT32 viewLine,UINT16 trackMode);
void EditorMoveCursor(EDITORVIEW *theView,UINT16 relativeMode);
void EditorMoveSelection(EDITORVIEW *theView,UINT16 relativeMode);
void EditorExpandNormalSelection(EDITORVIEW *theView,UINT16 relativeMode);
void EditorReduceNormalSelection(EDITORVIEW *theView,UINT16 relativeMode);
void EditorLocateLine(EDITORBUFFER *theBuffer,UINT32 theLine);
void EditorSelectAll(EDITORBUFFER *theBuffer);
void EditorDelete(EDITORVIEW *theView,UINT16 relativeMode);
void EditorInsert(EDITORBUFFER *theBuffer,UINT8 *theText,UINT32 textLength);
void EditorAuxInsert(EDITORBUFFER *theBuffer,UINT8 *theText,UINT32 textLength);
bool EditorInsertFile(EDITORBUFFER *theBuffer,char *thePath);
void EditorAutoIndent(EDITORBUFFER *theBuffer);
bool ClearBuffer(EDITORBUFFER *theBuffer);
bool LoadBuffer(EDITORBUFFER *theBuffer,char *thePath);
bool SaveBuffer(EDITORBUFFER *theBuffer,char *thePath,bool clearDirty);
void LinkViewToBuffer(EDITORVIEW *theView,EDITORBUFFER *theBuffer);
void UnlinkViewFromBuffer(EDITORVIEW *theView);
bool GotoEditorMark(EDITORBUFFER *theBuffer,MARKLIST *theMark);
MARKLIST *LocateEditorMark(EDITORBUFFER *theBuffer,char *markName);
MARKLIST *SetEditorMark(EDITORBUFFER *theBuffer,char *markName);
void ClearEditorMark(EDITORBUFFER *theBuffer,MARKLIST *theMark);
