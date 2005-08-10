################################################################################
# nxTools - Invite Script                                                      #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Invite {
    namespace import -force ::nxLib::*
}

# Invite Procedures
######################################################################

proc ::nxTools::Invite::ConfigRead {ConfigFile} {
    upvar ConfigComments ConfigComments invchan invchan rights rights
    set ConfigComments ""
    set ConfigSection -1
    if {![catch {set Handle [open $ConfigFile r]} error]} {
        while {![eof $Handle]} {
            set FileLine [string trim [gets $Handle]]
            if {![string length $FileLine]} {continue}

            if {[string index $FileLine 0] eq "#"} {
                append ConfigComments $FileLine "\n"; continue
            }
            if {[string match {\[*\]} $FileLine]} {
                set ConfigSection [lsearch -exact {[INVITES] [RIGHTS]} $FileLine]
            } else {
                switch -- $ConfigSection {
                    0 {set invchan([lindex $FileLine 0]) [lindex $FileLine 1]}
                    1 {set rights([lindex $FileLine 0]) [lindex $FileLine 1]}
                }
            }
        }
        close $Handle
    } else {
        ErrorReturn "Unable to load the invite configuration, contact a siteop."
        ErrorLog InviteConfigRead $error
    }
}

proc ::nxTools::Invite::ConfigWrite {ConfigFile} {
    upvar ConfigComments ConfigComments invchan invchan rights rights
    if {![catch {set Handle [open $ConfigFile w]} error]} {
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
    } else {ErrorLog InviteConfigWrite $error}
}

proc ::nxTools::Invite::FlagCheck {CurrentFlags NeedFlags} {
    set CurrentFlags [split $CurrentFlags ""]
    foreach NeedFlag [split $NeedFlags ""] {
        if {[string length $NeedFlag] && [lsearch -glob $CurrentFlags $NeedFlag] != -1} {return 1}
    }
    return 0
}

proc ::nxTools::Invite::RightsCheck {RightsList UserName GroupNames Flags} {
    foreach Right $RightsList {
        regexp {^(!?[=-]?)(.+)} $Right Result Prefix Right
        switch -- $Prefix {
            {!-} {if {[string match $Right $UserName]} {return 0}}
            {!=} {if {[lsearch -glob $GroupNames $Right] != -1} {return 0}}
            {!}  {if {[FlagCheck $Flags $Right]} {return 0}}
            {-}  {if {[string match $Right $UserName]} {return 1}}
            {=}  {if {[lsearch -glob $GroupNames $Right] != -1} {return 1}}
            default {if {[FlagCheck $Flags $Right]} {return 1}}
        }
    }
    return 0
}

# Invite Main
######################################################################

proc ::nxTools::Invite::Main {ArgV} {
    global invite misc flags group groups user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}

    set ArgList [ArgList $ArgV]
    foreach {event Option Target Value} $ArgV {break}
    set event [string tolower $event]
    set Target [string tolower $Target]

    if {[string equal "invite" $event]} {
        iputs ".-\[Invite\]---------------------------------------------------------------."
        ConfigRead $invite(ConfigFile)

        if {![string length $Option] || [string equal -nocase "help" $Option]} {
            LinePuts "Syntax : SITE INVITE <irc nick> \[target\]"
            LinePuts "Targets: [lsort -ascii [array names invchan]]"
        } elseif {[IsTrue $misc(dZSbotLogging)]} {
            LinePuts "Inviting the nickname \"$Option\"."
            putlog "INVITE: \"$user\" \"$group\" \"$Option\""
        } else {
            if {![string length $Target]} {set Target $invite(Default)}
            if {![info exists invchan($Target)] || ![string length $invchan($Target)]} {
                ErrorReturn "Invalid target, try \"SITE INVITE HELP\" to view available targets."
            }

            # Check if the user has access to the specified target.
            if {![info exists rights($Target)]} {
                ErrorReturn "The invite target \"$Target\" has no rights defined."
            } elseif {![RightsCheck $rights($Target) $user $groups $flags]} {
                ErrorReturn "You do not have access to the \"$Target\" target."
            }
            set InvTarget $invchan($Target)
            LinePuts "Inviting \"$Option\" to: [JoinLiteral $InvTarget]"
            putlog "INVITE: \"$user\" \"$group\" \"$Option\" \"$InvTarget\""
        }
    } elseif {[string equal "edit" $event]} {
        iputs ".-\[EditInvite\]-----------------------------------------------------------."
        ConfigRead $invite(ConfigFile)

        set Option [string tolower $Option]
        switch -- $Option {
            {addinv} {
                if {![string length $Target]} {
                    ErrorReturn "Invalid target, you must specify an invite target to add."
                } elseif {[info exists invchan($Target)]} {
                    ErrorReturn "The invite target \"$Target\" already exists, delete it first."
                } elseif {![string length $Value]} {
                    ErrorReturn "The invite target \"$Target\" must have a destination channel."
                }

                if {[string index $Value 0] ne "#"} {
                    LinePuts "The destination channel \"$Value\" is invalid."
                    ErrorReturn "Note: Make sure the channel begins with a \"#\" character."
                }
                set invchan($Target) [list $Value]
                set rights($Target) "!*"
                LinePuts "Created invite target \"$Target\", destination set to \"$Value\"."
                LinePuts "Note: Add channels and edit the invite target's rights."
                ConfigWrite $invite(ConfigFile)
            }
            {delinv} {
                if {![string length $Target] || ![info exists invchan($Target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                }
                unset -nocomplain invchan($Target) rights($Target)
                LinePuts "Removed the invite target \"$Target\" and all related settings."
                ConfigWrite $invite(ConfigFile)
            }
            {addchan} {
                if {![string length $Target] || ![info exists invchan($Target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                } elseif {![string length $Value]} {
                    ErrorReturn "Invalid channel, you must specify a channel to add."
                }

                if {![info exists invchan($Target)]} {set invchan($Target) ""}
                foreach ChanEntry $invchan($Target) {
                    if {[string equal -nocase $ChanEntry $Value]} {
                        ErrorReturn "The channel \"$Value\" already exists in the invite target \"$Target\"."
                    }
                }
                lappend invchan($Target) $Value
                LinePuts "Added the channel \"$Value\" to the invite target \"$Target\"."
                ConfigWrite $invite(ConfigFile)
            }
            {delchan} {
                if {![string length $Target] || ![info exists invchan($Target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                } elseif {![string length $Value]} {
                    ErrorReturn "Invalid channel, you must specify a channel to remove."
                }
                set Deleted 0; set Index 0
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
                    ConfigWrite $invite(ConfigFile)
                } else {
                    LinePuts "The channel \"$Value\" does not exist in the invite target \"$Target\"."
                }
            }
            {rights} {
                if {![string length $Target] || ![info exists invchan($Target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                } elseif {![string length $Value]} {
                    ErrorReturn "Invalid rights, you must specify the invite target's rights."
                }
                set rights($Target) $Value
                LinePuts "Invite target \"$Target\" rights set to \"$Value\"."
                ConfigWrite $invite(ConfigFile)
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
        ErrorLog InvalidArgs "unknown event \"[info script] $event\": check your ioFTPD.ini for errors"
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

::nxTools::Invite::Main [expr {[info exists args] ? $args : ""}]
