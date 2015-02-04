package provide [file tail [info script]] 1

# Set up the Tests menu
addmenu {} LASTCHILD 1 "Tests" "" ""
	addmenu {Tests} LASTCHILD 1 "Run Test"					{}					{RunTest 1}

proc RunTest {instances} \
{
	cd D:/projects/meretrx
	source classpath.tcl
	# set command "java -DVerbose=5 -DGUI=false -classpath $classpath meretrx.unittest.UnitTester Z:/Wip/mmanning/meretrx/";
	set classpath {./;}
	append classpath {D:/jdk1.1.7/lib/classes.zip;}

	# UBS Utilities
	append classpath {Z:/UnitTestFW/src/;}
	append classpath {Z:/Util/src/;}

	# IBM XML Stuff
	append classpath {Z:/Wip/mmanning/lib/xml4j.jar;}
	append classpath {Z:/Wip/mmanning/lib/xerces.jar;}
	
	set command "java -DVerbose=5 -DGUI=false -classpath $classpath com.ubs.cube.util.unittest.UnitTester Z:/Wip/mmanning/meretrx/";
	set theWindow "Test Output"
	set theWindow [NewScratchWindow $theWindow];
	task $theWindow $command
}

proc RunRemoteTests {clientList instances} \
{
	set startingAt 0	
	foreach host $clientList \
	{
		for {set x 0} {$x < $instances} {incr x} \
		{
			Run "rsh $host perl C:/Projects/RunTest.pl $startingAt";
			incr startingAt 1000
		}
	}
}

# These are order dependent as some rely on others, can tcl packages be used?
set TestFiles \
	{ \
	Z:/PerfTest/script/CreateNaturalPersonAsClient/run.tcl \
	Z:/PerfTest/script/GetNaturalPerson/run.tcl \
	Z:/PerfTest/script/Identifier/run.tcl \
	}

foreach file $TestFiles \
{
	if {[file exists $file]} \
	{
		if {[catch {source $file} msg]} \
		{
			okdialog "Error parsing source file \"$file\":\n$msg"
		}
	}
}
