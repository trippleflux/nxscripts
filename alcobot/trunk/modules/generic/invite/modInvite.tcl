#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Invite Module
#
# Author:
#   neoxed (neoxed@gmail.com) Sep 23, 2005
#
# Abstract:
#   Implements a module to invite users into selected IRC channel(s).
#

namespace eval ::alcoholicz::Invite {
    if {![info exists dataSource]} {
        variable dataSource ""
        variable hostCheck 0
        variable userCheck 0
        variable warnSection ""
    }
    namespace import -force ::alcoholicz::*
}

####
# DbConnect
#
# Connect to the ODBC data source.
#
proc ::alcoholicz::Invite::DbConnect {} {
    variable dataSource

    # If the TclODBC 'object' already exists, return.
    if {[llength [info commands [namespace current]::db]]} {
        return 1
    }

    if {[catch {database connect [namespace current]::db "DSN=$dataSource"} message]} {
        LogError ModInvite "Unable to connect to database \"$dataSource\": [lindex $message 2] ([lindex $message 0])"
        return 0
    }
    db set timeout 0

    # Check if the required tables exist.
    if {![llength [db tables "invite_hosts"]] || ![llength [db tables "invite_users"]]} {
        LogError ModInvite "The database \"$dataSource\" is missing the \"invite_hosts\" or \"invite_users\" table."
        db disconnect
        return 0
    }
    return 1
}

####
# CheckHash
#
# Compares a hash created with "MakeHash" with the given password.
#
proc ::alcoholicz::Invite::CheckHash {hash password} {
    # Convert the hex-encoded hash to binary form.
    set hashSplit [split $hash "$"]

    if {[catch {
        # The "decode" command will raise an error if
        # the data contains invalid hex characters.
        set saltBin [decode hex [lindex $hashSplit 0]]
        set hashBin [decode hex [lindex $hashSplit 1]]

        if {[string length $saltBin] != 4 || [string length $hashBin] != 32} {
            error "length of the salt or hash is incorrect"
        }
    } message]} {
        LogWarning ModInvite "Invalid password hash \"$hash\": $message"
        return 0
    }

    set result [crypt pkcs5 -v2 -rounds 100 sha256 $saltBin $password]
    return [string equal $result $hashBin]
}

####
# MakeHash
#
# Creates a PKCS #5 v2 hash with a 4 byte salt and hashed 100 rounds with SHA-256.
# Format: <hex encoded salt>$<hex encoded hash>
#
proc ::alcoholicz::Invite::MakeHash {password} {
    set salt [crypt rand 4]
    set hash [crypt pkcs5 -v2 -rounds 100 sha256 $salt $password]
    return [join [list [encode hex $salt] [encode hex $hash]] "$"]
}

####
# OnChannels
#
# Checks if the specified user is on an invite channel.
#
proc ::alcoholicz::Invite::OnChannels {ircUser {ignoreChannel ""}} {
    variable channels
    foreach channel [array names channels] {
        if {[string equal -nocase $ignoreChannel $channel]} {continue}
        if {[validchan $channel] && [onchan $ircUser $channel]} {return 1}
    }
    return 0
}

####
# Process
#
# Processes an invite request, performs IRC name and host checks
# if enabled before inviting the specified IRC user.
#
proc ::alcoholicz::Invite::Process {ircUser ircHost ftpUser group groupList flags} {
    # TODO:
    # - If IRC name checking is enabled, validate the users name.
    # - If host checking is enabled, validate the users host.
    # - Invite user to the defined channels:
    #   1) Skip channels they do not have access to it (permissions).
    #   2) Log a partyline error if the bot is not opped or in a channel.
    #   3) Announce the user before inviting them.
    #   4) Finally, send the /INVITE command.
    return
}

####
# Command
#
# Private message command, !invite <FTP user> <password>.
#
proc ::alcoholicz::Invite::Command {user host handle target argc argv} {
    if {$argc != 2} {
        CmdSendHelp $user message $::lastbind
        return
    }
    # TODO:
    # - Lookup the user's DB record, return if they don't exist.
    # - Validate the password, return if invalid.
    # - Get group list and flags for the user.
    return
}


