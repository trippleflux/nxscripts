#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   FTPD API Module
#
# Author:
#   neoxed (neoxed@gmail.com) Sep 25, 2005
#
# Abstract:
#   Uniform FTPD API, for ioFTPD.
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
        variable deleteFlag ""
        variable etcPath ""
        variable msgWindow ""
        variable timerId ""
    }
    namespace import -force ::alcoholicz::*
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
        LogInfo "FTP connection failed - [ftp::error $connection]"
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
        if {[ftp::status $connection] == 2} {
            ftp::command $connection "NOOP"
        } else {
            LogError FtpServer "FTP handle not connected, attemping to reconnect."
            ftp::connect $connection
        }
    } message]} {
        LogError FtpTimer $message
    }

    set timerId [timer 1 [namespace current]::FtpTimer]
    return
}

####
# ResolveGIDs
#
# Resolve a list of group IDs to their group names.
#
proc ::alcoholicz::FtpDaemon::ResolveGIDs {idList} {
    variable msgWindow

    set nameList [list]
    foreach groupId $idList {
        if {![catch {ioftpd group toname $msgWindow $groupId} group]} {
            lappend nameList $group
        }
    }
    return $nameList
}

####
# GetFlagTypes
#
# Retrieves flag types, results are saved to the given variable name.
#
proc ::alcoholicz::FtpDaemon::GetFlagTypes {varName} {
    variable deleteFlag

    upvar $varName flags
    array set flags [list deleted $deleteFlag gadmin "G2" siteop "M1"]
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
    variable etcPath

    set filePath [file join $etcPath "UserIdTable"]
    if {[catch {set handle [open $filePath r]} message]} {
        LogError UserList $message
        return [list]
    }
    set data [read -nonewline $handle]
    set userList [list]

    foreach line [split $data "\n"] {
        # User:UID:Module
        set line [split $line ":"]
        if {[llength $line] == 3} {
            lappend userList [string trim [lindex $line 0]]
        }
    }
    close $handle
    return $userList
}

####
# UserExists
#
# Checks if the given user exists.
#
proc ::alcoholicz::FtpDaemon::UserExists {userName} {
    variable msgWindow

    if {[catch {set exists [ioftpd user exists $msgWindow $userName]} message]} {
        LogError UserExists $message
        return 0
    }
    return $exists
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
    variable msgWindow
    upvar $varName user

    if {[catch {array set user [ioftpd user get $msgWindow $userName]} message]} {
        LogError UserInfo $message; return 0
    }

    set user(admin)    [ResolveGIDs $user(admingroups)]
    set user(groups)   [ResolveGIDs $user(groups)]
    set user(hosts)    $user(ips)
    set user(logins)   [lindex $user(limits) 4]
    set user(password) [encode hex $user(password)]
    set user(speed)    [lrange $user(limits) 1 2]
    set user(uid)      [UserNameToId $userName]

    unset user(admingroups) user(ips) user(limits) user(vfsfile)
    return 1
}

####
# UserIdToName
#
# Resolve a user ID to its corresponding user name.
#
proc ::alcoholicz::FtpDaemon::UserIdToName {userId} {
    variable msgWindow

    if {[catch {ioftpd user toname $msgWindow $userId} result]} {
        LogError UserIdToName $result
        return ""
    }
    return $result
}

####
# UserNameToId
#
# Resolve a user name to its corresponding user ID.
#
proc ::alcoholicz::FtpDaemon::UserNameToId {userName} {
    variable msgWindow

    if {[catch {ioftpd user toid $msgWindow $userName} result]} {
        LogError UserNameToId $result
        return -1
    }
    return $result
}

####
# GroupList
#
# Retrieves a list of groups.
#
proc ::alcoholicz::FtpDaemon::GroupList {} {
    variable etcPath

    set filePath [file join $etcPath "GroupIdTable"]
    if {[catch {set handle [open $filePath r]} message]} {
        LogError GroupList $message
        return [list]
    }
    set data [read -nonewline $handle]
    set groupList [list]

    foreach line [split $data "\n"] {
        # Group:GID:Module
        set line [split $line ":"]
        if {[llength $line] == 3} {
            lappend groupList [string trim [lindex $line 0]]
        }
    }
    close $handle
    return $groupList
}

####
# GroupExists
#
# Checks if the given group exists.
#
proc ::alcoholicz::FtpDaemon::GroupExists {groupName} {
    variable msgWindow

    if {[catch {set exists [ioftpd group exists $msgWindow $groupName]} message]} {
        LogError GroupExists $message
        return 0
    }
    return $exists
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
    variable msgWindow
    upvar $varName group

    if {[catch {array set group [ioftpd group get $msgWindow $groupName]} message]} {
        LogError GroupInfo $message; return 0
    }

    set group(gid)   [GroupNameToId $msgWindow $groupName]
    set group(leech) [lindex $group(slots) 1]
    set group(ratio) [lindex $group(slots) 0]

    unset group(slots) group(vfsfile)
    return 1
}

####
# GroupIdToName
#
# Resolve a group ID to its corresponding group name.
#
proc ::alcoholicz::FtpDaemon::GroupIdToName {groupId} {
    variable msgWindow

    if {[catch {ioftpd group toname $msgWindow $groupId} result]} {
        LogError GroupIdToName $result
        return ""
    }
    return $result
}

####
# GroupNameToId
#
# Resolve a group name to its corresponding group ID.
#
proc ::alcoholicz::FtpDaemon::GroupNameToId {groupName} {
    variable msgWindow

    if {[catch {ioftpd group toid $msgWindow $groupName} result]} {
        LogError GroupNameToId $result
        return -1
    }
    return $result
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::FtpDaemon::Load {firstLoad} {
    variable connection
    variable deleteFlag
    variable etcPath
    variable msgWindow
    variable timerId
    upvar ::alcoholicz::configHandle configHandle

    # Retrieve configuration options.
    foreach option {deleteFlag msgWindow host port user passwd secure} {
        set $option [config::get $configHandle Ftpd $option]
    }
    if {[string length $deleteFlag] != 1} {
        error "invalid flag \"$deleteFlag\": must be one character"
    }

    # Locate ioFTPD's "etc" directory.
    if {[catch {ioftpd info $msgWindow io}]} {
        error "the message window \"$msgWindow\" does not exist"
    }
    set etcPath [file join [file dirname [file dirname $io(path)]] "etc"]
    if {![file isdirectory $etcPath]} {
        error "the directory \"$etcPath\" does not exist"
    }

    # Open a connection to the FTP server.
    if {$firstLoad} {
        set timerId [timer 1 [namespace current]::FtpTimer]
    } else {
        ftp::close $connection
    }
    set connection [ftp::open $host $port $user $passwd \
        -notify [namespace current]::FtpNotify -secure $secure]
    ftp::connect $connection
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::FtpDaemon::Unload {} {
    variable connection
    variable timerId

    if {$connection ne ""} {
        ftp::close $connection
        set connection ""
    }
    if {$timerId ne ""} {
        catch {killtimer $timerId}
        set timerId ""
    }
}
