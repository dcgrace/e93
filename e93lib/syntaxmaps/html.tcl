# Syntax coloring rules for HTML
# March 22, 2000  TMS

addsyntaxmap "HTML" \
{
	exp {e_tag_start			"\\<"}
	exp {e_tag_end				"()\\>"}
	exp {e_word					"<\[!/0-9a-zA-Z\]+>"}
	exp {e_equals				"="}
	exp {e_variable				"<(\[^ =>\\t\\n\]+)\[ \\t\]*="}
	exp {e_value				"=\[ \\t\]*(\[^ >\\t\\n\]+)"}
	exp {e_comment_start		"\\<!--"}
	exp {e_comment_end			"--\\>"}
	exp {e_string_delimiter		"\""}
	exp {e_alt_string_delimiter	"'"}
	exp {e_ascii				"(&)\[#0-9a-z\]+(;)"}

	map {m_tag					e_tag_start				{e_tag_end}							$style_delimiter	0				$style_delimiter}
	map {m_keyword				e_word					{e_tag_end:0}						$style_keyword		0				0}
	map {m_boolean_variable		e_word					{}									$style_variable		0				0}
	map {m_variable				e_variable:0			{e_tag_end:0}						$style_variable		0				0}
	map {m_equals				e_equals				{e_tag_end:0}						$style_operator		0				0}
	map {m_value				e_value:0				{}									$style_value		0				0}
	map {m_comment				e_comment_start			{e_comment_end}						$style_comment		$style_comment	$style_comment}
	map {m_string				e_string_delimiter		{e_string_delimiter}				$style_delimiter	$style_string	$style_delimiter}
	map {m_alt_string			e_alt_string_delimiter	{e_alt_string_delimiter}			$style_delimiter	$style_string	$style_delimiter}
	map {m_ascii				e_ascii:0				{e_ascii:1}							$style_delimiter	$style_value	$style_delimiter}

	at {m_tag					{m_keyword}}
	at {m_keyword				{m_string m_alt_string m_variable m_boolean_variable}}
	at {m_variable				{m_equals m_variable m_boolean_variable}}
	at {m_equals				{m_string m_alt_string m_value m_variable m_boolean_variable}}

	root {m_comment m_tag m_ascii}
}

set	extensionHuntExpression(HTML)	{^((.*\.html)|(.*\.htm)|(.*\.HTM)|(.*\.shtml)|(.*\.css)|(.*\[*]))$}
set	extensionTabSize(HTML)			4
set	extensionColorScheme(HTML)		"e93"
set	extensionMapName(HTML)			"HTML"

