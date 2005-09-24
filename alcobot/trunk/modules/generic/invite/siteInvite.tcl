#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Site Invite Command
#
# Author:
#   neoxed (neoxed@gmail.com) Sep 23, 2005
#
# Abstract:
#   Implements a SITE command to manage invite options.
#

namespace eval ::siteInvite {
    # dataSource - Name of the ODBC data source.
    # logPath    - Path to the FTP daemon's log directory.
    # tclODBC    - Path to the TclODBC library extension.
    variable dataSource   "Alcoholicz"

    # Uncomment the following lines for ioFTPD:
    #variable logPath "../logs/"
    #variable tclODBC "./tclodbc25.dll"

    # Uncomment the following lines for glFTPD:
    #variable logPath "/glftpd/ftp-data/logs/"
    #variable tclODBC "/usr/lib/tclodbc25.so"

    # hostCheck    - Check a user's IRC host before inviting them into the channel.
    # hostFields   - Number of fields in the hostmask that are not wildcards.
    # requireIdent - Require a valid ident in the hostmask.
    # userCheck    - Check a user's IRC name before inviting them into the channel,
    #                this only effective on networks that allow you register usernames.
    variable hostCheck    True
    variable hostFields   2
    variable requireIdent False
    variable userCheck    True
}

interp alias {} IsTrue {} string is true -strict
interp alias {} IsFalse {} string is false -strict

####
# ArgsToList
#
# Convert an argument string into a Tcl list, respecting quoted text segments.
#
proc ::siteInvite::ArgsToList {argStr} {
    set argList [list]
    set length [string length $argStr]

    for {set index 0} {$index < $length} {incr index} {
        # Ignore leading white-space.
        while {[string is space -strict [string index $argStr $index]]} {incr index}
        if {$index >= $length} {break}

        if {[string index $argStr $index] eq "\""} {
            # Find the next quote character.
            set startIndex [incr index]
            while {[string index $argStr $index] ne "\"" && $index < $length} {incr index}
        } else {
            # Find the next white-space character.
            set startIndex $index
            while {![string is space -strict [string index $argStr $index]] && $index < $length} {incr index}
        }
        lappend argList [string range $argStr $startIndex [expr {$index - 1}]]
    }
    return $argList
}

####
# DbConnect
#
# Connect to the ODBC data source.
#
proc ::siteInvite::DbConnect {} {
    variable dataSource
    variable tclODBC

    if {[catch {load $tclODBC} message]} {
        LinePuts "Unable to load TclODBC: $message"
        return 0
    }
    if {[catch {database connect [namespace current]::db "DSN=$dataSource"} message]} {
        LinePuts "Unable to connect to database \"$dataSource\"."
        return 0
    }
    db set timeout 0

    # Check if the required tables exist.
    if {![llength [db tables "invite_hosts"]] || ![llength [db tables "invite_users"]]} {
        LinePuts "The database \"$dataSource\" is missing the \"invite_hosts\" or \"invite_users\" table."
        db disconnect
        return 0
    }
    return 1
}

