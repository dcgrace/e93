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


// This include file is used to communicate with the GUI
// it contains all prototypes for editor functions that the GUI is
// allowed to call, and all GUI functions that the editor is allowed
// to call
// It also defines all data structures and types that they have in common

// modifier bits (can come from keyboard, or elsewhere (like mouse))

// masks
enum
{
	EEM_CAPSLOCK=0x00000001,
	EEM_SHIFT=0x00000002,
	EEM_CTL=0x00000004,
	EEM_MOD0=0x00000008,
	EEM_MOD1=0x00000010,
	EEM_MOD2=0x00000020,
	EEM_MOD3=0x00000040,
	EEM_MOD4=0x00000080,
	EEM_MOD5=0x00000100,
	EEM_MOD6=0x00000200,
	EEM_MOD7=0x00000400,
	EEM_STATE0=0x00F0000,		// traditionally used to keep track of double, triple, quadruple, etc... clicks of mouse
	EEM_STATE1=0x0F00000,
	EEM_STATE2=0xF000000
};

// shifts
enum
{
	EES_CAPSLOCK=0,
	EES_SHIFT=1,
	EES_CTL=2,
	EES_MOD0=3,
	EES_MOD1=4,
	EES_MOD2=5,
	EES_MOD3=6,
	EES_MOD4=7,
	EES_MOD5=8,
	EES_MOD6=9,
	EES_MOD7=10,
	EES_STATE0=16,
	EES_STATE1=20,
	EES_STATE2=24
};

// view event types

enum
{
	VET_KEYDOWN,			// saw key/virtual key press within view
	VET_KEYREPEAT,			// saw repeat within view
	VET_POSITIONVERTICAL,	// attempt to position the view to given absolute line
	VET_POSITIONHORIZONTAL,	// attempt to position the view to given absolute pixel
	VET_CLICK,				// initial click in view
	VET_CLICKHOLD			// send while click is being held in view
};

// virtual view keys

enum
{
	VVK_SCROLLUP,
	VVK_SCROLLDOWN,
	VVK_DOCUMENTPAGEUP,
	VVK_DOCUMENTPAGEDOWN,
	VVK_SCROLLLEFT,
	VVK_SCROLLRIGHT,
	VVK_DOCUMENTPAGELEFT,
	VVK_DOCUMENTPAGERIGHT,
// keys that are normally found on a keyboard, but not clearly associated with an ASCII code
// these are allowed to have modifier data sent with them
	VVK_LEFTARROW,
	VVK_RIGHTARROW,
	VVK_UPARROW,
	VVK_DOWNARROW,
	VVK_HOME,
	VVK_END,
	VVK_PAGEUP,
	VVK_PAGEDOWN,
	VVK_INSERT,
	VVK_BACKSPACE,
	VVK_DELETE,
	VVK_HELP,
	VVK_UNDO,
	VVK_REDO,
	VVK_UNDOTOGGLE,
	VVK_CUT,
	VVK_COPY,
	VVK_PASTE,
	VVK_FIND,
	VVK_RETURN,
	VVK_TAB,
	VVK_ESC,
	VVK_F1,
	VVK_F2,
	VVK_F3,
	VVK_F4,
	VVK_F5,
	VVK_F6,
	VVK_F7,
	VVK_F8,
	VVK_F9,
	VVK_F10,
	VVK_F11,
	VVK_F12,
	VVK_F13,
	VVK_F14,
	VVK_F15,
	VVK_F16,
	VVK_F17,
	VVK_F18,
	VVK_F19,
	VVK_F20
};

// menu relationships
enum
{
	MR_NEXTSIBLING,			// next sibling
	MR_PREVIOUSSIBLING,		// previous sibling
	MR_FIRSTCHILD,			// first child
	MR_LASTCHILD			// last child
};

// ways to home view (this type is used horizontally and vertically)
enum
{
	HT_NONE,				// do not move
	HT_LENIENT,				// if already on view, do not move, if off view, move as little as possible to get on view
	HT_SEMISTRICT,			// if already on view, do not move, if off view, move strict
	HT_STRICT				// move strict
};

