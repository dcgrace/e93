package provide [file tail [info script]] 1

global Projects currentProject

set project e93

set "Projects($project)"	D:/projects/e93-2.8r2/e93Project.tcl
set currentProject $project
set windowName "e93"

cd D:/projects/e93-2.8r2

# insert this after the Projects menu
addmenu {Projects} FIRSTCHILD  1 "Current Project" "" ""
	addmenu {{Projects} {Current Project}} LASTCHILD 1 "$project"					{}					{}
	addmenu {{Projects} {Current Project}} LASTCHILD 1 {Space0}						{\\S}				{}
	addmenu {{Projects} {Current Project}} LASTCHILD 1 {Edit Project File}			{}					"OpenList $Projects($project)"

addmenu {{Projects} {Space0}} NEXTSIBLING 1 "Set project - $project"				{}					"source $Projects($project); global currentProject; set currentProject $project"

