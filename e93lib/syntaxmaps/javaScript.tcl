# JavaScript rules courtesy of Marc Sprecher
addsyntaxmap "JAVASCRIPT" \
{
	exp {e_js_types					"<(void|boolean|char|byte|short|int|long|float|double)>"}
	exp {e_js_keywords				"<(function|var|strictfp|abstract|interface|break|native|case|new|cast|null|catch|operator|outer|class|package|const|private|finally|implements|instanceof|continue|protected|default|public|do|rest|return|else|extends|switch|synchronized|static|byvalue|cast|volatile|transien|const|future|generic|goto|inner|operator|outer|rest|var|import|final|virtual|throws|false|true|while|try|throw|for|if|this|super)>"}
	exp {e_js_comment_start			"/\\*"}
	exp {e_js_comment_end			"\\*/"}
	exp {e_js_alt_comment_start		"//"}
	exp {e_js_end_of_line			"$"}
	exp {e_js_string_delimiter		"\""}
	exp {e_js_quoted_char			"(\\\\)(\[-\])"}
	exp {e_js_char_delimiter		"'"}
	exp {e_js_function_name			"(\[a-zA-Z_\]\[a-zA-Z0-9_\]*)\[ \\t\]*\\("}

	map {m_js_keywords				e_js_keywords				{}									$style_keyword		0				0}
	map {m_js_types					e_js_types					{}									$style_type			0				0}
	map {m_js_quoted_char			e_js_quoted_char:0			e_js_quoted_char:1						$style_delimiter	0				$style_char}
	map {m_js_comment				e_js_comment_start			e_js_comment_end						$style_comment		$style_comment	$style_comment}
	map {m_js_alt_comment			e_js_alt_comment_start		e_js_end_of_line						$style_comment		$style_comment	$style_comment}
	map {m_js_string				e_js_string_delimiter		e_js_string_delimiter					$style_delimiter	$style_string	$style_delimiter}
	map {m_js_char					e_js_char_delimiter			e_js_char_delimiter					$style_delimiter	$style_char		$style_delimiter}
	map {m_js_function_name			e_js_function_name:0		{}									$style_function		0				0}

	at {m_js_string					{m_js_quoted_char}}
	at {m_js_char					{m_js_quoted_char}}

	root {m_js_quoted_char m_js_keywords m_js_types m_js_comment m_js_alt_comment m_js_string m_js_char m_js_function_name}
}

set	extensionHuntExpression(JAVASCRIPT)	{^((.*\.js)|(.*\.htc))$}
set	extensionTabSize(JAVASCRIPT)			4
set	extensionColorScheme(JAVASCRIPT)		"JBuilder"
set	extensionMapName(JAVASCRIPT)			"JAVASCRIPT"


