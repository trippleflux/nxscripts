################################################################################
# nxTools - Open/Close Script                                                  #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

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

proc ::nxTools::Close::Main {ArgV} {
    global close misc flags group ioerror user
    set Result 0

    set ArgList [ArgList $ArgV]
    set Event [string toupper [lindex $ArgList 0]]
    switch -- $Event {
        {CLOSE} {
            iputs ".-\[Close\]-----------------------------------------------------------------."
            if {[catch {set CloseInfo [var get nxToolsClosed]}]} {
                set CloseReason [join [lrange $ArgList 1 end]]
                if {![string length $CloseReason]} {set CloseReason "No Reason"}
                set CloseInfo [list [clock seconds] $CloseReason]

                var set nxToolsClosed $CloseInfo
                LinePuts "Server is now closed for: $CloseReason."
                putlog "CLOSE: \"$user\" \"$group\" \"$CloseReason\""

                # Kick online users.
                if {[IsTrue $close(KickOnClose)] && [client who init "CID" "UID"] == 0} {
                    while {[set WhoData [client who fetch]] ne ""} {
                        set UserName [resolve uid [lindex $WhoData 1]]
                        set Flags ""
                        set GroupName "NoGroup"
                        if {[userfile open $UserName] == 0} {
                            set UserFile [userfile bin2ascii]
                            foreach UserLine [split $UserFile "\r\n"] {
                                set LineType [string tolower [lindex $UserLine 0]]
                                if {$LineType eq "groups"} {
                                    set GroupName [GetGroupName [lindex $UserLine 1]]
                                } elseif {$LineType eq "flags"} {
                                    set Flags [lindex $UserLine 1]
                                }
                            }
                        }
                        if {![ExcemptCheck $UserName $GroupName $Flags]} {
                            catch {client kill clientid [lindex $WhoData 0]}
                        }
                    }
                }
            } else {
                LinePuts "Server is currently closed, use \"SITE OPEN\" to open it."
            }
            iputs "'------------------------------------------------------------------------'"
        }
        {LOGIN} {
            if {![ExcemptCheck $user $group $flags] && ![catch {set CloseInfo [var get nxToolsClosed]}]} {
                set Duration [expr {[clock seconds] - [lindex $CloseInfo 0]}]
                iputs -nobuffer "530 Server Closed: [lindex $CloseInfo 1] (since [FormatDuration $Duration] ago)"
                set Result 1
            }
        }
        {OPEN} {
            iputs ".-\[Open\]-----------------------------------------------------------------."
            if {[catch {set CloseInfo [var get nxToolsClosed]}]} {
                LinePuts "Server is currently open, use \"SITE CLOSE \[reason\]\" to close it."
            } else {
                set Duration [expr {[clock seconds] - [lindex $CloseInfo 0]}]
                LinePuts "Server is now open, closed for [FormatDuration $Duration]."
                if {[IsTrue $misc(dZSbotLogging)]} {
                    set Duration [FormatDuration $Duration]
                }
                putlog "OPEN: \"$user\" \"$group\" \"$Duration\" \"[lindex $CloseInfo 1]\""
                catch {var unset nxToolsClosed}
            }
            iputs "'------------------------------------------------------------------------'"
        }
        default {
            ErrorLog InvalidArgs "unknown event \"[info script] $Event\": check your ioFTPD.ini for errors"
            set Result 1
        }
    }
    return [set ioerror $Result]
}

::nxTools::Close::Main [expr {[info exists args] ? $args : ""}]
