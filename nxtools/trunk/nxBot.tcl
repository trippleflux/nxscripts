################################################################################
# nxTools - Sitebot Commands                                                   #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} ErrorMsg]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $ErrorMsg
}

namespace eval ::nxTools::Bot {
    namespace import -force ::nxLib::*
}

# Bot Procedures
######################################################################

proc ::nxTools::Bot::AuthCheck {UserName Password} {
    set Valid 0
    if {[userfile open $UserName] == 0} {
        set UserFile [userfile bin2ascii]
        foreach UserLine [split $UserFile "\r\n"] {
            if {[string equal -nocase "password" [lindex $UserLine 0]]} {
                set UserHash [lindex $UserLine 1]; break
            }
        }
        if {[string equal -nocase $UserHash [sha1 $Password]]} {set Valid 1}
    }
    if {$Valid} {iputs [list AUTH True]} else {iputs [list AUTH False]}
    return 0
}

proc ::nxTools::Bot::Bandwidth {} {
    array set who [list BwDn 0.0 BwUp 0.0 UsersDn 0 UsersUp 0 UsersIdle 0]
    if {[client who init "STATUS" "TRANSFERSPEED"] == 0} {
        while {[set WhoData [client who fetch]] != ""} {
            foreach {Status Speed} $WhoData {break}
            switch -- $Status {
                0 - 3 {
                    incr who(UsersIdle)
                }
                1 {
                    set who(BwDn) [expr {double($who(BwDn)) + double($Speed)}]
                    incr who(UsersDn)
                }
                2 {
                    set who(BwUp) [expr {double($who(BwUp)) + double($Speed)}]
                    incr who(UsersUp)
                }
            }
        }
    }
    set who(BwTotal) [expr {double($who(BwUp)) + double($who(BwDn))}]
    set who(UsersTotal) [expr {$who(UsersUp) + $who(UsersDn) + $who(UsersIdle)}]
    iputs [list BW $who(BwUp) $who(BwDn) $who(BwTotal) $who(UsersUp) $who(UsersDn) $who(UsersIdle) $who(UsersTotal)]
    return 0
}

proc ::nxTools::Bot::UserStats {StatsType Target StartIndex EndIndex} {
    set StatsList ""
    foreach UserName [GetUserList] {
        set Found 0; set GroupName "NoGroup"
        set FileStats 0; set SizeStats 0; set TimeStats 0
        if {[userfile open $UserName] != 0} {continue}
        set UserFile [userfile bin2ascii]
        foreach UserLine [split $UserFile "\r\n"] {
            set LineType [string tolower [lindex $UserLine 0]]
            if {[string equal "groups" $LineType]} {
                set GroupName [GetGroupName [lindex $UserLine 1]]
                incr Found
            } elseif {[string equal $StatsType $LineType]} {
                MergeStats [lrange $UserLine $StartIndex $EndIndex] FileStats SizeStats TimeStats
                incr Found
            }
            if {$Found == 2} {break}
        }
        lappend StatsList [list $UserName $GroupName $FileStats $SizeStats $TimeStats]
    }

    set Count 0
    set StatsList [lsort -decreasing -integer -index 3 $StatsList]
    foreach UserStats $StatsList {
        incr Count
        foreach {UserName GroupName FileStats SizeStats TimeStats} $UserStats {break}
        if {![string match $Target $UserName]} {continue}
        iputs [list USTATS $Count $UserName $GroupName $FileStats $SizeStats $TimeStats]
    }
    return 0
}

proc ::nxTools::Bot::GroupStats {StatsType Target StartIndex EndIndex} {
    set StatsList ""
    foreach GroupName [GetGroupList] {
        set FileStats 0; set SizeStats 0; set TimeStats 0
        foreach UserName [GetGroupUsers [resolve group $GroupName]] {
            if {[userfile open $UserName] != 0} {continue}
            set UserFile [userfile bin2ascii]
            foreach UserLine [split $UserFile "\r\n"] {
                if {[string equal -nocase $StatsType [lindex $UserLine 0]]} {
                    MergeStats [lrange $UserLine $StartIndex $EndIndex] FileStats SizeStats TimeStats
                    break
                }
            }
        }
        lappend StatsList [list $GroupName $FileStats $SizeStats $TimeStats]
    }

    set Count 0
    set StatsList [lsort -decreasing -integer -index 2 $StatsList]
    foreach GroupStats $StatsList {
        incr Count
        foreach {GroupName FileStats SizeStats TimeStats} $GroupStats {break}
        if {![string match $Target $GroupName]} {continue}
        iputs [list GSTATS $Count $GroupName $FileStats $SizeStats $TimeStats]
    }
    return 0
}