// search types
enum
{
	ST_FIND,
	ST_FINDALL,
	ST_REPLACE,
	ST_REPLACEALL
};

// when the editor needs to read and write files, it communicates with the gui through this

typedef	struct editorFile EDITORFILE;	// this type is hidden from the editor (the editor only needs pointers to it)
typedef struct editorMenu EDITORMENU;	// menu definition (actual structure hidden from the editor)
typedef struct editorTask EDITORTASK;	// task definition (actual structure hidden)

typedef struct tokenlist				// token list definition
{
	char	*token;
	int		tokenNum;
} TOKENLIST;

typedef struct variableBinding VARIABLEBINDING;

typedef struct variableTable
{
	VARIABLEBINDING
		*variableBindingTable[256];		// array of heads of linked lists of variable bindings (crude hash table to get some speed when locating a binding)
	VARIABLEBINDING
		*variableBindingListHead;		// head of list than runs through all variables
} VARIABLETABLE;

typedef struct
{
	INT32
		x,y;
	UINT32
		w,h;
} EDITORRECT;

typedef UINT32	EDITORCOLOR;		// intensity values for each of the colors in RGB space (3 bytes R,G,B)

typedef struct chunkHeader
{
	struct chunkHeader
		*nextHeader,
		*previousHeader;
	UINT8
		*data;						// pointer to the data buffer for this chunk
	UINT32
		totalBytes;					// number of bytes of this chunk that are in use
	UINT32
		totalLines;					// number of new line characters in this chunk
} CHUNKHEADER;

typedef struct
{
	CHUNKHEADER
		*firstChunkHeader,			// pointer to the first chunk (NULL if there is none)
		*lastChunkHeader,			// pointer to the last chunk (NULL if there is none)
		*cacheChunkHeader;			// pointer to the cached chunk (NULL if there is none)
	UINT32
		totalBytes;					// total number of bytes of text
	UINT32
		totalLines;					// total number of new-line characters in text
	UINT32
		cacheBytes;					// total number of bytes up until the cacheChunkHeader
	UINT32
		cacheLines;					// total number of lines up until the cacheChunkHeader
} TEXTUNIVERSE;

typedef struct arrayChunkHeader
{
	struct arrayChunkHeader
		*nextHeader,
		*previousHeader;
	UINT8
		*data;						// pointer to the element data for this chunk
	UINT32
		totalElements;				// number of elements of this chunk that are in use
} ARRAYCHUNKHEADER;

typedef struct
{
	ARRAYCHUNKHEADER
		*firstChunkHeader,			// points to the first header of the chunk array (or NULL if there are no chunks)
		*lastChunkHeader;
	UINT32
		totalElements;				// total number of elements in this universe
	UINT32
		maxElements,				// maximum number of elements in a chunk of this universe
		elementSize;				// size of an element of a chunk in this universe
} ARRAYUNIVERSE;

typedef struct selectionUniverse SELECTIONUNIVERSE;
typedef struct styleUniverse STYLEUNIVERSE;
typedef struct syntaxInstance SYNTAXINSTANCE;

typedef struct undoElement
{
	UINT32
		frameNumber,				// tells which frame of undo this belongs to
		textStartPosition,			// position in text where replacement begins
		textLength,					// number of bytes to replace
		undoStartPosition,			// position in undo buffer of replacement text/style
		undoLength;					// length of replacement text
} UNDOELEMENT;

typedef struct
{
	ARRAYUNIVERSE
		undoArray;					// array which is used to keep track of the individual undos
	TEXTUNIVERSE
		*undoText;					// text used while undoing
	STYLEUNIVERSE
		*undoStyles;				// styles used while undoing
} UNDOGROUP;

