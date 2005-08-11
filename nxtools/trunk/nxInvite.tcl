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

proc ::nxTools::Invite::ConfigRead {configFile} {
    upvar ConfigComments ConfigComments invchan invchan rights rights
    set configComments ""
    set configSection -1
    if {![catch {set handle [open $configFile r]} error]} {
        while {![eof $handle]} {
            set line [string trim [gets $handle]]
            if {![string length $line]} {continue}

            if {[string index $line 0] eq "#"} {
                append configComments $line "\n"; continue
            }
            if {[string match {\[*\]} $line]} {
                set configSection [lsearch -exact {[INVITES] [RIGHTS]} $line]
            } else {
                switch -- $configSection {
                    0 {set invchan([lindex $line 0]) [lindex $line 1]}
                    1 {set rights([lindex $line 0]) [lindex $line 1]}
                }
            }
        }
        close $handle
    } else {
        ErrorReturn "Unable to load the invite configuration, contact a siteop."
        ErrorLog InviteConfigRead $error
    }
}

proc ::nxTools::Invite::ConfigWrite {configFile} {
    upvar ConfigComments ConfigComments invchan invchan rights rights
    if {![catch {set handle [open $configFile w]} error]} {
        puts $handle $configComments
        puts $handle "\[INVITES\]"
        foreach {name value} [array get invchan] {
            puts $handle "$name \"[lsort -ascii $value]\""
        }
        puts $handle "\n\[RIGHTS\]"
        foreach {name value} [array get rights] {
            puts $handle "$name \"$value\""
        }
        close $handle
    } else {ErrorLog InviteConfigWrite $error}
}

proc ::nxTools::Invite::FlagCheck {currentFlags needFlags} {
    set currentFlags [split $currentFlags ""]
    foreach needFlag [split $needFlags ""] {
        if {[string length $needFlag] && [lsearch -glob $currentFlags $needFlag] != -1} {return 1}
    }
    return 0
}

proc ::nxTools::Invite::RightsCheck {rightsList userName groupList flags} {
    foreach right $rightsList {
        regexp {^(!?[=-]?)(.+)} $right result prefix right
        switch -- $prefix {
            {!-} {if {[string match $right $userName]} {return 0}}
            {!=} {if {[lsearch -glob $groupList $right] != -1} {return 0}}
            {!}  {if {[FlagCheck $flags $right]} {return 0}}
            {-}  {if {[string match $right $userName]} {return 1}}
            {=}  {if {[lsearch -glob $groupList $right] != -1} {return 1}}
            default {if {[FlagCheck $flags $right]} {return 1}}
        }
    }
    return 0
}

# Invite Main
######################################################################

