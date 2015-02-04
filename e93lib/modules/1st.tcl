# Hello

# If you don't want this file to open each time e93 starts, remove or comment out the following line:
SmartOpenList [info script]

### READ THIS ####

# I am opening this file at e93 startup hoping that you'll read it. Please do.

# If this isn't the first time you've used e93 then...
# ...you might ask, "where have all the menus gone?"

# well...  it's like this; e93 in its raw form does very little.
# It being a "programmer's editor" you, the "programmer",
# are expected to configure it to do the things you want it to do.

# I have in the past included some of my personal sloppy configuration
# packages as examples of how you might choose to do things. They are
# still here, but they don't automatically load as the default anymore.

# I think the old way was confusing some people or at least allowing
# them to use e93 without understanding much about how they should use
# it. These scripts are mine and many are only useful to me in their
# unedited form. However, I include them hoping that they may provide a
# good source of information in the form of examples to some people.

# This is the file in which you specify which "packages" to load.

# Add the ones you want to load to the "SourceFiles" list below and they will load at startup.

# You should understand what they do and decide if you really want
# them before added them to the list. Some automatically load others
# that they need to do their job. If you know any Tcl this will all be
# obvious and easy. If you don't know any Tcl/Tk, you'll need to learn
# at least a little.

# You really need to understand this stuff to get the most out of e93.

# All my e93 "rc" files are defined as tcl packages in the e93/e93lib/packages directory

# This is where we keep all our "packages"
set package_dir [file join [file dirname $tcl_rcFileName]/packages]

# You could uncomment the following three lines to autoload procs defined in $package_dir, if you felt the need
# auto_mkindex $package_dir
# pkg_mkIndex $package_dir
# set auto_path "$package_dir $auto_path"

# tell Tcl where to find any packages you might want to load
foreach file [glob $package_dir/*.tcl] \
{
	package ifneeded [file tail $file] 1 "source $file"
}

# These are all the e93rc files will be loaded at startup.
# If the file don't really exist, no harm done as long as nothing requires them.
# (*** ADD YOUR OWN, OR MY EXAMPLES, TO THIS LIST ONE PER LINE ***)
set SourceFiles \
	"
	$::tcl_platform(platform).tcl
 	"

# "Require" all of the packages we defined to force their load now
foreach file $SourceFiles \
{
	catch "package require $file"
}

# Add any of the following "packages" to the SourceFiles list above and they will be loaded by e93 at startup.
# Some of them have dependencies on others, so e93 may load more than just the package you added to the list.
# You are encouraged to read them and learn how they work.
# You can load them one at a time like this:
#
# package require grep.tcl
#
# They might give you idea of how to configure e93 to your liking.
# Knowing how to do this is key to getting the most out of e93.
# Yes, you will have to learn some Tcl.

#foreach file [glob $package_dir/*.tcl] \
#{
#	puts #\t[file tail $file]
#}

# e93 example rc package files:
#	additions.tcl
#	ant.tcl
#	backup.tcl
#	c.tcl
#	clearcase.tcl
#	colorme.tcl
#	completion.tcl
#	eiffel.tcl
#	globals.tcl
#	grep.tcl
#	greputils.tcl
#	help.tcl
#	html.tcl
#	make.tcl
#	marks.tcl
#	projects.tcl
#	rcfiles.tcl
#	scratch.tcl
#	sgml.tcl
#	tcl.tcl
#	tests.tcl
#	tk.tcl
#	vc.tcl
#	windows.tcl
#	worksheet.tcl
#	www.tcl
#	xml.tcl
#	xslt.tcl

# You can (should) learn to write your own.
# If you do and think they might be useful to other people, send them to me:
# e93@meretrx.com
