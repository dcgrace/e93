# Eiffel rules courtesy of Sven Ehrke
addsyntaxmap "EIFFEL" \
{
	exp {e_types				"<(void)>"}
	exp {e_keywords				"<(strictfp|deferred|inspect|when|end|rescue|class|indexing|creation|is|feature|inherit|do|once|Result|else|elseif|false|true|from|until|loop|if|then|Current|precursor|require|ensure|invariant|check|and|or|and then|or else|local|not)>"}
	exp {e_comment_start		"/\\*"}
	exp {e_comment_end			"\\*/"}
	exp {e_alt_comment_start	"--"}
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

set	extensionHuntExpression(EIFFEL)	{^((.*\.e))$}
set	extensionTabSize(EIFFEL)			4
set	extensionColorScheme(EIFFEL)		"Deep Blue"
set	extensionMapName(EIFFEL)			"EIFFEL"

