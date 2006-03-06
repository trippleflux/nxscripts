#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Free Module
#
# Author:
#   neoxed (neoxed@gmail.com) Aug 25, 2005
#
# Abstract:
#   Implements a module to display available drive space.
#

namespace eval ::alcoholicz::Free {
    if {![info exists [namespace current]::volumeList]} {
        variable volumeList [list]
    }
    namespace import -force ::alcoholicz::*
}

####
# Command
#
# Implements a channel command to display available drive space.
#
proc ::alcoholicz::Free::Command {command target user host handle channel argv} {
    variable volumeList

    set argc [llength $argv]
    if {$argc > 1} {throw CMDHELP}

    # Only display the header and footer if no section was specified.
    if {!$argc} {
        SendTargetTheme $target freeHead
    }

    set count 0; set free 0; set used 0; set total 0
    foreach {volume sections} $volumeList {
        if {$argc && ![ListExists $sections [lindex $argv 0]]} {continue}

        if {[catch {volume info $volume info} message]} {
            LogError ModFree $message; continue
        }
        if {$info(total) == 0} {
            LogError ModFree "Invalid volume \"$volume\": the total size is zero."
            continue
        }

        set percentFree [expr {(double($info(free)) / double($info(total))) * 100.0}]
        set percentUsed [expr {(double($info(used)) / double($info(total))) * 100.0}]

        SendTargetTheme $target freeBody [list $info(free) $info(used) $info(total) \
            $percentFree $percentUsed [join $sections]]

        # Update volume totals.
        incr count
        set free [expr {wide($free) + $info(free)}]
        set used [expr {wide($used) + $info(used)}]
        set total [expr {wide($total) + $info(total)}]
    }

    if {!$argc} {
        if {$total} {
            set percentFree [expr {(double($free) / double($total)) * 100.0}]
            set percentUsed [expr {(double($used) / double($total)) * 100.0}]
        } else {
            set percentFree 0.0; set percentUsed 0.0
        }

        SendTargetTheme $target freeFoot [list $free $used $total \
            $percentFree $percentUsed $count]
    }
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::Free::Load {firstLoad} {
    variable volumeList
    upvar ::alcoholicz::configHandle configHandle

    set volumeList [list]
    foreach {name value} [config::getex $configHandle Module::Free] {
        lappend volumeList $name $value
    }

    CmdCreate channel free [namespace current]::Command \
        -aliases  "df"      -args "\[section\]" \
        -category "General" -desc "Display free disk space."
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Free::Unload {} {
}
