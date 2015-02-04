# Test getrect and setrect
set window [NewWindow]
set count 0
set orgRect [getrect $window]
while {$count < 5} \
{
	incr count
	set theRect [getrect $window]
	setrect $window [lindex $theRect 0] [lindex $theRect 1] [lindex $theRect 2] [lindex $theRect 3]
	if {![string equal $theRect $orgRect]} \
	{
		okdialog failed
	}
}
closewindow $window



