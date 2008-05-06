#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2008 Alcoholicz Scripting Team
#
# Module Name:
#   Help Module
#
# Author:
#   neoxed (neoxed@gmail.com) Sep 14, 2005
#
# Abstract:
#   Implements a module to display supported commands.
#

namespace eval ::Bot::Mod::Help {
    if {![info exists [namespace current]::cmdToken]} {
        variable cmdToken ""
    }
    namespace import -force ::Bot::*
}

####
# Command
#
# Implements a channel command to display supported commands.
#
proc ::Bot::Mod::Help::Command {target user host channel argv} {
    SendTargetTheme $target Module::Help head

    foreach {name value} [CmdGetList "channel" "*"] {
        foreach {argDesc cmdDesc category binds script token} $value {break}

        # Check if this command is present in the listed catagories.
        if {$cmdDesc eq "" || ([llength $argv] && ![ListExists $argv $category])} {
            continue
        }
        set command [lindex $binds 0]

        # Check if the user has access to the command.
        set display 1
        foreach {enabled name value} [CmdGetOptions "channel" [lindex $name 1]] {
            set result 0
            switch -- $name {
                all     {set result 1}
                channel {set result [string equal -nocase $value $channel]}
                flags   {
                    set handle [finduser "${user}!${host}"]
                    if {$handle ne "*"} {
                        set result [matchattr $handle $value]
                    }
                }
                host    {set result [string match -nocase $value $host]}
                user    {set result [string equal -nocase $value $user]}
            }
            if {$result} {
                if {!$enabled} {set display 0}
                break
            }
        }

        if {$display} {
            lappend output($category) [list $command $argDesc $cmdDesc]
        }
    }

    # Display all commands by category and alphabetically.
    foreach category [lsort [array names output]] {
        SendTargetTheme $target Module::Help type [list $category]

        foreach value [lsort -index 0 $output($category)] {
            SendTargetTheme $target Module::Help body $value
        }
    }
    SendTargetTheme $target Module::Help foot
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::Help::Load {firstLoad} {
    variable cmdToken
    set cmdToken [CmdCreate channel help [namespace current]::Command \
        -args "\[category\] \[category\] ..." \
        -category "General" -desc "Display a command list."]
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::Help::Unload {} {
    variable cmdToken
    CmdRemoveByToken $cmdToken
}
