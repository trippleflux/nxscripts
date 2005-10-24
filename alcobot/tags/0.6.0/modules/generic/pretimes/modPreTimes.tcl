#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Pre Times Module
#
# Author:
#   neoxed (neoxed@gmail.com) Sep 18, 2005
#
# Abstract:
#   Implements a module to display and search for release pre times.
#

namespace eval ::alcoholicz::PreTimes {
    if {![info exists dataSource]} {
        variable dataSource ""
        variable defLimit 5
    }
    namespace import -force ::alcoholicz::*
}

####
# DbConnect
#
# Connect to the ODBC data source.
#
proc ::alcoholicz::PreTimes::DbConnect {} {
    variable dataSource

    # If the TclODBC 'object' already exists, return.
    if {[llength [info commands [namespace current]::db]]} {
        return 1
    }

    if {[catch {database connect [namespace current]::db "DSN=$dataSource"} message]} {
        LogError ModPreTimes "Unable to connect to database \"$dataSource\": [lindex $message 2] ([lindex $message 0])"
        return 0
    }
    db set timeout 0

    # Check if the required table exists.
    if {![llength [db tables "pretimes"]]} {
        LogError ModInvite "The database \"$dataSource\" is missing the \"pretimes\" table."
        db disconnect
        return 0
    }
    return 1
}

####
# LogEvent
#
# Handle NEWDIR and PRE log events.
#
proc ::alcoholicz::PreTimes::LogEvent {event destSection pathSection path data} {
    variable defLimit
    upvar ::alcoholicz::variables variables

    # Record the time now for accuracy.
    set now [clock seconds]

    # Ignore sub-directories.
    if {[IsSubDir $path] || ![DbConnect]} {return 1}
    set release [SqlEscape [file tail $path]]

    if {$event eq "NEWDIR"} {
        set flags [GetFlagsFromSection $destSection]
        if {[FlagIsDisabled $flags "pretime"]} {return 1}

        set result [db "SELECT pretime FROM pretimes WHERE release='$release' LIMIT 1"]
        if {[llength $result]} {
            set preTime [lindex $result 0 0]

            # Check for a section time limit.
            if {![FlagGetValue $flags "pretime" limit]} {
                set limit $defLimit
            }
            set limit [expr {$limit * 60}]

            set age [expr {$now - $preTime}]
            if {$age > $limit} {
                set event "PRELATE"
            } else {
                set event "PRENEW"
            }
            SendSectionTheme $destSection $event [lappend data $preTime $age $limit]
            return 0
        }
    } elseif {$event eq "PRE" || $event eq "PRE-MP3"} {
        set section [SqlEscape $pathSection]
        set files 0; set kiloBytes 0; set disks 0

        # Retrieve the files, size, and disk count from the log data.
        if {[set index [lsearch -exact $variables($event) "files:n"]] != -1} {
            set files [SqlEscape [lindex $data $index]]
        }
        if {[set index [lsearch -exact $variables($event) "size:k"]] != -1} {
            set kiloBytes [SqlEscape [lindex $data $index]]
        }
        if {[set index [lsearch -exact $variables($event) "disks:n"]] != -1} {
            set disks [SqlEscape [lindex $data $index]]
        }

        if {[catch {db "INSERT INTO pretimes(pretime,section,release,files,kbytes,disks) \
                VALUES('$now','$section','$release','$files','$kiloBytes','$disks')"} message]} {
            LogError ModPreTime $message
        }
    } else {
        LogError ModPreTime "Unknown log event \"$event\"."
    }
    return 1
}

####
# Search
#
# Search for a release, command: !pre [-limit <num>] [-section <name>] <pattern>.
#
proc ::alcoholicz::PreTimes::Search {command target user host handle channel argv} {
    # Parse command options.
    set option(limit) -1
    if {[catch {set pattern [GetOptions $argv {{limit integer} {section arg}} option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    if {[set pattern [join $pattern]] eq ""} {
        CmdSendHelp $channel channel $command "you must specify a pattern"
        return
    }
    set option(limit) [GetResultLimit $option(limit)]

    # Build SQL query.
    set query "SELECT * FROM pretimes WHERE "
    if {[info exists option(section)]} {
        set section [SqlEscape $option(section)]
        append query "UPPER(section)=UPPER('$section') AND "
    }
    append query "release LIKE '[SqlGetPattern $pattern]' ORDER BY pretime DESC LIMIT $option(limit)"

    set count 0; set multi 0
    if {[DbConnect]} {
        set result [db $query]

        # If there's more than one row, send the output to
        # $target. Otherwise the output is sent to the channel.
        if {[llength $result] > 1} {
            SendTargetTheme $target preHead [list $pattern]
            set multi 1
        } else {
            set target "PRIVMSG $channel"
        }

        # Display results.
        foreach row $result {
            incr count
            foreach {id preTime section release files size disks nuked nukeTime reason} $row {break}
            set age [expr {[clock seconds] - $preTime}]

            if {$nuked} {
                SendTargetTheme $target preNuke [list $preTime $section \
                    $release $files $size $disks $age $nukeTime $reason $count]
            } else {
                SendTargetTheme $target preBody [list $preTime $section \
                    $release $files $size $disks $age $count]
            }
        }
    }

    if {!$count} {
        # Always send this message to the channel.
        SendTargetTheme "PRIVMSG $channel" preNone [list $pattern]
    }
    if {$multi} {SendTargetTheme $target preFoot}
    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::PreTimes::Load {firstLoad} {
    variable defLimit
    variable dataSource
    upvar ::alcoholicz::configHandle configHandle

    if {$firstLoad} {
        package require tclodbc
    }

    foreach option {addOnPre dataSource defLimit searchPres showOnNew} {
        set $option [ConfigGet $configHandle Module::PreTimes $option]
    }
    if {![string is digit -strict $defLimit]} {
        set defLimit 0
    }

    # Register event callbacks.
    if {[IsTrue $addOnPre]} {
        ScriptRegister   pre PRE     [namespace current]::LogEvent True
        ScriptRegister   pre PRE-MP3 [namespace current]::LogEvent True
    } else {
        ScriptUnregister pre PRE     [namespace current]::LogEvent
        ScriptUnregister pre PRE-MP3 [namespace current]::LogEvent
    }
    if {[IsTrue $showOnNew]} {
        ScriptRegister   pre NEWDIR [namespace current]::LogEvent True
    } else {
        ScriptUnregister pre NEWDIR [namespace current]::LogEvent
    }

    # Create channel commands.
    if {[IsTrue $searchPres]} {
        if {[ConfigExists $configHandle Module::PreTimes cmdPrefix]} {
            set prefix [ConfigGet $configHandle Module::PreTimes cmdPrefix]
        } else {
            set prefix $::alcoholicz::cmdPrefix
        }

        CmdCreate channel pre [namespace current]::Search \
            -category "General" -args "\[-limit <num>\] \[-section <name>\] <pattern>" \
            -prefix   $prefix   -desc "Search pre time database."
    }

    if {!$firstLoad} {
        # Reconnect to the data source on reload.
        catch {db disconnect}
    }
    DbConnect
    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::PreTimes::Unload {} {
    # Remove event callbacks.
    ScriptUnregister pre PRE     [namespace current]::LogEvent
    ScriptUnregister pre PRE-MP3 [namespace current]::LogEvent
    ScriptUnregister pre NEWDIR  [namespace current]::LogEvent

    catch {db disconnect}
    return
}
