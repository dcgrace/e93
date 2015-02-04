#!/bin/sh
# the next line restarts using tclsh\
exec tclsh "$0" "$@"

# This file takes the place of a proper XML parser and a XSL sytlesheet
# the input XML must contain TITLE and DESCRIPTION

# tagTable assosiates the tag name with the section name that will appear in the html
# TODO: put all the tag names into TclVars and replace all the literals in this file
set auto_path "scripts $auto_path"

# required
	set	tagTable(TITLE)				"TITLE"
	set	tagTable(DESCRIPTION)		"DESCRIPTION"

# the SYNOPSIS section is generated

#defaults
	# defaults to "Built-in" if not specified
	set	tagTable(TYPE)				"TYPE"

	# defaults to fist sentance of DESCRIPTION if not specified
	set	tagTable(SUMMARY)			"SUMMARY"

	# default to "None" if not specified
	set	tagTable(INPUT)				"INPUT"
	set	tagTable(OUTPUT)			"OUTPUT"

	# defaults to "TITLE always returns a status code of 0." if not specified
	set	tagTable(STATUS)			"STATUS"

	# 0-N
	set	tagTable(PARAMETERS)		"PARAMETER"
	set	tagTable(OPTION)			"OPTION"

	# 0-N PARAMETER tags (it should contain a PARAMDESCRIPTION for each one it includes)
	set	tagTable(PARAMDESCRIPTION)	"PARAMDESCRIPTION"

	# 0-N OPTION tags (it should contain an OPTDESCRIPTION for each one it includes)
	set	tagTable(OPTDESCRIPTION)	"OPTDESCRIPTION"

#optional
	# these sections will not appear unless explcitly included
	set	tagTable(EXAMPLES)			"EXAMPLES"
	set	tagTable(LIMITATIONS)		"LIMITATIONS"
	set	tagTable(SEEALSO)			"SEE ALSO"
	set	tagTable(LOCATION)			"LOCATION"

proc MakeSection {heading contents} \
{
	return "<H3>\n$heading\n</H3>\n<BLOCKQUOTE>\n<P>$contents</P>\n</BLOCKQUOTE>\n"
}

proc MakeVarDiscription {name contents} \
{
	return "<A name=\"$name\"></A><VAR>$name</VAR>\n<BLOCKQUOTE>\n<P>$contents</P>\n</BLOCKQUOTE>\n"
}

proc GetAllElements {tag sgml putIn} \
{
	upvar $putIn resultArray
	set arraryNames ""
	set count 0
	regsub -all "$tag" $sgml "\b$tag" sgml
	set tag "\b$tag"
	set text ""
	set w " \t\r\n"					;# white space
	set exp <\[$w]?$tag\(\[^>]*)>(\[^\b]*)\[$w]?<\[$w]?/$tag\[$w]?>(.*)
	while {[regexp -nocase $exp $sgml things attr text sgml]} \
	{
		GetAttributes $attr Attributes
		lappend arraryNames $count
		set resultArray($count,body) [string trim $text]
		foreach key [array names Attributes] \
		{
			set resultArray($count,$key) $Attributes($key)
		}
		catch "unset Attributes"
		incr count
	}

	return $arraryNames
}

proc RemoveElement {tag sgml} \
{
	regsub -all "$tag" $sgml "\b$tag" sgml
	set tag "\b$tag"
	set text ""
	set w " \t\r\n"					;# white space
	set exp <\[$w]?$tag\(\[^\b]*)\[$w]?<\[$w]?/$tag\[$w]?>
	regsub "$exp" $sgml "" sgml
	regsub -all "\b" $sgml "" sgml
	return $sgml
}

set CLASSPATH "D:/lib/XML/Apache/xerces.jar;"
append CLASSPATH "D:/lib/XML/Apache/xalan.jar;"
catch {append CLASSPATH "$env(CLASSPATH);"}


