#
# nxTools - Dupe Checker, nuker, pre, request and utilities.
# Copyright (c) 2004-2006 neoxed
#
# Module Name:
#   Close Site
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements commands to open and close the FTP server.
#

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Close {
    namespace import -force ::nxLib::*
}

# Close Procedures
######################################################################

proc ::nxTools::Close::ExcemptCheck {userName groupName flags} {
    global close
    if {[MatchFlags $close(Flags) $flags] || \
        [lsearch -exact $close(UserNames) $userName] != -1 || \
        [lsearch -exact $close(GroupNames) $groupName] != -1} {return 1}
    return 0
}

# Close Main
######################################################################

proc ::nxTools::Close::Main {argv} {
    global close misc flags group ioerror user
    set result 0

    set argList [ArgList $argv]
    set event [string toupper [lindex $argList 0]]
    switch -- $event {
        CLOSE {
            iputs ".-\[Close\]-----------------------------------------------------------------."
            if {[catch {set closeInfo [::nx::key get siteClosed]}]} {
                set reason [join [lrange $argList 1 end]]
                if {$reason eq ""} {set reason "No Reason"}
                ::nx::key set siteClosed [list [clock seconds] $reason]

                LinePuts "Server is now closed for: $reason."
                putlog "CLOSE: \"$user\" \"$group\" \"$reason\""

                # Kick online users.
                if {[IsTrue $close(KickOnClose)] && [client who init "CID" "UID"] == 0} {
                    while {[set whoData [client who fetch]] ne ""} {
                        set userName [resolve uid [lindex $whoData 1]]
                        GetUserInfo $userName groupName flags

                        if {![ExcemptCheck $userName $groupName $flags]} {
                            catch {client kill clientid [lindex $whoData 0]}
                        }
                    }
                }
            } else {
                LinePuts "Server is currently closed, use \"SITE OPEN\" to open it."
            }
            iputs "'------------------------------------------------------------------------'"
        }
        LOGIN {
            if {![ExcemptCheck $user $group $flags] && ![catch {set closeInfo [::nx::key get siteClosed]}]} {
                set duration [expr {[clock seconds] - [lindex $closeInfo 0]}]
                iputs -nobuffer "530 Server Closed: [lindex $closeInfo 1] (since [FormatDuration $duration] ago)"
                set result 1
            }
        }
        OPEN {
            iputs ".-\[Open\]-----------------------------------------------------------------."
            if {[catch {set closeInfo [::nx::key get siteClosed]}]} {
                LinePuts "Server is currently open, use \"SITE CLOSE \[reason\]\" to close it."
            } else {
                set duration [expr {[clock seconds] - [lindex $closeInfo 0]}]
                LinePuts "Server is now open, closed for [FormatDuration $duration]."
                putlog "OPEN: \"$user\" \"$group\" \"$duration\" \"[lindex $closeInfo 1]\""
                ::nx::key unset -nocomplain siteClosed
            }
            iputs "'------------------------------------------------------------------------'"
        }
        default {
            ErrorLog InvalidArgs "unknown event \"[info script] $event\": check your ioFTPD.ini for errors"
            set result 1
        }
    }
    return [set ioerror $result]
}

::nxTools::Close::Main [expr {[info exists args] ? $args : ""}]
