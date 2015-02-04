package provide [file tail [info script]] 1

# Add support for ClearCase version control software

# insert this before the Help menu
addmenu {Help} PREVIOUSSIBLING 1 "ClearCase" "" ""
	addmenu {ClearCase} LASTCHILD 1 "Check Out"						{}			{okdialog [CheckOut [ActiveWindowOrBeep]]}
	addmenu {ClearCase} LASTCHILD 1 "Check Out Unreserved"			{}			{okdialog [CheckOut [ActiveWindowOrBeep] -unres]}
	addmenu {ClearCase} LASTCHILD 1 "Check In"						{}			{okdialog [CheckIn [ActiveWindowOrBeep]]}
	addmenu {ClearCase} LASTCHILD 1 "Undo Check Out"				{}			{okdialog [UndoCO [ActiveWindowOrBeep]]}
	addmenu {ClearCase} LASTCHILD 1 "Add to Source Control..."		{}			{okdialog [AddToSourceControl [ActiveWindowOrBeep]]}
	addmenu {ClearCase} LASTCHILD 1 "Remove from Source Control..."	{}			{okdialog [RemoveFromSourceControl [ActiveWindowOrBeep]]}
	addmenu {ClearCase} LASTCHILD 1 "Space1"						{\S}		{}
	addmenu {ClearCase} LASTCHILD 1 "Clear Case Man"				{}			{ClearCaseMan [selectedtextlist [ActiveWindowOrBeep]]}
	addmenu {ClearCase} LASTCHILD 1 "Clear Case Help"				{}			{windowswinhelp D:/CCase/bin/cc_user.hlp [selectedtextlist [ActiveWindowOrBeep]]}

#
# Checkout from ClearCase
proc CheckOut {fileName {reserved -res}} \
{
	# see if there is a modification time tag associated with this file, if so, check against current mtime and complain if they differ
	if {[catch {getbuffervariable $fileName mtime} oldmtime]==0} \
	{
		if {[catch {file mtime $fileName} newmtime]==0} \
		{
			if {$oldmtime!=$newmtime} \
			{
				okcanceldialog "WARNING!\n\nIt looks like another application modified:\n$fileName\nwhile it was open for editing.\n\nWould you like to save anyway?"
			}
		} \
		else \
		{
			okcanceldialog "WARNING!\n\nIt looks like another application deleted:\n$fileName\nwhile it was open for editing.\n\nWould you like to save anyway?"
		}
	}
	
	set message [exec cleartool checkout -nc $reserved $fileName]
	catch {setbuffervariable $fileName mtime [file mtime $fileName]};
	
	return $message
}

# Checkin to ClearCase
proc CheckIn {fileName} \
{
	set message [exec cleartool checkin -nc $fileName]
	catch {setbuffervariable $fileName mtime [file mtime $fileName]};
	set out $message
}

# undo CheckOut form ClearCase
proc UndoCO {fileName} \
{
	set message [exec cleartool unco -rm $fileName]
	catch {setbuffervariable $fileName mtime [file mtime $fileName]};
	set out $message
}

# Add to ClearCase
proc AddToSourceControl {fileName} \
{
	cd  [file dirname $fileName]
	# CheckOut the parent directory
	CheckOut .
	catch [exec cleartool mkelem $fileName] message
	CheckIn .
	set out $message
}

proc RemoveFromSourceControl {fileName} \
{
	cd  [file dirname $fileName]
	# CheckOut the parent directory
	CheckOut .
	catch [exec cleartool rmname $fileName] message
	CheckIn .
	set out $message
}

proc ClearCaseMan {command} \
{
	if {[string length $command] == 0}\
	{
		set command [textdialog "What ClearCase Command?"];
	}
	
	exec cleartool man $command
}
