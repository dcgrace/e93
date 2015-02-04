set e93Help [file join $PROGRAMPATH "e93.hlp"]
windowswinhelp $e93Help deletemenu

set e93Help [file join $PROGRAMPATH "e93.chm"]
windowshtmlhelp $e93Help
windowshtmlhelp $e93Help deletemenu

catch {windowshtmlhelp Z:/Wip/mmanning/e93/wombat}
catch {windowshtmlhelp}

