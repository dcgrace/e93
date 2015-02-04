setchannelbuffer stdout
setchannelbuffer stdout

setchannelbuffer worksheet $workSheetBufName

puts this
puts stderr this
puts worksheet this

setchannelbuffer stdout $workSheetBufName
setchannelbuffer stderr $workSheetBufName

puts this
puts stderr this

setchannelbuffer stdout tclConsole
getchannelbuffer stdout


setchannelbuffer worksheet
puts worksheet this
catch {getchannelbuffer worksheet}
close worksheet
catch {puts worksheet this}

setchannelbuffer stderr tclConsole
closebuffer tclConsole
catch {puts stderr this}
catch {setchannelbuffer stderr tclConsole}

