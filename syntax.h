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

typedef struct expressionPiece EXPRESSIONPIECE;
typedef struct expressionPieceList EXPRESSIONPIECELIST;
typedef struct syntaxExpression SYNTAXEXPRESSION;
typedef struct syntaxStyleMapping SYNTAXSTYLEMAPPING;
typedef struct syntaxStyleMappingList SYNTAXSTYLEMAPPINGLIST;
typedef struct syntaxMap SYNTAXMAP;

bool UpdateSyntaxInformation(EDITORBUFFER *theBuffer,UINT32 startOffset,UINT32 endOffset,UINT32 numBytes);
bool RegenerateAllSyntaxInformation(EDITORBUFFER *theBuffer);
SYNTAXMAP *GetInstanceParentMap(SYNTAXINSTANCE *theInstance);
void CloseSyntaxInstance(SYNTAXINSTANCE *theInstance);
SYNTAXINSTANCE *OpenSyntaxInstance(SYNTAXMAP *theMap,EDITORBUFFER *theBuffer);
void RemoveMappingList(SYNTAXSTYLEMAPPINGLIST *theList);
SYNTAXSTYLEMAPPINGLIST *AddMappingListElement(SYNTAXSTYLEMAPPINGLIST **theListHead,SYNTAXSTYLEMAPPING *theMapping);
SYNTAXSTYLEMAPPINGLIST *AddBetweenListElementToMapping(SYNTAXSTYLEMAPPING *theMapping,SYNTAXSTYLEMAPPING *betweenMapping);
SYNTAXSTYLEMAPPINGLIST *AddRootListElementToMap(SYNTAXMAP *theMap,SYNTAXSTYLEMAPPING *rootMapping);
SYNTAXSTYLEMAPPING *LocateSyntaxStyleMapping(SYNTAXMAP *theMap,char *theName);
void SetMappingStartExpressionPiece(SYNTAXSTYLEMAPPING *theMapping,EXPRESSIONPIECE *startExpressionPiece);
void RemoveExpressionList(EXPRESSIONPIECELIST *theList);
EXPRESSIONPIECELIST *AddMappingEndExpressionPiece(SYNTAXSTYLEMAPPING *theMapping,EXPRESSIONPIECE *endExpressionPiece);
void SetMappingStyles(SYNTAXSTYLEMAPPING *theMapping,UINT32 startStyle,UINT32 betweenStyle,UINT32 endStyle);
void RemoveMappingFromSyntaxMap(SYNTAXMAP *theMap,SYNTAXSTYLEMAPPING *theMapping);
SYNTAXSTYLEMAPPING *AddMappingToSyntaxMap(SYNTAXMAP *theMap,char *mappingName);
EXPRESSIONPIECE *LocateExpressionPiece(SYNTAXEXPRESSION *theExpression,bool useRegisterMatch,UINT32 registerIndex);
void RemovePieceFromExpression(SYNTAXEXPRESSION *theExpression,EXPRESSIONPIECE *thePiece);
EXPRESSIONPIECE *AddPieceToExpression(SYNTAXEXPRESSION *theExpression,bool useRegisterMatch,UINT32 registerIndex);
SYNTAXEXPRESSION *LocateSyntaxMapExpression(SYNTAXMAP *theMap,char *theName);
void RemoveExpressionFromSyntaxMap(SYNTAXMAP *theMap,SYNTAXEXPRESSION *theExpression);
SYNTAXEXPRESSION *AddExpressionToSyntaxMap(SYNTAXMAP *theMap,char *expressionName,char *expressionText);
char *GetSyntaxMapName(SYNTAXMAP *theMap);
SYNTAXMAP *LocateNextSyntaxMap(SYNTAXMAP *theMap);
SYNTAXMAP *LocateSyntaxMap(char *theName);
void UnlinkSyntaxMap(SYNTAXMAP *theMap);
bool LinkSyntaxMap(SYNTAXMAP *theMap);
void CloseSyntaxMap(SYNTAXMAP *theMap);
SYNTAXMAP *OpenSyntaxMap(char *theName);
EDITORBUFFER *LocateNextEditorBufferOnMap(SYNTAXMAP *theMap,EDITORBUFFER *theBuffer);
SYNTAXMAP *GetAssignedSyntaxMap(EDITORBUFFER *theBuffer);
bool AssignSyntaxMap(EDITORBUFFER *theBuffer,SYNTAXMAP *theMap);
void UnInitSyntaxMaps();
bool InitSyntaxMaps();
