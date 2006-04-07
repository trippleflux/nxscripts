#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Site Command Module
#
# Author:
#   neoxed (neoxed@gmail.com) Dec 27, 2005
#
# Abstract:
#   Implements a module to issue SITE commands from IRC.
#

namespace eval ::Bot::Mod::SiteCmd {
    if {![info exists [namespace current]::cmdToken]} {
        variable cmdToken ""
    }
    namespace import -force ::Bot::*
    namespace import -force ::Bot::Mod::FtpConn::GetFtpConnection
}

####
# Command
#
# Channel command allowing IRC users to send SITE commands.
#
proc ::Bot::Mod::SiteCmd::Command {target user host channel argv} {
    if {[llength $argv] < 1} {throw CMDHELP}
    set name "SITE [join $argv]"
    SendTargetTheme $target Module::SiteCmd head [list $name]

    set connection [GetFtpConnection]
    if {[Ftp::GetStatus $connection] != 2} {
        SendTargetTheme $target Module::SiteCmd body [list "Not connected to the FTP server."]
        return
    }

    Ftp::Command $connection $name [list [namespace current]::Callback $target]
}

####
# Callback
#
# SITE command callback, display the server's response.
#
proc ::Bot::Mod::SiteCmd::Callback {target connection response} {
    set code [lindex $response end-1]
    if {[string index $code 0] == 2} {
        set message "Command successful."
    } else {
        set message "Command failed."
    }

    SendTargetTheme $target Module::SiteCmd body [list $message]
    SendTargetTheme $target Module::SiteCmd foot
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::SiteCmd::Load {firstLoad} {
    variable cmdToken
    set cmdToken [CmdCreate channel site [namespace current]::Command \
        -args "<command>" -category "Admin" -desc "Issue a SITE command."]
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::SiteCmd::Unload {} {
    variable cmdToken
    CmdRemoveByToken $cmdToken
}
