# e93 base startup file
# This is the top of the startup script tree for e93
# it finds this file, which in turn includes all other
# files which are executed at startup time.

# NOTE: normally, this file requires no modifications,
# since all of its functionality can be overridden in
# other files.

# WARNING: be careful when editing this file,
# e93 will stop processing this file at the FIRST error!

# Globals/Defaults

#######################################################################
# The following lets you assign the "command" key of e93 for Windows
# Normally it's Ctrl
#
# 17 is Ctrl
# 18 is Alt
#
# If you set EEM_MOD0 to 18 and EEM_CTL to 17, Alt will be the e93 command key
#
# This must be done early if the menu names are to be displayed correctly
# You can change it at any time e93 is running
# However, the old menus will list the old name if you do that
# Menus created after that point will list the new command key name correctly
# Set them to something other than 17 or 18 at your own peril
# Likewise setting them both to the same value would not be a good thing
# UNIX pays no mind to these variables
#
# Uncomment the following like to make Alt be the e93 command key on Windows rather than Ctrl
# set EEM_MOD0 18; set EEM_CTL 17

tk			appname					e93;				# set up application name so e93 can be located by remote startup script

set			tcl_precision			16;					# increase precision

set			sysPrefsDir				"[file dirname $tcl_rcFileName]";	# place where system preferences are stored
set			userPrefsDir			"~/.e93";			# place where user's preferences are stored

set			mapsDir					"syntaxmaps";			# place inside prefs directory where syntaxmaps are stored
set			auxMapsDir				"syntaxmaps_aux";		# user can add additional maps in here
set			highlightDir			"highlightschemes";		# place inside prefs directory where highlighting schemes are stored
set			auxHighlightDir			"highlightschemes_aux";	# user can add additional highlight schemes in here
set			modulesDir				"modules";				# place inside prefs directory where modules are stored
set			auxModulesDir			"modules_aux";			# user can add additional modules in here
set			imagesDir				"images";				# all images the editor uses are read from here
set			prefsDir				"prefs";				# place where user's preferences can be written

set			untitledCounter			0;					# used to give each new untitled window a unique name
set			lineWrap				0;					# no line wrapping currently
set			lineWrapColumn			70;					# column where lines should be wrapped

set			grepFilesChoice			"";					# initialize the data in the grep files dialog
set			replaceSelectionsChoice	"";					# initialize the data in the replace selections dialog
set			pipeSelectionsChoice	"";					# initialize the data in the pipe selections dialog
set			mailRecipientChoice		"";					# initialize the data in the mail recipient dialog
set			mailSubjectChoice		"";					# initialize the data in the mail subject dialog
set			columnLimitChoice		"70";				# initialize the data in the column limit dialog

#scrollbarplacement	left;								# X-specific command to set default placement for vertical scroll bars (defaults to right)

# Set the characters considered parts of words when double clicking.
setwordchars {ABCDEFGHJIKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@_}

# Set checkboxes in the search dialog to default values.
# Change these to have them default whatever way you would like when e93 starts.
set			sd_backwards			0;					# do not search backwards by default at startup
set			sd_wrapAround			0;					# do not wrap around by default at startup
set			sd_selectionExpression	0;					# do not use selection expressions by default at startup
set			sd_ignoreCase			0;					# do not ignore case by default
set			sd_limitScope			0;					# do not limit scope by default
set			sd_replaceProc			0;					# do not treat replace text as procedure

# set up default command used to print text in "theBuffer"
#set		defaultPrintCommand		"a2ps -Pps18 --stdin=\$theBuffer -T\[gettabsize \$theBuffer\]"
set			defaultPrintCommand		"lpr -#1 -J=\$theBuffer"


# if e93 is unable to match a document name, these will be used as the defaults
set			defaultTabSize			4
set			defaultColorScheme		"e93";				# these must exist, or e93 will complain
set			defaultSyntaxMap		"Unknown"


# Styles enumerations for syntax coloring rules
# These style numbers are arbitrary (except for style_default)
set			style_default			0;					# define some standard styles to make life easier
set			style_comment			1;
set			style_string			2;
set			style_char				3;
set			style_digit				4;
set			style_operator			5;
set			style_variable			6;
set			style_value				7;
set			style_delimiter			8;
set			style_keyword			9;
set			style_type				10;
set			style_directive			11;
set			style_function			12;
set			lastStyle				$style_function;	# set this to allow others to add more styles at the end

tk_setPalette background gray60 foreground black;		# set colors for dialogs

# define coloring rules for various languages

# see if the user has his own set of maps specified, if so, skip including the defaults, and get only his
if {[file exists [file join $userPrefsDir $mapsDir]]} \
{
	foreach theFile [glob -nocomplain [file join $userPrefsDir $mapsDir *.tcl]] \
	{
		source $theFile;
	}
} \
else \
{
	# use all system defined syntax maps
	foreach theFile [glob -nocomplain [file join $sysPrefsDir $mapsDir *.tcl]] \
	{
		source $theFile;
	}
}

# get user's additional syntax maps (if any)
if {[file exists [file join $userPrefsDir $auxMapsDir]]} \
{
	foreach theFile [glob -nocomplain [file join $userPrefsDir $auxMapsDir *.tcl]] \
	{
		source $theFile;
	}
}

# see if the user has his own set of highlight schemes specified, if so, skip including the defaults, and get only his
if {[file exists [file join $userPrefsDir $highlightDir]]} \
{
	foreach theFile [glob -nocomplain [file join $userPrefsDir $highlightDir *.tcl]] \
	{
		source $theFile;
	}
} \
else \
{
	# use all system defined schemes
	foreach theFile [glob -nocomplain [file join $sysPrefsDir $highlightDir *.tcl]] \
	{
		source $theFile;
	}
}

# get user's additional highlighting schemes (if any)
if {[file exists [file join $userPrefsDir $auxHighlightDir]]} \
{
	foreach theFile [glob -nocomplain [file join $userPrefsDir $auxHighlightDir *.tcl]] \
	{
		source $theFile;
	}
}

# Tcl console
newbuffer tclConsole; setbuffervariable tclConsole lowpriority "";		# create a buffer for Tcl's stdout
#setchannelbuffer stdout tclConsole;									# direct the messages there
#setchannelbuffer stderr tclConsole;	

# Clipboard buffers
newbuffer	clip0; setbuffervariable clip0 lowpriority "";				# set the lowpriority variable which tells us to treat windows on this buffer specially
newbuffer	clip1; setbuffervariable clip1 lowpriority "";
newbuffer	clip2; setbuffervariable clip2 lowpriority "";
newbuffer	clip3; setbuffervariable clip3 lowpriority "";
newbuffer	clip4; setbuffervariable clip4 lowpriority "";
newbuffer	clip5; setbuffervariable clip5 lowpriority "";
newbuffer	clip6; setbuffervariable clip6 lowpriority "";
newbuffer	clip7; setbuffervariable clip7 lowpriority "";
newbuffer	clip8; setbuffervariable clip8 lowpriority "";
newbuffer	clip9; setbuffervariable clip9 lowpriority "";

setclipboard clip0;		# set this as the current clipboard

# Search buffers
newbuffer	findBuffer; setbuffervariable findBuffer lowpriority "";				# create default buffer to use for "find"
newbuffer	replaceBuffer; setbuffervariable replaceBuffer lowpriority "";			# create default buffer to use for "replace"
newbuffer	tempFindBuffer; setbuffervariable tempFindBuffer lowpriority "";		# these buffers are for commands which are implemented with search/replace, but do not want to overwrite the find/replaceBuffers
newbuffer	tempReplaceBuffer; setbuffervariable tempReplaceBuffer lowpriority "";


# ---------------------------------------------------------------------------------------------------------------------------------

# Determine screen and window sizes.
set			screenWidth				[lindex [screensize] 0]
set			screenHeight			[lindex [screensize] 1]
set			windowWidth				[expr $screenWidth*5/6];	# set the initial default width for new windows
set			windowHeight			[expr $screenHeight*3/4];	# set the initial default height for new windows

# Determine how windows stagger when they are opened.
set			staggerInitialX			[expr $screenWidth/24];		# set the initial default X position for new windows
set			staggerInitialY			[expr $screenHeight/16];	# set the initial default Y position for new windows
set			staggerIncrementX		[expr $screenWidth/160];	# amount to increment by
set			staggerIncrementY		[expr $screenHeight/160];
set			staggerMaxX				[expr $screenWidth*3/24]
set			staggerOpenX			$staggerInitialX;			# set the current X position for new windows
set			staggerOpenY			$staggerInitialY;			# set the current Y position for new windows

# ---------------------------------------------------------------------------------------------------------------------------------
# Procedures

# Home a window to the start of selection (with arguments)
proc HomeWindowToSelectionStart {theWindow args} \
{
	set position [lindex [getselectionends $theWindow] 0];		# get selection start position
	eval [list homewindow $theWindow] $position $position $args;	# home to start
}

# Return true if the passed window name is one of the low-priority windows.
# A low priority window is one that e93 should not ask to save, even if it
# has been modified...
proc LowPriority {theWindow} \
{
	expr {[catch {getbuffervariable $theWindow lowpriority}]==0}
}

# When the interpreter does not understand a given command, send it to a shell.
# NOTE: no interactive commands can be executed this way, because Tcl would
# be suspended (waiting for the interactive command to finish). If Tcl is
# suspended, then e93 is also suspended (waiting for the Tcl command to finish)
# So there is no way that e93 could provide input to an interactive command.
# Interactive commands should be run as tasks.
#proc unknown args \
#{
#	return [uplevel exec $args]
#}

# NOTE: the code above has been replaced by this:
proc console {} {}
set tcl_interactive 1
# those 2 lines tell the tcl built-in unknown routine to behave as
# desired, and also get us auto loading and other useful features


# If there is no selection, select the current line
proc SelectLineWhenNoSelection {theBuffer} \
{
	set ends [getselectionends $theBuffer]
	set start [lindex $ends 0]
	set end [lindex $ends 1]
	if {$start==$end} \
	{
		selectline $theBuffer [lindex [positiontolineoffset $theBuffer $start] 0];	# select the line the cursor is on
	}
}

# Copy the selection in theBuffer to the current clipboard.
# If there is no selection in theBuffer, then select the line the cursor is on.
proc SmartCopy {theBuffer} \
{
	SelectLineWhenNoSelection $theBuffer
	copy $theBuffer
}

# Cut the selection in theBuffer to the current clipboard.
# If there is no selection in theBuffer, then select the line the cursor is on.
proc SmartCut {theBuffer} \
{
	SelectLineWhenNoSelection $theBuffer
	cut $theBuffer
}

