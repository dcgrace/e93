# Syntax coloring rules for CSS (cascading style sheets)

addsyntaxmap "CSS" \
{
	exp {e_keywords				"@\[^\\s\]+"}
	exp {e_media_type			"screen|print|aural|Braille|embossed|projection|tty|tv"}
	exp {e_prop_start			"display|color|margin-\[^:\]+|font-\[^:\]+|text-\[^:\]+|vertical-\[^:\]+|padding-\[^:\]+:"}
	exp {e_prop_end				";|\\n"}
	exp {e_rule_start			"\\\{"}
	exp {e_rule_end				"\}"}
	exp {e_selector				"\\*|\\>|,|first-child|linked|visited|active|hover|focus|lang|#"}
	exp {e_comment_start		"/\\*"}
	exp {e_comment_end			"\\*/"}
	

	map {m_keyword				e_keywords				{}					$style_keyword		0				0}
	map {m_media_type			e_media_type			{}					$style_type			0				0}
	map {m_prop					e_prop_start			{e_prop_end}		$style_keyword		$style_value	0}
	map {m_rule					e_rule_start			{e_rule_end}		$style_delimiter	$style_string	$style_delimiter}
	map {m_selector				e_selector				{}					$style_operator		0				0}
	map {m_comment				e_comment_start			{e_comment_end}						$style_comment		$style_comment	$style_comment}

	at {m_rule					{m_selector m_rule m_prop}}

	root {m_comment m_keyword m_media_type m_rule}
}

set	extensionHuntExpression(CSS)	{^((.*\.css))$}
set	extensionTabSize(CSS)			4
set	extensionColorScheme(CSS)		"Green"
set	extensionMapName(CSS)			"CSS"


