################################################################################
# nxTools - Stats and Tools                                                    #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Utils {
    namespace import -force ::nxLib::*
}

# User Procedures
######################################################################

proc ::nxTools::Utils::ChangeCredits {UserName Change {Section 0}} {
    incr Section
    if {[regexp {^(\+|\-)?(\d+)$} $Change Result Method Amount] && [userfile open $UserName] == 0} {
        set newUserFile ""
        userfile lock
        set userFile [userfile bin2ascii]
        foreach line [split $userFile "\r\n"] {
            if {[string equal -nocase "credits" [lindex $line 0]]} {
                if {![string length $Method]} {
                    set Value $Amount
                } else {
                    set Value [expr wide([lindex $line $Section]) $Method wide($Amount)]
                }
                set line [lreplace $line $Section $Section $Value]
            }
            append newUserFile $line "\r\n"
        }
        userfile ascii2bin $newUserFile
        userfile unlock
        return 1
    }
    return 0
}

proc ::nxTools::Utils::CheckHidden {UserName GroupName VirtualPath} {
    global hide
    if {[lsearch -exact $hide(UserNames) $UserName] != -1 || [lsearch -exact $hide(GroupNames) $GroupName] != -1} {return 1}
    if {[ListMatch $hide(Paths) $VirtualPath]} {return 1}
    return 0
}

proc ::nxTools::Utils::IsGroupAdmin {UserName Flags GroupId} {
    global misc
    if {![MatchFlags $misc(GAdminFlags) $Flags]} {
        return 1
    } elseif {[userfile open $UserName] == 0} {
        set userFile [userfile bin2ascii]
        if {[regexp -nocase {admingroups ([\s\d]+)} $userFile Result GroupIdList]} {
            if {[lsearch -exact $GroupIdList $GroupId] != -1} {return 1}
        }
    }
    return 0
}

proc ::nxTools::Utils::ResetuserFile {UserName StatsTypes {ResetCredits "False"}} {
    set newUserFile ""
    if {[userfile open $UserName] == 0} {
        userfile lock
        if {[IsTrue $ResetCredits]} {append newUserFile "credits" [string repeat " 0" 10] "\r\n"}
        foreach StatsField $StatsTypes {append newUserFile $StatsField [string repeat " 0" 30] "\r\n"}
        userfile ascii2bin $newUserFile
        userfile unlock
    }
}

# Tools Procedures
######################################################################

