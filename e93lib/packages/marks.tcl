package provide [file tail [info script]] 1

# goto marker theMarkNum
proc GotoMarkN {theMarkNum} \
{
	global markerArray

	if {[catch {gotomark $markerArray($theMarkNum) $theMarkNum}]==0} \
		{
		HomeWindowToSelectionStart $markerArray($theMarkNum)
		settopwindow $markerArray($theMarkNum)
		} \
	else \
		{
		beep
		}
}

set markerArray(1)	0
set markerArray(2)	0
set markerArray(3)	0
set markerArray(4)	0
set markerArray(5)	0
set markerArray(6)	0
set markerArray(7)	0
set markerArray(8)	0
set markerArray(9)	0
set markerArray(10)	0

# set marker theMarkNum for the theWindow
proc SetMarkN {theWindow theMarkNum} \
{
	global markerArray
	set markerArray($theMarkNum) $theWindow
	setmark $theWindow $theMarkNum
}

unbindkey F1		{x0010000000}
unbindkey F1		{x0000000000}
unbindkey F2		{x0010000000}
unbindkey F2		{x0000000000}
unbindkey F3		{x0010000000}
unbindkey F3		{x0000000000}
unbindkey F4		{x0010000000}
unbindkey F4		{x0000000000}

bindkey F1			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 1}
bindkey F2			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 2}
bindkey F3			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 3}
bindkey F4			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 4}
bindkey F5			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 5}
bindkey F6			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 6}
bindkey F7			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 7}
bindkey F8			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 8}
bindkey F9			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 9}
bindkey F10			{x1000000000} {SetMarkN [ActiveWindowOrBeep] 10}

bindkey F1			{x0000000000} {GotoMarkN 1}
bindkey F2			{x0000000000} {GotoMarkN 2}
bindkey F3			{x0000000000} {GotoMarkN 3}
bindkey F4			{x0000000000} {GotoMarkN 4}
bindkey F5			{x0000000000} {GotoMarkN 5}
bindkey F6			{x0000000000} {GotoMarkN 6}
bindkey F7			{x0000000000} {GotoMarkN 7}
bindkey F8			{x0000000000} {GotoMarkN 8}
bindkey F9			{x0000000000} {GotoMarkN 9}
bindkey F10			{x0000000000} {GotoMarkN 10}