####
# ChanEvent
#
# Updates the online status of IRC users.
#
proc ::alcoholicz::Invite::ChanEvent {event args} {
    variable channels

    set time [clock seconds]
    switch -- $event {
        JOIN {
            foreach {user host handle channel} $args {break}
            set online 1
        }
        KICK {
            foreach {kicker host handle channel user reason} $args {break}
            set online [OnChannels $user $channel]
        }
        NICK {
            foreach {user host handle channel newUser} $args {break}
            set online 0
        }
        PART - QUIT {
            foreach {user host handle channel message} $args {break}
            set online [OnChannels $user $channel]
        }
        default {
            LogError ModInvite "Unknown channel event \"$event\"."
            return
        }
    }

    # Only monitor invite channels for user activities.
    set valid 0
    foreach entry [array names channels] {
        if {[string equal -nocase $entry $channel]} {
            set valid 1; break
        }
    }
    if {$valid && [DbConnect]} {
        set user [SqlEscape $user]
        db "UPDATE invite_users SET online='$online', time='$time' WHERE UPPER(irc_user)=UPPER('$user')"
    }
    return
}

####
# LogEvent
#
# Handle "INVITE" log events.
#
proc ::alcoholicz::Invite::LogEvent {event destSection pathSection path data} {
    variable hostCheck

    if {[llength $data] != 5} {
        LogError ModInvite "Invalid number of items in log data \"$data\"."
        return 1
    }
    foreach {user group ircUser groupList flags} $data {break}

    if {$hostCheck} {
        variable whois
        set whois($ircUser) [list $user $group $groupList $flags]
        putquick "WHOIS $ircUser"
    } else {
        Process $ircUser "not@used" $user $group $groupList $flags
    }
    return 0
}

####
# Whois
#
# Raw event callback, executed when Eggdrop receives a "311" reply.
#
proc ::alcoholicz::Invite::Whois {server code text} {
    variable whois

    # WHOIS reply (#311): "<nick> <user> <ident> <host> * :<real name>"
    # Note, the <real name> field can be multiple words.
    set text [split $text]
    if {[llength $text] < 6} {
        LogError ModInvite "Invalid WHOIS reply \"[join $text]\"."
        return
    }
    foreach {me ircUser ident host asterix realName} $text {break}

    # Only proceed if we performed the WHOIS on this user.
    if {[info exists whois($ircUser)]} {
        foreach {user group groupList flags} $whois($ircUser) {break}
        unset whois($ircUser)
        Process $ircUser "$ident@$host" $user $group $groupList $flags
    }
    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::Invite::Load {firstLoad} {
    variable channels
    variable dataSource
    variable hostCheck
    variable userCheck
    variable warnSection
    upvar ::alcoholicz::configHandle configHandle

    if {$firstLoad} {
        package require tclodbc
    }

    foreach option {dataSource hostCheck userCheck warnSection} {
        set $option [ConfigGet $configHandle Module::Invite $option]
    }
    set hostCheck [IsTrue $hostCheck]
    set userCheck [IsTrue $userCheck]

    unset -nocomplain channels
    foreach entry [ArgsToList [ConfigGet $configHandle Module::Invite channels]] {
        set entry [split $entry]
        if {![llength $entry]} {
            LogError ModInvite "Invalid channel definition \"[join $entry]\"."
            continue
        }

        # If no channel permissions are defined,
        # assume the channel is available to everyone.
        if {[llength $entry] == 1} {
            set channels([lindex $entry 0]) "*"
        } else {
            set channels([lindex $entry 0]) [lrange $entry 1 end]
        }
    }

    CmdCreate message "!invite" [namespace current]::Command General \
        "Invite yourself into the channel." "<FTP user> <invite password>"

    # Register event callbacks.
    bind raw  311  -  [namespace current]::Whois
    bind join -|- "*" [list [namespace current]::ChanEvent JOIN]
    bind kick -|- "*" [list [namespace current]::ChanEvent KICK]
    bind nick -|- "*" [list [namespace current]::ChanEvent NICK]
    bind part -|- "*" [list [namespace current]::ChanEvent PART]
    bind sign -|- "*" [list [namespace current]::ChanEvent QUIT]
    ScriptRegister pre INVITE [namespace current]::LogEvent True

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
proc ::alcoholicz::Invite::Unload {} {
    # Remove event callbacks.
    unbind raw  311  -  [namespace current]::Whois
    unbind join -|- "*" [list [namespace current]::ChanEvent JOIN]
    unbind kick -|- "*" [list [namespace current]::ChanEvent KICK]
    unbind nick -|- "*" [list [namespace current]::ChanEvent NICK]
    unbind part -|- "*" [list [namespace current]::ChanEvent PART]
    unbind sign -|- "*" [list [namespace current]::ChanEvent QUIT]
    ScriptUnregister pre INVITE [namespace current]::LogEvent

    catch {db disconnect}
    return
}
