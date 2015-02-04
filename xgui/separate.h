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


// defines and data structures for separator

typedef struct separator
{
	EDITORWINDOW
		*parentWindow;
	bool
		horizontal;												// true if separator is horizontal
	INT32
		index,													// if horizontal, then Y, else X
		start,													// if horizontal, then X, else Y
		end;
} SEPARATOR;

typedef struct separatorDescriptor								// describes separator creation routines
{
	bool
		horizontal;												// true if separator is horizontal
	INT32
		index,													// if horizontal, then Y, else X
		start,													// if horizontal, then X, else Y
		end;
} SEPARATORDESCRIPTOR;

bool CreateSeparatorItem(DIALOGITEM *theItem,void *theDescription);
