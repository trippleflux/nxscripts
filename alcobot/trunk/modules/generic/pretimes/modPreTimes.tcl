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
    if {![info exists defLimit]} {
        variable dataSource ""
        variable defLimit 5
        variable timerId ""
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
        LogError ModPreTimes "Unable to connect to DSN $dataSource: [lindex $message 2] ([lindex $message 0])"
        return 0
    }

    db set timeout 0
    return 1
}

####
# DbPing
#
# Ping the server every 5 minutes to prevent the session from timing out.
#
proc ::alcoholicz::PreTimes::DbPing {} {
    variable timerId

    # TODO: Is it necessary to ping the server to
    # prevent the current session from timing out?

    set timerId [timer 5 [namespace current]::DbPing]
    return
}

####
# LogHandler
#
# Handle NEWDIR and PRE log events.
#
proc ::alcoholicz::PreTimes::LogHandler {event destSection pathSection path data} {
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

        set preTime [lindex [db "SELECT pretime FROM pretimes WHERE release='$release' LIMIT 1"] 0]
        if {[string is digit -strict $preTime]} {
            # Check for a section pre time.
            if {![FlagGetValue $flags "pretime" limit]} {
                set limit $defLimit
            }

            set age [expr {$now - $preTime}]
            set limit [expr {$limit * 60}]
            if {$age > $limit} {
                set event "PRELATE"
            } else {
                set event "PREOK"
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
        LogError ModPreTime "Unknown event \"$event\"."
    }
    return 1
}

####
# Search
#
# Search for a release, command: !pre [-limit <num>] [-section <name>] <pattern>.
#
proc ::alcoholicz::PreTimes::Search {user host handle channel target argc argv} {
    # Parse command options.
    set option(limit) -1
    if {[catch {set pattern [GetOptions $argv {{limit integer} {section arg}} option]} message]} {
        CmdSendHelp $channel channel $::lastbind $message
        return
    } elseif {$pattern eq ""} {
        CmdSendHelp $channel channel $::lastbind "you must specify a pattern"
        return
    }
    set option(limit) [GetResultLimit $option(limit)]

    # Build SQL query.
    set query "SELECT * FROM pretimes WHERE "
    if {[info exists option(section)]} {
        set section [SqlEscape [string tolower $option(section)]]
        append query "LOWER(section)='$section' AND "
    }
    append query "release LIKE '[SqlGetPattern $pattern]' ORDER BY pretime DESC LIMIT $option(limit)"

    set count 0; set multi 0
    if {[DbConnect]} {
        set result [db $query]

        # If there's more than one entry, send the output to
        # $target. Otherwise the output is sent to the channel.
        if {[llength $result] > 1} {
            SendTargetTheme $target preHead [list $pattern]
            set multi 1
        } else {
            set target "PRIVMSG $channel"
        }

        # Display results.
        foreach entry $result {
            incr count
            foreach {id preTime section release files size disks nuked nukeTime reason} $entry {break}
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
    variable timerId
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
        ScriptRegister   pre PRE     [namespace current]::LogHandler True
        ScriptRegister   pre PRE-MP3 [namespace current]::LogHandler True
    } else {
        ScriptUnregister pre PRE     [namespace current]::LogHandler
        ScriptUnregister pre PRE-MP3 [namespace current]::LogHandler
    }
    if {[IsTrue $showOnNew]} {
        ScriptRegister   pre NEWDIR [namespace current]::LogHandler True
    } else {
        ScriptUnregister pre NEWDIR [namespace current]::LogHandler
    }

    # Create channel commands.
    if {[IsTrue $searchPres]} {
        if {[ConfigExists $configHandle Module::PreTimes cmdPrefix]} {
            set prefix [ConfigGet $configHandle Module::PreTimes cmdPrefix]
        } else {
            set prefix $::alcoholicz::cmdPrefix
        }

        CmdCreate channel ${prefix}pre [namespace current]::Search \
            General "Search pre time database." "\[-limit <num>\] \[-section <name>\] <pattern>"
    }

    if {$firstLoad} {
        # Ping the server every five minutes.
        set timerId [timer 5 [namespace current]::DbPing]
    } else {
        # Reconnect to data-source.
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
    variable timerId

    # Remove event callbacks.
    ScriptUnregister pre PRE     [namespace current]::LogHandler
    ScriptUnregister pre PRE-MP3 [namespace current]::LogHandler
    ScriptUnregister pre NEWDIR  [namespace current]::LogHandler

    # Kill  checking timer.
    if {$timerId ne ""} {
        catch {killtimer $timerId}
        set timerId ""
    }

    catch {db disconnect}
    return
}
