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


// GUI includes

// standard C stuff

#include	<stdio.h>
#include	<stdarg.h>
#include	<stdlib.h>
#include	<errno.h>
#define		_BSD_SIGNALS
#include	<signal.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<time.h>
#include	<ctype.h>
#include	<memory.h>
#include	<string.h>
#include	<limits.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<dirent.h>
#include	<pwd.h>

// Xlib stuff

#include	<X11/Xatom.h>
#include	<X11/Intrinsic.h>
#include	<X11/Shell.h>
#include	<X11/Xlib.h>
#include	<X11/Xutil.h>
#include	<X11/cursorfont.h>
#include	<X11/keysym.h>

// Tcl stuff

#include	<tcl.h>

// high level e93 stuff

#include	"../defines.h"
#include	"../guidefs.h"

// prototypes, etc for our routines

#include	"fonts.h"
#include	"colors.h"
#include	"dialogs.h"
#include	"init.h"
#include	"pixmap.h"
#include	"events.h"
#include	"menus.h"
#include	"docwin.h"
#include	"window.h"
#include	"views.h"
#include	"tasks.h"
#include	"select.h"
#include	"misc.h"
#include	"memfuncs.h"
#include	"glob.h"

#include	"button.h"
#include	"checkbox.h"
#include	"scrollbar.h"
#include	"stattext.h"
#include	"textbox.h"
#include	"listbox.h"
#include	"separate.h"
#include	"gui.h"
#include	"shellcmd.h"

#include	"globals.h"
