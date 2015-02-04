# given a file name, make a sitemap Object entry
proc makeWindowsSitmapEntry {theFile} \
{
	# load the source file
	set inChan [open $theFile]
	# get the page Title out of the HTML
	set name [getTaggedText "TITLE" [read $inChan]]
	close $inChan
	
	return "<LI> <OBJECT type=\"text/sitemap\">\n<param name=\"Name\" value=\"$name\">\n<param name=\"Local\" value=\"$theFile\">\n</OBJECT>"
}

# make a .hhc file that the Windows HTML Help Workshop can understand
proc makeWindowsTOC {TOCFileName} \
{
	global stubsDir
	
	# open a destination file
	set outfile [open $TOCFileName w+]
	fconfigure $outfile -translation lf

	puts $outfile "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\n<HTML>\n<HEAD>\n<!-- Sitemap 1.0 -->\n</HEAD><BODY>\n<OBJECT type=\"text/site properties\">\n	<param name=\"ImageType\" value=\"Folder\">\n</OBJECT>"

	puts $outfile "<UL>"
	
	set chapters "$stubsDir/about.html $stubsDir/starting.html $stubsDir/environment.html $stubsDir/windows_buffers.html $stubsDir/cursors.html $stubsDir/selections.html $stubsDir/marks.html $stubsDir/cutcopypaste.html $stubsDir/scripting.html $stubsDir/keyboardinput.html $stubsDir/menuskeybindings.html $stubsDir/findreplace.html $stubsDir/undoredo.html $stubsDir/keynames.html $stubsDir/colors.html $stubsDir/standardkeyboardlayout.html $stubsDir/crlf.html $stubsDir/regex.html $stubsDir/xlfd.html $stubsDir/syntaxmaps.html $stubsDir/commandconventions.html"
 
	foreach theFile $chapters \
	{
		puts $outfile [makeWindowsSitmapEntry $theFile]
	}
	
	# make the Command Index inside the Microsoft TOC
	puts $outfile "<LI> <OBJECT type=\"text/sitemap\">\n<param name=\"Name\" value=\"Command Reference\">\n</OBJECT>"

	puts $outfile "<UL>"
	set commands [lsort [glob -nocomplain $stubsDir/commands/*.html]]
	foreach theFile $commands \
	{
		puts $outfile [makeWindowsSitmapEntry $theFile]
	}
	puts $outfile "</UL>"

	set chapters " $stubsDir/startup.html $stubsDir/copyright.html"

	foreach theFile $chapters \
	{
		puts $outfile [makeWindowsSitmapEntry $theFile]
	}

	puts $outfile "</UL>\n</BODY></HTML>"

	close $outfile
}

# make a .hhk file that the Windows HTML Help Workshop can understand
proc makeWindowsIndex {IndexFileName} \
{
	global stubsDir
	
	# open a destination file
	set outfile [open $IndexFileName w+]
	fconfigure $outfile -translation lf

	puts $outfile "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\n<HTML>\n<HEAD>\n<!-- Sitemap 1.0 -->\n</HEAD><BODY>"
	
		puts $outfile <UL>
			set files [glob -nocomplain $stubsDir/commands/*.html]
			append files " [glob -nocomplain $stubsDir/*.html]"
			set files [lsort -dictionary $files]
			foreach theFile $files \
			{
				puts $outfile [makeWindowsSitmapEntry $theFile]
			}
		puts $outfile "</UL>"

	puts $outfile "</BODY></HTML>"

	close $outfile
}

# toc.hhc
# make the Microsoft HTML Help table of contents, which contains the Command Index as an element
makeWindowsTOC toc.hhc

# e93.hhk
# make the Microsoft HTML Help index
makeWindowsIndex e93.hhk

# don't need to muck with the project file
