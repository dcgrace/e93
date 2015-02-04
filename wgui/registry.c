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


#include "includes.h"

bool OpenRegistry(HKEY baseKey,char *keyName,HKEY *returnKeyPtr)
/*	Passed a keyname, open the key setting returnKeyPtr to the key handle, and return TRUE.
	If there was an error opening the key, return FALSE. This will create a new key if the
	key name does not already exist.
 */
{
	DWORD
		dwDisposition;
	DWORD
		errorNum;

	if(ERROR_SUCCESS==(errorNum=RegCreateKeyEx(baseKey,keyName,0,"",REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,returnKeyPtr,&dwDisposition)))
	{
		if(dwDisposition==REG_CREATED_NEW_KEY || dwDisposition==REG_OPENED_EXISTING_KEY)
		{
			return(true);
		}
		else
		{
			RegCloseKey(*returnKeyPtr);
		}
	}
	else
	{
		SetLastError(errorNum);
		SetWindowsError();
	}
	return(false);
}

void CloseRegistry(HKEY theKey)
/*	Passed a handle to a key, close it. The handle will no longer be valid, and any data write to the
	registry will be saved to disk when system gets around to it, or when the user shuts down
 */
{
	RegCloseKey(theKey);
}

bool GetRegistryDataSize(HKEY theKey,char *valueName,unsigned long *size)
/*	Passed a key, set 'size' to the number of bytes that 'valueName' is and return true.
	If there was an error, return FALSE.
 */
{
	DWORD
		valType;
	DWORD
		errorNum;

	if(ERROR_SUCCESS==(errorNum=RegQueryValueEx(theKey,valueName,0,&valType,NULL,size)))
	{
		return(true);
	}
	else
	{
		SetLastError(errorNum);
		SetWindowsError();
	}
	return(false);
}

bool ReadRegistryBinaryData(HKEY theKey,char *valueName,unsigned char *theData,unsigned long numBytes)
/*	Passed a key, read 'numBytes' of data into 'theData' from the 'valueName' entry and return TRUE.
	If there was an error, return FALSE.
 */
{
	DWORD
		valType;
	DWORD
		errorNum;

	if(ERROR_SUCCESS==(errorNum=RegQueryValueEx(theKey,valueName,0,&valType,theData,&numBytes)))
	{
		return(true);
	}
	else
	{
		SetLastError(errorNum);
		SetWindowsError();
	}
	return(false);
}

bool WriteRegistryBinaryData(HKEY theKey,char *valueName,unsigned char *theData,int numBytes)
/*	Passed a key, write 'numBytes' of data from 'theData' to the registry entry 'valueName'and return TRUE.
	If there was an error, return FALSE.
 */
{
	DWORD
		errorNum;

	if(ERROR_SUCCESS==(errorNum=RegSetValueEx(theKey,valueName,0,REG_BINARY,theData,numBytes)))
	{
		return(true);
	}
	else
	{
		SetLastError(errorNum);
		SetWindowsError();
	}
	return(false);
}


bool ReadRegistryStringData(HKEY theKey,char *valueName,char *theData,unsigned long numBytes)
/*	Passed a key, read 'numBytes' of data into 'theData' from the 'valueName' entry and return TRUE.
	If there was an error, return FALSE.
 */
{
	DWORD
		valType;
	DWORD
		errorNum;

	if(ERROR_SUCCESS==(errorNum=RegQueryValueEx(theKey,valueName,0,&valType,(unsigned char *)theData,&numBytes)))
	{
		return(true);
	}
	else
	{
		SetLastError(errorNum);
		SetWindowsError();
	}
	return(false);
}

bool WriteRegistryStringData(HKEY theKey,char *valueName,unsigned char *theData,int numBytes)
/*	Passed a key, write 'numBytes' of data from 'theData' to the registry entry 'valueName'and return TRUE.
	If there was an error, return FALSE.
 */
{
	DWORD
		errorNum;

	if(ERROR_SUCCESS==(errorNum=RegSetValueEx(theKey,valueName,0,REG_SZ,theData,numBytes)))
	{
		return(true);
	}
	else
	{
		SetLastError(errorNum);
		SetWindowsError();
	}
	return(false);
}

bool RegistryKeyExistes(HKEY baseKey,char *keyName)
/*	Passed a registry key name, return TRUE if it exists, else FALSE if not.
 */
{
	HKEY
		theKey;
	DWORD
		errorNum;

	if(ERROR_SUCCESS==(errorNum=RegOpenKeyEx(baseKey,keyName,0,KEY_ALL_ACCESS,&theKey)))
	{
		RegCloseKey(theKey);
		return(true);
	}
	else
	{
		SetLastError(errorNum);
		SetWindowsError();
	}
	return(false);
}

