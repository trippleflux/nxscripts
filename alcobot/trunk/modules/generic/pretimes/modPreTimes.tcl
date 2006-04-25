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
        variable dbHandle ""
        variable defLimit 5
    }
    namespace import -force ::Bot::*
}

####
# DbNotify
#
# Called when the connection succeeds or fails.
#
proc ::Bot::Mod::PreTimes::DbNotify {handle success} {
    if {$success} {
        LogInfo "Database connection established."
    } else {
        LogInfo "Database connection failed - [Db::GetError $handle]"
    }
}

####
# LogEvent
#
# Handle NEWDIR and PRE log events.
#
proc ::Bot::Mod::PreTimes::LogEvent {event destSection pathSection path data} {
    variable dbHandle
    variable defLimit

    # Retrieve the time now for accuracy.
    set now [clock seconds]

    # Ignore sub-directories.
    set release [file tail $path]
    if {[IsSubDir $release] || ![Db::GetStatus $dbHandle]} {return 1}

    if {$event eq "NEWDIR"} {
        set flags [GetFlagsFromSection $destSection]
        if {[FlagIsDisabled $flags "pretime"]} {return 1}

        set query {SELECT [Name pretime] FROM [Name pretimes] WHERE [Name release]=[String $release] LIMIT 1}
        set result [Db::Select $dbHandle -list $query]
        if {[llength $result]} {
            set preTime [lindex $result 0]

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
        set query {INSERT INTO [Name pretimes] ([Name pretime section release files kbytes disks]) \
            VALUES([String $now $pathSection $release $files $kiloBytes $disks])}

        if {[catch {Db::Exec $dbHandle $query} message]} {
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
    variable dbHandle

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer} {section arg}} option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    set limit [GetResultLimit $option(limit)]

    # Build SQL query.
    set query {SELECT * FROM [Name pretimes] WHERE }
    if {[info exists option(section)]} {
        append query {UPPER([Name section])=UPPER([String $option(section)]) AND }
    }
    set likePattern [Db::Pattern $dbHandle $pattern]
    append query {[Like [Name release] '$likePattern'] ORDER BY [Name pretime] DESC LIMIT $limit}

    set count 0; set multi 0
    if {[Db::GetStatus $dbHandle]} {
        set result [Db::Select $dbHandle -llist $query]

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
    variable dbHandle
    variable defLimit
    upvar ::Bot::configHandle configHandle

    # Retrieve configuration options.
    array set option [Config::GetMulti $configHandle Module::PreTimes \
        addOnPre database defLimit searchPres showOnNew]

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

    # Open a new database connection.
    if {!$firstLoad} {
        Db::Close $dbHandle
    }
    set dbHandle [Db::Open $option(database) -debug ::Bot::LogDebug \
        -ping 3 -notify [namespace current]::DbNotify]
    Db::Connect $dbHandle
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::PreTimes::Unload {} {
    variable cmdToken
    variable dbHandle

    if {$dbHandle ne ""} {
        Db::Close $dbHandle
    }
    CmdRemoveByToken $cmdToken

    # Remove event callbacks.
    ScriptUnregister pre PRE     [namespace current]::LogEvent
    ScriptUnregister pre PRE-MP3 [namespace current]::LogEvent
    ScriptUnregister pre NEWDIR  [namespace current]::LogEvent
}
