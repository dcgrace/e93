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


void SplitPathAndFile(char *fullPath,char *pathPart,char *filePart);
void ConcatPathAndFile(char *thePath,char *theFile,char *theResult);
void FreeStringList(char **theList);
void SortStringList(char **theList,UINT32 numElements);
bool ReplaceStringInList(char *theString,char **theList,UINT32 theElement);
bool AddStringToList(char *theString,char ***theList,UINT32 *numElements);
char **NewStringList();
char **Glob(char *thePath,char *thePattern,bool dontGlobLast,bool passNoMeta,UINT32 *numElements);
char **GlobAll(char *thePath,UINT32 *numElements);
