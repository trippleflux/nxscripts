#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2008 Alcoholicz Scripting Team
#
# Module Name:
#   FTP Connection Module
#
# Author:
#   neoxed (neoxed@gmail.com) Mar 11, 2005
#
# Abstract:
#   Maintains a connection to the FTP server.
#
# Exported Procedures:
#   GetFtpConnection
#

namespace eval ::Bot::Mod::FtpConn {
    if {![info exists [namespace current]::handle]} {
        variable handle ""
        variable timerId ""
    }
    namespace import -force ::Bot::*
    namespace export GetFtpConnection
}

####
# GetFtpConnection
#
# Retrieves the main FTP connection handle.
#
proc ::Bot::Mod::FtpConn::GetFtpConnection {} {
    variable handle
    return $handle
}

####
# Notify
#
# Called when the initial connection succeeds or fails.
#
proc ::Bot::Mod::FtpConn::Notify {handle success} {
    if {$success} {
        LogInfo "FTP connection established."
    } else {
        LogInfo "FTP connection failed - [Ftp::GetError $handle]"
    }
}

####
# Timer
#
# Checks the status of the FTP connection every minute.
#
proc ::Bot::Mod::FtpConn::Timer {} {
    variable handle
    variable timerId

    # Wrap the FTP code in a catch statement in case the library
    # throws an error. The Eggdrop timer must be recreated.
    if {[catch {
        if {[Ftp::GetStatus $handle] == 2} {
            Ftp::Command $handle "NOOP"
        } else {
            LogError FtpServer "FTP handle not connected, attemping to reconnect."
            Ftp::Connect $handle
        }
    } message]} {
        LogError FtpServer $message
    }

    set timerId [timer 1 [namespace current]::Timer]
    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::FtpConn::Load {firstLoad} {
    variable handle
    variable timerId
    upvar ::Bot::configHandle configHandle

    # Retrieve configuration options.
    set server [Config::Get $configHandle Module::FtpConn server]

    # Open a connection to the FTP server.
    if {$firstLoad} {
        set timerId [timer 1 [namespace current]::Timer]
    } else {
        Ftp::Close $handle
    }
    set handle [Ftp::Open $server -debug ::Bot::LogDebug \
        -notify [namespace current]::Notify]
    Ftp::Connect $handle
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::FtpConn::Unload {} {
    variable handle
    variable timerId

    if {$handle ne ""} {
        Ftp::Close $handle
    }
    if {$timerId ne ""} {
        catch {killtimer $timerId}
    }
}
