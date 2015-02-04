package provide [file tail [info script]] 1

# we need globals.tcl for the filetypes global var
package require globals.tcl

# we want e93 to include .tcl as file extensions it shows in the open panel
append filetypes "\t{{Source Files}			{.tcl}						TEXT}\n" 
append filetypes "\t{{TCL Scripts}			{.tcl}						TEXT}\n" 

# we want e93 to know what to do when it encounters a .tcl file extension
set	extensionColorSchemes(Tcl)		"Deep Blue"

#
# there  may be a prettier way of building this block
#
regsub -all " " [info procs] "|" knownProcs
set theMap "\n\texp \{e_procs				\"<($knownProcs)>\"\}"

# commands shouldn't be changing very often ("load" may add more?)
# This contains all known procs too, so put it after [info procs]
regsub -all " . " [info commands] "|" knownCommands
regsub -all " " $knownCommands "|" knownCommands
append  theMap "\n\texp \{e_keywords		\"<($knownCommands)>\"\}"

# This contains all known vars
regsub -all " . " [info vars] "|" knownVars
regsub -all " " $knownVars "|" knownVars
append  theMap "\n\texp \{e_vars			\"<($knownVars)>\"\}"

append  theMap {
	exp {e_comment_start		"#"}
	exp {e_end_of_line			"$"}
	exp {e_string_delimiter		"\""}
	exp {e_quoted_char			"(\\\\)(\[-\])"}
	exp {e_list_start			"\\\{"}
	exp {e_list_end				"()\\\}"}
	exp {e_command_start		"\\\["}
	exp {e_command_end			"\\\]"}

	map {m_procs				e_procs					{}									$style_function		0				0}
	map {m_vars					e_vars					{}									$style_variable		0				0}
	map {m_keywords				e_keywords				{}									$style_keyword		0				0}
	map {m_quoted_char			e_quoted_char:0			{e_quoted_char:1}					$style_delimiter	0				$style_char}
	map {m_comment				e_comment_start			{e_end_of_line}						$style_comment		$style_comment	$style_comment}
	map {m_string				e_string_delimiter		{e_string_delimiter}				$style_delimiter	$style_string	$style_delimiter}
	map {m_list_string			e_string_delimiter		{e_string_delimiter e_list_end:0}	$style_delimiter	$style_string	$style_delimiter}
	map {m_list					e_list_start			{e_list_end}						$style_default		$style_default	$style_default}
	map {m_string_list			e_list_start			{e_list_end e_string_delimiter}		$style_string		$style_string	$style_string}
	map {m_command				e_command_start			{e_command_end}						$style_default		$style_default	$style_default}

	at {m_string				{m_quoted_char m_command}}
	at {m_list_string			{m_quoted_char m_string_list m_command}}
	at {m_string_list			{m_quoted_char m_string_list m_list_string m_command}}
	at {m_list					{m_procs m_vars m_keywords m_comment m_quoted_char m_list_string m_command}}
	at {m_command				{m_procs m_vars m_keywords m_quoted_char m_comment m_string m_list m_command}}

	root {m_procs m_vars m_keywords m_quoted_char m_comment m_string m_list m_command}
}

addsyntaxmap "Tcl" $theMap

