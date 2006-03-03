#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Utility Library
#
# Author:
#   neoxed (neoxed@gmail.com) May 16, 2005
#
# Abstract:
#   Implements commonly used library functions.
#

namespace eval ::alcoholicz {
    namespace export ArgsToList GetResultLimit JoinLiteral InList IsSubDir \
        PathParse PathParseSection PathStrip \
        PermCheck PermMatchFlags \
        SqlEscape SqlGetPattern SqlToLike \
        FormatDate FormatTime FormatDuration FormatDurationLong FormatSize FormatSpeed \
}

################################################################################
# Utilities                                                                    #
################################################################################

####
# ArgsToList
#
# Convert an argument string into a Tcl list, respecting quoted text segments.
#
proc ::alcoholicz::ArgsToList {argStr} {
    set argList [list]
    set length [string length $argStr]

    for {set index 0} {$index < $length} {incr index} {
        # Ignore leading white-space.
        while {[string is space -strict [string index $argStr $index]]} {incr index}
        if {$index >= $length} {break}

        if {[string index $argStr $index] eq "\""} {
            # Find the next quote character.
            set startIndex [incr index]
            while {[string index $argStr $index] ne "\"" && $index < $length} {incr index}
        } else {
            # Find the next white-space character.
            set startIndex $index
            while {![string is space -strict [string index $argStr $index]] && $index < $length} {incr index}
        }
        lappend argList [string range $argStr $startIndex [expr {$index - 1}]]
    }
    return $argList
}

####
# GetResultLimit
#
# Checks the number of requested results.
#
proc ::alcoholicz::GetResultLimit {results} {
    variable defaultResults
    variable maximumResults

    if {$results < 0} {
        return $defaultResults
    }
    if {$results > $maximumResults} {
        return $maximumResults
    }
    return $results
}

####
# JoinLiteral
#
# Convert a Tcl list into a human-readable list.
#
proc ::alcoholicz::JoinLiteral {list {word "and"}} {
    if {[llength $list] < 2} {
        return [join $list]
    }
    set literal [join [lrange $list 0 end-1] ", "]
    if {[llength $list] > 2} {
        append literal ","
    }
    return [append literal " " $word " " [lindex $list end]]
}

####
# InList
#
# Searches a list for a given element (case-insensitively). The -nocase switch
# was not added to lsearch until Tcl 8.5, so this function is provided for
# backwards compatibility with Tcl 8.4.
#
proc ::alcoholicz::InList {list element} {
    foreach entry $list {
        if {[string equal -nocase $entry $element]} {return 1}
    }
    return 0
}

################################################################################
# Path Parsing                                                                 #
################################################################################

####
# IsSubDir
#
# Check if the path's base directory is a known release sub-directory.
#
proc ::alcoholicz::IsSubDir {path} {
    variable subDirList
    set path [file tail $path]
    foreach subDir $subDirList {
        if {[string match -nocase $subDir $path]} {return 1}
    }
    return 0
}

####
# PathStrip
#
# Removes leading, trailing, and duplicate path separators.
#
proc ::alcoholicz::PathStrip {path} {
    regsub -all -- {[\\/]+} $path "/" path
    return [string trim $path "/"]
}

####
# PathParse
#
# Parses elements from a directory path, with subdirectory support.
#
proc ::alcoholicz::PathParse {fullPath {basePath ""}} {
    set basePath [split [PathStrip $basePath] "/"]
    set fullPath [split [PathStrip $fullPath] "/"]
    set fullPath [lrange $fullPath [llength $basePath] end]

    set relDir [lindex $fullPath end]
    set relFull [join [lrange $fullPath 0 end-1] "/"]

    if {[IsSubDir $relDir] && [llength $fullPath] > 1} {
        set relName "[lindex $fullPath end-1] ([lindex $fullPath end])"
        set relPath [join [lrange $fullPath 0 end-2] "/"]
    } else {
        set relName $relDir
        set relPath $relFull
    }

    if {[string length $relFull]} {append relFull "/"}
    if {[string length $relPath]} {append relPath "/"}
    return [list $relDir $relFull $relName $relPath]
}

####
# PathParseSection
#
# Basic PathParse wrapper, uses the section path as the base path.
#
proc ::alcoholicz::PathParseSection {fullPath useSection} {
    set basePath ""

    if {$useSection} {
        variable pathSections
        set section [GetSectionFromPath $fullPath]

        if {[info exists pathSections($section)]} {
            set basePath [lindex $pathSections($section) 0]
        }
    }
    return [PathParse $fullPath $basePath]
}

################################################################################
# Permissions (FTP Style)                                                      #
################################################################################

