#
# nxTools - Dupe Checker, nuker, pre, request and utilities.
# Copyright (c) 2004-2005 neoxed
#
# Module Name:
#   Utilities
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements miscellaneous utilities.
#

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Utils {
    namespace import -force ::nxLib::*
}

# User Procedures
######################################################################

proc ::nxTools::Utils::ChangeCredits {userName change {section 0}} {
    incr section
    if {[regexp {^(\+|\-)?(\d+)$} $change result method amount] && [userfile open $userName] == 0} {
        set newUserFile ""
        userfile lock
        set userFile [userfile bin2ascii]

        foreach line [split $userFile "\r\n"] {
            if {[string equal -nocase "credits" [lindex $line 0]]} {
                if {![string length $method]} {
                    set value $amount
                } else {
                    set value [expr wide([lindex $line $section]) $method wide($amount)]
                }
                set line [lreplace $line $section $section $value]
            }
            append newUserFile $line "\r\n"
        }
        userfile ascii2bin $newUserFile
        userfile unlock
        return 1
    }
    return 0
}

proc ::nxTools::Utils::CheckHidden {userName groupName virtualPath} {
    global hide
    if {[lsearch -exact $hide(UserNames) $userName] != -1 || \
        [lsearch -exact $hide(GroupNames) $groupName] != -1 || \
        [ListMatch $hide(Paths) $virtualPath]} {return 1}
    return 0
}

proc ::nxTools::Utils::IsGroupAdmin {userName flags groupId} {
    global misc
    if {![MatchFlags $misc(GAdminFlags) $flags]} {
        return 1
    } elseif {[userfile open $userName] == 0} {
        set userFile [userfile bin2ascii]
        if {[regexp -nocase {admingroups ([\s\d]+)} $userFile result groupIdList]} {
            if {[lsearch -exact $groupIdList $groupId] != -1} {return 1}
        }
    }
    return 0
}

proc ::nxTools::Utils::ResetUserFile {userName statTypes {resetCredits "False"}} {
    set newUserFile ""
    if {[userfile open $userName] == 0} {
        userfile lock
        if {[IsTrue $resetCredits]} {
            append newUserFile "credits" [string repeat " 0" 10] "\r\n"
        }
        foreach statsField $statTypes {
            append newUserFile $statsField [string repeat " 0" 30] "\r\n"
        }
        userfile ascii2bin $newUserFile
        userfile unlock
    }
}

# Tools Procedures
######################################################################

