package provide [file tail [info script]] 1

package require scratch.tcl
	# We need to create "scratch" windows for the ant output

package require make.tcl
	# We add ourselves to the Make menu

# set the ANT_HOME environment variable only if there isn't one set already
if {[catch {$env(ANT_HOME)}] !=0} \
{
	set env(ANT_HOME) "D:/Java/jakarta-ant-1.5.1"
}

# Evaluate the targets from build.xml and build a list to choose from.
proc ChooseAntTarget {} \
{
	global env

	set targets [exec sh $env(ANT_HOME)/bin/ant -projecthelp]
	set index 	[string first "Subtargets" $targets]
	set index   [expr $index + 11]
	set targets [string range $targets $index end]
	set index   [expr [string first "Default" $targets] -1]
	set targets [string range $targets 0 $index]

	listdialog "Select a target" $targets
}

# Executes Ant.
#
# Asks for the project's root directory and for the target.
# Ant expects the build.xml in the current directory or in
# one of its subdirectories.
proc ExecAnt {{target {}} {buildfile {build.xml}}} \
{
	global env

	if ![file exists $buildfile] \
	{
		set buildfile [opendialog "Choose a Ant build file:"]
	}

	Run "sh $env(ANT_HOME)/bin/ant -buildfile $buildfile $target"  IBM XML
}

addmenu {Make} LASTCHILD 0 "space0"			{\\S}		{}
addmenu {Make} LASTCHILD 1 "Ant"			{\\KA}		{ExecAnt}
addmenu {Make} LASTCHILD 1 "Ant Target..."	{}			{ExecAnt [ChooseAntTarget]}

# Given a "filename" from a list, try to interpret it as the output from Ant
#
#
# If it looks like that, then extract the filename and line number
# and return them in a list, otherwise return an error
proc SmartOpenListAntJavac {listItem} \
{
	if {[regexp {^\s+[\[javac\]]+\s([^:]:[^:]+):([0-9]+)} $listItem whole tempFile tempLine]} \
	{
		lappend temp $tempFile $tempLine;	# make a list of 2 elements (filename and line number)
		lappend temp2 $temp;				# return list of lists
		return $temp2;
	}
	return -code error;
}

set	smartOpenProcs(AntJavac)			{SmartOpenListAntJavac}
