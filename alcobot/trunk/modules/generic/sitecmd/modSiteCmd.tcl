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

namespace eval ::alcoholicz::SiteCmd {
    namespace import -force ::alcoholicz::*
    namespace import -force ::alcoholicz::FtpDaemon::GetFtpConnection
}

####
# Command
#
# Channel command allowing IRC users to send SITE commands.
#
proc ::alcoholicz::SiteCmd::Command {command target user host handle channel argv} {
    if {[llength $argv] < 1} {throw CMDHELP}
    set name "SITE [join $argv]"
    SendTargetTheme $target siteHead [list $name]

    set connection [GetFtpConnection]
    if {[ftp::status $connection] != 2} {
        SendTargetTheme $target siteBody [list "Not connected to the FTP server."]
        return
    }

    ftp::command $connection $name [list [namespace current]::Callback $target]
}

####
# Callback
#
# SITE command callback, display the server's response.
#
proc ::alcoholicz::SiteCmd::Callback {target connection response} {
    set code [lindex $response end-1]
    if {[string index $code 0] == 2} {
        set message "Command successful."
    } else {
        set message "Command failed."
    }

    SendTargetTheme $target siteBody [list $message]
    SendTargetTheme $target siteFoot
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::SiteCmd::Load {firstLoad} {
    upvar ::alcoholicz::configHandle configHandle

    if {[config::exists $configHandle Module::SiteCmd cmdPrefix]} {
        set prefix [config::get $configHandle Module::SiteCmd cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    CmdCreate channel site [namespace current]::Command \
        -category "Admin" -args "<command>" \
        -prefix   $prefix -desc "Issue a SITE command."
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::SiteCmd::Unload {} {
}
