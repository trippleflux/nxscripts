#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Bouncer Module
#
# Author:
#   neoxed (neoxed@gmail.com) Oct 15, 2005
#
# Abstract:
#   Implements a module to display bouncer status.
#

namespace eval ::alcoholicz::Bouncer {
    namespace import -force ::alcoholicz::*
}

####
# Command
#
# Display bouncer status.
#
proc ::alcoholicz::Bouncer::Command {command target user host handle channel argv} {
    variable bouncers
    SendTargetTheme $target bouncerHead

    foreach name [lsort [array names bouncers]] {
        foreach {host port user passwd secure} $bouncers($name) {break}

        # Connect to all bouncers.
        if {[catch {
            set connection [FtpOpen $host $port $user $passwd -secure $secure \
                -notify [list [namespace current]::Notify $target $name $host $port]]
            FtpConnect $connection
        } message]} {
            LogError ModBouncer $message\n$::errorInfo
            catch {FtpClose $connection}
        }
    }
}

####
# Notify
#
# Notified by the FTP library when the connection succeeds or fails.
#
proc ::alcoholicz::Bouncer::Notify {target name host port connection success} {
    if {$success} {
        set theme "bouncerUp"
    } else {
        set theme "bouncerDown"
        LogError ModBouncer "Bouncer \"$name\" is down: [FtpGetError $connection]."
    }

    SendTargetTheme $target $theme [list $name $host $port]
    FtpClose $connection
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::Bouncer::Load {firstLoad} {
    variable bouncers
    upvar ::alcoholicz::configHandle configHandle

    set volumeList [list]
    foreach {name value} [ConfigGetEx $configHandle Module::Bouncer] {
        if {$name eq "cmdPrefix"} {continue}

        # Values: <host> <port> <user> <password> [secure]
        set value [ArgsToList $value]
        if {([llength $value] != 4 && [llength $value] != 5) ||
                ![string is digit -strict [lindex $value 1]] ||
                [lsearch -exact {"" none ssl tls} [lindex $value 4]] == -1} {
            LogError ModBouncer "Invalid options for bouncer \"$name\"."
        }

        set bouncers($name) $value
    }

    # Create related commands.
    if {[ConfigExists $configHandle Module::Bouncer cmdPrefix]} {
        set prefix [ConfigGet $configHandle Module::Bouncer cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    CmdCreate channel bnc [namespace current]::Command \
        -category "General" -prefix $prefix -desc "Display bouncer status."

    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Bouncer::Unload {} {
    return
}