proc ::nxTools::Bot::UserInfo {UserName Section} {
    if {![string is digit -strict $Section] || $Section > 9} {set Section 1} else {incr Section}
    array set user [list Credits 0 Flags "" Group "NoGroup" Ratio 0 TagLine "No Tagline Set"]
    set file(alldn) 0; set size(alldn) 0; set time(alldn) 0
    set file(allup) 0; set size(allup) 0; set time(allup) 0

    if {[userfile open $UserName] != 0} {return 1}
    set UserFile [userfile bin2ascii]
    foreach UserLine [split $UserFile "\r\n"] {
        set LineType [string tolower [lindex $UserLine 0]]
        switch -- $LineType {
            {alldn} - {allup} {MergeStats [lrange $UserLine 1 end] file($LineType) size($LineType) time($LineType)}
            {credits} {set user(Credits) [lindex $UserLine $Section]}
            {flags}   {set user(Flags) [lindex $UserLine 1]}
            {groups}  {set user(Group) [GetGroupName [lindex $UserLine 1]]}
            {ratio}   {set user(Ratio) [lindex $UserLine $Section]}
            {tagline} {set user(TagLine) [string map {| ""} [ArgRange $UserLine 1 end]]}
        }
    }
    iputs [list USER $UserName $user(Group) $user(Flags) $user(TagLine) $user(Credits) $user(Ratio) $file(alldn) $size(alldn) $time(alldn) $file(allup) $size(allup) $time(allup)]
    return 0
}

proc ::nxTools::Bot::Who {} {
    if {[client who init "CID" "UID" "IDENT" "IP" "STATUS" "ACTION" "TIMEIDLE" "TRANSFERSPEED" "VIRTUALPATH" "VIRTUALDATAPATH"] == 0} {
        while {[set WhoData [client who fetch]] != ""} {
            foreach {ClientId UserId Ident IP Status Action IdleTime Speed VirtualPath DataPath} $WhoData {break}
            set UserName [resolve uid $UserId]
            set GroupName "NoGroup"
            if {[string match -nocase "pass *" $Action]} {set Action "PASS *****"}
            set FileName [file tail $DataPath]
            set Speed [expr {double($Speed)}]

            ## Find the user's primary group.
            if {[userfile open $UserName] == 0} {
                set UserFile [userfile bin2ascii]
                foreach UserLine [split $UserFile "\r\n"] {
                    if {[string equal -nocase "groups" [lindex $UserLine 0]]} {
                        set GroupName [GetGroupName [lindex $UserLine 1]]; break
                    }
                }
            }
            switch -- $Status {
                0 - 3 {set Status "IDLE"}
                1 {set Status "DNLD"}
                2 {set Status "UPLD"}
                default {continue}
            }
            iputs [list WHO $ClientId $UserName $GroupName $Ident $IP $Status $Action $VirtualPath $FileName $IdleTime $Speed]
        }
    }
    return 0
}

proc ::nxTools::Bot::Main {ArgV} {
    global misc ioerror
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}

    ## Safe argument handling.
    set ArgLength [llength [set ArgList [ArgList $ArgV]]]
    set Action [string tolower [lindex $ArgList 0]]
    set Result 0
    switch -- $Action {
        {auth} {
            if {$ArgLength > 2} {
                set Result [AuthCheck [lindex $ArgList 1] [lindex $ArgList 2]]
            } else {
                iputs "Syntax: SITE BOT AUTH <username> <password>"
            }
        }
        {bw} {
            set Result [Bandwidth]
        }
        {stats} {
            if {$ArgLength > 2} {
                ## Check stats type.
                set StatsType [string tolower [lindex $ArgList 1]]
                if {[lsearch -exact {alldn allup daydn dayup monthdn monthup wkdn wkup} $StatsType] == -1} {
                    set StatsType "Stats"
                }

                ## Check section number.
                set Section [lindex $ArgList 2]
                if {![string is digit -strict $Section] || $Section > 9} {
                    set StartIndex 1
                    set EndIndex "end"
                } else {
                    set StartIndex [incr Section]
                    set EndIndex [incr Section 2]
                }

                if {[string index [set Target [lindex $ArgList 2]] 0] == "="} {
                    set Target [string range $Target 1 end]
                    set Result [GroupStats $StatsType $Target $StartIndex $EndIndex]
                } else {
                    set Result [UserStats $StatsType $Target $StartIndex $EndIndex]
                }
            } else {
                iputs "Syntax: STATS <stats type> <username/=group> \[stats section\]"
            }
        }
        {user} {
            if {$ArgLength > 1} {
                set Result [UserInfo [lindex $ArgList 1] [lindex $ArgList 2]]
            } else {
                iputs "Syntax: SITE BOT USER <username> \[credit section\]"
            }
        }
        {who} {
            set Result [Who]
        }
        default {
            iputs "Syntax: SITE BOT <arguments>"
        }
    }
    return [set ioerror $Result]
}

::nxTools::Bot::Main [expr {[info exists args] ? $args : ""}]