typedef struct
{
	UINT32
		currentFrame;				// keeps track of the current undo frame index
	UINT32
		groupingDepth;				// knows the depth of begin/end undo group calls
	UINT32
		cleanElements;				// tells the number of elements in the undo-array when the buffer is considered "clean"
	bool
		cleanKnown;					// if false, cleanElements is meaningless and buffer is considered dirty
	bool
		undoActive;					// set true while undoing/redoing (tells not to delete redo information)
	bool
		undoing;					// set true while undoing (tells to place undo info into redo buffer)
	UNDOGROUP
		undoGroup;					// used to keep track of undos
	UNDOGROUP
		redoGroup;					// used to keep track of redos
} UNDOUNIVERSE;

typedef struct markList				// list of marks hanging off of editor universe
{
	char
		*markName;					// if NULL, then this mark is considered temporary
	SELECTIONUNIVERSE
		*selectionUniverse;			// the selection universe that defines this mark
	struct markList
		*nextMark;					// next mark in the list, NULL if no more
} MARKLIST;

typedef struct editorbufferhandle
{
	struct editorBuffer
		*theBuffer;					// points to the buffer being held
	struct editorbufferhandle
		*nextHandle; 				// next in linked list of handles
} EDITORBUFFERHANDLE;

typedef struct editorBuffer
{
	EDITORBUFFERHANDLE
		*bufferHandles;				// when something needs to know when a buffer has been deleted, it can get a handle to it which the buffer code will clear on deletion
	bool
		shellBusy;					// used by the shell to make sure it cannot modify a buffer while it is in use by some previous shell command
	bool
		fromFile;					// tells if contents of this buffer represent an actual file
	char
		*contentName;				// name that describes the contents of the buffer (if fromFile=true, then this is full absolute pathname of file)
	TEXTUNIVERSE
		*textUniverse;				// all the text
	UNDOUNIVERSE
		*undoUniverse;				// undo/redo information
	SELECTIONUNIVERSE
		*selectionUniverse;			// selection and cursor information
	SELECTIONUNIVERSE
		*auxSelectionUniverse;		// aux selection and cursor information (allows tasks to write to universe at place other than cursor)
	MARKLIST
		*theMarks;					// pointer to head of list of marks for this universe, NULL if none
	STYLEUNIVERSE
		*styleUniverse;				// maintains style array
	SYNTAXINSTANCE
		*syntaxInstance;			// keeps track of syntax highlighting (no syntax highlights if NULL)
	EDITORTASK
		*theTask;					// pointer to the task record for this buffer, or NULL if no task linked to this buffer
	UINT32
		taskBytes;					// how many bytes of data the last task added to theBuffer
	struct editorWindow
		*theWindow;					// pointer to document window for this buffer (if there is one, NULL if none)
	struct editorView
		*firstView;					// head of linked list of views onto this buffer
	VARIABLETABLE
		variableTable;				// table of variables linked to this buffer
	// NOTE: the members below help keep track of various things during replacements
	bool
		replaceHaveRange;			// tells if replacement range encountered yet
	UINT32
		replaceStartOffset,			// composite start offset in text
		replaceEndOffset;			// composite end offset in text (during the change)
	INT32
		replaceNumBytes;			// number of bytes added/(subtracted) from text
	// NOTE: the members below help keep track of various things during selection (this is a bit of a mess and should be simplified)
	UINT32
		anchorStartPosition,		// hold the ends of the selection while tracking
		anchorEndPosition;
	UINT32
		anchorLine;					// holds one corner of columnar selection while tracking
	INT32
		anchorX;
	UINT32
		columnarTopLine;			// when building a columnar selection, these remember the "rect" that bounds the selection
	INT32
		columnarLeftX;				// they are used when adding to the selection
	UINT32
		columnarBottomLine;
	INT32
		columnarRightX;
	bool
		haveStartX,					// tell if desiredStartX/desiredEndX are valid
		haveEndX;
	INT32
		desiredStartX,				// desired X position of the start of the selection when expanding/reducing, or of the cursor when no selection (used when moving the cursor up or down)
		desiredEndX;
	bool
		haveCurrentEnd;				// true if there is a current end of the selection being moved (used when extending the selection with the cursor keys)
	INT32
		currentIsStart;				// true if the current end being moved is the start
	struct editorBuffer
		*nextBuffer;				// next buffer in linked list of buffers
} EDITORBUFFER;

