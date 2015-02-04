package provide [file tail [info script]] 1

package require sgml.tcl

# we want e93 to include .xml as file extensions it shows in the open panel
append filetypes "\t{{XML Files}	{.xml}						TEXT}\n"  

# Replace all selections with some given text.
proc ReplaceSelectionWith {theWindow replaceSelectionWith} \
{
	global replaceSelectionsChoice;

	TextToBuffer tempFindBuffer {([^]+)};							# load up the expression (find anything, mark as \0)
	TextToBuffer tempReplaceBuffer [set replaceSelectionsChoice $replaceSelectionWith];	# load up the replacement
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope;	# do the replacement
}


addmenu {SGML} FIRSTCHILD 1 "XML" "" ""
	addmenu {SGML XML} LASTCHILD 1 "Make Comment"			{}					{ReplaceSelectionWith [ActiveWindowOrBeep] "<!-- \\0 -->"}
	addmenu {SGML XML} LASTCHILD 1 "Make Tag..."				{}					{set tag [textdialog "Entar a Tag Name" TAG]; ReplaceSelectionWith [ActiveWindowOrBeep] "<$tag>\\0</$tag>"}



















