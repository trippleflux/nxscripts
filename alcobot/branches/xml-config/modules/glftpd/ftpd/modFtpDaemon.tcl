#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   FTPD API Module
#
# Author:
#   neoxed (neoxed@gmail.com) Sep 25, 2005
#
# Abstract:
#   Uniform FTPD API, for glFTPD.
#
# Exported Procedures:
#   GetFlagTypes     <varName>
#   GetFtpConnection
#   UserList
#   UserExists       <userName>
#   UserInfo         <userName> <varName>
#   UserIdToName     <userId>
#   UserNameToId     <userName>
#   GroupList
#   GroupExists      <groupName>
#   GroupInfo        <groupName> <varName>
#   GroupIdToName    <groupId>
#   GroupNameToId    <groupName>
#

namespace eval ::alcoholicz::FtpDaemon {
    if {![info exists [namespace current]::connection]} {
        variable connection ""
        variable dataPath ""
        variable rootPath ""
        variable timerId ""
    }
    namespace import -force ::alcoholicz::*
    namespace import -force ::config::*
    namespace import -force ::ftp::*
    namespace export GetFlagTypes GetFtpConnection \
        UserExists UserList UserInfo UserIdToName UserNameToId \
        GroupExists GroupList GroupInfo GroupIdToName GroupNameToId
}

####
# FtpNotify
#
# Called when the initial connection succeeds or fails.
#
proc ::alcoholicz::FtpDaemon::FtpNotify {connection success} {
    if {$success} {
        LogInfo "FTP connection established."
    } else {
        LogInfo "FTP connection failed - [FtpGetError $connection]"
    }
}

####
# FtpTimer
#
# Checks the status of the FTP connection every minute.
#
proc ::alcoholicz::FtpDaemon::FtpTimer {} {
    variable connection
    variable timerId

    # Wrap the FTP connection code in a catch statement in case the FTP
    # library throws an error. The Eggdrop timer must be recreated.
    if {[catch {
        if {[FtpGetStatus $connection] == 2} {
            FtpCommand $connection "NOOP"
        } else {
            LogError FtpServer "FTP handle not connected, attemping to reconnect."
            FtpConnect $connection
        }
    } message]} {
        LogError FtpTimer $message
    }

    set timerId [timer 1 [namespace current]::FtpTimer]
    return
}

####
# FileChanged
#
# Checks if the size or modification time of a file has changed.
#
proc ::alcoholicz::FtpDaemon::FileChanged {filePath} {
    variable change

    file stat $filePath stat
    if {[info exists change($filePath)] &&
            [lindex $change($filePath) 0] == $stat(size) &&
            [lindex $change($filePath) 1] == $stat(mtime)} {
        set result 0
    } else {
        set result 1
    }

    set change($filePath) [list $stat(size) $stat(mtime)]
    return $result
}

####
# UpdateUsers
#
# Updates internal user list.
#
proc ::alcoholicz::FtpDaemon::UpdateUsers {} {
    variable users
    variable rootPath

    set filePath [file join $rootPath "etc" "passwd"]
    if {[FileChanged $filePath]} {
        unset -nocomplain users

        set handle [open $filePath r]
        set data [read -nonewline $handle]

        foreach line [split $data "\n"] {
            set line [split [string trim $line] ":"]
            # User:Password:UID:GID:CreationDate:HomeDir:NotUsed
            if {[llength $line] == 7 && [string index [lindex $line 0] 0] ne "#"} {
                set users([lindex $line 0]) [lrange $line 1 5]
            }
        }
        close $handle
    }
}

####
# UpdateGroups
#
# Updates internal group list.
#
proc ::alcoholicz::FtpDaemon::UpdateGroups {} {
    variable groups
    variable rootPath

    set filePath [file join $rootPath "etc" "group"]
    if {[FileChanged $filePath]} {
        unset -nocomplain groups

        set handle [open $filePath r]
        set data [read -nonewline $handle]

        foreach line [split $data "\n"] {
            set line [split [string trim $line] ":"]
            # Group:Description:GID:NotUsed
            if {[llength $line] == 4 && [string index [lindex $line 0] 0] ne "#"} {
                set groups([lindex $line 0]) [lrange $line 1 2]
            }
        }
        close $handle
    }
}

####
# GetFlagTypes
#
# Retrieves flag types, results are saved to the given variable name.
#
proc ::alcoholicz::FtpDaemon::GetFlagTypes {varName} {
    upvar $varName flags
    array set flags [list deleted "6" gadmin "2" siteop "1"]
}

####
# GetFtpConnection
#
# Retrieves the main FTP connection handle.
#
proc ::alcoholicz::FtpDaemon::GetFtpConnection {} {
    variable connection
    return $connection
}

