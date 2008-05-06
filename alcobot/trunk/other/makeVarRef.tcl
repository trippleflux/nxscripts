#!/bin/tclsh
#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2008 Alcoholicz Scripting Team
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

# Load required packages.
lappend auto_path [file join [pwd] "packages" "alcolibs"]
package require alco::config

puts "- Opening \"variables.txt\" for writing."
set handle [open "variables.txt" w]

# Write file header.
puts $handle "################################################################################"
puts $handle "#                         AlcoBot - Variable Reference                         #"
puts $handle "################################################################################"
puts $handle {
Format: %[width].[precision](variableName)

- All variable names are in camel casing (lower-case for the first word
  and the first character of all subsequent words is capitalized).

- The "width" and "precision" fields work just as they do in the C printf()
  function.

- There are several variables common to all theme entries:

    nowDate  - Current date
    nowTime  - Current time
    prefix   - Theme prefix (defined in [Theme::Core])
    siteName - Site name (defined in AlcoBot.conf)
    siteTag  - Site tag (defined in AlcoBot.conf)
}

proc ParseGroup {handle group data} {
    puts $handle [format {  [Theme::%s]} $group]
    array set variables $data

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
        puts $handle [format "  %-*s - %s" $longest $themeName $nameList]
    }

    puts $handle ""
    return
}

foreach type [lsort [glob -nocomplain -types d "modules/*"]] {
    # Write module type header.
    puts $handle "######################################################################"
    puts $handle [format "# %-66s #" [file tail $type]]
    puts $handle "######################################################################"
    puts $handle ""
    puts $handle "- Modules located in \"$type\"."
    puts $handle ""

    foreach path [lsort [glob -nocomplain -types f [file join $type "*/*.vars"]]] {
        puts "- Reading file: [file tail $path]"

        # Write module header.
        set pathSplit [file split $path]
        puts $handle "  ############################################################"
        puts $handle [format "  # %-56s #" [lindex $pathSplit end-1]]
        puts $handle "  ############################################################"
        puts $handle ""

        # Read variable definition file.
        set config [Config::Open $path]
        Config::Read $config
        foreach section [lsort [Config::Sections $config Variables::*]] {
            set group [string range $section 11 end]
            ParseGroup $handle $group [Config::GetEx $config $section]
        }
        Config::Close $config
    }
}

puts "- Finished."
close $handle
return 0
