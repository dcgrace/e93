package provide [file tail [info script]] 1

proc Backup {} {
Run {xxcopy /YY /CLONE /X*.class D:\\weblogic6_1\\config\\wrapworlddomain\\applications\\WrapWorld\\ P:\\Michael\\WrapWorld}
Run {xxcopy /YY /CLONE D:\\webdev_webapp\\projects\\WrapWorldWebApp\\src P:\\Michael\\src; xxcopy /YY /CLONE  /X*.class  /X*.obj D:\\Projects\\ P:\\Michael\\Projects}
Run {xxcopy /YY /CLONE  /X*.class  /X*.obj D:\\Projects\\ P:\\Michael\\Projects}
}

addmenu {} LASTCHILD 1 "Backup" "" ""
	addmenu {Backup} LASTCHILD 1 "Backup"						{}	{Backup}
	addmenu {Backup} LASTCHILD 1 "Mirror"						{}	{Run {cd C:/Mirror; perl mirror to_home.pkg}}
