#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   ioA Module
#
# Author:
#   neoxed (neoxed@gmail.com) Dec 17, 2005
#
# Abstract:
#   Implements a module to interact with ioA's data files.
#

namespace eval ::alcoholicz::IoA {
    namespace import -force ::alcoholicz::*
}

####
# OpenFile
#
# Open a data file for reading.
#
proc ::alcoholicz::IoA::OpenFile {filePath handleVar} {
    upvar $handleVar handle
    if {[catch {set handle [open $filePath]} message]} {
        LogError ModIoA $message
        return 0
    }
    return 1
}

####
# Nukes
#
# Display recent nukes, command: !nukes [-limit <num>] [pattern].
#
proc ::alcoholicz::IoA::Nukes {command target user host handle channel argv} {
    variable nukesFile

    # Parse command options.
    set option(limit) -1
    if {[catch {set pattern [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    set limit [GetResultLimit $option(limit)]
    set pattern [join $pattern]

    # Read nukes data file.
    if {[OpenFile $nukesFile handle]} {
        while {![eof $handle]} {
            # Format: <release>|<multi>x|<amount>|<nuker>|<nukee>|MMDD-HH:SS|<reason>
            set line [split [gets $handle] "|"]
            if {[llength $line] == 7 && ($pattern eq "" || [string match -nocase $pattern [lindex $line 0]])} {
                # TODO
            }
        }
        close $handle
    }
    return
}

####
# OneLines
#
# Display recent one-lines, command: !onel.
#
proc ::alcoholicz::IoA::OneLines {command target user host handle channel argv} {
    variable onelinesFile

    # Read one-lines data file.
    if {[OpenFile $onelinesFile handle]} {
        while {![eof $handle]} {
            if {[gets $handle line] > 1} {
                # TODO
            }
        }
        close $handle
    }
    return
}

####
# Requests
#
# Display current requests, command: !requests.
#
proc ::alcoholicz::IoA::Requests {command target user host handle channel argv} {
    variable requestsFile

    # Read requests data file.
    if {[OpenFile $requestsFile handle]} {
        while {![eof $handle]} {
            if {[gets $handle line] > 1} {
                # TODO
            }
        }
        close $handle
    }
    return
}

####
# Search
#
# Search for a release, command: !search [-limit <num>] <pattern>.
#
proc ::alcoholicz::IoA::Search {command target user host handle channel argv} {
    variable searchFile
    variable searchSort

    # Parse command options.
    set option(limit) -1
    if {[catch {set pattern [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    if {[set pattern [join $pattern]] eq ""} {
        CmdSendHelp $channel channel $command "you must specify a pattern"
        return
    }
    set limit [GetResultLimit $option(limit)]

    # Read search data file.
    if {[OpenFile $searchFile handle]} {
        while {![eof $handle]} {
            # Format: <time>|<path>|<release>
            set line [split [gets $handle] "|"]
            if {[llength $line] == 3 && [string match -nocase $pattern [lindex $line 2]]} {
                # TODO
            }
        }
        close $handle
    }
    return
}

####
# Unnukes
#
# Display recent unnukes, command: !unnukes [-limit <num>] [pattern].
#
proc ::alcoholicz::IoA::Unnukes {command target user host handle channel argv} {
    variable unnukesFile

    # Parse command options.
    set option(limit) -1
    if {[catch {set pattern [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    set limit [GetResultLimit $option(limit)]
    set pattern [join $pattern]

    # Read unnukes data file.
    if {[OpenFile $unnukesFile handle]} {
        while {![eof $handle]} {
            # Format: <release>|<multi>x|<amount>|<nuker>|<nukee>|MMDD-HH:SS|<reason>
            set line [split [gets $handle] "|"]
            if {[llength $line] == 7 && ($pattern eq "" || [string match -nocase $pattern [lindex $line 0]])} {
                # TODO
            }
        }
        close $handle
    }
    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::IoA::Load {firstLoad} {
    upvar ::alcoholicz::configHandle configHandle

    # Open ioA's configuration file.
    set ioaFile [ConfigGet $configHandle Module::IoA configFile]
    if {![file isfile $ioaFile]} {
        error "the file \"$ioaFile\" does not exist"
    }
    set ioaHandle [ConfigOpen $ioaFile]
    ConfigRead $ioaHandle

    foreach {varName section key} {
        nukesFile    Nuke    Nuke_Log_File
        onelinesFile Oneline Oneline_File
        requestsFile Request Request_File
        searchFile   Search  Search_Log_File
        searchSort   Search  Search_Sort_Order
        unnukesFile  Unnuke  UnNuke_Log_File
    } {
        set value [ConfigGet $ioaHandle $section $key]
        variable $varName [string trim $value " \t\""]
    }
    ConfigClose $ioaHandle

    # Create related commands.
    CmdCreate channel nukes    [namespace current]::Nukes \
        -category "Data"  -args "\[-limit <num>\] \[pattern\]" \
        -prefix   $prefix -desc "Display recent nukes."

    CmdCreate channel onel     [namespace current]::OneLines \
        -category "General" -desc "Display recent one-lines." -prefix $prefix

    CmdCreate channel requests [namespace current]::Requests \
        -category "General" -desc "Display current requests." -prefix $prefix

    CmdCreate channel search   [namespace current]::Search \
        -category "Data"  -args "\[-limit <num>\] <pattern>" \
        -prefix   $prefix -desc "Search for a release."

    CmdCreate channel unnukes  [namespace current]::Unnukes \
        -category "Data"  -args "\[-limit <num>\] \[pattern\]" \
        -prefix   $prefix -desc "Display recent unnukes."

    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::IoA::Unload {} {
    return
}
