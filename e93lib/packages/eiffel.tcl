package provide [file tail [info script]] 1

# ----------------------------------------------------------------------------------------
# Given an "filename" from a list, try to interpret it as the output from
# Eiffel:
# line 18 column 3 file ide_xml_property_manager.e
# If it looks like that, then extract the filename and line number
# and return them in a list, otherwise return an error
proc SmartOpenListSmallEiffelException {listItem} \
{
	if {[regexp {^line\ ([0-9]+).+file\ (.+)} $listItem whole tempLine tempFile]} \
	{
		if {[file exists $tempFile]} \
		{
			lappend temp $tempFile $tempLine;	# make a list of 2 elements (filename and line number)
			lappend temp2 $temp;				# return list of lists
			return $temp2;
		}
	}
	return -code error;
}
set	smartOpenProcs(SmartOpenListSmallEiffelException)	{SmartOpenListSmallEiffelException}



# ----------------------------------------------------------------------------------------
# Given an "filename" from a list, try to interpret it as the output from
# Eiffel:
# Line 131 column 25 in MEXP_TREE_HANDLER (C:\se\src\eiffel4xml/mexp_handlers/mexp_tree/mexp_tree_handler.e) :
# If it looks like that, then extract the filename and line number
# and return them in a list, otherwise return an error
proc SmartOpenListSmallEiffelCompilationError {listItem} \
{
	if {[regexp {^Line\ ([0-9]+).+\ \((.+)\)\ :$} $listItem whole tempLine tempFile]} \
	{
		if {[file exists $tempFile]} \
		{
			lappend temp $tempFile $tempLine;	# make a list of 2 elements (filename and line number)
			lappend temp2 $temp;				# return list of lists
			return $temp2;
		}
	}
	return -code error;
}

set	smartOpenProcs(SmartOpenListSmallEiffelCompilationError)	{SmartOpenListSmallEiffelCompilationError}
