#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# File Name:
#   groups.tcl
#
# Author:
#   neoxed (neoxed@gmail.com) June 14, 2005
#
# Abstract:
#   Module to manage affiliated and banned groups.
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
        putserv "PRIVMSG $channel :Usage: $::lastbind <section> <group>"
        return
    }
    ConfigRead $groupsHandle

    set section [lindex $argv 0]
    set group [lindex $argv 1]
    set groupList [ConfigGet $groupsHandle Affils $section]

    if {$event eq "ADD"} {
        if {[lsearch -exact $groupList $group] != -1} {
            putserv "PRIVMSG $channel :[b]$group[b] already exists in the [b]$section[b] affil list."
            return
        }

        # Add group to the section's affil list.
        ConfigSet $groupsHandle Affils $section [lappend groupList $group]
        if {[catch {ConfigWrite $groupsHandle} message]} {
            putserv "PRIVMSG $channel :Unable to update groups file: $message"
        } else {
            putserv "PRIVMSG $channel :Added [b]$group[b] to the [b]$section[b] affil list."
        }
    } elseif {$event eq "DEL"} {
        if {[set index [lsearch -exact $groupList $group]] == -1} {
            putserv "PRIVMSG $channel :[b]$group[b] is not in the [b]$section[b] affil list."
            return
        }

        # Remove group from the section's affil list.
        ConfigSet $groupsHandle Affils $section [lreplace $groupList $index $index]
        if {[catch {ConfigWrite $groupsHandle} message]} {
            putserv "PRIVMSG $channel :Unable to update groups file: $message"
        } else {
            putserv "PRIVMSG $channel :Removed [b]$group[b] from the [b]$section[b] affil list."
        }
    } else {
        LogError ChangeAffils "Unknown event \"$event\"."
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
        putserv "PRIVMSG $channel :Usage: $::lastbind <section> <group>"
        return
    }
    ConfigRead $groupsHandle

    set section [lindex $argv 0]
    set group [lindex $argv 1]
    set groupList [ConfigGet $groupsHandle Banned $section]

    if {$event eq "ADD"} {
        if {[lsearch -exact $groupList $group] != -1} {
            putserv "PRIVMSG $channel :[b]$group[b] already exists in the [b]$section[b] ban list."
            return
        }

        # Add group to the section's ban list.
        ConfigSet $groupsHandle Banned $section [lappend groupList $group]
        if {[catch {ConfigWrite $groupsHandle} message]} {
            putserv "PRIVMSG $channel :Unable to update groups file: $message"
        } else {
            putserv "PRIVMSG $channel :Added [b]$group[b] to the [b]$section[b] ban list."
        }
    } elseif {$event eq "DEL"} {
        if {[set index [lsearch -exact $groupList $group]] == -1} {
            putserv "PRIVMSG $channel :[b]$group[b] is not in the [b]$section[b] ban list."
            return
        }

        # Remove group from the section's ban list.
        ConfigSet $groupsHandle Banned $section [lreplace $groupList $index $index]
        if {[catch {ConfigWrite $groupsHandle} message]} {
            putserv "PRIVMSG $channel :Unable to update groups file: $message"
        } else {
            putserv "PRIVMSG $channel :Removed [b]$group[b] from the [b]$section[b] ban list."
        }
    } else {
        LogError ChangeBanned "Unknown event \"$event\"."
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
# Module initialization procedure, called when the module is loaded.
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
    CmdCreate - ${prefix}affils  [namespace current]::ListAffils
    CmdCreate - ${prefix}affills [namespace current]::ListAffils
    CmdCreate - ${prefix}banned  [namespace current]::ListBanned

    # Administration commands (add/remove groups).
    CmdCreate o ${prefix}addaffil [namespace current]::ChangeAffils ADD
    CmdCreate o ${prefix}delaffil [namespace current]::ChangeAffils DEL
    CmdCreate o ${prefix}addban   [namespace current]::ChangeBanned ADD
    CmdCreate o ${prefix}delban   [namespace current]::ChangeBanned DEL

    return
}

####
# Unload
#
# Module clean-up procedure, called before the module is unloaded.
#
proc ::alcoholicz::Groups::Unload {} {
    variable groupsHandle
    if {[info exists groupsHandle]} {
        ConfigClose $groupsHandle
    }
    return
}
