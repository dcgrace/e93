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

# ----------------------------------------------------
# Machine specific portion of make process for e93
# You may need to edit this file to get e93 to compile
# for your platform
# This will be made more automatic in the future. At
# the moment, it is set up for most Linux machines...
# ----------------------------------------------------

# Set path used to install e93
# You may wish to change this to /usr
# or to "~"
#
# e93 will install its single executable "e93" in $PREFIX/bin
# and it will place a directory called "e93lib" in $PREFIX/lib
PREFIX=/usr/local


# The following lines may need to be altered if the Tcl
# files are located elsewhere on your system:
TCL_INCLUDE=-I/usr/include
TCL_LIB=-L/lib
# uncomment and change if you want to link with specific versions of Tcl/Tk
TCL_VERSION=8.0


# The following lines may need to be altered if the X include and library
# files are located elsewhere on your system:
X_INCLUDE=-I/usr/X11R6/include
X_LIB=-L/usr/X11R6/lib


# if your system needs some extra libraries, add them here:
# EXTRALIBS=-lm -ldl

# Uncomment this line if running Solaris:
#MACHINESPEC=-DSOLARIS

# set compiler to use
CC=gcc


# set some compiler options
OPTIONS=-O2 -Wall -x c++ -g
#OPTIONS = -Wall -O2 -x c++ -mcpu=21164a -Wa,-m21164a -g
