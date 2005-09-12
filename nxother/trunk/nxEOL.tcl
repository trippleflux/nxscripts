#
# nxEOL - End of line translation.
# Copyright (c) 2005 neoxed
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements a script to translate Mac (CR) and UNIX (LF) style
#   end-of-line characters to Windows (CR/LF).
#
# Installation:
#   1. Copy the nxEOL.tcl file to x:\ioFTPD\scripts\.
#   2. Configure the script.
#   3. Add the following to your ioFTPD.ini:
#
#   [Events]
#   OnUploadComplete = TCL ..\scripts\nxEOL.tcl
#
#   4. Rehash or restart ioFTPD for the changes to take effect.
#

namespace eval ::nxEOL {
    # Files masks to perform EOL translation on.
    variable fileMasks  {*.diz *.nfo *.m3u *.sfv}

    # Paths to exempt from EOL translation.
    variable exemptPaths {/STAFF/*}
}

proc ::nxEOL::ArgList {argv} {
    set argList [list]
    set length [string length $argv]

    for {set index 0} {$index < $length} {incr index} {
        # Ignore leading white-space.
        while {[string is space -strict [string index $argv $index]]} {incr index}
        if {$index >= $length} {break}

        if {[string index $argv $index] eq "\""} {
            # Find the next quote character.
            set startIndex [incr index]
            while {[string index $argv $index] ne "\"" && $index < $length} {incr index}
        } else {
            # Find the next white-space character.
            set startIndex $index
            while {![string is space -strict [string index $argv $index]] && $index < $length} {incr index}
        }
        lappend argList [string range $argv $startIndex [expr {$index - 1}]]
    }
    return $argList
}

proc ::nxEOL::ErrorLog {type message} {
    set filePath "../logs/nxError.log"
    if {![catch {set handle [open $filePath a]} error]} {
        set now [clock format [clock seconds] -format "%m-%d-%Y %H:%M:%S"]
        puts $handle "$now - [format %-12s $type] : $message"
        close $handle
    } else {iputs $error}
}

proc ::nxEOL::ListMatch {patternList string} {
    foreach pattern $patternList {
        if {[string match -nocase $pattern $string]} {return 1}
    }
    return 0
}

proc ::nxEOL::TranslateFile {filePath} {
    if {[catch {
        set handle [open $filePath r]
        set data [read $handle]
        close $handle

        # Rewrite file.
        set handle [open $filePath w]
        fconfigure $handle -translation {auto crlf}
        puts -nonewline $handle $data
        close $handle
    } message]} {
        ErrorLog nxEOL $message
    }
    return
}

proc ::nxEOL::Main {argv} {
    variable fileMasks
    variable exemptPaths

    set argList [ArgList $argv]
    set filePath [lindex $argList 0]
    set virtualPath [file dirname [lindex $argList 2]]

    if {[ListMatch $fileMasks $filePath] && ![ListMatch $exemptPaths $virtualPath]} {
        TranslateFile $filePath
    }

    return 0
}

::nxEOL::Main [expr {[info exists args] ? $args : ""}]
