#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Groups Module
#
# Author:
#   neoxed (neoxed@gmail.com) June 14, 2005
#
# Abstract:
#   Implements a module to manage affiliated and banned groups.
#

namespace eval ::alcoholicz::Groups {
    namespace import -force ::alcoholicz::*
}

####
# ChangeAffils
#
# Implements a channel command to add or remove affiliated groups.
#
proc ::alcoholicz::Groups::ChangeAffils {event user host handle channel target argc argv} {
    variable groupsHandle
    if {$argc != 2} {
        # Channel commands should display the usage message in the
        # channel they were invoked from, not the output target.
        SendTargetTheme "PRIVMSG $channel" commandHelp [list $::lastbind "<section> <group>"]
        return
    }
    ConfigRead $groupsHandle

    set section [lindex $argv 0]
    set group [lindex $argv 1]
    set groupList [ConfigGet $groupsHandle Affils $section]

    if {$event eq "ADD"} {
        if {[lsearch -exact $groupList $group] != -1} {
            SendTarget $target "[b]$group[b] already exists in the [b]$section[b] affil list."
            return
        }

        # Add group to the section's affil list.
        ConfigSet $groupsHandle Affils $section [lappend groupList $group]
        if {[catch {ConfigWrite $groupsHandle} message]} {
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
        ConfigSet $groupsHandle Affils $section [lreplace $groupList $index $index]
        if {[catch {ConfigWrite $groupsHandle} message]} {
            SendTarget $target "Unable to update groups file: $message"
        } else {
            SendTarget $target "Removed [b]$group[b] from the [b]$section[b] affil list."
        }
    } else {
        LogError ModGroups "Unknown affil event \"$event\"."
    }

    ConfigFree $groupsHandle
}

####
# ChangeBanned
#
# Implements a channel command to add or remove banned groups.
#
proc ::alcoholicz::Groups::ChangeBanned {event user host handle channel target argc argv} {
    variable groupsHandle
    if {$argc != 2} {
        SendTargetTheme "PRIVMSG $channel" commandHelp [list $::lastbind "<section> <group>"]
        return
    }
    ConfigRead $groupsHandle

    set section [lindex $argv 0]
    set group [lindex $argv 1]
    set groupList [ConfigGet $groupsHandle Banned $section]

    if {$event eq "ADD"} {
        if {[lsearch -exact $groupList $group] != -1} {
            SendTarget $target "[b]$group[b] already exists in the [b]$section[b] ban list."
            return
        }

        # Add group to the section's ban list.
        ConfigSet $groupsHandle Banned $section [lappend groupList $group]
        if {[catch {ConfigWrite $groupsHandle} message]} {
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
        ConfigSet $groupsHandle Banned $section [lreplace $groupList $index $index]
        if {[catch {ConfigWrite $groupsHandle} message]} {
            SendTarget $target "Unable to update groups file: $message"
        } else {
            SendTarget $target "Removed [b]$group[b] from the [b]$section[b] ban list."
        }
    } else {
        LogError ModGroups "Unknown banned event \"$event\"."
    }

    ConfigFree $groupsHandle
}

####
# ListAffils
#
# Implements a channel command to display affiliated groups.
#
proc ::alcoholicz::Groups::ListAffils {user host handle channel target argc argv} {
    variable groupsHandle
    ConfigRead $groupsHandle
    set sections [lsort [ConfigKeys $groupsHandle Affils]]
    SendTargetTheme $target affilsHead

    set groupList [list]
    foreach section $sections {
        set groups [ConfigGet $groupsHandle Affils $section]
        SendTargetTheme $target affilsBody [list [join [lsort $groups]] $section]
        eval lappend groupList $groups
    }

    set groupList [lsort -unique $groupList]
    SendTargetTheme $target affilsFoot [list [llength $groupList] [llength $sections]]
    ConfigFree $groupsHandle
}

####
# ListBanned
#
# Implements a channel command to display banned groups.
#
proc ::alcoholicz::Groups::ListBanned {user host handle channel target argc argv} {
    variable groupsHandle
    ConfigRead $groupsHandle
    set sections [lsort [ConfigKeys $groupsHandle Banned]]
    SendTargetTheme $target bannedHead

    set groupList [list]
    foreach section $sections {
        set groups [ConfigGet $groupsHandle Banned $section]
        SendTargetTheme $target bannedBody [list [join [lsort $groups]] $section]
        eval lappend groupList $groups
    }

    set groupList [lsort -unique $groupList]
    SendTargetTheme $target bannedFoot [list [llength $groupList] [llength $sections]]
    ConfigFree $groupsHandle
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
        [ConfigGet $configHandle Module::Groups groupsFile]]

    if {![file isfile $groupsFile]} {
        error "the file \"$groupsFile\" does not exist"
    }

    if {$firstLoad} {
        set groupsHandle [ConfigOpen $groupsFile -align 2]
    } else {
        ConfigChange $groupsHandle -path $groupsFile
    }

    # Create related commands.
    if {[ConfigExists $configHandle Module::Groups cmdPrefix]} {
        set prefix [ConfigGet $configHandle Module::Groups cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    # User commands (list groups).
    CmdCreate channel ${prefix}affils  [namespace current]::ListAffils \
        General "List affiliated groups."

    CmdCreate channel ${prefix}banned  [namespace current]::ListBanned \
        General "List banned groups."

    # Administration commands (add/remove groups).
    CmdCreate channel ${prefix}addaffil [list [namespace current]::ChangeAffils ADD] \
        Admin "Add an affiliated group." "<section> <group>"

    CmdCreate channel ${prefix}delaffil [list [namespace current]::ChangeAffils DEL] \
        Admin "Removed an affiliated group." "<section> <group>"

    CmdCreate channel ${prefix}addban   [list [namespace current]::ChangeBanned ADD] \
        Admin "Add a banned group." "<section> <group>"

    CmdCreate channel ${prefix}delban   [list [namespace current]::ChangeBanned DEL] \
        Admin "Removed a banned group." "<section> <group>"

    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Groups::Unload {} {
    variable groupsHandle
    if {[info exists groupsHandle]} {
        ConfigClose $groupsHandle
    }
    return
}