typedef struct editorWindow
{
	UINT16
		windowType;					// which type of editor window this is
	EDITORBUFFER
		*theBuffer;					// if this is a document window, then this points to the buffer, otherwise it is NULL
	void
		*windowInfo;				// pointer to gui specific structure that keeps all info needed for this window type
	void
		*userData;					// pointer to any additional gui specific info for this window (like the gui's actual window structure)
} EDITORWINDOW;

// NOTE: it is possible to have multiple views of the same editor universe
typedef struct editorView
{
	EDITORBUFFER
		*parentBuffer;				// the buffer which this view is placed
	EDITORWINDOW
		*parentWindow;				// the window that contains this view
	void
		*viewInfo;					// pointer to gui specific info about this view (gui needs to remember font, size, bounds, top line, etc)
	struct editorView
		*nextBufferView;			// next view of the buffer that this view is on
	void
		(*viewTextChangedVector)(struct editorView *theView);	// this vector is called by the view when it has finished a text change
	void
		(*viewSelectionChangedVector)(struct editorView *theView);	// this vector is called by the view when it has finished a selection change
	void
		(*viewPositionChangedVector)(struct editorView *theView);	// this vector is called by the view when it has finished a position change
	// all of these variables are used to facilitate updates of views
	bool
		wantHome;					// used during buffer update, tells if given view should be homed
	INT32
		updateStartX;				// used during update, keeps the X position into the line being updated (if it is on the view)
	UINT32
		startPosition,				// used during update, keep track of the character positions that the view spans
		endPosition;
} EDITORVIEW;

typedef struct
{
	bool
		isVirtual;					// tells if the keyCode is a virtual one or not
	UINT32
		keyCode;					// gives keyboard position of non-modifier key that was hit, or the virtual key number if it was a virtual key
	UINT32
		modifiers;					// modifier bits (enumerated above)
} EDITORKEY;

typedef struct
{
	UINT32
		modifiers;					// keyboard style modifier bits (enumerated above)
	UINT32
		position;					// absolute position (in pixels, or lines)
} VIEWPOSEVENTDATA;

typedef struct
{
	UINT32
		keyCode;					// mouse button (0-n)
	UINT32
		modifiers;					// keyboard style modifier bits (enumerated above)
	INT32
		xClick,
		yClick;						// view relative click point
} VIEWCLICKEVENTDATA;

typedef struct						// high level view events (sent to the shell from the view)
{
	UINT16
		eventType;
	EDITORVIEW
		*theView;					// the view that this event belongs to
	void
		*eventData;					// pointer to event specific data for event
} VIEWEVENT;

#define	HORIZONTALSCROLLTHRESHOLD	32				// number of "pixels" to scroll horizontally whenever we do so

// ALL the routines that the editor requires from the GUI ------------------------------------------------------------------------------------------

void EditorBeep();
void ReportMessage(char *format,...);
void GetEditorScreenDimensions(UINT32 *theWidth,UINT32 *theHeight);

