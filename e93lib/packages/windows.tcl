package provide [file tail [info script]] 1

# Things which are specific to e93 on the Windows(TM) platform go in this file

set theCurPath "";									# current path is blank
set	minTick	60;										# set minTick (minutes) to unreachable value

windowssetglobalstatusbarfont "-*-courier new-medium-r-normal-*-10-*-*-*-*-*"
windowssetwindowstatusbarfont "-*-courier new-medium-r-normal-*-10-*-*-*-*-*"

# In a Windows MDI world, screen size is not fixed since the "screen" is the outer MDI window
# calculations based on screen size must be re-calculated before using them.
#
# If anyone tries to use the screenWidth or screenHeight variables, recalculate them first
# in case the outer MDI window has been resized.
trace variable screenWidth	r	UpdateGeometry
trace variable screenHeight	r	UpdateGeometry
trace variable windowWidth	r	UpdateGeometry
trace variable windowHeight	r	UpdateGeometry

proc UpdateGeometry {name element op} \
{
	uplevel #0 \
	{
		set	screenWidth				[lindex [screensize] 0]
		set	screenHeight			[lindex [screensize] 1]

		set	windowWidth				[expr $screenWidth*3/4];	# set the initial default width for new windows
		set windowHeight			[expr $screenHeight*2/3];	# set the initial default height for new windows

		set	staggerMaxX				[expr $screenWidth - $windowWidth]
		set	staggerMaxY				[expr $screenHeight - $windowHeight]
	}
}

# if the current working directory is different from theCurPath, display the new one and set theCurPath to it 
proc DisplayPath {} \
{
	global theCurPath
	if {$theCurPath!=[pwd]} \
	{
		set theCurPath [pwd]
		windowssetglobalstatusfield 4 [windowsgetstatusbarstringwidth $theCurPath] $theCurPath
	}
}

# display the current time of day, this gets the current time and if the minutes has changed
# since the last time we looked, display the new time
proc DisplayTOD {} \
{
	global	minTick

	set curMin [clock format [clock scan now] -format "%M"]
	# only update when the minute changes
	if {$minTick != $curMin} \
	{
		set minTick $curMin
		
		set curTime [clock format [clock scan now] -format "%H:%M %p"]
		windowssetglobalstatusfield 3 [windowsgetstatusbarstringwidth $curTime] $curTime
	}
}

# this is called be e93rc 1 time a second, for us to do any work we want to do
proc RespondToTimerEvent {} \
{
	DisplayTOD
	DisplayPath
}

# LocateFunctionsProc --
#
#		Search for lines that look like either Tcl or C function definitions, and
#		display them in a popup menu.  When the user selects one, go to the appropriate
#		line.
#
# Arguments:
#		theWindow			the buffer in which to search.
#
# Results:
#		The name of the function selected, or an empty string if none found.

