#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
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
# Exported Procedures:
#   GetFtpUser <ircUser>
#   GetIrcUser <ftpUser>
#

namespace eval ::Bot::Mod::Invite {
    if {![info exists [namespace current]::channels]} {
        variable channels [list]
        variable cmdToken ""
        variable dataSource ""
        variable hostCheck 0
        variable userCheck 0
        variable warnSection ""
    }
    namespace import -force ::Bot::*
    namespace import -force ::Bot::Mod::Ftpd::*
    namespace export GetFtpUser GetIrcUser
}

####
# DbConnect
#
# Connect to the ODBC data source.
#
proc ::Bot::Mod::Invite::DbConnect {} {
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
# GetFtpUser
#
# Looks up the FTP user-name for the given IRC nick-name.
#
proc ::Bot::Mod::Invite::GetFtpUser {ircUser} {
    if {![DbConnect]} {
        error "invite database is offline"
    }

    # IRC nick-names are NOT case-senstive.
    set result [db "SELECT ftp_user FROM invite_users WHERE online='1' \
        AND UPPER(irc_user)=UPPER('[SqlEscape $ircUser]') LIMIT 1"]

    if {[llength $result]} {
        # First row and first column.
        return [lindex $result 0 0]
    } else {
        error "unknown IRC user, please re-invite yourself"
    }
}

####
# GetIrcUser
#
# Looks up the IRC nick-name for the given FTP user-name.
#
proc ::Bot::Mod::Invite::GetIrcUser {ftpUser} {
    if {![DbConnect]} {
        error "invite database is offline"
    }

    # FTP user-names are case-senstive.
    set result [db "SELECT irc_user FROM invite_users WHERE online='1' \
        AND ftp_user='[SqlEscape $ftpUser]' LIMIT 1"]

    if {[llength $result]} {
        # First row and first column.
        return [lindex $result 0 0]
    } else {
        error "unknown FTP user, please re-invite yourself"
    }
}

####
# CheckHash
#
# Compares a hash created with "MakeHash" with the given password.
#
proc ::Bot::Mod::Invite::CheckHash {hash password} {
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
proc ::Bot::Mod::Invite::MakeHash {password} {
    set salt [crypt rand 4]
    set hash [crypt pkcs5 -v2 -rounds 100 sha256 $salt $password]
    return [join [list [encode hex $salt] [encode hex $hash]] "$"]
}

####
# OnChannels
#
# Checks if the specified user is on an invite channel.
#
proc ::Bot::Mod::Invite::OnChannels {ircUser {ignoreChannel ""}} {
    variable channels
    foreach channel [array names channels] {
        if {[string equal -nocase $ignoreChannel $channel]} {continue}
        if {[validchan $channel] && [onchan $ircUser $channel]} {return 1}
    }
    return 0
}

####
# SendTheme
#
# Send a themed message to the user and warning section.
#
proc ::Bot::Mod::Invite::SendTheme {user type {valueList ""}} {
    variable warnSection

    if {$warnSection ne ""} {
        SendSectionTheme $warnSection Module::Invite $type $valueList
    }
    if {[string equal -length 7 "invalid" $type]} {
        # Display a less verbose message to the possible intruder.
        set type "invalid"
    }
    SendTargetTheme "PRIVMSG $user" Module::Invite $type $valueList
}

####
# Process
#
# Processes an invite request, performs IRC name and host checks
# if enabled before inviting the specified IRC user.
#
proc ::Bot::Mod::Invite::Process {ircUser ircHost ftpUser ftpGroup ftpGroupList ftpFlags} {
    variable channels
    variable hostCheck
    variable userCheck

    set time [clock seconds]
    if {![DbConnect]} {
        SendTheme $ircUser databaseDown [list $ftpUser $ircUser]
        return
    }

    set ftpUserEsc [SqlEscape $ftpUser]
    if {$hostCheck} {
        # Validate IRC host.
        set valid 0
        foreach row [db "SELECT hostmask FROM invite_hosts WHERE ftp_user='$ftpUserEsc'"] {
            if {[string match -nocase [lindex $row 0] $ircHost]} {
                set valid 1; break
            }
        }
        if {!$valid} {
            SendTheme $ircUser invalidHost [list $ftpUser $ircUser $ircHost]
            return
        }
    }

    # Update the user's IRC user name, online status, and time stamp.
    set online [OnChannels $ircUser]
    set query "UPDATE invite_users SET "
    if {!$userCheck} {
        append query "irc_user='[SqlEscape $ircUser]', "
    }
    append query "online='$online', time='$time' WHERE ftp_user='$ftpUserEsc'"
    db $query

    set failed [list]
    foreach channel [lsort [array names channels]] {
        if {![PermCheck $channels($channel) $ftpUser $ftpGroupList $ftpFlags]} {continue}

        # Make sure the bot is opped in the channel.
        if {![validchan $channel] || ![botonchan $channel] || ![botisop $channel]} {
            lappend failed $channel
        } else {
            SendTargetTheme "PRIVMSG $channel" Module::ReadLogs INVITE \
                [list $ftpUser $ftpGroup $ircUser]
            putquick "INVITE $ircUser $channel"
        }
    }

    if {[llength $failed]} {
        set failed [ListConvert [lsort $failed]]
        SendTheme $ircUser needOps [list $ftpUser $ircUser $failed]
    }
}

####
# Command
#
# Private message command, !invite <FTP user> <password>.
#
proc ::Bot::Mod::Invite::Command {target user host argv} {
    variable userCheck

    if {[llength $argv] != 2} {throw CMDHELP}
    set ftpUser [lindex $argv 0]
    set password [lindex $argv 1]

    if {[DbConnect]} {
        set result [db "SELECT irc_user, password FROM invite_users WHERE ftp_user='[SqlEscape $ftpUser]'"]
        set result [lindex $result 0]

        # Validate password.
        if {![llength $result] || ![CheckHash [lindex $result 1] $password]} {
            SendTheme $user invalidPass [list $ftpUser $user $host]
            return
        }

        # Validate IRC username.
        if {$userCheck && ![string equal -nocase [lindex $result 0] $user]} {
            SendTheme $user invalidUser [list $ftpUser $user $host]
            return
        }

        # Look-up the users groups and flags.
        if {[UserInfo $ftpUser uinfo]} {
            GetFlagTypes type

            if {[PermMatchFlags $uinfo(flags) $type(deleted)]} {
                # Remove the user's invite record if they are deleted.
                db "DELETE FROM invite_users WHERE ftp_user='$ftpUserEsc'"
                db "DELETE FROM invite_hosts WHERE ftp_user='$ftpUserEsc'"

                SendTheme $user invalidUser [list $ftpUser $user $host]
            } else {
                set ftpGroup [lindex $uinfo(groups) 0]
                Process $user $host $ftpUser $ftpGroup $uinfo(groups) $uinfo(flags)
            }
        }
    } else {
        SendTheme $user databaseDown [list $ftpUser $user]
    }
}

####
# ChanEvent
#
# Updates the online status of IRC users.
#
proc ::Bot::Mod::Invite::ChanEvent {event args} {
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
            # When an IRC user changes their nickname, their online status is
            # set to offline instead of updating the nickname. This is done to
            # prevent an exploit and race condition introduced when multiple
            # bots attempt to change a user's IRC nickname.
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
proc ::Bot::Mod::Invite::LogEvent {event destSection pathSection path data} {
    variable hostCheck

    if {$event eq "INVITE"} {
        if {[llength $data] != 5} {
            LogError ModInvite "Invalid number of items in log data \"$data\"."
            return 1
        }
        foreach {ftpUser ftpGroup ftpGroupList ftpFlags ircUser} $data {break}

        if {$hostCheck} {
            variable whois
            set whois($ircUser) [list $ftpUser $ftpGroup $ftpGroupList $ftpFlags]
            putquick "WHOIS $ircUser"
        } else {
            Process $ircUser "disabled@disabled" $ftpUser $ftpGroup $ftpGroupList $ftpFlags
        }
    } elseif {$event eq "DELUSER" || $event eq "PURGED"} {
        # Remove invite record when a user is deleted or purged.
        set ftpUserEsc [SqlEscape [lindex $data 1]]
        if {[DbConnect]} {
            db "DELETE FROM invite_users WHERE ftp_user='$ftpUserEsc'"
            db "DELETE FROM invite_hosts WHERE ftp_user='$ftpUserEsc'"
        }
    } else {
        LogError ModInvite "Unknown log event \"$event\"."
    }
    return 0
}

####
# Whois
#
# Raw event callback, executed when Eggdrop receives a "311" reply.
#
proc ::Bot::Mod::Invite::Whois {server code text} {
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
        foreach {ftpUser ftpGroup ftpGroupList ftpFlags} $whois($ircUser) {break}
        unset whois($ircUser)
        Process $ircUser "$ident@$host" $ftpUser $ftpGroup $ftpGroupList $ftpFlags
    }
    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::Invite::Load {firstLoad} {
    variable channels
    variable cmdToken
    variable dataSource
    variable hostCheck
    variable userCheck
    variable warnSection
    upvar ::Bot::configHandle configHandle
    upvar ::Bot::chanSections chanSections
    upvar ::Bot::pathSections pathSections

    if {$firstLoad} {
        package require tclodbc
    }

    # Retrieve configuration options.
    array set option [Config::GetMulti $configHandle Module::Invite \
        channels dataSource hostCheck userCheck warnSection]
    set dataSource  $option(dataSource)
    set warnSection $option(warnSection)
    set hostCheck   [IsTrue $option(hostCheck)]
    set userCheck   [IsTrue $option(userCheck)]

    # Check if the defined section exists.
    if {$warnSection ne "" && ![info exists chanSections($warnSection)] && ![info exists pathSections($warnSection)]} {
        LogError ModInvite "Invalid channel section \"$warnSection\"."
        set warnSection ""
    }

    # Parse invite channels.
    unset -nocomplain channels
    foreach entry [ListParse $option(channels)] {
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

    set cmdToken [CmdCreate private invite [namespace current]::Command \
        -args "<FTP user> <invite password>" \
        -category "General" -desc "Invite yourself into the channel."]

    # Register event callbacks.
    bind raw  -|- 311 [namespace current]::Whois
    bind join -|- "*" [list [namespace current]::ChanEvent JOIN]
    bind kick -|- "*" [list [namespace current]::ChanEvent KICK]
    bind nick -|- "*" [list [namespace current]::ChanEvent NICK]
    bind part -|- "*" [list [namespace current]::ChanEvent PART]
    bind sign -|- "*" [list [namespace current]::ChanEvent QUIT]
    ScriptRegister pre INVITE  [namespace current]::LogEvent True
    ScriptRegister pre DELUSER [namespace current]::LogEvent True
    ScriptRegister pre PURGED  [namespace current]::LogEvent True

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
proc ::Bot::Mod::Invite::Unload {} {
    variable cmdToken
    CmdRemoveByToken $cmdToken

    # Remove event callbacks.
    catch {unbind raw  -|- 311 [namespace current]::Whois}
    catch {unbind join -|- "*" [list [namespace current]::ChanEvent JOIN]}
    catch {unbind kick -|- "*" [list [namespace current]::ChanEvent KICK]}
    catch {unbind nick -|- "*" [list [namespace current]::ChanEvent NICK]}
    catch {unbind part -|- "*" [list [namespace current]::ChanEvent PART]}
    catch {unbind sign -|- "*" [list [namespace current]::ChanEvent QUIT]}
    ScriptUnregister pre INVITE  [namespace current]::LogEvent
    ScriptUnregister pre DELUSER [namespace current]::LogEvent
    ScriptUnregister pre PURGED  [namespace current]::LogEvent

    # Close ODBC connection.
    catch {db disconnect}
}
