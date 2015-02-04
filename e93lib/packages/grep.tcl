# Contributed by David Grace

package provide [file tail [info script]] 1

package require greputils.tcl
package require scratch.tcl

# create globals for grep so I don't have to type in my search pattern over each time
set grepOptions(recursive)		0
set grepOptions(ignoreCase)		0
set grepOptions(globPattern)	"*.*"
set grepOptions(dir)			[pwd]
set grepOptions(pattern)		""

# look-and-feel options
option add *background						#c0c0c0
option add *activeBackground				#c0c0c0
option add *selectColor						#ffffff
option add *takeFocus						0
option add *padX							0
option add *padY							0
option add *highlightThickness				0
option add *Entry.background				#ffffff
option add *Entry.takeFocus					1
option add *Checkbutton.highlightThickness	1
option add *Checkbutton.takeFocus			1


proc GrepToBuffer {dir globPatterns pattern recursive ignoreCase theBuffer} \
{
	global package_dir
	set command "source $package_dir/greputils.tcl;"
	set dir [file attributes $dir -shortname] 
	append command [format {puts [eval [Grep %s %s %s %d %d]]} $dir $globPatterns $pattern $ignoreCase $recursive]
	task $theBuffer $command
}

proc CenterTkWindow {w} \
{
	update
	set sWidth		[winfo screenwidth $w]
	set sHeight		[winfo screenheight $w]
	set wWidth		[winfo reqwidth $w]
	set wHeight		[winfo reqheight $w]
	set x			[expr $sWidth/2 - $wWidth/2]
	set y			[expr $sHeight/2 - $wHeight/2]
	wm geometry $w [format "%dx%d+%d+%d" $wWidth $wHeight $x $y]
}

proc FindGrep {} \
{
	global grepOptions
	global grepResult

	# If there's an active window with text selected which does not span more than one
	# line, make that text the grep pattern. Else use whatever was the last grep pattern
	set text $grepOptions(pattern)
	if {![catch {activewindow} theWindow]} {
		set ends		[getselectionends $theWindow]
		set start		[lindex $ends 0]
		set end			[lindex $ends 1]
		if {$start!=$end} {
			# some text is selected. If it doesn't span more than one line, stick it in the grep buffer
			set selectedText [join [selectedtextlist $theWindow]]
			if {[regexp "\n" $selectedText match] == 0} {
				set grepOptions(pattern) $selectedText
			}
		}
	}
	# create dialog for grep information
	set w .wGrep
	set grepResult 0
	toplevel $w
	wm title		$w	"Find in Files"
	wm transient	$w	.
	wm resizable	$w	0 0
	pack [set f [frame $w.fButton]] -side right -anchor n -padx 8 -pady 8
	pack [button $f.bOk -text "Find" -command "set ::grepResult 1; destroy $w" -borderwidth 3] -side top -fill x -pady 4
	# make return key invoke the "Ok" button
	bind $w <Key-Return> "$f.bOk invoke"
	pack [button $f.bCancel -text "Cancel" -command "destroy $w"] -side top -fill x -pady 4
	pack [set f [frame $w.fExp]] -side top -fill x -pady 2
	pack [label $f.l -text "Find What:"] -side left
	pack [entry $f.e -textvariable ::grepOptions(pattern) -bg white] -side right
	pack [set f [frame $w.fPattern]] -side top -fill x -pady 2
	pack [label $f.l -text "In files/types:"] -side left
	pack [entry $f.e -textvariable ::grepOptions(globPattern) -bg white] -side right
	pack [set f [frame $w.fDir]] -side top -fill x -pady 2
	pack [label $f.l -text "In folder:"] -side left
	pack [entry $f.e -textvariable ::grepOptions(dir) -bg white] -side right
	pack [set f [frame $w.fOptions]] -side top -anchor w -pady 2
	pack [checkbutton $f.cbIgnoreCase -text "Ignore Case" -variable ::grepOptions(ignoreCase)] -side top -anchor w
	pack [checkbutton $f.cbRecursive -text "Search Subfolders" -variable ::grepOptions(recursive)] -side top -anchor w
	CenterTkWindow $w
	focus -force $w
	focus $w.fExp.e
	grab $w
	tkwait window $w
	if {$grepResult && ($grepOptions(globPattern) != "")} \
	{
		# user pressed "Ok" button

		set recurs ""
		if {$grepOptions(recursive)} \
		{
			set recurs "recursively"
		}
		set ignoreCase ""
		if {$grepOptions(ignoreCase)} \
		{
			set ignoreCase "ignoring case "
		}
		set theWindow [NewScratchWindow "Search for text '$grepOptions(pattern)' $recurs $ignoreCase in directory $grepOptions(dir) in all files matching $grepOptions(globPattern)"];														# make a buffer to hold the result
		GrepToBuffer $grepOptions(dir) $grepOptions(globPattern) $grepOptions(pattern) $grepOptions(recursive) $grepOptions(ignoreCase) $theWindow;	# perform the grep, insert results into buffer
		# movecursor $theWindow STARTDOC;							# move to the top of the result buffer
	}
}

catch {deletemenu {"Find" "Grep -n For..."}}
	addmenu {Find}		LASTCHILD	1	"Grep For..."				{\\KF}	FindGrep


