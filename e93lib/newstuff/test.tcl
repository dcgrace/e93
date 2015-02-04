# test code for Tk dialog boxes

proc newokdialog {text} \
{
	toplevel .dlg -class Dialog
	wm geometry .dlg +200+100
	message .dlg.msg -text $text
	pack .dlg.msg -side left
	button .dlg.ok -text "Ok" -command {NewWindow}
	button .dlg.cancel -text "Cancel" -command {}
	pack .dlg.cancel -side right
	pack .dlg.ok -side right
}

newokdialog "This is a dialog box"


