#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Online Module
#
# Author:
#   neoxed (neoxed@gmail.com) Oct 7, 2005
#
# Abstract:
#   Implements a module to display online statistics.
#

namespace eval ::alcoholicz::Online {
    if {![info exists [namespace current]::hideUsers]} {
        variable hideUsers  [list]
        variable hideGroups [list]
        variable hidePaths  [list]
        variable msgWindow  ""
    }
    namespace import -force ::alcoholicz::*
}

####
# IsHidden
#
# Checks if the given user's name, group, or current path are hidden.
#
proc ::alcoholicz::Online::IsHidden {user group vpath} {
    variable hideUsers
    variable hideGroups
    variable hidePaths

    if {[lsearch -exact $hideUsers $user] != -1 || [lsearch -exact $hideGroups $group] != -1} {
        return 1
    }
    foreach pattern $hidePaths {
        if {[string match -nocase $pattern $vpath]} {return 1}
    }
    return 0
}

####
# Bandwidth
#
# Implements a channel command to display current site bandwidth.
#
proc ::alcoholicz::Online::Bandwidth {event command target user host handle channel argv} {
    variable msgWindow

    switch -- $event {
        ALL {set theme "bandwidth"}
        DN  {set theme "bandwidthDn"}
        UP  {set theme "bandwidthUp"}
        default {
            LogError ModOnline "Unknown bandwidth event \"$event\"."
            return
        }
    }
    set speedDn 0.0; set speedUp 0.0
    set usersDn 0; set usersUp 0; set usersIdle 0

    if {![catch {set online [ioftpd who $msgWindow "speed status"]} message]} {
        foreach entry $online {
            set speed [lindex $entry 0]

            # User Status:
            # 0 - Idle
            # 1 - Download
            # 2 - Upload
            # 3 - Listing
            switch -- [lindex $entry 1] {
                0 - 3 {incr usersIdle}
                1 {
                    incr usersDn
                    set speedDn [expr {$speedDn + $speed}]
                }
                2 {
                    incr usersUp
                    set speedUp [expr {$speedUp + $speed}]
                }
            }
        }
    } else {
        LogError ModOnline $message
    }
    set speedTotal [expr {$speedDn + $speedUp}]
    set usersTotal [expr {$usersDn + $usersUp + $usersIdle}]

    SendTargetTheme $target $theme [list $speedDn $speedUp $speedTotal \
        $usersDn $usersUp $usersIdle $usersTotal]
}

####
# Status
#
# Implements a channel command to display the status of current users.
#
proc ::alcoholicz::Online::Status {event command target user host handle channel argv} {
    variable msgWindow

    switch -- $event {
        DN {
            set filter {expr {$status == 1}}
            set theme "download"
        }
        ID {
            set filter {expr {$status == 0 || $status == 3}}
            set theme "idle"
        }
        UP {
            set filter {expr {$status == 2}}
            set theme "upload"
        }
        default {
            LogError ModOnline "Unknown status event \"$event\"."
            return
        }
    }
    SendTargetTheme $target ${theme}Head
    set total 0.0; set users 0

    if {![catch {set online [ioftpd who $msgWindow "status user group idletime size speed vpath realdatapath"]} message]} {
        foreach entry [lsort -index 1 $online] {
            foreach {status user group idle size speed vpath dataPath} $entry {break}
            if {![eval $filter] || [IsHidden $user $group $vpath]} {continue}

            incr users
            switch -- $status {
                0 - 3 {
                    SendTargetTheme $target ${theme}Body [list $user $group $idle $vpath]
                }
                1 - 2 {
                    set total [expr {$total + $speed}]
                    SendTargetTheme $target ${theme}Body [list $user $group \
                        $size $speed $vpath [file tail $dataPath]]
                }
            }
        }
    } else {
        LogError ModOnline $message
    }
    SendTargetTheme $target ${theme}Foot [list $total $users]
}

