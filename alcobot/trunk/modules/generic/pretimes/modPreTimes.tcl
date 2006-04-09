#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
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

namespace eval ::Bot::Mod::PreTimes {
    if {![info exists [namespace current]::cmdToken]} {
        variable cmdToken ""
        variable dataSource ""
        variable defLimit 5
    }
    namespace import -force ::Bot::*
}

####
# DbConnect
#
# Connect to the ODBC data source.
#
proc ::Bot::Mod::PreTimes::DbConnect {} {
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
proc ::Bot::Mod::PreTimes::LogEvent {event destSection pathSection path data} {
    variable defLimit

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

            lappend data $preTime $age $limit
            SendSectionTheme $destSection Module::ReadLogs $event $data
            return 0
        }
    } elseif {$event eq "PRE" || $event eq "PRE-MP3"} {
        set section [SqlEscape $pathSection]
        set disks 0; set files 0; set kiloBytes 0

        # Retrieve the files, size, and disk count from the log data.
        set varList [VarGetEntry Module::ReadLogs $event]

        if {[set index [lsearch -exact $varList "disks:n"]] != -1} {
            set disks [lindex $data $index]
        }
        if {[set index [lsearch -exact $varList "files:n"]] != -1} {
            set files [lindex $data $index]
        }
        if {[set index [lsearch -exact $varList "size:k"]] != -1} {
            set kiloBytes [lindex $data $index]
        } elseif {[set index [lsearch -exact $varList "size:m"]] != -1} {
            set kiloBytes [expr {[lindex $data $index] * 1024.0}]
        }

        set disks [SqlEscape $disks]
        set files [SqlEscape $files]
        set kiloBytes [SqlEscape $kiloBytes]
        if {[catch {db "INSERT INTO pretimes (pretime, section, release, files, kbytes, disks) \
                VALUES('$now', '$section', '$release', '$files', '$kiloBytes', '$disks')"} message]} {
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
proc ::Bot::Mod::PreTimes::Search {target user host channel argv} {
    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer} {section arg}} option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    set limit [GetResultLimit $option(limit)]

    # Build SQL query.
    set query "SELECT * FROM pretimes WHERE "
    if {[info exists option(section)]} {
        set section [SqlEscape $option(section)]
        append query "UPPER(section)=UPPER('$section') AND "
    }
    append query "release LIKE '[SqlGetPattern $pattern]' ORDER BY pretime DESC LIMIT $limit"

    set count 0; set multi 0
    if {[DbConnect]} {
        set result [db $query]

        # If there's more than one row, send the output to
        # $target. Otherwise the output is sent to the channel.
        if {[llength $result] > 1} {
            SendTargetTheme $target Module::PreTimes head [list $pattern]
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
                SendTargetTheme $target Module::PreTimes nuke \
                    [list $preTime $section $release $files $size $disks $age $nukeTime $reason $count]
            } else {
                SendTargetTheme $target Module::PreTimes body \
                    [list $preTime $section $release $files $size $disks $age $count]
            }
        }
    }

    if {!$count} {
        # Always send this message to the channel.
        SendTargetTheme "PRIVMSG $channel" Module::PreTimes none [list $pattern]
    }
    if {$multi} {SendTargetTheme $target Module::PreTimes foot}
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::PreTimes::Load {firstLoad} {
    variable cmdToken
    variable defLimit
    variable dataSource
    upvar ::Bot::configHandle configHandle

    if {$firstLoad} {
        package require tclodbc
    }

    # Retrieve configuration options.
    array set option [Config::GetMulti $configHandle Module::PreTimes \
        addOnPre dataSource defLimit searchPres showOnNew]

    set dataSource $option(dataSource)
    if {[string is digit -strict $option(defLimit)]} {
        set defLimit $option(defLimit)
    } else {
        set defLimit 0
    }

    # Register event callbacks.
    if {[IsTrue $option(addOnPre)]} {
        ScriptRegister   pre PRE     [namespace current]::LogEvent True
        ScriptRegister   pre PRE-MP3 [namespace current]::LogEvent True
    } else {
        ScriptUnregister pre PRE     [namespace current]::LogEvent
        ScriptUnregister pre PRE-MP3 [namespace current]::LogEvent
    }
    if {[IsTrue $option(showOnNew)]} {
        ScriptRegister   pre NEWDIR [namespace current]::LogEvent True
    } else {
        ScriptUnregister pre NEWDIR [namespace current]::LogEvent
    }

    if {[IsTrue $option(searchPres)]} {
        set cmdToken [CmdCreate channel pre [namespace current]::Search \
            -args "\[-limit <num>\] \[-section <name>\] <pattern>" \
            -category "General" -desc "Search pre time database."]
    } else {
        CmdRemoveByToken $cmdToken
    }

    if {!$firstLoad} {
        # Reconnect to the data source on reload.
        catch {db disconnect}
    }
    DbConnect
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::PreTimes::Unload {} {
    variable cmdToken
    CmdRemoveByToken $cmdToken

    # Remove event callbacks.
    ScriptUnregister pre PRE     [namespace current]::LogEvent
    ScriptUnregister pre PRE-MP3 [namespace current]::LogEvent
    ScriptUnregister pre NEWDIR  [namespace current]::LogEvent

    # Close ODBC connection.
    catch {db disconnect}
}