proc ::nxTools::Invite::Main {argv} {
    global invite misc flags group groups user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}

    set argList [ArgList $argv]
    foreach {event option target value} $argv {break}
    set event [string tolower $event]
    set target [string tolower $target]

    if {[string equal "invite" $event]} {
        iputs ".-\[Invite\]---------------------------------------------------------------."
        ConfigRead $invite(ConfigFile)

        if {![string length $option] || [string equal -nocase "help" $option]} {
            LinePuts "Syntax : SITE INVITE <irc nick> \[target\]"
            LinePuts "Targets: [lsort -ascii [array names invchan]]"
        } elseif {[IsTrue $misc(dZSbotLogging)]} {
            LinePuts "Inviting the nickname \"$option\"."
            putlog "INVITE: \"$user\" \"$group\" \"$option\""
        } else {
            if {![string length $target]} {set target $invite(Default)}
            if {![info exists invchan($target)] || ![string length $invchan($target)]} {
                ErrorReturn "Invalid target, try \"SITE INVITE HELP\" to view available targets."
            }

            # Check if the user has access to the specified target.
            if {![info exists rights($target)]} {
                ErrorReturn "The invite target \"$target\" has no rights defined."
            } elseif {![RightsCheck $rights($target) $user $groups $flags]} {
                ErrorReturn "You do not have access to the \"$target\" target."
            }
            set invTarget $invchan($target)
            LinePuts "Inviting \"$option\" to: [JoinLiteral $invTarget]"
            putlog "INVITE: \"$user\" \"$group\" \"$option\" \"$invTarget\""
        }
    } elseif {[string equal "edit" $event]} {
        iputs ".-\[EditInvite\]-----------------------------------------------------------."
        ConfigRead $invite(ConfigFile)

        set option [string tolower $option]
        switch -- $option {
            {addinv} {
                if {![string length $target]} {
                    ErrorReturn "Invalid target, you must specify an invite target to add."
                } elseif {[info exists invchan($target)]} {
                    ErrorReturn "The invite target \"$target\" already exists, delete it first."
                } elseif {![string length $value]} {
                    ErrorReturn "The invite target \"$target\" must have a destination channel."
                }

                if {[string index $value 0] ne "#"} {
                    LinePuts "The destination channel \"$value\" is invalid."
                    ErrorReturn "Note: Make sure the channel begins with a \"#\" character."
                }
                set invchan($target) [list $value]
                set rights($target) "!*"
                LinePuts "Created invite target \"$target\", destination set to \"$value\"."
                LinePuts "Note: Add channels and edit the invite target's rights."
                ConfigWrite $invite(ConfigFile)
            }
            {delinv} {
                if {![string length $target] || ![info exists invchan($target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                }
                unset -nocomplain invchan($target) rights($target)
                LinePuts "Removed the invite target \"$target\" and all related settings."
                ConfigWrite $invite(ConfigFile)
            }
            {addchan} {
                if {![string length $target] || ![info exists invchan($target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                } elseif {![string length $value]} {
                    ErrorReturn "Invalid channel, you must specify a channel to add."
                }

                if {![info exists invchan($target)]} {set invchan($target) ""}
                foreach entry $invchan($target) {
                    if {[string equal -nocase $entry $value]} {
                        ErrorReturn "The channel \"$value\" already exists in the invite target \"$target\"."
                    }
                }
                lappend invchan($target) $value
                LinePuts "Added the channel \"$value\" to the invite target \"$target\"."
                ConfigWrite $invite(ConfigFile)
            }
            {delchan} {
                if {![string length $target] || ![info exists invchan($target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                } elseif {![string length $value]} {
                    ErrorReturn "Invalid channel, you must specify a channel to remove."
                }
                set deleted 0; set index 0
                if {![info exists invchan($target)]} {set invchan($target) ""}
                foreach entry $invchan($target) {
                    if {[string equal -nocase $entry $value]} {
                        set invchan($target) [lreplace $invchan($target) $index $index]
                        set deleted 1; break
                    }
                    incr index
                }
                if {$deleted} {
                    LinePuts "Removed the channel \"$value\" from the invite target \"$target\"."
                    ConfigWrite $invite(ConfigFile)
                } else {
                    LinePuts "The channel \"$value\" does not exist in the invite target \"$target\"."
                }
            }
            {rights} {
                if {![string length $target] || ![info exists invchan($target)]} {
                    ErrorReturn "Invalid target, try \"SITE EDITINV HELP\" to view available targets."
                } elseif {![string length $value]} {
                    ErrorReturn "Invalid rights, you must specify the invite target's rights."
                }
                set rights($target) $value
                LinePuts "Invite target \"$target\" rights set to \"$value\"."
                ConfigWrite $invite(ConfigFile)
            }
            {view} {
                LinePuts "Targets:"
                foreach name [lsort -ascii [array names invchan]] {
                    LinePuts [format "%-10s - %s" $name $invchan($name)]
                }
                LinePuts ""; LinePuts "Rights:"
                foreach name [lsort -ascii [array names rights]] {
                    LinePuts [format "%-10s - %s" $name $rights($name)]
                }
            }
            default {
                set option [string tolower $target]
                switch -- $option {
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
