package provide [file tail [info script]] 1

package require sgml.tcl

set XERCES_HOME "D:/Java/xerces-2.0.2"

set XSLT_JARS ""
append XSLT_JARS "$XERCES_HOME/dom.jar"
append XSLT_JARS "$XERCES_HOME/jaxp-api.jar"
append XSLT_JARS "$XERCES_HOME/samples"
append XSLT_JARS "$XERCES_HOME/sax.jar"
append XSLT_JARS "$XERCES_HOME/xalan.jar"
append XSLT_JARS "$XERCES_HOME/xercesImpl.jar"
append XSLT_JARS "$XERCES_HOME/xsltc.jar"

proc Selection2File {theBuffer fileName} \
{
	newbuffer contents
	copy $theBuffer contents
	setselectionends contents 0 0;
	insert contents "<!DOCTYPE xsl:stylesheet \[<!ENTITY nbsp \"&#160;\">\]><TempRootWrapperForCleaning xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">";
	selectall contents				
	set position [lindex [getselectionends contents] 1];
	setselectionends contents $position $position;					
	insert contents "</TempRootWrapperForCleaning>";							
	savebufferas contents $fileName
	closebuffer $fileName
}

proc TransformFile {xslFile xmlFile} \
{
	global XSLT_JARS

	set  command  "java -classpath \"$XSLT_JARS\" org.apache.xalan.xslt.Process -Q -xsl $xslFile -in $xmlFile"
	set theWindow [NewScratchWindow "XSLTOutput.xml" 0 "Deep Blue" XML]
	task $theWindow $command	
}

proc TransformSelection {theWindow xslFile} \
{
	global XSLT_JARS

	set inFile [file join [pwd] temp_input.xml]
	Selection2File $theWindow $inFile

	set command "java -classpath \"$XSLT_JARS\" org.apache.xalan.xslt.Process -Q -xsl $xslFile -in $inFile"

	TextToBuffer tempFindBuffer {[^]+};		
	TextToBuffer tempReplaceBuffer "catch \{exec $command\} message; set message";	# load up the replacement, return results of command
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope -replacescript;			# do the replacement
	file delete -force $inFile
}

proc TransformSelectionSaxon {theWindow xslFile} \
{
	global XSLT_JARS

	set inFile [file join [pwd] temp_input.xml]
	Selection2File $theWindow $inFile

	set command "D:/Tools/Saxon/saxon.exe $inFile $xslFile"

	TextToBuffer tempFindBuffer {[^]+};		
	TextToBuffer tempReplaceBuffer "catch \{exec $command\} message; set message";	# load up the replacement, return results of command
	ReplaceAllBeep $theWindow tempFindBuffer tempReplaceBuffer -regex -limitscope -replacescript;			# do the replacement
	file delete -force $inFile
}

proc TransformSelectionNew {theWindow xslFile} \
{
	global XSLT_JARS

	set inFile [file join [pwd] temp_input.xml]
	Selection2File $theWindow $inFile

	set command "java -classpath \"$XSLT_JARS\" org.apache.xalan.xslt.Process -Q -xsl $xslFile -in $inFile"

	set theWindow [NewScratchWindow "XSLT Output.xml" 0 "Deep Blue" XML]
	task $theWindow $command	
}

set xsl_dir [file join [file dirname $tcl_rcFileName]/xsl]


addmenu {SGML} FIRSTCHILD 1 "XSLT" "" ""
	addmenu {SGML XSLT} LASTCHILD 1 "Replace Selection with transformation..."		{}		{TransformSelection [ActiveWindowOrBeep] [opendialog "Choose an XSL file:"]}
	addmenu {SGML XSLT} LASTCHILD 1 "Transform Selection..."						{}		{TransformSelectionNew [ActiveWindowOrBeep] [opendialog "Choose an XSL file:"]}
	addmenu {SGML XSLT} LASTCHILD 1 "Transform File..."								{}		{TransformFile [opendialog "Choose an XSL file:"] [opendialog "Choose an XML file:"]}
	addmenu {SGML XSLT} LASTCHILD 0 "space0"										{\\S}	{}
	addmenu {SGML XSLT} LASTCHILD 1 "Clean XML"										{}		{TransformSelection [ActiveWindowOrBeep] $xsl_dir/Clean.xsl}
	addmenu {SGML XSLT} LASTCHILD 1 "Clean XML Saxon"								{}		{TransformSelectionSaxon [ActiveWindowOrBeep] $xsl_dir/Clean.xsl}
	addmenu {SGML XSLT} LASTCHILD 1 "Make pseudo XPath"								{}		{TransformSelection [ActiveWindowOrBeep] $xsl_dir/PseudoXPath.xsl}

