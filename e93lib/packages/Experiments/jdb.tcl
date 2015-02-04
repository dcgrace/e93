package provide [file tail [info script]] 1


package require java
	# we must have java/tcl blend to create our debugger bean, etc...
package require java.tcl
	# we need the java rc file for many procs
package require scratch.tcl
	# we put the debugger output in a scratch window

# NOTE: you'll need the DebugBean class for all this to work

set javaDebugger {}
set currentThread {}
set debugWindow {}

# define the fully qualified class name of the debug bean here, if you've changed its java package
#
# set debugBeanClassName e93.tools.debug.DebugBean
set debugBeanClassName meretrx.debug.DebugBean

unbindkey F5			{x0000000000}
unbindkey F5			{x0010000000}


set debugWindowName "jdb debug console"

	addmenu {Java} LASTCHILD 0 "space1"						{\S}					{}
	addmenu {Java} LASTCHILD 1 "Go"							{\\KF5}					{RunJavaDebug}
	addmenu {Java} LASTCHILD 1 "Restart"					{\Kx1110000000F5}		{okdialog "Restart not yet implemented"}
	addmenu {Java} LASTCHILD 1 "Stop Debugging"				{\Kx1010000000F5}		{okdialog "Stop not yet implemented"}
	addmenu {Java} LASTCHILD 1 "Break"						{\KCancel}				{okdialog "Break not yet implemented"}
	addmenu {Java} LASTCHILD 0 "space2"						{\S}					{}
	addmenu {Java} LASTCHILD 1 "Step Into"					{\\KF11}				{global currentThread; $currentThread step true}
	addmenu {Java} LASTCHILD 1 "Step Over"					{\\KF10}				{global currentThread; $currentThread next}
	addmenu {Java} LASTCHILD 1 "Step Out"					{\Kx1010000000F11}		{global currentThread; $currentThread stepOut}
	addmenu {Java} LASTCHILD 1 "Run To Cursor"				{\Kx0110000000F10}		{okdialog "Run To Cursor not yet implemented"}
	addmenu {Java} LASTCHILD 0 "space3"						{\S}					{}
	addmenu {Java} LASTCHILD 1 "Stack"						{\\KF7}					{ShowStack}
	addmenu {Java} LASTCHILD 1 "Local Variables"			{\\KF4}					{ShowLocalVariables}
	addmenu {Java} LASTCHILD 1 "Quick View"					{\Kx1010000000F9}		{ShowSelectedVariable [ActiveWindowOrBeep]}
	addmenu {Java} LASTCHILD 1 "Continue"					{}						{global currentThread; $currentThread cont}
	addmenu {Java} LASTCHILD 0 "space4"						{\S}					{}
	addmenu {Java} LASTCHILD 1 "Set Breakpoint"				{\\KF9}					{SetJDBBreakpoint [ActiveWindowOrBeep]}

set breakpointArray("") {}
unset breakpointArray("")
# update the breakpoint menu anytime the breakpoint array is changed
trace variable breakpointArray	wu BuildBreakpointMenu

proc BuildBreakpointMenu {name element op} \
{
	global breakpointArray

	addmenu {Java} LASTCHILD 1 "Breakpoints"				{}					{}
	addmenu {Java} LASTCHILD 1 "Clear Breakpoint"			{}					{}
    foreach bpName [lsort [array names breakpointArray]] \
    {
		addmenu {"Java" "Breakpoints"}		LASTCHILD 1		"$bpName"			{}			"GoToBreakpoint \{$bpName\}"
		addmenu {"Java" "Clear Breakpoint"} LASTCHILD 1		"$bpName"			{}			"ClearBreakpoint \{$bpName\}"
    }
}

proc StartJdb {}\
{
	global javaDebugger debugBeanClassName debugWindowName debugWindow breakpointArray
	set javaDebugger {}
	if {[string length $javaDebugger] == 0} \
	{
		set debugWindow [NewScratchWindow $debugWindowName 1]
		set buggerBean [java::new $debugBeanClassName]
		set classpath [getClassPath]
		set javaDebugger [java::new sun.tools.debug.RemoteDebugger "-classpath $classpath -DVerbose=5 -DGUI=false " $buggerBean false]
		java::bind $buggerBean print {global debugWindow; regsub -all {\r} [java::event message] {} output; insert $debugWindow $output}
		java::bind $buggerBean breakpoint {JavaBreakPointEvent [java::event thread]}
		java::lock $buggerBean
	    foreach bpName [array names breakpointArray] \
	    {
	    	set locationString $breakpointArray($bpName)
			SetBreakpoint $javaDebugger [getFQClassName $locationString] [getLineNumber $locationString]
	    }
	}
	return $javaDebugger
}

