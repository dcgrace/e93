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

typedef struct variableBinding VARIABLEBINDING;

char *GetVariableBindingName(VARIABLEBINDING *theBinding);
char *GetVariableBindingText(VARIABLEBINDING *theBinding);
VARIABLEBINDING *LocateNextVariableBinding(VARIABLEBINDING *currentBinding);
VARIABLEBINDING *LocateVariableBinding(VARIABLETABLE *theVariableTable,char *variableName);
bool DeleteVariableBinding(VARIABLETABLE *theVariableTable,char *variableName);
bool CreateVariableBinding(VARIABLETABLE *theVariableTable,char *variableName,char *variableText);
bool InitVariableBindingTable(VARIABLETABLE *theVariableTable);
void UnInitVariableBindingTable(VARIABLETABLE *theVariableTable);
