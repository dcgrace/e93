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


#define	E93SUBKEYNAME	"SOFTWARE\\e93"
#define	E93CLASSNAME	""

#define	E93KEYVALUENAME_APPWINDOW		"AppWindow"
#define	E93KEYVALUENAME_APPWINDOWSIZE	"AppWindowSize"
#define	E93KEYVALUENAME_MAXIMIZEAPP		"MaximizeApp"
#define	E93KEYVALUENAME_MAXIMIZEMDI		"MaximizeMDI"
#define	E93KEYVALUENAME_OPENANOTHEREE93	"OpenAnothereE93"

bool OpenRegistry(HKEY baseKey,char *keyName,HKEY *returnKeyPtr);
void CloseRegistry(HKEY theKey);
bool GetRegistryDataSize(HKEY theKey,char *valueName,unsigned long *size);
bool ReadRegistryBinaryData(HKEY theKey,char *valueName,unsigned char *theData,unsigned long numBytes);
bool WriteRegistryBinaryData(HKEY theKey,char *valueName,unsigned char *theData,int numBytes);
bool ReadRegistryStringData(HKEY theKey,char *valueName,char *theData,unsigned long numBytes);
bool WriteRegistryStringData(HKEY theKey,char *valueName,unsigned char *theData,int numBytes);
bool RegistryKeyExistes(HKEY baseKey,char *keyName);
bool DeleteRegistryEntry(HKEY baseKey,char *keyRoot,char *keyName);
bool ReadRegistrySettings(INT32 *xPos,INT32 *yPos,INT32 *width,INT32 *height,bool *maximized,bool *maximizedMDI,bool *openE93,bool *dataInRegistry);
bool WriteRegistrySettings(INT32 xPos,INT32 yPos,INT32 width,INT32 height,bool maximized,bool maximizedMDI,bool openE93);
bool ReadOpenAnotherE93RegistrySetting(bool *openE93);
bool WriteOpenAnotherE93RegistrySetting(bool openE93);
bool ReadRegistryString(HKEY baseKey,char *keyPath,char *valName,char *strPtr,UINT32 strLength);
bool WriteRegistryString(HKEY baseKey,char *keyPath,char *valName,char *strPtr);
bool WriteMDIShouldZoomRegistrySetting(bool zoomed);