####
# UserList
#
# Retrieves a list of users.
#
proc ::alcoholicz::FtpDaemon::UserList {} {
    variable users

    if {[catch {UpdateUsers} message]} {
        LogError UserList $message; return [list]
    }
    return [lsort [array names users]]
}

####
# UserExists
#
# Checks if the given user exists.
#
proc ::alcoholicz::FtpDaemon::UserExists {userName} {
    variable users

    if {[catch {UpdateUsers} message]} {
        LogError UserExists $message; return 0
    }
    return [info exists users($userName)]
}

####
# UserInfo
#
# Retrieve information about a user, results are saved to the given variable name.
#  - admin    <group list>
#  - alldn    <30 ints>
#  - allup    <30 ints>
#  - credits  <10 ints>
#  - daydn    <30 ints>
#  - dayup    <30 ints>
#  - flags    <flags>
#  - groups   <group list>
#  - hosts    <host list>
#  - logins   <max logins>
#  - monthdn  <30 ints>
#  - monthup  <30 ints>
#  - password <hash>
#  - ratio    <10 ints>
#  - speed    <max down> <max up>
#  - tagline  <tagline>
#  - uid      <user ID>
#  - wkdn     <30 ints>
#  - wkup     <30 ints>
#
proc ::alcoholicz::FtpDaemon::UserInfo {userName varName} {
    variable dataPath
    variable users
    upvar $varName dest

    if {[catch {UpdateUsers} message]} {
        LogError UserInfo $message; return 0
    }
    if {![info exists users($userName)]} {return 0}

    set filePath [file join $dataPath "users" $userName]
    if {[catch {set handle [open $filePath r]} message]} {
        LogError UserInfo $message; return 0
    }

    # Set default values.
    array set dest [list                      \
        admin    ""                           \
        credits  {0 0 0 0 0 0 0 0 0 0}        \
        flags    ""                           \
        groups   ""                           \
        hosts    ""                           \
        logins   0                            \
        password [lindex $users($userName) 0] \
        ratio    {0 0 0 0 0 0 0 0 0 0}        \
        speed    {0 0}                        \
        tagline  ""                           \
        uid      [lindex $users($userName) 1] \
    ]
    foreach type {alldn allup daydn dayup monthdn monthup wkdn wkup} {
        set dest($type) {0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
    }

    # Parse user file.
    set data [read -nonewline $handle]; set i -1
    foreach line [split $data "\n"] {
        set line [split [string trim $line]]

        set type [string tolower [lindex $line 0]]
        switch -- $type {
            alldn - allup -
            daydn - dayup -
            monthdn - monthup -
            wkdn - wkup {
                foreach {files amount time} [lrange $line 1 end] {break}
                set dest($type) [lreplace $dest($type) $i [expr {$i + 2}] $files $amount $time]
            }
            credits {
                incr i
                set dest(credits) [lreplace $dest(credits) $i $i [lindex $line 1]]
            }
            flags {
                set dest(flags) [lindex $line 1]
            }
            general {
                set dest(speed) [lrange $line 3 4]
            }
            group {
                if {[lindex $line 2] == 1} {
                    lappend dest(admin) [lindex $line 1]
                }
                lappend dest(groups) [lindex $line 1]
            }
            dns - ip {
                lappend dest(hosts) [lindex $line 1]
            }
            logins {
                set dest(logins)  [lindex $line 1]
            }
            ratio {
                set dest(ratio)   [lreplace $dest(ratio) $i $i [lindex $line 1]]
            }
            tagline {
                set dest(tagline) [join [lrange $line 1 end]]
            }
        }
    }
    close $handle
    return 1
}

####
# UserIdToName
#
# Resolve a user ID to its corresponding user name.
#
proc ::alcoholicz::FtpDaemon::UserIdToName {userId} {
    variable users

    if {[catch {UpdateUsers} message]} {
        LogError UserIdToName $message
    } else {
        foreach name [array names users] {
            if {[lindex $users($name) 1] == $userId} {return $name}
        }
    }
    return ""
}

####
# UserNameToId
#
# Resolve a user name to its corresponding user ID.
#
proc ::alcoholicz::FtpDaemon::UserNameToId {userName} {
    variable users

    if {[catch {UpdateUsers} message]} {
        LogError UserIdToName $message
    } elseif {[info exists users($userName)]} {
        return [lindex $users($userName) 1]
    }
    return -1
}

####
# GroupList
#
# Retrieves a list of groups.
#
proc ::alcoholicz::FtpDaemon::GroupList {} {
    variable groups

    if {[catch {UpdateGroups} message]} {
        LogError GroupList $message; return [list]
    }
    return [lsort [array names groups]]
}

####
# GroupExists
#
# Checks if the given group exists.
#
proc ::alcoholicz::FtpDaemon::GroupExists {groupName} {
    variable groups

    if {[catch {UpdateGroups} message]} {
        LogError GroupExists $message; return 0
    }
    return [info exists groups($groupName)]
}

####
# GroupInfo
#
# Retrieve information about a group, results are saved to the given variable name.
#  - desc  <description>
#  - gid   <group ID>
#  - leech <leech slots>
#  - ratio <ratio slots>
#
proc ::alcoholicz::FtpDaemon::GroupInfo {groupName varName} {
    variable dataPath
    variable groups
    upvar $varName dest

    if {[catch {UpdateGroups} message]} {
        LogError GroupInfo $message; return 0
    }
    if {![info exists groups($groupName)]} {return 0}

    set filePath [file join $dataPath "groups" $groupName]
    if {[catch {set handle [open $filePath r]} message]} {
        LogError UserInfo $message; return 0
    }

    # Set default values.
    array set dest [list                     \
        desc  [lindex $groups($groupName) 0] \
        gid   [lindex $groups($groupName) 1] \
        leech 0                              \
        ratio 0                              \
    ]

    # Parse group file.
    set data [read -nonewline $handle]
    foreach line [split $data "\n"] {
        set line [split [string trim $line]]

        if {[string equal -nocase "slots" [lindex $line 0]]} {
            # SLOTS <ratio> <leech> <weekly allotment> <max allotment size>
            set dest(ratio) [lindex $line 1]
            set dest(leech) [lindex $line 2]
            break
        }
    }
    close $handle
    return 1
}

####
# GroupIdToName
#
# Resolve a group ID to its corresponding group name.
#
proc ::alcoholicz::FtpDaemon::GroupIdToName {groupId} {
    variable groups

    if {[catch {UpdateGroups} message]} {
        LogError GroupIdToName $message
    } else {
        foreach name [array names groups] {
            if {[lindex $groups($name) 1] == $groupId} {return $name}
        }
    }
    return ""
}

####
# GroupNameToId
#
# Resolve a group name to its corresponding group ID.
#
proc ::alcoholicz::FtpDaemon::GroupNameToId {groupName} {
    variable groups

    if {[catch {UpdateGroups} message]} {
        LogError GroupNameToId $message
    } elseif {[info exists groups($groupName)]} {
        return [lindex $groups($groupName) 1]
    }
    return -1
}

####
# NukeEvent
#
# Handle NUKE and UNNUKE log events.
#
proc ::alcoholicz::FtpDaemon::NukeEvent {event destSection pathSection path data} {
    # glFTPD v2.x quotes each nukee separately when logging nukes, while AlcoBot's
    # theming system expects them to be quoted together. So we have to do a bit
    # of work to keep the two compatible.

    # Before: path nuker multi reason nukee1 nukee2 ...
    # After : path nuker multi reason {nukee1 nukee2 ...}
    set nukees [join [lrange $data 4 end]]
    set data [lreplace $data 4 end $nukees]

    SendSectionTheme $destSection $event $data
    return 0
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::FtpDaemon::Load {firstLoad} {
    variable change
    variable connection
    variable dataPath
    variable rootPath
    variable timerId
    upvar ::alcoholicz::configHandle configHandle

    # Retrieve configuration options.
    foreach option {dataPath rootPath host port user passwd secure version} {
        set $option [ConfigGet $configHandle Ftpd $option]
    }
    if {![file isdirectory $dataPath]} {
        error "the directory \"$dataPath\" does not exist"
    }
    if {![file isdirectory $rootPath]} {
        error "the directory \"$rootPath\" does not exist"
    }
    if {[package vcompare $version 2.0] == -1} {
        error "you must be using glFTPD v2.0 or later"
    }

    # Open a connection to the FTP server.
    if {$firstLoad} {
        set timerId [timer 1 [namespace current]::FtpTimer]
    } else {
        FtpClose $connection
    }
    set connection [FtpOpen $host $port $user $passwd \
        -notify [namespace current]::FtpNotify -secure $secure]
    FtpConnect $connection

    # Register event callbacks.
    ScriptRegister pre NUKE   [namespace current]::NukeEvent
    ScriptRegister pre UNNUKE [namespace current]::NukeEvent

    # Force a reload on all cached files.
    unset -nocomplain change
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::FtpDaemon::Unload {} {
    variable connection
    variable timerId

    # Remove event callbacks.
    ScriptUnregister pre NUKE   [namespace current]::NukeEvent
    ScriptUnregister pre UNNUKE [namespace current]::NukeEvent

    if {$connection ne ""} {
        FtpClose $connection
        set connection ""
    }
    if {$timerId ne ""} {
        catch {killtimer $timerId}
        set timerId ""
    }
}