# Replace the entire contents of a buffer with the passed text, and clear undos on the buffer.
# NOTE: it would be bad if this were called on a document that had been edited for hours and not
# saved! -- It would blow away the entire contents.
proc TextToBuffer {theBuffer theText} \
{
	selectall $theBuffer;								# select everything, so insert will remove it
	insert $theBuffer $theText;							# write over everything in theBuffer
	flushundos $theBuffer;								# get rid of any undo information for this buffer
}

# Move the stagger position to the next spot for a new staggered window.
proc UpdateStaggerPosition {} \
{
	uplevel #0 \
	{
		incr staggerOpenX $staggerIncrementX;
		incr staggerOpenY $staggerIncrementY;
		if {$staggerOpenX>$staggerMaxX} \
		{
			set staggerOpenX $staggerInitialX;
		 	set staggerOpenY $staggerInitialY
		}
	}
}

# Tile the windows on the display.
proc TileWindows {} \
{
	set horizontalNumber 4;								# number of windows we want across
	set verticalNumber 4;								# number we want down
	set horizontalBorder 30;							# number of pixels to leave around edges of screen
	set verticalBorder 30;
	set screendimensions [screensize]
	set width [lindex $screendimensions 0]
	set height [lindex $screendimensions 1]
	set windowWidth [expr ($width-($horizontalBorder*2))/$horizontalNumber];	# get the width of each individual window
	set windowHeight [expr ($height-($verticalBorder*2))/$verticalNumber];		# get the height of each individual window
	set horizontalIndex 0; set verticalIndex 0;			# init counters that tell us where we are
	foreach theWindow [windowlist] \
	{
		if {![LowPriority $theWindow]} \
		{
			setrect $theWindow [expr $horizontalBorder+$horizontalIndex*$windowWidth] [expr $verticalBorder+$verticalIndex*$windowHeight] $windowWidth $windowHeight
			incr horizontalIndex
			if {$horizontalIndex>=$horizontalNumber} \
			{
				set horizontalIndex 0;
				incr verticalIndex;
				if {$verticalIndex>=$verticalNumber} \
				{
					set verticalIndex 0;
				}
			}
		}
	}
}

# Stack the windows on the display.
proc StackWindows {} \
{
	global staggerInitialX staggerInitialY staggerOpenX staggerOpenY windowWidth windowHeight

	set staggerOpenX $staggerInitialX;					# set the initial default X
	set staggerOpenY $staggerInitialY;					# set the initial default Y
	foreach theWindow [windowlist] \
	{
		if {![LowPriority $theWindow]} \
		{
			setrect $theWindow $staggerOpenX $staggerOpenY $windowWidth $windowHeight
			UpdateStaggerPosition
		}
	}
}

# return the foreground color for the given index of the given color scheme
proc HighlightSchemeForegroundColor {schemeName styleIndex} \
{
	global HighlightSchemes
	set schemeArray $HighlightSchemes($schemeName);	# get the name of the array which holds the scheme information
	global $schemeArray

	set elementValue [lindex [array get $schemeArray $styleIndex] 1]
	return [lindex $elementValue 0]
}

# return the background color for the given index of the given color scheme
proc HighlightSchemeBackgroundColor {schemeName styleIndex} \
{
	global HighlightSchemes
	set schemeArray $HighlightSchemes($schemeName);
	global $schemeArray

	set elementValue [lindex [array get $schemeArray $styleIndex] 1]
	return [lindex $elementValue 1]
}

# return the font name for the given index of the given color scheme
proc HighlightSchemeFont {schemeName styleIndex} \
{
	global HighlightSchemes
	set schemeArray $HighlightSchemes($schemeName);
	global $schemeArray

	set elementValue [lindex [array get $schemeArray $styleIndex] 1]
	return [lindex $elementValue 2]
}

# Given a window, set the highlight scheme (which colors and fonts to use)
proc SetHighlightScheme {theWindow schemeName} \
{
	global HighlightSchemes
	set schemeArray $HighlightSchemes($schemeName);	# get the name of the array which holds the scheme information
	global $schemeArray
	foreach element [array names $schemeArray] \
	{
		set elementValue [lindex [array get $schemeArray $element] 1]
		if {$element=="selection"} \
		{
			setselectioncolors $theWindow [lindex $elementValue 0] [lindex $elementValue 1]
		} \
		else \
		{
			setcolors $theWindow [lindex $elementValue 0] [lindex $elementValue 1] $element
			setfont   $theWindow [lindex $elementValue 2] $element
		}
	}
}

# Given a buffer, set the highlight mode (which syntax to parse)
proc SetHighlightMode {theBuffer highlightModeName} \
{
	setsyntaxmap $theBuffer $highlightModeName
}

# work out the default color scheme, highlight mode, and tab size for theBuffer
# return a list which contains a boolean that tells if something other than the
# default is being returned, the tab size, the color scheme name, and the syntax map name
proc GetDefaultStyleInfo {theBuffer} \
{
	global extensionHuntExpression extensionTabSize extensionColorScheme extensionMapName defaultTabSize defaultColorScheme defaultSyntaxMap

	set hadMatch 0

	# set up defaults
	set tabSize $defaultTabSize
	set schemeName $defaultColorScheme
	set mapName $defaultSyntaxMap

	foreach extensionAttrName [array names extensionHuntExpression] \
	{
		if {[regexp $extensionHuntExpression($extensionAttrName) [file tail $theBuffer]]} \
		{
			set tabSize $extensionTabSize($extensionAttrName)
			set schemeName $extensionColorScheme($extensionAttrName)
			set mapName $extensionMapName($extensionAttrName)
			set hadMatch 1
			break
		}
	}
	lappend temp $hadMatch $tabSize $schemeName $mapName;		# return the 3 items
}

# when a window's name has been changed, this is called to update the
# styles in use for the window
# if the name is recognized, this will change the style, otherwise it
# does nothing
proc UpdateStyle {theWindow} \
{
	set temp [GetDefaultStyleInfo $theWindow]
	if {[lindex $temp 0]} \
	{
		settabsize $theWindow [lindex $temp 1]
		SetHighlightScheme $theWindow [lindex $temp 2]
		SetHighlightMode $theWindow [lindex $temp 3]
	}
}

# Create a window onto theBuffer, using given values for position, size, font, tabSize, color, and highlighting.
proc OpenWindow {theBuffer x y width height tabSize schemeName mapName} \
{
	global style_default

	if {[haswindow $theBuffer]} \
	{
		settopwindow $theBuffer;						# if it was already open, then just put it to the top, change nothing
	} \
	else \
	{
		openwindow $theBuffer $x $y $width $height [HighlightSchemeFont $schemeName $style_default] $tabSize [HighlightSchemeForegroundColor $schemeName $style_default] [HighlightSchemeBackgroundColor $schemeName $style_default]
		SetHighlightScheme $theBuffer $schemeName
		SetHighlightMode $theBuffer $mapName
		HomeWindowToSelectionStart $theBuffer;			# go to the cursor/selection
	}
}

# Create a window onto theBuffer, using default values for position and size
# and given values for, font, tabSize, color, and highlighting.
proc OpenStaggeredWindow {theBuffer tabSize schemeName mapName} \
{
	global staggerOpenX staggerOpenY screenWidth screenHeight windowWidth windowHeight

	if {[haswindow $theBuffer]} \
	{
		settopwindow $theBuffer;						# if it was already open, then just put it to the top
	} \
	else \
	{
		if {[LowPriority $theBuffer]} \
		{
			set width [expr $screenWidth*3/4]
			set height [expr $screenHeight*1/8]
			set x [expr ($screenWidth-$width)/2]
			set y [expr $screenHeight-$height]
		} \
		else \
		{
			set width $windowWidth
			set height $windowHeight
			set x $staggerOpenX
			set y $staggerOpenY
			UpdateStaggerPosition;
		}
		OpenWindow $theBuffer $x $y $width $height $tabSize $schemeName $mapName
	}
}

# Create a window onto theBuffer, using default values for position, size, font, tabSize, color, and highlighting.
proc OpenDefaultWindow {theBuffer} \
{
	if {[haswindow $theBuffer]} \
	{
		settopwindow $theBuffer;						# if it was already open, then just put it to the top
	} \
	else \
	{
		if {[LowPriority $theBuffer]} \
		{
			set tabSize 4
			set schemeName "Low Priority"
			set mapName ""
		} \
		else \
		{
			set temp [GetDefaultStyleInfo $theBuffer]
			set tabSize [lindex $temp 1]
			set schemeName [lindex $temp 2]
			set mapName [lindex $temp 3]
		}
		OpenStaggeredWindow $theBuffer $tabSize $schemeName $mapName
	}
}

# Return a good name for a new window.
proc NewWindowName {} \
{
	global untitledCounter;								# reference the global
	set theName "Untitled-$untitledCounter";			# get new name
	incr untitledCounter;								# next time, make the count one larger
	return $theName;									# return the name of the new window
}

# Create a new window with a default name, return the name.
proc NewWindow {} \
{
	OpenDefaultWindow [set theName [newbuffer [NewWindowName]]];	# open a new window with new name
	return $theName;									# return the name of the new window
}

# Open the passed file name into a buffer, and record the modification time
# of the file into the variable "mtime" attached to the buffer
# if there is a problem, fail just like openbuffer would
proc OpenBufferRecordMtime {theFile} \
{
	if {[catch {openbuffer $theFile} message]==0} \
	{
		# see if there is ALREADY a modification time tag associated with this file, if so, check against current mtime and complain if they differ
		# NOTE: there could already be a variable assigned if the file was open already
		if {[catch {getbuffervariable $message mtime} oldmtime]==0} \
		{
			if {[catch {file mtime $message} newmtime]==0} \
			{
				if {$oldmtime!=$newmtime} \
				{
					okdialog "WARNING!\n\nIt looks like another application modified:\n$message\nwhile it was open for editing.\n"
				}
			} \
			else \
			{
				okdialog "WARNING!\n\nA buffer exists for the requested file:\n$message\nBut could not read mtime.\n"
			}
		} \
		else \
		{
			catch {setbuffervariable $message mtime [file mtime $message]};
		}
		return $message;
	} \
	else \
	{
		return -code error $message;
	}
}

# Attempt to open a list of files one at a time.
# If any fails to open, report why, and give the user a chance to cancel.
proc OpenList {theList} \
{
	set numItems [llength $theList]
	set theItem 0
	while {$theItem<$numItems} \
	{
		set theFile [lindex $theList $theItem]
		incr theItem
		if {[catch {OpenDefaultWindow [OpenBufferRecordMtime $theFile]} errorMessage]!=0} \
		{
			if {$theItem==$numItems} \
			{
				okdialog "Failed to open '$theFile'\n$errorMessage"
			} \
			else \
			{
				okcanceldialog "Failed to open '$theFile'\n$errorMessage\n\nContinue?"
			}
		}
	}
}