proc RunJavaDebug {} \
{
	set javaDebugger [StartJdb]
	set runArgs [getRunArgs]
	set argList [split $runArgs { }]
	set numArgs [llength $argList]
	set theArgs [java::new {String[]} $numArgs $argList]
	set threadGroup [$javaDebugger run $numArgs $theArgs]
}

proc ShowStack {}\
{
	global currentThread
	set newList {}
	set stack [$currentThread dumpStack]
	set numFrames [$stack length]
	for {set frameNum 0} {$frameNum<$numFrames} {incr frameNum}\
	{
		set frame [$stack get $frameNum]
		set rClass [$frame getRemoteClass]
		lappend newList "[$rClass getName].[$frame getMethodName]:[$frame getLineNumber]"
	}
	set userSelect [listdialog "Stack:" $newList]
	set newIndexNum [lsearch -exact $newList $userSelect]
	$currentThread setCurrentFrameIndex $newIndexNum
	ShowFileForFrame [$stack get $newIndexNum]
}

proc JavaBreakPointEvent {curThread} \
{
	global currentThread;
	set currentThread $curThread
	set stackFrame [$currentThread getCurrentFrame]
	ShowFileForFrame $stackFrame
}

proc ShowFileForFrame {stackFrame} \
{
	set found 0
	set fileToOpen ""
	set remoteClass [$stackFrame getRemoteClass]
	set javaFile [$remoteClass getSourceFileName]
	set lineNumber [$stackFrame getLineNumber]
	if {[file exists $javaFile] == 0} \
	{
		set className [$remoteClass getName]
		OpenJavaClass $className $lineNumber
	}\
	else \
	{
		SmartOpenList "$javaFile:$lineNumber"
	}
}

proc GoToBreakpoint {bpName} \
{
	global breakpointArray
	FindJavaSource $breakpointArray($bpName)
}

proc ClearBreakpoint {bpName} \
{
	global breakpointArray
	GoToBreakpoint $bpName
	unset "breakpointArray($bpName)"
	set theBuffer [ActiveWindowOrBeep]
	set lineNumber [lindex [positiontolineoffset $theBuffer [lindex [getselectionends $theBuffer] 0] ] 0]
	# we could quit here, since breakpoints are re-set from the list each time jdb is started

	set javaFile [file tail $theBuffer]
	set javaClass [file rootname $javaFile]
	set packageName [GetPackageName $theBuffer]
	set javaDebugger [StartJdb]
	set rClass [$javaDebugger findClass $packageName$javaClass]
	if ([java::isnull $rClass]) \
	{
		okdialog "Break point not Cleared for $packageName.$javaClass because the class wasn't found"
	} \
	else \
	{
		set errMess [$rClass clearBreakpointLine $lineNumber ]
		if {[string length $errMess] > 0}\
		{
			okdialog "$packageName$javaClass $errMess"
		} \
	}
}

proc SetJDBBreakpoint {theBuffer} \
{
	global breakpointArray
	SelectLineWhenNoSelection $theBuffer
	set javaFile [file tail $theBuffer]
	set javaClass [file rootname $javaFile]
	set packageName [GetPackageName $theBuffer]
	set fullyQualifiedClassName "$packageName$javaClass"
	set methodName "crap"
	#TODO: get the real method name, at the moment it's not used though
	set lineNumber [lindex [positiontolineoffset $theBuffer [lindex [getselectionends $theBuffer] 0] ] 0]
	set bpName "$javaFile:$lineNumber"
	set breakpointArray($bpName) "$fullyQualifiedClassName.$methodName\($javaFile:$lineNumber\)"
}

proc SetBreakpoint {javaDebugger fullyQualifiedClassName lineNumber} \
{
	set rClass [$javaDebugger findClass $fullyQualifiedClassName]
	if ([java::isnull $rClass]) \
	{
		okdialog "Break point not set for $fullyQualifiedClassName because the class wasn't found"
	} \
	else \
	{
		set errMess [$rClass setBreakpointLine $lineNumber ]
		if {[string length $errMess] > 0}\
		{
			okdialog "$fullyQualifiedClassName $errMess"
		}
	}
}

proc ShowLocalVariables { } \
{
	global currentThread
	set rFrame [$currentThread getCurrentFrame]
	set rLocalVars [$rFrame getLocalVariables]
	set numLocalVars [$rLocalVars length]
	set newList ""
	for {set varNum 0} {$varNum<$numLocalVars} {incr varNum}\
	{
		set strValue ""
		set stackVar [$rLocalVars get $varNum]
		if {[$stackVar inScope] }\
		{
			set strValue [GetFieldValue [$stackVar getValue]]
		}\
		else\
		{
			set strValue "Not in scope"
		}
		lappend newList "[$stackVar getName]:$strValue"
	}
	set userSelect [listdialog "Local variables: [[$rFrame getRemoteClass] typeName].[$rFrame getMethodName]" $newList]
	set newIndexNum [lsearch -exact $newList $userSelect]
	# with the selection show an okdialog for simple vars otherwise call function to display object
	set selectedVar [$rLocalVars get $newIndexNum]
	ShowVariable [$selectedVar getName] [$selectedVar getValue]
}

