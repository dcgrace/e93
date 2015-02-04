# Test addmenu
addmenu {}		LASTCHILD		0	{space0}		{\\S}	{}
addmenu {} 		FIRSTCHILD		1	{Test}			{}		{}
addmenu {} 		LASTCHILD		1	{Test}			{\\KT}	{okdialog {command on top menu}}
# stop, check to see if the name appeared OK

	addmenu {Test}		 	LASTCHILD			1	{Test1} 				{} 						{}
	addmenu {Test}		 	LASTCHILD			1	{Test2} 				{} 						{}
	addmenu {Test}		 	LASTCHILD			0	{Test3} 				{\\K6} 					{okdialog {Shouldn't get to this}}
	addmenu {Test}		 	LASTCHILD			1	{Test4} 				{} 						{}
	addmenu {Test}		 	LASTCHILD			1	{Test5} 				{\\K5}					{okdialog {Command-5}}
	addmenu {Test}		 	LASTCHILD			1	{Test6} 				{} 						{}
	addmenu {Test}		 	LASTCHILD			1	{Test7} 				{\\S} 					{}
	addmenu {Test Test5} 	LASTCHILD			1	{child of command}		{}						{}
	addmenu {Test Test3} 	PREVIOUSSIBLING		1	{before 3}				{}						{}
	addmenu {Test Test3} 	NEXTSIBLING			1	{after 3}				{}						{}
	addmenu {Test Test5} 	PREVIOUSSIBLING		0	{before 5}				{}						{}
	addmenu {Test Test5} 	NEXTSIBLING			0	{after 5}				{}						{}
	
	# this only works on Windows
	addmenu {Test}			LASTCHILD			1	{Bound}					{\Kx1110000000F5}		{okdialog "Bound to Ctrl+Shift+Alt F5"}


addmenu {Test} 	NEXTSIBLING		1	{after Test} 	{}		{}
addmenu {Test} 	PREVIOUSSIBLING 1	{before Test} 	{}		{}


addmenu {Test} LASTCHILD		1	{org r}			{\\Kr}	{okdialog {org r}}
addmenu {Test} LASTCHILD		1	{new r}			{\\Kr}	{okdialog {new r}}

# The folloing character may, or may not, be a key on your keyboard.
# If it is, e93 will use it for the menu command key, if it isn't e93 will ignore it
addmenu {Test} LASTCHILD		1	{Unusual Char}	{\\Kü}	{okdialog {Command-ü}}

# Key name translation
# these are equvilent on Windows, but the name will always appear in the menu 
addmenu {Test} LASTCHILD		1	{Comma}			{\\K,}			{okdialog {Command-comma}}
addmenu {Test} LASTCHILD		1	{Comma}			{\\Kcomma}		{okdialog {Command-comma}}
addmenu {Test} LASTCHILD		1	{Space}			{\\Kspace}		{okdialog {Command-space}}
addmenu {Test} LASTCHILD		1	{Dollar}		{\\K$}			{okdialog {Command-dollar}}
addmenu {Test} LASTCHILD		1	{Dollar}		{\\Kdollar}		{okdialog {Command-dollar}}
addmenu {Test} LASTCHILD		1	{Exclam}		{\\K!}			{okdialog {Command-exclam}}
addmenu {Test} LASTCHILD		1	{Exclam}		{\\Kexclam}		{okdialog {Command-exclam}}
addmenu {Test} LASTCHILD		1	{Plus}			{\\K+}			{okdialog {Command-plus}}
addmenu {Test} LASTCHILD		1	{Plus}			{\\Kplus}		{okdialog {Command-plus}}
addmenu {Test} LASTCHILD		1	{Add}			{\\KKP_Add}		{okdialog {Command-add}}
addmenu {Test} LASTCHILD		1	{Slash}			{\\K/}			{okdialog {Command-divide}}
addmenu {Test} LASTCHILD		1	{Slash}			{\\Kslash}		{okdialog {Command-divide}}
addmenu {Test} LASTCHILD		1	{Divide}		{\\KKP_Divide}	{okdialog {Command-divide}}

# Translations will happen if the name applies to a modified key on the current keyboard
# for example, "grave" is unmodified on a US keyboard, but is shifted on a Swiss-German one
# So if you use the name "grave" for a key with that keyboard you'll get "Shift AsciiCircum",
# which is the correct key combination to produce a "grave" character.


addmenu {} LASTCHILD			0	{disabled}		{} 		{}
addmenu {disabled} LASTCHILD	1	{child}			{} 		{}

addmenu {Mark} NEXTSIBLING		1	{after Mark} 	{} 		{}
addmenu {Mark} PREVIOUSSIBLING	1	{before Mark} 	{} 		{}

# undo all that
deletemenu {space0}
deletemenu {Test {before 3}}
deletemenu {Test}
deletemenu {{before Test}}
deletemenu {{after Test}}
deletemenu {{disabled}}
deletemenu {{before Mark}}
deletemenu {{after Mark}}


# intentional Invalid menu path
addmenu {Crap}		 	LASTCHILD			1	{Test1} 	{} 						{}

addmenu {} 				FIRSTCHILD			1	{Test}		{}						{}
# intentional Invalid key name: badKeyName
addmenu {Test}		 	LASTCHILD			1	{Test1} 	{\\KbadKeyName} 		{}




