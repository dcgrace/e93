# Copyright (C) 1999, 2000 Core Technologies
#
# This file is part of e93.
#
# e93 is free software; you can redistribute it and/or modify
# it under the terms of the e93 LICENSE AGREEMENT.
#
# e93 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# e93 LICENSE AGREEMENT for more details.
#
# You should have received a copy of the e93 LICENSE AGREEMENT
# along with e93; see the file "LICENSE.TXT".


# Include machine/platform specific information into the build
include ../machdef.mk

CFLAGS=-I. $(TCL_INCLUDE) $(X_INCLUDE) $(OPTIONS) $(MACHINESPEC) -DPREFIX=\"$(PREFIX)\"

OBJECTS = \
	globals.o \
	init.o \
	pixmap.o \
	fonts.o \
	colors.o \
	events.o \
	menus.o \
	docwin.o \
	window.o \
	views.o \
	tasks.o \
	scrollbar.o \
	dialogs.o \
	button.o \
	checkbox.o \
	textbox.o \
	listbox.o \
	stattext.o \
	separate.o \
	select.o \
	misc.o \
	memfuncs.o \
	glob.o \
	shellcmd.o

libgui.a : $(OBJECTS)
	ld -r $(OBJECTS) -o libgui.a

clean :
	rm -f *.o
	rm -f libgui.a
