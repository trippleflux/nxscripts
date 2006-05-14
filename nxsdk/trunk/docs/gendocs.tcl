#
# nxSDK - ioFTPD Software Development Kit
# Copyright (c) 2006 neoxed
#
# Module Name:
#   Gen Docs
#
# Author:
#   neoxed (neoxed@gmail.com) May 13, 2006
#
# Abstract:
#   Generates source documentation.
#

# Base directory to change to.
set baseDir ".."

# Location for the output file.
set outputFile "docs.txt"

# Location of the input files.
set inputFiles {
    "src/nxsdk.h"
    "src/lib/*.c"
    "src/lib/*.h"
}

proc ReadFile {path} {
    set handle [open $path "r"]
    set data [read -nonewline $handle]
    close $handle
    return $data
}

proc ParseBlocks {text} {
    set inBlock 0
    set result [list]

    foreach line [split $text "\n"] {
        if {$line eq "/*++"} {
            if {$inBlock} {
                puts "    - Found comment start but we are already in a comment block."
            } else {
                set inBlock 1
                set block [list]
            }
        } elseif {$line eq "--*/"} {
            if {!$inBlock} {
                puts "    - Found comment end but we are not in a comment block."
            } else {
                lappend result $block
                set inBlock 0
                set block [list]
            }
        } elseif {$inBlock} {
            lappend block $line
        }
    }

    return $result
}

puts "\n\tGenerating Source Documents\n"

puts "- Changing to base directory"
set currentDir [pwd]
cd $baseDir

puts "- Parsing source files"
foreach pattern $inputFiles {
    foreach path [glob -nocomplain $pattern] {
        puts "  - Parsing file: $path"
        set text [ReadFile $path]
        set data($path) [ParseBlocks $text]
    }
}

puts "- Changing back to original directory"
cd $currentDir

puts "- Transforming data"
set handle [open $outputFile "w"]
foreach path [lsort [array names data]] {
    puts $handle "################################################################################"
    puts $handle "# $path"
    puts $handle "################################################################################"

    foreach block $data($path) {
        foreach line $block {puts $handle $line}
        puts $handle "############################################################"
    }
}
close $handle

puts "- Finished"
return 0
