#!/bin/tclsh
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
# ioFTPD Installation:
#   1. Copy the siteInvite.tcl file to x:\ioFTPD\scripts\.
#   2. Copy the AlcoExt0.x and tclodbc2.x directories from
#      Eggdrop\AlcoBot\libs\ to x:\ioFTPD\lib\.
#   3. Configure the script and uncomment the logPath option for ioFTPD.
#   4. Add the following to your ioFTPD.ini:
#
#   [FTP_Custom_Commands]
#   invite    = TCL ..\scripts\siteInvite.tcl INVITE
#   invadmin  = TCL ..\scripts\siteInvite.tcl ADMIN
#   invpasswd = TCL ..\scripts\siteInvite.tcl PASSWD
#
#   [FTP_SITE_Permissions]
#   invite    = !A *
#   invadmin  = 1M
#   invpasswd = !A *
#
#   5. Rehash or restart ioFTPD for the changes to take effect.
#
# glFTPD Installation:
#   1. Copy the siteInvite.tcl file to /glftpd/bin/.
#   2. Configure the script and uncomment the logPath option for glFTPD.
#   3. Add the following to your glftpd.conf:
#
#   site_cmd INVITE    EXEC /bin/siteInvite.tcl[:space:]INVITE
#   site_cmd INVADMIN  EXEC /bin/siteInvite.tcl[:space:]ADMIN
#   site_cmd INVPASSWD EXEC /bin/siteInvite.tcl[:space:]PASSWD
#
#   custom-invite    !8 *
#   custom-invadmin  1
#   custom-invpasswd !8 *
#
#   4. Rehash or restart ioFTPD for the changes to take effect.
#

namespace eval ::siteInvite {
    # dataSource - Name of the ODBC data source.
    # logPath    - Path to the FTP daemon's log directory.
    variable dataSource "Alcoholicz"

    # Uncomment the following line for your FTPD:
    #variable logPath   "../logs/"
    #variable logPath   "/glftpd/ftp-data/logs/"

    # hostCheck - Check a user's IRC host before inviting them into the channel.
    # userCheck - Check a user's IRC name before inviting them into the channel,
    #             this only effective on networks that allow you register usernames.
    variable hostCheck  True
    variable userCheck  True
}

interp alias {} IsTrue {} string is true -strict

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
# Admin
#
# Change and update IRC invite user options.
#
proc ::siteInvite::Admin {argList} {
    global user flags
    variable hostCheck
    variable hostFields
    variable userCheck

    # Check parameters and event names.
    set event [string toupper [lindex $argList 0]]
    array set params {
        ADDHOST  3 ADDIP  3
        DELHOST  3 DELIP  3
        HOSTS    2
        NICK     3 USER   3
        PASS     3 PASSWD 3
        PASSWORD 3
    }
    if {![info exists params($event)] || [llength $argList] != $params($event)} {
        set event HELP
    }

    switch -- $event {
        ADDHOST - ADDIP {
            if {![IsTrue $hostCheck]} {
                LinePuts "Host checking is disabled."
                return 1
            }
            # TODO
        }
        DELHOST - DELIP {
            if {![IsTrue $hostCheck]} {
                LinePuts "Host checking is disabled."
                return 1
            }
            # TODO
        }
        HOSTS {
            if {![IsTrue $hostCheck]} {
                LinePuts "Host checking is disabled."
                return 1
            }
            # TODO
        }
        NICK - USER {
            if {![IsTrue $userCheck]} {
                LinePuts "Host checking is disabled."
                return 1
            }
            # TODO
        }
        PASS - PASSWD - PASSWORD {
            # TODO
        }
        default {
            LinePuts "Invite Admin Commands"

            LinePuts ""
            LinePuts "Manage Hosts:"
            if {[IsTrue $hostCheck]} {
                LinePuts "- SITE INVADMIN ADDHOST <user> <host>"
                LinePuts "- SITE INVADMIN DELHOST <user> <host>"
                LinePuts "- SITE INVADMIN HOSTS   <user>"
            } else {
                LinePuts "- Host checking is disabled."
            }

            LinePuts ""
            LinePuts "Set IRC User:"
            if {[IsTrue $userCheck]} {
                LinePuts "- SITE INVADMIN USER <user> <nick>"
            } else {
                LinePuts "- Username checking is disabled."
            }

            LinePuts ""
            LinePuts "Set Password:"
            LinePuts "- SITE INVADMIN PASSWD <user> <password>"
        }
    }
    return 0
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
        LinePuts "Usage: SITE INVITE <nick>"
        return 1
    }
    set ftpUserEsc [SqlEscape $user]
    set ircUser [lindex $argList 0]

    # Check if the user has an invite record.
    set result [db "SELECT irc_user, password FROM invite_users WHERE ftp_user='$ftpUserEsc'"]
    set result [lindex $result 0]
    if {![llength $result] || [lindex $result 1] eq ""} {
        LinePuts "No invite password found for your FTP account."
        LinePuts "Please set a password using \"SITE INVPASSWD <password>\"."
        return 1
    }

    # Validate IRC user-name.
    if {$userCheck} {
        set required [lindex $result 0]
        if {$required eq ""} {
            LinePuts "Your account does not have an IRC nick-name defined."
            LinePuts "Ask a siteop to set your IRC nick-name."
            return 1
        }
        if {![string equal -nocase $required $ircUser]} {
            LinePuts "Invalid IRC nick-name \"$ircUser\"."
            LinePuts "You must invite yourself using the nick-name \"$required\"."
            return 1
        }
    }

    # Check if the user has any IRC hosts added.
    if {$hostCheck && ![db "SELECT count(*) FROM invite_hosts WHERE ftp_user='$ftpUserEsc'"]} {
        LinePuts "Your account does not have any IRC hosts added."
        LinePuts "Ask a siteop to add your IRC host-mask."
        return 1
    }

    LinePuts "Inviting the IRC nick-name \"$ircUser\"."
    putlog "INVITE: \"$user\" \"$group\" \"$groups\" \"$flags\" \"$ircUser\""
    return 0
}

####
# Passwd
#
# Allows a user to set his or her own password.
#
proc ::siteInvite::Passwd {argList} {
    global user
    if {[llength $argList] != 1} {
        LinePuts "Usage: SITE INVPASSWD <password>"
        return 1
    }
    # TODO
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
        proc iputs {text} {puts stdout $text}
        proc putlog {text} {
            set filePath [file join $::siteInvite::logPath "glftpd.log"]
            set timeStamp [clock format [clock seconds] -format "%a %b %d %T %Y"]
            if {![catch {set handle [open $filePath a]} error]} {
                puts $handle [format "%.24s %s" $timeStamp $text]
                close $handle
            } else {iputs $error}
        }
    }

    iputs ".-\[Invite\]----------------------------------------------------."
    set result 1
    if {![info exists logPath] || ![file exists $logPath]} {
        LinePuts "Invalid log path, check configuration."

    } elseif {[catch {package require AlcoExt} message]} {
        LinePuts $message

    } elseif {[catch {package require tclodbc} message]} {
        LinePuts $message

    } elseif {[DbConnect]} {
        set event [string toupper [lindex $argList 0]]
        set argList [lrange $argList 1 end]

        if {$event eq "ADMIN"} {
            set result [Admin $argList]
        } elseif {$event eq "INVITE"} {
            set result [Invite $argList]
        } elseif {$event eq "PASSWD"} {
            set result [Passwd $argList]
        } else {
            LinePuts "Unknown event \"$event\"."
        }
        db disconnect
    }

    iputs "'-------------------------------------------------------------'"
    return $result
}

::siteInvite::Main
