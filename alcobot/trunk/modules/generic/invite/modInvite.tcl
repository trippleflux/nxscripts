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
        variable dbHandle ""
        variable hostCheck 0
        variable userCheck 0
        variable warnSection ""
    }
    namespace import -force ::Bot::*
    namespace import -force ::Bot::Mod::Ftpd::*
    namespace export GetFtpUser GetIrcUser
}

####
# GetFtpUser
#
# Looks up the FTP user-name for the given IRC nick-name.
#
proc ::Bot::Mod::Invite::GetFtpUser {ircUser} {
    variable dbHandle
    if {![Db::GetStatus $dbHandle]} {
        error "invite database is offline"
    }

    # IRC nick-names are NOT case-senstive.
    set query {SELECT [Name ftp_user] FROM [Name invite_users] WHERE online=1 \
        AND UPPER([Name irc_user])=UPPER([String $ircUser]) LIMIT 1}
    set result [Db::Select $dbHandle -list $query]

    if {[llength $result]} {
        return [lindex $result 0]
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
    variable dbHandle
    if {![Db::GetStatus $dbHandle]} {
        error "invite database is offline"
    }

    # FTP user-names are case-senstive.
    set query {SELECT [Name irc_user] FROM [Name invite_users] WHERE online=1 \
        AND [Name ftp_user]=[String $ftpUser] LIMIT 1}
    set result [Db::Select $dbHandle -list $query]

    if {[llength $result]} {
        return [lindex $result 0]
    } else {
        error "unknown FTP user, please re-invite yourself"
    }
}

####
# CheckHash
#
# Checks if a password matches the given hash.
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
# DbNotify
#
# Called when the connection succeeds or fails.
#
proc ::Bot::Mod::Invite::DbNotify {handle success} {
    if {!$success} {
        LogInfo "Database connection failed - [Db::GetError $handle]"
        return
    }
    LogInfo "Database connection established."
    set tables [Db::Info $handle tables]

    if {[lsearch -exact $tables "invite_hosts"] == -1} {
        set query {CREATE TABLE [Name invite_hosts] (
            [Column ftp_user varchar(100) -notnull],
            [Column hostmask varchar(255) -notnull],
            PRIMARY KEY ([Name ftp_user hostmask])
        )}
        LogInfo "Creating the \"invite_hosts\" table."
        Db::Exec $handle $query
    }

    if {[lsearch -exact $tables "invite_users"] == -1} {
        set query {CREATE TABLE [Name invite_users] (
            [Column ftp_user varchar(100) -notnull],
            [Column irc_user varchar(100) -default NULL],
            [Column online   smallint(1)  -default 0 -notnull],
            [Column password varchar(100) -default NULL],
            [Column time     int          -default 0 -notnull],
            PRIMARY KEY ([Name ftp_user])
        )}
        LogInfo "Creating the \"invite_users\" table."
        Db::Exec $handle $query
    }
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
    variable dbHandle
    variable hostCheck
    variable userCheck

    set time [clock seconds]
    if {![Db::GetStatus $dbHandle]} {
        SendTheme $ircUser databaseDown [list $ftpUser $ircUser]
        return
    }

    if {$hostCheck} {
        # Validate IRC host.
        set valid 0
        set query {SELECT [Name hostmask] FROM [Name invite_hosts] \
            WHERE [Name ftp_user]=[String $ftpUser]}

        foreach hostMask [Db::Select $dbHandle -list $query] {
            if {[string match -nocase $hostMask $ircHost]} {
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
    set query {UPDATE [Name invite_users] SET }
    if {!$userCheck} {
        append query {[Name irc_user]=[String $ircUser], }
    }
    append query {[Name online]=$online, [Name time]=$time WHERE [Name ftp_user]=[String $ftpUser]}
    Db::Exec $dbHandle $query

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
    variable dbHandle
    variable userCheck

    if {[llength $argv] != 2} {throw CMDHELP}
    set ftpUser [lindex $argv 0]
    set password [lindex $argv 1]

    if {[Db::GetStatus $dbHandle]} {
        set query {SELECT [Name irc_user password] FROM [Name invite_users] \
            WHERE [Name ftp_user]=[String $ftpUser]}
        set result [Db::Select $dbHandle -list $query]

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
                SendTheme $user invalidUser [list $ftpUser $user $host]

                # Remove the user's invite record if they are deleted.
                set query {DELETE FROM [Name invite_hosts] WHERE [Name ftp_user]=[String $ftpUser]; \
                           DELETE FROM [Name invite_users] WHERE [Name ftp_user]=[String $ftpUser];}

                if {[catch {Db::Exec $dbHandle $query} message]} {
                    LogError ModInvite $message
                }
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
    variable dbHandle

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
    if {$valid && [Db::GetStatus $dbHandle]} {
        set query {UPDATE [Name invite_users] SET [Name online]=$online, \
            [Name time]=$time WHERE UPPER([Name irc_user])=UPPER([String $user])}

        if {[catch {Db::Exec $dbHandle $query} message]} {
            LogError ModInvite $message
        }
    }
    return
}

####
# LogEvent
#
# Handle "INVITE" log events.
#
proc ::Bot::Mod::Invite::LogEvent {event destSection pathSection path data} {
    variable dbHandle
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
        set ftpUser [lindex $data 1]
        if {[Db::GetStatus $dbHandle]} {
            set query {DELETE FROM [Name invite_hosts] WHERE [Name ftp_user]=[String $ftpUser]; \
                       DELETE FROM [Name invite_users] WHERE [Name ftp_user]=[String $ftpUser];}

            if {[catch {Db::Exec $dbHandle $query} message]} {
                LogError ModInvite $message
            }
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
    variable dbHandle
    variable hostCheck
    variable userCheck
    variable warnSection
    upvar ::Bot::configHandle configHandle
    upvar ::Bot::chanSections chanSections
    upvar ::Bot::pathSections pathSections

    # Retrieve configuration options.
    array set option [Config::GetMulti $configHandle Module::Invite \
        channels database hostCheck userCheck warnSection]
    set hostCheck  [IsTrue $option(hostCheck)]
    set userCheck  [IsTrue $option(userCheck)]

    # Check if the defined section exists.
    set warnSection $option(warnSection)
    if {$warnSection ne "" && ![info exists chanSections($warnSection)] && ![info exists pathSections($warnSection)]} {
        error "Invalid channel section \"$warnSection\"."
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
proc ::Bot::Mod::Invite::Unload {} {
    variable cmdToken
    variable dbHandle

    if {$dbHandle ne ""} {
        Db::Close $dbHandle
    }
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
}
