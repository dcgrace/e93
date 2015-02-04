package provide [file tail [info script]] 1

package require scratch.tcl
	# We need to create "scratch" windows for the make output

package require snack

deletemenu {Build}

# A proc to make a target of your choice which will remember the target
# the next time you call it
#
set lastTarget "-f makefile.win DEBUGGING=ON"

# Ask the use what to make. Execute the make command, put the output in a new window
proc MakeTarget {} \
{
	global lastTarget
	set target [textdialog "Enter make target:" $lastTarget]
	set lastTarget $target
	Make $target
}

proc BuildComplete {buffer status} \
{
	global sysPrefsDir
	
	snack::sound snd
	if {$status==0} \
	{
		snd read "$sysPrefsDir/sounds/groovy.snd"
		snd play
	} \
	else \
	{
		if {$status==9} \
		{
			snd read "$sysPrefsDir/sounds/gun.snd"
			snd play
		} \
		else \
		{
			snd read "$sysPrefsDir/sounds/duhhh.snd"
			snd play
		}
	}
}

# Execute the make command, put the output in a new window
proc Make {target}\
{
	switch $::tcl_platform(platform) \
	{
		"unix" \
		{
			set theData "make $target"		
		}
		"windows" \
		{
			set theData "nmake $target"
		}
	}
	set theWindow [NewScratchWindow "[pwd] make $target.mk"]
	task $theWindow $theData BuildComplete
}

# A rather lame "language" syntaxmapname to highlight some of the crap nmake spits out
# I was playing, you may not want this
set syntaxmapname "Make"

set			style_error			[expr $lastStyle + 1];
set			lastStyle			$style_error;	# set this to allow others to add more styles at the end

set			style_path			[expr $lastStyle + 1];
set			lastStyle			$style_path;	# set this to allow others to add more styles at the end

addsyntaxmap $syntaxmapname \
{
	exp {e_keywords				"<(parsed|loaded|checking|wrote|done)>"}
	exp {e_error				"<(error|errors)>"}
	exp {e_path					"<(^([^:^\n]+):([0-9]+))>"}

	map {m_keywords				e_keywords				{}								$style_keyword		0				0}
	map {m_error				e_error					{}								$style_error		0				0}
	map {m_path					e_path					{}								$style_path			0				0}

	root {m_path m_keywords m_error}
}

# add our Make language mode to the menu
addmenu {Window "Language Mode"} LASTCHILD 1 "$syntaxmapname"		{}			"SetHighlightMode \[ActiveWindowOrBeep\] \"$syntaxmapname\""

#fontdialog "Choose Font:" [getfont [ActiveWindowOrBeep]]

# make files
set hs_make($style_default)		{gold			DarkSlateGray 				"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_make($style_error)		{red			DarkSlateGray				"-*-IBM3270-bold-r-normal-sans-9-*-*-*-*-*"}
set hs_make($style_keyword)		{orange			DarkSlateGray				"-*-IBM3270-bold-r-normal-sans-9-*-*-*-*-*"}
set hs_make($style_path)		{yellow			DarkSlateGray				"-*-courier new-medium-r-normal-*-9-*-*-*-*-*"}
set hs_make(selection)			{midnightblue   yellow3}
set "HighlightSchemes($syntaxmapname)"	hs_make

# Add it to the list
addmenu {Window "Color Scheme"} LASTCHILD 1 "$syntaxmapname"	{}				"SetHighlightScheme \[ActiveWindowOrBeep\] \"$syntaxmapname\""

# we want e93 to know what to do when it encounters a .mk file extension
set	extensionHuntExpression($syntaxmapname)		{^((.*\.mk))$}
set	extensionTabSize($syntaxmapname)			4
set	extensionColorScheme($syntaxmapname)		"$syntaxmapname"
set	extensionMapName($syntaxmapname)			"$syntaxmapname"

set lastMakeWindow {}

proc GoToNextError {} \
{
	global lastMakeWindow
	DisplayError ""

	if {[lsearch [bufferlist] $lastMakeWindow] != -1} \
	{
		set theWindow $lastMakeWindow
		TextToBuffer tempFindBuffer {^([^:^\n]+):([0-9]+)}
		FindBeep $theWindow tempFindBuffer -regex -wrap
		SmartOpenList [selectedtextlist $theWindow]
		TextToBuffer tempFindBuffer ": "
		FindBeep $theWindow tempFindBuffer -regex
		TextToBuffer tempFindBuffer ".*"
		FindBeep $theWindow tempFindBuffer -regex
		DisplayError [lindex [selectedtextlist $theWindow] end]
	} \
	else \
	{
		set lastMakeWindow {}
	}
}

addmenu {} LASTCHILD 1 "Make" "" ""
	addmenu {Make} LASTCHILD 1 "Make"						{\\Km}	{Make ""}
	addmenu {Make} LASTCHILD 1 "Make Target"				{\\KM}	{MakeTarget}
	addmenu {Make} LASTCHILD 1 "Make Debug"					{}		{Make "DEBUGGING=ON"}
	addmenu {Make} LASTCHILD 1 "Next Error"					{\\KN}	{GoToNextError}


