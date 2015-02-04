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


// all the includes needed to compile e93

// system include files

#include	<stdio.h>
#include	<stdarg.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<time.h>
#include	<ctype.h>
#include	<memory.h>
#include	<string.h>
#include	<limits.h>
#include	<locale.h>

#include	<tcl.h>
#include	<tk.h>

#include	"defines.h"
#include	"guidefs.h"
#include	"tokens.h"
#include	"shell.h"
#include	"shellcmd.h"
#include	"channels.h"
#include	"buffer.h"
#include	"docwin.h"
#include	"syntax.h"
#include	"edit.h"
#include	"select.h"
#include	"sparsearray.h"
#include	"style.h"
#include	"regex.h"
#include	"search.h"
#include	"clipbrd.h"
#include	"undo.h"
#include	"carray.h"
#include	"text.h"
#include	"varbind.h"
#include	"keybind.h"
#include	"errors.h"
#include	"globals.h"
#include	"list.h"
#include	"dictionary.h"
