# Java rules courtesy of Michel Dalal
addsyntaxmap "JAVA" \
{
	exp {e_types				"<(void|boolean|char|byte|short|int|long|float|double)>"}
	exp {e_keywords				"<(strictfp|abstract|interface|break|native|case|new|cast|null|catch|operator|outer|class|package|const|private|finally|implements|instanceof|continue|protected|default|public|do|rest|return|else|extends|switch|synchronized|static|byvalue|cast|volatile|transien|const|future|generic|goto|inner|operator|outer|rest|var|import|final|virtual|throws|false|true|while|try|throw|for|if|this|super)>"}
	exp {e_comment_start		"/\\*"}
	exp {e_comment_end			"\\*/"}
	exp {e_alt_comment_start	"//"}
	exp {e_end_of_line			"$"}
	exp {e_string_delimiter		"\""}
	exp {e_quoted_char			"(\\\\)(\[-\])"}
	exp {e_char_delimiter		"'"}
	exp {e_function_name		"(\[a-zA-Z_\]\[a-zA-Z0-9_\]*)\[ \\t\]*\\("}

	map {m_keywords				e_keywords				{}									$style_keyword		0				0}
	map {m_types				e_types					{}									$style_type			0				0}
	map {m_quoted_char			e_quoted_char:0			{e_quoted_char:1}					$style_delimiter	0				$style_char}
	map {m_comment				e_comment_start			e_comment_end						$style_comment		$style_comment	$style_comment}
	map {m_alt_comment			e_alt_comment_start		e_end_of_line						$style_comment		$style_comment	$style_comment}
	map {m_string				e_string_delimiter		e_string_delimiter					$style_delimiter	$style_string	$style_delimiter}
	map {m_char					e_char_delimiter		e_char_delimiter					$style_delimiter	$style_char		$style_delimiter}
	map {m_function_name		e_function_name:0		{}									$style_function		0				0}

	at {m_string				{m_quoted_char}}
	at {m_char					{m_quoted_char}}

	root {m_quoted_char m_keywords m_types m_comment m_alt_comment m_string m_char m_function_name}
}

set	extensionHuntExpression(JAVA)	{^((.*\.java))$}
set	extensionTabSize(JAVA)			4
set	extensionColorScheme(JAVA)		"JBuilder"
set	extensionMapName(JAVA)			"JAVA"

