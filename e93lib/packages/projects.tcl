package provide [file tail [info script]] 1

package require worksheet.tcl

set statefile [file dirname $tcl_rcFileName]/state$tcl_platform(platform).e93

global Projects

set currentProject "none"
set Projects(none) ""

addmenu {} LASTCHILD 1 "Projects" "" ""
	addmenu {Projects} LASTCHILD 1 "Save State"				{}					{SaveState}
	addmenu {Projects} LASTCHILD 1 "Add a Project..."		{}					{source [opendialog "Choose a project file:"]}
	addmenu {Projects} LASTCHILD 1 "Clear project list"		{}					{unset Projects}
	addmenu {Projects} LASTCHILD 1 {Space0}					{\\S}				{}

# load up the last state of e93
#
# TODO: in the future we will have more than one state
# these will be know as "projects", hence the name of this rc file
if {[file exists $statefile]} {
	global statefile
	catch \
	{
		source $statefile} result
		# load any project specific .tcl files
	    catch {foreach file [glob *Project.tcl] \
	    {
			source $file
	    }
    }
}


namespace eval Project \
{
	# save the body of the current TryToQuit proc since we only want to add functonality not override it
	if {![info exists ::super_TryToQuit]} \
	{
		variable super_TryToQuit [info body TryToQuit]
	}
}

# Psudo OO with tcl. Extend TryToQuit
# TODO: unsaved windows should not be added to the state since they
# will not exist in the file system. However, they may get added later in the
# trytoquit so we can't just ignore them. What to do?
proc TryToQuit {} \
{
	catch SaveState
	eval $Project::super_TryToQuit
}

proc savearray {a chan} \
{
    upvar 1 $a theArray
    if {[array exists theArray]} \
    {
	    foreach name [array names theArray] \
	    {
			puts $chan "set \"$a\($name\)\" \{$theArray($name)\}"
	    }
    }
}

proc SaveState {} {
	global statefile currentProject Projects
	set outfile [open $statefile w+]
	puts $outfile "cd \{[pwd]\}"
	# TODO: This will change the window order, preserve it
	set numWin [llength [windowlist]]
	while {$numWin>=0} \
	{
		set window [lindex [windowlist] $numWin]
		incr numWin -1
		if {[file exists $window]} \
		{
			set theRect [getrect $window]
			puts $outfile "OpenList \[list \{$window\}\]"
			puts $outfile "setrect \{$window\} $theRect"
			puts $outfile "setselectionends [list $window] [getselectionends $window]"
			puts $outfile "HomeWindowToSelectionStart [list $window]"
			#foreach mark [marklist $window] \
			#{
			#	gotomark $window $mark;
			#	puts $outfile "setselectionends $window [getselectionends $window]"
			#	puts $outfile "setmark $window $mark"
			#}
		}
	}
	puts $outfile "AddDirectoryMenu \[pwd\]"
	global breakpointArray
	savearray breakpointArray $outfile
	# TODO: Save var values?
	# TODO: Save procs?
	puts $outfile "global statefile currentProject Projects"
    foreach project [array names Projects] \
    {
    	puts $outfile "set Projects($project)	$Projects($project)"
		puts $outfile "addmenu {Projects} LASTCHILD 1 \"Set project - $project\"			{}					{source $Projects($project); global currentProject; set currentProject $project}"
    }

	puts $outfile "set currentProject $currentProject"
	puts $outfile "catch {source $Projects($currentProject)}"

	close $outfile
}


