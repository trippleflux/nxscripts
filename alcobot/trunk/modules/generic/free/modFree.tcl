#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Free Space Module
#
# Author:
#   neoxed (neoxed@gmail.com) Aug 25, 2005
#
# Abstract:
#   Implements a module to display available drive space.
#

namespace eval ::alcoholicz::Free {
    if {![info exists volumeList]} {
        variable volumeList [list]
    }
    namespace import -force ::alcoholicz::*
}

####
# Command
#
# Implements a channel command to display available drive space.
#
proc ::alcoholicz::Free::Command {user host handle channel target argc argv} {
    variable volumeList

    if {$argc > 1} {
        SendTargetTheme $target commandHelp [list $::lastbind "\[section\]"]
        return
    }

    # Only display the header and footer if no section was specified.
    if {!$argc} {
        SendTargetTheme $target freeHead
    }

    set count 0; set free 0; set used 0; set total 0
    foreach {volume sections} $volumeList {
        if {$argc == 1 && [lsearch -exact $sections [lindex $argv 0]] == -1} {continue}

        if {[catch {volume info $volume info} message]} {
            LogError ModFree $message; continue
        }
        set percentFree [expr {(double($info(free)) / double($info(total))) * 100}]
        set percentUsed [expr {(double($info(used)) / double($info(total))) * 100}]
        SendTargetTheme $target freeBody [list $info(free) $info(used) $info(total) $percentFree $percentUsed [join $sections]]

        # Update volume totals.
        incr count
        set free [expr {wide($free) + $info(free)}]
        set used [expr {wide($used) + $info(used)}]
        set total [expr {wide($total) + $info(total)}]
    }

    if {!$argc} {
        if {$total} {
            set percentFree [expr {(double($free) / double($total)) * 100}]
            set percentUsed [expr {(double($used) / double($total)) * 100}]
        } else {
            set percentFree 0.0; set percentUsed 0.0
        }

        SendTargetTheme $target freeFoot [list $free $used $total $percentFree $percentUsed $count]
    }
    return
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
    foreach {name value} [ConfigGetEx $configHandle Module::Free] {
        # Ignore other options.
        if {$name ne "cmdPrefix"} {
            lappend volumeList $name $value
        }
    }

    # Create related commands.
    if {[ConfigExists $configHandle Module::Free cmdPrefix]} {
        set prefix [ConfigGet $configHandle Module::Free cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    CmdCreate - ${prefix}df   [namespace current]::Command
    CmdCreate - ${prefix}free [namespace current]::Command
    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Free::Unload {} {
    return
}
