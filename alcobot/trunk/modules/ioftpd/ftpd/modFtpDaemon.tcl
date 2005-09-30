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

namespace eval ::alcoholicz::FtpDaemon {
    if {![info exists rootPath]} {
        variable rootPath ""
    }
    namespace import -force ::alcoholicz::*
    namespace export UserExists UserList UserInfo GroupExists GroupList GroupInfo
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

    set filePath [file join $rootPath "etc" "UserIdTable"]
    if {[FileChanged $filePath]} {
        unset -nocomplain users

        set handle [open $filePath r]
        set data [read -nonewline $handle]

        foreach line [split $data "\n"] {
            set line [split [string trim $line] ":"]
            # User:UID:Module
            if {[llength $line] == 3} {
                set users([lindex $line 0]) [lindex $line 1]
            }
        }
        close $handle
    }
    return
}

####
# UpdateGroups
#
# Updates internal group list.
#
proc ::alcoholicz::FtpDaemon::UpdateGroups {} {
    variable groups
    variable rootPath

    set filePath [file join $rootPath "etc" "GroupIdTable"]
    if {[FileChanged $filePath]} {
        unset -nocomplain groups

        set handle [open $filePath r]
        set data [read -nonewline $handle]

        foreach line [split $data "\n"] {
            set line [split [string trim $line] ":"]
            # Group:GID:Module
            if {[llength $line] == 3} {
                set groups([lindex $line 0]) [lindex $line 1]
            }
        }
        close $handle
    }
    return
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
#  - admin    {group list}
#  - alldn    {30 ints}
#  - allup    {30 ints}
#  - credits  {10 ints}
#  - daydn    {30 ints}
#  - dayup    {30 ints}
#  - flags    {flags}
#  - groups   {group list}
#  - ips      {ip list}
#  - logins   {maxLogins}
#  - monthdn  {30 ints}
#  - monthup  {30 ints}
#  - password {hash}
#  - ratio    {10 ints}
#  - speed    {maxDown maxUp}
#  - tagline  {tagline}
#  - weekdn   {30 ints}
#  - weekup   {30 ints}
#
proc ::alcoholicz::FtpDaemon::UserInfo {userName varName} {
    variable users
    variable groups
    upvar $varName dest

    if {[catch {UpdateUsers; UpdateGroups} message]} {
        LogError UserInfo $message; return 0
    }
    if {![info exists users($userName)]} {return 0}

    array set dest {
        admin    ""
        credits  {0 0 0 0 0 0 0 0 0 0}
        flags    ""
        groups   ""
        home     ""
        ips      ""
        logins   0
        password ""
        ratio    {0 0 0 0 0 0 0 0 0 0}
        speed    {0 0}
        tagline  ""
    }
    foreach type {alldn allup daydn dayup monthdn monthup weekdn weekup} {
        set dest($type) {0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
    }

    # TODO: Parse user file.

    return 0
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
# UserExists
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
#  - desc  {description}
#  - leech {leechSlots}
#  - ratio {ratioSlots}
#
proc ::alcoholicz::FtpDaemon::GroupInfo {groupName varName} {
    variable groups
    upvar $varName dest

    if {[catch {UpdateGroups} message]} {
        LogError GroupInfo $message; return 0
    }
    if {![info exists groups($groupName)]} {return 0}

    array set dest {
        desc  ""
        leech 0
        ratio 0
    }

    # TODO: Parse group file.

    return 1
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::FtpDaemon::Load {firstLoad} {
    variable change
    variable rootPath
    upvar ::alcoholicz::configHandle configHandle

    set rootPath [ConfigGet $configHandle IoFtpd rootPath]
    if {![file isdirectory $rootPath]} {
        error "the directory \"$rootPath\" does not exist"
    }

    # Force a reload on all files.
    unset -nocomplain change
    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::FtpDaemon::Unload {} {
    return
}
