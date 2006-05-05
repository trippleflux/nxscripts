#!/bin/tclsh
#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
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
#   1. Copy the siteInvite.tcl file to ioFTPD\scripts\Invite\.
#
#   2. Copy the following directories to ioFTPD\scripts\Invite\.
#
#      Eggdrop\AlcoBot\packages\AlcoExt0.6\
#      Eggdrop\AlcoBot\packages\alcolibs\
#      Eggdrop\AlcoBot\packages\mysqltcl3.0\
#      Eggdrop\AlcoBot\packages\pgtcl1.5\
#      Eggdrop\AlcoBot\packages\sqlite3\
#
#   3. Configure siteInvite.tcl and uncomment the logPath option for ioFTPD.
#
#   4. Add the following to your ioFTPD.ini:
#
#      [FTP_Custom_Commands]
#      invite    = TCL ..\scripts\Invite\siteInvite.tcl INVITE
#      invadmin  = TCL ..\scripts\Invite\siteInvite.tcl ADMIN
#      invpasswd = TCL ..\scripts\Invite\siteInvite.tcl PASSWD
#
#      [FTP_SITE_Permissions]
#      invite    = !A *
#      invadmin  = 1M
#      invpasswd = !A *
#
#   5. Rehash or restart ioFTPD for the changes to take effect.
#
# glFTPD Installation:
#   1. Install Tcl to glFTPD's chroot environment (varies between systems).
#
#      cp -f /usr/local/bin/tclsh8.4 /glftpd/bin/tclsh
#      cp -R /usr/local/lib/tcl8.4 /glftpd/lib
#      bash /glftpd/libcopy.sh
#
#   2. Copy the siteInvite.tcl file to /glftpd/bin/.
#
#   3. Copy the following directories to glFTPD's chroot environment.
#
#      cp -R AlcoBot/packages/alcolibs  /glftpd/lib
#      cp -R /usr/local/lib/AlcoExt0.6  /glftpd/lib
#      cp -R /usr/local/lib/mysqltcl3.0 /glftpd/lib
#      cp -R /usr/local/lib/pgtcl1.5    /glftpd/lib
#      cp -R /usr/local/lib/sqlite3     /glftpd/lib
#
#   4. Configure siteInvite.tcl and uncomment the logPath option for glFTPD.
#
#   5. Add the following to your glftpd.conf file:
#
#      site_cmd INVITE    EXEC /bin/siteInvite.tcl[:space:]INVITE
#      site_cmd INVADMIN  EXEC /bin/siteInvite.tcl[:space:]ADMIN
#      site_cmd INVPASSWD EXEC /bin/siteInvite.tcl[:space:]PASSWD
#
#      custom-invite    !8 *
#      custom-invadmin  1
#      custom-invpasswd !8 *
#

namespace eval ::Invite {
    # database   - Database URI string, see AlcoBot.conf for more information.
    variable database   "mysql://user:password@alcoholicz.com/database"

    # logPath    - Path to the FTP daemon's log directory, uncomment the correct
    #              line for your FTPD (ioFTPD then glFTPD, respectively).
    #variable logPath   "../logs/"
    #variable logPath   "/ftp-data/logs/"

    # hostCheck - Check a user's IRC host before inviting them into the channel.
    # userCheck - Check a user's IRC name before inviting them into the channel,
    #             this only effective on networks that allow you register user
    #             names (e.g. NickServ).
    variable hostCheck  True
    variable userCheck  True

    # passLength - Minimum length of a password, in characters.
    # passFlags  - Password security checks, these only apply to the "SITE INVPASSWD" command.
    #  A - Alphanumeric characters in the password (e.g. a-z or A-Z).
    #  N - Numbers in the password.
    #  S - Special characters in the password: !@#$%^&*()_+|-=`{}[]:";'<>?,.
    #  U - FTP user name must NOT be in the password.
    variable passLength 6
    variable passFlags  "ANU"
}

interp alias {} IsTrue {} string is true -strict

