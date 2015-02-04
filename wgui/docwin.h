// Copyright (C) 2000 Core Technologies.

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

void LinkWindowElement(WINDOWLISTELEMENT *theElement);
void UnlinkWindowElement(WINDOWLISTELEMENT *theElement);
void ReAdjustDocumentColors(EDITORWINDOW *theWindow);
void AdjustDocumentWindowStatus(EDITORWINDOW *theWindow);
void AdjustDocumentWindowViewStatus(EDITORVIEW *theView);
void AdjustDocumentWindowScrollThumbs(EDITORVIEW *theView);
void ScrollEditViewEvent(EDITORVIEW *theView,UINT32 eventCode,bool repeating);
void ScrollEditViewThumbVerticalEvent(EDITORVIEW *theView,UINT32 newValue);
void ScrollEditViewThumbHorizontalEvent(EDITORVIEW *theView,UINT32 newValue);
void UpdateEditorWindows();
void SetTopDocumentWindow(EDITORWINDOW *theWindow);
EDITORWINDOW *GetActiveDocumentWindow();
EDITORWINDOW *GetTopDocumentWindow();
EDITORWINDOW *GetNextDocumentWindow(EDITORWINDOW *previousWindow);
EDITORBUFFER *GetDocumentWindowEditorUniverse(EDITORWINDOW *theWindow);
EDITORVIEW *GetDocumentWindowCurrentView(EDITORWINDOW *theWindow);
bool GetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle,UINT32 stringBytes);
bool SetDocumentWindowTitle(EDITORWINDOW *theWindow,char *theTitle);
bool GetSortedDocumentWindowList(UINT32 *numElements,EDITORWINDOW ***theList);
bool GetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *foregroundColor);
bool SetEditorDocumentWindowForegroundColor(EDITORWINDOW *theWindow,EDITORCOLOR foregroundColor);
bool GetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR *backgroundColor);
bool SetEditorDocumentWindowBackgroundColor(EDITORWINDOW *theWindow,EDITORCOLOR backgroundColor);
bool GetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect);
bool SetEditorDocumentWindowRect(EDITORWINDOW *theWindow,EDITORRECT *theRect);
void GetEditorScreenDimensions(UINT32 *theWidth,UINT32 *theHeight);
EDITORWINDOW *OpenDocumentWindow(EDITORBUFFER *theUniverse,EDITORRECT *theRect,char *theTitle,char *fontName,UINT32 tabSize,EDITORCOLOR foreground,EDITORCOLOR background);
void CloseDocumentWindow(EDITORWINDOW *theWindow);
bool MDIShouldZoom();
void InitMDIShouldZoom(bool zoomed);
void SetMDIShouldZoom(bool zoomed);