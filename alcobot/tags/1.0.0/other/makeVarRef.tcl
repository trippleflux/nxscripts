#!/bin/tclsh
#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Variable Reference Generator
#
# Author:
#   neoxed (neoxed@gmail.com) Oct 8, 2005
#
# Abstract:
#   Implements a script to generate a listing of all available variables.
#

puts "\n\tVariable Index Generator\n\n"

puts -nonewline "Path to the AlcoBot directory \[..\]: "
flush stdout
gets stdin basePath
if {$basePath eq ""} {set basePath ".."}

if {![file isdirectory $basePath]} {
    puts "The directory \"$basePath\" does not exist."
    return 1
}
cd $basePath

# Source required libraries.
puts "- Loading libraries."
source "libs/libConfig.tcl"
source "libs/libTree.tcl"
namespace import -force ::alcoholicz::*

puts "- Opening \"variables.txt\" for writing."
set handle [open "variables.txt" w]

# Write file header.
puts $handle "################################################################################"
puts $handle "#                         AlcoBot - Variable Reference                         #"
puts $handle "################################################################################"
puts $handle {
Format: %[width].[precision](variableName)

- All variable names are in camel casing (lower-case for the first word and the
  first character of all subsequent words is capitalized).

- The 'width' and 'precision' fields work just as they do in the C printf()
  function.
}

proc IndexFile {handle desc filePath} {
    puts "- Indexing file: $filePath"
    puts $handle "############################################################"
    puts $handle [format "# %-56s #" $desc]
    puts $handle "############################################################"
    puts $handle ""

    set config [ConfigOpen $filePath]
    ConfigRead $config
    array set variables [ConfigGetEx $config Variables]
    ConfigClose $config

    # Find the longest theme name.
    set longest 0
    foreach themeName [array names variables] {
        if {[string length $themeName] > $longest} {
            set longest [string length $themeName]
        }
    }

    foreach themeName [lsort [array names variables]] {
        # Find loop variables.
        set varList [list]
        foreach entry $variables($themeName) {
            if {[llength $entry] > 1} {
                foreach {varName loopName} $entry {
                    lappend varList $varName
                }
            } else {
                lappend varList $entry
            }
        }

        # Check substitution types.
        set nameList [list]
        foreach varName $varList {
            if {![regexp -- {^(\w+):(\w)$} $varName dummy name type]} {
                puts "  - Invalid variable \"$varName\" for \"$themeName\"."
                continue
            }
            switch -- $type {
                p - P {
                    lappend nameList ${name}Dir ${name}Full ${name}Name ${name}Path
                }
                t {
                    lappend nameList ${name}Date ${name}Time
                }
                b - d - e - k - m - n - s - z {
                    lappend nameList $name
                }
                default {
                    puts "  - Invalid substitution type \"$varName\" for \"$themeName\"."
                }
            }
        }

        set nameList [join [lsort $nameList] {, }]
        puts $handle [format "%-*s - %s" $longest $themeName $nameList]
    }

    puts $handle ""
    return
}

# Index general variable definition files.
foreach filePath [lsort [glob ./vars/*.vars]] {
    set desc "File: [file tail $filePath]"
    IndexFile $handle $desc $filePath
}

# Index module variable definition files.
foreach filePath [lsort [glob ./modules/*/*/*.vars]] {
    set fileSplit [file split $filePath]
    set desc "Module: [lindex $fileSplit end-1] ([lindex $fileSplit end-2])"
    IndexFile $handle $desc $filePath
}

puts "- Finished."
close $handle
return 0