####
# PermCheck
#
# FTPD style permissions checks: -user, =group, flags, and an
# exclamation character is used for negations.
#
proc ::alcoholicz::PermCheck {rightsList userName groupList flags} {
    foreach right $rightsList {
        regexp -- {^(!?[=-]?)(.+)} $right result prefix right
        switch -- $prefix {
            {!-} {if {[string match $right $userName]} {return 0}}
            {!=} {if {[lsearch -glob $groupList $right] != -1} {return 0}}
            {!}  {if {[PermMatchFlags $flags $right]} {return 0}}
            {-}  {if {[string match $right $userName]} {return 1}}
            {=}  {if {[lsearch -glob $groupList $right] != -1} {return 1}}
            default {if {[PermMatchFlags $flags $right]} {return 1}}
        }
    }
    return 0
}

####
# PermMatchFlags
#
# Check if any of the required flags are present in the current flags.
#
proc ::alcoholicz::PermMatchFlags {currentFlags needFlags} {
    set currentFlags [split $currentFlags {}]
    foreach flag [split $needFlags {}] {
        if {[lsearch -glob $currentFlags $flag] != -1} {return 1}
    }
    return 0
}

################################################################################
# SQL Functions                                                                #
################################################################################

####
# SqlEscape
#
# Escape SQL quote characters with a backslash.
#
proc ::alcoholicz::SqlEscape {string} {
    return [string map {\\ \\\\ ` \\` ' \\' \" \\\"} $string]
}

####
# SqlGetPattern
#
# Prepend, append, and replace all spaces with wildcards then convert
# standard wildcard characters to SQL LIKE characters.
#
proc ::alcoholicz::SqlGetPattern {pattern} {
    set pattern "*$pattern*"
    regsub -all -- {[\s\*]+} $pattern "*" pattern
    return [SqlToLike $pattern]
}

####
# SqlToLike
#
# Convert standard wildcard characters to SQL LIKE characters.
#
proc ::alcoholicz::SqlToLike {pattern} {
    set pattern [SqlEscape $pattern]
    return [string map {* % ? _} [string map {% \\% _ \\_} $pattern]]
}

################################################################################
# Formatting                                                                   #
################################################################################

####
# FormatDate
#
# Formats an integer time value into a human-readable date. If a time value
# is not given, the current date will be used.
#
proc ::alcoholicz::FormatDate {{clockVal ""}} {
    variable format
    variable localTime

    if {![string is digit -strict $clockVal]} {
        set clockVal [clock seconds]
    }
    return [clock format $clockVal -format $format(date) -gmt [expr {!$localTime}]]
}

####
# FormatTime
#
# Formats an integer time value into a human-readable time. If a time value
# is not given, the current time will be used.
#
proc ::alcoholicz::FormatTime {{clockVal ""}} {
    variable format
    variable localTime

    if {![string is digit -strict $clockVal]} {
        set clockVal [clock seconds]
    }
    return [clock format $clockVal -format $format(time) -gmt [expr {!$localTime}]]
}

####
# FormatDuration
#
# Formats a time duration into a human-readable format.
#
proc ::alcoholicz::FormatDuration {seconds} {
    if {$seconds < 0} {
        set seconds [expr {-$seconds}]
    }
    set duration [list]
    foreach div {31536000 604800 86400 3600 60 1} unit {y w d h m s} {
        set num [expr {$seconds / $div}]
        if {$num > 0} {lappend duration "[b]$num[b]$unit"}
        set seconds [expr {$seconds % $div}]
    }
    if {[llength $duration]} {return [join $duration]} else {return "[b]0[b]s"}
}

####
# FormatDurationLong
#
# Formats a time duration into a human-readable format.
#
proc ::alcoholicz::FormatDurationLong {seconds} {
    if {$seconds < 0} {
        set seconds [expr {-$seconds}]
    }
    set duration [list]
    foreach div {31536000 604800 86400 3600 60 1} unit {year week day hour min sec} {
        set num [expr {$seconds / $div}]
        if {$num > 1} {
            lappend duration "[b]$num[b] ${unit}s"
        } elseif {$num == 1} {
            lappend duration "[b]$num[b] $unit"
        }
        set seconds [expr {$seconds % $div}]
    }
    if {[llength $duration]} {return [join $duration {, }]} else {return "[b]0[b] secs"}
}

####
# FormatSize
#
# Formats a value in kilobytes into a human-readable amount.
#
proc ::alcoholicz::FormatSize {size} {
    variable format
    variable sizeDivisor
    foreach unit {sizeKilo sizeMega sizeGiga sizeTera} {
        if {abs($size) < $sizeDivisor} {break}
        set size [expr {double($size) / double($sizeDivisor)}]
    }
    return [VarReplace $format($unit) "size:n" $size]
}

####
# FormatSpeed
#
# Formats a value in kilobytes per second into a human-readable speed.
#
proc ::alcoholicz::FormatSpeed {speed {seconds 0}} {
    variable format
    variable speedDivisor
    if {$seconds > 0} {set speed [expr {double($speed) / $seconds}]}
    foreach unit {speedKilo speedMega speedGiga} {
        if {abs($speed) < $speedDivisor} {break}
        set speed [expr {double($speed) / double($speedDivisor)}]
    }
    return [VarReplace $format($unit) "speed:n" $speed]
}
