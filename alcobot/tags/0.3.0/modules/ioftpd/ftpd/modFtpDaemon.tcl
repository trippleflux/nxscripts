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
    if {![info exists deleteFlag]} {
        variable deleteFlag ""
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
# UserIdToName
#
# Resolve the given group ID to it's group name.
#
proc ::alcoholicz::FtpDaemon::UserIdToName {userId} {
    variable users

    foreach name [array names users] {
        if {$userId == $users($name)} {return $name}
    }
    return ""
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
# GroupIdToName
#
# Resolve the given group ID to it's group name.
#
proc ::alcoholicz::FtpDaemon::GroupIdToName {groupId} {
    variable groups

    foreach name [array names groups] {
        if {$groupId == $groups($name)} {return $name}
    }
    return ""
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
#  - wkdn     <30 ints>
#  - wkup     <30 ints>
#
proc ::alcoholicz::FtpDaemon::UserInfo {userName varName} {
    variable users
    variable rootPath
    upvar $varName dest

    if {[catch {UpdateUsers; UpdateGroups} message]} {
        LogError UserInfo $message; return 0
    }
    if {![info exists users($userName)]} {return 0}

    set filePath [file join $rootPath "users" $users($userName)]
    if {[catch {set handle [open $filePath r]} message]} {
        LogError UserInfo $message; return 0
    }

    # Set default values.
    array set dest [list               \
        admin    ""                    \
        credits  {0 0 0 0 0 0 0 0 0 0} \
        flags    ""                    \
        groups   ""                    \
        ips      ""                    \
        logins   0                     \
        password ""                    \
        ratio    {0 0 0 0 0 0 0 0 0 0} \
        speed    {0 0}                 \
        tagline  ""                    \
        uid      $users($userName)     \
    ]
    foreach type {alldn allup daydn dayup monthdn monthup wkdn wkup} {
        set dest($type) {0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}
    }

    # Parse user file.
    set data [read -nonewline $handle]
    foreach line [split $data "\n"] {
        set line [split [string trim $line]]

        set type [string tolower [lindex $line 0]]
        switch -- $type {
            alldn - allup -
            daydn - dayup -
            monthdn - monthup -
            wkdn - wkup {
                set dest($type) [lrange $line 1 30]
            }
            admingroups {
                foreach groupId [lrange $line 1 end] {
                    lappend dest(admin) [GroupIdToName $groupId]
                }
            }
            credits - ratio {
                set dest($type) [lrange $line 1 10]
            }
            flags {
                set dest(flags) [lindex $line 1]
            }
            groups {
                foreach groupId [lrange $line 1 end] {
                    lappend dest(groups) [GroupIdToName $groupId]
                }
            }
            ips {
                set dest(ips) [lrange $line 1 end]
            }
            limits {
                set dest(logins) [lindex $line 4]
                set dest(speed)  [lrange $line 1 2]
            }
            password {
                set dest(password) [lindex $line 1]
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
    variable groups
    variable rootPath
    upvar $varName dest

    if {[catch {UpdateGroups} message]} {
        LogError GroupInfo $message; return 0
    }
    if {![info exists groups($groupName)]} {return 0}

    set filePath [file join $rootPath "groups" $groups($groupName)]
    if {[catch {set handle [open $filePath r]} message]} {
        LogError UserInfo $message; return 0
    }

    # Set default values.
    array set dest [list          \
        desc  ""                  \
        gid   $groups($groupName) \
        leech 0                   \
        ratio 0                   \
    ]

    # Parse group file.
    set data [read -nonewline $handle]
    foreach line [split $data "\n"] {
        set line [split [string trim $line]]

        set type [string tolower [lindex $line 0]]
        if {$type eq "description"} {
            set dest(desc) [join [lrange $line 1 end]]
        } elseif {$type eq "slots"} {
            set dest(ratio) [lindex $line 1]
            set dest(leech) [lindex $line 2]
        }
    }
    close $handle
    return 1
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::FtpDaemon::Load {firstLoad} {
    variable change
    variable deleteFlag
    variable rootPath
    upvar ::alcoholicz::configHandle configHandle

    set deleteFlag [ConfigGet $configHandle IoFtpd deleteFlag]
    if {[string length $deleteFlag] != 1} {
        error "invalid flag \"$deleteFlag\": must be one character"
    }

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
