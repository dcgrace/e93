# Color map (syntax map) for Prolog
# Author: Andrew Khimenko
#
# Remarks:
# 	template for numbers: [sign] [digits] [.digits] [{ e | E} [sign] digits]

addsyntaxmap "Prolog" \
{
	exp {e_keywords				"<(domains|predicates|clauses|goal)>"}
	exp	{e_types				"<(integer|real|char|symbol)>"}
	exp {e_comment_start		"/\\*"}
	exp {e_comment_end			"\\*/"}
	exp	{e_alt_comment_start	"%"}
	exp	{e_alt_comment_end		"$"}
	exp {e_string_delimiter		"\""}
	exp {e_char_delimiter		"'"}
	exp {e_number				"((\\+?)|(\\-?))(\[0-9\])((\\.(\[0-9\])+)|(((e?)|(E?)))(((\\+?)|(\\-?))(\[0-9\]+)))"}

	map {m_keywords			e_keywords				{}						$style_keyword		0				0}
	map	{m_types			e_types					{}						$style_type			0				0}
	map {m_comment			e_comment_start			{e_comment_end}			$style_comment		$style_comment	$style_comment}
	map	{m_alt_comment		e_alt_comment_start		{e_alt_comment_end}		$style_comment		$style_comment	$style_comment}
	map {m_string			e_string_delimiter		{e_string_delimiter}	$style_delimiter	$style_string	$style_delimiter}
	map {m_char				e_char_delimiter		{e_char_delimiter}		$style_delimiter	$style_char		$style_delimiter}
	map {m_number			e_number				{}						$style_digit		0				0}
	
	root {m_keywords m_types m_comment m_alt_comment m_string m_char m_number}
}

set extensionHuntExpression(Prolog)		{^((.*\.pro)|(.*\.PRO))$}
set	extensionTabSize(Prolog)			4
set	extensionColorScheme(Prolog)		"e93"
set	extensionMapName(Prolog)			"Prolog"

