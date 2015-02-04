# syntax map for VHDL contributed by Andrew Pines
addsyntaxmap "VHDL" \
{
	exp {e_types				"<(buffer|in|inout|out)>"}
	exp {e_keywords				"<(abs|access|after|alias|all|and|architecture|array|assert|attribute|begin|block|body|bus|case|component|disconnect|downto|else|elsif|end|entity|exit|file|for|function|generate|generic|group|guarded|if|impure|inertial|is|label|library|linkage|literal|loop|map|mod|nand|new|next|nor|not|null|of|on|open|or|others|package|port|postponed|procedure|process|pure|range|record|register|reject|rem|report|return|rol|ror|select|severity|signal|shared|sla|sll|sra|srl|subtype|then|to|transport|type|unaffected|units|until|use|variable|wait|when|while|with|xnor|xor)>"}
	exp {e_comment_start		"--"}
	exp {e_end_of_line			"$"}
	exp {e_string_delimiter		"\""}
	exp {e_char_delimited		"(').(')"}
	exp {e_char					"."}

	map {m_types				e_types					{}									$style_type			0				0}
	map {m_keywords				e_keywords				{}									$style_keyword		0				0}
	map {m_comment				e_comment_start			{e_end_of_line}						$style_comment		$style_comment	$style_comment}
	map {m_string				e_string_delimiter		{e_string_delimiter}				$style_delimiter	$style_string	$style_delimiter}
	map {m_char_delimited		e_char_delimited:0		{e_char_delimited:1}				$style_delimiter	$style_char		$style_delimiter}
	map {m_char					e_char					{}									$style_char			$style_char		$style_char}

	root {m_types m_keywords m_comment m_string m_char_delimited}
}

set	extensionHuntExpression(VHDL)	{^((.*\.vhdl)|(.*\.vhd)|(.*\.vh)|(.*\.vhh))$}
set	extensionTabSize(VHDL)			4
set	extensionColorScheme(VHDL)		"e93"
set	extensionMapName(VHDL)			"VHDL"