####
# SqlEscape
#
# Escape SQL quote characters with a backslash.
#
proc ::siteInvite::SqlEscape {string} {
    return [string map {\\ \\\\ ` \\` ' \\' \" \\\"} $string]
}

####
# LinePuts
#
# Write a formatted line to the client's FTP control channel.
#
proc ::siteInvite::LinePuts {text} {
    iputs [format "| %-60s |" $text]
}

####
# LogError
#
# Write a message to Error.log.
#
proc ::siteInvite::LogError {text} {
    variable isWindows
    variable logPath

    if {$isWindows} {
        set filePath [file join $logPath "Error.log"]
        set timeStamp [clock format [clock seconds] -format "%m-%d-%Y %H:%M:%S"]
    } else {
        set filePath [file join $logPath "error.log"]
        set timeStamp [clock format [clock seconds] -format "%a %b %d %T %Y"]
        set timeStamp [format "%.24s \[%-8d\]" $timeStamp [pid]]
    }

    if {![catch {set handle [open $filePath a]} error]} {
        puts $handle "$timeStamp $text"
        close $handle
    } else {iputs $error}
}

####
# LogMain
#
# Write a message to ioFTPD.log or glftpd.log.
#
proc ::siteInvite::LogMain {text} {
    variable isWindows
    variable logPath

    if {$isWindows} {
        putlog $text
    } else {
        set filePath [file join $logPath "glftpd.log"]
        set timeStamp [clock format [clock seconds] -format "%a %b %d %T %Y"]

        if {![catch {set handle [open $filePath a]} error]} {
            puts $handle [format "%.24s %s" $timeStamp $text]
            close $handle
        } else {iputs $error}
    }
}

####
# Invite
#
# Invites a user into the IRC channels.
#
proc ::siteInvite::Invite {argList} {
    global user group groups flags
    variable hostCheck
    variable userCheck

    if {[llength $argList] != 1} {
        LinePuts "Usage: SITE INVITE <IRC user>"
        return 1
    }
    # TODO
    return 0
}

####
# Update
#
# Change and update IRC invite user options.
#
proc ::siteInvite::Update {argList} {
    global user flags
    variable hostCheck
    variable hostFields
    variable requireIdent
    variable userCheck

    set argLength [llength $argList]
    set event [string toupper [lindex $argList 0]]
    switch -- $event {
        ADDHOST - ADDIP {
            # Add an IRC host.
            if {![IsTrue $hostCheck]} {
                LinePuts "Host checking is disabled."
                return 1
            }
            if {$argLength != 2} {
                LinePuts "Usage: SITE IRCINVITE $event <host>"
                return 1
            }

            # TODO
        }
        DELHOST - DELIP {
            # Remove an IRC host.
            if {![IsTrue $hostCheck]} {
                LinePuts "Host checking is disabled."
                return 1
            }
            if {$argLength != 2} {
                LinePuts "Usage: SITE IRCINVITE $event <host>"
                return 1
            }

            # TODO
        }
        HOSTS {
            # List IRC hosts.
            if {![IsTrue $hostCheck]} {
                LinePuts "Host checking is disabled."
                return 1
            }

            # TODO
        }
        NICK - USER {
            # Set IRC username.
            if {![IsTrue $userCheck]} {
                LinePuts "Host checking is disabled."
                return 1
            }
            if {$argLength != 2} {
                LinePuts "Usage: SITE IRCINVITE $event <IRC user>"
                return 1
            }

            # TODO
        }
        PASS - PASSWD - PASSWORD {
            # Set IRC invite password.
            if {$argLength != 2} {
                LinePuts "Usage: SITE IRCINVITE $event <password>"
                return 1
            }

            # TODO
        }
        default {
            LinePuts "IRC invite command help."

            LinePuts ""
            LinePuts "Manage Hosts:"
            if {[IsTrue $hostCheck]} {
                LinePuts "- SITE IRCINVITE ADDHOST <host>"
                LinePuts "- SITE IRCINVITE DELHOST <host>"
                LinePuts "- SITE IRCINVITE HOSTS"
            } else {
                LinePuts "- Host checking is disabled."
            }

            LinePuts ""
            LinePuts "Set IRC User:"
            if {[IsTrue $userCheck]} {
                LinePuts "- SITE IRCINVITE USER <IRC user>"
           } else {
                LinePuts "- Username checking is disabled."
            }

            LinePuts ""
            LinePuts "Set Password:"
            LinePuts "- SITE IRCINVITE PASS <password>"
        }
    }
    return 0
}

####
# Main
#
# Script entry point.
#
proc ::siteInvite::Main {} {
    variable isWindows
    variable logPath
    variable tclODBC

    if {$::tcl_platform(platform) eq "windows"} {
        set isWindows 1
        set argList [ArgsToList [expr {[info exists ::args] ? $::args : ""}]]
    } else {
        global env user group groups flags
        set isWindows 0

        # Map variables.
        set argList $::argv
        set user    $env(USER)
        set group   $env(GROUP)
        set groups  [list $env(GROUP)]
        set flags   $env(FLAGS)

        # Emulate ioFTPD's Tcl commands.
        proc ::iputs {text} {puts stdout $text}
    }

    iputs ".-\[Invite\]----------------------------------------------------."
    set result 1
    if {![info exists logPath] || ![file exists $logPath]} {
        LinePuts "Invalid log path, check configuration."

    } elseif {![info exists tclODBC] || ![file exists $tclODBC]} {
        LinePuts "Invalid path to the TclODBC library, check configuration."

    } elseif {[DbConnect]} {
        set event [string toupper [lindex $argList 0]]

        if {$event eq "INVITE"} {
            set result [Invite [lrange $argList 1 end]]
        } elseif {$event eq "UPDATE"} {
            set result [Update [lrange $argList 1 end]]
        } else {
            LinePuts "Unknown event \"$event\"."
        }
        db disconnect
    }

    iputs "'-------------------------------------------------------------'"
    return $result
}

::siteInvite::Main