####
# Users
#
# Implements a channel command to display current users.
#
proc ::alcoholicz::Online::Users {event command target user host handle channel argv} {
    variable msgWindow

    if {$event eq "SPEED"} {
        if {[llength $argv] != 1} {
            CmdSendHelp $channel channel $command
            return
        }
        SendTargetTheme $target speedHead $argv
    } elseif {$event eq "WHO"} {
        SendTargetTheme $target whoHead
    } else {
        LogError ModOnline "Unknown users event \"$event\"."
        return
    }
    set speedDn 0.0; set speedUp 0.0
    set usersDn 0; set usersUp 0; set usersIdle 0
    set theme [string tolower $event]

    if {![catch {set online [ioftpd who $msgWindow "status user group idletime size speed vpath realdatapath"]} message]} {
        foreach entry [lsort -index 1 $online] {
            foreach {status user group idle size speed vpath dataPath} $entry {break}
            if {($event eq "SPEED" && $user ne [lindex $argv 0]) || [IsHidden $user $group $vpath]} {continue}

            switch -- $status {
                0 - 3 {
                    incr usersIdle
                    SendTargetTheme $target ${theme}Idle [list $user $group $idle $vpath]
                }
                1 {
                    incr usersDn
                    set speedDn [expr {$speedDn + $speed}]
                    SendTargetTheme $target ${theme}Download [list $user $group \
                        $size $speed $vpath [file tail $dataPath]]
                }
                2 {
                    incr usersUp
                    set speedUp [expr {$speedUp + $speed}]
                    SendTargetTheme $target ${theme}Upload [list $user $group \
                        $size $speed $vpath [file tail $dataPath]]
                }
            }
        }
    } else {
        LogError ModOnline $message
    }
    set speedTotal [expr {$speedDn + $speedUp}]
    set usersTotal [expr {$usersDn + $usersUp + $usersIdle}]

    if {$usersTotal} {
        SendTargetTheme $target ${theme}Foot [list $speedDn $speedUp $speedTotal \
            $usersDn $usersUp $usersIdle $usersTotal]
    } else {
        SendTargetTheme $target ${theme}None
    }
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::Online::Load {firstLoad} {
    variable hideUsers
    variable hideGroups
    variable hidePaths
    variable msgWindow
    upvar ::alcoholicz::configHandle configHandle

    set msgWindow [ConfigGet $configHandle Ftpd msgWindow]
    if {[catch {ioftpd info $msgWindow io}]} {
        error "the message window \"$msgWindow\" does not exist"
    }

    foreach option {hideUsers hideGroups hidePaths} {
        set $option [ArgsToList [ConfigGet $configHandle Module::Online $option]]
    }

    if {[ConfigExists $configHandle Module::Online cmdPrefix]} {
        set prefix [ConfigGet $configHandle Module::Online cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    # Bandwidth commands.
    CmdCreate channel bw   [list [namespace current]::Bandwidth ALL] \
        -category "Online" -prefix $prefix -desc "Total bandwidth usage."

    CmdCreate channel bwdn [list [namespace current]::Bandwidth DN] \
        -category "Online" -prefix $prefix -desc "Outgoing bandwidth usage."

    CmdCreate channel bwup [list [namespace current]::Bandwidth UP] \
        -category "Online" -prefix $prefix -desc "Incoming bandwidth usage."

    # Status commands.
    CmdCreate channel idlers    [list [namespace current]::Status ID] \
        -aliases "idle" -category "Online" -prefix $prefix \
        -desc    "Users currently idling."

    CmdCreate channel leechers  [list [namespace current]::Status DN] \
        -aliases "dn" -category "Online" -prefix $prefix \
        -desc    "Users currently downloading."

    CmdCreate channel uploaders [list [namespace current]::Status UP] \
        -aliases "up" -category "Online" -prefix $prefix \
        -desc    "Users currently uploading."

    # User list commands.
    CmdCreate channel speed [list [namespace current]::Users SPEED] \
        -category "Online" -args "<user>" -prefix $prefix \
        -desc     "Status of a given user."

    CmdCreate channel who   [list [namespace current]::Users WHO] \
        -category "Online" -prefix $prefix -desc "Who is online."
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Online::Unload {} {
}
