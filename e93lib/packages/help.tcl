package provide [file tail [info script]] 1

addmenu {} LASTCHILD 1 "Help"					{}		{}

# I have cygwin tools on my machine, so man works on Windows too, 
# if you don't then move this into the unix case below
addmenu {Help} LASTCHILD 1 "Get man Page..."				{}				{ManPage [textdialog "Enter man page subject:"]}
addmenu {Help} LASTCHILD 1 "Man Selection"					{}				{ManPage [selectedtextlist [ActiveWindowOrBeep]]}
#

switch $::tcl_platform(platform) \
{
	"unix" \
	{
	}
	"windows" \
	{
		array set HelpFiles [list "e93 Help" "[file dirname [info nameofexecutable]]/e93.hlp"]

		regsub -all "\\." [info tclversion] "" version
		regsub -all {\\} [info library] {/} library
		set tclHelp [file join $library "../../doc/tcl$version.hlp"]
		
		
		array set HelpFiles [list "tcl Help" "$tclHelp"]
		array set HelpFiles [list "XML Help" "P:/MYDOCU~1/XML/XML4J_3_0_0EA3/docs/docs/html/XML.HLP"]

		proc OpenHelp {HelpFile} \
		{
			windowswinhelp $HelpFile [selectedtextlist [ActiveWindowOrBeep]]
		}

		foreach name [array names HelpFiles] \
		{
			set HelpFile $HelpFiles($name)
			if {[file exists $HelpFile]} \
			{
				addmenu {Help} LASTCHILD 1 "$name"			{}		"OpenHelp [list $HelpFile]"
			}
		}

		proc OpenHTMLHelp {HelpFile} \
		{
			windowshtmlhelp $HelpFile [selectedtextlist [ActiveWindowOrBeep]]
		}

		set tclHelp [file join $library "../../doc/ActiveTclHelp.chm"]
		array set HTMLHelpFiles [list "e93 HTML Help" "[file dirname [info nameofexecutable]]/e93.chm"]
		array set HTMLHelpFiles [list "tcl HTML Help" "$tclHelp"]
		
		
		foreach name [array names HTMLHelpFiles] \
		{
			set HelpFile $HTMLHelpFiles($name)
			if {[file exists $HelpFile]} \
			{
				addmenu {Help} LASTCHILD 1 "$name"			{}		"OpenHTMLHelp [list $HelpFile]"
			}
		}
	}
}

