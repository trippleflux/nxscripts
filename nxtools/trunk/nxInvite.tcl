################################################################################
# nxTools - Invite Script                                                      #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} ErrorMsg]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $ErrorMsg
}

namespace eval ::nxTools::Invite {
    namespace import -force ::nxLib::*
}

# Invite Procedures
######################################################################

proc ::nxTools::Invite::ConfigLoader {ConfigFile} {
    upvar ConfigComments ConfigComments invchan invchan rights rights
    set ConfMode 0; set ConfigComments ""
    if {![catch {set Handle [open $ConfigFile r]} ErrorMsg]} {
        while {![eof $Handle]} {
            set FileLine [string trim [gets $Handle]]
            if {[string equal "" $FileLine]} {continue}
            ## Check config section
            if {[string index $FileLine 0] == "#"} {append ConfigComments $FileLine "\n"; continue
            } elseif {[string equal {[INVITES]} $FileLine]} {set ConfMode 1; continue
            } elseif {[string equal {[RIGHTS]} $FileLine]} {set ConfMode 2; continue
            } elseif {[string match {\[*\]} $FileLine]} {set ConfMode 0; continue}
            switch -- $ConfMode {
                1 {set invchan([lindex $FileLine 0]) [lindex $FileLine 1]}
                2 {set rights([lindex $FileLine 0]) [lindex $FileLine 1]}
            }
        }
        close $Handle
    } else {
        ErrorReturn "Unable to load the invite configuration, contact a siteop."
        ErrorLog InviteConfigLoader $ErrorMsg
    }
}

proc ::nxTools::Invite::ConfigWriter {ConfigFile} {
    upvar ConfigComments ConfigComments invchan invchan rights rights
    if {![catch {set Handle [open $ConfigFile w]} ErrorMsg]} {
        puts $Handle $ConfigComments
        puts $Handle "\[INVITES\]"
        foreach {Name Value} [array get invchan] {
            puts $Handle "$Name \"[lsort -ascii $Value]\""
        }
        puts $Handle "\n\[RIGHTS\]"
        foreach {Name Value} [array get rights] {
            puts $Handle "$Name \"$Value\""
        }
        close $Handle
    } else {ErrorLog InviteConfigWriter $ErrorMsg}
}

proc ::nxTools::Invite::FlagCheck {CurrentFlags NeedFlags} {
    set CurrentFlags [split $CurrentFlags ""]
    foreach NeedFlag [split $NeedFlags ""] {
        if {![string equal "" $NeedFlag] && [lsearch -glob $CurrentFlags $NeedFlag] != -1} {return 1}
    }
    return 0
}

proc ::nxTools::Invite::RightsCheck {UserName GroupNames Flags RightsList} {
    set Result 0
    foreach Rights $RightsList {
        if {[string index $Rights 0] == "!"} {
            set Rights [string range $Rights 1 end]
            if {[string index $Rights 0] == "-"} {
                set Rights [string range $Rights 1 end]
                if {[string match $Rights $UserName]} {return 0}
            } elseif {[string index $Rights 0] == "="} {
                set Rights [string range $Rights 1 end]
                if {[lsearch -glob $GroupNames $Rights] != -1} {return 0}
            } elseif {[FlagCheck $Flags $Rights]} {
                return 0
            }
        } elseif {[string index $Rights 0] == "-"} {
            set Rights [string range $Rights 1 end]
            if {[string match $Rights $UserName]} {set Result 1}
        } elseif {[string index $Rights 0] == "="} {
            set Rights [string range $Rights 1 end]
            if {[lsearch -glob $GroupNames $Rights] != -1} {set Result 1}
        } elseif {[FlagCheck $Flags $Rights]} {
            set Result 1
        }
    }
    return $Result
}

# Invite Main
######################################################################

