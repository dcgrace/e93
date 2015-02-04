# Define some highlight schemes.

# NOTE: for the default style, an empty string (or any invalid font
# name) forces e93 to choose a reasonable font. For all other styles, an
# empty font or color string tells e93 to use the default entry's
# values.
#
# NOTE: if either selection color is left unspecified, then the given
# color (foreground or background) of selected text will be left alone.
# NOTE: if both selection colors are unspecified, then the selection
# will invert the foreground and background colors...
# 
#										foreground		background		font
#										----------		----------		----
set hs_e93($style_default)				{white			37378a		 	"-b&h-lucidatypewriter-medium-r-normal-*-12-*-*-*-*-*-*-*"}
set hs_e93($style_comment)				{aquamarine		""				""}
set hs_e93($style_string)				{green			""				""}
set hs_e93($style_char)					{green			""				""}
set hs_e93($style_digit)				{orange			""				""}
set hs_e93($style_operator)				{yellow			""				""}
set hs_e93($style_variable)				{cyan			""				""}
set hs_e93($style_value)				{gold			""				""}
set hs_e93($style_delimiter)			{yellow			""				"-b&h-lucidatypewriter-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1"}
set hs_e93($style_keyword)				{yellow			""				"-b&h-lucidatypewriter-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1"}
set hs_e93($style_type)					{yellow			""				"-b&h-lucidatypewriter-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1"}
set hs_e93($style_directive)			{cyan			""				"-b&h-lucidatypewriter-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1"}
set hs_e93($style_function)				{orange			""				""}
set hs_e93(selection)					{black			yellow}
set "HighlightSchemes(e93)"				hs_e93

set hs_deepblue($style_default)			{white			27277a			"-b&h-lucidatypewriter-medium-r-normal-*-12-*-*-*-*-*-*-*"}
set hs_deepblue($style_comment)			{green			""				""}
set hs_deepblue($style_string)			{steelblue2		""				""}
set hs_deepblue($style_char)			{steelblue2		""				""}
set hs_deepblue($style_digit)			{steelblue2		""				""}
set hs_deepblue($style_operator)		{gold			""				""}
set hs_deepblue($style_variable)		{lightblue		""				""}
set hs_deepblue($style_value)			{orange			""				""}
set hs_deepblue($style_delimiter)		{yellow3		""				""}
set hs_deepblue($style_keyword)			{gold			""				""}
set hs_deepblue($style_type)			{yellow2		""				""}
set hs_deepblue($style_directive)		{lightblue		""				""}
set hs_deepblue($style_function)		{orange			""				""}
set hs_deepblue(selection)				{midnightblue	gold}
set "HighlightSchemes(Deep Blue)"		hs_deepblue

set hs_nedit($style_default)			{black			gray90 			"-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*"}
set hs_nedit($style_comment)			{""				""				"-adobe-courier-medium-o-normal--12-*-*-*-*-*-*"}
set hs_nedit($style_string)				{darkgreen		""				""}
set hs_nedit($style_char)				{darkgreen		""				""}
set hs_nedit($style_digit)				{""				""				""}
set hs_nedit($style_operator)			{""				""				""}
set hs_nedit($style_variable)			{darkblue		""				""}
set hs_nedit($style_value)				{darkred		""				""}
set hs_nedit($style_delimiter)			{""				""				""}
set hs_nedit($style_keyword)			{""				""				"-adobe-courier-bold-r-normal--12-*-*-*-*-*-*"}
set hs_nedit($style_type)				{darkred		""				"-adobe-courier-bold-r-normal--12-*-*-*-*-*-*"}
set hs_nedit($style_directive)			{darkblue		""				""}
set hs_nedit($style_function)			{""				""				""}
set hs_nedit(selection)					{""				gray80}
set "HighlightSchemes(Nedit)"			hs_nedit

set hs_netscape($style_default)			{black			c0c0c0 			"-adobe-courier-medium-r-normal-*-10-*-*-*-*-*-*"}
set hs_netscape($style_comment)			{""				""				"-adobe-courier-medium-o-normal--10-*-*-*-*-*-*"}
set hs_netscape($style_string)			{003e98			""				""}
set hs_netscape($style_char)			{003e98			""				""}
set hs_netscape($style_digit)			{""				""				""}
set hs_netscape($style_operator)		{""				""				""}
set hs_netscape($style_variable)		{""				""				"-adobe-courier-bold-r-normal--10-*-*-*-*-*-*"}
set hs_netscape($style_value)			{003e98			""				""}
set hs_netscape($style_delimiter)		{""				""				"-adobe-courier-bold-r-normal--10-*-*-*-*-*-*"}
set hs_netscape($style_keyword)			{purple4		""				"-adobe-courier-bold-r-normal--10-*-*-*-*-*-*"}
set hs_netscape($style_type)			{darkred		""				"-adobe-courier-bold-r-normal--10-*-*-*-*-*-*"}
set hs_netscape($style_directive)		{003e98			""				""}
set hs_netscape($style_function)		{""				""				""}
set hs_netscape(selection)				{black			ffffcc}
set "HighlightSchemes(Netscape)"		hs_netscape

