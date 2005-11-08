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
#   Uniform FTPD API, for ioFTPD.
#
# Exported Procedures:
#   GetFtpConnection
#   GetFlagTypes     <varName>
#   UserList
#   UserExists       <userName>
#   UserInfo         <userName> <varName>
#   GroupList
#   GroupExists      <groupName>
#   GroupInfo        <groupName> <varName>
#

namespace eval ::alcoholicz::FtpDaemon {
    if {![info exists connection]} {
        variable connection ""
        variable deleteFlag ""
        variable etcPath ""
        variable msgWindow ""
        variable timerId ""
    }
    namespace import -force ::alcoholicz::*
    namespace export GetFtpConnection GetFlagTypes \
        UserExists UserList UserInfo \
        GroupExists GroupList GroupInfo
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
    return
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
            set message [FtpGetError $connection]
            LogError FtpServer "FTP handle not connected ($message), attemping to reconnect."
            FtpConnect $connection
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
        lappend nameList [ioftpd group toname $msgWindow $groupId]
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
        set line [split [string trim $line] ":"]
        # User:UID:Module
        if {[llength $line] == 3} {
            lappend userList [lindex $line 0]
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
    set user(uid)      [ioftpd user toid $msgWindow $userName]

    unset user(admingroups) user(ips) user(limits) user(vfsfile)
    return 1
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
        set line [split [string trim $line] ":"]
        # Group:GID:Module
        if {[llength $line] == 3} {
            lappend groupList [lindex $line 0]
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

    set group(gid)   [ioftpd group toid $msgWindow $groupName]
    set group(leech) [lindex $group(slots) 1]
    set group(ratio) [lindex $group(slots) 0]

    unset group(slots) group(vfsfile)
    return 1
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
        set $option [ConfigGet $configHandle IoFtpd $option]
    }
    if {[string length $deleteFlag] != 1} {
        error "invalid flag \"$deleteFlag\": must be one character"
    }

    # Locate ioFTPD's "etc" directory.
    ioftpd info $msgWindow io
    set etcPath [file join [file dirname [file dirname $io(path)]] "etc"]
    if {![file isdirectory $etcPath]} {
        error "the directory \"$etcPath\" does not exist"
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
    return
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
        FtpClose $connection
        set connection ""
    }
    if {$timerId ne ""} {
        catch {killtimer $timerId}
        set timerId ""
    }
    return
}
