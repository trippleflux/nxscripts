#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   ioA Module
#
# Author:
#   neoxed (neoxed@gmail.com) Dec 17, 2005
#
# Abstract:
#   Implements a module to interact with ioA's log files.
#

namespace eval ::Bot::Mod::IoA {
    if {![info exists [namespace current]::cmdTokens]} {
        variable cmdTokens [list]
    }
    namespace import -force ::Bot::*
}

####
# OpenFile
#
# Open a log file for reading.
#
proc ::Bot::Mod::IoA::OpenFile {filePath handleVar} {
    upvar $handleVar handle
    if {[catch {set handle [open $filePath]} message]} {
        LogError ModIoA $message
        return 0
    }
    return 1
}

####
# ParseTime
#
# Parse a time stamp value in the form of "MMDD-hh:mm".
#
proc ::Bot::Mod::IoA::ParseTime {value} {
    variable utcTime

    foreach {date time} [split $value "-"] {break}
    set month [string range $date 0 1]
    set day [string range $date 2 3]
    return [clock scan "$month/$day $time" -gmt $utcTime]
}

####
# Nukes
#
# Display recent nukes, command: !nukes [-limit <num>] [pattern].
#
proc ::Bot::Mod::IoA::Nukes {target user host channel argv} {
    variable nukesFile

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target nukesHead
    set data [list]
    if {$limit > 0 && [OpenFile $nukesFile handle]} {
        set range [expr {$limit - 1}]

        while {![eof $handle]} {
            # Format: <release>|<multi>x|<size>|<nuker>|<nukee>|<MMDD-hh:mm>|<reason>
            set line [split [gets $handle] "|"]
            if {[llength $line] == 7 && ($pattern eq "" || [string match -nocase $pattern [lindex $line 0]])} {
                set data [lrange [linsert $data 0 $line] 0 $range]
            }
        }
        close $handle
    }

    # Display results.
    if {[llength $data]} {
        set count 0
        foreach item $data {
            incr count
            foreach {release multi size nuker nukee time reason} $item {break}
            set multi [string trimright $multi "x"]
            set time [ParseTime $time]
            set age [expr {[clock seconds] - $time}]

            SendTargetTheme $target nukesBody [list $count \
                $nuker $release $time $multi $reason $size $age]
        }
    } else {
        SendTargetTheme $target nukesNone
    }
    SendTargetTheme $target nukesFoot
}

####
# OneLines
#
# Display recent one-lines, command: !onel [-limit <num>].
#
proc ::Bot::Mod::IoA::OneLines {target user host channel argv} {
    variable onelinesFile

    # Parse command options.
    set option(limit) -1
    GetOpt::Parse $argv {{limit integer}} option
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target oneLinesHead
    set data [list]
    if {$limit > 0 && [OpenFile $onelinesFile handle]} {
        set range [expr {$limit - 1}]

        while {![eof $handle]} {
            if {[gets $handle line] > 1} {
                set data [lrange [linsert $data 0 $line] 0 $range]
            }
        }
        close $handle
    }

    # Display results.
    if {[llength $data]} {
        set count 0
        foreach item $data {
            incr count
            SendTargetTheme $target oneLinesBody [list $count $item]
        }
    } else {
        SendTargetTheme $target oneLinesNone
    }
    SendTargetTheme $target oneLinesFoot
}

####
# Requests
#
# Display current requests, command: !requests.
#
proc ::Bot::Mod::IoA::Requests {target user host channel argv} {
    variable requestsFile
    SendTargetTheme $target requestsHead

    # Read requests log file.
    set count 0
    if {[OpenFile $requestsFile handle]} {
        while {![eof $handle]} {
            if {[gets $handle line] > 1} {
                incr count
                SendTargetTheme $target requestsBody [list $count $line]
            }
        }
        close $handle
    }

    if {!$count} {SendTargetTheme $target requestsNone}
    SendTargetTheme $target requestsFoot
}

