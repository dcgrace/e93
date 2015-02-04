# Syntax coloring rules for Tcl
# March 19, 2000  TMS

# best of luck to anyone who tries to understand the list_string, string_list stuff in here :-)
addsyntaxmap "Tcl" \
{
	exp {e_keywords				"<(after|append|array|binary|break|case|catch|cd|clock|close|commands|concat|continue|else|eof|error|eval|exec|exit|expr|fblocked|fconfigure|fcopy|file|fileevent|flush|for|foreach|format|gets|glob|global|if|incr|info|info|interp|join|lappend|lindex|linsert|list|llength|load|lrange|lreplace|lsearch|lsort|namespace|open|package|pid|proc|puts|pwd|read|regexp|regsub|rename|return|scan|seek|set|socket|source|split|string|subst|switch|tell|time|trace|unknown|unset|update|uplevel|upvar|variable|vwait|while)>"}
	exp {e_comment_start		"(;|^)[ \t]*(#)"}
	exp {e_end_of_line			"$"}
	exp {e_string_delimiter		"\""}
	exp {e_quoted_char			"(\\\\)(\[-\])"}
	exp {e_list_start			"\\\{"}
	exp {e_list_end				"()\\\}"}
	exp {e_command_start		"\\\["}
	exp {e_command_end			"\\\]"}

	map {m_keywords				e_keywords				{}									$style_keyword		0				0}
	map {m_quoted_char			e_quoted_char:0			{e_quoted_char:1}					$style_delimiter	0				$style_char}
	map {m_comment				e_comment_start:1		{e_end_of_line}						$style_comment		$style_comment	$style_comment}
	map {m_string				e_string_delimiter		{e_string_delimiter}				$style_delimiter	$style_string	$style_delimiter}
	map {m_list_string			e_string_delimiter		{e_string_delimiter e_list_end:0}	$style_delimiter	$style_string	$style_delimiter}
	map {m_list					e_list_start			{e_list_end}						$style_default		$style_default	$style_default}
	map {m_string_list			e_list_start			{e_list_end e_string_delimiter}		$style_string		$style_string	$style_string}
	map {m_command				e_command_start			{e_command_end}						$style_default		$style_default	$style_default}

	at {m_string				{m_quoted_char m_command}}
	at {m_list_string			{m_quoted_char m_string_list m_command}}
	at {m_string_list			{m_quoted_char m_string_list m_list_string m_command}}
	at {m_list					{m_keywords m_comment m_quoted_char m_list_string m_command}}
	at {m_command				{m_keywords m_quoted_char m_comment m_string m_list m_command}}

	root {m_keywords m_quoted_char m_comment m_string m_list m_command}
}

set	extensionHuntExpression(Tcl)	{^((.*\.tcl)|(.*\.tk))$}
set	extensionTabSize(Tcl)			4
set	extensionColorScheme(Tcl)		"e93"
set	extensionMapName(Tcl)			"Tcl"
