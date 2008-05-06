#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2008 Alcoholicz Scripting Team
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
    # Parse command options.
    set argv [GetOpt::Parse $argv {quiet} option]
    if {[llength $argv] < 1} {throw CMDHELP}
    set quiet [info exists option(quiet)]

    set name "SITE [join $argv]"
    SendTargetTheme $target Module::SiteCmd head [list $name]

    set connection [GetFtpConnection]
    if {[Ftp::GetStatus $connection] != 2} {
        SendTargetTheme $target Module::SiteCmd body [list "Not connected to the FTP server."]
        SendTargetTheme $target Module::SiteCmd foot
        return
    }

    Ftp::Command $connection $name [list [namespace current]::Callback $quiet $target]
}

####
# Callback
#
# SITE command callback, display the server's response.
#
proc ::Bot::Mod::SiteCmd::Callback {quiet target connection response} {
    if {$quiet} {
        # Only display the success/failure message.
        set response [lrange $response end-1 end]
    }
    foreach {code message} $response {
        SendTargetTheme $target Module::SiteCmd body [list $message]
    }
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
        -args "\[-quiet\] <command>" -category "Admin" -desc "Issue a SITE command."]
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