####
# Search
#
# Search for a release, command: !search [-limit <num>] <pattern>.
#
proc ::Bot::Mod::IoA::Search {target user host channel argv} {
    variable searchFile
    variable searchSort

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target searchHead [list $pattern]
    set data [list]
    if {$limit > 0 && [OpenFile $searchFile handle]} {
        set range [expr {$limit - 1}]

        while {![eof $handle]} {
            # Format: <time>|<path>|<release>
            set line [split [gets $handle] "|"]
            if {[llength $line] == 3 && [string match -nocase $pattern [lindex $line 2]]} {
                set data [lrange [linsert $data 0 $line] 0 $range]
            }
        }
        close $handle
    }

    # Display results.
    if {[llength $data]} {
        set count 0
        foreach item $data {
            incr count
            foreach {time path release} $item {break}
            # Convert a 64bit FILETIME value into a UNIX epoch value.
            set time [expr {(wide($time) - 116444736000000000) / 10000000}]
            set age [expr {[clock seconds] - $time}]

            SendTargetTheme $target searchBody [list $count \
                [file join $path $release] $time $age]
        }
    } else {
        SendTargetTheme $target searchNone [list $pattern]
    }
    SendTargetTheme $target searchFoot
}

####
# Unnukes
#
# Display recent unnukes, command: !unnukes [-limit <num>] [pattern].
#
proc ::Bot::Mod::IoA::Unnukes {target user host channel argv} {
    variable unnukesFile

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target unnukesHead
    set data [list]
    if {$limit > 0 && [OpenFile $unnukesFile handle]} {
        set range [expr {$limit - 1}]

        while {![eof $handle]} {
            # Format: <release>|<multi>x|<size>|<unnuker>|<nukee>|<MMDD-hh:mm>|<reason>
            set line [split [gets $handle] "|"]
            if {[llength $line] == 7 && ($pattern eq "" || [string match -nocase $pattern [lindex $line 0]])} {
                set data [lrange [linsert $data 0 $line] 0 $range]
            }
        }
        close $handle
    }

    # Display results.
    if {[llength $data]} {
        set count 0
        foreach item $data {
            incr count
            foreach {release multi size unnuker nukee time reason} $item {break}
            set multi [string trimright $multi "x"]
            set time [ParseTime $time]
            set age [expr {[clock seconds] - $time}]

            SendTargetTheme $target unnukesBody [list $count \
                $unnuker $release $time $multi $reason $size $age]
        }
    } else {
        SendTargetTheme $target unnukesNone
    }
    SendTargetTheme $target unnukesFoot
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::IoA::Load {firstLoad} {
    variable cmdTokens
    variable utcTime
    variable nukesFile
    variable onelinesFile
    variable requestsFile
    variable searchFile
    variable unnukesFile
    upvar ::Bot::configHandle configHandle

    # Open ioA's configuration file.
    set ioaFile [Config::Get $configHandle Module::IoA configFile]
    if {![file isfile $ioaFile]} {
        error "the file \"$ioaFile\" does not exist"
    }
    set ioaHandle [Config::Open $ioaFile]
    Config::Read $ioaHandle

    foreach {varName section key} {
        localTime    General Use_Local_Time_Instead_of_UTC
        nukesFile    Nuke    Nuke_Log_File
        onelinesFile Oneline Oneline_File
        requestsFile Request Request_File
        searchFile   Search  Search_Log_File
        unnukesFile  Unnuke  UnNuke_Log_File
    } {
        set value [Config::Get $ioaHandle $section $key]
        set $varName [string trim $value " \t\""]
    }
    Config::Close $ioaHandle

    # Check if ioA logs time stamps in UTC time.
    set utcTime [expr {![IsTrue $localTime]}]

    # Create channel commands.
    set cmdTokens [list]

    lappend cmdTokens [CmdCreate channel nukes [namespace current]::Nukes \
        -args "\[-limit <num>\] \[pattern\]" \
        -category "Data" -desc "Display recent nukes."]

    lappend cmdTokens [CmdCreate channel onel [namespace current]::OneLines \
        -args "\[-limit <num>\]" \
        -category "General" -desc "Display recent one-lines."]

    lappend cmdTokens [CmdCreate channel requests [namespace current]::Requests \
        -category "General" -desc "Display current requests."]

    lappend cmdTokens [CmdCreate channel search [namespace current]::Search \
        -args "\[-limit <num>\] <pattern>" \
        -category "Data" -desc "Search for a release."]

    lappend cmdTokens [CmdCreate channel unnukes [namespace current]::Unnukes \
        -args "\[-limit <num>\] \[pattern\]" \
        -category "Data" -desc "Display recent unnukes."]
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::IoA::Unload {} {
    variable cmdTokens
    foreach token $cmdTokens {
        CmdRemoveByToken $token
    }
}
