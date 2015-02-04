package provide [file tail [info script]] 1

proc MakeGrepCommand {dir globPattern pattern {ignoreCase "0"}} \
{
	set options "-n"
	if {$ignoreCase} \
	{
		append options " -i"
	}

	set files [file join $dir $globPattern]
	return "if {\[catch {grep $options $pattern $files} result\]== 0} {puts -nonewline \$result}\;\n"
}


proc Grep {dir globPatterns pattern {ignoreCase "0"} {recursive "0"}} \
{
	foreach globPattern [split $globPatterns] \
	{
		append commands [MakeGrepCommand $dir $globPattern $pattern $ignoreCase]
	}
	
	if {$recursive} \
	{
		# check for any directories in the target directory, and process them
		foreach subDirectory [glob -nocomplain [file join $dir *]] \
		{
			if {[file isdirectory $subDirectory]} \
			{
				set subDirectory [file tail $subDirectory]
				set subDir [file join $dir $subDirectory]
				foreach globPattern [split $globPatterns] \
				{
					append commands [Grep $subDir $globPattern $pattern $ignoreCase 1]
				}
			}
		}
	}
	
	return $commands
}
