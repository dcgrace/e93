# Setup favorite highlight scheme for all installed syntax maps
# overriding the "e93" default.
# Adam Yellen with much help from Todd Squires

set favoriteScheme	"Deep Blue";

foreach map [syntaxmaps] \
{
	set extensionColorScheme($map) $favoriteScheme;
}