# Given an "filename" from a list, try to interpret it as the output from
# an GNU tool:
# filename:nnnn
# If it looks like that, then extract the filename and line number
# and return them in a list, otherwise return an error
proc SmartOpenListGNU {listItem} \
{
	if {[regexp {^([^:]+):([0-9]+)} $listItem whole tempFile tempLine]} \
	{
		if {[file exists $tempFile]} \
		{
			lappend temp $tempFile $tempLine;	# make a list of 2 elements (filename and line number)
			lappend temp2 $temp;				# return list of lists
			return $temp2;
		}
	}
	return -code error;
}

set	smartOpenProcs(GNU)			{SmartOpenListGNU}

# Given an "filename" from a list, try to interpret it as the output from
# an MPW tool:
# File filename;Line nnnn
# If it looks like that, then extract the filename and line number
# and return them in a list, otherwise return an error
proc SmartOpenListMPW {listItem} \
{
	if {[regexp {^File *'?([^' ]+)'? *; *Line *([0-9]+)} $listItem whole tempFile tempLine]} \
	{
		if {[file exists $tempFile]} \
		{
			lappend temp $tempFile $tempLine;	# make a list of 2 elements (filename and line number)
			lappend temp2 $temp;				# return list of lists
			return $temp2;
		}
	}
	return -code error;
}

set	smartOpenProcs(MPW)			{SmartOpenListMPW}