proc transformCommandTcl {srcFile destfile} \
{
	global tagTable

	# load the source file
	set inChan [open $srcFile]
	set body [read $inChan]
	close $inChan

	set body [GetElement "COMMAND" $body Attributes]
	set title [GetElement "NAME" $body Attributes]
	set body [RemoveElement NAME $body]

	set type "Built-in"
	catch {set type $Attributes(type)}

	set summary [getTaggedText "SUMMARY" $body]
	if {$summary == ""} \
	{
		puts "# WARNING # no summary element, using first sentence of 'DESCRIPTION' from file:"
		puts [file join [pwd] $srcFile]
		set summary [getTaggedText "DESCRIPTION" $body]
		# if there is no summary, use the first sentence of the discription
		set exp \[^\.<]*
		regexp -nocase $exp $summary summary
	}

	# open a destination file
	set outfile [open $destfile w+]
	fconfigure $outfile -translation lf

	puts $outfile "<TITLE>$title</TITLE>"
	puts $outfile "<LINK REL=\"STYLESHEET\" HREF=\"../cmdref.css\" CHARSET=\"ISO-8859-1\" TYPE=\"text/css\">"
	puts $outfile "<!-- <OBJECT type=\"text/summary\">\n$summary\n</OBJECT> -->"
	puts $outfile "<TABLE WIDTH=\"100%\" BORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\">\n<TR>\n<TD WIDTH=\"60%\" ALIGN=\"LEFT\"><H1>$title</H1></TD>\n<TD WIDTH=\"40%\" ALIGN=\"RIGHT\"><H1>$type</H1></TD>\n</TR>\n</TABLE>"

	# manditory sections

	# build SYNOPSIS section
		set section SYNOPSIS
		set contents "<CODE>$title"

		set parameterKeys [GetAllElements "PARAMETER" $body parameterArray]
		foreach key $parameterKeys \
		{
			set param $parameterArray($key,body)
			set name [string trim [GetElement "NAME" $param Attributes]]
			set parameterArray($key,body) [RemoveElement NAME $parameterArray($key,body)]
			set parameterArray($key,name) $name

			set isOptional 0
			if {![catch {set optionalFlag $parameterArray($key,optional)} optionalFlag]} \
			{
				set isOptional [string equal $optionalFlag "true"]
			}

			set name [string trim $name]
			if {$isOptional} \
			{
				append contents " <A href=\"#$name\"><VAR>?$name?</VAR></A>"
			} \
			else \
			{
				append contents " <A href=\"#$name\"><VAR>$name</VAR></A>"
			}
	    }

		# TODO: Options may have arguments
		set optionKeys [GetAllElements "OPTION" $body optionArray]
		foreach key $optionKeys \
		{
			set param $optionArray($key,body)
			set name [string trim [GetElement "NAME" $param Attributes]]
			set optionArray($key,name) $name
			set optionArray($key,body) [RemoveElement NAME $optionArray($key,body)]
			set optionArray($key,name) $name

			set isOptional 0
			if {![catch {set optionalFlag $optionArray($key,optional)} optionalFlag]} \
			{
				set isOptional [string equal $optionalFlag "true"]
			}

			set name [string trim $name]
			if {$isOptional} \
			{
				append contents " <A href=\"#$name\"><VAR>?$name?</VAR></A>"
			} \
			else \
			{
				append contents " <A href=\"#$name\"><VAR>$name</VAR></A>"
			}
	    }
	    append contents "</CODE>"
		puts $outfile [MakeSection "$section" $contents]
	# end SYNOPSIS

	# get manditory sections, use default if they are not part of the XML
	# INPUT (no input section for now)
	foreach section {DESCRIPTION OUTPUT} \
	{
		set contents [getTaggedText "$section" $body]
		if {$contents == ""} \
		{
			set contents "None"
		}

		puts $outfile [MakeSection "$section" $contents]
	}

	# build PARAMETERS section
	set section PARAMETERS
	set contents ""
	if {[llength $parameterKeys]==0} \
	{
		set contents "None"
	}

	foreach key $parameterKeys \
	{
		regsub -all "<CHOICE" $parameterArray($key,body) "\b" sgml
		set exp (\[^\b]*)
		regexp -nocase $exp $sgml discription
		set choices "<P>[string trim $discription]</P>\n"

		set choiceKeys [GetAllElements "CHOICE" $parameterArray($key,body) choiceArray]
		# okdialog "$parameterArray($key,body)"
		if [expr [llength $choiceKeys] > 0] \
		{
			if [expr [llength $choiceKeys] == 2] \
			{
				append choices "<P>This may be either one of the following values:</P>\n"
			} \
			else \
			{
				append choices "<P>This may be any one of the following values:</P>\n"
			}

			foreach choiceKey $choiceKeys \
			{
				set param $choiceArray($choiceKey,body)
				set name [string trim [GetElement "NAME" $param Attributes]]
				append choices [MakeVarDiscription $name [RemoveElement NAME $choiceArray($choiceKey,body)]]
			}
			append contents [MakeVarDiscription $parameterArray($key,name) $choices]
		} \
		else \
		{
			append contents [MakeVarDiscription $parameterArray($key,name) $parameterArray($key,body)]
		}
	}
	puts $outfile [MakeSection "$section" $contents]

	# build OPTIONS section
	# TODO: Options may have arguments, none do yet
	set section OPTIONS
	set contents ""
	if {[llength $optionKeys]==0} \
	{
		set contents "None"
	}
	foreach key $optionKeys \
	{
		regsub -all "<CHOICE" $optionArray($key,body) "\b" sgml
		set exp (\[^\b]*)
		regexp -nocase $exp $sgml discription
		set choices "<P>[string trim $discription]</P>\n"

		set choiceKeys [GetAllElements "CHOICE" $optionArray($key,body) choiceArray]
		if [expr [llength $choiceKeys] > 0] \
		{
			if [expr [llength $choiceKeys] == 2] \
			{
				append choices "<P>This may be either one of the following values:</P>\n"
			} \
			else \
			{
				append choices "<P>This may be any one of the following values:</P>\n"
			}

			foreach key $choiceKeys \
			{
				set param $choiceArray($choiceKey,body)
				set name [string trim [GetElement "NAME" $param Attributes]]
				append choices [MakeVarDiscription $name [RemoveElement NAME $choiceArray($choiceKey,body)]]
			}
			append contents [MakeVarDiscription $optionArray($key,name) $choices]
		} \
		else \
		{
			append contents [MakeVarDiscription $optionArray($key,name) $optionArray($key,body)]
		}
	}
	puts $outfile [MakeSection "$section" $contents]

	# build STATUS section
	set section "STATUS"
	set contents [getTaggedText $section $body]
	if {$contents == ""} \
	{
		set contents "$title always returns a status code of 0."
	}
	puts $outfile [MakeSection "$section" $contents]

	# optional sections
	foreach section {EXAMPLES LIMITATIONS SEEALSO LOCATION} \
	{
		set contents [getTaggedText "$section" $body]
		if {$contents != ""} \
		{
			puts $outfile [MakeSection $tagTable($section) $contents]
		}
	}

	close $outfile
}

