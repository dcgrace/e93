package provide [file tail [info script]] 1

package require scratch.tcl
	# We need NewWindowName to create windows for the man output

# Iconify all the windows
proc MinimizeWindows {} \
{
	foreach theWindow [windowlist] \
	{
		minimizewindow $theWindow
	}
}

# Restore all the windows to normal size.
proc RestoreWindows {} \
{
	foreach theWindow [windowlist] \
	{
		unminimizewindow $theWindow
	}
}

# Add a couple of commands to the Misc menu
	addmenu {Misc} LASTCHILD 0 "space10"				{\\S}							{}
	addmenu {Misc} LASTCHILD 1 "CD Here"				{\\Kperiod}						{cd [file dirname [ActiveWindowOrBeep]]; AddDirectoryMenu [pwd]}
	addmenu {Misc} LASTCHILD 1 "Choose a File"			{\\KC}							{insert [ActiveWindowOrBeep] [opendialog "Choose a file:"]}
	addmenu {Misc} LASTCHILD 1 "Choose a Directory"		{\\KD}							{insert [ActiveWindowOrBeep] [pathdialog "Choose directory:" [pwd]]}

# Add a couple of commands to the Window menu
	addmenu {Window} LASTCHILD 1 "Iconify All Windows"	{\\Kx1010000000KP_Subtract}		{MinimizeWindows}
	addmenu {Window} LASTCHILD 1 "Restore All Windows"	{\\Kx1010000000KP_Add}			{RestoreWindows}
	addmenu {Window} LASTCHILD 1 "Minimize Window"		{\\K<}							{minimizewindow [ActiveWindowOrBeep]}
	addmenu {Window} LASTCHILD 1 "Unminimize Window"	{\\K>}							{unminimizewindow [ActiveWindowOrBeep]}

# This is an improved version of the proc from e93rc.tcl, it should be rolled into the base e93 release.
# Quiz user to find out if he really wants to "revert".
proc AskRevert {theBuffer} \
{
	if {[isdirty $theBuffer]} \
	{
		okcanceldialog "Do you really want to discard changes to:\n'$theBuffer'"
		revertbuffer $theBuffer
		catch {setbuffervariable $theBuffer mtime [file mtime $theBuffer]};
	} \
	else \
	{
		revertbuffer $theBuffer;			# may be reverting because the file was changed, so just do it without asking
		catch {setbuffervariable $theBuffer mtime [file mtime $theBuffer]};
	}
}

# This is an improved version of the proc from e93rc.tcl, it should be rolled into the base e93 release.

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
					# if the buffer is not dirty, we have made no edits, just reload the file
					if {[isdirty $message]} \
					{
						if {[yesnodialog "WARNING!\n\nIt looks like another application modified:\n$message\nwhile it was open for editing.\nLoad changes?"]} \
						{
							revertbuffer $message
							catch {setbuffervariable $message mtime [file mtime $message]};
						}
					} \
					else \
					{
						revertbuffer $message
						catch {setbuffervariable $message mtime [file mtime $message]};
					}
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

# This is an improved version of the proc from e93rc.tcl, it should be rolled into the base e93 release.
# Requires scratch.tcl

# Get a manual page, and dump it into a window.
proc ManPage theParams \
{
	global staggerOpenX staggerOpenY windowHeight style_keyword style_string

	set theName [newbuffer [NewWindowName "man page for $theParams"]];			# make a buffer to hold the man page
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

# no-op the exit proc for Tcl scripts
proc exit {} {}

# ignore case by default in search dialogs
set			sd_ignoreCase			1;					# ignore case by default

# Get the font string to use from the following:   [fontdialog "Choose Font:" [getfont [ActiveWindowOrBeep]]]

# re-define Deep Blue with a new font
set hs_deepblue($style_default)			{white			27277a 				"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_deepblue($style_delimiter)		{yellow3		""					""}
set hs_deepblue($style_keyword)			{gold			""					"-*-IBM3270-bold-o-normal-sans-9-*-*-*-*-*"}
set hs_deepblue($style_type)			{yellow2		""					"-*-IBM3270-bold-o-normal-sans-9-*-*-*-*-*"}
set "HighlightSchemes(Deep Blue)"		hs_deepblue

# define some highlight schemes
# NOTE: the empty string (or any invalid font name) forces e93 to choose a reasonable font.
set hs_e93($style_default)				{white			37378a		 	"-b&h-lucida console-medium-r-normal-sans-9-120-75-75-m-70-iso8859-1"}
set hs_e93($style_delimiter)			{yellow3		""				""}
set hs_e93($style_keyword)				{yellow			""				"-b&h-lucida console-bold-r-normal-sans-9-120-75-75-m-70-iso8859-1"}
set hs_e93($style_type)					{yellow			""				"-b&h-lucida console-bold-r-normal-sans-9-120-75-75-m-70-iso8859-1"}
set hs_e93($style_directive)			{cyan			""				"-b&h-lucida console-bold-r-normal-sans-9-120-75-75-m-70-iso8859-1"}
set hs_e93(selection)					{black			yellow}
set "HighlightSchemes(e93)"				hs_e93

# define a plain highlight scheme based on Dev Workshop without the garish colors
set hs_simplewhite($style_default)		{black			white				"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set "HighlightSchemes(Simple White)"	hs_simplewhite

set hs_manpage($style_default)			{white			darkblue		"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_manpage($style_keyword)			{yellow			""				"-*-IBM3270-bold-r-normal-sans-9-*-*-*-*-*"}
set "HighlightSchemes(Man Page)"		hs_manpage