void ResetEditorViewCursorBlink(EDITORVIEW *theView);
void InvalidateViewPortion(EDITORVIEW *theView,UINT32 startLine,UINT32 endLine,UINT32 startPixel,UINT32 endPixel);
void ScrollViewPortion(EDITORVIEW *theView,UINT32 startLine,UINT32 endLine,INT32 numLines,UINT32 startPixel,UINT32 endPixel,INT32 numPixels);
void SetViewTopLeft(EDITORVIEW *theView,UINT32 newTopLine,INT32 newLeftPixel);
void ViewsStartTextChange(EDITORBUFFER *theBuffer);
void ViewsEndTextChange(EDITORBUFFER *theBuffer);
void ViewsStartSelectionChange(EDITORBUFFER *theBuffer);
void ViewsEndSelectionChange(EDITORBUFFER *theBuffer);
void ViewsStartStyleChange(EDITORBUFFER *theBuffer);
void ViewsEndStyleChange(EDITORBUFFER *theBuffer);
void GetEditorViewGraphicToTextPosition(EDITORVIEW *theView,UINT32 linePosition,INT32 xPosition,UINT32 *betweenOffset,UINT32 *charOffset);
void GetEditorViewTextToGraphicPosition(EDITORVIEW *theView,UINT32 thePosition,INT32 *xPosition,bool limitMax,UINT32 *slopLeft,UINT32 *slopRight);
void GetEditorViewTextInfo(EDITORVIEW *theView,UINT32 *topLine,UINT32 *numLines,INT32 *leftPixel,UINT32 *numPixels);
void SetEditorViewTopLine(EDITORVIEW *theView,UINT32 lineNumber);
UINT32 GetEditorViewTabSize(EDITORVIEW *theView);
bool SetEditorViewTabSize(EDITORVIEW *theView,UINT32 theSize);

bool GetEditorViewStyleFont(EDITORVIEW *theView,UINT32 theStyle,char *theFont,UINT32 stringBytes);
bool SetEditorViewStyleFont(EDITORVIEW *theView,UINT32 theStyle,char *theFont);
void ClearEditorViewStyleFont(EDITORVIEW *theView,UINT32 theStyle);
bool GetEditorViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR *foregroundColor);
bool SetEditorViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR foregroundColor);
void ClearEditorViewStyleForegroundColor(EDITORVIEW *theView,UINT32 theStyle);
bool GetEditorViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR *backgroundColor);
bool SetEditorViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle,EDITORCOLOR backgroundColor);
void ClearEditorViewStyleBackgroundColor(EDITORVIEW *theView,UINT32 theStyle);
bool GetEditorViewSelectionForegroundColor(EDITORVIEW *theView,EDITORCOLOR *foregroundColor);
bool SetEditorViewSelectionForegroundColor(EDITORVIEW *theView,EDITORCOLOR foregroundColor);
void ClearEditorViewSelectionForegroundColor(EDITORVIEW *theView);
bool GetEditorViewSelectionBackgroundColor(EDITORVIEW *theView,EDITORCOLOR *backgroundColor);
bool SetEditorViewSelectionBackgroundColor(EDITORVIEW *theView,EDITORCOLOR backgroundColor);
void ClearEditorViewSelectionBackgroundColor(EDITORVIEW *theView);

bool EditorColorNameToColor(char *colorName,EDITORCOLOR *theColor);

void UpdateEditorWindows();

void MinimizeDocumentWindow(EDITORWINDOW *theWindow);
void UnminimizeDocumentWindow(EDITORWINDOW *theWindow);
void SetTopDocumentWindow(EDITORWINDOW *theWindow);
EDITORWINDOW *GetActiveDocumentWindow();
EDITORWINDOW *GetTopDocumentWindow();
bool GetSortedDocumentWindowList(UINT32 *numElements,EDITORWINDOW ***theList);
EDITORVIEW *GetDocumentWindowCurrentView(EDITORWINDOW *theWindow);

bool GetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle,UINT32 stringBytes);
bool SetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle);
bool GetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *foregroundColor);
bool SetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR foregroundColor);
bool GetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *backgroundColor);
bool SetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR backgroundColor);
bool GetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect);
bool SetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect);
EDITORWINDOW *OpenDocumentWindow(EDITORBUFFER *theBuffer,EDITORRECT *theRect,char *theTitle,char *fontName,UINT32 tabSize,EDITORCOLOR foreground,EDITORCOLOR background);
void CloseDocumentWindow(EDITORWINDOW *theWindow);

char *CreateWindowTitleFromPath(char *absolutePath);
char *CreateAbsolutePath(char *relativePath);
bool LocateStartupScript(char *scriptPath);

