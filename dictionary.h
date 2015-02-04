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


typedef List Dictionary;

Dictionary *CreateDictionary();
void DeleteDictionary(Dictionary *theTable);
void *GetFromDictionary(Dictionary *theTable,const char *key);
bool AddToDictionary(Dictionary *theTable,const char *key,void *value);
void RemoveFromDictionary(Dictionary *theTable,const char *key);
List *GetKeysFromDictionary(Dictionary *theTable);
