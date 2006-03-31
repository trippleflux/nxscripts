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
#   Implements a module to display online users and bandwidth usage.
#

namespace eval ::Bot::Mod::Online {
    if {![info exists [namespace current]::cmdTokens]} {
        variable cmdTokens [list]
        variable hideCount  0
        variable hideUsers  [list]
        variable hideGroups [list]
        variable hidePaths  [list]
        variable session ""
    }
    namespace import -force ::Bot::*
}

####
# IsHidden
#
# Checks if the given user's name, group, or current path are hidden.
#
proc ::Bot::Mod::Online::IsHidden {user group path} {
    variable hideUsers
    variable hideGroups
    variable hidePaths

    if {[lsearch -exact $hideUsers $user] != -1 || [lsearch -exact $hideGroups $group] != -1} {
        return 1
    }
    foreach pattern $hidePaths {
        if {[string match -nocase $pattern $path]} {return 1}
    }
    return 0
}

####
# Bandwidth
#
# Implements a channel command to display current site bandwidth.
#
proc ::Bot::Mod::Online::Bandwidth {event target user host channel argv} {
    variable hideCount
    variable session

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

    if {![catch {set online [glftpd who $session "status user group speed path"]} message]} {
        foreach entry $online {
            foreach {status user group speed path} $entry {break}
            if {!$hideCount && [IsHidden $user $group $path]} {continue}

            # User Status:
            # 0 - Idle
            # 1 - Download
            # 2 - Upload
            # 3 - Listing
            switch -- $status {
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
        set usersMax [glftpd info maxusers $session]
    } else {
        LogError ModOnline $message
        set usersMax 0
    }
    set speedTotal [expr {$speedDn + $speedUp}]
    set usersTotal [expr {$usersDn + $usersUp + $usersIdle}]

    SendTargetTheme $target $theme [list $speedDn $speedUp $speedTotal \
        $usersDn $usersUp $usersIdle $usersTotal $usersMax]
}

####
# Status
#
# Implements a channel command to display the status of current users.
#
proc ::Bot::Mod::Online::Status {event target user host channel argv} {
    variable session

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

    if {![catch {set online [glftpd who $session "status user group idletime size speed path"]} message]} {
        foreach entry [lsort -index 1 $online] {
            foreach {status user group idle size speed path} $entry {break}
            if {![eval $filter] || [IsHidden $user $group $path]} {continue}

            incr users
            switch -- $status {
                0 - 3 {
                    SendTargetTheme $target ${theme}Body [list $user $group $idle $path]
                }
                1 - 2 {
                    set total [expr {$total + $speed}]
                    SendTargetTheme $target ${theme}Body [list $user $group \
                        $size $speed [file dirname $path] [file tail $path]]
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
proc ::Bot::Mod::Online::Users {event target user host channel argv} {
    variable session

    if {$event eq "SPEED"} {
        if {[llength $argv] != 1} {throw CMDHELP}
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

    if {![catch {set online [glftpd who $session "status user group idletime size speed path"]} message]} {
        foreach entry [lsort -index 1 $online] {
            foreach {status user group idle size speed path} $entry {break}
            if {($event eq "SPEED" && $user ne [lindex $argv 0]) || [IsHidden $user $group $path]} {continue}

            switch -- $status {
                0 - 3 {
                    incr usersIdle
                    SendTargetTheme $target ${theme}Idle [list $user $group $idle $path]
                }
                1 {
                    incr usersDn
                    set speedDn [expr {$speedDn + $speed}]
                    SendTargetTheme $target ${theme}Download [list $user $group \
                        $size $speed [file dirname $path] [file tail $path]]
                }
                2 {
                    incr usersUp
                    set speedUp [expr {$speedUp + $speed}]
                    SendTargetTheme $target ${theme}Upload [list $user $group \
                        $size $speed [file dirname $path] [file tail $path]]
                }
            }
        }
        set usersMax [glftpd info maxusers $session]
    } else {
        LogError ModOnline $message
        set usersMax 0
    }
    set speedTotal [expr {$speedDn + $speedUp}]
    set usersTotal [expr {$usersDn + $usersUp + $usersIdle}]

    if {$usersTotal} {
        SendTargetTheme $target ${theme}Foot [list $speedDn $speedUp $speedTotal \
            $usersDn $usersUp $usersIdle $usersTotal $usersMax]
    } else {
        SendTargetTheme $target ${theme}None
    }
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::Online::Load {firstLoad} {
    variable cmdTokens
    variable hideCount
    variable hideUsers
    variable hideGroups
    variable hidePaths
    variable session
    upvar ::Bot::configHandle configHandle

    # Retrieve configuration options.
    foreach option {rootPath shmKey version} {
        set $option [Config::Get $configHandle Ftpd $option]
    }
    foreach option {hideUsers hideGroups hidePaths} {
        set $option [ListParse [Config::Get $configHandle Module::Online $option]]
    }
    set hideCount [IsTrue [Config::Get $configHandle Module::Online hideCount]]

    set etcPath [file join $rootPath "etc"]
    if {![file isdirectory $etcPath]} {
        error "the directory \"$etcPath\" does not exist"
    }

    if {![scan $shmKey "%lx" shmKey]} {
        error "invalid shared memory key \"$shmKey\""
    }

    # Open a glFTPD session handle.
    if {$session eq ""} {
        set session [glftpd open $shmKey]
    } else {
        glftpd config $session -key $shmKey
    }
    glftpd config $session -etc $etcPath -version $version
    set cmdTokens [list]

    # Bandwidth commands.
    lappend cmdTokens [CmdCreate channel bw [list [namespace current]::Bandwidth ALL] \
        -category "Online" -desc "Total bandwidth usage."]

    lappend cmdTokens [CmdCreate channel bwdn [list [namespace current]::Bandwidth DN] \
        -category "Online" -desc "Outgoing bandwidth usage."]

    lappend cmdTokens [CmdCreate channel bwup [list [namespace current]::Bandwidth UP] \
        -category "Online" -desc "Incoming bandwidth usage."]

    # Status commands.
    lappend cmdTokens [CmdCreate channel idlers [list [namespace current]::Status ID] \
        -alias "idle" -category "Online" -desc "Users currently idling."]

    lappend cmdTokens [CmdCreate channel leechers [list [namespace current]::Status DN] \
        -alias "dn" -category "Online" -desc "Users currently downloading."]

    lappend cmdTokens [CmdCreate channel uploaders [list [namespace current]::Status UP] \
        -alias "up" -category "Online" -desc "Users currently uploading."]

    # User list commands.
    lappend cmdTokens [CmdCreate channel speed [list [namespace current]::Users SPEED] \
        -args "<user>" -category "Online" -desc "Status of a given user."]

    lappend cmdTokens [CmdCreate channel who [list [namespace current]::Users WHO] \
        -category "Online" -desc "Who is online."]
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::Online::Unload {} {
    variable cmdTokens
    variable session

    foreach token $cmdTokens {
        CmdRemoveByToken $token
    }
    if {$session ne ""} {
        glftpd close $session
        set session ""
    }
}
