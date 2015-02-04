#!/bin/sh
# the next line restarts using tclsh\
exec tclsh "$0" "$@"

# good enough pile of Tcl used to assemble the e93 documents "stubs" into HTML documentation
# This all works, but still needs some work

set auto_path "scripts $auto_path"
auto_mkindex scripts

set stubsDir docsource/html/

proc wrap {srcFile templateFile prev next destfile} \
{
	puts "wrapping $srcFile"
	# load the template file
	set templateChan [open $templateFile]
	set template [read $templateChan]
	close $templateChan

	# load the source file
	set inChan [open $srcFile]
	set body [read $inChan]
	close $inChan

	# set the Tcl vars our template needs
	set date [clock format [clock seconds] -format "%B %Y"]

	# get the page Title out of the HTML
	set name [getTaggedText "TITLE" $body]
	regsub -all "&" $body "%amp%" body
	regsub -all "%name%" $template $name template
	regsub -all "%date%" $template [clock format [clock seconds] -format "%B %Y"] template
	regsub -all "%prev%" $template $prev template
	regsub -all "%next%" $template $next template
	regsub -all "%body%" $template $body template
	regsub -all "%amp%" $template {\&} template

	# open a destination file
	set outfile [open $destfile w+]
	fconfigure $outfile -translation lf
	puts $outfile $template
	close $outfile
}

proc makeCommandIndex {} {
	global stubsDir

	# open a destination file
	set outfile [open html/commands.html w+]
	fconfigure $outfile -translation lf

	puts $outfile "<!-- THIS FILE IS GENERATED: DON'T EDIT -->"
	puts $outfile "<TITLE>\nCommand Reference\n</TITLE>"
	puts $outfile "<LINK REL=\"STYLESHEET\" HREF=\"cmdref.css\" CHARSET=\"ISO-8859-1\" TYPE=\"text/css\">"
	puts $outfile "<CENTER>\n<H1>\nCommand Reference\n</H1>\n</CENTER>"
	puts $outfile "<CENTER>"
	puts $outfile "<P>"
	puts $outfile "<!-- <OBJECT type=\"text/summary\">\nA dictionary of e93 commands\n</OBJECT> -->"

	set letter 65
	while {$letter<90} \
	{
		set theChar [format "%c" $letter]
		incr letter
		puts -nonewline $outfile "<A HREF=\"#$theChar\">$theChar</A> <CODE>|</CODE>"
	}
	puts $outfile "<A HREF=\"#Z\">Z</A>"
	puts $outfile "</P>"
	puts $outfile "<HR WIDTH=\"100%\">"

	set letter 65
	while {$letter<91} \
	{
		set theChar [format "%c" $letter]
		puts $outfile "<A NAME=\"$theChar\"></A>"
		puts $outfile "<!-- Table Start -->\n<TABLE WIDTH=\"90%\" BORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">"
		incr letter
		# UNIX will need both cases globed, for now keep the source files lowercase
		set commands [lsort [glob -nocomplain $stubsDir/commands/$theChar*.html]]

		if {[llength $commands]>0} \
		{
			foreach theFile $commands \
			{
				# load the source file
				set inChan [open $theFile]
				set body [read $inChan]
				close $inChan

				# get the page Title out of the HTML
				set name [getTaggedText "TITLE" $body]
				# set summary [getTaggedText "SUMMARY" $body]
				set summary [getTaggedText "OBJECT" $body " type=\"text/summary\""]

				puts $outfile "\t<TR ALIGN=\"LEFT\"><TD ALIGN=\"LEFT\" WIDTH=\"40%\"><A HREF=\"commands/[file tail $theFile]\"><CODE>$name</CODE></A></TD><TD WIDTH=\"60%\">$summary</TD></TR>"
			}
		}
		puts $outfile "</TABLE>\n<!-- Table End -->\n"
	}

	puts $outfile "</CENTER>"
	close $outfile
}

