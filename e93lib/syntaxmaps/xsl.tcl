# Syntax coloring rules for XSL
# Supports highlighting imbedded JavaScript
# Shows HTML tags differently than XML tags

addsyntaxmap "XSL" \
{
	exp {e_html_start				"\\</?"}
	exp {e_xslt_element_start		"\\</?xsl"}
	exp {e_keywords					"<(PCDATA)>"}
	exp {e_tag_end					"()/?\\>"}
	exp {e_word						"<\[!0-9a-zA-Z\\-\]+>"}
	exp {e_equals					"="}
	exp {e_variable					"<(\[^ =>\\t\\n\]+)\[ \\t\]*="}
	exp {e_value					"=\[ \\t\]*(\[^ >\\t\\n\]+)"}
	exp {e_comment_start			"\\<!--"}
	exp {e_comment_end				"--\\>"}
	exp {e_string_delimiter			"\""}
	exp {e_alt_string_delimiter		"'"}
	exp {e_ascii					"(&)\[#0-9a-z\]+(;)"}

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

	
	map {m_html						e_html_start			{e_tag_end}					$style_delimiter	0				$style_delimiter}
	map {m_html_keyword				e_word					{e_tag_end:0}				$style_delimiter	0				0}

	map {m_XSLT_element				e_xslt_element_start	{e_tag_end}					$style_keyword		0				$style_keyword}
	map {m_function					e_word					{e_tag_end:0}				$style_function		0				0}
	map {m_boolean_variable			e_word					{}							$style_variable		0				0}
	map {m_variable					e_variable:0			{e_tag_end:0}				$style_variable		0				0}
	map {m_equals					e_equals				{e_tag_end:0}				$style_operator		0				0}
	map {m_value					e_value:0				{}							$style_value		0				0}
	map {m_comment					e_comment_start			{e_comment_end}				$style_comment		$style_comment	$style_comment}
	map {m_string					e_string_delimiter		{e_string_delimiter}		$style_delimiter	$style_string	$style_delimiter}
	map {m_alt_string				e_alt_string_delimiter	{e_alt_string_delimiter}	$style_delimiter	$style_string	$style_delimiter}
	map {m_ascii					e_ascii:0				{e_ascii:1}					$style_delimiter	$style_value	$style_delimiter}

	map {m_js_keywords				e_js_keywords			{}							$style_keyword		0				0}
	map {m_js_types					e_js_types				{}							$style_type			0				0}
	map {m_js_quoted_char			e_js_quoted_char:0		e_js_quoted_char:1			$style_delimiter	0				$style_char}
	map {m_js_comment				e_js_comment_start		e_js_comment_end			$style_comment		$style_comment	$style_comment}
	map {m_js_alt_comment			e_js_alt_comment_start	e_js_end_of_line			$style_comment		$style_comment	$style_comment}
	map {m_js_string				e_js_string_delimiter	e_js_string_delimiter		$style_delimiter	$style_string	$style_delimiter}
	map {m_js_char					e_js_char_delimiter		e_js_char_delimiter			$style_delimiter	$style_char		$style_delimiter}
	map {m_js_function_name			e_js_function_name:0	{}							$style_function		0				0}

	
	at {m_XSLT_element				{m_function}}
	at {m_html						{m_html_keyword}}
	at {m_function					{m_string m_alt_string m_variable m_boolean_variable}}
	at {m_html_keyword				{m_string m_alt_string m_variable m_boolean_variable}}
	at {m_variable					{m_equals m_variable m_boolean_variable}}
	at {m_equals					{m_string m_alt_string m_value m_variable m_boolean_variable}}
	
	at {m_js_string					{m_js_quoted_char}}
	at {m_js_char					{m_js_quoted_char}}

	root {m_XSLT_element m_comment m_html m_ascii}
}

set	extensionHuntExpression(XSL)	{^((.*\.xsl))$}
set	extensionTabSize(XSL)			4
set	extensionColorScheme(XSL)		"Deep Blue"
set	extensionMapName(XSL)			"XSL"

# TODO: move this to only scan in a <SCRIPT> tag
# m_js_quoted_char m_js_keywords m_js_types m_js_comment m_js_alt_comment m_js_string m_js_char m_js_function_name