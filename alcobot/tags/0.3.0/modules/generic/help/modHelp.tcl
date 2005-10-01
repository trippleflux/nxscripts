#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
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

namespace eval ::alcoholicz::Help {
    namespace import -force ::alcoholicz::*
}

####
# Command
#
# Implements a channel command to display supported commands.
#
proc ::alcoholicz::Help::Command {user host handle channel target argc argv} {
    SendTargetTheme $target helpHead

    foreach {name value} [CmdGetList channel *] {
        foreach {script category argDesc cmdDesc} $value {break}

        # Check if this command is present in the listed catagories.
        if {$cmdDesc eq "" || ($argc > 0 && ![InList $argv $category])} {
            continue
        }
        set command [lindex $name 1]

        # Check if the user has access to the command.
        set display 1
        foreach {enabled name value} [CmdGetFlags $command] {
            set result 0
            switch -- $name {
                all     {set result 1}
                channel {set result [string equal -nocase $value $channel]}
                flags   {set result [matchattr $user $value]}
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
        SendTargetTheme $target helpType [list $category]

        foreach value [lsort -index 0 $output($category)] {
            foreach {command argDesc cmdDesc} $value {break}
            SendTargetTheme $target helpBody [list $argDesc $command $cmdDesc]
        }
    }

    SendTargetTheme $target helpFoot
    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::Help::Load {firstLoad} {

    # Create related commands.
    CmdCreate channel ${::alcoholicz::cmdPrefix}help [namespace current]::Command \
        General "Display a command list." "\[category\] \[category\] ..."

    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Help::Unload {} {
    return
}