bool DeleteRegistryEntry(HKEY baseKey,char *keyRoot,char *keyName)
/*	Passed a key root name and a key name inside it, delete the key name, and delete the root entry
	if there are no other subkeys in it and return TRUE.
	If there was an error, return FALSE.

 */
{
	HKEY
		theKey;
	char
		rClass[256];
	DWORD
		errorNum,
		lpcchClass,
		lpcSubKeys,
		lpcchMaxSubkey,
		lpcchMaxClass,
		lpcValues,
		lpcchMaxValueName,
		lpcbMaxValueData,
		lpcbSecurityDescriptor;
	FILETIME
		lpftLastWriteTime;
	bool
		result;

	result=false;
	if(RegistryKeyExistes(baseKey,keyRoot))
	{
		if(OpenRegistry(baseKey,keyRoot,&theKey))
		{
    		lpcchClass=255;
			if(ERROR_SUCCESS==(errorNum=RegQueryInfoKey(theKey,rClass,&lpcchClass,0,&lpcSubKeys,&lpcchMaxSubkey,&lpcchMaxClass,&lpcValues,&lpcchMaxValueName,&lpcbMaxValueData,&lpcbSecurityDescriptor,&lpftLastWriteTime)))
			{
				if(ERROR_SUCCESS==(errorNum=RegDeleteKey(theKey,keyName)))
				{
					result=true;
				}
				else
				{
					SetLastError(errorNum);
					SetWindowsError();
				}
			}
			else
			{
				SetLastError(errorNum);
				SetWindowsError();
			}
			CloseRegistry(theKey);
			if(result && lpcSubKeys==1)
			{
				if(ERROR_SUCCESS!=RegDeleteKey(baseKey,keyRoot))
				{
					SetLastError(errorNum);
					SetWindowsError();
					result=false;
				}
			}
		}
	}
	return(result);
}

bool ReadRegistrySettings(INT32 *xPos,INT32 *yPos,INT32 *width,INT32 *height,bool *maximized,bool *maximizedMDI,bool *openE93,bool *dataInRegistry)
/*	Read the registry settings for the MDI frame windows pos, size, and maximize state, return TRUE is no error,
	else FALSE. 'dataInRegistry' gets set TRUE if there is data.
 */
{
	HKEY
		theKey;
	INT32
		dataArray[2];
	bool
		result;

	result=false;
	(*dataInRegistry)=false;
	if(RegistryKeyExistes(HKEY_CURRENT_USER,E93SUBKEYNAME))
	{
		if(OpenRegistry(HKEY_CURRENT_USER,E93SUBKEYNAME,&theKey))
		{
			if(ReadRegistryBinaryData(theKey,E93KEYVALUENAME_APPWINDOW,(UINT8 *)dataArray,sizeof(INT32)*2))
			{
				(*xPos)=dataArray[0];
				(*yPos)=dataArray[1];
				if(ReadRegistryBinaryData(theKey,E93KEYVALUENAME_APPWINDOWSIZE,(UINT8 *)dataArray,sizeof(INT32)*2))
				{
					(*width)=dataArray[0];
					(*height)=dataArray[1];
					if(ReadRegistryBinaryData(theKey,E93KEYVALUENAME_MAXIMIZEAPP,(UINT8 *)dataArray,sizeof(INT32)*1))
					{
						(*maximized)=(dataArray[0]!=0);
						if(ReadRegistryBinaryData(theKey,E93KEYVALUENAME_MAXIMIZEMDI,(UINT8 *)dataArray,sizeof(INT32)*1))
						{
							(*maximizedMDI)=(dataArray[0]!=0);
							if(ReadRegistryBinaryData(theKey,E93KEYVALUENAME_OPENANOTHEREE93,(UINT8 *)dataArray,sizeof(INT32)*1))
							{
								(*openE93)=(dataArray[0]!=0);
								(*dataInRegistry)=true;
								result=true;
							}
						}
					}
				}
			}
			CloseRegistry(theKey);
		}
	}
	else
	{
		result=true;
	}
	return(result);
}

