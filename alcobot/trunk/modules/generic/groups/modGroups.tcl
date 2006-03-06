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
proc ::alcoholicz::Groups::ChangeAffils {event target user host channel argv} {
    variable groupsHandle

    if {[llength $argv] != 2} {throw CMDHELP}
    set section [lindex $argv 0]
    set group [lindex $argv 1]

    Config::Read $groupsHandle
    set groupList [Config::Get $groupsHandle Affils $section]

    if {$event eq "ADD"} {
        if {[lsearch -exact $groupList $group] != -1} {
            SendTarget $target "[b]$group[b] already exists in the [b]$section[b] affil list."
            return
        }

        # Add group to the section's affil list.
        Config::Set $groupsHandle Affils $section [lappend groupList $group]
        if {[catch {Config::Write $groupsHandle} message]} {
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
        Config::Set $groupsHandle Affils $section [lreplace $groupList $index $index]
        if {[catch {Config::Write $groupsHandle} message]} {
            SendTarget $target "Unable to update groups file: $message"
        } else {
            SendTarget $target "Removed [b]$group[b] from the [b]$section[b] affil list."
        }
    } else {
        LogError ModGroups "Unknown affil event \"$event\"."
    }

    Config::Free $groupsHandle
}

####
# ChangeBanned
#
# Implements a channel command to add or remove banned groups.
#
proc ::alcoholicz::Groups::ChangeBanned {event target user host channel argv} {
    variable groupsHandle

    if {[llength $argv] != 2} {throw CMDHELP}
    set section [lindex $argv 0]
    set group [lindex $argv 1]

    Config::Read $groupsHandle
    set groupList [Config::Get $groupsHandle Banned $section]

    if {$event eq "ADD"} {
        if {[lsearch -exact $groupList $group] != -1} {
            SendTarget $target "[b]$group[b] already exists in the [b]$section[b] ban list."
            return
        }

        # Add group to the section's ban list.
        Config::Set $groupsHandle Banned $section [lappend groupList $group]
        if {[catch {Config::Write $groupsHandle} message]} {
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
        Config::Set $groupsHandle Banned $section [lreplace $groupList $index $index]
        if {[catch {Config::Write $groupsHandle} message]} {
            SendTarget $target "Unable to update groups file: $message"
        } else {
            SendTarget $target "Removed [b]$group[b] from the [b]$section[b] ban list."
        }
    } else {
        LogError ModGroups "Unknown banned event \"$event\"."
    }

    Config::Free $groupsHandle
}

####
# ListAffils
#
# Implements a channel command to display affiliated groups.
#
proc ::alcoholicz::Groups::ListAffils {target user host channel argv} {
    variable groupsHandle
    Config::Read $groupsHandle
    set sections [lsort [Config::Keys $groupsHandle Affils]]
    SendTargetTheme $target affilsHead

    set groupList [list]
    foreach section $sections {
        set groups [Config::Get $groupsHandle Affils $section]
        SendTargetTheme $target affilsBody [list [join [lsort $groups]] $section]
        eval lappend groupList $groups
    }

    set groupList [lsort -unique $groupList]
    SendTargetTheme $target affilsFoot [list [llength $groupList] [llength $sections]]
    Config::Free $groupsHandle
}

####
# ListBanned
#
# Implements a channel command to display banned groups.
#
proc ::alcoholicz::Groups::ListBanned {target user host channel argv} {
    variable groupsHandle
    Config::Read $groupsHandle
    set sections [lsort [Config::Keys $groupsHandle Banned]]
    SendTargetTheme $target bannedHead

    set groupList [list]
    foreach section $sections {
        set groups [Config::Get $groupsHandle Banned $section]
        SendTargetTheme $target bannedBody [list [join [lsort $groups]] $section]
        eval lappend groupList $groups
    }

    set groupList [lsort -unique $groupList]
    SendTargetTheme $target bannedFoot [list [llength $groupList] [llength $sections]]
    Config::Free $groupsHandle
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
        [Config::Get $configHandle Module::Groups groupsFile]]

    if {![file isfile $groupsFile]} {
        error "the file \"$groupsFile\" does not exist"
    }

    if {$firstLoad} {
        set groupsHandle [Config::Open $groupsFile -align 2]
    } else {
        Config::Change $groupsHandle -path $groupsFile
    }

    if {[Config::Exists $configHandle Module::Groups cmdPrefix]} {
        set prefix [Config::Get $configHandle Module::Groups cmdPrefix]
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
        Config::Close $groupsHandle
    }
}