void DeactivateEditorMenu(EDITORMENU *theMenu);
void ActivateEditorMenu(EDITORMENU *theMenu);
char *GetEditorMenuName(EDITORMENU *theMenu);
bool SetEditorMenuName(EDITORMENU *theMenu,char *theName);
char *GetEditorMenuAttributes(EDITORMENU *theMenu);
bool SetEditorMenuAttributes(EDITORMENU *theMenu,char *theAttributes);
char *GetEditorMenuDataText(EDITORMENU *theMenu);
bool SetEditorMenuDataText(EDITORMENU *theMenu,char *theDataText);
EDITORMENU *GetRelatedEditorMenu(EDITORMENU *theMenu,int menuRelationship);
bool GetEditorMenu(int argc,char *argv[],EDITORMENU **theMenu);
EDITORMENU *CreateEditorMenu(int argc,char *argv[],int menuRelationship,char *theName,char *theAttributes,char *dataText,bool active);
void DisposeEditorMenu(EDITORMENU *theMenu);

bool EditorKeyNameToKeyCode(char *theName,UINT32 *theCode);
char *EditorKeyCodeToKeyName(UINT32 theCode);
bool EditorGetKeyPress(UINT32 *keyCode,UINT32 *editorModifiers,bool wait,bool clearBuffered);

void EditorQuit();
void EditorEventLoop(int argc,char *argv[]);
void ClearAbort();
bool CheckAbort();

void ShowBusy();
void ShowNotBusy();

bool ImportClipboard();
bool ExportClipboard();

EDITORFILE *OpenEditorReadFile(char *thePath);
EDITORFILE *OpenEditorWriteFile(char *thePath);
bool ReadEditorFile(EDITORFILE *theFile,UINT8 *theBuffer,UINT32 bytesToRead,UINT32 *bytesRead);
bool WriteEditorFile(EDITORFILE *theFile,UINT8 *theBuffer,UINT32 bytesToWrite,UINT32 *bytesWritten);
void CloseEditorFile(EDITORFILE *theFile);

bool OkDialog(char *theText);
bool OkCancelDialog(char *theText,bool *cancel);
bool YesNoCancelDialog(char *theText,bool *yes,bool *cancel);
bool GetSimpleTextDialog(char *theTitle,char *enteredText,UINT32 stringBytes,bool *cancel);
bool SearchReplaceDialog(EDITORBUFFER *searchUniverse,EDITORBUFFER *replaceUniverse,bool *backwards,bool *wrapAround,bool *selectionExpr,bool *ignoreCase,bool *limitScope,bool *replaceProc,UINT16 *searchType,bool *cancel);
bool SimpleListBoxDialog(char *theTitle,UINT32 numElements,char **listElements,bool *selectedElements,bool *cancel);
bool OpenFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,char ***listElements,bool *cancel);
void FreeOpenFileDialogPaths(char **thePaths);
bool SaveFileDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel);
bool ChoosePathDialog(char *theTitle,char *fullPath,UINT32 stringBytes,bool *cancel);
bool ChooseFontDialog(char *theTitle,char *theFont,UINT32 stringBytes,bool *cancel);

bool WriteBufferTaskData(EDITORBUFFER *theBuffer,UINT8 *theData,UINT32 numBytes, char *completionProc);
bool SendEOFToBufferTask(EDITORBUFFER *theBuffer);
void DisconnectBufferTask(EDITORBUFFER *theBuffer);
bool UpdateBufferTask(EDITORBUFFER *theBuffer,bool *didUpdate);
bool KillBufferTask(EDITORBUFFER *theBuffer);

void *MNewPtr(UINT32 theSize);
void *MNewPtrClr(UINT32 theSize);
void MDisposePtr(void *thePointer);
void MMoveMem(void *source,void *dest,UINT32 numBytes);
UINT32 MGetNumAllocatedPointers();

char *GetEditorLocalVersion();
bool AddSupplementalShellCommands(Tcl_Interp *theInterpreter);
bool InitEnvironment();
void UnInitEnvironment();
bool EarlyInit();
void EarlyUnInit();

char *GetMainWindowID();
void PrintMessage(FILE *stream, char *format,...);

void EditorSetModal();
void EditorClearModal();