proc ::nxTools::Invite::Main {ArgV} {
    global invite misc args flags group groups user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    foreach {Action Option Target Value} $ArgV {break}
    set Action [string tolower $Action]
    set Target [string tolower $Target]

    if {[string equal "invite" $Action]} {
        iputs ".-\[Invite\]---------------------------------------------------------------."
        ConfigLoader $invite(ConfigFile)

        if {[string equal "" $Option] || [string equal -nocase "help" $Option]} {
            LinePuts "Syntax : SITE INVITE <irc nick> \[target\]"
            LinePuts "Targets: [lsort -ascii [array names invchan]]"
        } elseif {[IsTrue $misc(dZSbotLogging)]} {
            LinePuts "Inviting the nickname \"$Option\"."
            putlog "INVITE: \"$user\" \"$group\" \"$Option\""
        } else {
            ## Check invite target
            if {[string equal "" $Target]} {set Target $invite(Default)}
            if {![info exists invchan($Target)] || [string equal "" $invchan($Target)]} {
                ErrorReturn "Invalid target, try \"SITE INVITE HELP\" to view available targets."
            }

            ## Check if the user has the rights to the specified target
            if {![info exists rights($Target)]} {
                ErrorReturn "The invite target \"$Target\" has no rights defined."
            } elseif {![RightsCheck $user $groups $flags $rights($Target)]} {
                ErrorReturn "You are not allowed to invite yourself to \"$Target\"."
            }
            set InvTarget $invchan($Target)
            LinePuts "Inviting the nickname \"$Option\" to the channel(s): [join $InvTarget {, }]"
            putlog "INVITE: \"$user\" \"$group\" \"$Option\" \"$InvTarget\""
        }
    } elseif {[string equal "edit" $Action]} {
        iputs ".-\[EditInvite\]-----------------------------------------------------------."
        ConfigLoader $invite(ConfigFile)

        set Option [string tolower $Option]
        switch -- $Option {
            {addinv} {
                if {[string equal "" $Target]} {
                    ErrorReturn "Invalid target, you must specify an invite target to add."
                } elseif {[info exists invchan($Target)]} {
                    ErrorReturn "The invite target \"$Target\" already exists, delete it first."
                } elseif {[string equal "" $Value]} {
                    ErrorReturn "The invite target \"$Target\" must have a destination channel."
                }

                ## Check if the channel is valid
                if {[string index $Value 0] != "#"} {
                    LinePuts "The destination channel \"$Value\" is invalid."
                    ErrorReturn "Note: Make sure the channel begins with a \"#\" character."
                }
                set invchan($Target) [list $Value]
                set rights($Target) "!*"
                LinePuts "Created invite target \"$Target\", destination set to \"$Value\"."
                LinePuts "Note: Add channels and edit the invite target's rights."
                ConfigWriter $invite(ConfigFile)
            }
            {delinv} {
                if {[string equal "" $Target] || ![info exists invchan($Target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                }
                unset -nocomplain invchan($Target) rights($Target)
                LinePuts "Removed the invite target \"$Target\" and all related settings."
                ConfigWriter $invite(ConfigFile)
            }
            {addchan} {
                if {[string equal "" $Target] || ![info exists invchan($Target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                } elseif {[string equal "" $Value]} {
                    ErrorReturn "Invalid channel, you must specify a channel to add."
                }

                ## Check if the channel already exists
                if {![info exists invchan($Target)]} {set invchan($Target) ""}
                foreach ChanEntry $invchan($Target) {
                    if {[string equal -nocase $ChanEntry $Value]} {
                        ErrorReturn "The channel \"$Value\" already exists in the invite target \"$Target\"."
                    }
                }
                lappend invchan($Target) $Value
                LinePuts "Added the channel \"$Value\" to the invite target \"$Target\"."
                ConfigWriter $invite(ConfigFile)
            }
            {delchan} {
                if {[string equal "" $Target] || ![info exists invchan($Target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                } elseif {[string equal "" $Value]} {
                    ErrorReturn "Invalid channel, you must specify a channel to remove."
                }
                set Deleted 0; set Index 0

                ## Remove the channel from the invite target
                if {![info exists invchan($Target)]} {set invchan($Target) ""}
                foreach ChanEntry $invchan($Target) {
                    if {[string equal -nocase $ChanEntry $Value]} {
                        set invchan($Target) [lreplace $invchan($Target) $Index $Index]
                        set Deleted 1; break
                    }
                    incr Index
                }
                if {$Deleted} {
                    LinePuts "Removed the channel \"$Value\" from the invite target \"$Target\"."
                    ConfigWriter $invite(ConfigFile)
                } else {
                    LinePuts "The channel \"$Value\" does not exist in the invite target \"$Target\"."
                }
            }
            {rights} {
                if {[string equal "" $Target] || ![info exists invchan($Target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                } elseif {[string equal "" $Value]} {
                    ErrorReturn "Invalid rights, you must specify the invite target's rights."
                }
                set rights($Target) $Value
                LinePuts "Invite target \"$Target\" rights set to \"$Value\"."
                ConfigWriter $invite(ConfigFile)
            }
            {view} {
                LinePuts "Targets:"
                foreach Name [lsort -ascii [array names invchan]] {
                    LinePuts [format "%-10s - %s" $Name $invchan($Name)]
                }
                LinePuts ""; LinePuts "Rights:"
                foreach Name [lsort -ascii [array names rights]] {
                    LinePuts [format "%-10s - %s" $Name $rights($Name)]
                }
            }
            default {
                set Option [string tolower $Target]
                switch -- $Option {
                    {addinv} {
                        LinePuts "Description:"
                        LinePuts " - Create an invite target and destination channel."
                        LinePuts "Syntax : SITE EDITINV ADDINV <target> <channel>"
                        LinePuts "Example: SITE EDITINV ADDINV Home #Xtreme"
                    }
                    {delinv} {
                        LinePuts "Description:"
                        LinePuts " - Delete an invite target and related settings."
                        LinePuts "Syntax : SITE EDITINV DELINV <target>"
                        LinePuts "Example: SITE EDITINV DELINV Home"
                    }
                    {addchan} {
                        LinePuts "Description:"
                        LinePuts " - Add a channel to a target's channel list."
                        LinePuts "Syntax : SITE EDITINV ADDCHAN <target> <channel>"
                        LinePuts "Example: SITE EDITINV ADDCHAN Home #Hangout"
                    }
                    {delchan} {
                        LinePuts "Description:"
                        LinePuts " - Delete a channel from a target's channel list."
                        LinePuts "Syntax : SITE EDITINV DELCHAN <target> <channel>"
                        LinePuts "Example: SITE EDITINV DELCHAN Home #Hangout"
                    }
                    {rights} {
                        LinePuts "Description:"
                        LinePuts " - Modify a target's rights and restrictions."
                        LinePuts " - You may restrict specific targets to users, groups, or flags."
                        LinePuts "Syntax : SITE EDITINV RIGHTS <target> <rights>"
                        LinePuts "Example: SITE EDITINV RIGHTS Home \"1M -someuser =Friends !A\""
                    }
                    default {
                        LinePuts "Description:"
                        LinePuts " - Used to edit invite targets and channels."
                        LinePuts " - For more detailed help, try \"SITE EDITINV HELP\" <option>"
                        LinePuts "Syntax : SITE EDITINV <option> <target> \[value\]"
                        LinePuts "Option : addinv delinv addchan delchan rights view"
                        LinePuts "Targets: [lsort -ascii [array names invchan]]"
                    }
                }
            }
        }
    } else {
        ErrorLog InvalidArgs "invalid parameter \"[info script] $Action\": check your ioFTPD.ini for errors"
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

::nxTools::Invite::Main [expr {[info exists args] ? $args : ""}]