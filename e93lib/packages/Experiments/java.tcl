package provide [file tail [info script]] 1

package require scratch.tcl
	# we put the javac and the java output in a scratch windows
package require globals.tcl
	# We need filetypes to add our file extensions to
package require make.tcl
	# We need filetypes to add our file extensions to

set JAVA_COMPILER javac
set JAVA java
set E93_CLASSPATH ""
set JAVA_RUN_ARGS ""
set JAVA_OUTPUT_DIR "."

proc getClassPath {} \
{
	if {[file exists classpath.tcl]} \
	{
		source classpath.tcl
	}
	
	return $classpath
}

proc loadJavaProj {} \
{	
	if {[file exists javaProj.tcl]} \
	{
		source javaProj.tcl
	}
}

proc GetPackageName {theBuffer} \
{
	set packageName ""
	setmark $theBuffer temp;				# remember what was selected, we will not disturb it
	setselectionends $theBuffer 0 0;		# move to the top of the buffer to perform the search
	TextToBuffer tempFindBuffer {package [A-Z|a-z|\.]+;}; # load up the expression
	if {[catch {find $theBuffer tempFindBuffer -regex} message]==0} \
	{
		set theText [lindex [selectedtextlist $theBuffer] 0]
		if {[regexp {(package )([A-Z|a-z|\.]+)} $theText whole package packageName]}\
		{
			set packageName $packageName.
		}
	}
	gotomark $theBuffer temp;				# put back the user's selection
	clearmark $theBuffer temp
	return $packageName
}

proc GetImportList {theBuffer} \
{
	set importList ""
	setmark $theBuffer temp;				# remember what was selected, we will not disturb it
	setselectionends $theBuffer 0 0;		# move to the top of the buffer to perform the search
	TextToBuffer tempFindBuffer {import [A-Z|a-z|\.]+(\.\*)?;}; # load up the expression
	while {[catch {eval [list find $theBuffer tempFindBuffer] -regex} message]==0} \
	{
		if {[llength $message]} \
		{
			set theText [lindex [selectedtextlist $theBuffer] 0]
			#ignore the last part of the import, it is either a class name or a *
			if {[regexp {(import )([A-Z|a-z|\.]+)\.(\w+|\*)} $theText whole import importName allClasses]}\
			{
				lappend importList $importName.
			}
			set newStart [lindex [getselectionends $theBuffer] 1]
			setselectionends $theBuffer $newStart $newStart
		} else {break}
	} 
	gotomark $theBuffer temp;				# put back the user's selection
	clearmark $theBuffer temp
	return $importList
}

proc OpenJavaClassName {buffer} \
{
	set className [selectedtextlist $buffer]
	set importList [GetImportList $buffer]
	set packageName [GetPackageName $buffer]
	#search this package as well
	lappend importList $packageName
	foreach classPackagePath $importList \
	{
		if { [catch {OpenJavaClass $classPackagePath$className 0}] == 0}\
		{
			break
		}
	}
}

proc getRunArgs {} \
{
	set runArgs ""
	if {[file exists runargs.tcl]} \
	{
		source runargs.tcl
	}

	return $runArgs
}

proc getFQClassName {locationString} \
{
	regsub -all {[\:\(\)]} $locationString { } classPackagePath
	set path [lindex $classPackagePath 0]
	set lastDot [string last "." $path]
	# set methodName [string range $path [expr $lastDot + 1] end]
	# set javaFileName [lindex $classPackagePath 1]
	return [string range $path 0 [expr $lastDot - 1]]
}

proc getLineNumber {locationString} \
{
	regsub -all {[\:\(\)]} $locationString { } classPackagePath
	return [lindex $classPackagePath 2]
}

proc FindJavaSource {locationString} \
{
	OpenJavaClass [getFQClassName $locationString] [getLineNumber $locationString]
}

proc OpenJavaClass {className lineNumber} \
{
	global E93_CLASSPATH
	set found 0
	set E93_CLASSPATH [getClassPath]
	loadJavaProj
	regsub -all {\.} $className {/} classPackagePath
	set classList [split $E93_CLASSPATH {;}]
	foreach classDir $classList \
	{
		if {[string index $classDir end] != {/} }\
		{
			set classDir "$classDir/"
		}
		if {[file exists "$classDir$classPackagePath.java"]} \
		{
			set fileToOpen "$classDir$classPackagePath.java"
			set found 1
			break
		}
	}
	if { $found } \
	{
		SmartOpenList "$fileToOpen:$lineNumber"
	}\
	else \
	{
		error "$className:$lineNumber not found"
	}
}

# send the file associated with the active window to javac
proc JavaCompile {target} \
{
	global lastMakeWindow E93_CLASSPATH JAVA_OUTPUT_DIR JAVA_COMPILER theData
	# I'd like to assure the case of the file is correct here, how?
	# set realFileName [fixCase $target]
	if {[file exists $target] && [file extension $target] == ".java"} \
	{
		set curDir [pwd]
		cd [file dirname $target]
		set E93_CLASSPATH [getClassPath]
		loadJavaProj
		set javaFile [file tail $target]
		set theData "$JAVA_COMPILER -g -deprecation  -d $JAVA_OUTPUT_DIR -verbose $javaFile"
		# set theData "$JAVA_COMPILER -g -deprecation  -d $JAVA_OUTPUT_DIR -classpath \"$E93_CLASSPATH\" -verbose $javaFile"
		# set theData "jikes +E -g -deprecation -classpath $E93_CLASSPATH -verbose $javaFile"
		set theWindow [NewScratchWindow "make $javaFile.mk"]
		set lastMakeWindow $theWindow
		task $theWindow $theData
		cd $curDir
	}
}

proc RunJava {{schemeName "e93"} {mapName ""}} \
{
	global E93_CLASSPATH JAVA_RUN_ARGS JAVA
	set E93_CLASSPATH [getClassPath]
	loadJavaProj
	Run "$JAVA -classpath \"$E93_CLASSPATH\" $JAVA_RUN_ARGS" $schemeName $mapName;
}

proc ListMembers theBuffer \
{
	# get the name of the class preceding this period
	return {this that the}
}

proc DoJavaPopup theBuffer \
{
	if {[string equal [file extension $theBuffer] .java]} \
	{
		# get members
		set members [ListMembers $theBuffer]
		InsertAndHome $theBuffer .[windowsdopopupmenu $members]
	} \
	else \
	{
		InsertAndHome $theBuffer ".";	# insert a period
	}
}

# bindkey .		{x0000000000} {DoJavaPopup [ActiveWindowOrBeep]};		# when period is hit, check

addmenu {} LASTCHILD 1 "Java" "" ""
	addmenu {Java} LASTCHILD 1 "Java Compile"			{\\Kj}					{JavaCompile [ActiveWindowOrBeep]}
	addmenu {Java} LASTCHILD 1 "Java Run"				{\\Kr}					{RunJava}
	addmenu {Java} LASTCHILD 1 "Java Open Class"		{}						{OpenJavaClassName [ActiveWindowOrBeep]}

# we want e93 to include .java as file extensions it shows in the open panel
append filetypes "\t{{Source Files}			{.java}						TEXT}\n" 
append filetypes "\t{{Java Source Files}	{.java}						TEXT}\n" 

# you only need the java package if you want to create java objects in the e93 process
# if you just want to write and compile java code comment out the next line
# nothing above actually requires it, that's why I've put it at the end
# package require java
