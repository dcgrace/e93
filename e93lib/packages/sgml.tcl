package provide [file tail [info script]] 1

addmenu {} LASTCHILD 1 "SGML" "" ""

addmenu {SGML}	LASTCHILD 1 "Space"					{\\S}				{}
addmenu {SGML}	LASTCHILD 1 "Open in browser"		{}					{OpenURL "file://[file attributes [ActiveWindowOrBeep] -shortname]"}
addmenu {SGML}	LASTCHILD 1 "lowercase tags and attributes"		{} 		{LowercaseTagsAndAttributes [ActiveWindowOrBeep]}