proc ::nxTools::Utils::NewDate {FindArea} {
    global misc newdate
    iputs ".-\[NewDate\]--------------------------------------------------------------."

    set DateArea "Default"
    foreach AreaName [array names newdate] {
        if {[string equal -nocase $AreaName $FindArea]} {set DateArea $AreaName; break}
    }
    if {![info exists newdate($DateArea)]} {
        ErrorLog NewDateArea "unknown newdate area \"$DateArea\""
        ErrorReturn "The newdate area \"$DateArea\" is not defined."
    }
    LinePuts "Creating [llength $newdate($DateArea)] date directories for the \"$DateArea\" area."
    set TimeNow [clock seconds]

    foreach NewArea $newdate($DateArea) {
        if {[llength $NewArea] != 10} {
            ErrorLog NewDate "wrong number of options in line: \"$NewArea\""; continue
        }
        foreach {AreaName Description VirtualPath RealPath SymLink DoLog DayOffset UserId GroupId Chmod} $NewArea {break}
        LinePuts ""; LinePuts "$AreaName ($Description):"

        # Format cookies and directory paths.
        set AreaTime [expr {$TimeNow + ($DayOffset * 86400)}]
        set VirtualPath [clock format $AreaTime -format $VirtualPath -gmt [IsTrue $misc(UtcTime)]]
        set RealPath [clock format $AreaTime -format $RealPath -gmt [IsTrue $misc(UtcTime)]]

        if {[file isdirectory $RealPath] || ![catch {file mkdir $RealPath} error]} {
            LinePuts "Created directory: $RealPath"
            catch {vfs write $RealPath $UserId $GroupId $Chmod}

            if {[string length $SymLink]} {
                if {[file isdirectory $SymLink] || ![catch {file mkdir $SymLink} error]} {
                    LinePuts "Created symlink: $SymLink"
                    catch {vfs chattr $SymLink 1 $VirtualPath}
                    LinePuts "Linked to vpath: $VirtualPath"
                } else {
                    LinePuts "Unable to create symlink: $SymLink"
                    ErrorLog NewDateLink $error
                }
            }
            if {[IsTrue $DoLog]} {putlog "NEWDATE: \"$VirtualPath\" \"$AreaName\" \"$Description\""}
        } else {
            LinePuts "Unable to create directory: $RealPath"
            ErrorLog NewDateDir $error
        }
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::OneLines {Message} {
    global misc group user
    if {[catch {DbOpenFile [namespace current]::OneDb "OneLines.db"} error]} {
        ErrorLog OneLinesDb $error
        return 1
    }
    if {![string length $Message]} {
        foreach fileExt {Header Body None Footer} {
            set template($fileExt) [ReadFile [file join $misc(Templates) "OneLines.$fileExt"]]
        }
        OutputText $template(Header)
        set Count 0
        OneDb eval {SELECT * FROM OneLines ORDER BY TimeStamp DESC LIMIT $misc(OneLines)} values {
            incr Count
            set ValueList [clock format $values(TimeStamp) -format {{%S} {%M} {%H} {%d} {%m} {%y} {%Y}} -gmt [IsTrue $misc(UtcTime)]]
            lappend ValueList $values(UserName) $values(GroupName) $values(Message)
            OutputText [ParseCookies $template(Body) $ValueList {sec min hour day month year2 year4 user group message}]
        }
        if {!$Count} {OutputText $template(None)}
        OutputText $template(Footer)
    } else {
        iputs ".-\[OneLines\]-------------------------------------------------------------."
        set TimeStamp [clock seconds]
        OneDb eval {INSERT INTO OneLines(TimeStamp,UserName,GroupName,Message) VALUES($TimeStamp,$user,$group,$Message)}
        LinePuts "Added message \"$Message\" by $user/$group."
        iputs "'------------------------------------------------------------------------'"
    }
    OneDb close
}

proc ::nxTools::Utils::RotateLogs {} {
    global log
    iputs ".-\[RotateLogs\]-----------------------------------------------------------."
    set DoRotate 0
    set TimeNow [clock seconds]
    switch -- [string tolower $log(Frequency)] {
        {month} - {monthly} {
            set DateFormat "%Y-%m"
            if {[clock format $TimeNow -format "%d"] eq "01"} {set DoRotate 1}
        }
        {week} - {weekly} {
            set DateFormat "%Y-Week%W"
            if {[clock format $TimeNow -format "%w"] eq "0"} {set DoRotate 1}
        }
        {day} - {daily} {set DateFormat "%Y-%m-%d"; set DoRotate 1}
        default {ErrorLog RotateLogs "invalid log rotation frequency: must be montly, weekly, or daily"}
    }
    if {!$DoRotate} {return 0}

    set MinimumSize [expr {$log(MinimumSize) * 1024 * 1024}]
    foreach LogFile $log(RotateList) {
        # Archive log file if it exists and meets the size requirement.
        if {![file isfile $LogFile]} {
            LinePuts "Skipping log \"[file tail $LogFile]\" - the file does not exist."
        } elseif {[file size $LogFile] < $MinimumSize} {
            LinePuts "Skipping log \"[file tail $LogFile]\" - the file is not larger then $log(MinimumSize)MB."
        } else {
            LinePuts "Rotated log \"[file tail $LogFile]\" successfully."
            if {[ArchiveFile $LogFile $DateFormat]} {
                catch {close [open $LogFile a]}
            } else {ErrorLog RotateLogs "unable to archive log file \"$LogFile\""}
        }
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SearchLog {LogFile MaxResults Pattern} {
    set LogData ""
    if {![catch {set Handle [open $LogFile r]} error]} {
        while {![eof $Handle]} {
            if {[gets $Handle LogLine] > 0 && [string match -nocase $Pattern $LogLine]} {
                set LogData [linsert $LogData 0 [string trim $LogLine]]
            }
        }
        close $Handle
    } else {ErrorLog SearchLog $error}

    set Count 0
    foreach LogLine $LogData {
        if {[incr Count] > $MaxResults} {break}
        iputs " $LogLine"
    }

    if {!$Count} {LinePuts "No results found."}
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::WeeklyCredits {WkTarget WkAmount} {
    global weekly
    iputs ".-\[WeeklyCredits\]--------------------------------------------------------."
    set CfgComments ""; set TargetList ""
    if {![catch {set Handle [open $weekly(ConfigFile) r]} error]} {
        while {![eof $Handle]} {
            set FileLine [string trim [gets $Handle]]
            if {[string index $FileLine 0] eq "#"} {
                append CfgComments $FileLine "\n"
            } elseif {[llength [set FileLine [split $FileLine "|"]]] == 3} {
                foreach {Target Section Credits} $FileLine {break}
                lappend TargetList [list $Target $Section $Credits]
            }
        }
        close $Handle
    } else {
        ErrorLog WeeklyRead $error
        ErrorReturn "Unable to load the weekly credits configuration, contact a siteop."
    }

    # Display weekly credit targets.
    if {![string length $WkTarget]} {
        if {[llength $TargetList]} {
            iputs "|     Target     |   Section   |  Credit Amount                          |"
            iputs "|------------------------------------------------------------------------|"
            foreach ListItem [lsort -ascii -index 0 $TargetList] {
                foreach {Target Section Credits} $ListItem {break}
                iputs [format "| %-14s | %11d | %-39s |" $Target $Section "[expr {wide($Credits) / 1024}]MB"]
            }
        } else {LinePuts "There are currently no weekly credit targets."}
    } elseif {[regexp {^(\d),((\+|\-)?\d+)$} $WkAmount Result Section Credits]} {
        # Add or remove weekly credit targets.
        set Deleted 0; set Index 0
        set CreditsKB [expr {wide($Credits) * 1024}]
        foreach ListItem $TargetList {
            if {$ListItem eq [list $WkTarget $Section $CreditsKB]} {
                set Deleted 1
                set TargetList [lreplace $TargetList $Index $Index]
            } else {incr Index}
        }
        if {$Deleted} {
            LinePuts "Removed the target \"$WkTarget\" (${Credits}MB in section $Section) from weekly credits."
        } else {
            # Check if the user or group exists.
            if {[string index $WkTarget 0] eq "="} {
                if {[resolve group [string range $WkTarget 1 end]] == -1} {
                    ErrorReturn "The specified group does not exist."
                }
            } elseif {[resolve user $WkTarget] == -1} {
                ErrorReturn "The specified user does not exist."
            }
            LinePuts "Added the target \"$WkTarget\" (${Credits}MB in section $Section) to weekly credits."
            lappend TargetList [list $WkTarget $Section $CreditsKB]
        }

        # Rewrite weekly configuration file.
        if {![catch {set Handle [open $weekly(ConfigFile) w]} error]} {
            puts -nonewline $Handle $CfgComments
            foreach ListItem [lsort -ascii -index 0 $TargetList] {
                puts $Handle [join $ListItem "|"]
            }
            close $Handle
        } else {ErrorLog WeeklyWrite $error}
    } else {
        LinePuts "Syntax:"
        LinePuts "  User Credits - SITE WEEKLY <username> <section>,<credits mb>"
        LinePuts " Group Credits - SITE WEEKLY =<group> <section>,<credits mb>"
        LinePuts "  List Targets - SITE WEEKLY"
        LinePuts "Notes:"
        LinePuts " - To add(+) or subtract(-) a credits, use the appropriate sign."
        LinePuts " - To remove a user or group, enter the target's section and credits."
    }
    iputs "'------------------------------------------------------------------------'"
}

proc ::nxTools::Utils::WeeklySet {} {
    global weekly
    iputs ".-\[WeeklySet\]------------------------------------------------------------."
    set TargetList ""
    if {![catch {set Handle [open $weekly(ConfigFile) r]} error]} {
        while {![eof $Handle]} {
            set FileLine [string trim [gets $Handle]]
            if {[string index $FileLine 0] ne "#" && [llength [set FileLine [split $FileLine "|"]]] == 3} {
                foreach {Target Section Credits} $FileLine {break}
                lappend TargetList [list $Target $Section $Credits]
            }
        }
        close $Handle
    } else {
        ErrorLog WeeklyRead $error
        ErrorReturn "Unable to load the weekly credits configuration, contact a siteop."
    }
    if {[llength $TargetList]} {
        foreach ListItem [lsort -ascii -index 0 $TargetList] {
            foreach {Target Section Credits} $ListItem {break}
            if {[string index $Target 0] eq "="} {
                set Target [string range $Target 1 end]
                if {[set GroupId [resolve group $Target]] == -1} {
                    LinePuts "Skipping invalid group \"$Target\"."; continue
                }
                set UserList [GetGroupUsers $GroupId]
                set UserCount [llength $UserList]
                # Split credits evenly amongst its group members.
                if {$UserCount > 1 && [IsTrue $weekly(SplitGroup)]} {
                    if {![regexp {^(\+|\-)?(\d+)$} $Credits Result Method Amount]} {continue}
                    set Credits $Method; append Credits [expr {wide($Amount) / $UserCount}]
                }
                LinePuts "Weekly credits given to $UserCount users in $Target ([expr {wide($Credits) / 1024}]MB in section $Section each)."
                foreach UserName $UserList {ChangeCredits $UserName $Credits $Section}
            } elseif {[resolve user $Target] == -1} {
                LinePuts "Skipping invalid user \"$Target\"."
            } else {
                LinePuts "Weekly credits given to $Target ([expr {wide($Credits) / 1024}]MB in section $Section)."
                ChangeCredits $Target $Credits $Section
            }
        }
    } else {LinePuts "There are currently no weekly credit targets."}
    iputs "'------------------------------------------------------------------------'"
}

# Site Commands
######################################################################

proc ::nxTools::Utils::SiteCredits {event Target Amount Section} {
    global group user
    iputs ".-\[Credits\]--------------------------------------------------------------."
    if {[resolve user $Target] == -1} {
        ErrorReturn "The specified user does not exist."
    }
    if {![regexp {^(\d+)(.*)$} $Amount Result Amount Unit]} {
        ErrorReturn "The specified amount \"$Amount\" is invalid."
    }
    set UnitName [string toupper [string index $Unit 0]]
    switch -- $UnitName {
        {G} {set Multi 1048576}
        {M} {set Multi 1024}
        {K} {set Multi 1}
        {}  {set Multi 1024; set UnitName "M"}
        default {ErrorReturn "The specified size unit \"$Unit\" is invalid."}
    }
    append UnitName "B"

    if {![string is digit -strict $Section] || $Section > 9} {set Section 0}
    set AmountKB [expr {wide($Amount) * $Multi}]
    set AmountMB [expr {wide($AmountKB) / 1024}]

    if {$event eq "GIVE"} {
        ChangeCredits $Target "+$AmountKB" $Section
        LinePuts "Gave $Amount$UnitName of credits to $Target in section $Section."
    } elseif {$event eq "TAKE"} {
        ChangeCredits $Target "-$AmountKB" $Section
        LinePuts "Took $Amount$UnitName of credits from $Target in section $Section."
    } else {
        ErrorLog SiteCredits "unknown event \"$event\""
    }
    putlog "${event}: \"$user\" \"$group\" \"$AmountMB\" \"$Target\""
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteDrives {} {
    iputs ".-\[Drives\]---------------------------------------------------------------."
    iputs "|  Volume Type  | File System |  Volume Name  | Free Space | Total Space |"
    iputs "|------------------------------------------------------------------------|"
    set Free 0; set Total 0
    foreach VolName [file volumes] {
        set VolName [string map {/ \\} $VolName]

        # We're only interested in fixed volumes and network volumes.
        switch -- [::nx::volume type $VolName] {
            3 {set TypeName "Fixed"}
            4 {set TypeName "Network"}
            default {continue}
        }
        if {[catch {::nx::volume info $VolName volume} error]} {
            LinePuts "$VolName Unable to retrieve volume information."
        } else {
            set Free [expr {wide($Free) + $volume(free)}]
            set Total [expr {wide($Total) + $volume(total)}]

            set volume(free) [FormatSize [expr {$volume(free) / 1024}]]
            set volume(total) [FormatSize [expr {$volume(total) / 1024}]]
            iputs [format "| %-3s %9s | %-11s | %-13s | %10s | %11s |" $VolName $TypeName $volume(fs) $volume(name) $volume(free) $volume(total)]
        }
    }
    iputs "|------------------------------------------------------------------------|"
    set Free [FormatSize [expr {wide($Free) / 1024}]]
    set Total [FormatSize [expr {wide($Total) / 1024}]]
    iputs [format "|                                       Total | %10s | %11s |" $Free $Total]
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteGroupInfo {GroupName Section} {
    global misc flags user
    iputs ".-\[GroupInfo\]------------------------------------------------------------."
    if {[set GroupId [resolve group $GroupName]] == -1} {
        ErrorReturn "The specified group does not exist."
        return 1
    } elseif {![IsGroupAdmin $user $flags $GroupId]} {
        ErrorReturn "You do not have admin rights for this group."
    }
    iputs "|  Username          |   All Up   |   All Dn   |   Ratio    |   Flags    |"
    iputs "|------------------------------------------------------------------------|"

    # Validate section number.
    if {![string is digit -strict $Section] || $Section > 9} {set Section 1} else {incr Section}
    set LeechCount 0
    set UserList [GetGroupUsers $GroupId]
    set file(alldn) 0; set size(alldn) 0; set time(alldn) 0
    set file(allup) 0; set size(allup) 0; set time(allup) 0

    foreach UserName $UserList {
        array set uinfo [list alldn 0 allup 0 AdminGroups "" Flags "" Prefix "" Ratio 0]
        if {[userfile open $UserName] == 0} {
            set userFile [userfile bin2ascii]
            foreach line [split $userFile "\r\n"] {
                set type [string tolower [lindex $line 0]]
                switch -- $type {
                    {admingroups} {set uinfo(AdminGroups) [lrange $line 1 end]}
                    {alldn} - {allup} {
                        MergeStats [lrange $line 1 end] file($type) uinfo($type) time($type)
                        set size($type) [expr {wide($uinfo($type)) + wide($size($type))}]
                    }
                    {flags} {set uinfo(Flags) [lindex $line 1]}
                    {ratio} {set uinfo(Ratio) [lindex $line $Section]}
                }
            }
        }
        if {$uinfo(Ratio) != 0} {
            set uinfo(Ratio) "1:$uinfo(Ratio)"
        } else {
            set uinfo(Ratio) "Unlimited"; incr LeechCount
        }

        # Siteop and group admin prefix.
        if {[MatchFlags $misc(SiteopFlags) $uinfo(Flags)]} {
            set uinfo(Prefix) "*"
        } elseif {[lsearch -exact $uinfo(AdminGroups) $GroupId] != -1} {
            set uinfo(Prefix) "+"
        }
        iputs [format "| %-18s | %10s | %10s | %-10s | %-10s |" $uinfo(Prefix)$UserName [FormatSize $uinfo(allup)] [FormatSize $uinfo(alldn)] $uinfo(Ratio) $uinfo(Flags)]
    }

    # Find the group's description and slot count.
    array set ginfo [list Slots "0 0" TagLine "No TagLine Set"]
    if {[groupfile open $GroupName] == 0} {
        set groupFile [groupfile bin2ascii]
        foreach line [split $groupFile "\r\n"] {
            set type [string tolower [lindex $line 0]]
            if {$type eq "description"} {
                set ginfo(TagLine) [ArgRange $line 1 end]
            } elseif {$type eq "slots"} {
                set ginfo(Slots) [lrange $line 1 2]
            }
        }
    }
    iputs "|------------------------------------------------------------------------|"
    iputs [format "| * Denotes SiteOp    + Denotes GAdmin    %30.30s |" $ginfo(TagLine)]
    iputs [format "| User Slots: %-5d   Leech Slots: %-37d |" [lindex $ginfo(Slots) 0] [lindex $ginfo(Slots) 1]]
    iputs "|------------------------------------------------------------------------|"
    iputs [format "| Total All Up: %10s  Total Files Up: %-10ld  Group Users: %-3d |" [FormatSize $size(allup)] $file(allup) [llength $UserList]]
    iputs [format "| Total All Dn: %10s  Total Files Dn: %-10ld   With Leech: %-3d |" [FormatSize $size(alldn)] $file(alldn) $LeechCount]
    iputs "'------------------------------------------------------------------------'"
    return 1
}

proc ::nxTools::Utils::SiteResetStats {ArgList} {
    set ResetStats ""
    set StatsTypes {alldn allup daydn dayup monthdn monthup wkdn wkup}
    iputs ".-\[ResetStats\]-----------------------------------------------------------."
    foreach Arg $ArgList {
        set Arg [string tolower $Arg]
        switch -- $Arg {
            {-all}  {set ResetStats $StatsTypes; break}
            {all}   {lappend ResetStats "alldn" "allup"}
            {month} {lappend ResetStats "monthdn" "monthup"}
            {wk} - {week} {lappend ResetStats "wkdn" "wkup"}
            {day}   {lappend ResetStats "daydn" "dayup"}
            default {if {[lsearch -exact $StatsTypes $Arg]} {lappend ResetStats $Arg}}
        }
    }
    if {![llength $ResetStats]} {
        LinePuts "No valid stats Types specified."
        LinePuts "Types: $StatsTypes"
    } else {
        foreach UserName [GetUserList] {ResetuserFile $UserName $ResetStats}
        LinePuts "Stats have been reset."
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteResetUser {UserName} {
    global reset
    iputs ".-\[ResetUser\]------------------------------------------------------------."
    if {[resolve user $UserName] == -1} {
        LinePuts "The specified user does not exist."
    } else {
        ResetuserFile $UserName {alldn allup daydn dayup monthdn monthup wkdn wkup} $reset(Credits)
        if {[IsTrue $reset(Credits)]} {LinePuts "Credits Reset...Complete."}
        LinePuts "Stats Reset.....Complete."
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteSize {VirtualPath} {
    global size
    iputs ".-\[Size\]-----------------------------------------------------------------."
    set RealPath [resolve pwd $VirtualPath]
    if {![file exists $RealPath]} {
        ErrorReturn "The specified file or directory does not exist."
    }

    GetDirStats $RealPath stats ".ioFTPD*"
    set stats(TotalSize) [expr {wide($stats(TotalSize)) / 1024}]
    LinePuts "$stats(FileCount) File(s), $stats(DirCount) Directory(s), [FormatSize $stats(TotalSize)]"
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteTraffic {Target} {
    iputs ".-\[Traffic\]--------------------------------------------------------------."
    if {![string length $Target]} {
        set TrafficType 0
        set UserList [GetUserList]
    } elseif {[string index $Target 0] eq "="} {
        set Target [string range $Target 1 end]
        if {[resolve group $Target] == -1} {
            ErrorReturn "The specified group does not exist."
        }
        set TrafficType 1
        set UserList [GetGroupUsers [resolve group $Target]]
    } else {
        if {[resolve user $Target] == -1} {
            ErrorReturn "The specified user does not exist."
        }
        set TrafficType 2
        set UserList $Target
    }
    array set file [list alldn 0 allup 0 daydn 0 dayup 0 monthdn 0 monthup 0 wkdn 0 wkup 0]
    array set size [list alldn 0 allup 0 daydn 0 dayup 0 monthdn 0 monthup 0 wkdn 0 wkup 0]
    array set time [list alldn 0 allup 0 daydn 0 dayup 0 monthdn 0 monthup 0 wkdn 0 wkup 0]
    foreach UserName $UserList {
        if {[userfile open $UserName] != 0} {continue}
        set userFile [userfile bin2ascii]
        foreach line [split $userFile "\r\n"] {
            set type [string tolower [lindex $line 0]]
            if {[lsearch -exact {alldn allup daydn dayup monthdn monthup wkdn wkup} $type] != -1} {
                MergeStats [lrange $line 1 end] file($type) size($type) time($type)
            }
        }
    }
    iputs "|     Totals      |      Files      |      Amount     |      Speed       |"
    iputs "|------------------------------------------------------------------------|"
    iputs [format "| Total Uploads   | %15ld | %15s | %16s |" $file(allup) [FormatSize $size(allup)] [FormatSpeed $size(allup) $time(allup)]]
    iputs [format "| Total Downloads | %15ld | %15s | %16s |" $file(alldn) [FormatSize $size(alldn)] [FormatSpeed $size(alldn) $time(alldn)]]
    iputs [format "| Month Uploads   | %15ld | %15s | %16s |" $file(monthup) [FormatSize $size(monthup)] [FormatSpeed $size(monthup) $time(monthup)]]
    iputs [format "| Month Downloads | %15ld | %15s | %16s |" $file(monthdn) [FormatSize $size(monthdn)] [FormatSpeed $size(monthdn) $time(monthdn)]]
    iputs [format "| Week Uploads    | %15ld | %15s | %16s |" $file(wkup) [FormatSize $size(wkup)] [FormatSpeed $size(wkup) $time(wkup)]]
    iputs [format "| Week Downloads  | %15ld | %15s | %16s |" $file(wkdn) [FormatSize $size(wkdn)] [FormatSpeed $size(wkdn) $time(wkdn)]]
    iputs [format "| Day Uploads     | %15ld | %15s | %16s |" $file(dayup) [FormatSize $size(dayup)] [FormatSpeed $size(dayup) $time(dayup)]]
    iputs [format "| Day Downloads   | %15ld | %15s | %16s |" $file(daydn) [FormatSize $size(daydn)] [FormatSpeed $size(daydn) $time(daydn)]]
    iputs "|------------------------------------------------------------------------|"
    switch -- $TrafficType {
        0 {LinePuts "Stats for all [llength $UserList] user(s)."}
        1 {LinePuts "Stats for the group $Target."}
        2 {LinePuts "Stats for the user $Target."}
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteWho {} {
    global misc cid flags
    array set who [list BwDn 0.0 BwUp 0.0 UsersDn 0 UsersUp 0 UsersIdle 0]
    set IsAdmin [MatchFlags $misc(SiteopFlags) $flags]
    iputs ".------------------------------------------------------------------------."
    iputs "|    User    |   Group    |  Info          |  Action                     |"
    iputs "|------------------------------------------------------------------------|"
    if {[client who init "CID" "UID" "STATUS" "TIMEIDLE" "TRANSFERSPEED" "VIRTUALPATH" "VIRTUALDATAPATH"] == 0} {
        while {[set WhoData [client who fetch]] ne ""} {
            foreach {ClientId UserId Status IdleTime Speed VirtualPath DataPath} $WhoData {break}
            set IsMe [expr {$cid == $ClientId ? "*" : ""}]
            set UserName [resolve uid $UserId]
            set GroupName "NoGroup"; set TagLine "No Tagline Set"
            set FileName [file tail $DataPath]

            # Find the user's group and tagline.
            if {[userfile open $UserName] == 0} {
                set userFile [userfile bin2ascii]
                foreach line [split $userFile "\r\n"] {
                    set type [string tolower [lindex $line 0]]
                    if {$type eq "groups"} {
                        set GroupName [GetGroupName [lindex $line 1]]
                    } elseif {$type eq "tagline"} {
                        set TagLine [ArgRange $line 1 end]
                    }
                }
            }

            # Show hidden users to either admins or the user.
            if {$IsAdmin || $cid == $ClientId || ![CheckHidden $UserName $GroupName $VirtualPath]} {
                switch -- $Status {
                    0 - 3 {
                        set Action "IDLE: [FormatDuration $IdleTime]"
                        incr who(UsersIdle)
                    }
                    1 {
                        set Action [format "DL: %-12.12s - %.0fKB/s" $FileName $Speed]
                        set who(BwDn) [expr {double($who(BwDn)) + double($Speed)}]
                        incr who(UsersDn)
                    }
                    2 {
                        set Action [format "UL: %-12.12s - %.0fKB/s" $FileName $Speed]
                        set who(BwUp) [expr {double($who(BwUp)) + double($Speed)}]
                        incr who(UsersUp)
                    }
                    default {continue}
                }
                iputs [format "| %-10.10s | %-10.10s | %-14.14s | %-27s |" "$IsMe$UserName" $GroupName $TagLine $Action]
            }
        }
    }
    set who(BwTotal) [expr {double($who(BwUp)) + double($who(BwDn))}]
    set who(UsersTotal) [expr {$who(UsersUp) + $who(UsersDn) + $who(UsersIdle)}]
    iputs "|------------------------------------------------------------------------|"
    iputs [format "| Up: %-16s | Dn: %-16s | Total: %-17s |" "$who(UsersUp)@$who(BwUp)KB/s" "$who(UsersDn)@$who(BwDn)KB/s" "$who(UsersTotal)@$who(BwTotal)KB/s"]
    iputs "'------------------------------------------------------------------------'"
    return 0
}

# Tools Main
######################################################################

proc ::nxTools::Utils::Main {ArgV} {
    global IsSiteBot log misc group ioerror pwd user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set IsSiteBot [expr {[info exists user] && $misc(SiteBot) eq $user}]
    set result 0

    set ArgLength [llength [set ArgList [ArgList $ArgV]]]
    set event [string toupper [lindex $ArgList 0]]
    switch -- $event {
        {DAYSTATS} {
            if {![IsTrue $misc(dZSbotLogging)]} {
                putlog "DAYSTATS: \"Launch Daystats\""
            }
        }
        {DRIVES} {
            set result [SiteDrives]
        }
        {ERRLOG} {
            if {$ArgLength > 1 && [GetOptions [lrange $ArgList 1 end] MaxResults Pattern]} {
                iputs ".-\[ErrorLog\]-------------------------------------------------------------."
                set result [SearchLog $log(Error) $MaxResults $Pattern]
            } else {
                iputs "Syntax: SITE ERRLOG \[-max <limit>\] <pattern>"
            }
        }
        {GINFO} {
            if {$ArgLength > 1 && [string is digit [lindex $ArgList 2]]} {
                set result [SiteGroupInfo [lindex $ArgList 1] [lindex $ArgList 2]]
            } else {
                iputs "Syntax: SITE GINFO <group> \[credit section\]"
            }
            set result 1
        }
        {GIVE} - {TAKE} {
            foreach {Target Amount Section} [lrange $ArgList 1 end] {break}
            if {$ArgLength > 2} {
                set result [SiteCredits $event $Target $Amount $Section]
            } else {
                iputs "Syntax: SITE $event <username> <credits> \[credit section\]"
            }
        }
        {NEWDATE} {
            set result [NewDate [lindex $ArgList 1]]
        }
        {ONELINES} {
            set result [OneLines [join [lrange $ArgList 1 end]]]
        }
        {RESETSTATS} {
            if {$ArgLength > 1} {
                set result [SiteResetStats [join [lrange $ArgList 1 end]]]
            } else {
                iputs "Syntax: SITE RESETSTATS <stats type(s)>"
            }
        }
        {RESETUSER} {
            if {$ArgLength > 1} {
                set result [SiteResetUser [lindex $ArgList 1]]
            } else {
                iputs "Syntax: SITE RESETUSER <username>"
            }
        }
        {ROTATE} {
            set result [RotateLogs]
        }
        {SIZE} {
            if {$ArgLength > 1} {
                set VirtualPath [GetPath $pwd [join [lrange $ArgList 1 end]]]
                set result [SiteSize $VirtualPath]
            } else {
                iputs " Usage: SITE SIZE <file/directory>"
            }
        }
        {SYSLOG} {
            if {$ArgLength > 1 && [GetOptions [lrange $ArgList 1 end] MaxResults Pattern]} {
                iputs ".-\[SysopLog\]-------------------------------------------------------------."
                set result [SearchLog $log(SysOp) $MaxResults $Pattern]
            } else {
                iputs "Syntax: SITE SYSLOG \[-max <limit>\] <pattern>"
            }
        }
        {TRAFFIC} {
            set result [SiteTraffic [lindex $ArgList 1]]
        }
        {WEEKLY} {
            set result [WeeklyCredits [lindex $ArgList 1] [lindex $ArgList 2]]
        }
        {WEEKLYSET} {
            set result [WeeklySet]
        }
        {WHO} {
            set result [SiteWho]
        }
        default {
            ErrorLog InvalidArgs "unknown event \"[info script] $event\": check your ioFTPD.ini for errors"
            set result 1
        }
    }
    return [set ioerror $result]
}

::nxTools::Utils::Main [expr {[info exists args] ? $args : ""}]
