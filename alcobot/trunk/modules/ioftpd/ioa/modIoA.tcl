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
# Nukes
#
# Display recent nukes, command: !nukes [-limit <num>] [pattern].
#
proc ::alcoholicz::IoA::Nukes {command target user host handle channel argv} {
    variable nukesFile
    return
}

####
# OneLines
#
# Display recent one-lines, command: !onel.
#
proc ::alcoholicz::IoA::OneLines {command target user host handle channel argv} {
    variable onelinesFile
    return
}

####
# Requests
#
# Display current requests, command: !requests.
#
proc ::alcoholicz::IoA::Requests {command target user host handle channel argv} {
    variable requestsFile
    return
}

####
# Search
#
# Search for a release, command: !search [-limit <num>] <pattern>.
#
proc ::alcoholicz::IoA::Search {command target user host handle channel argv} {
    variable searchFile
    variable searchLength
    variable searchSort
    return
}

####
# Unnukes
#
# Display recent unnukes, command: !unnukes [-limit <num>] [pattern].
#
proc ::alcoholicz::IoA::Unnukes {command target user host handle channel argv} {
    variable unnukesFile
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
        searchLength Search  Search_Minimum_Length
        searchSort   Search  Search_Sort_Order
        unnukesFile  Unnuke  UnNuke_Log_File
    } {
        set value [ConfigGet $ioaHandle $section $key]
        variable $varName [string trim $value " \t\""]
    }
    ConfigClose $ioaHandle

    # Create related commands.
    CmdCreate channel nukes     [namespace current]::Nukes \
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