proc LocateFunctionsProc {} \
{
	set theWindow [ActiveWindowOrBeep]
	# remember what was selected
	setmark $theWindow temp
	TextToBuffer tempFindBuffer {^[A-Za-z][^(;,\n]+\([^\n]*\)$};	# load up the expression
	# move to the top of the buffer to perform the search
	setselectionends $theWindow 0 0;							# move to the top of the buffer to perform the search
	if {[catch {findall $theWindow tempFindBuffer -regex} message]==0} \
	{
		# found at least one function-looking line - get the pertinent parts of each
		# to put in the popup menu
		set funcList	[selectedtextlist $theWindow]
		set menuList	{}

		foreach function $funcList \
		{
			if {[regexp {^proc[\t ]+([A-Za-z][^\{]*)|(^[A-Za-z][^(]+)} $function match]} \
			{
				lappend menuList [string trim [lindex $match end]]
			}
		}
		if {[set choice [windowsdopopupmenu $menuList [gotomark $theWindow temp]]] != ""} \
		{
			set function [lindex $funcList [eval lsearch \$menuList $choice]]
			TextToBuffer tempFindBuffer $function
			setselectionends $theWindow 0 0
			catch {find $theWindow tempFindBuffer} error
			HomeWindowToSelectionStart $theWindow -strict
		} \
		else \
		{
			# the user cancelled the menu without selecting anything
		}
		return $choice
	}
	# put back the user's selection
	gotomark $theWindow temp
	clearmark $theWindow temp
	return ""
}

proc ChooseWindowProc {} \
{
	set windows ""
	
	foreach fileName [windowlist] \
	{
		set tag "[file tail $fileName] -- [file dirname $fileName]"
		set files($tag) $fileName
		lappend windows $tag
	}

	set windows [lsort -dictionary $windows]
	# remove the un-needed crap windowsdopopupmenu adds with trim
	set tag [string trim [windowsdopopupmenu $windows] "\{\}"]
	if ![string equal $tag ""] \
	{
		set theWindow $files($tag) 
		settopwindow $theWindow
	}
}

proc ClearROFlagForBuffer {theWindow} \
{
	file attributes $theWindow -readonly 0 
}

proc RegisterExtensionsToe93 {} \
{	
	package require registry

	# setup the e93file type so we can have .c,.h,.asm,.... point to this instead of VC++
	registry set HKEY_CLASSES_ROOT\\e93file\\shell\\open\\command {} "[file dirname [info nameofexecutable]]/e93.exe \"%1\""
	registry set HKEY_CLASSES_ROOT\\e93file\\DefaultIcon {} [file dirname [info nameofexecutable]]/e93.exe,1

	registry set HKEY_CLASSES_ROOT\\.c {} e93file
	registry set HKEY_CLASSES_ROOT\\.h {} e93file
	registry set HKEY_CLASSES_ROOT\\.asm {} e93file
	registry set HKEY_CLASSES_ROOT\\.cpp {} e93file
	registry set HKEY_CLASSES_ROOT\\.java {} e93file
	registry set HKEY_CLASSES_ROOT\\.e93 {} e93file
}

windowssetglobalstatusbutton 0 [windowsgetstatusbarstringwidth "{}"] "{}" LocateFunctionsProc
windowssetglobalstatusbutton 1 [windowsgetstatusbarstringwidth "Files"] "Files" ChooseWindowProc

addmenu {Window} LASTCHILD 0 "space2"					{\S}				{}
addmenu {Window} LASTCHILD 1 "Cascade"					{}					{windowscascade}
addmenu {Window} LASTCHILD 1 "Tile Horizontally"		{}					{windowstilehorz}
addmenu {Window} LASTCHILD 1 "Tile Vertically"			{}					{windowstilevert}
addmenu {Window} LASTCHILD 1 "Arrange Icons"			{}					{windowsarrangeicons}

addmenu {Misc} LASTCHILD 1 "Clear RO Flag"				{}					{ClearROFlagForBuffer [ActiveWindowOrBeep]}

set filetranslationmode on;			# we are in PC land, translate all files

# put the e93 directory in the path
set inc [array names env]
set pathVar [lindex $inc [lsearch -regexp $inc "^path$|^Path$|^PATH$"]]
set env($pathVar) "[file dirname [info nameofexecutable]];$env($pathVar)"

proc PrintBuffer {bufferName} \
{
	windowsprint $bufferName
}

# Given an "filename" from a list, try to interpret it as the output from
# an GNU tool in a Microsoft world:
# C:/Folder/filename:nnnn
# If it looks like that, then extract the filename and line number
# and return them in a list, otherwise return an error
proc SmartOpenListWindows {listItem} \
{
	if {[regexp {^([^:]:[^:]+):([0-9]+)} $listItem whole tempFile tempLine]} \
	{
		lappend temp $tempFile $tempLine;	# make a list of 2 elements (filename and line number)
		lappend temp2 $temp;				# return list of lists
		return $temp2;
	}
	return -code error;
}

set	smartOpenProcs(Windows)			{SmartOpenListWindows}

#	rename openbuffer openbufferOrg

#	proc openbuffer {pathName} \
#	{
#		openbufferOrg [file attributes $pathName -shortname]
#	}

proc DisplayError {theError} \
{
	windowssetglobalstatusfield 5 [windowsgetstatusbarstringwidth $theError] $theError
}


addmenu {Window} LASTCHILD 1 "Rotate Windows 2"				{\\KTab}		{RotateWindows}

bindkey Insert		{x0010000000} {SmartCopy [ActiveWindowOrBeep];flushundos [getclipboard]}
bindkey Insert		{x1000000000} {HomeWindowToSelectionStart [set theWindow [ActiveWindowOrBeep]];paste $theWindow;HomeWindowToSelectionStart $theWindow -lenient}

proc HomeWindowToSelectionEnd {theWindow args} \
{
	set position [lindex [getselectionends $theWindow] 1];			# get selection end position
	eval [list homewindow $theWindow] $position $position $args;	# home to end
}

# command-shift-left selects to start of word
bindkey	Left		{x1010000000} {set theWindow [ActiveWindowOrBeep]; expandselection $theWindow LEFTWORD; HomeWindowToSelectionStart $theWindow -semistrict}
# command-left moves to start of word
bindkey	Left		{x0010000000} {set theWindow [ActiveWindowOrBeep]; movecursor $theWindow LEFTWORD; HomeWindowToSelectionStart $theWindow -semistrict}
# command-shift-right selects to end of word
bindkey	Right		{x1010000000} {set theWindow [ActiveWindowOrBeep]; expandselection $theWindow RIGHTWORD; HomeWindowToSelectionEnd $theWindow -semistrict}
# command-right moves to end of word
bindkey	Right		{x0010000000} {set theWindow [ActiveWindowOrBeep]; movecursor $theWindow RIGHTWORD; HomeWindowToSelectionEnd $theWindow -semistrict}


