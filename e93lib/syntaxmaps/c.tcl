# Syntax coloring rules for 'C' and 'C++'
# March 19, 2000  TMS

addsyntaxmap "C/C++" \
{
	exp {e_types				"<(auto|bool|char|class|const|double|enum|export|extern|float|friend|inline|int|long|mutable|namespace|operator|private|protected|public|register|short|signed|static|struct|template|typedef|union|unsigned|virtual|void|volatile|wchar_t)>"}
	exp {e_keywords				"<(and|and_eq|asm|bitand|bitor|break|case|catch|compl|const_cast|continue|default|delete|do|dynamic_cast|else|explicit|false|for|goto|if|new|not|not_eq|or|or_eq|reinterpret_cast|static_cast|true|typeid|typename|using|return|sizeof|switch|this|throw|try|while|xor|xor_eq)>"}
	exp {e_directive_intro		"^\[ \\t\]*#"}
	exp {e_directive_include	"<include>"}
	exp {e_directive_others		"<(pragma|define|undef|if|ifdef|ifndef|else|endif|line)>"}
	exp {e_include_quote_start	"\\<"}
	exp {e_include_quote_end	"\\>"}
	exp {e_comment_start		"/\\*"}
	exp {e_comment_end			"\\*/"}
	exp {e_alt_comment_start	"//"}
	exp {e_end_of_line			"$"}
	exp {e_string_delimiter		"\""}
	exp {e_quoted_char			"(\\\\)(\[-\])"}
	exp {e_char_delimiter		"'"}

	map {m_keywords				e_keywords				{}									$style_keyword		0				0}
	map {m_types				e_types					{}									$style_type			0				0}
	map {m_directive			e_directive_intro		{e_end_of_line}						$style_directive	0				0}
	map {m_directive_include	e_directive_include		{e_end_of_line}						$style_directive	0				0}
	map {m_directive_others		e_directive_others		{}									$style_directive	0				0}
	map {m_include_quote		e_include_quote_start	{e_include_quote_end}				$style_delimiter	$style_string	$style_delimiter}
	map {m_quoted_char			e_quoted_char:0			{e_quoted_char:1}					$style_delimiter	0				$style_char}
	map {m_comment				e_comment_start			{e_comment_end}						$style_comment		$style_comment	$style_comment}
	map {m_alt_comment			e_alt_comment_start		{e_end_of_line}						$style_comment		$style_comment	$style_comment}
	map {m_string				e_string_delimiter		{e_string_delimiter}				$style_delimiter	$style_string	$style_delimiter}
	map {m_char					e_char_delimiter		{e_char_delimiter}					$style_delimiter	$style_char		$style_delimiter}

	at {m_directive				{m_quoted_char m_directive_include m_directive_others m_keywords m_types m_comment m_alt_comment m_string m_char}}
	at {m_directive_include		{m_quoted_char m_keywords m_types m_comment m_alt_comment m_string m_include_quote m_char}}
	at {m_include_quote			{m_quoted_char}}
	at {m_string				{m_quoted_char}}
	at {m_char					{m_quoted_char}}

	root {m_quoted_char m_directive m_keywords m_types m_comment m_alt_comment m_string m_char}
}

set	extensionHuntExpression(C/C++)	{^((.*\.c)|(.*\.h)|(.*\.m)|(.*\.cpp)|(.*\.C)|(.*\.c\+\+)|(.*\.cc))$}
set	extensionTabSize(C/C++)			4
set	extensionColorScheme(C/C++)		"e93"
set	extensionMapName(C/C++)			"C/C++"