proc ::nxTools::Utils::NewDate {findArea} {
    global misc newdate
    iputs ".-\[NewDate\]--------------------------------------------------------------."

    set dateArea "Default"
    foreach areaName [array names newdate] {
        if {[string equal -nocase $areaName $findArea]} {
            set dateArea $areaName; break
        }
    }
    if {![info exists newdate($dateArea)]} {
        ErrorLog NewDateArea "unknown newdate area \"$dateArea\""
        ErrorReturn "The newdate area \"$dateArea\" is not defined."
    }
    LinePuts "Creating [llength $newdate($dateArea)] date directories for the \"$dateArea\" area."
    set timeNow [clock seconds]

    foreach newArea $newdate($dateArea) {
        if {[llength $newArea] != 10} {
            ErrorLog NewDate "wrong number of options in line: \"$newArea\""
            continue
        }
        foreach {areaName description virtualPath realPath symLink doLog dayOffset userId groupId chmod} $newArea {break}
        LinePuts ""; LinePuts "$areaName ($description):"

        # Format cookies and directory paths.
        set areaTime [expr {$timeNow + ($dayOffset * 86400)}]
        set virtualPath [clock format $areaTime -format $virtualPath -gmt [IsTrue $misc(UtcTime)]]
        set realPath [clock format $areaTime -format $realPath -gmt [IsTrue $misc(UtcTime)]]

        if {[file isdirectory $realPath] || ![catch {file mkdir $realPath} error]} {
            LinePuts "Created directory: $realPath"
            catch {vfs write $realPath $userId $groupId $chmod}

            if {[string length $symLink]} {
                if {[file isdirectory $symLink] || ![catch {file mkdir $symLink} error]} {
                    LinePuts "Created symlink: $symLink"
                    catch {vfs chattr $symLink 1 $virtualPath}
                    LinePuts "Linked to vpath: $virtualPath"
                } else {
                    LinePuts "Unable to create symlink: $symLink"
                    ErrorLog NewDateLink $error
                }
            }
            if {[IsTrue $doLog]} {putlog "NEWDATE: \"$virtualPath\" \"$areaName\" \"$description\""}
        } else {
            LinePuts "Unable to create directory: $realPath"
            ErrorLog NewDateDir $error
        }
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::OneLines {message} {
    global misc group user
    if {[catch {DbOpenFile [namespace current]::OneDb "OneLines.db"} error]} {
        ErrorLog OneLinesDb $error
        return 1
    }
    if {![string length $message]} {
        foreach fileExt {Header Body None Footer} {
            set template($fileExt) [ReadFile [file join $misc(Templates) "OneLines.$fileExt"]]
        }
        OutputText $template(Header)
        set count 0
        OneDb eval {SELECT * FROM OneLines ORDER BY TimeStamp DESC LIMIT $misc(OneLines)} values {
            incr count
            set valueList [clock format $values(TimeStamp) -format {{%S} {%M} {%H} {%d} {%m} {%y} {%Y}} -gmt [IsTrue $misc(UtcTime)]]
            lappend valueList $values(UserName) $values(GroupName) $values(Message)
            OutputText [ParseCookies $template(Body) $valueList {sec min hour day month year2 year4 user group message}]
        }
        if {!$count} {OutputText $template(None)}
        OutputText $template(Footer)
    } else {
        iputs ".-\[OneLines\]-------------------------------------------------------------."
        set timeStamp [clock seconds]
        OneDb eval {INSERT INTO OneLines(TimeStamp,UserName,GroupName,Message)
            VALUES($timeStamp,$user,$group,$message)}
        LinePuts "Added message \"$message\" by $user/$group."
        iputs "'------------------------------------------------------------------------'"
    }
    OneDb close
}

proc ::nxTools::Utils::RotateLogs {} {
    global log
    iputs ".-\[RotateLogs\]-----------------------------------------------------------."
    set doRotate 0
    set timeNow [clock seconds]
    switch -- [string tolower $log(Frequency)] {
        {month} - {monthly} {
            set dateFormat "%Y-%m"
            if {[clock format $timeNow -format "%d"] eq "01"} {set doRotate 1}
        }
        {week} - {weekly} {
            set dateFormat "%Y-Week%W"
            if {[clock format $timeNow -format "%w"] eq "0"} {set doRotate 1}
        }
        {day} - {daily} {
            set dateFormat "%Y-%m-%d"
            set doRotate 1
        }
        default {ErrorLog RotateLogs "invalid log rotation frequency: must be montly, weekly, or daily"}
    }
    if {!$doRotate} {return 0}

    set minimumSize [expr {$log(MinimumSize) * 1024 * 1024}]
    foreach logFile $log(RotateList) {
        # Archive log file if it exists and meets the size requirement.
        if {![file isfile $logFile]} {
            LinePuts "Skipping log \"[file tail $logFile]\" - the file does not exist."
        } elseif {[file size $logFile] < $minimumSize} {
            LinePuts "Skipping log \"[file tail $logFile]\" - the file is not larger then $log(MinimumSize)MB."
        } else {
            LinePuts "Rotated log \"[file tail $logFile]\" successfully."
            if {[ArchiveFile $logFile $dateFormat]} {
                catch {close [open $logFile a]}
            } else {ErrorLog RotateLogs "unable to archive log file \"$logFile\""}
        }
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SearchLog {logFile limit pattern} {
    set data [list]
    if {![catch {set handle [open $logFile r]} error]} {
        while {![eof $handle]} {
            if {[gets $handle line] > 0 && [string match -nocase $pattern $line]} {
                set data [linsert $data 0 [string trim $line]]
            }
        }
        close $handle
    } else {ErrorLog SearchLog $error}

    set count 0
    foreach line $data {
        if {[incr count] > $limit} {break}
        iputs " $line"
    }
    if {!$count} {LinePuts "No results found."}

    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::WeeklyCredits {wkTarget wkAmount} {
    global weekly
    iputs ".-\[WeeklyCredits\]--------------------------------------------------------."
    set comments ""
    set targetList [list]

    if {![catch {set handle [open $weekly(ConfigFile) r]} error]} {
        while {![eof $handle]} {
            set line [string trim [gets $handle]]
            if {[string index $line 0] eq "#"} {
                append comments $line "\n"
            } elseif {[llength [set line [split $line "|"]]] == 3} {
                foreach {target section credits} $line {break}
                lappend targetList [list $target $section $credits]
            }
        }
        close $handle
    } else {
        ErrorLog WeeklyRead $error
        ErrorReturn "Unable to load the weekly credits configuration, contact a siteop."
    }

    # Display weekly credit targets.
    if {![string length $wkTarget]} {
        if {[llength $targetList]} {
            iputs "|     Target     |   Section   |  Credit Amount                          |"
            iputs "|------------------------------------------------------------------------|"
            foreach element [lsort -ascii -index 0 $targetList] {
                foreach {target section credits} $element {break}
                iputs [format "| %-14s | %11d | %-39s |" $target $section "[expr {wide($credits) / 1024}]MB"]
            }
        } else {
            LinePuts "There are currently no weekly credit targets."
        }
    } elseif {[regexp {^(\d),((\+|\-)?\d+)$} $wkAmount result section credits]} {
        # Add or remove weekly credit targets.
        set deleted 0; set index 0
        set creditsKB [expr {wide($credits) * 1024}]
        foreach element $targetList {
            if {$element eq [list $wkTarget $section $creditsKB]} {
                set deleted 1
                set targetList [lreplace $targetList $index $index]
            } else {incr index}
        }
        if {$deleted} {
            LinePuts "Removed \"$wkTarget\" (${credits}MB in section $section) from weekly credits."
        } else {
            # Check if the user or group exists.
            if {[string index $wkTarget 0] eq "="} {
                if {[resolve group [string range $wkTarget 1 end]] == -1} {
                    ErrorReturn "The specified group does not exist."
                }
            } elseif {[resolve user $wkTarget] == -1} {
                ErrorReturn "The specified user does not exist."
            }
            LinePuts "Added \"$wkTarget\" (${credits}MB in section $section) to weekly credits."
            lappend targetList [list $wkTarget $section $creditsKB]
        }

        # Rewrite weekly configuration file.
        if {![catch {set handle [open $weekly(ConfigFile) w]} error]} {
            puts -nonewline $handle $comments
            foreach element [lsort -ascii -index 0 $targetList] {
                puts $handle [join $element "|"]
            }
            close $handle
        } else {ErrorLog WeeklyWrite $error}
    } else {
        LinePuts "Syntax:"
        LinePuts "  User Credits - SITE WEEKLY <username> <section>,<credits mb>"
        LinePuts " Group Credits - SITE WEEKLY =<group> <section>,<credits mb>"
        LinePuts "  List Targets - SITE WEEKLY"
        LinePuts "Notes:"
        LinePuts " - To add(+) or subtract(-) credits, use the appropriate sign."
        LinePuts " - To remove a user or group, enter the target's section and credits."
    }
    iputs "'------------------------------------------------------------------------'"
}

proc ::nxTools::Utils::WeeklySet {} {
    global weekly
    iputs ".-\[WeeklySet\]------------------------------------------------------------."
    set targetList [list]

    if {![catch {set handle [open $weekly(ConfigFile) r]} error]} {
        while {![eof $handle]} {
            set line [string trim [gets $handle]]
            if {[string index $line 0] ne "#" && [llength [set line [split $line "|"]]] == 3} {
                foreach {target section credits} $line {break}
                lappend targetList [list $target $section $credits]
            }
        }
        close $handle
    } else {
        ErrorLog WeeklyRead $error
        ErrorReturn "Unable to load the weekly credits configuration, contact a siteop."
    }

    if {[llength $targetList]} {
        foreach element [lsort -ascii -index 0 $targetList] {
            foreach {target section credits} $element {break}
            if {[string index $target 0] eq "="} {
                set target [string range $target 1 end]
                if {[set groupId [resolve group $target]] == -1} {
                    LinePuts "Skipping invalid group \"$target\"."; continue
                }
                set userList [GetGroupUsers $groupId]
                set userCount [llength $userList]

                # Split credits evenly amongst its group members.
                if {$userCount > 1 && [IsTrue $weekly(SplitGroup)]} {
                    if {![regexp {^(\+|\-)?(\d+)$} $credits result method amount]} {continue}
                    set credits $method
                    append credits [expr {wide($amount) / $userCount}]
                }

                LinePuts "Weekly credits given to $userCount user(s) in $target ([expr {wide($credits) / 1024}]MB in section $section each)."
                foreach userName $userList {
                    ChangeCredits $userName $credits $section
                }
            } elseif {[resolve user $target] == -1} {
                LinePuts "Skipping invalid user \"$target\"."
            } else {
                LinePuts "Weekly credits given to $target ([expr {wide($credits) / 1024}]MB in section $section)."
                ChangeCredits $target $credits $section
            }
        }
    } else {LinePuts "There are currently no weekly credit targets."}
    iputs "'------------------------------------------------------------------------'"
}

# Site Commands
######################################################################

proc ::nxTools::Utils::SiteCredits {event target amount section} {
    global misc group user
    iputs ".-\[Credits\]--------------------------------------------------------------."
    if {[resolve user $target] == -1} {
        ErrorReturn "The specified user does not exist."
    }
    if {![regexp {^(\d+)(.*)$} $amount result amount unit]} {
        ErrorReturn "The specified amount \"$amount\" is invalid."
    }
    set unitName [string toupper [string index $unit 0]]
    switch -- $unitName {
        {G} {set multi 1048576}
        {M} {set multi 1024}
        {K} {set multi 1}
        {}  {set multi 1024; set unitName "M"}
        default {ErrorReturn "The specified size unit \"$unit\" is invalid."}
    }
    append unitName "B"

    if {![string is digit -strict $section] || $section > 9} {set section 0}
    set amountKB [expr {wide($amount) * $multi}]
    set amountMB [expr {wide($amountKB) / 1024}]

    if {$event eq "GIVE"} {
        ChangeCredits $target "+$amountKB" $section
        LinePuts "Gave $amount$unitName of credits to $target in section $section."
    } elseif {$event eq "TAKE"} {
        ChangeCredits $target "-$amountKB" $section
        LinePuts "Took $amount$unitName of credits from $target in section $section."
    } else {
        ErrorLog SiteCredits "unknown event \"$event\""
    }

    if {[IsTrue $misc(dZSbotLogging)]} {
        set amountKB $amountMB
    }
    putlog "${event}: \"$user\" \"$group\" \"$amountKB\" \"$target\""

    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteDrives {} {
    iputs ".-\[Drives\]---------------------------------------------------------------."
    iputs "|  Volume Type  | File System |  Volume Name  | Free Space | Total Space |"
    iputs "|------------------------------------------------------------------------|"
    set free 0; set total 0
    foreach volName [file volumes] {
        set volName [string map {/ \\} $volName]

        # We're only interested in fixed volumes and network volumes.
        switch -- [::nx::volume type $volName] {
            3 {set type "Fixed"}
            4 {set type "Network"}
            default {continue}
        }
        if {[catch {::nx::volume info $volName volume} error]} {
            LinePuts "$volName Unable to retrieve volume information."
        } else {
            set free [expr {wide($free) + $volume(free)}]
            set total [expr {wide($total) + $volume(total)}]

            set volume(free) [FormatSize [expr {$volume(free) / 1024}]]
            set volume(total) [FormatSize [expr {$volume(total) / 1024}]]
            iputs [format "| %-3s %9s | %-11s | %-13s | %10s | %11s |" $volName $type $volume(fs) $volume(name) $volume(free) $volume(total)]
        }
    }
    iputs "|------------------------------------------------------------------------|"
    set free [FormatSize [expr {wide($free) / 1024}]]
    set total [FormatSize [expr {wide($total) / 1024}]]
    iputs [format "|                                       Total | %10s | %11s |" $free $total]
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteGroupInfo {groupName section} {
    global misc flags user
    iputs ".-\[GroupInfo\]------------------------------------------------------------."
    if {[set groupId [resolve group $groupName]] == -1} {
        ErrorReturn "The specified group does not exist."
        return 1
    } elseif {![IsGroupAdmin $user $flags $groupId]} {
        ErrorReturn "You do not have admin rights for this group."
    }
    iputs "|  Username          |   All Up   |   All Dn   |   Ratio    |   Flags    |"
    iputs "|------------------------------------------------------------------------|"

    # Validate section number.
    if {![string is digit -strict $section] || $section > 9} {set section 1} else {incr section}
    set leechCount 0
    set userList [GetGroupUsers $groupId]
    set file(alldn) 0; set size(alldn) 0; set time(alldn) 0
    set file(allup) 0; set size(allup) 0; set time(allup) 0

    foreach userName $userList {
        array set uinfo [list alldn 0 allup 0 AdminGroups "" Flags "" Prefix "" Ratio 0]
        if {[userfile open $userName] == 0} {
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
                    {ratio} {set uinfo(Ratio) [lindex $line $section]}
                }
            }
        }
        if {$uinfo(Ratio) != 0} {
            set uinfo(Ratio) "1:$uinfo(Ratio)"
        } else {
            incr leechCount
            set uinfo(Ratio) "Unlimited"
        }

        # Siteop and group admin prefix.
        if {[MatchFlags $misc(SiteopFlags) $uinfo(Flags)]} {
            set uinfo(Prefix) "*"
        } elseif {[lsearch -exact $uinfo(AdminGroups) $groupId] != -1} {
            set uinfo(Prefix) "+"
        }
        iputs [format "| %-18s | %10s | %10s | %-10s | %-10s |" $uinfo(Prefix)$userName [FormatSize $uinfo(allup)] [FormatSize $uinfo(alldn)] $uinfo(Ratio) $uinfo(Flags)]
    }

    # Find the group's description and slot count.
    array set ginfo [list Slots "0 0" TagLine "No TagLine Set"]
    if {[groupfile open $groupName] == 0} {
        set groupFile [groupfile bin2ascii]

        foreach line [split $groupFile "\r\n"] {
            set type [string tolower [lindex $line 0]]
            if {$type eq "description"} {
                set ginfo(TagLine) [StringRange $line 1 end]
            } elseif {$type eq "slots"} {
                set ginfo(Slots) [lrange $line 1 2]
            }
        }
    }

    iputs "|------------------------------------------------------------------------|"
    iputs [format "| * Denotes SiteOp    + Denotes GAdmin    %30.30s |" $ginfo(TagLine)]
    iputs [format "| User Slots: %-5d   Leech Slots: %-37d |" [lindex $ginfo(Slots) 0] [lindex $ginfo(Slots) 1]]
    iputs "|------------------------------------------------------------------------|"
    iputs [format "| Total All Up: %10s  Total Files Up: %-10ld  Group Users: %-3d |" [FormatSize $size(allup)] $file(allup) [llength $userList]]
    iputs [format "| Total All Dn: %10s  Total Files Dn: %-10ld   With Leech: %-3d |" [FormatSize $size(alldn)] $file(alldn) $leechCount]
    iputs "'------------------------------------------------------------------------'"
    return 1
}

proc ::nxTools::Utils::SiteResetStats {argList} {
    set resetStats ""
    set statTypes {alldn allup daydn dayup monthdn monthup wkdn wkup}
    iputs ".-\[ResetStats\]-----------------------------------------------------------."

    foreach arg $argList {
        set arg [string tolower $arg]
        switch -- $arg {
            {-all}  {set resetStats $statTypes; break}
            {all}   {lappend resetStats "alldn" "allup"}
            {month} {lappend resetStats "monthdn" "monthup"}
            {wk} -
            {week}  {lappend resetStats "wkdn" "wkup"}
            {day}   {lappend resetStats "daydn" "dayup"}
            default {
                if {[lsearch -exact $statTypes $arg]} {lappend resetStats $arg}
            }
        }
    }

    if {![llength $resetStats]} {
        LinePuts "No valid stats Types specified."
        LinePuts "Types: $statTypes"
    } else {
        LinePuts "Resetting: [JoinLiteral $resetStats]"
        foreach userName [GetUserList] {
            ResetUserFile $userName $resetStats
        }
    }

    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteResetUser {userName} {
    global reset
    iputs ".-\[ResetUser\]------------------------------------------------------------."

    if {[resolve user $userName] == -1} {
        LinePuts "The specified user does not exist."
    } else {
        ResetUserFile $userName {alldn allup daydn dayup monthdn monthup wkdn wkup} $reset(Credits)
        if {[IsTrue $reset(Credits)]} {
            LinePuts "Credits Reset...Complete."
        }
        LinePuts "Stats Reset.....Complete."
    }

    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteSize {virtualPath} {
    global size
    iputs ".-\[Size\]-----------------------------------------------------------------."

    set realPath [resolve pwd $virtualPath]
    if {![file exists $realPath]} {
        ErrorReturn "The specified file or directory does not exist."
    }
    GetDirStats $realPath stats ".ioFTPD*"
    set stats(TotalSize) [expr {wide($stats(TotalSize)) / 1024}]
    LinePuts "$stats(FileCount) File(s), $stats(DirCount) Directory(s), [FormatSize $stats(TotalSize)]"

    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteTraffic {target} {
    iputs ".-\[Traffic\]--------------------------------------------------------------."

    if {![string length $target]} {
        set traffic 0
        set userList [GetUserList]
    } elseif {[string index $target 0] eq "="} {
        set target [string range $target 1 end]
        if {[resolve group $target] == -1} {
            ErrorReturn "The specified group does not exist."
        }
        set traffic 1
        set userList [GetGroupUsers [resolve group $target]]
    } else {
        if {[resolve user $target] == -1} {
            ErrorReturn "The specified user does not exist."
        }
        set traffic 2
        set userList $target
    }

    array set file [list alldn 0 allup 0 daydn 0 dayup 0 monthdn 0 monthup 0 wkdn 0 wkup 0]
    array set size [list alldn 0 allup 0 daydn 0 dayup 0 monthdn 0 monthup 0 wkdn 0 wkup 0]
    array set time [list alldn 0 allup 0 daydn 0 dayup 0 monthdn 0 monthup 0 wkdn 0 wkup 0]

    foreach userName $userList {
        if {[userfile open $userName] != 0} {continue}
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
    switch -- $traffic {
        0 {LinePuts "Stats for all [llength $userList] user(s)."}
        1 {LinePuts "Stats for the group $target."}
        2 {LinePuts "Stats for the user $target."}
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Utils::SiteWho {} {
    global misc cid flags
    iputs ".------------------------------------------------------------------------."
    iputs "|    User    |   Group    |  Info          |  Action                     |"
    iputs "|------------------------------------------------------------------------|"
    array set who [list BwDn 0.0 BwUp 0.0 UsersDn 0 UsersUp 0 UsersIdle 0]
    set isAdmin [MatchFlags $misc(SiteopFlags) $flags]

    if {[client who init "CID" "UID" "STATUS" "TIMEIDLE" "TRANSFERSPEED" "VIRTUALPATH" "VIRTUALDATAPATH"] == 0} {
        while {[set whoData [client who fetch]] ne ""} {
            foreach {clientId userId status idleTime speed virtualPath dataPath} $whoData {break}
            set isMe [expr {$cid == $clientId ? "*" : ""}]
            set userName [resolve uid $userId]
            set groupName "NoGroup"; set tagLine "No Tagline Set"
            set fileName [file tail $dataPath]

            # Find the user's group and tagline.
            if {[userfile open $userName] == 0} {
                set userFile [userfile bin2ascii]

                foreach line [split $userFile "\r\n"] {
                    set type [string tolower [lindex $line 0]]
                    if {$type eq "groups"} {
                        set groupName [GetGroupName [lindex $line 1]]
                    } elseif {$type eq "tagline"} {
                        set tagLine [StringRange $line 1 end]
                    }
                }
            }

            # Show hidden users to either admins or the user.
            if {$isAdmin || $cid == $clientId || ![CheckHidden $userName $groupName $virtualPath]} {
                switch -- $status {
                    0 - 3 {
                        set action "IDLE: [FormatDuration $idleTime]"
                        incr who(UsersIdle)
                    }
                    1 {
                        set action [format "DL: %-12.12s - %.0fKB/s" $fileName $speed]
                        set who(BwDn) [expr {double($who(BwDn)) + double($speed)}]
                        incr who(UsersDn)
                    }
                    2 {
                        set action [format "UL: %-12.12s - %.0fKB/s" $fileName $speed]
                        set who(BwUp) [expr {double($who(BwUp)) + double($speed)}]
                        incr who(UsersUp)
                    }
                    default {continue}
                }
                iputs [format "| %-10.10s | %-10.10s | %-14.14s | %-27s |" "$isMe$userName" $groupName $tagLine $action]
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

proc ::nxTools::Utils::Main {argv} {
    global isBot log misc group ioerror pwd user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set isBot [expr {[info exists user] && $misc(SiteBot) eq $user}]
    set result 0

    set argLength [llength [set argList [ArgList $argv]]]
    set event [string toupper [lindex $argList 0]]
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
            if {$argLength > 1 && [GetOptions [lrange $argList 1 end] limit pattern]} {
                iputs ".-\[ErrorLog\]-------------------------------------------------------------."
                set result [SearchLog $log(Error) $limit $pattern]
            } else {
                iputs "Syntax: SITE ERRLOG \[-max <limit>\] <pattern>"
            }
        }
        {GINFO} {
            if {$argLength > 1 && [string is digit [lindex $argList 2]]} {
                set result [SiteGroupInfo [lindex $argList 1] [lindex $argList 2]]
            } else {
                iputs "Syntax: SITE GINFO <group> \[credit section\]"
            }
            set result 1
        }
        {GIVE} - {TAKE} {
            foreach {target amount section} [lrange $argList 1 end] {break}
            if {$argLength > 2} {
                set result [SiteCredits $event $target $amount $section]
            } else {
                iputs "Syntax: SITE $event <username> <credits> \[credit section\]"
            }
        }
        {INVITE} {
            if {$argLength == 2} {
                iputs ".-\[Invite\]---------------------------------------------------------------."
                set ircNick [lindex $argList 1]
                LinePuts "Inviting the IRC nick \"$ircNick\"."
                putlog "INVITE: \"$user\" \"$group\" \"$ircNick\""
                iputs "'------------------------------------------------------------------------'"
            } else {
                iputs "Syntax: SITE INVITE <irc nick>"
            }
        }
        {NEWDATE} {
            set result [NewDate [lindex $argList 1]]
        }
        {ONELINES} {
            set result [OneLines [join [lrange $argList 1 end]]]
        }
        {RESETSTATS} {
            if {$argLength > 1} {
                set result [SiteResetStats [join [lrange $argList 1 end]]]
            } else {
                iputs "Syntax: SITE RESETSTATS <stats type(s)>"
            }
        }
        {RESETUSER} {
            if {$argLength > 1} {
                set result [SiteResetUser [lindex $argList 1]]
            } else {
                iputs "Syntax: SITE RESETUSER <username>"
            }
        }
        {ROTATE} {
            set result [RotateLogs]
        }
        {SIZE} {
            if {$argLength > 1} {
                set virtualPath [GetPath $pwd [join [lrange $argList 1 end]]]
                set result [SiteSize $virtualPath]
            } else {
                iputs " Usage: SITE SIZE <file/directory>"
            }
        }
        {SYSLOG} {
            if {$argLength > 1 && [GetOptions [lrange $argList 1 end] limit pattern]} {
                iputs ".-\[SysopLog\]-------------------------------------------------------------."
                set result [SearchLog $log(SysOp) $limit $pattern]
            } else {
                iputs "Syntax: SITE SYSLOG \[-max <limit>\] <pattern>"
            }
        }
        {TRAFFIC} {
            set result [SiteTraffic [lindex $argList 1]]
        }
        {WEEKLY} {
            set result [WeeklyCredits [lindex $argList 1] [lindex $argList 2]]
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
