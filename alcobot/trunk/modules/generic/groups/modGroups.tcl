#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Groups Module
#
# Author:
#   neoxed (neoxed@gmail.com) Jun 14, 2005
#
# Abstract:
#   Implements a module to display affiliated and banned groups.
#

namespace eval ::alcoholicz::Groups {
    namespace import -force ::alcoholicz::*
}

####
# ChangeAffils
#
# Implements a channel command to add or remove affiliated groups.
#
proc ::alcoholicz::Groups::ChangeAffils {event command target user host handle channel argv} {
    variable groupsHandle

    if {[llength $argv] != 2} {throw CMDHELP}
    set section [lindex $argv 0]
    set group [lindex $argv 1]

    config::read $groupsHandle
    set groupList [config::get $groupsHandle Affils $section]

    if {$event eq "ADD"} {
        if {[lsearch -exact $groupList $group] != -1} {
            SendTarget $target "[b]$group[b] already exists in the [b]$section[b] affil list."
            return
        }

        # Add group to the section's affil list.
        config::set $groupsHandle Affils $section [lappend groupList $group]
        if {[catch {config::write $groupsHandle} message]} {
            SendTarget $target "Unable to update groups file: $message"
        } else {
            SendTarget $target "Added [b]$group[b] to the [b]$section[b] affil list."
        }
    } elseif {$event eq "DEL"} {
        if {[set index [lsearch -exact $groupList $group]] == -1} {
            SendTarget $target "[b]$group[b] is not in the [b]$section[b] affil list."
            return
        }

        # Remove group from the section's affil list.
        config::set $groupsHandle Affils $section [lreplace $groupList $index $index]
        if {[catch {config::write $groupsHandle} message]} {
            SendTarget $target "Unable to update groups file: $message"
        } else {
            SendTarget $target "Removed [b]$group[b] from the [b]$section[b] affil list."
        }
    } else {
        LogError ModGroups "Unknown affil event \"$event\"."
    }

    config::free $groupsHandle
}

####
# ChangeBanned
#
# Implements a channel command to add or remove banned groups.
#
proc ::alcoholicz::Groups::ChangeBanned {event command target user host handle channel argv} {
    variable groupsHandle

    if {[llength $argv] != 2} {throw CMDHELP}
    set section [lindex $argv 0]
    set group [lindex $argv 1]

    config::read $groupsHandle
    set groupList [config::get $groupsHandle Banned $section]

    if {$event eq "ADD"} {
        if {[lsearch -exact $groupList $group] != -1} {
            SendTarget $target "[b]$group[b] already exists in the [b]$section[b] ban list."
            return
        }

        # Add group to the section's ban list.
        config::set $groupsHandle Banned $section [lappend groupList $group]
        if {[catch {config::write $groupsHandle} message]} {
            SendTarget $target "Unable to update groups file: $message"
        } else {
            SendTarget $target "Added [b]$group[b] to the [b]$section[b] ban list."
        }
    } elseif {$event eq "DEL"} {
        if {[set index [lsearch -exact $groupList $group]] == -1} {
            SendTarget $target "[b]$group[b] is not in the [b]$section[b] ban list."
            return
        }

        # Remove group from the section's ban list.
        config::set $groupsHandle Banned $section [lreplace $groupList $index $index]
        if {[catch {config::write $groupsHandle} message]} {
            SendTarget $target "Unable to update groups file: $message"
        } else {
            SendTarget $target "Removed [b]$group[b] from the [b]$section[b] ban list."
        }
    } else {
        LogError ModGroups "Unknown banned event \"$event\"."
    }

    config::free $groupsHandle
}

####
# ListAffils
#
# Implements a channel command to display affiliated groups.
#
proc ::alcoholicz::Groups::ListAffils {command target user host handle channel argv} {
    variable groupsHandle
    config::read $groupsHandle
    set sections [lsort [config::keys $groupsHandle Affils]]
    SendTargetTheme $target affilsHead

    set groupList [list]
    foreach section $sections {
        set groups [config::get $groupsHandle Affils $section]
        SendTargetTheme $target affilsBody [list [join [lsort $groups]] $section]
        eval lappend groupList $groups
    }

    set groupList [lsort -unique $groupList]
    SendTargetTheme $target affilsFoot [list [llength $groupList] [llength $sections]]
    config::free $groupsHandle
}

####
# ListBanned
#
# Implements a channel command to display banned groups.
#
proc ::alcoholicz::Groups::ListBanned {command target user host handle channel argv} {
    variable groupsHandle
    config::read $groupsHandle
    set sections [lsort [config::keys $groupsHandle Banned]]
    SendTargetTheme $target bannedHead

    set groupList [list]
    foreach section $sections {
        set groups [config::get $groupsHandle Banned $section]
        SendTargetTheme $target bannedBody [list [join [lsort $groups]] $section]
        eval lappend groupList $groups
    }

    set groupList [lsort -unique $groupList]
    SendTargetTheme $target bannedFoot [list [llength $groupList] [llength $sections]]
    config::free $groupsHandle
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::Groups::Load {firstLoad} {
    variable groupsHandle
    upvar ::alcoholicz::configHandle configHandle

    # Open group configuration file.
    set groupsFile [file join $::alcoholicz::scriptPath \
        [config::get $configHandle Module::Groups groupsFile]]

    if {![file isfile $groupsFile]} {
        error "the file \"$groupsFile\" does not exist"
    }

    if {$firstLoad} {
        set groupsHandle [config::open $groupsFile -align 2]
    } else {
        config::change $groupsHandle -path $groupsFile
    }

    if {[config::exists $configHandle Module::Groups cmdPrefix]} {
        set prefix [config::get $configHandle Module::Groups cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    # User commands (list groups).
    CmdCreate channel affils [namespace current]::ListAffils \
        -category "General" -desc "List affiliated groups." -prefix $prefix

    CmdCreate channel banned [namespace current]::ListBanned \
        -category "General" -desc "List banned groups." -prefix $prefix

    # Administration commands (add/remove groups).
    CmdCreate channel addaffil [list [namespace current]::ChangeAffils ADD] \
        -category "Admin" -args "<section> <group>" \
        -prefix   $prefix -desc "Add an affiliated group."

    CmdCreate channel delaffil [list [namespace current]::ChangeAffils DEL] \
        -category "Admin" -args "<section> <group>" \
        -prefix   $prefix -desc "Remove an affiliated group."

    CmdCreate channel addban   [list [namespace current]::ChangeBanned ADD] \
        -category "Admin" -args "<section> <group>" \
        -prefix   $prefix -desc "Add a banned group."

    CmdCreate channel delban   [list [namespace current]::ChangeBanned DEL] \
        -category "Admin" -args "<section> <group>" \
        -prefix   $prefix -desc "Remove a banned group."
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Groups::Unload {} {
    variable groupsHandle
    if {[info exists groupsHandle]} {
        config::close $groupsHandle
    }
}