proc transformChapterTcl {srcFile destfile} \
{
	# load the source file
	set inChan [open $srcFile]
	set body [read $inChan]
	close $inChan

	set title [getTaggedText "TITLE" $body]

	set summary [getTaggedText "SUMMARY" $body]
	if {$summary == ""} \
	{
		set summary [getTaggedText "BODY" $body]
		set exp \[^\.<]*
		regexp -nocase $exp $summary summary
	}

	# open a destination file
	set outfile [open $destfile w+]
	fconfigure $outfile -translation lf

	puts $outfile "<TITLE>$title</TITLE>"
	puts $outfile "<LINK REL=\"STYLESHEET\" HREF=\"cmdref.css\" CHARSET=\"ISO-8859-1\" TYPE=\"text/css\">"
	puts $outfile "<!-- <OBJECT type=\"text/summary\">\n$summary\n</OBJECT> -->"
	puts $outfile "<CENTER>\n<H1>\n$title\n</H1>\n</CENTER>"
	# if we wanted sections we would do it here
	puts $outfile "<P>[getTaggedText "BODY" $body]</P>"

	close $outfile
}

proc transformXalan {srcFile destfile xslFile} \
{
	global CLASSPATH

	catch "exec java -classpath \"$CLASSPATH\" org.apache.xalan.xslt.Process -Q -in $srcFile -lxcin $xslFile -out $destfile" message
	# catch "exec java -classpath \"$CLASSPATH\" org.apache.xalan.xslt.Process -Q -in $srcFile -xsl $xslFile -out $destfile" message
	set message $message
}

proc transformCommandXalan {srcFile destfile} \
{
	transformXalan $srcFile $destfile xml/commands/commands.style
	# transformXalan $srcFile $destfile xml/commands/commands.xsl
}


proc transformChapterXalan {srcFile destfile} \
{
	transformXalan $srcFile $destfile xml/chapters.style
	# transformXalan $srcFile $destfile xml/chapters.xsl
}

proc transformSaxon {srcFile destfile} \
{
	exec D:/Tools/saxon/saxon.exe -a -o $destfile $srcFile
}

set xmlDir xml
set htmlDir html

# remove the old html dir
puts "removing old files"
file delete -force $htmlDir

# make the html dir
puts "making output directories"
file mkdir $htmlDir/commands

# pre-compile the xsl file for speed, if using Xalan
catch "exec java -classpath \"$CLASSPATH\" org.apache.xalan.xslt.Process -Q -xsl xml/commands/commands.xsl -lxcout xml/commands/commands.style" message
catch "exec java -classpath \"$CLASSPATH\" org.apache.xalan.xslt.Process -Q -xsl xml/chapters.xsl -lxcout xml/chapters.style" message

# make the commands
puts "generating command reference"
set sourceFiles [lsort [glob -nocomplain $xmlDir/commands/*.xml]]
foreach srcFile $sourceFiles \
{
	set destfile $htmlDir/commands/[file tail [file rootname $srcFile]].html
	puts "generating $destfile"
	transformCommandTcl $srcFile $destfile
	# transformCommandXalan  $srcFile $destfile
	# transformSaxon $srcFile $destfile
}

# make the chapters
puts "generating main chapters"
set sourceFiles [lsort [glob -nocomplain $xmlDir/*.xml]]
foreach srcFile $sourceFiles \
{
	puts "generating $destfile"
	set destfile $htmlDir/[file tail [file rootname $srcFile]].html
	transformChapterTcl $srcFile $destfile
	# transformChapterXalan  $srcFile $destfile
	# transformSaxon $srcFile $destfile
}

foreach extension ".css .jpg .gif" \
{
	foreach srcFile [glob -nocomplain $xmlDir/*$extension] \
	{
		puts "copying $srcFile"
		set srcFile [file tail $srcFile]
		file copy -force $xmlDir/$srcFile $htmlDir/
	}

	foreach srcFile [glob -nocomplain $xmlDir/commands/*$extension] \
	{
		puts "copying $srcFile"
		set srcFile [file tail $srcFile]
		file copy -force $xmlDir/commands/$srcFile $htmlDir/commands/
	}
}


