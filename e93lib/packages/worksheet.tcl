package provide [file tail [info script]] 1

set workSheetBufName "";
bindkey F12			{x0000000000} {OpenWorkSheet}

proc OpenWorkSheet {} \
{
	global env workSheetBufName tcl_rcFileName

	set			screenWidth				[lindex [screensize] 0]
	set			screenHeight			[lindex [screensize] 1]
	set			windowWidth				[expr $screenWidth*5/6];	# set the initial default width for new windows
	set			windowHeight			[expr $screenHeight*3/4];	# set the initial default height for new windows
	
	set theWorkSheetName [file dirname $tcl_rcFileName]/worksht.e93
	if {[file exists $theWorkSheetName]} \
	{
		OpenWindow [set workSheetBufName [openbuffer $theWorkSheetName]] 10 10 $windowWidth $windowHeight 4 "Worksheet" "Tcl"
		setbuffervariable $workSheetBufName worksheet "";
	} \
	else \
	{
		OpenWindow [set workSheetBufName [newbuffer $theWorkSheetName]] 10 10  $windowWidth $windowHeight 4 "Worksheet" "Tcl"
		setbuffervariable $workSheetBufName worksheet "";
		if {[catch {savebufferas $workSheetBufName $theWorkSheetName} errorMessage]!=0} \
		{
			okdialog "Failed to save:\n'$theWorkSheetName'\n$errorMessage"
			return -code error
		}
	}
}

proc SaveWorkSheet {} \
{
	global env workSheetBufName

	foreach theWindow [windowlist] \
	{
		if {[IsThisTheWorkSheetWindow $theWindow]} \
		{
			if {[catch {savebuffer $theWindow} errorMessage]!=0} \
			{
				okdialog "Failed to save:\n'$theWindow'\n$errorMessage"
			}
		}
	}
}

proc IsThisTheWorkSheetWindow {theWindow} \
{
	expr {[catch {getbuffervariable $theWindow worksheet} errorMessage]==0}
}


# Psudo OO with tcl. Extend AskClose
rename AskClose Worksheet_AskClose
proc AskClose {theBuffer} \
{
	# you are not allowed to close the worksheet
	if {![IsThisTheWorkSheetWindow $theBuffer]} \
		{
			Worksheet_AskClose $theBuffer
		}
}

# Psudo OO with tcl. Extend AskSaveAs
rename AskSaveAs Worksheet_AskSaveAs
proc AskSaveAs {theBuffer} \
{
	# you are not allowed to close the worksheet
	if {![IsThisTheWorkSheetWindow $theBuffer]} \
	{
		Worksheet_AskSaveAs $theBuffer
	}
}

# Psudo OO with tcl. Extend TryToQuit
rename TryToQuit Worksheet_TryToQuit
proc TryToQuit {} \
{
	SaveWorkSheet
	Worksheet_TryToQuit
}

# Make a style to make the worksheet look different from other windows
set hs_worksheet($style_default)		{black			lightyellow 		"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_comment)		{darkgreen	 	lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_string)			{midnightblue	lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_char)			{midnightblue	lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_digit)			{black			lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_operator)		{midnightblue	lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_delimiter)		{midnightblue	lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_keyword)		{blue			lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_type)			{blue			lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_directive)		{blue			lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet($style_function)		{maroon			lightyellow			"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set hs_worksheet(selection)				{lightyellow	black				"-*-IBM3270-medium-r-normal-sans-9-*-*-*-*-*"}
set "HighlightSchemes(Worksheet)"		hs_worksheet

# Add it to the menu
addmenu {Window "Color Scheme"} LASTCHILD 1 Worksheet	{}					"SetHighlightScheme \[ActiveWindowOrBeep\] \"Worksheet\""

OpenWorkSheet;     # open the worksheet
movecursor $workSheetBufName ENDDOC
HomeWindowToSelectionStart $workSheetBufName




