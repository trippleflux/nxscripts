#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2008 Alcoholicz Scripting Team
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

namespace eval ::Bot::Mod::Bouncer {
    if {![info exists [namespace current]::cmdToken]} {
        variable cmdToken ""
        variable interval 1
        variable timerId ""
    }
    namespace import -force ::Bot::*
}

####
# Command
#
# Display bouncer status.
#
proc ::Bot::Mod::Bouncer::Command {target user host channel argv} {
    variable bouncers
    SendTargetTheme $target Module::Bouncer head
    set offline 0; set online 0; set unknown 0

    for {set index 0} {[info exists bouncers($index)]} {incr index} {
        foreach {name host port status time handle} $bouncers($index) {break}

        set values [list $name $host $port]
        switch -- $status {
            1 {
                lappend values [expr {[clock seconds] - $time}]
                set theme "offline"; incr offline
            }
            2 {
                lappend values [expr {[clock seconds] - $time}]
                set theme "online"; incr online
            }
            default {
                set theme "unknown"; incr unknown
            }
        }
        SendTargetTheme $target Module::Bouncer $theme $values
    }

    SendTargetTheme $target Module::Bouncer foot \
        [list $offline $online $unknown [expr {$offline + $online + $unknown}]]
}

####
# Notify
#
# Notified by the FTP library when the connection succeeds or fails.
#
proc ::Bot::Mod::Bouncer::Notify {index handle success} {
    variable bouncers

    if {[info exists bouncers($index)]} {
        if {!$success} {
            set name [lindex $bouncers($index) 0]
            set status 1
            LogError ModBouncer "Bouncer \"$name\" is down: [Ftp::GetError $handle]"
        } else {
            set status 2
        }

        # Update bouncer status and time stamp.
        lset bouncers($index) 3 $status
        lset bouncers($index) 4 [clock seconds]
    }

    Ftp::Disconnect $handle
}

####
# Timer
#
# Checks the status of a bouncer every minute.
#
proc ::Bot::Mod::Bouncer::Timer {index} {
    variable bouncers
    variable interval
    variable timerId

    if {[info exists bouncers($index)]} {
        set handle [lindex $bouncers($index) 5]

        if {[catch {Ftp::Connect $handle} message]} {
            Notify $index $handle 0
        }
    }
    incr index

    # Reset the index if we're out of bounds.
    if {![info exists bouncers($index)]} {
        set index 0
    }

    set timerId [timer $interval [list [namespace current]::Timer $index]]
    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::Bouncer::Load {firstLoad} {
    variable bouncers
    variable cmdToken
    variable interval
    variable timerId
    upvar ::Bot::configHandle configHandle

    set index 0
    foreach {name value} [Config::GetEx $configHandle Module::Bouncer] {
        set handle [Ftp::Open $value -debug ::Bot::LogDebug \
            -notify [list [namespace current]::Notify $index]]
        set host [Ftp::Change $handle -host]
        set port [Ftp::Change $handle -port]

        # Values: <name> <host> <port> <status> <time> <handle>
        # Status: 0=unknown, 1=down, and 2=up.
        set bouncers($index) [list $name $host $port 0 0 $handle]
        incr index
    }
    if {!$index} {error "no bouncers defined"}

    set cmdToken [CmdCreate channel bnc [namespace current]::Command \
        -category "General" -desc "Display bouncer status."]

    # Calculate the check interval (time inbetween checks). Allow a
    # threshold of 15 minutes for every 5 bouncers, this ensures the
    # interval is always at least 3 minutes.
    incr index
    set threshold [expr {(($index + 4) / 5) * 15}]
    set interval  [expr {$threshold / $index}]

    if {$firstLoad} {
        # Kick off the first check in one minute.
        set timerId [timer 1 [list [namespace current]::Timer 0]]
    }
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::Bouncer::Unload {} {
    variable bouncers
    variable cmdToken
    variable timerId

    CmdRemoveByToken $cmdToken

    foreach name [array names bouncers] {
        Ftp::Close [lindex $bouncers($name) 5]
    }
    unset -nocomplain bouncers

    if {$timerId ne ""} {
        catch {killtimer $timerId}
    }
}
