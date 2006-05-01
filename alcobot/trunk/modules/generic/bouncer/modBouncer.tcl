#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
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
    if {![info exists [namespace current]::checkIndex]} {
        variable checkIndex 0
        variable cmdToken ""
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
        foreach {name host port status time connection} $bouncers($index) {break}

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
# CheckTimer
#
# Checks the status of a bouncer every minute.
#
proc ::Bot::Mod::Bouncer::CheckTimer {} {
    variable bouncers
    variable checkIndex
    variable timerId

    if {[info exists bouncers($checkIndex)]} {
        set connection [lindex $bouncers($checkIndex) 5]

        if {[catch {Ftp::Connect $connection} message]} {
            Notify $checkIndex $connection 0
        }
    }
    incr checkIndex

    # Reset the index if we're out of bounds.
    if {![info exists bouncers($checkIndex)]} {
        set checkIndex 0
    }

    set timerId [timer 1 [namespace current]::CheckTimer]
    return
}

####
# Notify
#
# Notified by the FTP library when the connection succeeds or fails.
#
proc ::Bot::Mod::Bouncer::Notify {index connection success} {
    variable bouncers

    if {[info exists bouncers($index)]} {
        if {!$success} {
            set name [lindex $bouncers($index) 0]
            set status 1
            LogError ModBouncer "Bouncer \"$name\" is down: [Ftp::GetError $connection]"
        } else {
            set status 2
        }

        # Update bouncer status and time stamp.
        lset bouncers($index) 3 $status
        lset bouncers($index) 4 [clock seconds]
    }

    Ftp::Disconnect $connection
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::Bouncer::Load {firstLoad} {
    variable bouncers
    variable checkIndex
    variable cmdToken
    variable timerId
    upvar ::Bot::configHandle configHandle

    set index 0
    foreach {name value} [Config::GetEx $configHandle Module::Bouncer] {
        if {$name eq "cmdPrefix"} {continue}

        # Values: <host> <port> <user> <password> [secure]
        set value [ListParse $value]
        foreach {host port user passwd secure} $value {break}
        if {$secure eq ""} {set secure "none"}

        if {([llength $value] != 4 && [llength $value] != 5) ||
                ![string is digit -strict $port] ||
                [lsearch -exact {"" none ssl tls} $secure] == -1} {
            LogError ModBouncer "Invalid options for bouncer \"$name\"."
            continue
        }

        set connection [Ftp::Open $host $port $user $passwd -secure $secure \
            -notify [list [namespace current]::Notify $index]]

        # Values: <name> <host> <port> <status> <time> <connection>
        # Status: 0=unknown, 1=down, and 2=up.
        set bouncers($index) [list $name $host $port 0 0 $connection]
        incr index
    }
    if {!$index} {error "no bouncers defined"}

    set cmdToken [CmdCreate channel bnc [namespace current]::Command \
        -category "General" -desc "Display bouncer status."]

    # Reset the bouncer check index.
    set checkIndex 0
    if {$firstLoad} {
        set timerId [timer 1 [namespace current]::CheckTimer]
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
