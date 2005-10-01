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

namespace eval ::alcoholicz::FtpDaemon {
    if {![info exists dataPath]} {
        variable dataPath ""
        variable rootPath ""
    }
    namespace import -force ::alcoholicz::*
    namespace export GetFlagTypes \
        UserExists UserList UserInfo \
        GroupExists GroupList GroupInfo
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
            if {[llength $line] == 7} {
                set users([lindex $line 0]) [lrange $line 1 5]
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

    set filePath [file join $rootPath "etc" "group"]
    if {[FileChanged $filePath]} {
        unset -nocomplain groups

        set handle [open $filePath r]
        set data [read -nonewline $handle]

        foreach line [split $data "\n"] {
            set line [split [string trim $line] ":"]
            # Group:Description:GID:NotUsed
            if {[llength $line] == 4} {
                set groups([lindex $line 0]) [lrange $line 1 2]
            }
        }
        close $handle
    }
    return
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
#  - ips      <IP list>
#  - logins   <max logins>
#  - monthdn  <30 ints>
#  - monthup  <30 ints>
#  - password <hash>
#  - ratio    <10 ints>
#  - speed    <max down> <max up>
#  - tagline  <tagline>
#  - uid      <user ID>
#  - weekdn   <30 ints>
#  - weekup   <30 ints>
#
proc ::alcoholicz::FtpDaemon::UserInfo {userName varName} {
    variable dataPath
    variable users
    upvar $varName dest

    if {[catch {UpdateUsers} message]} {
        LogError UserInfo $message; return 0
    }
    if {![info exists users($userName)]} {return 0}

    array set dest [list                      \
        admin    ""                           \
        credits  {0 0 0 0 0 0 0 0 0 0}        \
        flags    ""                           \
        groups   ""                           \
        home     ""                           \
        ips      ""                           \
        logins   0                            \
        password [lindex $users($userName) 0] \
        ratio    {0 0 0 0 0 0 0 0 0 0}        \
        speed    {0 0}                        \
        tagline  ""                           \
        uid      [lindex $users($userName) 1] \
    ]
    foreach type {alldn allup daydn dayup monthdn monthup weekdn weekup} {
        set dest($type) {0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
    }

    # TODO: Parse user file.

    set filePath [file join $dataPath "users" $groupName]

    return 1
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

    array set dest [list                     \
        desc  [lindex $groups($groupName) 0] \
        gid   [lindex $groups($groupName) 1] \
        leech 0                              \
        ratio 0                              \
    ]

    # TODO: Parse group file.

    set filePath [file join $dataPath "groups" $groupName]

    return 1
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::FtpDaemon::Load {firstLoad} {
    variable change
    variable dataPath
    variable rootPath
    upvar ::alcoholicz::configHandle configHandle

    set dataPath [ConfigGet $configHandle GlFtpd dataPath]
    if {![file isdirectory $dataPath]} {
        error "the directory \"$dataPath\" does not exist"
    }

    set rootPath [ConfigGet $configHandle GlFtpd rootPath]
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
