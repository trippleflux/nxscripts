################################################################################
# nxTools - Open/Close Script                                                  #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} ErrorMsg]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $ErrorMsg
}

namespace eval ::nxTools::Close {
    namespace import -force ::nxLib::*
}

# Close Procedures
######################################################################

proc ::nxTools::Close::ExcemptCheck {UserName GroupName Flags} {
    global close
    if {[regexp "\[$close(Flags)\]" $Flags]} {return 1}
    if {[lsearch -exact $close(UserNames) $UserName] != -1 || [lsearch -exact $close(GroupNames) $GroupName] != -1} {return 1}
    return 0
}

# Close Main
######################################################################

proc ::nxTools::Close::Main {ArgV} {
    global close misc flags group ioerror user

    ## Safe argument handling
    set ArgList [ArgList $ArgV]
    set Action [string tolower [lindex $ArgList 0]]
    set Result 0
    switch -- $Action {
        {close} {
            iputs ".-\[Close\]-----------------------------------------------------------------."
            if {[catch {set CloseInfo [var get nxToolsClosed]}]} {
                if {[set CloseReason [join [lrange $ArgList 1 end]]] == ""} {
                    set CloseReason "No Reason"
                }
                LinePuts "Server is now closed for: $CloseReason."
                putlog "CLOSE: \"$user\" \"$group\" \"$CloseReason\""

                ## Save the close info to a "global" variable
                set CloseInfo [list [clock seconds] $CloseReason]
                var set nxToolsClosed $CloseInfo

                ## Kick online users
                if {[IsTrue $close(KickOnClose)] && [client who init "CID" "UID"] == 0} {
                    while {[set WhoData [client who fetch]] != ""} {
                        set UserName [resolve uid [lindex $WhoData 1]]
                        set GroupName "NoGroup"; set Flags ""
                        if {[userfile open $UserName] == 0} {
                            set UserFile [userfile bin2ascii]
                            foreach UserLine [split $UserFile "\r\n"] {
                                set LineType [string tolower [lindex $UserLine 0]]
                                if {[string equal "groups" $LineType]} {
                                    set GroupName [GetGroupName [lindex $UserLine 1]]
                                } elseif {[string equal "flags" $LineType]} {
                                    set Flags [lindex $UserLine 1]
                                }
                            }
                        }
                        ## Kick any user non-excempted users
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
        {login} {
            if {![ExcemptCheck $user $group $flags] && ![catch {set CloseInfo [var get nxToolsClosed]}]} {
                set Duration [expr {[clock seconds] - [lindex $CloseInfo 0]}]
                iputs -nobuffer "530 Server Closed: [lindex $CloseInfo 1] (since [FormatDuration $Duration] ago)"
                set Result 1
            }
        }
        {open} {
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
                ## Unset the "global" variable
                catch {var unset nxToolsClosed}
            }
            iputs "'------------------------------------------------------------------------'"
        }
        default {
            ErrorLog InvalidArgs "invalid parameter \"[info script] $Action\": check your ioFTPD.ini for errors"
        }
    }
    return [set ioerror $Result]
}

::nxTools::Close::Main [expr {[info exists args] ? $args : ""}]