set hs_visualstudio($style_default)		{black			white 			"-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*"}
set hs_visualstudio($style_comment)		{darkgreen	 	""				""}
set hs_visualstudio($style_string)		{green			""				""}
set hs_visualstudio($style_char)		{green			""				""}
set hs_visualstudio($style_digit)		{""				""				""}
set hs_visualstudio($style_operator)	{orange			""				""}
set hs_visualstudio($style_variable)	{darkblue		""				""}
set hs_visualstudio($style_value)		{darkred		""				""}
set hs_visualstudio($style_delimiter)	{""				""				""}
set hs_visualstudio($style_keyword)		{blue			""				""}
set hs_visualstudio($style_type)		{blue			""				""}
set hs_visualstudio($style_directive)	{blue			""				""}
set hs_visualstudio($style_function)	{""				""				""}
set hs_visualstudio(selection)			{white			darkblue}
set "HighlightSchemes(Visual Studio)"	hs_visualstudio

# define a plain highlight scheme based on Visual Studio without the garish colors
set hs_simplewhite($style_default)		{black			white			"-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*"}
set hs_simplewhite($style_comment)		{orange			""				""}
set hs_simplewhite($style_string)		{maroon			""				""}
set hs_simplewhite($style_char)			{maroon			""				""}
set hs_simplewhite($style_digit)		{""				""				""}
set hs_simplewhite($style_operator)		{""				""				""}
set hs_simplewhite($style_variable)		{darkblue		""				""}
set hs_simplewhite($style_value)		{darkred		""				""}
set hs_simplewhite($style_delimiter)	{""				""				""}
set hs_simplewhite($style_keyword)		{blue			""				""}
set hs_simplewhite($style_type)			{blue			""				""}
set hs_simplewhite($style_directive)	{blue			""				""}
set hs_simplewhite($style_function)		{darkgreen		""				""}
set hs_simplewhite(selection)			{black			gray80			""}
set "HighlightSchemes(Simple White)"	hs_simplewhite

# useful for looking at manual pages
set hs_manpage($style_default)			{white			darkblue		"-b&h-lucidatypewriter-medium-r-normal-*-12-*-*-*-*-*-*-*"}
set hs_manpage($style_comment)			{""				""				""}
set hs_manpage($style_string)			{""				""				"-b&h-lucida-medium-i-normal-sans-12-120-75-75-p-71-iso8859-1"}
set hs_manpage($style_char)				{""				""				""}
set hs_manpage($style_digit)			{""				""				""}
set hs_manpage($style_operator)			{""				""				""}
set hs_manpage($style_variable)			{""				""				""}
set hs_manpage($style_value)			{""				""				""}
set hs_manpage($style_delimiter)		{""				""				""}
set hs_manpage($style_keyword)			{yellow			""				"-b&h-lucidatypewriter-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1"}
set hs_manpage($style_type)				{""				""				""}
set hs_manpage($style_directive)		{""				""				""}
set hs_manpage($style_function)			{""				""				""}
set hs_manpage(selection)				{""				""				""}
set "HighlightSchemes(Man Page)"		hs_manpage

# used when opening windows which are low-priority (like clipboards and temporary buffers) 
set hs_lowpriority($style_default)		{white			black		 	"-b&h-lucidatypewriter-medium-r-normal-*-12-*-*-*-*-*-*-*"}
set hs_lowpriority($style_comment)		{aquamarine		""				""}
set hs_lowpriority($style_string)		{green			""				""}
set hs_lowpriority($style_char)			{green			""				""}
set hs_lowpriority($style_digit)		{orange			""				""}
set hs_lowpriority($style_operator)		{yellow			""				""}
set hs_lowpriority($style_variable)		{cyan			""				""}
set hs_lowpriority($style_value)		{gold			""				""}
set hs_lowpriority($style_delimiter)	{yellow			""				"-b&h-lucidatypewriter-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1"}
set hs_lowpriority($style_keyword)		{yellow			""				"-b&h-lucidatypewriter-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1"}
set hs_lowpriority($style_type)			{yellow			""				"-b&h-lucidatypewriter-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1"}
set hs_lowpriority($style_directive)	{cyan			""				"-b&h-lucidatypewriter-bold-r-normal-sans-12-120-75-75-m-70-iso8859-1"}
set hs_lowpriority($style_function)		{orange			""				""}
set hs_lowpriority(selection)			{""				""}
set "HighlightSchemes(Low Priority)"	hs_lowpriority

