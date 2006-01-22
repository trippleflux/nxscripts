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
#   Implements a module to interact with ioA's log files.
#

namespace eval ::alcoholicz::IoA {
    namespace import -force ::alcoholicz::*
    namespace import -force ::config::*
    namespace import -force ::getopt::*
}

####
# OpenFile
#
# Open a log file for reading.
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
# ParseTime
#
# Parse a time stamp value in the form of "MMDD-hh:mm".
#
proc ::alcoholicz::IoA::ParseTime {value} {
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
    SendTargetTheme $target nukesHead

    # Read nukes log file.
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
proc ::alcoholicz::IoA::OneLines {command target user host handle channel argv} {
    variable onelinesFile

    # Parse command options.
    set option(limit) -1
    if {[catch {set pattern [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    set limit [GetResultLimit $option(limit)]
    SendTargetTheme $target oneLinesHead

    # Read one-lines log file.
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
proc ::alcoholicz::IoA::Requests {command target user host handle channel argv} {
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
    SendTargetTheme $target searchHead [list $pattern]

    # Read search log file.
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
    SendTargetTheme $target unnukesHead

    # Read unnukes log file.
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
proc ::alcoholicz::IoA::Load {firstLoad} {
    variable utcTime
    variable nukesFile
    variable onelinesFile
    variable requestsFile
    variable searchFile
    variable unnukesFile
    upvar ::alcoholicz::configHandle configHandle

    # Open ioA's configuration file.
    set ioaFile [ConfigGet $configHandle Module::IoA configFile]
    if {![file isfile $ioaFile]} {
        error "the file \"$ioaFile\" does not exist"
    }
    set ioaHandle [ConfigOpen $ioaFile]
    ConfigRead $ioaHandle

    foreach {varName section key} {
        localTime    General Use_Local_Time_Instead_of_UTC
        nukesFile    Nuke    Nuke_Log_File
        onelinesFile Oneline Oneline_File
        requestsFile Request Request_File
        searchFile   Search  Search_Log_File
        unnukesFile  Unnuke  UnNuke_Log_File
    } {
        set value [ConfigGet $ioaHandle $section $key]
        set $varName [string trim $value " \t\""]
    }
    ConfigClose $ioaHandle

    # Check if ioA logs time stamps in UTC time.
    set utcTime [expr {![IsTrue $localTime]}]

    # Create channel commands.
    if {[ConfigExists $configHandle Module::IoA cmdPrefix]} {
        set prefix [ConfigGet $configHandle Module::IoA cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    CmdCreate channel nukes    [namespace current]::Nukes \
        -category "Data"  -args "\[-limit <num>\] \[pattern\]" \
        -prefix   $prefix -desc "Display recent nukes."

    CmdCreate channel onel     [namespace current]::OneLines \
        -category "General" -args "\[-limit <num>\]" \
        -prefix   $prefix   -desc "Display recent one-lines."

    CmdCreate channel requests [namespace current]::Requests \
        -category "General" -desc "Display current requests." -prefix $prefix

    CmdCreate channel search   [namespace current]::Search \
        -category "Data"  -args "\[-limit <num>\] <pattern>" \
        -prefix   $prefix -desc "Search for a release."

    CmdCreate channel unnukes  [namespace current]::Unnukes \
        -category "Data"  -args "\[-limit <num>\] \[pattern\]" \
        -prefix   $prefix -desc "Display recent unnukes."
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::IoA::Unload {} {
}