# Given an "filename" from a list, try to interpret it as the output from
# a Visual Studio tool:
# filename(nnnn)
# If it looks like that, then extract the filename and line number
# and return them in a list, otherwise return an error
proc SmartOpenListVisualStudio {listItem} \
{
	if {[regexp {^([^ (]+) *\(([0-9]+)\)} $listItem whole tempFile tempLine]} \
	{
		if {[file exists $tempFile]} \
		{
			lappend temp $tempFile $tempLine;	# make a list of 2 elements (filename and line number)
			lappend temp2 $temp;				# return list of lists
			return $temp2;
		}
	}
	return -code error;
}

set	smartOpenProcs(VisualStudio)	{SmartOpenListVisualStudio}

# Given an "filename" from a list, try to interpret it as a 'C' include
# statement:
# #include <filename>	(looks in /usr/include)
# #include "filename"	(looks in current directory)
# If it looks like that, then extract the filename
# and return it, otherwise return an error
proc SmartOpenListInclude {listItem} \
{
	if {[regexp "^\[ \t\]*#\[ \t\]*include\[ \t\]+\\<(\[^>\]+)\\>" $listItem whole tempFile]} \
	{
		lappend temp "/usr/include/$tempFile"
		lappend temp2 $temp;				# return list of lists
		return $temp2;
	} \
	else \
	{
		if {[regexp "^\[ \t\]*#\[ \t\]*include\[ \t\]+\"(\[^\"\]+)\"" $listItem whole tempFile]} \
		{
			lappend temp $tempFile;
			lappend temp2 $temp;			# return list of lists
			return $temp2;
		}
	}
	return -code error;
}

set	smartOpenProcs(Include)		{SmartOpenListInclude}

# Given an "filename" from a list, try to interpret it as a globbed list
# of names.
# If it looks like that, then extract the filename
# and return it, otherwise return an error
proc SmartOpenListGlob {listItem} \
{
	if {[catch {glob $listItem} globList]==0} \
	{
		return $globList;
	}
	return -code error;
}

set	smartOpenProcs(Glob)			{SmartOpenListGlob}


# Just like OpenList, but uses an array of functions to try to identify filenames
# with line numbers that might appear in various forms.
# If it can find a line number, then it opens the file to that line.
proc SmartOpenList {theList} \
{
	global smartOpenProcs

	set numItems [llength $theList]
	set theItem 0
	while {$theItem<$numItems} \
	{
		set listItem [lindex $theList $theItem]
		incr theItem
		set claimed 0

		# strip off newline if it exists
		regexp "^(\[^\n\]+)" $listItem whole listItem;

		foreach procListElement [array names smartOpenProcs] \
		{
			# see if one of the functions will claim this item as its own
			if {[catch {set results [$smartOpenProcs($procListElement) $listItem]} errorMessage]==0} \
			{
				set numResults [llength $results]
				set resultIndex 0
				while {$resultIndex<$numResults} \
				{
					set result [lindex $results $resultIndex];		# pick out one of the file/line pairs from the return result
					incr resultIndex;
					set theFile [lindex $result 0];					# get the pieces
					set theLine [lindex $result 1]

					if {[catch {OpenDefaultWindow [set newBuffer [OpenBufferRecordMtime $theFile]]} errorMessage]==0} \
					{
						if {[string length $theLine]!=0} \
						{
							selectline $newBuffer $theLine;
							HomeWindowToSelectionStart $newBuffer;
						}
					} \
					else \
					{
						if {$theItem==$numItems&&$resultIndex==$numResults} \
						{
							okdialog "Failed to open '$theFile'\n$errorMessage"
						} \
						else \
						{
							okcanceldialog "Failed to open '$theFile'\n$errorMessage\n\nContinue?"
						}
					}
				}
				set claimed 1;
				break;				# had a function claim the list element, so stop searching
			}
		}
		if {$claimed==0} \
		{
			if {[catch {OpenDefaultWindow [set newBuffer [OpenBufferRecordMtime $listItem]]} errorMessage]!=0} \
			{
				if {$theItem==$numItems} \
				{
					okdialog "Failed to open '$listItem'\n$errorMessage"
				} \
				else \
				{
					okcanceldialog "Failed to open '$listItem'\n$errorMessage\n\nContinue?"
				}
			}
		}
	}
}

# Attempt to include a list of files one at a time, into the given buffer.
# If any fails to include, report why, and give the user a chance to cancel.
proc IncludeList {theBuffer theList} \
{
	set numItems [llength $theList]
	set theItem 0

	while {$theItem<$numItems} \
	{
		set theFile [lindex $theList $theItem]
		incr theItem
		if {[catch {insertfile $theBuffer $theFile} errorMessage]!=0} \
		{
			if {$theItem==$numItems} \
			{
				okdialog "Failed to include '$theFile'\n$errorMessage"
			} \
			else \
			{
				okcanceldialog "Failed to include '$theFile'\n$errorMessage\n\nContinue?"
			}
		}
	}
}

# Swap the top two windows on the screen.
proc SwapWindows {} \
{
	set windowList [windowlist]
	if {[llength $windowList]>1} \
	{
		settopwindow [lindex $windowList 1]
	} \
	else \
	{
		beep
	}
}

# Bring the bottom window to the top.
proc RotateWindows {} \
{
	set windowList [windowlist]
	set numWindows [llength $windowList]
	if {$numWindows>1} \
	{
		incr numWindows -1
		settopwindow [lindex $windowList $numWindows]
	} \
	else \
	{
		beep
	}
}

# Choose a window to be top-most (this places a * next to windows which have
# been modified)
proc ChooseWindow {} \
{
	set newList "";
	foreach theWindow [windowlist]\
	{
		if {[isdirty $theWindow]} \
		{
			lappend newList "* $theWindow";
		} \
		else \
		{
			lappend newList "  $theWindow";
		}
 	}
	foreach theWindow [listdialog "Choose a Window:" $newList] \
	{
		settopwindow [string range $theWindow 2 end];	# select the window (removing the modified indication)
	}
}

# Return the active window if there is one, otherwise
# just beep, and return an error.
proc ActiveWindowOrBeep {} \
{
	if {[catch {activewindow} message]==0} \
	{
		return $message
	}
	beep
	return -code error
}

# Report the current clipboard, or a message that indicates that
# there is none
proc ShowCurrentClipboard {} \
{
	if {[catch {getclipboard} message]==0} \
	{
		okdialog "Current clipboard:\n\n$message"
	} \
	else \
	{
		okdialog "There is no current clipboard"
	}
}

# Open the current clipboard, or show a message that indicates that
# there is none
proc OpenCurrentClipboard {} \
{
	if {[catch {getclipboard} message]==0} \
	{
		OpenDefaultWindow $message
	} \
	else \
	{
		okdialog "There is no current clipboard"
	}
}

# Quiz user to find out if he really wants to "revert".
proc AskRevert {theBuffer} \
{
	if {[isdirty $theBuffer]} \
	{
		okcanceldialog "Do you really want to discard changes to:\n'$theBuffer'"
		revertbuffer $theBuffer
	} \
	else \
	{
		revertbuffer $theBuffer;			# may be reverting because the file was changed, so just do it without asking
	}
}

# Get name of file to "save as", then do it for the given buffer.
# NOTE: this returns the new name of the buffer if it completes successfully.
proc AskSaveAs {theBuffer} \
{
	set newPath [savedialog "Save File:" $theBuffer]
	if {[file exists $newPath]} \
	{
		okcanceldialog "File:\n'$newPath'\nalready exists. Overwrite?"
	}
	if {[catch {savebufferas $theBuffer $newPath} message]==0} \
	{
		catch {setbuffervariable $message mtime [file mtime $message]};		# update the modification time so we can look at it when saving
	} \
	else \
	{
		okdialog "Failed to save:\n'$newPath'\n$message"
		return -code error
	}
	UpdateStyle $message;				# update the styles of this window
	return $message;					# return the new name
}

# Get name of file to "save to", then do it for the given buffer.
proc AskSaveTo {theBuffer} \
{
	set newPath [savedialog "Save Copy To:" $theBuffer]
	if {[file exists $newPath]} \
	{
		okcanceldialog "File:\n'$newPath'\nalready exists. Overwrite?"
	}
	if {[catch {savebufferto $theBuffer $newPath} errorMessage]!=0} \
	{
		okdialog "Failed to save:\n'$newPath'\n$errorMessage"
		return -code error
	}
}

# See if theBuffer is not linked to a file, and if not, ask for
# a file name to save it to, else just save it.
# Either way, return the name of the buffer after it is saved,
# as it may have changed during the save process.
proc AskSave {theBuffer} \
{
	if {[fromfile $theBuffer]} \
	{
		# see if there is a modification time tag associated with this file, if so, check against current mtime and complain if they differ
		if {[catch {getbuffervariable $theBuffer mtime} oldmtime]==0} \
		{
			if {[catch {file mtime $theBuffer} newmtime]==0} \
			{
				if {$oldmtime!=$newmtime} \
				{
					okcanceldialog "WARNING!\n\nIt looks like another application modified:\n$theBuffer\nwhile it was open for editing.\n\nWould you like to save anyway?"
				}
			} \
			else \
			{
				okcanceldialog "WARNING!\n\nIt looks like another application deleted:\n$theBuffer\nwhile it was open for editing.\n\nWould you like to save anyway?"
			}
		}

		if {[catch {savebuffer $theBuffer} message]==0} \
		{
			catch {setbuffervariable $theBuffer mtime [file mtime $theBuffer]};		# update the modification time so we can look at it next time we save
		} \
		else \
		{
			okdialog "Failed to save:\n'$theBuffer'\n$message"
			return -code error
		}
	} \
	else \
	{
		set theBuffer [AskSaveAs $theBuffer];				# pick up the new name of the buffer
	}
	return $theBuffer
}

# Ask the user if he really wants to save all the dirty windows, if so, do it.
proc AskSaveAll {} \
{
	ActiveWindowOrBeep;										# make sure there is a window, if not, just beep and leave
	okcanceldialog "Really save all modified windows?"
	foreach theWindow [windowlist] \
	{
		if {[isdirty $theWindow]} \
		{
			if {[catch {AskSave $theWindow}]!=0} \
			{
				okdialog "Save All aborted\n"
				return -code error
			}
		}
	}
}

# See if theBuffer is dirty before closing.
# If it is, give user a chance to save.
proc AskClose {theBuffer} \
{
	# See if theBuffer points to a something low-priority, we just close the windows on them, not the buffer
	if {[LowPriority $theBuffer]} \
	{
		closewindow $theBuffer
	} \
	else \
	{
		if {[isdirty $theBuffer]} \
		{
			if {[yesnodialog "Save Changes To:\n'$theBuffer'\nBefore Closing?"]} \
			{
				set theBuffer [AskSave $theBuffer];				# if it is saved, get the new name so we can close it
			}
		}
		closebuffer $theBuffer
	}
}

# Ask for a list of windows which should be printed
proc PrintWindows {} \
{
	global defaultPrintCommand;

	foreach theBuffer [listdialog "Choose Window(s) to Print:" [windowlist]] \
	{
		set command [textdialog "If needed, modify the print command below:" "[eval format "%s" \"$defaultPrintCommand\"]"]
		setmark $theBuffer temp;						# remember what was selected, we will not disturb it
		selectall $theBuffer;							# make a selection of all of the text in the passed buffer
		if {[catch {eval exec $command << {[lindex [selectedtextlist $theBuffer] 0]}} theResult]!=0} \
		{
			okdialog "Print status:\n\n$theResult\n"
		}
		gotomark $theBuffer temp;						# put selection back
		clearmark $theBuffer temp;						# get rid of temp selection
	}
}

# Attempt to get lpr to output this document to the local printer
proc PrintBuffer {theBuffer} \
{
	global defaultPrintCommand;

	set command [textdialog "If needed, modify the print command below:" "[eval format "%s" \"$defaultPrintCommand\"]"]
	setmark $theBuffer temp;							# remember what was selected, we will not disturb it
	selectall $theBuffer;								# make a selection of all of the text in the passed buffer
	if {[catch {eval exec $command << {[lindex [selectedtextlist $theBuffer] 0]}} theResult]!=0} \
	{
		okdialog "Print status:\n\n$theResult\n"
	}
	gotomark $theBuffer temp;							# put selection back
	clearmark $theBuffer temp;							# get rid of temp selection
}

# print the first selection to the local printer
proc PrintSelection {theBuffer} \
{
	global defaultPrintCommand;

	if {[lindex [selectioninfo $theBuffer] 6]} \
	{
		set command [textdialog "If needed, modify the print command below:" "[eval format "%s" \"$defaultPrintCommand\"]"]
		if {[catch {eval exec $command << {[lindex [selectedtextlist $theBuffer] 0]}} theResult]!=0} \
		{
			okdialog "Print status:\n\n$theResult\n"
		}
	} \
	else \
	{
		beep
	}
}

# Attempt to close all open windows, asking to save any that are dirty.
# If the user cancels, then back out, otherwise, quit.
proc TryToQuit {} \
{
	foreach theWindow [windowlist] \
	{
		AskClose $theWindow
	}
	forceQUIT
}

# Just like find, but will place the result in a dialog if it fails, and home if it succeeds, or beep if it finds nothing.
proc FindBeep {theWindow theBuffer args} \
{
	if {[catch {eval [list find $theWindow $theBuffer] $args} message]==0} \
	{
		if {[llength $message]} {homewindow $theWindow [lindex $message 0] [lindex $message 1]} else {beep}
	} \
	else \
	{
		okdialog $message
	}
}

# Just like findall, but will place the result in a dialog if it fails, and home if it succeeds, or beep if it finds nothing.
proc FindAllBeep {theWindow theBuffer args} \
{
	if {[catch {eval [list findall $theWindow $theBuffer] $args} message]==0} \
	{
		if {[llength $message]} {homewindow $theWindow [lindex $message 0] [lindex $message 1]} else {beep}
	} \
	else \
	{
		okdialog $message
	}
}

# Just like replace, but will place the result in a dialog if it fails, and home if it succeeds, or beep if it finds nothing.
proc ReplaceBeep {theWindow theFindBuffer theReplaceBuffer args} \
{
	if {[catch {eval [list replace $theWindow $theFindBuffer $theReplaceBuffer] $args} message]==0} \
	{
		if {[llength $message]} {homewindow $theWindow [lindex $message 0] [lindex $message 1]} else {beep}
	} \
	else \
	{
		okdialog $message
	}
}

# Just like replaceall, but will place the result in a dialog if it fails, and home if it succeeds, or beep if it finds nothing.
proc ReplaceAllBeep {theWindow theFindBuffer theReplaceBuffer args} \
{
	if {[catch {eval [list replaceall $theWindow $theFindBuffer $theReplaceBuffer] $args} message]==0} \
	{
		if {[llength $message]} {homewindow $theWindow [lindex $message 0] [lindex $message 1]} else {beep}
	} \
	else \
	{
		okdialog $message
	}
}

# Turn the booleans that come back from the search dialog into flags which can be passed
# to the search functions
proc CreateSearchOptions {} \
{
	global sd_backwards sd_wrapAround sd_selectionExpression sd_ignoreCase sd_limitScope sd_replaceProc

	set result ""
	if {$sd_backwards!=0} \
	{
		lappend result "-backward";
	}
	if {$sd_wrapAround!=0} \
	{
		lappend result "-wrap";
	}
	if {$sd_selectionExpression!=0} \
	{
		lappend result "-regex";
	}
	if {$sd_ignoreCase!=0} \
	{
		lappend result "-ignorecase";
	}
	if {$sd_limitScope!=0} \
	{
		lappend result "-limitscope";
	}
	if {$sd_replaceProc!=0} \
	{
		lappend result "-replacescript";
	}
	return $result
}

# Ask the user just what he wishes to search for, and how, then do it if he does not cancel.
proc AskSearch {theWindow} \
{
	global sd_backwards sd_wrapAround sd_selectionExpression sd_ignoreCase sd_limitScope sd_replaceProc

	set searchType [searchdialog "" findBuffer replaceBuffer sd_backwards sd_wrapAround sd_selectionExpression sd_ignoreCase sd_limitScope sd_replaceProc]
	switch $searchType \
	{
		find
		{
			eval [list FindBeep $theWindow findBuffer] [CreateSearchOptions]
		}
		findall
		{
			eval [list FindAllBeep $theWindow findBuffer] [CreateSearchOptions]
		}
		replace
		{
			eval [list ReplaceBeep $theWindow findBuffer replaceBuffer] [CreateSearchOptions]
		}
		replaceall
		{
			eval [list ReplaceAllBeep $theWindow findBuffer replaceBuffer] [CreateSearchOptions]
		}
	}
}

# Used for find same backwards/forwards.
proc FindNext {theWindow backward} \
{
	set flags [CreateSearchOptions]
	# if asked to search backwards, then add this flag to the list
	if {$backward!=0} \
	{
		lappend flags "-backward"
	} \
	else \
	{
		lappend flags "-forward"
	}
	eval [list FindBeep $theWindow findBuffer] $flags
}

# if selection then first copy the selection and then do FindAllBeep
# if no selection then try to find what is in the findBuffer
proc FindSelectionNext {theWindow backward} \
{
	set ends [getselectionends $theWindow]
	set start [lindex $ends 0]
	set end [lindex $ends 1]
	if {$start!=$end} \
	{
		copy $theWindow findBuffer
	}
	FindNext $theWindow $backward
}

# Used for replace same backwards/forwards.
proc ReplaceNext {theWindow backward} \
{
	set flags [CreateSearchOptions]
	# if asked to search backwards, then add this flag to the list
	if {$backward!=0} \
	{
		lappend flags "-backward"
	} \
	else \
	{
		lappend flags "-forward"
	}
	eval [list ReplaceBeep $theWindow findBuffer replaceBuffer] $flags
}

# Replace all selections with some given text.
proc ReplaceSelections {theWindow} \
{
	global replaceSelectionsChoice;

	TextToBuffer tempFindBuffer {([^]+)};							# load up the expression (find anything, mark as \0)
	TextToBuffer tempReplaceBuffer [set replaceSelectionsChoice [textdialog "Replacement string?" $replaceSelectionsChoice]];	# load up the replacement
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope;	# do the replacement
}

# Shift the text but attempt to leave the selection as little disturbed as possible
proc HandleShifts {theBuffer} \
{
	set numSelections [lindex [selectioninfo $theBuffer] 6]

	if {$numSelections>0} \
	{
		# if one selection, try not to "columnarize" it
		if {$numSelections==1} \
		{
			newbuffer tempBuffer;							# create a place to do some messing around in
			copy $theBuffer tempBuffer;						# move text in question to temp buffer
			setselectionends tempBuffer 0 0;				# move to top for replace

			set start [lindex [getselectionends $theBuffer] 0];	# get start of selection

			if {[catch {replaceall tempBuffer tempFindBuffer tempReplaceBuffer -regex} message]==0} \
			{
				if {[llength $message]} \
				{
					setselectionends tempBuffer 0 0;		# get rid of selections in temp buffer so paste does what we want
					paste $theBuffer tempBuffer;			# place results back, write over old selection
					set length [lindex [textinfo tempBuffer] 1]
					setselectionends $theBuffer $start [expr $start+$length] ;	# re-select the block
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
			closebuffer tempBuffer;
		} \
		else \
		{
			if {[catch {replaceall $theBuffer tempFindBuffer tempReplaceBuffer -regex -limitscope} message]==0} \
			{
				if {[llength $message]==0} {beep}
			} \
			else \
			{
				okdialog $message
			}
		}
	} \
	else \
	{
		beep
	}
}

# Align text left, removing all tabs or spaces at the start of lines in the selection.
proc AlignLeft {theBuffer} \
{
	TextToBuffer tempFindBuffer {^[\t ]+(.*)|(.+)};				# load up the expression
	TextToBuffer tempReplaceBuffer {\0\1};						# load up the replacement
	HandleShifts $theBuffer;
}

# Shift text left, removing tabs or spaces at the start of lines in the selection.
proc ShiftLeft {theBuffer} \
{
	TextToBuffer tempFindBuffer {^[\t ](.*)|(.+)};				# load up the expression (expression is slightly strange, so that lines which are not altered are not deselected)
	TextToBuffer tempReplaceBuffer {\0\1};						# load up the replacement
	HandleShifts $theBuffer;
}

# Shift text right, adding tabs at the start of lines in the selection.
proc ShiftRight {theBuffer} \
{
	TextToBuffer tempFindBuffer {^(.+)};						# load up the expression
	TextToBuffer tempReplaceBuffer {	\0};					# load up the replacement
	HandleShifts $theBuffer;
}

# Unselect any whitspace which is currently selected.
# This is sometimes useful during columnar select when tabs or spaces
# get in the way
proc UnselectWhitespace {theBuffer} \
{
	TextToBuffer tempFindBuffer {[^ \t\n]+};					# load up the expression to match any non-white characters
	if {[catch {findall $theBuffer tempFindBuffer -regex -limitscope} message]==0} \
	{
		if {[llength $message]==0} \
		{
			# no non-white characters were located, so selection is ALL white, just eliminate the selection
			set start [lindex [getselectionends $theBuffer] 0];		# get start of selection
			setselectionends $theBuffer $start $start;				# reduce selection to nothing
		}
	} \
	else \
	{
		okdialog $message
	}
}

# Report interesting information about the passed buffer
proc BufferInfo {theBuffer} \
{
	set textInfo [textinfo $theBuffer]
	set textLines [lindex $textInfo 0]
	set textChars [lindex $textInfo 1]

	set selectionInfo [selectioninfo $theBuffer]
	set startPosition [lindex $selectionInfo 0]
	set endPosition [lindex $selectionInfo 1]
	set startLine [lindex $selectionInfo 2]
	set endLine [lindex $selectionInfo 3]
	set startLinePosition [lindex $selectionInfo 4]
	set endLinePosition [lindex $selectionInfo 5]
	set totalSegments [lindex $selectionInfo 6]
	set totalSpan [lindex $selectionInfo 7]

	if {$endLinePosition&&$totalSpan} \
	{
		set totalLines [expr $endLine-$startLine+1];	# this is kind of screwy, but it gives the results that are easiest for the user to understand
	} \
	else \
	{
		set totalLines [expr $endLine-$startLine]
	}

	okdialog "\
$theBuffer\n\n\
Tab stops                [gettabsize $theBuffer]\n\
Total lines              $textLines\n\
Total bytes              $textChars\n\
Selection start position $startPosition ($startLine:$startLinePosition)\n\
Selection end position   $endPosition ($endLine:$endLinePosition)\n\
Total selection segments $totalSegments\n\
Total selected chars     $totalSpan\n\
Total lines spanned      $totalLines\n"
}

# Pipe the selections through a unix command, collect output, and replace selections.
proc PipeSelection {theWindow} \
{
	global pipeSelectionsChoice;

	set command [set pipeSelectionsChoice [textdialog "Enter command to pipe selections through:" $pipeSelectionsChoice]]
	TextToBuffer tempFindBuffer {[^]+};							# load up the expression (find absolutely anything)
	TextToBuffer tempReplaceBuffer "catch \{exec -keepnewline $command <<\$found\} message; set message";	# load up the replacement, return results of command
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope -replacescript;			# do the replacement
}

# Convert all letters of the given selection to upper case.
proc UppercaseSelection theWindow \
{
	TextToBuffer tempFindBuffer {[^]+};							# load up the expression (find anything)
	TextToBuffer tempReplaceBuffer "string toupper \$found";	# load up the replacement
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope -replacescript;	# do the replacement
}

# Convert all letters of the given selection to lower case.
proc LowercaseSelection theWindow \
{
	TextToBuffer tempFindBuffer {[^]+};							# load up the expression (find anything)
	TextToBuffer tempReplaceBuffer "string tolower \$found";	# load up the replacement
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope -replacescript;	# do the replacement
}

# Locate all the numbers in the current selection, and increment them by the given amount.
proc IncrementSelection {theWindow amount} \
{
	TextToBuffer tempFindBuffer {-?(([0-9]+(\.[0-9]+)?)|(\.[0-9]+))};		# load up the expression (find any (possibly floating point) number including negative ones)
	TextToBuffer tempReplaceBuffer "expr [string trimleft \$found 0] + $amount";	# load up the replacement, return the result of adding amount
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope -replacescript;	# do the replacement
}

# Replace each selection by an incrementing number.
proc EnumerateSelection {theWindow startAt amount} \
{
	global enumStart
	set enumStart $startAt
	TextToBuffer tempFindBuffer {.+};							# load up the expression (find anything)
	TextToBuffer tempReplaceBuffer "global enumStart;set temp \$enumStart;incr enumStart $amount;format %d \$temp";		# load up the replacement, return an incremented number
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope -replacescript;	# do the replacement
}

# Report the sum of all selected numbers.
proc SumSelection {theWindow} \
{
	setmark $theWindow temp;									# remember what was selected, we will not disturb it
	TextToBuffer tempFindBuffer {-?(([0-9]+(\.[0-9]+)?)|(\.[0-9]+))};		# load up the expression (find any (possibly floating point) number including negative ones)
	if {[catch {findall $theWindow tempFindBuffer -regex -limitscope} message]==0} \
	{
		if {[llength $message]} \
		{
			set total 0
			foreach theNumber [selectedtextlist $theWindow] \
			{
				set total [expr [string trimleft $theNumber 0]+$total];	# convert number with leading 0's to DECIMAL, not octal!
			}
			okdialog "Sum = $total"
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

# Sort the selections, and replace them in sorted order.
proc SortSelection {theWindow} \
{
	global sortedArray sortedIndex
	catch {unset sortedArray}
	set sortedIndex 0
	foreach theElement [lsort [selectedtextlist $theWindow]] \
	{
		set sortedArray($sortedIndex) $theElement;				# copy elements into an array to speed things up
		incr sortedIndex
	}
	set sortedIndex 0
	TextToBuffer tempFindBuffer {[^]+};							# load up the expression (find absolutely anything)
	TextToBuffer tempReplaceBuffer {global sortedArray sortedIndex;incr sortedIndex;set sortedArray([expr $sortedIndex-1])};		# load up the replacement, return an entry from the array
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope -replacescript;	# do the replacement
	catch {unset sortedArray}
	unset sortedIndex
}

# Replace the selections in reverse order.
proc ReverseSelection {theWindow} \
{
	global reverseArray reverseIndex
	catch {unset reverseArray}
	set reverseIndex 0
	foreach theElement [selectedtextlist $theWindow] \
	{
		set reverseArray($reverseIndex) $theElement;			# copy elements into an array to speed things up
		incr reverseIndex
	}
	TextToBuffer tempFindBuffer {[^]+};							# load up the expression (find absolutely anything)
	TextToBuffer tempReplaceBuffer {global reverseArray reverseIndex;incr reverseIndex -1;set reverseArray($reverseIndex)};	# load up the replacement, return an entry from the array
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope -replacescript;	# do the replacement
	catch {unset reverseArray}
	unset reverseIndex
}

# Get a manual page, and dump it into a window.
proc ManPage theParams \
{
	global staggerOpenX staggerOpenY windowHeight style_keyword style_string

	set theName [newbuffer [NewWindowName]];			# make a buffer to hold the man page
	execute $theName "catch \{exec man $theParams\} theResult; set theResult";

# man thinks it is outputing to a printer where it can backspace over stuff to get effects
# like BOLD, or dot points. The replace strips out the mess

# first, go through and make keywords out of anything man was trying to make bold
	setselectionends $theName 0 0;						# move to the top of the man page buffer
	TextToBuffer tempFindBuffer {((.)\x08\1)+};			# load up the expression
	catch {findall $theName tempFindBuffer -regex};		# do this, ignore any errors
	selectiontostyle $theName $style_keyword;			# set what we found to style_keyword

# now, go through and make strings out of anything man was trying to make underlines
	setselectionends $theName 0 0;						# move to the top of the man page buffer
	TextToBuffer tempFindBuffer {(_\x08[^\n_])+};		# load up the expression
	catch {findall $theName tempFindBuffer -regex};		# do this, ignore any errors
	selectiontostyle $theName $style_string;			# set what we found to style_string

# remove all the backspace characters
	setselectionends $theName 0 0;						# move to the top of the man page buffer
	TextToBuffer tempFindBuffer {.\x08};				# load up the expression
	TextToBuffer tempReplaceBuffer {};					# replace with nothing
	catch {replaceall $theName tempFindBuffer tempReplaceBuffer -regex};	# do this, ignore any errors

# get rid of any huge page gaps
	setselectionends $theName 0 0;						# move to the top of the buffer again
	TextToBuffer tempFindBuffer {^\n{2,}};				# load up the expression
	TextToBuffer tempReplaceBuffer "\n";				# replace with nothing
	catch {replaceall $theName tempFindBuffer tempReplaceBuffer -regex};	# do this, ignore any errors
	setselectionends $theName 0 0;						# move to the top of the buffer again

	flushundos $theName;								# get rid of any undos we made here, just to be nice
	cleardirty $theName;								# make it non-modified

	OpenWindow $theName $staggerOpenX $staggerOpenY [expr 8*80] $windowHeight 8 "Man Page" ""
	UpdateStaggerPosition;
}

# Refill a selected paragraph, so that the lines of text do not extend past the given column.
# This is a little ugly, but it does the job quite nicely
proc RefillSelection theBuffer \
{
	global columnLimitChoice;

	set limit [expr [set columnLimitChoice [textdialog "Max column:" $columnLimitChoice]]];		# get column limit from user

	set ends [getselectionends $theBuffer]
	set start [lindex $ends 0]
	set end [lindex $ends 1]
	if {$start!=$end} \
	{
		setselectionends $theBuffer $start $end;		# make only one selection, even if more than one

		newbuffer tempBuffer;							# create a place to do some messing around in
		copy $theBuffer tempBuffer;						# move text in question to temp buffer

		TextToBuffer tempFindBuffer "^\[ \\t\]+";		# remove white space at starts of lines
		TextToBuffer tempReplaceBuffer {};
		setselectionends tempBuffer 0 0;				# move to top for replace
		catch {replaceall tempBuffer tempFindBuffer tempReplaceBuffer -regex};	# do this, ignore any errors

		TextToBuffer tempFindBuffer "\[ \\t\]+";		# make tabs, or multiple spaces into single spaces
		TextToBuffer tempReplaceBuffer { };
		setselectionends tempBuffer 0 0;				# move to top for replace
		catch {replaceall tempBuffer tempFindBuffer tempReplaceBuffer -regex};	# do this, ignore any errors

		TextToBuffer tempFindBuffer "(^\\n)|(\\n)";		# make all newlines at the ends of lines into spaces
		TextToBuffer tempReplaceBuffer {if {[string length $1] > 0} {set found { }}; set found};
		selectall tempBuffer;							# select all so replace will start at the bottom (replacing backwards)
		catch {replaceall tempBuffer tempFindBuffer tempReplaceBuffer -backward -regex -replacescript};	# do this, ignore any errors

		TextToBuffer tempFindBuffer "(.\{1,$limit\})((\[ \\t\]+)|(.$))";	# cut lines to length at whitespace
		TextToBuffer tempReplaceBuffer {if {[string length $2] > 0} {format "%s\n" $0} else {format "%s%s" $0 $3}};
		setselectionends tempBuffer 0 0;				# move to top for replace
		catch {replaceall tempBuffer tempFindBuffer tempReplaceBuffer -regex -replacescript};	# do this, ignore any errors

		setselectionends tempBuffer 0 0;				# get rid of selections in temp buffer so paste does what we want
		paste $theBuffer tempBuffer;					# place results back, write over old selection

		closebuffer tempBuffer;
	} \
	else \
	{
		beep;
	}
}

# Check spelling in theBuffer, make a new window containing the misspelled words.
proc SpellDocument theBuffer \
{
	global staggerOpenX staggerOpenY windowHeight

	set theName [newbuffer [NewWindowName]];			# make a buffer to hold the output of the spell command
	setmark $theBuffer temp;							# remember what was selected, we will not disturb it
	selectall $theBuffer;								# make a selection of all of the text in the passed buffer
	execute $theName {catch {exec spell << [lindex [selectedtextlist $theBuffer] 0]} theResult; set theResult};
	gotomark $theBuffer temp;							# put selection back
	setselectionends $theName 0 0;						# move to the top of the output buffer

	flushundos $theName;								# get rid of any undos we made here, just to be nice
	cleardirty $theName;								# make it non-modified
	openwindow $theName $staggerOpenX $staggerOpenY [expr 7*50] $windowHeight "" 8 black lightyellow;	# make a new window, set width to roughly 50 columns of an 7 pixel wide font
	UpdateStaggerPosition;
	clearmark $theBuffer temp;							# get rid of temp selection
}

# Get an address, and mail the contents of theBuffer
# to that address.
# NOTE: this would be much better handled with a Tk dialog
proc MailDocument theBuffer \
{
	global mailRecipientChoice;
	global mailSubjectChoice;

	# get recipient
	set recipient [set mailRecipientChoice [textdialog "Mail $theBuffer To:" $mailRecipientChoice]]
	# get subject
	set subject [set mailSubjectChoice [textdialog "Subject:" $mailSubjectChoice]]

	newbuffer tempBuffer;								# create temp buffer
	setmark $theBuffer temp;							# remember what was selected, we will not disturb it
	selectall $theBuffer;								# make a selection of all of the text in the passed buffer
	copy $theBuffer tempBuffer;							# copy selection to temp buffer
	gotomark $theBuffer temp;							# put selection back
	clearmark $theBuffer temp;							# get rid of temp selection
	setselectionends tempBuffer 0 0;					# move to the top of the temp buffer
	insert tempBuffer "To: $recipient\nSubject: $subject\n\n";	# insert recipient and subject to buffer
	set theResult 0;									# predefine no error happend
	selectall tempBuffer;								# select what to mail

	if {[catch {exec /usr/lib/sendmail -t << [lindex [selectedtextlist tempBuffer] 0]} theResult]!=0} \
	{
		okdialog "Mail status:\n\n$theResult\n"
	}
	closebuffer tempBuffer;								# get rid of temp buffer
}

# mail contents of chosen windows to chosen recipients
proc MailWindows {} \
{
	foreach theBuffer [listdialog "Choose Window(s) to Mail:" [windowlist]] \
	{
		MailDocument $theBuffer
	}
}

# Strip the white space at the ends of lines, attempt to leave the selection as it was.
# return the number of characters removed
proc StripWhite {theWindow} \
{
	set initialChars [lindex [textinfo $theWindow] 1];			# find out how many characters we have at the start
	set numRemoved 0;
	setmark $theWindow temp;									# remember what was selected, we will not disturb it
	TextToBuffer tempFindBuffer {[ \t]+$};						# load up the expression
	TextToBuffer tempReplaceBuffer {};							# replace with nothing
	setselectionends $theWindow 0 0;							# move to the top of the buffer to perform the search
	if {[catch {replaceall $theWindow tempFindBuffer tempReplaceBuffer -regex} message]==0} \
	{
		set finalChars [lindex [textinfo $theWindow] 1];		# find out how many characters we have now that we are done
		set numRemoved [expr $initialChars-$finalChars];
	} \
	else \
	{
		okdialog $message
	}
	gotomark $theWindow temp;									# put back the user's selection
	clearmark $theWindow temp;									# get rid of temp selection
	return $numRemoved
}

# Set line termination to the given sequence
proc SetLineTermination {theWindow theTermination} \
{
	setmark $theWindow temp;									# remember what was selected, we will not disturb it
	TextToBuffer tempFindBuffer {\r\n|\r|\n};					# load up the expression
	TextToBuffer tempReplaceBuffer $theTermination;				# replace with given characters
	setselectionends $theWindow 0 0;							# move to the top of the buffer to perform the search
	catch {replaceall $theWindow tempFindBuffer tempReplaceBuffer -regex}
	gotomark $theWindow temp;									# put back the user's selection
	clearmark $theWindow temp;									# get rid of temp selection
}

# insert a character into theWindow, and then home the window so that
# the character is visible
proc InsertAndHome {theWindow theCharacter} \
{
	insert $theWindow $theCharacter;							# insert the character
	HomeWindowToSelectionStart $theWindow -lenient;				# make sure it is visible
}

# Inserts newline if the cursor position is above a given value
# This is a simple way to implement wrapping lines at word boundaries
# Thanks to Juergen Reiss
proc WrapLine theBuffer \
{
	global lineWrap;
	global lineWrapColumn;
	set ends [getselectionends $theBuffer];						# get end of selection
	set start [lindex $ends 0];									# line number
	set pos [lindex [positiontolineoffset $theBuffer $start] 1];# get column number
	if {$pos>$lineWrapColumn} \
	{
		InsertAndHome $theBuffer "\n";							# insert newline
	} \
	else \
	{
		InsertAndHome $theBuffer " ";							# insert space
	}
}

# Changes the status of line-wrapping
proc WrapOnOff {} \
{
	global lineWrap;
	set temp "off";
	if {$lineWrap} \
	{
		set temp "on";
	}
	set lineWrap [yesnodialog "Line wrapping is currently '$temp'\nDo you want to have line wrapping on?"];
	if {$lineWrap} \
	{
		bindkey space		{x0000000000} {WrapLine [ActiveWindowOrBeep]};		# when space is hit, see if above the given column and insert newline if needed
	} \
	else \
	{
		unbindkey space		{x0000000000}
	}
}

# Changes the column at which lines wrap
proc WrapNewColumn {} \
{
	global lineWrapColumn;
	set lineWrapColumn [expr [textdialog "New line wrap column:" $lineWrapColumn]];		# get new column limit from user
}

# Get text to evaluate from theWindow, then stick in a newline.
proc EvalText {theWindow} \
{
	set ends [getselectionends $theWindow]
	set start [lindex $ends 0]
	set end [lindex $ends 1]
	if {$start==$end} \
	{
		set theLine [lindex [positiontolineoffset $theWindow $start] 0];			# get line we want to select
		set start [lineoffsettoposition $theWindow $theLine 0];						# point to the start
		set end [lineoffsettoposition $theWindow [expr $theLine+1] 0];				# point to the end
	}
	if {[lindex [positiontolineoffset $theWindow $end] 1]==0} \
	{
		if {$start!=$end} \
		{
			incr end -1;															# move back past last newline
		}
	}
	setselectionends $theWindow $start $end;										# make only one selection, even if more than one
	set theResult [lindex [selectedtextlist $theWindow] 0]\n;						# get contents of selection to return (add in new line)
	setselectionends $theWindow $end $end;											# move to the end
	breakundo $theWindow;															# force a break in the undo stream for this window
	InsertAndHome $theWindow "\n";													# add newline
	set theResult;																	# return this
}

# this is called be e93rc N times a second, for us to do any work we need to do
# this is a dummy proc that we can override elsewhere
proc RespondToTimerEvent {} {}

# When a shell command arrives, this handles it
# NOTE: ShellCommand is a procedure that the editor expects to be defined.
# The editor calls this procedure in response to certain events.
# The following is a list of the commands that are currently defined:
# initialargs	-- sent when the editor begins, args is the list of command line arguments
# open			-- editor is asked to open documents, args is path name list
# close			-- editor is asked to close documents, args is buffer list
# quit			-- editor is asked to quit, args is not used
proc ShellCommand {theCommand args} \
{
	switch $theCommand \
	{
		"initialargs"
		{
			OpenList $args;							# later, if we want, we can interpret this for command line switches, or whatever
		}
		"open"
		{
			OpenList $args
		}
		"close"
		{
			foreach theWindow $args \
			{
				AskClose $theWindow
			}
		}
		"quit"
		{
			TryToQuit
		}
		"timer"
		{
			RespondToTimerEvent
		}
		"taskcomplete"
		{
		}
	}
}

# display the e93 about box
proc AboutBox {} \
{
	global sysPrefsDir imagesDir tcl_rcFileName;

	catch {image delete about}
	image create photo about -file [file join $sysPrefsDir $imagesDir about.ppm]

	set w .image1
	catch {destroy $w};		# if the window already exists, get rid of it
	toplevel $w
	wm title $w "About e93"
	wm geometry $w +300+300

	frame $w.buttons
	pack $w.buttons -side bottom -fill x -pady 2m
	button $w.buttons.ok -text Ok -command "destroy $w"
	pack $w.buttons.ok -side left -expand 1

	label $w.l1 -image about

	label $w.l2 -text "[version]\nTcl: [info tclversion]\nStartup Script: $tcl_rcFileName\n\n$::tcl_platform(os) $::tcl_platform(osVersion) $::tcl_platform(machine)\nProcess ID: [pid]\n\nNumber of open windows: [llength [windowlist]]\nNumber of open buffers: [llength [bufferlist]]\n\nFor the latest version, visit www.e93.org"

	pack $w.l1 -side top
	pack $w.l2 -side bottom
}

# Add another directory path to the directory menu.
proc AddDirectoryMenu {thePath} \
{
	addmenu {Directory} LASTCHILD 1 "$thePath" {} "cd \{$thePath\}"
}

# Define the menus.
addmenu {} LASTCHILD 1 "e93" "" ""
	addmenu {e93} LASTCHILD 1 "About..."						{}				{AboutBox}
	addmenu {e93} LASTCHILD 0 "space0"							{\\S}			{}

	addmenu {e93} LASTCHILD 1 "Help for e93"					{}				{OpenList [file join $sysPrefsDir README.e93]}
	addmenu {e93} LASTCHILD 1 "Help for regular expressions"	{}				{OpenList [file join $sysPrefsDir README.regex]}
	addmenu {e93} LASTCHILD 1 "Help for syntax maps"			{}				{OpenList [file join $sysPrefsDir README.syntaxmaps]}

addmenu {} LASTCHILD 0 "space0" {\\S}	{}

addmenu {} LASTCHILD 1 "File" "" ""
	addmenu {File} LASTCHILD 1 "New"							{\\Kn}			{NewWindow}
	addmenu {File} LASTCHILD 1 "Open..."						{\\Ko}			{OpenList [opendialog "Open File:"]}
	addmenu {File} LASTCHILD 1 "Open Selection"					{\\Kd}			{SmartOpenList [SelectLineWhenNoSelection [set theWindow [ActiveWindowOrBeep]]; selectedtextlist $theWindow]}
	addmenu {File} LASTCHILD 1 "Include..."						{}				{IncludeList [set theWindow [ActiveWindowOrBeep]] [opendialog "Include:"];HomeWindowToSelectionStart $theWindow}
	addmenu {File} LASTCHILD 0 "space0"							{\\S}			{}
	addmenu {File} LASTCHILD 1 "Close"							{\\Kw}			{AskClose [ActiveWindowOrBeep]}
	addmenu {File} LASTCHILD 1 "Close All"						{\\KW}			{foreach theWindow [windowlist] {AskClose $theWindow}}
	addmenu {File} LASTCHILD 1 "Save"							{\\Ks}			{AskSave [ActiveWindowOrBeep]}
	addmenu {File} LASTCHILD 1 "Save As..."						{\\KS}			{AskSaveAs [ActiveWindowOrBeep]}
	addmenu {File} LASTCHILD 1 "Save To..."						{}				{AskSaveTo [ActiveWindowOrBeep]}
	addmenu {File} LASTCHILD 1 "Save All"						{}				{AskSaveAll}
	addmenu {File} LASTCHILD 1 "Revert To Saved"				{}				{AskRevert [ActiveWindowOrBeep]}
	addmenu {File} LASTCHILD 0 "space1"							{\\S}			{}
	addmenu {File} LASTCHILD 1 "Print..."						{\\Kp}			{PrintBuffer [ActiveWindowOrBeep]}
	addmenu {File} LASTCHILD 1 "Print Selection..."				{\\KP}			{PrintSelection [ActiveWindowOrBeep]}
	addmenu {File} LASTCHILD 1 "Print Windows..."				{}				{PrintWindows}
	addmenu {File} LASTCHILD 0 "space2"							{\\S}			{}
	addmenu {File} LASTCHILD 1 "Quit"							{\\Kq}			{TryToQuit}

addmenu {} LASTCHILD 1 "Edit" "" ""
	addmenu {Edit} LASTCHILD 1 "Undo/Redo Toggle"				{\\Kz}			{if {[undotoggle [set theWindow [ActiveWindowOrBeep]]]!=0} {HomeWindowToSelectionStart $theWindow} else {beep}}
	addmenu {Edit} LASTCHILD 1 "Undo"							{\\Ku}			{if {[undo [set theWindow [ActiveWindowOrBeep]]]!=0} {HomeWindowToSelectionStart $theWindow} else {beep}}
	addmenu {Edit} LASTCHILD 1 "Redo"							{\\Ky}			{if {[redo [set theWindow [ActiveWindowOrBeep]]]!=0} {HomeWindowToSelectionStart $theWindow} else {beep}}
	addmenu {Edit} LASTCHILD 1 "Flush Undo/Redo Buffer"			{}				{okcanceldialog "Really flush undos for:\n'[set theWindow [ActiveWindowOrBeep]]'?";flushundos $theWindow}
	addmenu {Edit} LASTCHILD 0 "space0" 						{\\S}			{}
	addmenu {Edit} LASTCHILD 1 "Cut"							{\\Kx}			{SmartCut [set theWindow [ActiveWindowOrBeep]];HomeWindowToSelectionStart $theWindow;flushundos [getclipboard]}
	addmenu {Edit} LASTCHILD 1 "Copy"							{\\Kc}			{SmartCopy [ActiveWindowOrBeep];flushundos [getclipboard]}
	addmenu {Edit} LASTCHILD 1 "Paste"							{\\Kv}			{HomeWindowToSelectionStart [set theWindow [ActiveWindowOrBeep]];paste $theWindow;HomeWindowToSelectionStart $theWindow -lenient}
	addmenu {Edit} LASTCHILD 1 "Clear"							{}				{clear [set theWindow [ActiveWindowOrBeep]];HomeWindowToSelectionStart $theWindow}
	addmenu {Edit} LASTCHILD 0 "space1" 						{\\S}			{}
	addmenu {Edit} LASTCHILD 1 "Clipboards"						{}				{}
		addmenu {Edit Clipboards} LASTCHILD 1 "Show Current Clipboard..."	{}	{ShowCurrentClipboard}
		addmenu {Edit Clipboards} LASTCHILD 1 "Open Current Clipboard..."	{}	{OpenCurrentClipboard}
		addmenu {Edit Clipboards} LASTCHILD 0 "space0"			{\\S}			{setclipboard clip0}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 0"		{\\K0}			{setclipboard clip0}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 1"		{\\K1}			{setclipboard clip1}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 2"		{\\K2}			{setclipboard clip2}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 3"		{\\K3}			{setclipboard clip3}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 4"		{\\K4}			{setclipboard clip4}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 5"		{\\K5}			{setclipboard clip5}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 6"		{\\K6}			{setclipboard clip6}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 7"		{\\K7}			{setclipboard clip7}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 8"		{\\K8}			{setclipboard clip8}
		addmenu {Edit Clipboards} LASTCHILD 1 "Clipboard 9"		{\\K9}			{setclipboard clip9}
	addmenu {Edit} LASTCHILD 0 "space2" 						{\\S}			{}
	addmenu {Edit} LASTCHILD 1 "Select All"						{\\Ka}			{selectall [ActiveWindowOrBeep]}
	addmenu {Edit} LASTCHILD 1 "Unselect Whitespace"			{\\Kb}			{UnselectWhitespace [ActiveWindowOrBeep]}
	addmenu {Edit} LASTCHILD 0 "space3" 						{\\S}			{}
	addmenu {Edit} LASTCHILD 1 "Align Left"						{\\Kbraceleft}	{AlignLeft [ActiveWindowOrBeep]}
	addmenu {Edit} LASTCHILD 1 "Shift Left"						{\\Kbracketleft}	{ShiftLeft [ActiveWindowOrBeep]}
	addmenu {Edit} LASTCHILD 1 "Shift Right"					{\\Kbracketright}	{ShiftRight [ActiveWindowOrBeep]}

addmenu {} LASTCHILD 1 "Find" "" ""
	addmenu {Find} LASTCHILD 1 "Find/Replace..."				{\\Kf}			{AskSearch [ActiveWindowOrBeep]}
	addmenu {Find} LASTCHILD 1 "Find Same Backwards"			{\\KG}			{FindNext [ActiveWindowOrBeep] 1}
	addmenu {Find} LASTCHILD 1 "Find Same"						{\\Kg}			{FindNext [ActiveWindowOrBeep] 0}
	addmenu {Find} LASTCHILD 1 "Find Selection Backwards"		{\\KH}			{FindSelectionNext [ActiveWindowOrBeep] 1}
	addmenu {Find} LASTCHILD 1 "Find Selection"					{\\Kh}			{FindSelectionNext [ActiveWindowOrBeep] 0}
	addmenu {Find} LASTCHILD 0 "space0" 						{\\S}			{}
	addmenu {Find} LASTCHILD 1 "Replace Same Backwards"			{\\KT}			{ReplaceNext [ActiveWindowOrBeep] 1}
	addmenu {Find} LASTCHILD 1 "Replace Same"					{\\Kt}			{ReplaceNext [ActiveWindowOrBeep] 0}
	addmenu {Find} LASTCHILD 0 "space1" 						{\\S}			{}
	addmenu {Find} LASTCHILD 1 "Replace Selections With..."		{\\KR}			{ReplaceSelections [ActiveWindowOrBeep]}
	addmenu {Find} LASTCHILD 0 "space2" 						{\\S}			{}
	addmenu {Find} LASTCHILD 1 "Go To Line..."					{\\Kl}			{selectline [set theWindow [ActiveWindowOrBeep]] [expr [textdialog "Go to line:"]];HomeWindowToSelectionStart $theWindow}
	addmenu {Find} LASTCHILD 1 "Locate Selection"				{\\KL}			{HomeWindowToSelectionStart [ActiveWindowOrBeep] -strict}
	addmenu {Find} LASTCHILD 0 "space3" 						{\\S}			{}
	addmenu {Find} LASTCHILD 1 "Grep -n For..."					{\\KF}			{set theData "grep -E -n [set grepFilesChoice [textdialog "Enter pattern, and file list: (eg. void *.c)" $grepFilesChoice]]"; set theWindow [NewWindow]; task $theWindow $theData}

addmenu {} LASTCHILD 1 "Window" "" ""
	addmenu {Window} LASTCHILD 1 "Tcl console"					{}				{OpenDefaultWindow tclConsole}
	addmenu {Window} LASTCHILD 0 "space0"						{\\S}			{}
	addmenu {Window} LASTCHILD 1 "Get Information"				{\\Ki}			{BufferInfo [ActiveWindowOrBeep]}
	addmenu {Window} LASTCHILD 1 "Set Font..."					{\\KE}			{setfont [set theWindow [ActiveWindowOrBeep]] [fontdialog "Choose Font:" [getfont $theWindow]]}
	addmenu {Window} LASTCHILD 1 "Set Tab Size..."				{\\Ke}			{settabsize [set theWindow [ActiveWindowOrBeep]] [expr [textdialog "New tab size:" [gettabsize $theWindow]]]}
	addmenu {Window} LASTCHILD 0 "space1"						{\\S}			{}
	addmenu {Window} LASTCHILD 1 "Swap Top Windows"				{\\Kspace}		{SwapWindows}
	addmenu {Window} LASTCHILD 1 "Rotate Windows"				{\\Kgrave}		{RotateWindows}
	addmenu {Window} LASTCHILD 1 "Choose Window..."				{\\KO}			{ChooseWindow}
	addmenu {Window} LASTCHILD 0 "space2"						{\\S}			{}
	addmenu {Window} LASTCHILD 1 "Stack Windows"				{}				{StackWindows}
	addmenu {Window} LASTCHILD 1 "Tile Windows"					{}				{TileWindows}
	addmenu {Window} LASTCHILD 0 "space3"						{\\S}			{}
	addmenu {Window} LASTCHILD 1 "Language Mode"				{}				{}
		addmenu {Window "Language Mode"} LASTCHILD 1 "<none>"	{}				{SetHighlightMode [ActiveWindowOrBeep] ""}
	foreach element [syntaxmaps] \
	{
		addmenu {Window "Language Mode"} LASTCHILD 1 $element	{}				"SetHighlightMode \[ActiveWindowOrBeep\] $element"
	}
	addmenu {Window} LASTCHILD 1 "Color Scheme"					{}				{}
	foreach element [lsort -dictionary [array names HighlightSchemes]] \
	{
		addmenu {Window "Color Scheme"} LASTCHILD 1 $element	{}				"SetHighlightScheme \[ActiveWindowOrBeep\] \"$element\""
	}

addmenu {} LASTCHILD 1 "Mark" "" ""
	addmenu {Mark} LASTCHILD 1 "Mark..."						{}				{setmark [ActiveWindowOrBeep] [textdialog "Mark selection with what name?"]}
	addmenu {Mark} LASTCHILD 1 "Unmark..."						{}				{foreach theMark [listdialog "Delete which mark(s):" [marklist [set theWindow [ActiveWindowOrBeep]]]] {clearmark $theWindow $theMark}}
	addmenu {Mark} LASTCHILD 1 "Goto Mark..."					{}				{foreach theMark [listdialog "Go to which mark:" [marklist [set theWindow [ActiveWindowOrBeep]]]] {gotomark $theWindow $theMark; HomeWindowToSelectionStart $theWindow -strict}}

addmenu {} LASTCHILD 1 "Directory" "" ""
	addmenu {Directory} LASTCHILD 1 "Show Current Directory"	{}				{okdialog "Current directory:\n\n[pwd]"}
	addmenu {Directory} LASTCHILD 1 "Set Directory..."			{}				{set thePath [pathdialog "Choose directory:"]; cd "$thePath"; AddDirectoryMenu "$thePath"}
	addmenu {Directory} LASTCHILD 1 "space0"					{\\S}			{}

addmenu {} LASTCHILD 1 "Misc" "" ""
	addmenu {Misc} LASTCHILD 1 "Strip EOL Whitespace"			{}				{set numRemoved [StripWhite [set theWindow [ActiveWindowOrBeep]]];okdialog "$theWindow\n\nCharacters removed: $numRemoved"}
	addmenu {Misc} LASTCHILD 1 "Set Unix LF Line Termination"	{\\KU}				{SetLineTermination [ActiveWindowOrBeep] "\n"}
	addmenu {Misc} LASTCHILD 1 "Set Mac CR Line Termination"	{}				{SetLineTermination [ActiveWindowOrBeep] "\r"}
	addmenu {Misc} LASTCHILD 1 "Set PC CRLF Line Termination"	{}				{SetLineTermination [ActiveWindowOrBeep] "\r\n"}

	addmenu {Misc} LASTCHILD 1 "In All Open Windows ..."		{}				{}
	addmenu {Misc "In All Open Windows ..."} LASTCHILD 1 "Strip EOL Whitespace"			{}	{ActiveWindowOrBeep;set numRemoved 0;foreach theWindow [windowlist] {incr numRemoved [StripWhite $theWindow]};okdialog "Total characters removed: $numRemoved"}
	addmenu {Misc "In All Open Windows ..."} LASTCHILD 1 "Set Unix Line Termination"	{}	{ActiveWindowOrBeep;foreach theWindow [windowlist] {SetLineTermination $theWindow "\n"};okdialog "All windows completed"}
	addmenu {Misc "In All Open Windows ..."} LASTCHILD 1 "Set Mac Line Termination"		{}	{ActiveWindowOrBeep;foreach theWindow [windowlist] {SetLineTermination $theWindow "\r"};okdialog "All windows completed"}
	addmenu {Misc "In All Open Windows ..."} LASTCHILD 1 "Set PC Line Termination"		{}	{ActiveWindowOrBeep;foreach theWindow [windowlist] {SetLineTermination $theWindow "\r\n"};okdialog "All windows completed"}

	addmenu {Misc} LASTCHILD 1 "Uppercase Selection"			{}				{UppercaseSelection [ActiveWindowOrBeep]}
	addmenu {Misc} LASTCHILD 1 "Lowercase Selection"			{}				{LowercaseSelection [ActiveWindowOrBeep]}
	addmenu {Misc} LASTCHILD 1 "Increment Selected Numbers..."	{}				{IncrementSelection [ActiveWindowOrBeep] [textdialog "How much to increment:" 1]}
	addmenu {Misc} LASTCHILD 1 "Enumerate Selections..."		{}				{EnumerateSelection [ActiveWindowOrBeep] [textdialog "Starting value:" 1] [textdialog "How much to increment:" 1]}
	addmenu {Misc} LASTCHILD 1 "Sum of Selected Numbers"		{}				{SumSelection [ActiveWindowOrBeep]}
	addmenu {Misc} LASTCHILD 1 "Sort Selections"				{}				{SortSelection [ActiveWindowOrBeep]}
	addmenu {Misc} LASTCHILD 1 "Reverse Selections"				{}				{ReverseSelection [ActiveWindowOrBeep]}
	addmenu {Misc} LASTCHILD 1 "Refill Selection..."			{\\Kr}			{RefillSelection [ActiveWindowOrBeep]}
	addmenu {Misc} LASTCHILD 1 "Line Wrapping..."				{}				{WrapOnOff}
	addmenu {Misc} LASTCHILD 1 "Get man Page..."				{}				{ManPage [textdialog "Enter man page subject:"]}
	addmenu {Misc} LASTCHILD 1 "Spell"							{}				{SpellDocument [ActiveWindowOrBeep]}
	addmenu {Misc} LASTCHILD 1 "Mail To..."						{}				{MailDocument [ActiveWindowOrBeep]}
	addmenu {Misc} LASTCHILD 1 "Mail Windows To..."				{}				{MailWindows}

addmenu {} LASTCHILD 1 "Tasks" "" ""
	addmenu {Tasks} LASTCHILD 1 "Execute Shell Task"			{\\KKP_Enter}	{task [set theWindow [ActiveWindowOrBeep]] [EvalText $theWindow]}
	addmenu {Tasks} LASTCHILD 1 "Send EOF To Task"				{\\Kk}			{eoftask [ActiveWindowOrBeep]}
	addmenu {Tasks} LASTCHILD 1 "Kill Task"						{\\KK}			{killtask [ActiveWindowOrBeep]}
	addmenu {Tasks} LASTCHILD 1 "Detatch Task"					{}				{detatchtask [ActiveWindowOrBeep]}
	addmenu {Tasks} LASTCHILD 0 "space0"						{\\S}			{}
	addmenu {Tasks} LASTCHILD 1 "Pipe Selections Through..."	{}				{PipeSelection [ActiveWindowOrBeep]}

addmenu {} LASTCHILD 1 "Build" "" ""
	addmenu {Build} LASTCHILD 1 "make"							{\\Km}			{HomeWindowToSelectionStart [set theWindow [ActiveWindowOrBeep]];breakundo $theWindow;task $theWindow make}

# Bind keys to various useful things.
# l=caps lock, s=shift, c=control, 0-7 are additional modifiers such as command, alt, option, command, etc...
# Bind flags		 lsc01234567 (x means don't care, 0 means not pressed, 1 means pressed)

bindkey F1			{x0010000000} {setmark [ActiveWindowOrBeep] F1}
bindkey F1			{x0000000000} {gotomark [set theWindow [ActiveWindowOrBeep]] F1;HomeWindowToSelectionStart $theWindow -strict}
bindkey F2			{x0010000000} {setmark [ActiveWindowOrBeep] F2}
bindkey F2			{x0000000000} {gotomark [set theWindow [ActiveWindowOrBeep]] F2;HomeWindowToSelectionStart $theWindow -strict}
bindkey F3			{x0010000000} {setmark [ActiveWindowOrBeep] F3}
bindkey F3			{x0000000000} {gotomark [set theWindow [ActiveWindowOrBeep]] F3;HomeWindowToSelectionStart $theWindow -strict}
bindkey F4			{x0010000000} {setmark [ActiveWindowOrBeep] F4}
bindkey F4			{x0000000000} {gotomark [set theWindow [ActiveWindowOrBeep]] F4;HomeWindowToSelectionStart $theWindow -strict}

bindkey F5			{x0000000000} {WrapOnOff};
bindkey F5			{x0010000000} {WrapNewColumn};

bindkey KP_Enter	{x0000000000} {execute [set theWindow [ActiveWindowOrBeep]] [EvalText $theWindow];catch {HomeWindowToSelectionStart $theWindow -lenient}}
bindkey Return		{x1000000000} {execute [set theWindow [ActiveWindowOrBeep]] [EvalText $theWindow];catch {HomeWindowToSelectionStart $theWindow -lenient}};  # this is for laptop machines without a keypad
bindkey	Return		{x0010000000} {task [set theWindow [ActiveWindowOrBeep]] [EvalText $theWindow]}; # this is for laptop machines without a keypad
bindkey Help		{x0000000000} {ManPage [textdialog "Enter man page subject:"]}
bindkey F11			{x0000000000} {raiserootmenu}

# Specify initial paths to be placed into the directory menu.

AddDirectoryMenu /
catch {AddDirectoryMenu [glob ~]};		# this can fail if user has no home directory
AddDirectoryMenu [pwd]


# see if the user has his own set of modules specified, if so, skip including the defaults, and get only his
if {[file exists [file join $userPrefsDir $modulesDir]]} \
{
	foreach theFile [glob -nocomplain [file join $userPrefsDir $modulesDir *.tcl]] \
	{
		source $theFile;
	}
} \
else \
{
	# use all system defined modules
	foreach theFile [glob -nocomplain [file join $sysPrefsDir $modulesDir *.tcl]] \
	{
		source $theFile;
	}
}

# get user's additional modules (if any)
if {[file exists [file join $userPrefsDir $auxModulesDir]]} \
{
	foreach theFile [glob -nocomplain [file join $userPrefsDir $auxModulesDir *.tcl]] \
	{
		source $theFile;
	}
}

# get user's preferences (if any)
if {[file exists [file join $userPrefsDir $prefsDir]]} \
{
	foreach theFile [glob -nocomplain [file join $userPrefsDir $prefsDir *.tcl]] \
	{
		source $theFile;
	}
}