bool WriteRegistrySettings(INT32 xPos,INT32 yPos,INT32 width,INT32 height,bool maximized,bool maximizedMDI,bool openE93)
/*	Write the MDI frame window's pos, size, and maximized state to the registry.
 */
{
	HKEY
		theKey;
	INT32
		dataArray[2];
	bool
		result;

	result=false;
	if(OpenRegistry(HKEY_CURRENT_USER,E93SUBKEYNAME,&theKey))
	{
		dataArray[0]=xPos;
		dataArray[1]=yPos;
		if(WriteRegistryBinaryData(theKey,E93KEYVALUENAME_APPWINDOW,(UINT8 *)dataArray,sizeof(INT32)*2))
		{
			dataArray[0]=width;
			dataArray[1]=height;
			if(WriteRegistryBinaryData(theKey,E93KEYVALUENAME_APPWINDOWSIZE,(UINT8 *)dataArray,sizeof(INT32)*2))
			{
				dataArray[0]=maximized;
				if(WriteRegistryBinaryData(theKey,E93KEYVALUENAME_MAXIMIZEAPP,(UINT8 *)dataArray,sizeof(INT32)*1))
				{
					dataArray[0]=maximizedMDI;
					if(WriteRegistryBinaryData(theKey,E93KEYVALUENAME_MAXIMIZEMDI,(UINT8 *)dataArray,sizeof(INT32)*1))
					{
						dataArray[0]=openE93;
						if(WriteRegistryBinaryData(theKey,E93KEYVALUENAME_OPENANOTHEREE93,(UINT8 *)dataArray,sizeof(INT32)*1))
						{
							result=true;
						}
					}
				}
			}
		}
		CloseRegistry(theKey);
	}
	return(result);
}

bool ReadOpenAnotherE93RegistrySetting(bool *openE93)
{
	HKEY
		theKey;
	INT32
		dataArray[2];
	bool
		result;

	result=false;
	if(RegistryKeyExistes(HKEY_CURRENT_USER,E93SUBKEYNAME))
	{
		if(OpenRegistry(HKEY_CURRENT_USER,E93SUBKEYNAME,&theKey))
		{
			if(ReadRegistryBinaryData(theKey,E93KEYVALUENAME_OPENANOTHEREE93,(UINT8 *)dataArray,sizeof(INT32)*1))
			{
				(*openE93)=(dataArray[0]!=0);
				result=true;
			}
			CloseRegistry(theKey);
		}
	}
	else
	{
		result=true;
	}
	return(result);
}

bool WriteOpenAnotherE93RegistrySetting(bool openE93)
{
	HKEY
		theKey;
	INT32
		dataArray[2];
	bool
		result;

	result=false;
	if(OpenRegistry(HKEY_CURRENT_USER,E93SUBKEYNAME,&theKey))
	{
		dataArray[0]=openE93;
		if(WriteRegistryBinaryData(theKey,E93KEYVALUENAME_OPENANOTHEREE93,(UINT8 *)dataArray,sizeof(INT32)*1))
		{
			result=true;
		}
		CloseRegistry(theKey);
	}
	return(result);
}

bool WriteMDIShouldZoomRegistrySetting(bool zoomed)
{
	HKEY
		theKey;
	INT32
		dataArray[2];
	bool
		result;

	result=false;
	if(OpenRegistry(HKEY_CURRENT_USER,E93SUBKEYNAME,&theKey))
	{
		dataArray[0]=zoomed;
		if(WriteRegistryBinaryData(theKey,E93KEYVALUENAME_MAXIMIZEMDI,(UINT8 *)dataArray,sizeof(INT32)*1))
		{
			result=true;
		}
		CloseRegistry(theKey);
	}
	return(result);
}

bool ReadRegistryString(HKEY baseKey,char *keyPath,char *valName,char *strPtr,UINT32 strLength)
/*	Passed a key path, and a value name, read the string data from that value into strPtr, and
	return TRUE. If the path or value name doesn't exist, return FALSE.
 */
{
	HKEY
		theKey;
	bool
		result;

	result=false;
	if(RegistryKeyExistes(baseKey,keyPath))
	{
		if(OpenRegistry(baseKey,keyPath,&theKey))
		{
			if(ReadRegistryStringData(theKey,valName,strPtr,strLength))
			{
				result=true;
			}
			CloseRegistry(theKey);
		}
	}
	return(result);
}

bool WriteRegistryString(HKEY baseKey,char *keyPath,char *valName,char *strPtr)
/*	Passed a key path, and a value name, write the string into the value name. If the value name
	had existing data, it is replaced. If no value name existed, it does now. Return TRUE if no error, else
	FALSE.
 */
{
	HKEY
		theKey;
	bool
		result;

	result=false;
	if(OpenRegistry(baseKey,keyPath,&theKey))
	{
		if(WriteRegistryStringData(theKey,valName,(unsigned char *)strPtr,strlen(strPtr)+1))
		{
			result=true;
		}
		CloseRegistry(theKey);
	}
	return(result);
}
