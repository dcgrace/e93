package provide [file tail [info script]] 1

# This package redefines some of the e93 GUI functionality to Tk

# we need Tk to do the work in here, it should now be part of e93
package require Tk

# we need globals.tcl for the filetypes global var
package require globals.tcl

proc opendialog {dialogTitle {initialfile {}} } \
{
	global filetypes
	set initialDir [file dirname $initialfile]
	set initialfile [file tail $initialfile]
#	set filename [tk_getOpenFile -title $dialogTitle -multiple 1 -initialdir $initialDir -initialfile $initialfile -filetypes $filetypes]
	set filename [tk_getOpenFile -title $dialogTitle -initialdir $initialDir -initialfile $initialfile -filetypes $filetypes]
	if {$filename != ""} \
	{
		return $filename
	} \
	else \
	{
		error {}
	}
}

proc savedialog {dialogTitle {initialfile {}} } \
{
	set initialDir [file dirname $initialfile]
	set initialfile [file tail $initialfile]
	set filename [tk_getSaveFile -title $dialogTitle -initialdir $initialDir -initialfile [file tail $initialfile]]
	if {$filename != ""} \
	{
		return $filename
	} \
	else \
	{
		error {}
	}
}

proc okdialogTk {dialogText} \
{
	tk_messageBox -title e93 -message $dialogText -icon warning 
}

proc okcanceldialog {dialogText} \
{
	if {[tk_messageBox -title e93 -type okcancel -message $dialogText -icon question] == "cancel"} \
	{
		error {} 
	}
}

proc yesnodialog {dialogText} \
{
	switch [tk_messageBox -title e93 -type yesnocancel -message $dialogText  -icon question]  \
	{
		yes
		{
			return 1
		}
		no
		{
			return 0
		}
		cancel
		{
			error {}
		}
	}
}

# dialogs that have no TK equivalent
# pathdialog
# fontdialog
# textdialog
# searchdialog
# listdialog

# TK dialogs that could be used, but aren't
# tk_chooseColor

