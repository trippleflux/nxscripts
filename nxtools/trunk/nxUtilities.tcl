################################################################################
# nxTools - Stats and Tools                                                    #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} ErrorMsg]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $ErrorMsg
}

namespace eval ::nxTools::Utils {
    namespace import -force ::nxLib::*
}

# User Procedures
######################################################################

proc ::nxTools::Utils::ChangeCredits {UserName Change {Section 0}} {
    incr Section
    if {[regexp {^(\+|\-)?(\d+)$} $Change Result Method Amount] && [userfile open $UserName] == 0} {
        set NewUserFile ""
        userfile lock
        set UserFile [userfile bin2ascii]
        foreach UserLine [split $UserFile "\r\n"] {
            if {[string equal -nocase "credits" [lindex $UserLine 0]]} {
                if {![string length $Method]} {
                    set Value $Amount
                } else {
                    set Value [expr wide([lindex $UserLine $Section]) $Method wide($Amount)]
                }
                set UserLine [lreplace $UserLine $Section $Section $Value]
            }
            append NewUserFile $UserLine "\r\n"
        }
        userfile ascii2bin $NewUserFile
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
    if {![regexp "\[$misc(GAdminFlags)\]" $Flags]} {
        return 1
    } elseif {[userfile open $UserName] == 0} {
        set UserFile [userfile bin2ascii]
        if {[regexp -nocase {admingroups ([\s\d]+)} $UserFile Result GroupIdList]} {
            if {[lsearch -exact $GroupIdList $GroupId] != -1} {return 1}
        }
    }
    return 0
}

proc ::nxTools::Utils::ResetUserFile {UserName StatsTypes {ResetCredits "False"}} {
    set NewUserFile ""
    if {[userfile open $UserName] == 0} {
        userfile lock
        if {[IsTrue $ResetCredits]} {append NewUserFile "credits" [string repeat " 0" 10] "\r\n"}
        foreach StatsField $StatsTypes {append NewUserFile $StatsField [string repeat " 0" 30] "\r\n"}
        userfile ascii2bin $NewUserFile
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

        ## Format cookies and directory paths.
        set AreaTime [expr {$TimeNow + ($DayOffset * 86400)}]
        set VirtualPath [clock format $AreaTime -format $VirtualPath -gmt [IsTrue $misc(UtcTime)]]
        set RealPath [clock format $AreaTime -format $RealPath -gmt [IsTrue $misc(UtcTime)]]

        if {[file isdirectory $RealPath] || ![catch {file mkdir $RealPath} ErrorMsg]} {
            LinePuts "Created directory: $RealPath"
            catch {vfs write $RealPath $UserId $GroupId $Chmod}

            if {[string length $SymLink]} {
                if {[file isdirectory $SymLink] || ![catch {file mkdir $SymLink} ErrorMsg]} {
                    LinePuts "Created symlink: $SymLink"
                    catch {vfs chattr $SymLink 1 $VirtualPath}
                    LinePuts "Linked to vpath: $VirtualPath"
                } else {
                    LinePuts "Unable to create symlink: $SymLink"
                    ErrorLog NewDateLink $ErrorMsg
                }
            }
            if {[IsTrue $DoLog]} {putlog "NEWDATE: \"$VirtualPath\" \"$AreaName\" \"$Description\""}
        } else {
            LinePuts "Unable to create directory: $RealPath"
            ErrorLog NewDateDir $ErrorMsg
        }
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::OneLines {Message} {
    global misc group user
    if {[catch {DbOpenFile [namespace current]::OneDb "OneLines.db"} ErrorMsg]} {
        ErrorLog OneLinesDb $ErrorMsg
        return 1
    }
    if {![string length $Message]} {
        foreach MessageType {Header Body None Footer} {
            set template($MessageType) [ReadFile [file join $misc(Templates) "OneLines.$MessageType"]]
        }
        OutputData $template(Header)
        set Count 0
        OneDb eval {SELECT * FROM OneLines ORDER BY TimeStamp DESC LIMIT $misc(OneLines)} values {
            incr Count
            set ValueList [clock format $values(TimeStamp) -format {{%S} {%M} {%H} {%d} {%m} {%y} {%Y}} -gmt [IsTrue $misc(UtcTime)]]
            lappend ValueList $values(UserName) $values(GroupName) $values(Message)
            OutputData [ParseCookies $template(Body) $ValueList {sec min hour day month year2 year4 user group message}]
        }
        if {!$Count} {OutputData $template(None)}
        OutputData $template(Footer)
    } else {
        iputs ".-\[OneLines\]-------------------------------------------------------------."
        set TimeStamp [clock seconds]
        OneDb eval {INSERT INTO OneLines (TimeStamp,UserName,GroupName,Message) VALUES($TimeStamp,$user,$group,$Message)}
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
            if {[clock format $TimeNow -format "%d"] == "01"} {set DoRotate 1}
        }
        {week} - {weekly} {
            set DateFormat "%Y-Week%W"
            if {[clock format $TimeNow -format "%w"] == "0"} {set DoRotate 1}
        }
        {day} - {daily} {set DateFormat "%Y-%m-%d"; set DoRotate 1}
        default {ErrorLog RotateLogs "invalid log rotation frequency: must be montly, weekly, or daily"}
    }
    if {!$DoRotate} {return 0}

    set MinimumSize [expr {$log(MinimumSize) * 1024 * 1024}]
    foreach LogFile $log(RotateList) {
        ## Archive log file if it exists and meets the size requirement.
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
    if {![catch {set Handle [open $LogFile r]} ErrorMsg]} {
        while {![eof $Handle]} {
            if {[gets $Handle LogLine] > 0 && [string match -nocase $Pattern $LogLine]} {
                set LogData [linsert $LogData 0 [string trim $LogLine]]
            }
        }
        close $Handle
    } else {ErrorLog SearchLog $ErrorMsg}

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
    if {![catch {set Handle [open $weekly(ConfigFile) r]} ErrorMsg]} {
        while {![eof $Handle]} {
            set FileLine [string trim [gets $Handle]]
            if {[string index $FileLine 0] == "#"} {
                append CfgComments $FileLine "\n"
            } elseif {[llength [set FileLine [split $FileLine "|"]]] == 3} {
                foreach {Target Section Credits} $FileLine {break}
                lappend TargetList [list $Target $Section $Credits]
            }
        }
        close $Handle
    } else {
        ErrorLog WeeklyRead $ErrorMsg
        ErrorReturn "Unable to load the weekly credits configuration, contact a siteop."
    }

    ## Display weekly credit targets.
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
        ## Add or remove weekly credit targets.
        set Deleted 0; set Index 0
        set CreditsKB [expr {wide($Credits) * 1024}]
        foreach ListItem $TargetList {
            if {[string equal $ListItem [list $WkTarget $Section $CreditsKB]]} {
                set Deleted 1
                set TargetList [lreplace $TargetList $Index $Index]
            } else {incr Index}
        }
        if {$Deleted} {
            LinePuts "Removed the target \"$WkTarget\" (${Credits}MB in section $Section) from weekly credits."
        } else {
            ## Check if the user or group exists.
            if {[string index $WkTarget 0] == "="} {
                if {[resolve group [string range $WkTarget 1 end]] == -1} {
                    ErrorReturn "The specified group does not exist."
                }
            } elseif {[resolve user $WkTarget] == -1} {
                ErrorReturn "The specified user does not exist."
            }
            LinePuts "Added the target \"$WkTarget\" (${Credits}MB in section $Section) to weekly credits."
            lappend TargetList [list $WkTarget $Section $CreditsKB]
        }

        ## Rewrite weekly configuration file.
        if {![catch {set Handle [open $weekly(ConfigFile) w]} ErrorMsg]} {
            puts -nonewline $Handle $CfgComments
            foreach ListItem [lsort -ascii -index 0 $TargetList] {
                puts $Handle [join $ListItem "|"]
            }
            close $Handle
        } else {ErrorLog WeeklyWrite $ErrorMsg}
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
    if {![catch {set Handle [open $weekly(ConfigFile) r]} ErrorMsg]} {
        while {![eof $Handle]} {
            set FileLine [string trim [gets $Handle]]
            if {[string index $FileLine 0] != "#" && [llength [set FileLine [split $FileLine "|"]]] == 3} {
                foreach {Target Section Credits} $FileLine {break}
                lappend TargetList [list $Target $Section $Credits]
            }
        }
        close $Handle
    } else {
        ErrorLog WeeklyRead $ErrorMsg
        ErrorReturn "Unable to load the weekly credits configuration, contact a siteop."
    }
    if {[llength $TargetList]} {
        foreach ListItem [lsort -ascii -index 0 $TargetList] {
            foreach {Target Section Credits} $ListItem {break}
            if {[string index $Target 0] == "="} {
                set Target [string range $Target 1 end]
                if {[set GroupId [resolve group $Target]] == -1} {
                    LinePuts "Skipping invalid group \"$Target\"."; continue
                }
                set UserList [GetGroupUsers $GroupId]
                set UserCount [llength $UserList]
                ## Split credits evenly amongst its group members.
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

proc ::nxTools::Utils::SiteCredits {Type Target Amount Section} {
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

    if {[string equal -nocase $Type "give"]} {
        ChangeCredits $Target "+$AmountKB" $Section
        LinePuts "Gave $Amount$UnitName of credits to $Target in section $Section."
        putlog "GIVE: \"$user\" \"$group\" \"$AmountMB\" \"$Target\""
    } elseif {[string equal -nocase $Type "take"]} {
        ChangeCredits $Target "-$AmountKB" $Section
        LinePuts "Took $Amount$UnitName of credits from $Target in section $Section."
        putlog "TAKE: \"$user\" \"$group\" \"$AmountMB\" \"$Target\""
    }
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

        ## We're only interested in fixed volumes and network volumes.
        switch -- [::nx::volume type $VolName] {
            3 {set TypeName "Fixed"}
            4 {set TypeName "Network"}
            default {continue}
        }
        if {[catch {::nx::volume info $VolName volume} ErrorMsg]} {
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

    ## Validate section number.
    if {![string is digit -strict $Section] || $Section > 9} {set Section 1} else {incr Section}
    set LeechCount 0
    set UserList [GetGroupUsers $GroupId]
    set file(alldn) 0; set size(alldn) 0; set time(alldn) 0
    set file(allup) 0; set size(allup) 0; set time(allup) 0

    foreach UserName $UserList {
        array set uinfo [list alldn 0 allup 0 AdminGroups "" Flags "" Prefix "" Ratio 0]
        if {[userfile open $UserName] == 0} {
            set UserFile [userfile bin2ascii]
            foreach UserLine [split $UserFile "\r\n"] {
                set LineType [string tolower [lindex $UserLine 0]]
                switch -- $LineType {
                    {admingroups} {set uinfo(AdminGroups) [lrange $UserLine 1 end]}
                    {alldn} - {allup} {
                        MergeStats [lrange $UserLine 1 end] file($LineType) uinfo($LineType) time($LineType)
                        set size($LineType) [expr {wide($uinfo($LineType)) + wide($size($LineType))}]
                    }
                    {flags} {set uinfo(Flags) [lindex $UserLine 1]}
                    {ratio} {set uinfo(Ratio) [lindex $UserLine $Section]}
                }
            }
        }
        if {$uinfo(Ratio) != 0} {
            set uinfo(Ratio) "1:$uinfo(Ratio)"
        } else {
            set uinfo(Ratio) "Unlimited"; incr LeechCount
        }

        ## Siteop and group admin prefix.
        if {[regexp "\[$misc(SiteopFlags)\]" $uinfo(Flags)]} {
            set uinfo(Prefix) "*"
        } elseif {[lsearch -exact $uinfo(AdminGroups) $GroupId] != -1} {
            set uinfo(Prefix) "+"
        }
        iputs [format "| %-18s | %10s | %10s | %-10s | %-10s |" $uinfo(Prefix)$UserName [FormatSize $uinfo(allup)] [FormatSize $uinfo(alldn)] $uinfo(Ratio) $uinfo(Flags)]
    }

    ## Find the group's description and slot count.
    array set ginfo [list Slots "0 0" TagLine "No TagLine Set"]
    if {[groupfile open $GroupName] == 0} {
        set GroupFile [groupfile bin2ascii]
        foreach GroupLine [split $GroupFile "\r\n"] {
            set LineType [string tolower [lindex $GroupLine 0]]
            if {[string equal "description" $LineType]} {
                set ginfo(TagLine) [ArgRange $GroupLine 1 end]
            } elseif {[string equal "slots" $LineType]} {
                set ginfo(Slots) [lrange $GroupLine 1 2]
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
        foreach UserName [GetUserList] {ResetUserFile $UserName $ResetStats}
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
        ResetUserFile $UserName {alldn allup daydn dayup monthdn monthup wkdn wkup} $reset(Credits)
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
    } elseif {[string index $Target 0] == "="} {
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
        set UserFile [userfile bin2ascii]
        foreach UserLine [split $UserFile "\r\n"] {
            set LineType [string tolower [lindex $UserLine 0]]
            if {[lsearch -exact {alldn allup daydn dayup monthdn monthup wkdn wkup} $LineType] != -1} {
                MergeStats [lrange $UserLine 1 end] file($LineType) size($LineType) time($LineType)
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
    set IsAdmin [regexp "\[$misc(SiteopFlags)\]" $flags]
    iputs ".------------------------------------------------------------------------."
    iputs "|    User    |   Group    |  Info          |  Action                     |"
    iputs "|------------------------------------------------------------------------|"
    if {[client who init "CID" "UID" "STATUS" "TIMEIDLE" "TRANSFERSPEED" "VIRTUALPATH" "VIRTUALDATAPATH"] == 0} {
        while {[set WhoData [client who fetch]] != ""} {
            foreach {ClientId UserId Status IdleTime Speed VirtualPath DataPath} $WhoData {break}
            set IsMe [expr {$cid == $ClientId ? "*" : ""}]
            set UserName [resolve uid $UserId]
            set GroupName "NoGroup"; set TagLine "No Tagline Set"
            set FileName [file tail $DataPath]

            ## Find the user's group and tagline.
            if {[userfile open $UserName] == 0} {
                set UserFile [userfile bin2ascii]
                foreach UserLine [split $UserFile "\r\n"] {
                    set LineType [string tolower [lindex $UserLine 0]]
                    if {[string equal "groups" $LineType]} {
                        set GroupName [GetGroupName [lindex $UserLine 1]]
                    } elseif {[string equal "tagline" $LineType]} {
                        set TagLine [ArgRange $UserLine 1 end]
                    }
                }
            }

            ## Show hidden users to either admins or the user.
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
    set IsSiteBot [expr {[info exists user] && [string equal $misc(SiteBot) $user]}]

    set ArgLength [llength [set ArgList [ArgList $ArgV]]]
    set Event [string tolower [lindex $ArgList 0]]
    set Result 0
    switch -- $Event {
        {daystats} {
            if {![IsTrue $misc(dZSbotLogging)]} {
                putlog "DAYSTATS: \"Launch Daystats\""
            }
        }
        {drives} {
            set Result [SiteDrives]
        }
        {errlog} {
            if {$ArgLength > 1 && [GetOptions [lrange $ArgList 1 end] MaxResults Pattern]} {
                iputs ".-\[ErrorLog\]-------------------------------------------------------------."
                set Result [SearchLog $log(Error) $MaxResults $Pattern]
            } else {
                iputs "Syntax: SITE ERRLOG \[-max <limit>\] <pattern>"
            }
        }
        {ginfo} {
            if {$ArgLength > 1 && [string is digit [lindex $ArgList 2]]} {
                set Result [SiteGroupInfo [lindex $ArgList 1] [lindex $ArgList 2]]
            } else {
                iputs "Syntax: SITE GINFO <group> \[credit section\]"
            }
            set Result 1
        }
        {give} {
            foreach {Target Amount Section} [lrange $ArgList 1 end] {break}
            if {$ArgLength > 2} {
                set Result [SiteCredits give $Target $Amount $Section]
            } else {
                iputs "Syntax: SITE GIVE <username> <credits> \[credit section\]"
            }
        }
        {newdate} {
            set Result [NewDate [lindex $ArgList 1]]
        }
        {onelines} {
            set Result [OneLines [join [lrange $ArgList 1 end]]]
        }
        {resetstats} {
            if {$ArgLength > 1} {
                set Result [SiteResetStats [join [lrange $ArgList 1 end]]]
            } else {
                iputs "Syntax: SITE RESETSTATS <stats type(s)>"
            }
        }
        {resetuser} {
            if {$ArgLength > 1} {
                set Result [SiteResetUser [lindex $ArgList 1]]
            } else {
                iputs "Syntax: SITE RESETUSER <username>"
            }
        }
        {rotate} {
            set Result [RotateLogs]
        }
        {size} {
            if {$ArgLength > 1} {
                set VirtualPath [GetPath $pwd [join [lrange $ArgList 1 end]]]
                set Result [SiteSize $VirtualPath]
            } else {
                iputs " Usage: SITE SIZE <file/directory>"
            }
        }
        {syslog} {
            if {$ArgLength > 1 && [GetOptions [lrange $ArgList 1 end] MaxResults Pattern]} {
                iputs ".-\[SysopLog\]-------------------------------------------------------------."
                set Result [SearchLog $log(SysOp) $MaxResults $Pattern]
            } else {
                iputs "Syntax: SITE SYSLOG \[-max <limit>\] <pattern>"
            }
        }
        {take} {
            foreach {Target Amount Section} [lrange $ArgList 1 end] {break}
            if {$ArgLength > 2} {
                set Result [SiteCredits take $Target $Amount $Section]
            } else {
                iputs "Syntax: SITE TAKE <username> <credits> \[credit section\]"
            }
        }
        {traffic} {
            set Result [SiteTraffic [lindex $ArgList 1]]
        }
        {weekly} {
            set Result [WeeklyCredits [lindex $ArgList 1] [lindex $ArgList 2]]
        }
        {weeklyset} {
            set Result [WeeklySet]
        }
        {who} {
            set Result [SiteWho]
        }
        default {
            ErrorLog InvalidArgs "invalid parameter \"[info script] $Event\": check your ioFTPD.ini for errors"
        }
    }
    return [set ioerror $Result]
}

::nxTools::Utils::Main [expr {[info exists args] ? $args : ""}]
