#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
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
        VarReplace VarReplaceBase VarReplaceCommon
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

################################################################################
# Variable Replacement                                                         #
################################################################################

####
# VarFormat
#
# Formats a given variable according to its substitution type.
#
proc ::alcoholicz::VarFormat {valueVar name type width precision} {
    variable sizeDivisor
    upvar $valueVar value

    # Type of variable substitution. Error checking is not performed
    # for all types because of the performance implications in doing so.
    switch -- $type {
        b {set value [FormatSize [expr {double($value) / double($sizeDivisor)}]]}
        d {set value [FormatDuration $value]}
        e {set value [subst -nocommands -novariables $value]}
        k {set value [FormatSize $value]}
        m {set value [FormatSize [expr {double($value) * double($sizeDivisor)}]]}
        s {set value [FormatSpeed $value]}
        p -
        P -
        t {return 0}
        n {
            # Integer or floating point.
            if {[string is integer -strict $value]} {
                set value [format "%${width}d" $value]
                return 1
            } elseif {[string is double -strict $value]} {
                set value [format "%${width}.${precision}f" $value]
                return 1
            }
            LogWarning VarFormat "Invalid numerical value \"$value\" for \"$name\"."
        }
        z {
            variable replace
            if {![string length $value] && [info exists replace($name)]} {
                set value $replace($name)
            }
        }
        default {LogWarning VarFormat "Unknown substitution type \"$type\" for \"$name\"."}
    }

    set value [format "%${width}.${precision}s" $value]
    return 1
}

####
# VarReplace
#
# Substitute a list of variables and values in a given string.
#
proc ::alcoholicz::VarReplace {input varList valueList} {
    set inputLength [string length $input]
    set output ""

    set index 0
    foreach varName $varList value $valueList {
        if {[string match {*:[Ppt]} $varName]} {
            set type [string index $varName end]
            set varName [string range $varName 0 end-2]

            if {$type eq "P" || $type eq "p"} {
                lappend varList ${varName}Dir:z ${varName}Full:z ${varName}Name:z ${varName}Path:z
                eval lappend valueList [PathParseSection $value [string equal $type "P"]]
            } elseif {$type eq "t"} {
                lappend varList ${varName}Date:z ${varName}Time:z
                lappend valueList [FormatDate $value] [FormatTime $value]
            }
        } elseif {[llength $varName] > 1} {
            variable theme
            variable variables

            set value [ArgsToList $value]
            foreach {varName loopName} $varName {
                set joinName "${loopName}_JOIN"
                if {![info exists theme($loopName)] || ![info exists theme($joinName)]} {
                    LogError VarReplace "Missing theme definition for \"$loopName\" or \"$joinName\"."
                    continue
                }

                set data [list]
                set varCount [llength $variables($loopName)]
                set valueCount [llength $value]
                for {set i 0} {$i < $valueCount} {incr i $varCount} {
                    set values [lrange $value $i [expr {$i + $varCount - 1}]]
                    lappend data [VarReplace $theme($loopName) $variables($loopName) $values]
                }
                lappend varList $varName
                lappend valueList [join $data $theme($joinName)]
            }
        } else {
            incr index; continue
        }

        # Remove the original variable and its value.
        set varList [lreplace $varList $index $index]
        set valueList [lreplace $valueList $index $index]
    }

    for {set inputIndex 0} {$inputIndex < $inputLength} {incr inputIndex} {
        if {[string index $input $inputIndex] ne "%"} {
            append output [string index $input $inputIndex]
        } else {
            set startIndex $inputIndex

            # Find the width field (before dot).
            set beforeIndex [incr inputIndex]
            if {[string index $input $inputIndex] eq "-"} {
                # Ignore the negative sign if a number does not follow, e.g. %-(variable).
                if {[string is digit -strict [string index $input [incr inputIndex]]]} {
                    incr inputIndex
                } else {incr beforeIndex}
            }
            while {[string is digit -strict [string index $input $inputIndex]]} {incr inputIndex}
            if {$beforeIndex != $inputIndex} {
                set width [string range $input $beforeIndex [expr {$inputIndex - 1}]]
            } else {
                set width 0
            }

            # Find the precision field (after dot).
            if {[string index $input $inputIndex] eq "."} {
                set beforeIndex [incr inputIndex]
                # Ignore the negative sign, ex: %.-(variable).
                if {[string index $input $inputIndex] eq "-"} {incr beforeIndex; incr inputIndex}
                while {[string is digit -strict [string index $input $inputIndex]]} {incr inputIndex}
                if {$beforeIndex != $inputIndex} {
                    set precision [string range $input $beforeIndex [expr {$inputIndex - 1}]]
                } else {
                    set precision 0
                }
            } else {
                # Tcl's format function does not accept -1 for the precision field
                # like printf() does, so a reasonably large number will suffice.
                set precision 9999
            }

            # Find the variable name.
            if {[string index $input $inputIndex] eq "("} {
                set beforeIndex [incr inputIndex]
                while {[string index $input $inputIndex] ne ")" && $inputIndex <= $inputLength} {incr inputIndex}
                set varName [string range $input $beforeIndex [expr {$inputIndex - 1}]]
            } else {
                # Invalid variable format, an opening parenthesis is expected.
                append output [string range $input $startIndex $inputIndex]
                continue
            }

            # Variable names must be composed of alphanumeric
            # and/or connector characters (a-z, A-Z, 0-9, and _).
            if {[string is wordchar -strict $varName] && [set valueIndex [lsearch -glob $varList ${varName}:?]] != -1} {
                set value [lindex $valueList $valueIndex]
                set type [string index [lindex $varList $valueIndex] end]

                if {[VarFormat value $varName $type $width $precision]} {
                    append output $value; continue
                }
            }

            # Unknown variable name, the string will still appended to
            # notify the user of their mistake (most likely a typo).
            append output [string range $input $startIndex $inputIndex]
        }
    }
    return $output
}

####
# VarReplaceBase
#
# Replaces static content and control codes (i.e. bold, colour, and underline).
#
proc ::alcoholicz::VarReplaceBase {text {doPrefix 1}} {
    # Replace static variables.
    set vars {siteName:z siteTag:z}
    set values [list $::alcoholicz::siteName $::alcoholicz::siteTag]

    if {$doPrefix} {
        lappend vars prefix:z
        lappend values $::alcoholicz::format(prefix)
    }
    set text [VarReplace $text $vars $values]

    # Mapping for control code replacement.
    set map [list {[b]} [b] {[c]} [c] {[o]} [o] {[r]} [r] {[u]} [u]]
    return [subst -nocommands -novariables [string map $map $text]]
}

####
# VarReplaceCommon
#
# Replace dynamic content, such as the current date, time, and section colours.
#
proc ::alcoholicz::VarReplaceCommon {text section} {
    variable colours
    variable defaultSection

    set vars [list now:t]
    set values [list [clock seconds]]
    if {$section eq ""} {
        # If no section is specified, use the default section.
        set section $defaultSection
    } else {
        lappend vars "section:z"
        lappend values $section

        if {![info exists colours($section)]} {
            # If there are no colours available for the current
            # section, use the default section's colours.
            set section $defaultSection
        }
    }

    if {[info exists colours($section)]} {
        set text [string map $colours($section) $text]
    } else {
        LogDebug VarReplaceCommon "No section colours defined for \"$section\"."
    }
    return [VarReplace $text $vars $values]
}
