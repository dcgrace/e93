package provide [file tail [info script]] 1

proc GetAttributes {attr putIn} \
{
	upvar $putIn Attributes

	set w " \t\r\n"					;# white space
	set exp (\[^=]*)=\[$w]?\"?(\[^$w\"]*)\"?(.*)
	while {[regexp -nocase $exp $attr things attName attVal	attr]} \
	{
		set Attributes([string trim [string tolower $attName]]) [string trim $attVal]
	}
}

# get the contents found between tags in an sgml document
# even if the tags are inside a comment
proc GetElement {tag sgml putIn} \
{
	upvar $putIn Attributes

	regsub -all "$tag" $sgml "\b$tag" sgml
	set tag "\b$tag"
	set text ""
	set w " \t\r\n"					;# white space
	set exp <\[$w]?$tag\(\[^>]*)>(\[^\b]*)\[$w]?<\[$w]?/$tag\[$w]?>
	regexp -nocase $exp $sgml things attr text
	GetAttributes $attr Attributes
	return $text
}

# get the contents found between tags in an sgml document
# even if the tags are inside a comment
proc getTaggedText {tag html {attr {}}} \
{
	regsub -all "\\?" $attr "\\?" attr
	regsub -all "$tag" $html "\b$tag" html
	set tag "\b$tag"
	set text ""
	set w " \t\r\n"					;# white space
	set exp <$tag+$attr>\[$w]*(\[^\b]*)\[$w]*</$tag>
	regexp -nocase $exp $html tag text
	# strip whitespace
	string trim $text
}

# get all the contents found between tags in an sgml document
# even if the tags are inside a comment
# return a list
proc getAllTaggedText {tag html {attr {}}} \
{
	regsub -all "\\?" $attr "\\?" attr
	regsub -all "$tag" $html "\b$tag" html
	set tag "\b$tag"
	set text ""
	set w " \t\r\n"					;# white space
	set exp <$tag+$attr>\[$w]*(\[^\b]*)\[$w]*</$tag>
	set text [regexp -nocase -all -inline $exp $html]
	# strip whitespace
	set theList [string trim $text]
	set length [llength $theList]
	set count 1
	set matches ""
	while {$count < $length} \
	{
		lappend matches [lindex $theList $count]
		incr count 2
	}
	
	return $matches
}

