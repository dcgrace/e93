package provide [file tail [info script]] 1

# save the body of the current AskClose proc since we only want to add functonality not override it
namespace eval Scratch \
{
	if {![info exists ::Scratch::AskClose]} \
	{
		rename AskClose ::Scratch::AskClose
	}
}

# Psudo OO with tcl. Extend AskClose
proc AskClose {theBuffer} \
{
	if {[Scratch $theBuffer]} \
	{
		closebuffer $theBuffer
	} \
	else \
	{
		::Scratch::AskClose $theBuffer
	}
}

# Make a throw-away window to hold the output of a command we want tasked, run the task in it
proc Run {command {schemeName "Terminal"} {mapName ""}} \
{
	set theWindow [NewScratchWindow $command 0 $schemeName $mapName]
	task $theWindow $command
}

# Return a good name for a new buffer prefaced with "name" and a number appended only if it's needed to make it unique
proc NewBufferName {{name "Untitled"}} \
{
	set theName $name;									# use the name passed in
	set untitledCounter 0;								# always start renaming at 0
	while {[catch {haswindow $theName} message]==0} \
	{
		if {[haswindow $theName]==1} \
		{
			set theName "$name - $untitledCounter";		# make a new name
			incr untitledCounter;						# make the count one larger for next time
		}
	}
	return $theName;									# return the name of the new window
}

# ** OVERRIDES: NewWindowName in e93rc **
# Return a good name for a new window.
proc NewWindowName { {base "Untitled"}} \
{
	return [NewBufferName $base];				# return the name of the new window
}

# Empty a buffer before using it again
proc ClearBuffer {name} \
{
	selectall $name;
	clear $name;
}

# Return 1 if the passed window name is a "scratch" window.
proc Scratch {theWindow} \
{
	expr {[catch {getbuffervariable $theWindow scratch} errorMessage] == 0}
}

# Make an new grep window, include the grep command if you want it in the title
proc NewGrepWindow {{command ""}} \
{
	NewScratchWindow "Grep $command" 1
}

# Create a new window with a name, return the name,
# if lowpriority is not 0 the buffer will persist for the life of the e93 session
proc NewScratchWindow {name {lowpriority 0} {schemeName ""} {mapName ""} } \
{
	set temp [GetDefaultStyleInfo $name]
	set tabSize [lindex $temp 1]
	if {$schemeName == ""} \
	{
		set schemeName [lindex $temp 2]
	}
	if {$mapName == ""} \
	{
		set mapName [lindex $temp 3]
	}
	OpenStaggeredWindow [NewScratchBuffer $name $lowpriority] $tabSize $schemeName $mapName
	return $name
}

# Create a new buffer with a name, return the name
# the buffer will persist for the life of the e93 session
proc NewScratchBuffer {name {lowpriority 0} {reuse 1}} \
{
	# see if the buffer is still hanging around, reuse it if it is
	if {$reuse} \
	{
		set reuse 0
		if {[lsearch [bufferlist] $name] != -1} \
		{
			set reuse 1
		}
	}
	
	if {$reuse}  \
	{
		if {[hastask $name]} \
		{
			killtask $name
		}
		ClearBuffer $name
	} \
	else \
	{
		newbuffer [NewBufferName $name];
		if $lowpriority \
		{
			setbuffervariable $name lowpriority "";
		}
		setbuffervariable $name scratch "";		# either way it's scratch
	}

	return $name;								# return the name of the new window
}

# close all our throw away windows, man, grep, make, various tasks, etc...
proc CloseAllScratchWindows {} \
{
	foreach theWindow [windowlist] \
	{
		if {[Scratch $theWindow]} \
		{
			# TODO: not if a task is running
			AskClose $theWindow
		}
	}
}

addmenu {Window} LASTCHILD 1 "Close All Scratch Windows"	{}						{CloseAllScratchWindows}

# define a highlight scheme that looks like an ugly DOS terminal
set hs_terminal($style_default)			{green			black			"-*-Lucida Console-book-r-normal-*-10-*-*-*-*-*"}
set hs_terminal($style_comment)			{green			black			""}
set hs_terminal($style_string)			{blue			black			""}
set hs_terminal($style_char)			{blue			black			""}
set hs_terminal($style_digit)			{black			black			""}
set hs_terminal($style_operator)		{red			black			""}
set hs_terminal($style_delimiter)		{green			black			""}
set hs_terminal($style_keyword)			{darkblue		black			""}
set hs_terminal($style_type)			{blue			black			""}
set hs_terminal($style_directive)		{blue			black			""}
set hs_terminal($style_function)		{red			black			""}
set hs_terminal(selection)				{black			yellow			""}
set "HighlightSchemes(Terminal)"		hs_terminal