# define a highlight scheme that looks something like what Borland's JBuilder uses
set hs_java($style_default)				{black			white			"-adobe-courier new-medium-r-normal-*-8-*-*-*-*-*-*"}
set hs_java($style_comment)				{darkgreen		""				""}
set hs_java($style_string)				{blue			""				""}
set hs_java($style_char)				{blue			""				""}
set hs_java($style_digit)				{black			""				""}
set hs_java($style_operator)			{red			""				""}
set hs_java($style_variable)			{black			""				""}
set hs_java($style_delimiter)			{grey40			""				""}
set hs_java($style_keyword)				{darkblue		""				"-adobe-courier new-bold-r-normal-*-8-*-*-*-*-*-*"}
set hs_java($style_type)				{blue			""				""}
set hs_java($style_directive)			{blue			""				""}
set hs_java($style_function)			{red			""				""}
set hs_java(selection)					{white			darkblue		""}
set "HighlightSchemes(JBuilder)"		hs_java

# 
set hs_green($style_default)			{gray80			003530			"-*-*-medium-*-normal-sans-8-*-*-*-*-*-*-*"}
set hs_green($style_comment)			{00751e			""				""}
set hs_green($style_string)				{0080c0			""				""}
set hs_green($style_char)				{lightgreen		""				""}
set hs_green($style_digit)				{tomato			""				""}
set hs_green($style_operator)			{orange			""				""}
set hs_green($style_variable)			{lightblue		""				""}
set hs_green($style_value)				{orange			""				""}
set hs_green($style_delimiter)			{yellow			""				"-b&h-lucidatypewriter-bold-r-normal-sans-8-120-75-75-m-70-iso8859-1"}
set hs_green($style_keyword)			{b9b900			""				"-b&h-lucidatypewriter-bold-r-normal-sans-8-120-75-75-m-70-iso8859-1"}
set hs_green($style_type)				{orange			""				""}
set hs_green($style_directive)			{green			""				""}
set hs_green($style_function)			{lightblue		""				"-*-lucida-bold-r-normal-sans-9-*-*-*-*-*-*-*"}
set hs_green(selection)					{black			yellow			""}
set "HighlightSchemes(Green)"			hs_green

set hs_ibm($style_default)				{black			white 			"-adobe-courier-medium-r-normal-*-10-*-*-*-*-*-*"}
set hs_ibm($style_comment)				{008080	 		""				""}
set hs_ibm($style_string)				{800080			""				""}
set hs_ibm($style_char)					{800080			""				""}
set hs_ibm($style_digit)				{""				""				""}
set hs_ibm($style_operator)				{darkblue		""				""}
set hs_ibm($style_variable)				{darkblue		""				""}
set hs_ibm($style_value)				{darkred		""				""}
set hs_ibm($style_delimiter)			{""				""				""}
set hs_ibm($style_keyword)				{blue			""				""}
set hs_ibm($style_type)					{blue			""				""}
set hs_ibm($style_directive)			{blue			""				""}
set hs_ibm($style_function)				{""				""				""}
set hs_ibm(selection)					{white			darkblue}
set "HighlightSchemes(IBM)"	hs_ibm

set hs_ie($style_default)				{darkred			white 		"-adobe-courier-medium-r-normal-*-10-*-*-*-*-*-*"}
set hs_ie($style_comment)				{gray60	 		""				""}
set hs_ie($style_string)				{black			""				"-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*"}
set hs_ie($style_char)					{black			""				"-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*"}
set hs_ie($style_digit)					{""				""				""}
set hs_ie($style_operator)				{blue			""				""}
set hs_ie($style_variable)				{darkred		""				""}
set hs_ie($style_value)					{darkred		""				""}
set hs_ie($style_delimiter)				{blue			""				""}
set hs_ie($style_keyword)				{blue			""				""}
set hs_ie($style_type)					{""				""				""}
set hs_ie($style_directive)				{blue			""				""}
set hs_ie($style_function)				{""				""				""}
set hs_ie(selection)					{white			darkblue}
set "HighlightSchemes(InternetExplorer)"	hs_ie
