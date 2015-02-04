package provide [file tail [info script]] 1
# give me a menu with all the rc files listed, so I can open and muck with them at will

# we need the list of "SourceFiles" defined in .e93rcl to populate our menu

set packageDir /packages

# Set up the Preferences menu
addmenu {} LASTCHILD 1 "RC files" "" ""
addmenu {"RC files"} LASTCHILD 1 [file tail $tcl_rcFileName]	{}			{OpenList [list $tcl_rcFileName]}

# see if the user has his own set of modules specified,
# if so, skip including the defaults, and get only his
addmenu {"RC files"} LASTCHILD 1	"Modules"				{}			{}
if {[file exists $userPrefsDir/$modulesDir]} \
{
	foreach theFile [lsort [glob $userPrefsDir/$modulesDir/*.tcl]] \
	{
		addmenu {"RC files" "Modules"}					LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"
	}
} \
else \
{
	# use all system defined modules
	foreach theFile [lsort [glob $sysPrefsDir/$modulesDir/*.tcl]] \
	{
		addmenu {"RC files" "Modules"}					LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"
	}
}

# make sure the commands defined in our packages are known to Tcl
auto_mkindex [file dirname $tcl_rcFileName]$packageDir

addmenu {"RC files"}		LASTCHILD 1	"Packages"			{}			{}
foreach theFile [lsort [glob [file dirname $tcl_rcFileName]$packageDir/*.tcl]] \
{
	if {[file exists [file dirname $tcl_rcFileName]$packageDir/$theFile]} \
	{
		set theFile [file dirname $tcl_rcFileName]$packageDir/$theFile
		addmenu {"RC files" "Packages"}					LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"
	} \
	else \
	{
		if {[file exists $theFile]} \
		{
			addmenu {"RC files" "Packages"}				LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"
		}
	}
}

# see if the user has his own set of maps specified,$
# if so, skip list the defaults, and get only his
addmenu {"RC files"}		LASTCHILD 1	"Syntax Maps"			{}			{}
if {[file exists $userPrefsDir/$mapsDir]} \
{
	foreach theFile [lsort [glob $userPrefsDir/$mapsDir/*.tcl]] \
	{
		addmenu {"RC files" "Syntax Maps"}				LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"
	}
} \
else \
{
	# use all system defined color maps
	foreach theFile [lsort [glob $sysPrefsDir/$mapsDir/*.tcl]] \
	{
		addmenu {"RC files" "Syntax Maps"}				LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"

	}
}

# list user's additional syntax maps (if any)
if {[file exists $userPrefsDir/$auxMapsDir]} \
{
	foreach theFile [lsort [glob $userPrefsDir/$auxMapsDir/*.tcl]] \
	{
		addmenu {"RC files" "Syntax Maps"}				LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"
	}
}

# see if the user has his own set of highlight schemes specified,
# if so, skip list the defaults, and get only his
addmenu {"RC files"}		LASTCHILD 1	"Highlight Schemes"			{}			{}
if {[file exists $userPrefsDir/$highlightDir]} \
{
	foreach theFile [lsort [glob $userPrefsDir/$highlightDir/*.tcl]] \
	{
		addmenu {"RC files" "Highlight Schemes"}		LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"
	}
} \
else \
{
	# use all system defined color maps
	foreach theFile [lsort [glob $sysPrefsDir/$highlightDir/*.tcl]] \
	{
		addmenu {"RC files" "Highlight Schemes"}		LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"
	}
}

# list user's additional highlighting schemes (if any)
if {[file exists $userPrefsDir/$auxHighlightDir]} \
{
	foreach theFile [lsort [glob $userPrefsDir/$auxHighlightDir/*.tcl]] \
	{
		addmenu {"RC files" "Highlight Schemes"}		LASTCHILD 1	[file tail [file rootname $theFile]]	{}	"OpenList \{[list $theFile]\}"
	}
}

