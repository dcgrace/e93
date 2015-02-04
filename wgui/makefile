#	This file is part of e93.
#
#	e93 is free software; you can redistribute it and/or modify
#	it under the terms of the e93 LICENSE AGREEMENT.
#
#	e93 is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	e93 LICENSE AGREEMENT for more details.
#
#	You should have received a copy of the e93 LICENSE AGREEMENT
#	along with e93; see the file "LICENSE.TXT".
#

TCL_DIR=C:\Program Files (x86)\Tcl

!include ../e93make.mak

LIB32=LIB
LIB32OPTS=/NOLOGO /MACHINE:IX86 /OUT:$(LIB_FILE) 
RESCOMP=RC


RES_FILE=e93

LIB_FILE=WGUI.LIB

LIB_OBJS=\
	clipbrd.obj\
	colors.obj\
	dialogs.obj\
	docwin.obj\
	events.obj\
	fileio.obj\
	fonts.obj\
	globals.obj\
	init.obj\
	kill.obj\
	memfuncs.obj\
	menus.obj\
	misc.obj\
	popmenu.obj\
	printer.obj\
	registry.obj\
	statusbr.obj\
	tasks.obj\
	views.obj\
	wintcl.obj

all: $(LIB_FILE) $(RES_FILE).res $(LIB_OBJS)

$(LIB_FILE) : $(LIB_OBJS)
	$(LIB32) @<<
		$(LIB32OPTS) $(LIB_OBJS)
<<

.c.obj:
	$(CC) $(CCOPTS) $<

$(RES_FILE).res : $(RES_FILE).rc
	$(RESCOMP) $(RES_FILE).rc


