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
    if {![info exists [namespace current]::cmdTokens]} {
        variable cmdTokens [list]
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
# Group
#
# Display pre statistics for a group, command: !pregroup <name>.
#
proc ::Bot::Mod::PreTimes::Group {target user host channel argv} {
    variable dbHandle
    if {![llength $argv]} {throw CMDHELP}
    set group [join $argv]

    if {[Db::GetStatus $dbHandle]} {
        set pattern [string map {% \\% _ \\_} $group]
        set pattern "%-$pattern"

        # Stats queries.
        set query(rels) {SELECT COUNT(*) FROM [Name pretimes] \
            WHERE [Like [Name release] [String $pattern]]}

        set query(nukes) {SELECT COUNT(*) FROM [Name pretimes] \
            WHERE [Like [Name release] [String $pattern]] AND [Name nuked]=1}

        # Release queries.
        set query(first) {SELECT [Name pretime section release files kbytes disks] FROM [Name pretimes] \
            WHERE [Like [Name release] [String $pattern]] ORDER BY [Name pretime] ASC LIMIT 1}

        set query(last) {SELECT [Name pretime section release files kbytes disks] FROM [Name pretimes] \
            WHERE [Like [Name release] [String $pattern]] ORDER BY [Name pretime] DESC LIMIT 1}

        set query(nuke) {SELECT [Name pretime section release files kbytes disks nuketime reason] FROM [Name pretimes] \
            WHERE [Like [Name release] [String $pattern]] AND [Name nuked]=1 ORDER BY [Name pretime] DESC LIMIT 1}

        foreach {name value} [array get query] {
            set result($name) [Db::Select $dbHandle -list $value]
        }
    } else {
        array set result [list rels 0 nukes 0 first {} last {} nuke {}]
    }
    SendTargetTheme $target Module::PreTimes groupHead [list $group]

    set nukes $result(nukes); set releases $result(rels)
    SendTargetTheme $target Module::PreTimes groupStats [list $group $releases $nukes \
        [expr {$releases > 1 ? double($nukes)/double($releases) * 100.0 : 0.0}]]

    if {$releases > 1} {
        set age [expr {[clock seconds] - [lindex $result(first) 0]}]
        SendTargetTheme $target Module::PreTimes groupFirst [lappend result(first) $age]

        set age [expr {[clock seconds] - [lindex $result(last) 0]}]
       SendTargetTheme $target Module::PreTimes groupLast [lappend result(last) $age]
    }
    if {$nukes > 1} {
        set age [expr {[clock seconds] - [lindex $result(nuke) 0]}]
       SendTargetTheme $target Module::PreTimes groupNuke [lappend result(nuke) $age]
    }
    SendTargetTheme $target Module::PreTimes groupFoot
}

####
# Search
#
# Search for a release, command: !pre [-limit <num>] [-match exact|regexp|wild] [-section <name>] <pattern>.
#
proc ::Bot::Mod::PreTimes::Search {target user host channel argv} {
    variable dbHandle

    # Parse command options.
    set option(limit) -1
    set option(match) wild
    set pattern [join [GetOpt::Parse $argv {{limit integer} {match arg {exact regexp wild}} {section arg}} option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    set limit [GetResultLimit $option(limit)]

    set count 0; set multi 0
    if {[Db::GetStatus $dbHandle]} {
        # Build search query.
        set query {SELECT [Name pretime section release files kbytes disks nuked nuketime reason] FROM [Name pretimes] WHERE }
        if {[info exists option(section)]} {
            append query {UPPER([Name section])=UPPER([String $option(section)]) AND }
        }
        switch -- $option(match) {
            exact {
                append query {UPPER([Name release])=UPPER([String $pattern])}
            }
            regexp {
                append query {[Regexp [Name release] [String $pattern]]}
            }
            wild {
                set likePattern [Db::Pattern $dbHandle $pattern]
                append query {[Like [Name release] '$likePattern']}
            }
        }
        append query { ORDER BY [Name pretime] DESC LIMIT $limit}
        set result [Db::Select $dbHandle -llist $query]

        # If there's more than one row, send the output to
        # $target. Otherwise the output is sent to the channel.
        if {[llength $result] > 1} {
            SendTargetTheme $target Module::PreTimes searchHead [list $pattern]
            set multi 1
        } else {
            set target "PRIVMSG $channel"
        }

        # Display results.
        foreach row $result {
            incr count
            foreach {preTime section release files size disks nuked nukeTime reason} $row {break}
            set age [expr {[clock seconds] - $preTime}]

            if {$nuked} {
                SendTargetTheme $target Module::PreTimes searchNuke \
                    [list $preTime $section $release $files $size $disks $age $nukeTime $reason $count]
            } else {
                SendTargetTheme $target Module::PreTimes searchBody \
                    [list $preTime $section $release $files $size $disks $age $count]
            }
        }
    }

    if {!$count} {
        # Always send this message to the channel.
        SendTargetTheme "PRIVMSG $channel" Module::PreTimes searchNone [list $pattern]
    }
    if {$multi} {SendTargetTheme $target Module::PreTimes searchFoot}
}

####
# Total
#
# Display database statistics, command: !predb.
#
proc ::Bot::Mod::PreTimes::Total {target user host channel argv} {
    variable dbHandle

    if {[Db::GetStatus $dbHandle]} {
        set query(rels)  {SELECT COUNT(*) FROM [Name pretimes]}
        set query(nukes) {SELECT COUNT(*) FROM [Name pretimes] WHERE [Name nuked]=1}
        set query(first) {SELECT [Name pretime] FROM [Name pretimes] ORDER BY [Name pretime] ASC LIMIT 1}

        foreach {name value} [array get query] {
            set result($name) [Db::Select $dbHandle -list $value]
        }
    } else {
        array set result [list rels 0 nukes 0 first 0]
    }
    set age [expr {[clock seconds] - $result(first)}]

    SendTargetTheme $target Module::PreTimes totalHead
    SendTargetTheme $target Module::PreTimes totalBody \
        [list $result(rels) $result(nukes) $result(first) $age]
    SendTargetTheme $target Module::PreTimes totalFoot
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::PreTimes::Load {firstLoad} {
    variable cmdTokens
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
        set cmdTokens [list]

        lappend cmdTokens [CmdCreate channel pre [namespace current]::Search \
            -args "\[-limit <num>\] \[-match exact|regexp|wild\] \[-section <name>\] <pattern>" \
            -category "Pre" -desc "Search for a release."]

        lappend cmdTokens [CmdCreate channel predb [namespace current]::Total \
            -category "Pre" -desc "Display database statistics."]

        lappend cmdTokens [CmdCreate channel pregroup [namespace current]::Group \
            -args "<name>" -category "Pre" -desc "Display group statistics."]
    } else {
        foreach token $cmdTokens {
            CmdRemoveByToken $token
        }
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
    variable cmdTokens
    variable dbHandle

    foreach token $cmdTokens {
        CmdRemoveByToken $token
    }
    if {$dbHandle ne ""} {
        Db::Close $dbHandle
    }

    # Remove event callbacks.
    ScriptUnregister pre PRE     [namespace current]::LogEvent
    ScriptUnregister pre PRE-MP3 [namespace current]::LogEvent
    ScriptUnregister pre NEWDIR  [namespace current]::LogEvent
}
