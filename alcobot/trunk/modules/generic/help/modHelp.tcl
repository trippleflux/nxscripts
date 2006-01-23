#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
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
proc ::alcoholicz::Help::Command {command target user host handle channel argv} {
    SendTargetTheme $target helpHead

    foreach {name value} [CmdGetList channel *] {
        foreach {argDesc cmdDesc category binds script} $value {break}

        # Check if this command is present in the listed catagories.
        if {$cmdDesc eq "" || ([llength $argv] && ![InList $argv $category])} {
            continue
        }
        set command [lindex $binds 0]

        # Check if the user has access to the command.
        set display 1
        foreach {enabled name value} [CmdGetFlags [lindex $name 1]] {
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
            SendTargetTheme $target helpBody $value
        }
    }
    SendTargetTheme $target helpFoot
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::Help::Load {firstLoad} {
    CmdCreate channel help [namespace current]::Command -category "General" \
        -desc "Display a command list." -args "\[category\] \[category\] ..."
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Help::Unload {} {
}