proc wrapCommands {} \
{
	global stubsDir destDir

	# all commands in alphabetical order
	set commands [lsort [glob -nocomplain $stubsDir/commands/*.html]]
	# they all point back to the command index
	set root {../commands.html}
	set count 1
	set prev $root
	foreach theFile $commands \
	{
		set next [file tail [lindex $commands $count]]
		if {$next == ""} \
		{
			set next $root
		}

		regsub -all $stubsDir/ $theFile $destDir/ outFileName
		wrap $theFile templates/commands.html $prev $next $outFileName

		set prev [file tail $theFile]
		incr count
	}
}

proc wrapChapters {} \
{
	global stubsDir destDir chapters

	# they all point back to the main index
	set root index.html
	set count 1
	set prev $root
	foreach theFile $chapters \
	{
		set next [file tail [lindex $chapters $count]]
		if {$next == ""} \
		{
			set next $root
		}
		regsub -all "$stubsDir/" $theFile $destDir/ outFileName
		wrap $theFile templates/sections.html $prev $next $outFileName
		set prev [file tail $theFile]
		incr count
	}
}

proc makeHTMLTOCEntry {theFile} \
{
	# load the source file
	set inChan [open $theFile]
	# get the page Title out of the HTML
	set body [read $inChan]
	set name [getTaggedText "TITLE" $body]
	# set summary [getTaggedText "SUMMARY" $body]
	set summary [getTaggedText "OBJECT" $body " type=\"text/summary\""]
	set theFile [file tail $theFile]
	close $inChan

	return "<TR>\n<TD WIDTH=300>\n<H3>\n<A HREF=\"$theFile\">$name</A>\n</H3>\n</TD>\n<TD>\n$summary\n</TD>\n</TR>"
}

proc makeHTMLTOC {TOCFileName} \
{
	global stubsDir chapters

	# open a destination file
	set outfile [open $TOCFileName w+]
	fconfigure $outfile -translation lf

	foreach theFile $chapters \
	{
		puts $outfile [makeHTMLTOCEntry $theFile]
	}

	close $outfile
}

# relative path from this file to the directory that contains the .html stub files
set stubsDir "html"

# relative path from this file to the directory to generate the final html into
set destDir "../docs"

file delete -force [pwd]$destDir

# make the dest dir if its not already there
file mkdir $destDir/commands

# convert the .xml commands to .html
source scripts/xml2html.tcl

# wrap the command stubs in their navigation header and footer and link them together
puts "wrapping commands"
wrapCommands

# make the "Command Index" chapter
puts "making Command Index"
makeCommandIndex

# the main chapters
# set chapters [lsort -dictionary [glob $stubsDir/*.html]]
set chapters "html/about.html html/starting.html html/environment.html html/windows_buffers.html html/cursors.html html/selections.html html/marks.html html/cutcopypaste.html html/scripting.html html/keyboardinput.html html/menuskeybindings.html html/findreplace.html html/undoredo.html html/keynames.html html/colors.html html/standardkeyboardlayout.html html/crlf.html html/regex.html html/xlfd.html html/syntaxmaps.html html/commandconventions.html html/commands.html html/startup.html html/copyright.html"

# wrap the main chapter stubs in their navigation header and footer and link them together
puts "wrapping chapters"
wrapChapters

# make the TOC, which lists the main chapters
puts "making Table Of Contents"
makeHTMLTOC $destDir/index.html
# wrap it in the header and footer found in templates/index.html
wrap $destDir/index.html templates/index.html {} {} $destDir/index.html

foreach extension ".css .jpg .gif" \
{
	foreach srcFile [glob -nocomplain $stubsDir/*$extension] \
	{
		puts "copying $srcFile"
		set srcFile [file tail $srcFile]
		file copy -force $stubsDir/$srcFile $destDir/
	}

	foreach srcFile [glob -nocomplain $stubsDir/commands/*$extension] \
	{
		puts "copying $srcFile"
		set srcFile [file tail $srcFile]
		file copy -force $stubsDir/commands/$srcFile $destDir/commands/
	}
}

# make a Windows html help project for our files
source scripts/makeWindowsHelpProject.tcl

exit
