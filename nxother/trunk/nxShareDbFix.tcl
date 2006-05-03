################################################################################
# nxShareDbFix - Corrects Problems In ioShareDb                                #
################################################################################
# Author  : neoxed                                                             #
# Date    : 13/07/2005                                                         #
# Version : 1.0.0                                                              #
################################################################################
#
# Installation:
# 1. Copy the nxShareDbFix.itcl file to x:\ioFTPD\scripts
# 2. Configure the script options.
# 3. Add the following to the ioFTPD.ini:
#
# [FTP_Pre-Command_Events]
# site = TCL ..\scripts\nxShareDbFix.tcl
#
# 4. Configure the script.
# 4. Rehash ioFTPD for the changes to take affect (SITE CONFIG REHASH).
# 5. Good luck! ;)
#
# Change Log:
#
# v1.0.0 - Initial release.
#
################################################################################

namespace eval ::nxShareDbFix {
    # Group admin flag (one flag only!).
    variable groupAdminFlag     "G"

    # Check leech slots on 'SITE CHANGE <target> RATIO 0' for group admins.
    variable updateLeechSlots   True
}

interp alias {} IsTrue {} string is true -strict
interp alias {} IsFalse {} string is false -strict

proc ::nxShareDbFix::ArgList {argv} {
    split [string trim [regsub -all {\s+} $argv { }]]
}

proc ::nxShareDbFix::GetUserAdminGroupIds {user} {
    if {[userfile open $user] != 0} {return [list]}
    foreach line [split [userfile bin2ascii] "\r\n"] {
        set line [ArgList $line]
        if {[string equal -nocase "admingroups" [lindex $line 0]]} {
            return [lrange $line 1 end]
        }
    }
}

proc ::nxShareDbFix::GetUserNames {} {
    set users [list]
    foreach userId [user list] {lappend users [resolve uid $userId]}
    return $users
}

proc ::nxShareDbFix::GetGroupNames {} {
    set groups [list]
    foreach groupId [group list] {lappend groups [resolve gid $groupId]}
    return $groups
}

proc ::nxShareDbFix::GetGroupLeechSlots {groupName} {
    if {[groupfile open $name] == 0} {
        foreach line [split [groupfile bin2ascii] "\r\n"] {
            set line [ArgList $line]
            if {[string equal -nocase "slots" [lindex $line 0]]} {
                return [lindex $line 2]
            }
        }
    }
    return 0
}

proc ::nxShareDbFix::SetGroupLeechSlots {groupName leechSlots} {
    if {[groupfile open $name] == 0} {
        groupfile lock
        set currentFile [split [groupfile bin2ascii] "\r\n"]
        set newFile [list]
        foreach line $groupFile {
            set lineList [ArgList $line]
            if {[string equal -nocase "slots" [lindex $lineList 0]]} {
                set line [join [lreplace $lineList 2 2 $leechSlots]]
            }
            lappend newFile $line
        }
        groupfile ascii2bin [join $newFile "\r\n"]
        groupfile unlock
    }
    return 0
}

proc ::nxShareDbFix::GetUsersInGroup {groupId} {
    set users [list]
    foreach name [GetUserNames] {
        if {[userfile open $name] != 0} {continue}
        if {[regexp -nocase {groups ([\s\d]+)} [userfile bin2ascii] result groupIdList]} {
            if {[lsearch -exact $groupIdList $groupId] != -1} {lappend users $name}
        }
    }
    return $UserList
}

proc ::nxShareDbFix::GetUsersFromPattern {pattern} {
    # Check for the group prefix.
    if {[string index $pattern 0] eq "="} {
        set group [string range $pattern 1 end]
        return [GetUsersInGroup [resolve gid $group]]
    } else {
        set users [GetUserNames]
        # If there are no wild-cards present we can finish now.
        if {[string first "?" $pattern] == -1 && [string first "*" $pattern] == -1} {
            return $users
        }

        # Escape range-match characters.
        set pattern [string map {[ \\[ ] \\]} $pattern]
        set result [list]
        foreach name $users {
            if {[string match $pattern $name]} {lappend result $name}
        }
        return $result
    }
}

proc ::nxShareDbFix::IsGroupAdmin {name flags groupId} {
    variable groupAdminFlag
    if {[string first $groupAdminFlag $flags] == -1} {
        return 1
    } elseif {[userfile open $name] == 0} {
        if {[regexp -nocase {admingroups ([\s\d]+)} [userfile bin2ascii] result groupIdList]} {
            if {[lsearch -exact $groupIdList $groupId] != -1} {return 1}
        }
    }
    return 0
}

proc ::nxShareDbFix::SiteRatio {pattern ratio} {
    global user flags
    variable groupAdminFlag

    iputs "DBG: start"

    # Check if the current user is a group admin and the ratio is 0.
    if {[string first $groupAdminFlag $flags] == -1 || $ratio != 0} {
        iputs "bail"
        return 0
    }

    set adminGroupIds [GetUserAdminGroupIds $user]
    iputs "DBG: adminGroupIds=\"$adminGroupIds\""
    if {![llength $adminGroupIds]} {
        iputs "You do not admin any groups."
        return 1
    }

    foreach groupId $adminGroupIds {
        set leechSlots [GetGroupLeechSlots [resolve gid $groupId]]
        iputs "DBG: leechSlots for $groupId is \"$leechSlots\""
        if {$leechSlots > 0} {
            set slots($groupId) $leechSlots
        }
    }

    if {![array exists slots]} {
        iputs "Your groups do not have any leech slots left."
        return 1
    }

    # To handle the following:
    # SITE CHANGE * RATIO 0
    # SITE CHANGE <user> RATIO 0
    # SITE CHANGE =<group> RATIO 0
    set users [GetUsersFromPattern [lindex $argList 2]]
    iputs "DBG: users=\"$users\""

    # Process all matching users.
    foreach user $users {
        set groupIds [list]
        if {[userfile open $name] != 0} {continue}
        foreach line [split [userfile bin2ascii] "\r\n"] {
            set line [ArgList $line]
            if {[string equal -nocase "groups" [lindex $line 0]]} {
                set groupIds [lrange $line 1 end]
                break
            }
        }

        # Process all groups the user is in.
        foreach groupId $groupIds {
            if {[lsearch -exact [array names slots] $groupId] == -1} {continue}

            # Check for remaining leech slots.
            set groupName [resolve gid $groupId]
            if {$slots($groupId) <= 0} {
                iputs "No leech slots left for '$groupName'."
                return 1
            }

            # Remove leech slot from group.
            incr slots($groupId) -1
            SetGroupLeechSlots $groupName $slots($groupId)
        }
    }

    return 0
}

proc ::nxShareDbFix::Main {argv} {
    global ioerror
    variable updateLeechSlots
    set result 0

    # Safe argument handling.
    set argList [ArgList $argv]
    set cmd [string toupper [lindex $argList 1]]
    set subCmd [string toupper [lindex $argList 3]]

    if {$cmd eq "CHANGE" && $subCmd eq "RATIO" && [IsTrue $updateLeechSlots]} {
        set result [SiteRatio [lindex $argList 2] [lindex $argList 3]]
    }

    # Return the error to ioFTPD.
    return [set ioerror $result]
}

::nxShareDbFix::Main [expr {[info exists args] ? $args : ""}]