// Editor routines and globals visible to the GUI ------------------------------------------------------------------------------------------

extern char
	*programName;

bool MatchToken(char *theString,TOKENLIST *theList,int *theToken);

void PositionToLinePosition(TEXTUNIVERSE *theUniverse,UINT32 thePosition,UINT32 *theLine,UINT32 *theLineOffset,CHUNKHEADER **theChunk,UINT32 *theChunkOffset);
void PositionToChunkPositionPastEnd(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER **theChunk,UINT32 *theOffset);
void PositionToChunkPosition(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER **theChunk,UINT32 *theOffset);
void LineToChunkPosition(TEXTUNIVERSE *theUniverse,UINT32 theLine,CHUNKHEADER **theChunk,UINT32 *theOffset,UINT32 *thePosition);
void ChunkPositionToNextLine(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,CHUNKHEADER **newChunk,UINT32 *newOffset,UINT32 *distanceMoved);
bool InsertUniverseChunks(TEXTUNIVERSE *theUniverse,UINT32 thePosition,CHUNKHEADER *textChunk,UINT32 textOffset,UINT32 numBytes);
bool InsertUniverseText(TEXTUNIVERSE *theUniverse,UINT32 thePosition,UINT8 *theText,UINT32 numBytes);
bool ExtractUniverseText(TEXTUNIVERSE *theUniverse,CHUNKHEADER *theChunk,UINT32 theOffset,UINT8 *theText,UINT32 numBytes,CHUNKHEADER **nextChunk,UINT32 *nextOffset);
bool DeleteUniverseText(TEXTUNIVERSE *theUniverse,UINT32 thePosition,UINT32 numBytes);
TEXTUNIVERSE *OpenTextUniverse();
void CloseTextUniverse(TEXTUNIVERSE *theUniverse);

EDITORBUFFER *EditorGetCurrentClipboard();
void EditorSetCurrentClipboard(EDITORBUFFER *theClipboard);
void EditorCopy(EDITORBUFFER *theBuffer,EDITORBUFFER *theClipboard,bool append);
void EditorCut(EDITORBUFFER *theBuffer,EDITORBUFFER *theClipboard,bool append);
void EditorPaste(EDITORBUFFER *theBuffer,EDITORBUFFER *theClipboard);
void EditorColumnarPaste(EDITORVIEW *theView,EDITORBUFFER *theClipboard);
EDITORBUFFER *EditorStartImportClipboard();
void EditorEndImportClipboard();
EDITORBUFFER *EditorStartExportClipboard();
void EditorEndExportClipboard();

bool GetSelectionAtOrAfterPosition(SELECTIONUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements);
bool GetSelectionAtOrBeforePosition(SELECTIONUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements);
bool GetSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements,bool *isActive);
bool GetSelectionAtPosition(SELECTIONUNIVERSE *theUniverse,UINT32 position);
bool SetSelectionRange(SELECTIONUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements);
bool IsSelectionEmpty(SELECTIONUNIVERSE *theUniverse);
void GetSelectionEndPositions(SELECTIONUNIVERSE *theUniverse,UINT32 *startPosition,UINT32 *endPosition);
UINT32 GetSelectionCursorPosition(SELECTIONUNIVERSE *theUniverse);
void SetSelectionCursorPosition(SELECTIONUNIVERSE *theUniverse,UINT32 thePosition);
void CloseSelectionUniverse(SELECTIONUNIVERSE *theUniverse);
SELECTIONUNIVERSE *OpenSelectionUniverse();

void EditorGetSelectionInfo(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse,UINT32 *startPosition,UINT32 *endPosition,UINT32 *startLine,UINT32 *endLine,UINT32 *startLinePosition,UINT32 *endLinePosition,UINT32 *totalSegments,UINT32 *totalSpan);

