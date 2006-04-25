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

namespace eval ::Bot::Mod::Groups {
    if {![info exists [namespace current]::cmdTokens]} {
        variable cmdTokens [list]
        variable groupsHandle ""
    }
    namespace import -force ::Bot::*
}

####
# ChangeAffils
#
# Implements a channel command to add or remove affiliated groups.
#
proc ::Bot::Mod::Groups::ChangeAffils {event target user host channel argv} {
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
        set groupList [lreplace $groupList $index $index]
        if {[llength $groupList]} {
            Config::Set $groupsHandle Affils $section $groupList
        } else {
            Config::Unset $groupsHandle Affils $section
        }
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
proc ::Bot::Mod::Groups::ChangeBanned {event target user host channel argv} {
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
        set groupList [lreplace $groupList $index $index]
        if {[llength $groupList]} {
            Config::Set $groupsHandle Banned $section $groupList
        } else {
            Config::Unset $groupsHandle Banned $section
        }
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
proc ::Bot::Mod::Groups::ListAffils {target user host channel argv} {
    variable groupsHandle
    Config::Read $groupsHandle
    set sections [lsort [Config::Keys $groupsHandle Affils]]
    SendTargetTheme $target Module::Groups affilsHead

    set groupList [list]
    foreach section $sections {
        set groups [Config::Get $groupsHandle Affils $section]
        SendTargetTheme $target Module::Groups affilsBody [list [join [lsort $groups]] $section]
        eval lappend groupList $groups
    }

    set groupList [lsort -unique $groupList]
    SendTargetTheme $target Module::Groups affilsFoot [list [llength $groupList] [llength $sections]]
    Config::Free $groupsHandle
}

####
# ListBanned
#
# Implements a channel command to display banned groups.
#
proc ::Bot::Mod::Groups::ListBanned {target user host channel argv} {
    variable groupsHandle
    Config::Read $groupsHandle
    set sections [lsort [Config::Keys $groupsHandle Banned]]
    SendTargetTheme $target Module::Groups bannedHead

    set groupList [list]
    foreach section $sections {
        set groups [Config::Get $groupsHandle Banned $section]
        SendTargetTheme $target Module::Groups bannedBody [list [join [lsort $groups]] $section]
        eval lappend groupList $groups
    }

    set groupList [lsort -unique $groupList]
    SendTargetTheme $target Module::Groups bannedFoot [list [llength $groupList] [llength $sections]]
    Config::Free $groupsHandle
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::Groups::Load {firstLoad} {
    variable cmdTokens
    variable groupsHandle
    upvar ::Bot::configHandle configHandle

    # Open group configuration file.
    set groupsFile [file join $::Bot::scriptPath \
        [Config::Get $configHandle Module::Groups groupsFile]]

    if {![file isfile $groupsFile]} {
        error "the file \"$groupsFile\" does not exist"
    }

    if {$firstLoad} {
        set groupsHandle [Config::Open $groupsFile -align 2]
    } else {
        Config::Change $groupsHandle -path $groupsFile
    }
    set cmdTokens [list]

    # User commands (list groups).
    lappend cmdTokens [CmdCreate channel affils [namespace current]::ListAffils \
        -category "General" -desc "List affiliated groups."]

    lappend cmdTokens [CmdCreate channel banned [namespace current]::ListBanned \
        -category "General" -desc "List banned groups."]

    # Administration commands (add/remove groups).
    lappend cmdTokens [CmdCreate channel addaffil [list [namespace current]::ChangeAffils ADD] \
        -args "<section> <group>" \
        -category "Admin" -desc "Add an affiliated group."]

    lappend cmdTokens [CmdCreate channel delaffil [list [namespace current]::ChangeAffils DEL] \
        -args "<section> <group>" \
        -category "Admin" -desc "Remove an affiliated group."]

    lappend cmdTokens [CmdCreate channel addban [list [namespace current]::ChangeBanned ADD] \
        -args "<section> <group>" \
        -category "Admin" -desc "Add a banned group."]

    lappend cmdTokens [CmdCreate channel delban [list [namespace current]::ChangeBanned DEL] \
        -args "<section> <group>" \
        -category "Admin" -desc "Remove a banned group."]
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::Groups::Unload {} {
    variable cmdTokens
    variable groupsHandle

    foreach token $cmdTokens {
        CmdRemoveByToken $token
    }
    if {$groupsHandle ne ""} {
        Config::Close $groupsHandle
    }
}