proc GetFieldValue { rValue } \
{
	set strValue ""
	if {[java::isnull $rValue]}\
	{
		set strValue "null"
	}\
	else\
	{
		if {[$rValue isObject] && ![$rValue isString]}\
		{
			set strValue "[$rValue typeName]"
		}\
		else\
		{
			set strValue [$rValue toString]
		}
	}
	return $strValue
}

proc ShowVariable { varName rValue }\
{
	if {[$rValue isObject] && ![$rValue isString]}\
	{
		if {[string equal [$rValue typeName] "array"]}\
		{
			ShowArray $rValue
		}\
		else\
		{
			ShowObject $rValue
		}
	}\
	else \
	{
		ShowPrimitiveType $varName $rValue
	}
}

proc ShowArray { rValue }\
{
	set arrayVar [java::cast sun.tools.debug.RemoteArray $rValue]
	set rElements [$arrayVar getElements]
	set numElements [$rElements length]
	set newList ""
	for {set elementNum 0} {$elementNum<$numElements} {incr elementNum}\
	{
		set strValue ""
		set elementVar [$rElements get $elementNum]
		set strValue [GetFieldValue $elementVar]
		lappend newList "$elementNum:$strValue"
	}
	set userSelect [listdialog "Array:" $newList]
	set newIndexNum [lsearch -exact $newList $userSelect]
	# with the selection show an okdialog for simple vars otherwise call function to display object
	set selectedVar [$rElements get $newIndexNum]
	ShowVariable $elementNum $selectedVar
}

proc ShowPrimitiveType { varName rValue }\
{
	okdialog "$varName:[$rValue toString]"
}

proc ShowObject { rValue }\
{
	set newList ""
	set selectedObject ""
	set objectVar [java::cast sun.tools.debug.RemoteObject $rValue]
	set classVars [GetObjectVariables $objectVar]
	set numInstVars [llength $classVars]
	# now the static variables
	set staticVars [GetObjectVariables [$objectVar getClazz]]
	set newList [concat $classVars $staticVars]
	set userSelect [listdialog "Class variables: [[$objectVar getClazz] typeName]" $newList]
	set newIndexNum [lsearch -exact $newList $userSelect]
	# with the selection show an okdialog for simple vars otherwise call function to display object
	# if the index is less than the number of instance variables it is an instance variable
	# otherwise it is a class varaible
	if { $newIndexNum < $numInstVars}\
	{
		set selectedObject $objectVar
	}\
	else\
	{
		set selectedObject [$objectVar getClazz]
		set newIndexNum [expr $newIndexNum - $numInstVars]
	}
	set selectedFields [$selectedObject getFields]
	set selectedVarName [[$selectedFields get $newIndexNum] getName]
	set selectedVar [$selectedObject getFieldValue $selectedVarName]
	ShowVariable $selectedVarName $selectedVar
}

proc GetObjectVariables { objectVar } \
{
	set varList ""
	set objFields [$objectVar getFields]
	set numFields [$objFields length]
	for {set fieldNum 0} {$fieldNum<$numFields} {incr fieldNum}\
	{
		set strValue ""
		set objField [$objFields get $fieldNum]
		set objFieldValue [$objectVar getFieldValue [$objField getName]]
		set strValue [GetFieldValue $objFieldValue]
		lappend varList "[$objField getName]:$strValue"
	}
	return $varList
}

proc ShowSelectedVariable { buffer }\
{
	global currentThread;
	set var ""
	set errMess ""
	set stackFrame [$currentThread getCurrentFrame]
	set varName [selectedtextlist $buffer]
	if {[catch {$stackFrame getLocalVariable $varName} staticVar] || [java::isnull $staticVar]}\
	{
		set thisClass [$stackFrame getRemoteClass]
		if {[catch {$thisClass getFieldValue $varName} var]}\
		{
			set thisVar [$stackFrame getLocalVariable this]
			if {![java::isnull $thisVar]}\
			{
				set thisObj [java::cast sun.tools.debug.RemoteObject [$thisVar getValue]]
				if {[catch {$thisObj getFieldValue $varName} var] || [java::isnull $staticVar]}\
				{
					set errMess "$varName not found"
				}
			}\
			else\
			{
				set errMess "$varName not found"
			}
		}
	}\
	else\
	{
		if { [$staticVar inScope] }\
		{
			 set var [$staticVar getValue]
		}\
		else\
		{
			set errMess "$varName is not in scope"
		}
	}
	if {[string length $errMess] == 0} \
	{
		ShowVariable $varName $var
	}\
	else\
	{
		okdialog $errMess
	}
}