void DeleteStyleRange(STYLEUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements);
bool InsertStyleRange(STYLEUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements);
bool GetStyleRange(STYLEUNIVERSE *theUniverse,UINT32 position,UINT32 *startPosition,UINT32 *numElements,UINT32 *theStyle);
UINT32 GetStyleAtPosition(STYLEUNIVERSE *theUniverse,UINT32 position);
bool SetStyleRange(STYLEUNIVERSE *theUniverse,UINT32 startPosition,UINT32 numElements,UINT32 theStyle);
bool SetStyleAtPosition(STYLEUNIVERSE *theUniverse,UINT32 position,UINT32 theStyle);
bool SelectionToStyle(SELECTIONUNIVERSE *theSelections,STYLEUNIVERSE *theStyles,UINT32 theStyle);
void CloseStyleUniverse(STYLEUNIVERSE *theUniverse);
STYLEUNIVERSE *OpenStyleUniverse();

bool AtUndoCleanPoint(EDITORBUFFER *theBuffer);

void EditorHomeViewToRange(EDITORVIEW *theView,UINT32 startPosition,UINT32 endPosition,bool useEnd,int horizontalMovement,int verticalMovement);
void EditorHomeViewToSelection(EDITORVIEW *theView,bool useEnd,int horizontalMovement,int verticalMovement);
void EditorHomeViewToSelectionEdge(EDITORVIEW *theView,bool useEnd,int horizontalMovement,int verticalMovement);
void EditorVerticalScroll(EDITORVIEW *theView,INT32 amountToScroll);
void EditorVerticalScrollByPages(EDITORVIEW *theView,INT32 pagesToScroll);
void EditorHorizontalScroll(EDITORVIEW *theView,INT32 amountToScroll);
void EditorHorizontalScrollByPages(EDITORVIEW *theView,INT32 pagesToScroll);
void EditorStartReplace(EDITORBUFFER *theBuffer);
bool ReplaceEditorChunks(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,CHUNKHEADER *textChunk,UINT32 textOffset,UINT32 numBytes);
bool ReplaceEditorText(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,UINT8 *theText,UINT32 numBytes);
void EditorEndReplace(EDITORBUFFER *theBuffer);
void EditorDeleteSelection(EDITORBUFFER *theBuffer,SELECTIONUNIVERSE *theSelectionUniverse);
void EditorTrackViewPointer(EDITORVIEW *theView,INT32 viewLine,INT32 viewXPosition,UINT16 trackMode);
void EditorMoveCursor(EDITORVIEW *theView,UINT16 relativeMode);
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
void EditorCloseBuffer(EDITORBUFFER *theBuffer);
EDITORBUFFER *EditorNewBuffer(char *bufferName);
EDITORBUFFER *EditorOpenBuffer(char *thePath);
void LinkViewToBuffer(EDITORVIEW *theView,EDITORBUFFER *theBuffer);
void UnlinkViewFromBuffer(EDITORVIEW *theView);

bool GotoEditorMark(EDITORBUFFER *theBuffer,MARKLIST *theMark);
MARKLIST *LocateEditorMark(EDITORBUFFER *theBuffer,char *markName);
MARKLIST *SetEditorMark(EDITORBUFFER *theBuffer,char *markName);
void ClearEditorMark(EDITORBUFFER *theBuffer,MARKLIST *theMark);

void SetError(char *errorFamily,char *familyMember,char *memberDescription);
void SetStdCLibError();
void GetError(char **errorFamily,char **familyMember,char **memberDescription);

EDITORBUFFER *EditorGetFirstBuffer();
EDITORBUFFER *EditorGetNextBuffer(EDITORBUFFER *fromBuffer);

void InsertBufferTaskData(EDITORBUFFER *theBuffer,UINT8 *theData,UINT32 numBytes);
bool HandleBoundKeyEvent(UINT32 keyCode,UINT32 modifierValue);
void HandleMenuEvent(EDITORMENU *theMenu);
void HandleNonStandardControlEvent(char *theString);
void HandleViewEvent(VIEWEVENT *theEvent);
bool HandleShellCommand(char *theCommand,int argc,char *argv[]);
void ShellDoBackground();

bool tkHasGrab();
