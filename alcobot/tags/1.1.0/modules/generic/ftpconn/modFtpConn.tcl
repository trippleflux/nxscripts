#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
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
    if {![info exists [namespace current]::connection]} {
        variable connection ""
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
    variable connection
    return $connection
}

####
# Notify
#
# Called when the initial connection succeeds or fails.
#
proc ::Bot::Mod::FtpConn::Notify {connection success} {
    if {$success} {
        LogInfo "FTP connection established."
    } else {
        LogInfo "FTP connection failed - [Ftp::GetError $connection]"
    }
}

####
# Timer
#
# Checks the status of the FTP connection every minute.
#
proc ::Bot::Mod::FtpConn::Timer {} {
    variable connection
    variable timerId

    # Wrap the FTP connection code in a catch statement in case the FTP
    # library throws an error. The Eggdrop timer must be recreated.
    if {[catch {
        if {[Ftp::GetStatus $connection] == 2} {
            Ftp::Command $connection "NOOP"
        } else {
            LogError FtpServer "FTP handle not connected, attemping to reconnect."
            Ftp::Connect $connection
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
    variable connection
    variable timerId
    upvar ::Bot::configHandle configHandle

    # Retrieve configuration options.
    array set option [Config::GetMulti $configHandle Module::FtpConn \
        host port user passwd secure]

    # Open a connection to the FTP server.
    if {$firstLoad} {
        set timerId [timer 1 [namespace current]::Timer]
    } else {
        Ftp::Close $connection
    }
    set connection [Ftp::Open $option(host) $option(port) $option(user) $option(passwd) \
        -notify [namespace current]::Notify -secure $option(secure)]
    Ftp::Connect $connection
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::FtpConn::Unload {} {
    variable connection
    variable timerId

    if {$connection ne ""} {
        Ftp::Close $connection
        set connection ""
    }
    if {$timerId ne ""} {
        catch {killtimer $timerId}
        set timerId ""
    }
}
