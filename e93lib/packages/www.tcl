package provide [file tail [info script]] 1

proc OpenURL {url} {

    switch $::tcl_platform(platform) {
       "unix" {
            if {0 == [info exists ::env(BROWSER)]} {
               if { [ string length [ auto_execok netscape ] ] } {
                  set ::env(BROWSER) [ auto_execok netscape ]
               } elseif {[info exists ::env(NETSCAPE)]} {
                  # Some of us use this envvar instead... - DKF
                  set ::env(BROWSER) $::env(NETSCAPE)
               } elseif { [ string length [ auto_execok lynx ] ] } {
                  # lynx can also output formatted text to a variable
                  # with the -dump option, as a last resort:
                  # set formatted_text [ exec lynx -dump $url ] - PSE
                  set ::env(BROWSER) [ auto_execok lynx ]
               }
            }
            if {[catch {exec $::env(BROWSER) -remote $url}]} {
               # perhaps browser doesn't understand -remote flag
               if {[catch {exec $::env(BROWSER) $url &} emsg]} {
                  error "Error displaying $url in browser\n$emsg"
                  # Another possibility is to just pop a window up with
                  # the URL to visit in it. - DKF
               }
            }
      }
      "windows" {
          if {$::tcl_platform(os) == "Windows NT"} {
             set rc [catch {exec $::env(COMSPEC) /c start $url &} emsg]
          } else {
             # Windows 95/98
             set rc [catch {exec start $url} emsg]
          }
          if {$rc} {
             error "Error displaying $url in browser\n$emsg"
          }
      }
      "macintosh" {
          # AppleScript should be able to provide the default browser,
          # but I'm not enough of an AppleScript hacker to figure out.
          # Help!
          if {0 == [info exists ::env(BROWSER)]} {
             error "Couldn't find a usable browser.  Set env(BROWSER) in your application's preferences file."
          }
          if {[catch {
             AppleScript execute "tell application \"$::env(BROWSER)\"
             open url \"$url\"
             end tell
             "} emsg]
          } then {
             error "Error displaying $url in browser\n$emsg"
          }
       }
    } ;## end of switch
 }