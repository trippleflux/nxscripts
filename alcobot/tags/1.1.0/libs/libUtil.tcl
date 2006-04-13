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

namespace eval ::Bot {
    namespace export GetResultLimit GlobEscape \
        ListConvert ListExists ListParse ListRemove \
        IsSubDir PathParse PathParseSection PathStrip \
        PermCheck PermMatchFlags \
        SqlEscape SqlGetPattern SqlToLike
}

################################################################################
# Utilities                                                                    #
################################################################################

####
# IsTrue
# IsFalse
#
# Boolean value checks.
#
interp alias {} IsTrue {} string is true -strict
interp alias {} IsFalse {} string is false -strict

####
# throw
#
# Throw an error with the given error-code and message.
#
proc ::throw {code {message ""}} {error $message "" $code}

####
# GetResultLimit
#
# Checks the number of requested results.
#
proc ::Bot::GetResultLimit {results} {
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
# GlobEscape
#
# Escapes a glob pattern (*, ?, []).
#
proc ::Bot::GlobEscape {string} {
    return [string map {* \\* ? \\? \\ \\\\ \[ \\\[ \] \\\]} $string]
}

################################################################################
# Lists                                                                        #
################################################################################

####
# ListConvert
#
# Convert a Tcl list into a human-readable list.
#
proc ::Bot::ListConvert {list {word "and"}} {
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
# ListExists
#
# Searches a list for a given element (case-insensitively). The -nocase switch
# was not added to lsearch until Tcl 8.5, so this function is provided for
# backwards compatibility with Tcl 8.4.
#
proc ::Bot::ListExists {list element} {
    foreach entry $list {
        if {[string equal -nocase $entry $element]} {return 1}
    }
    return 0
}

####
# ListParse
#
# Parses an argument string into a list, respecting quoted text segments.
#
proc ::Bot::ListParse {argStr} {
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
# ListRemove
#
# Removes an element from a list.
#
proc ::Bot::ListRemove {list element} {
    set index [lsearch -exact $list $element]
    return [lreplace $list $index $index]
}

################################################################################
# Path Parsing                                                                 #
################################################################################

####
# IsSubDir
#
# Check if the path's base directory is a known release sub-directory.
#
proc ::Bot::IsSubDir {path} {
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
proc ::Bot::PathStrip {path} {
    regsub -all -- {[\\/]+} $path "/" path
    return [string trim $path "/"]
}

####
# PathParse
#
# Parses elements from a directory path, with subdirectory support.
#
proc ::Bot::PathParse {fullPath {basePath ""}} {
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
proc ::Bot::PathParseSection {fullPath useSection} {
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
proc ::Bot::PermCheck {rightsList userName groupList flags} {
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
proc ::Bot::PermMatchFlags {currentFlags needFlags} {
    set currentFlags [split $currentFlags {}]
    foreach flag [split $needFlags {}] {
        if {[lsearch -glob $currentFlags $flag] != -1} {return 1}
    }
    return 0
}

################################################################################
# SQL Procedures                                                               #
################################################################################

####
# SqlEscape
#
# Escape SQL quote characters with a backslash.
#
proc ::Bot::SqlEscape {string} {
    return [string map {\\ \\\\ ` \\` ' \\' \" \\\"} $string]
}

####
# SqlGetPattern
#
# Prepend, append, and replace all spaces with wildcards then convert
# standard wildcard characters to SQL LIKE characters.
#
proc ::Bot::SqlGetPattern {pattern} {
    set pattern "*$pattern*"
    regsub -all -- {[\s\*]+} $pattern "*" pattern
    return [SqlToLike $pattern]
}

####
# SqlToLike
#
# Convert standard wildcard characters to SQL LIKE characters.
#
proc ::Bot::SqlToLike {pattern} {
    set pattern [SqlEscape $pattern]
    return [string map {* % ? _} [string map {% \\% _ \\_} $pattern]]
}