####
# ListParse
#
# Convert an argument string into a Tcl list, respecting quoted text segments.
#
proc ::Invite::ListParse {argStr} {
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
# LinePuts
#
# Write a formatted line to the client's FTP control channel.
#
proc ::Invite::LinePuts {text} {
    iputs [format "| %-60s |" $text]
}

####
# MakeHash
#
# Creates a PKCS #5 v2 hash with a 4 byte salt and hashed 100 rounds with SHA-256.
# Format: <hex encoded salt>$<hex encoded hash>
#
proc ::Invite::MakeHash {password} {
    set salt [crypt rand 4]
    set hash [crypt pkcs5 -v2 -rounds 100 sha256 $salt $password]
    return [format {%s$%s} [encode hex $salt] [encode hex $hash]]
}

####
# SetIrcUser
#
# Set the IRC nick-name for a user.
#
proc ::Invite::SetIrcUser {ftpUser ircUser} {
    variable dbHandle

    # Build queries.
    set queryUpdate {UPDATE [Name invite_users] SET [Name irc_user]=[String $ircUser] \
        WHERE [Name ftp_user]=[String $ftpUser]}

    set queryInsert {INSERT INTO [Name invite_users] ([Name ftp_user irc_user]) \
        VALUES([String $ftpUser $ircUser])}

    if {![Db::Exec $dbHandle $queryUpdate]} {
        Db::Exec $dbHandle $queryInsert
    }
}

####
# SetPassword
#
# Set the invite password for a user.
#
proc ::Invite::SetPassword {ftpUser password} {
    variable dbHandle
    set hash [MakeHash $password]

    # Build queries.
    set queryUpdate {UPDATE [Name invite_users] SET [Name password]=[String $hash] \
        WHERE [Name ftp_user]=[String $ftpUser]}

    set queryInsert {INSERT INTO [Name invite_users] ([Name ftp_user password]) \
        VALUES([String $ftpUser $hash])}

    if {![Db::Exec $dbHandle $queryUpdate]} {
        Db::Exec $dbHandle $queryInsert
    }
}

####
# Admin
#
# Change and update IRC invite user options.
#
proc ::Invite::Admin {argList} {
    variable dbHandle
    variable hostCheck
    variable userCheck

    # Check parameters and event names.
    set event [string toupper [lindex $argList 0]]
    array set params {
        ADDHOST  3 ADDIP  3
        DELHOST  3 DELIP  3
        HOSTS    2
        DELUSER  2
        LIST     1 USERS  1
        NICK     3 USER   3
        PASS     3 PASSWD 3 PASSWORD 3
    }
    if {![info exists params($event)] || [llength $argList] != $params($event)} {
        set event HELP
    }

    set ftpUser [lindex $argList 1]
    switch -- $event {
        ADDHOST - ADDIP {
            if {![IsTrue $hostCheck]} {
                LinePuts "Host checking is disabled."
                return 0
            }
            set hostMask [lindex $argList 2]

            if {[string match "*!*@*" $hostMask]} {
                LinePuts "Invalid host-mask, must be \"ident@host\" NOT \"nick!ident@host\"."
                return 1
            }
            if {![string match "*?@?*" $hostMask]} {
                LinePuts "Invalid host-mask, must be \"ident@host\"."
                return 1
            }
            set query {INSERT INTO [Name invite_hosts] ([Name ftp_user hostmask]) \
                VALUES([String $ftpUser $hostMask])}

            catch {Db::Exec $dbHandle $query}
            LinePuts "Added host-mask \"$hostMask\" to user \"$ftpUser\"."
        }
        DELHOST - DELIP {
            if {![IsTrue $hostCheck]} {
                LinePuts "Host checking is disabled."
                return 0
            }
            set hostMask [lindex $argList 2]
            set query {DELETE FROM [Name invite_hosts] WHERE [Name ftp_user]=[String $ftpUser] \
                AND [Name hostmask]=[String $hostMask]}

            if {[Db::Exec $dbHandle $query]} {
                LinePuts "Deleted host-mask \"$hostMask\" from user \"$ftpUser\"."
            } else {
                LinePuts "Invalid host-mask \"$hostMask\" for user \"$ftpUser\"."
                return 1
            }
        }
        HOSTS {
            if {![IsTrue $hostCheck]} {
                LinePuts "Host checking is disabled."
                return 0
            }
            LinePuts "Hosts for user \"$ftpUser\":"
            set count 0
            set query {SELECT [Name hostmask] FROM [Name invite_hosts] \
                WHERE [Name ftp_user]=[String $ftpUser] ORDER BY [Name hostmask] ASC}

            foreach hostmask [Db::Select $dbHandle -list $query] {
                LinePuts " $hostmask"; incr count
            }
            if {!$count} {LinePuts "- No hosts found."}
        }
        DELUSER {
            set  count [Db::Exec $dbHandle {DELETE FROM [Name invite_hosts] WHERE [Name ftp_user]=[String $ftpUser]}]
            incr count [Db::Exec $dbHandle {DELETE FROM [Name invite_users] WHERE [Name ftp_user]=[String $ftpUser]}]
            if {$count} {
                LinePuts "Deleted user \"$ftpUser\"."
            } else {
                LinePuts "Invalid user \"$ftpUser\", check \"SITE INVADMIN USERS\"."
                return 1
            }
        }
        LIST - USERS {
            iputs "| FTP User       | IRC User       | Last Online                |"
            iputs "|--------------------------------------------------------------|"
            set count 0
            set query {SELECT [Name ftp_user irc_user online time] \
                FROM [Name invite_users] ORDER BY [Name ftp_user] ASC}

            foreach {ftpUser ircUser online time} [Db::Select $dbHandle -list $query] {
                incr count
                if {!$time} {
                    set online "Never"
                } elseif {$online} {
                    set online "Now"
                } else {
                    set online [clock format $time -format "%b %d, %Y %H:%M:%S GMT" -gmt 1]
                }
                iputs [format "| %-14s | %-14s | %-26s |" $ftpUser $ircUser $online]
            }
            if {!$count} {LinePuts "No users found."}
        }
        USER - NICK {
            if {![IsTrue $userCheck]} {
                LinePuts "User name checking is disabled."
                return 0
            }
            set ircUser [lindex $argList 2]
            SetIrcUser $ftpUser $ircUser
            LinePuts "IRC nick-name for user \"$ftpUser\" set to \"$ircUser\"."
        }
        PASS - PASSWD - PASSWORD {
            set password [lindex $argList 2]
            SetPassword $ftpUser $password
            LinePuts "Password for user \"$ftpUser\" set to \"$password\"."
        }
        default {
            LinePuts "Invite Admin Commands"

            LinePuts ""
            LinePuts "Manage Users:"
            LinePuts "- SITE INVADMIN DELUSER <user>"
            LinePuts "- SITE INVADMIN USERS"

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
                LinePuts "- User name checking is disabled."
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
proc ::Invite::Invite {argList} {
    global user group groups flags
    variable dbHandle
    variable hostCheck
    variable userCheck

    if {[llength $argList] != 1} {
        LinePuts "Usage: SITE INVITE <nick>"
        return 1
    }
    set ircUser [lindex $argList 0]

    # Check if the user has an invite record.
    set query {SELECT [Name irc_user password] FROM [Name invite_users] WHERE [Name ftp_user]=[String $user]}
    set result [Db::Select $dbHandle -list $query]
    if {![llength $result] || [lindex $result 1] eq ""} {
        LinePuts "No invite password found for your FTP account."
        LinePuts "Please set a password using \"SITE INVPASSWD <password>\"."
        return 1
    }

    # Validate IRC user-name.
    if {[IsTrue $userCheck]} {
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
    set query {SELECT COUNT(*) FROM [Name invite_hosts] WHERE [Name ftp_user]=[String $user]}
    if {[IsTrue $hostCheck] && ![Db::Select $dbHandle -list $query]} {
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
proc ::Invite::Passwd {argList} {
    global user
    variable dbHandle
    variable passLength
    variable passFlags

    if {[llength $argList] != 1} {
        LinePuts "Usage: SITE INVPASSWD <password>"
        return 1
    }
    set password [lindex $argList 0]

    if {[string length $password] < $passLength} {
        LinePuts "The password must be at least $passLength character(s)."
        return 1
    }

    # Security flag checks (hackish, but better than nothing).
    if {[string first "A" $passFlags] != -1 && ![regexp -all -- {\w} $password]} {
        LinePuts "Your password must contain alphanumeric characters."
        return 1
    }
    if {[string first "N" $passFlags] != -1 && ![regexp -all -- {\d} $password]} {
        LinePuts "Your password must contain numbers (e.g. 0-9)."
        return 1
    }
    if {[string first "S" $passFlags] != -1 && ![regexp -all -- {\W} $password]} {
        LinePuts "Your password must contain special characters."
        return 1
    }
    if {[string first "U" $passFlags] != -1} {
        if {[string first [string toupper $user] [string toupper $password]] != -1} {
            LinePuts "Your FTP user name must not be present in the password."
            return 1
        }
    }

    SetPassword $user $password
    LinePuts "Invite password set to \"$password\"."
    return 0
}

####
# Main
#
# Script entry point.
#
proc ::Invite::Main {} {
    global auto_path tcl_platform
    variable database
    variable dbHandle
    variable isWindows
    variable logPath

    if {$tcl_platform(platform) eq "windows"} {
        global args
        set isWindows 1
        set argList [ListParse [expr {[info exists args] ? $args : ""}]]
    } else {
        global argv env user group groups flags
        set isWindows 0

        # Map variables.
        set argList $argv
        set user    $env(USER)
        set group   $env(GROUP)
        set groups  [list $env(GROUP)]
        set flags   $env(FLAGS)

        # Emulate ioFTPD's Tcl commands.
        proc ::Invite::iputs {text} {puts stdout $text}
        proc ::Invite::putlog {text} {
            variable logPath
            set filePath [file join $logPath "glftpd.log"]
            set timeStamp [clock format [clock seconds] -format "%a %b %d %T %Y" -gmt 0]
            if {![catch {set handle [open $filePath a]} error]} {
                puts $handle [format "%.24s %s" $timeStamp $text]
                close $handle
            } else {iputs $error}
        }
    }

    # Add the current directory to the package search path.
    set currentPath [file dirname [file normalize [info script]]]
    if {![info exists auto_path] || [lsearch -exact $auto_path $currentPath] == -1} {
        lappend auto_path $currentPath
    }

    iputs ".-\[Invite\]-----------------------------------------------------."
    set result 1
    if {![info exists logPath] || ![file isdirectory $logPath]} {
        LinePuts "Invalid log path, check configuration."
    } elseif {[catch {
                package require AlcoExt 0.6
                package require alco::db 1.2
                set dbHandle [Db::Open $database]
                Db::Connect $dbHandle
            } message]} {
        LinePuts $message
    } else {
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
        Db::Close $dbHandle
    }

    iputs "'--------------------------------------------------------------'"
    return $result
}

::Invite::Main
