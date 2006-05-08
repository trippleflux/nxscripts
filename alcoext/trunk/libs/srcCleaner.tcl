#
# AlcoExt - Alcoholicz Tcl extension.
# Copyright (c) 2006 Alcoholicz Scripting Team
#
# Module Name:
#   Source Cleaner
#
# Author:
#   neoxed (neoxed@gmail.com) May 8, 2006
#
# Abstract:
#   Cleans source files.
#

# Remove trailing and leading newlines.
set removeNewLines 1

# Remove whitespace from the end of lines.
set removeWhitespace 1

# Ignore lines matching these wildcard patterns.
set ignorePatterns {
    {*$Source: * $*}
    {*$Revision: * $*}
    {*$Date: * $*}
}

# Clean files matching these wildcard patterns.
set filePatterns {
    {*.c}
    {*.cpp}
    {*.h}
    {*.hpp}
}

puts "\n\tSource File Cleaner\n\n"

puts -nonewline "Target Directory: "; flush stdout
set targetDir [file normalize [gets stdin]]
if {![file isdirectory $targetDir]} {
	puts "The target directory does not exist, exiting."
	exit 1
}

puts "Cleaning files..."
set failureList [list]
set successList [list]

proc ListMatch {patterns value} {
    foreach pattern $patterns {
        if {[string match $pattern $value]} {return 1}
    }
    return 0
}

foreach path [glob -nocomplain -directory $targetDir -types f "*"] {
    if {![ListMatch $filePatterns $path]} {continue}

    # Open the file for reading and buffer its contents.
    if {[catch {set handle [open $path "r"]} message]} {
        lappend failureList [file tail $path]
        continue
    }
    set data [read -nonewline $handle]
    close $handle

    # Rewrite the file.
    if {[catch {set handle [open $path "w"]} message]} {
        lappend failureList [file tail $path]
        continue
    }

    # Remove whitespace and unwanted lines.
    set newData [list]
    foreach line [split $data "\n"] {
        if {[ListMatch $ignorePatterns $line]} {continue}
        if {$removeWhitespace} {
            set line [string trimright $line]
        }
        append newData $line "\n"
    }
    if {$removeNewLines} {
        set newData [string trim $newData]
        append newData "\n"
    }

    puts -nonewline $handle $newData
    close $handle
    lappend successList [file tail $path]
}

puts "Cleaned [llength $successList] file(s)."
if {[llength $failureList]} {
    puts "Unable to clean [llength $failureList] file(s): [join $failureList {, }]"
}
puts "Finished."
exit 0
