# This module is courtesy of Adam Yellen.
#
package provide [file tail [info script]] 1

# Darken the background of theWindow.
# This makes orginization easier when working on many open files.
# This procedure will preserve the hue of the background
proc DarkenWindow {theWindow} \
{
	# get the current foreground and background colors
	set colorstring [getcolors $theWindow]
	set fg [lindex $colorstring 0]
	set bg [lindex $colorstring 1]

	# getcolors returns values as hexidecimal but without the leading 0x, so we must append 0x so that Tcl can understand the value correctly
	set bg "0x$bg";

	# reduce the value only if we can safely do so
	# we don't want to destroy any color information
	set tempR [expr $bg & "0xFF0000"];
	set tempG [expr $bg & "0x00FF00"];
	set tempB [expr $bg & "0x0000FF"];
	if {[expr $tempR >= "0x100000"] && [expr $tempG >= "0x001000"] && [expr $tempB >= "0x000010"]} \
	{
		# reduce value
		set newbg [expr $bg - "0x101010"];
	
		# convert back to hexidecimal
		set newbg [format "%X" $newbg];

		# set the window colors using the original foreground and the new background value
		setcolors $theWindow $fg $newbg
	} \
	else \
	{
		beep
	}
}

# Lighten the background of theWindow.
# This makes orginization easier when working on many open files.
# This procedure will preserve the hue of the background
proc LightenWindow {theWindow} \
{
	# get the current foreground and background colors
	set colorstring [getcolors $theWindow]
	set fg [lindex $colorstring 0]
	set bg [lindex $colorstring 1]

	# getcolors returns values as hexidecimal but without the leading 0x, so we must append 0x so that Tcl can understand the value correctly
	set bg "0x$bg";

	# increase the value only if we can safely do so
	# we don't want to destroy any color information
	set tempR [expr $bg & "0xFF0000"];
	set tempG [expr $bg & "0x00FF00"];
	set tempB [expr $bg & "0x0000FF"];
	if {[expr $tempR <= "0xEF0000"] && [expr $tempG <= "0x00EF00"] && [expr $tempB <= "0x0000EF"]} \
	{
		# increase value
		set newbg [expr $bg + "0x101010"];
	
		# convert back to hexidecimal
		set newbg [format "%X" $newbg];

		# set the window colors using the original foreground and the new background value
		setcolors $theWindow $fg $newbg
	} \
	else \
	{
		beep
	}
}

addmenu {Window} LASTCHILD 0 "space0"						{\\S}						{}
addmenu {Window} LASTCHILD 1 "Lighten Background"			{\\KKP_Add}					{LightenWindow [ActiveWindowOrBeep]}
addmenu {Window} LASTCHILD 1 "Darken Background"			{\\KKP_Subtract}			{DarkenWindow [ActiveWindowOrBeep]}
