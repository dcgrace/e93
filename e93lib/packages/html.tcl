package provide [file tail [info script]] 1

package require sgml.tcl

# we want e93 to include .html as file extensions it shows in the open panel
append filetypes "\t{{HTML Files}	{.html}						TEXT}\n" 
append filetypes "\t{{HTML Files}	{.htm}						TEXT}\n" 

package require http
::http::config

# Copy a URL to a file and print meta-data
proc ::http::copy { url file {chunk 4096} } \
{
    set out [open $file w]
    fconfigure $out -translation lf
    set token [geturl $url -channel $out -progress ::http::Progress -blocksize $chunk]
    close $out
    # This ends the line started by http::Progress
    puts stderr ""
    upvar #0 $token state
    set max 0
    foreach {name value} $state(meta) \
	{
		if {[string length $name] > $max} \
		{
		    set max [string length $name]
		}
		if {[regexp -nocase ^location$ $name]} \
		{
		    # Handle URL redirects
		    puts stderr "Location:$value"
		    return [copy [string trim $value] $file $chunk]
		}
    }
    incr max
    foreach {name value} $state(meta) \
    {
		puts [format "%-*s %s" $max $name: $value]
    }

    return $token
}

proc ::http::Progress {args} \
{
    puts -nonewline stderr . ; flush stderr
}

# Replace all selections with some given text.
proc ReplaceSelectionWith {theWindow replaceSelectionWith} \
{
	global replaceSelectionsChoice;

	TextToBuffer tempFindBuffer {([^]+)};							# load up the expression (find anything, mark as \0)
	TextToBuffer tempReplaceBuffer [set replaceSelectionsChoice $replaceSelectionWith];	# load up the replacement
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope;	# do the replacement
}

proc MakeList {theWindow} \
{
	breakundo $theWindow
	insert $theWindow "<UL>\n"
	foreach theItem [selectedtextlist [getclipboard]] \
	{
		insert $theWindow "<LI>$theItem</LI>\n"
	}
	insert $theWindow "</UL>\n"
	breakundo $theWindow
}

proc LowercaseTagsAndAttributes {theWindow} \
{
	set hs [lindex [GetDefaultStyleInfo $theWindow] end]
	setmark $theWindow temp;									# remember what was selected, we will not disturb it
	setselectionends $theWindow 0 0;							# move to the top of the buffer to perform the search
	SetHighlightMode $theWindow HTML
	styletoselection $theWindow 6
	styletoselection $theWindow 9
	LowercaseSelection $theWindow
	gotomark $theWindow temp;									# put back the user's selection
	clearmark $theWindow temp;									# get rid of temp selection
	SetHighlightMode $theWindow $hs 
}

proc TidyHTML {fileName} \
{
	Run "java -classpath D:/Projects/jtidy-04aug2000r7-dev/build/Tidy.jar org.w3c.tidy.Tidy $fileName"
}

addmenu {SGML} FIRSTCHILD 1 "HTML" "" ""
	addmenu {{SGML} {HTML}} LASTCHILD 1 "Make Paragraph"		{}					{ReplaceSelectionWith [ActiveWindowOrBeep] "<P>\\0</P>"}
	addmenu {{SGML} {HTML}} LASTCHILD 1 "Make Bold"				{}					{ReplaceSelectionWith [ActiveWindowOrBeep] "<B>\\0</B>"}
	addmenu {{SGML} {HTML}} LASTCHILD 1 "Make Code"				{}					{ReplaceSelectionWith [ActiveWindowOrBeep] "<CODE>\\0</CODE>"}
	addmenu {{SGML} {HTML}} LASTCHILD 1 "Make Var"				{}					{ReplaceSelectionWith [ActiveWindowOrBeep] "<VAR>\\0</VAR>"}
	addmenu {{SGML} {HTML}} LASTCHILD 1 "Make Blockquote"		{}					{ReplaceSelectionWith [ActiveWindowOrBeep] "<BLOCKQUOTE>\n\\0\n</BLOCKQUOTE>"}
	addmenu {{SGML} {HTML}} LASTCHILD 1 "Make Comment"			{}					{ReplaceSelectionWith [ActiveWindowOrBeep] "<!-- \\0 -->"}
	addmenu {{SGML} {HTML}} LASTCHILD 1 "Make Tag..."			{}					{set tag [textdialog "Entar a Tag Name" TAG]; ReplaceSelectionWith [ActiveWindowOrBeep] "<$tag>\n\\0\n</$tag>"}
	addmenu {{SGML} {HTML}} LASTCHILD 1 "Make List"				{}					{cut [ActiveWindowOrBeep]; MakeList [ActiveWindowOrBeep]}
	addmenu {{SGML} {HTML}}	LASTCHILD 1 "Space"					{\\S}				{}
	addmenu {{SGML} {HTML}} LASTCHILD 1 "Tidy"					{}					{TidyHTML [ActiveWindowOrBeep]}

