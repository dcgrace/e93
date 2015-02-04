package provide [file tail [info script]] 1

package require vc.tcl
package require scratch.tcl

# Useful functions for C programmers

# Locate all things which look like function definitions in C, collect them up, and
# dump them into a new window as prototypes.
proc GetCPrototypes {theWindow} \
{
	setmark $theWindow temp;									# remember what was selected, we will not disturb it
	TextToBuffer tempFindBuffer {^[A-Za-z][^(;,\n]+\([^\n]*\)$};	# load up the expression
	setselectionends $theWindow 0 0;							# move to the top of the buffer to perform the search
	if {[catch {findall $theWindow tempFindBuffer -regex} message]==0} \
	{
		if {$message!=-1} \
		{
			set theName [newbuffer [NewWindowName]];			# make a buffer to hold the prototypes
			copy $theWindow $theName;							# get selected prototypes into a buffer
			setselectionends $theName 0 0;						# move to the top of the prototypes buffer
			TextToBuffer tempFindBuffer {^[ \t]*static.*\n};	# get rid of any static functions
			TextToBuffer tempReplaceBuffer {};
			catch {replaceall $theName tempFindBuffer tempReplaceBuffer -regex} message;	# do this, ignore any errors
			setselectionends $theName 0 0;						# move to the top of the prototypes buffer
			TextToBuffer tempFindBuffer {\)[\t ]*$};			# make all parens at end of line have ;'s
			TextToBuffer tempReplaceBuffer {);};
			catch {replaceall $theName tempFindBuffer tempReplaceBuffer -regex} message;	# do this, ignore any errors

			set end [lindex [textinfo $theName] 1];				# get position of end of buffer
			# see if the last line is terminated with a new-line, if not, add one
			if {[lindex [positiontolineoffset $theName $end] 1]!=0} \
			{
				setselectionends $theName $end $end;			# move to the end of the prototypes buffer
				insert $theName "\n";							# terminate the last line
			}

			setselectionends $theName 0 0;						# move to the top of the prototypes buffer
			flushundos $theName;								# get rid of any undos we made here, just to be nice
			cleardirty $theName;								# make it non-modified
			OpenDefaultWindow $theName;							# give it to the user
			SetHighlightMode $theName "C/C++";					# make it look like C code
		} \
		else \
		{
			beep
		}
	} \
	else \
	{
		okdialog $message
	}
	gotomark $theWindow temp;									# put back the user's selection
	clearmark $theWindow temp;									# get rid of temp selection
}

