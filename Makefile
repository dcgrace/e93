# Copyright (C) 1999, 2000 Core Technologies.
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


# NOTE: This Makefile should be platform independent...
# all machine specific changes should be made in the file
# "machdef.mk"

# Include machine/platform specific information
include machdef.mk

CFLAGS=-I. $(TCL_INCLUDE) $(X_INCLUDE) $(OPTIONS) $(MACHINESPEC)

OBJECTS = \
	e93.o \
	globals.o \
	tokens.o \
	shell.o \
	shellcmd.o \
	channels.o \
	buffer.o \
	docwin.o \
	edit.o \
	select.o \
	sparsearray.o \
	style.o \
	syntax.o \
	search.o \
	regex.o \
	clipbrd.o \
	undo.o \
	carray.o \
	text.o \
	varbind.o \
	keybind.o \
	errors.o \
	list.o \
	dictionary.o

all : libgui e93 docs

e93 : $(OBJECTS) xgui/libgui.a
	$(CC) -O $(OBJECTS) -Lxgui -lgui \
		$(X_LIB) -lX11 \
		$(TCL_LIB) -ltcl$(TCL_VERSION) -ltk$(TCL_VERSION) \
		$(EXTRALIBS) \
		-o e93

libgui :
	cd xgui;make

docs :
	cd docsource;tclsh makedocs.tcl

clean :
	cd xgui;make clean
	rm -f *.o
	rm -f e93

$(PREFIX)/bin/e93 : e93

install : $(PREFIX)/bin/e93
	cp e93 $(PREFIX)/bin
	cp e93r $(PREFIX)/bin
	mkdir -p $(PREFIX)/lib/e93lib
	cp -r e93lib $(PREFIX)/lib/
	rm -f $(PREFIX)/lib/e93lib/README.e93
	cp README.e93 $(PREFIX)/lib/e93lib
	chmod 444 $(PREFIX)/lib/e93lib/README.e93
	rm -f $(PREFIX)/lib/e93lib/README.regex
	cp README.regex $(PREFIX)/lib/e93lib
	chmod 444 $(PREFIX)/lib/e93lib/README.regex
	rm -f $(PREFIX)/lib/e93lib/README.syntaxmaps
	cp README.syntaxmaps $(PREFIX)/lib/e93lib
	chmod 444 $(PREFIX)/lib/e93lib/README.syntaxmaps

install-strip :	install
	strip $(PREFIX)/bin/e93