# Locate all function like lines, bring them up in a list box, allowing the
# user to choose one, then move to the chosen one.
# NOTE: this breaks relatively easily, and does not work for some people's preferred
# function syntax.... It could probably stand to be redone.
# If you make a better version, send it to squirest@e93.org, and I will include
# it in the next release.
proc SelectCFunction {theWindow} \
{
	setmark $theWindow temp;									# remember what was selected, we will not disturb it
	TextToBuffer tempFindBuffer {^[A-Za-z][^(;,\n]+\([^\n]*\)$};	# load up the expression
	setselectionends $theWindow 0 0;							# move to the top of the buffer to perform the search
	if {[catch {findall $theWindow tempFindBuffer -regex} message]==0} \
	{
		if {$message!=-1} \
		{
			catch {listdialog "Choose a Function:" [selectedtextlist $theWindow]} theChoice
			if {[llength $theChoice]>=1} \
			{
				TextToBuffer tempFindBuffer [lindex $theChoice 0]
				setselectionends $theWindow 0 0;				# move to the top of the buffer to perform the search
				catch {find $theWindow tempFindBuffer} message
				homewindow $theWindow strict
			} \
			else \
			{
				gotomark $theWindow temp;						# put back the user's selection
			}
		} \
		else \
		{
			beep
			gotomark $theWindow temp;							# put back the user's selection
		}
	} \
	else \
	{
		okdialog $message
		gotomark $theWindow temp;								# put back the user's selection
	}
	clearmark $theWindow temp;									# get rid of temp selection
}

# Paste all of the selections in the clipboard as case statements in 'C'
# into theWindow.
# This is handy when columnar selecting a bunch of enumerations, or
# defines to build a large switch statement from.
proc CasePaste {theWindow} \
{
	breakundo $theWindow
	foreach theItem [selectedtextlist [getclipboard]] \
	{
		insert $theWindow "case $theItem:\n\tbreak;\n"
	}
	breakundo $theWindow
}

# This list var is used by "GrepIncludes" and "SmartOpenList"
#
# be careful that the include paths are in the same order that your compiler searches for them
switch $::tcl_platform(platform) \
{
	"unix" \
	{
		set includePathList /usr/include/
	}
	"windows" \
	{
		set paths {}
		# get the includes from the env var
		catch {global paths; regsub -all {;} $env(INCLUDE) { } paths}
		regsub -all "\\\\" $paths "/" paths
		set includePathList $paths
	}
}

# Given an "filename" from a list, try to interpret it as a 'C' include
# statement:
# #include <filename>	(looks in /usr/include)
# #include "filename"	(looks in current directory)
# If it looks like that, then extract the filename
# and return it, otherwise return an error
proc SmartOpenListInclude {listItem} \
{
	if {[regexp "^\[ \\t\]*#include\[ \\t\]+\\<(\[^>\]+)\\>" $listItem whole tempFile]} \
	{
		global env includePathList
		foreach thePath $includePathList \
		{
			if {[file exists $thePath/$tempFile]} \
			{
				lappend temp "$thePath/$tempFile"
				break;
			}
		}
		lappend temp2 $temp;				# return list of lists
		return $temp2;
	} \
	else \
	{
		if {[regexp "^\[ \\t\]*#include\[ \\t\]+\"(\[^\"\]+)\"" $listItem whole tempFile]} \
		{
			lappend temp $tempFile;
			lappend temp2 $temp;			# return list of lists
			return $temp2;
		}
	}
	return -code error;
}

proc isupper {aString} \
{
	if {[string toupper $aString] == $aString} \
	{
		return 1
	}
	return 0
}

proc OpenImplementationFile {theWindow} \
{
	set rootname [file rootname $theWindow]
	set extensionList ".cpp .ipp .imp .c .m"
	set found 0
	
	if {[isupper [file extension $theWindow]] == 1} \
	{
		set extensionList [string toupper $extensionList]
	}

	foreach extension $extensionList \
	{
		if {[file exists $rootname$extension]} \
		{
		OpenDefaultWindow [openbuffer $rootname$extension]
		set found 1
		}
	}
	if {!$found} \
	{
		beep
	}
}

proc SwapHeader {theWindow} \
{
	switch [file extension $theWindow]\
	{
		".cpp"		{OpenDefaultWindow [openbuffer [file rootname $theWindow].h]}
		".c"		{OpenDefaultWindow [openbuffer [file rootname $theWindow].h]}
		".C"		{OpenDefaultWindow [openbuffer [file rootname $theWindow].H]}
		".m"		{OpenDefaultWindow [openbuffer [file rootname $theWindow].h]}
		".ipp"		{OpenDefaultWindow [openbuffer [file rootname $theWindow].h]}
		".h"		{OpenImplementationFile $theWindow}
		".H"		{OpenImplementationFile $theWindow}
		default		{beep}
	}
}

# grep the include path for searchData
# grep the include path for searchData
proc GrepIncludes {searchData} \
{
	# global env
	# set inc [array names env]
	# set thePathList [split $env([lindex $inc [lsearch -regexp $inc "include|Include|INCLUDE"]]) \;]

	global includePathList
	set theData "grep -n $searchData"

	foreach thePathItem $includePathList \
	{
		set theData "$theData $thePathItem/*.H"
		set theData "$theData $thePathItem/*.h"
	}
	set theWindow [NewGrepWindow $theData]
	task $theWindow $theData
}

proc GrepExtension {} \
{
	set theDrive [pathdialog "Path to search in:"]
	set theExt [textdialog "What extension"]
	set theGrep [textdialog "What to grep for"]
	set theWindow [NewGrepWindow]
	set globData *
	set theItem 0
	while {$theItem<8} \
	{
		if {[catch {set outData [glob "$theDrive/$globData.$theExt"]} message]==0} \
		{
			foreach thePathItem $outData \
			{
				set theData "grep -y -n $theGrep $thePathItem"

				breakundo $theWindow
				insert $theWindow $thePathItem:
				set ends [getselectionends $theWindow]
				set start1 [lindex $ends 0]
				set end1 [lindex $ends 1]
				task $theWindow $theData
				while {[hastask $theWindow]} \
				{
					updatetask $theWindow
				}
				set ends [getselectionends $theWindow]
				set start2 [lindex $ends 0]
				set end2 [lindex $ends 1]
				if {$start1==$start2 && $end1==$end2} \
				{
					undo $theWindow
				}
			}
		}
		set globData */$globData
		incr theItem
	}
}

proc GlobExtension {} \
{
	set theDrive [textdialog "Drive to search in:"]
	set theExt [textdialog "What extension"]
	set globData *
	set theItem 0
	set theWindow [NewGrepWindow]
	while {$theItem<12} \
	{
		if {[catch {set outData [glob "$theDrive:/$globData.$theExt"]} message]==0} \
		{
			foreach thePathItem $outData \
			{
				insert $theWindow $thePathItem\n
			}
		}
		set globData */$globData
		incr theItem
	}
}

# grep the include path for searchData
proc GrepSource {searchData} \
{
	global env
	set theWindow [NewGrepWindow]
	set inc [array names env]
	set thePathList [split $env([lindex $inc [lsearch -regexp $inc "src|Src|SRC"]]) \;]
	set theData "grep -n $searchData"
	foreach thePathItem $thePathList \
		{
		set theData "$theData $thePathItem\\*.c"
		}
	task $theWindow $theData
}

addmenu {} LASTCHILD 1 "C" "" ""
	addmenu {C} LASTCHILD 1 "Get Prototypes"				{}					{GetCPrototypes [ActiveWindowOrBeep]}
	addmenu {C} LASTCHILD 1 "Locate Function..."			{}					{SelectCFunction [ActiveWindowOrBeep]}
	addmenu {C} LASTCHILD 0 "space1"						{\S}				{}
	addmenu {C} LASTCHILD 1 "Case Paste"					{}					{CasePaste [ActiveWindowOrBeep]}
	addmenu {C} LASTCHILD 1 "Swap header/impementation"		{\\Ke}				{SwapHeader [ActiveWindowOrBeep]}
	addmenu {C} LASTCHILD 0 "space2"						{\S}				{}
	addmenu {C} LASTCHILD 1 "Grep C Includes"				{}					{GrepIncludes [selectedtextlist [ActiveWindowOrBeep]]}
	addmenu {C} LASTCHILD 1 "Glob Extension"				{}					{GlobExtension}
	addmenu {C} LASTCHILD 1 "Grep Extension"				{}					{GrepExtension}

set syntaxmapname "C"

# should define C syntaxmap here

# add our C language mode to the menu
# addmenu {Window "Language Mode"} LASTCHILD 1 $syntaxmapname		{}			"SetHighlightMode \[ActiveWindowOrBeep\] $syntaxmapname"

# we want e93 to include .c as file extensions it shows in the open panel
append filetypes "\t{{Source Files}			{.c}						TEXT}\n" 
append filetypes "\t{{C Source Files}		{.c}						TEXT}\n"

append filetypes "\t{{Source Files}			{.cpp}						TEXT}\n" 
append filetypes "\t{{C++ Source Files}		{.cpp}						TEXT}\n"

append filetypes "\t{{Source Files}			{.h}						TEXT}\n"
append filetypes "\t{{Header Files}			{.h}						TEXT}\n"

append filetypes "\t{{Source Files}			{.m}						TEXT}\n"
append filetypes "\t{{Obj-C Source Files}	{.m}						TEXT}\n"


