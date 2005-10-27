#
# nxTools - Dupe Checker, nuker, pre, request and utilities.
# Copyright (c) 2004-2005 neoxed
#
# Module Name:
#   Nuker
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements a nuker and related statistical commands.
#

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Nuke {
    namespace import -force ::nxLib::*
}

# Nuke Procedures
######################################################################

proc ::nxTools::Nuke::FindTags {realPath tagFormat} {
    regsub -all -- {%\(\w+\)} $tagFormat {*} tagFormat
    set tagFormat [string map {\[ \\\[ \] \\\] \{ \\\{ \} \\\}} $tagFormat]
    return [glob -nocomplain -types d -directory $realPath $tagFormat]
}

proc ::nxTools::Nuke::GetName {virtualPath} {
    set release [file tail [TrimTag $virtualPath]]
    if {[IsDiskPath $release]} {
        set parentPath [file tail [file dirname $virtualPath]]
        if {[string length $parentPath]} {set release "$parentPath ($release)"}
    }
    return $release
}

proc ::nxTools::Nuke::TrimTag {virtualPath} {
    global nuke
    set parentPath [file dirname $virtualPath]
    set release [file tail $virtualPath]
    if {![string first $nuke(Prefix) $release]} {
        set release [string range $release [string length $nuke(Prefix)] end]
    }
    return [file join $parentPath $release]
}

proc ::nxTools::Nuke::UpdateRecord {realPath {buffer ""}} {
    set record ""
    set realPath [file join $realPath ".ioFTPD.nxNuke"]
    set openMode [expr {$buffer eq "" ? "RDONLY CREAT" : "w"}]

    # Tcl cannot open hidden files, so the
    # hidden attribute must be removed first.
    catch {file attributes $realPath -hidden 0}

    if {[catch {set handle [open $realPath $openMode]} error]} {
        ErrorLog NukeRecord $error
    } elseif {![string length $buffer]} {
        set record [read $handle]
        close $handle
    } else {
        puts $handle $buffer
        close $handle
    }

    catch {file attributes $realPath -hidden 1}
    return [string trim $record]
}

proc ::nxTools::Nuke::UpdateUser {isNuke userName multi size files stats creditSection statSection} {
    set creditSection [expr {$creditSection + 1}]
    set statSection [expr {$statSection * 3 + 1}]
    set groupName "NoGroup"
    set newUserFile ""

    if {[userfile open $userName] == 0} {
        userfile lock
        set userFile [split [userfile bin2ascii] "\r\n"]
        foreach line $userFile {
            set type [string tolower [lindex $line 0]]
            if {$type eq "credits"} {
                set creditsOld [lindex $line $creditSection]
            } elseif {$type eq "groups"} {
                set groupName [GetGroupName [lindex $line 1]]
            } elseif {$type eq "ratio"} {
                set ratio [lindex $line $creditSection]
            }
        }

        if {$ratio != 0} {
            set creditsDiff [expr {(wide($size) * $ratio) + (wide($size) * ($multi - 1))}]
            set creditsNew [expr {wide($creditsOld) + ($isNuke ? -wide($creditsDiff) : wide($creditsDiff))}]
        } else {
            set creditsDiff 0
            set creditsNew $creditsOld
        }
        if {$isNuke} {
            set files [expr {-wide($files)}]
            set size [expr {-wide($size)}]
            set stats [expr {-wide($stats)}]
        }
        foreach line $userFile {
            set type [string tolower [lindex $line 0]]
            if {[lsearch -exact {allup dayup monthup wkup} $type] != -1} {
                set newFiles [expr {wide([lindex $line $statSection]) + $files}]
                set newStats [expr {wide([lindex $line [expr {$statSection + 1}]]) + wide($stats)}]
                set line [lreplace $line $statSection [expr {$statSection + 1}] $newFiles $newStats]
            } elseif {$type eq "credits"} {
                set line [lreplace $line $creditSection $creditSection $creditsNew]
            }
            append newUserFile $line "\r\n"
        }
        userfile ascii2bin $newUserFile
        userfile unlock
    }
    return [list $groupName $ratio $creditsOld $creditsNew $creditsDiff]
}

# Nuke Main
######################################################################

proc ::nxTools::Nuke::Main {argv} {
    global approve misc nuke flags gid group groups pwd uid user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}

    set argList [ArgList $argv]
    set event [string toupper [lindex $argList 0]]
    switch -- $event {
        NUKE - UNNUKE {
            if {$event eq "NUKE"} {
                iputs ".-\[Nuke\]-----------------------------------------------------------------."
                foreach {dummy target multi reason} $argList {break}
                if {[llength $argList] < 4 || ![string is digit -strict $multi]} {
                    ErrorReturn "Syntax: SITE NUKE <directory> <multiplier> <reason>"
                }
                if {$multi > $nuke(MultiMax)} {
                    ErrorReturn "The specified multiplier is to large, the max is $nuke(MultiMax)\x."
                }
                set isNuke 1
            } elseif {$event eq "UNNUKE"} {
                iputs ".-\[UnNuke\]---------------------------------------------------------------."
                if {[llength $argList] < 3} {
                    ErrorReturn "Syntax: SITE UNNUKE <directory> <reason>"
                }
                foreach {dummy target reason} $argList {break}
                set isNuke 0
            }
            set virtualPath [GetPath $pwd $target]
            set realPath [resolve pwd $virtualPath]
            if {![file isdirectory $realPath]} {
                ErrorReturn "The specified directory does not exist."
            }
            set matchPath [string range $virtualPath 0 [string last "/" $virtualPath]]
            if {[ListMatch $nuke(NoPaths) $matchPath]} {
                ErrorReturn "Not allowed to nuke from here."
            }
            if {$isNuke && [llength [FindTags $realPath $approve(DirTag)]]} {
                ErrorReturn "Approved releases cannot be nuked."
            }

            # Check if there is an available nuke record.
            set nukeId ""
            set record [UpdateRecord $realPath]
            set recordSplit [split $record "|"]

            # Record versions:
            # v1: nuke-type|user|group|multi|reason
            # v2: 2|status|id|user|group|multi|reason
            switch -regexp -- $record {
                {^2\|(0|1)\|\d+\|\S+\|\d+\|.+$} {
                    set nukeId [lindex $recordSplit 2]
                    # Only use the multiplier if it's a nuke record.
                    if {!$isNuke && [lindex $recordSplit 1] == 0} {
                        set multi [lindex $recordSplit 5]
                    }
                }
                {^(NUKE|UNNUKE)\|\S+\|\S+\|\d+\|.+$} {
                    # Only use the multiplier if it's a nuke record.
                    if {!$isNuke && [lindex $recordSplit 0] eq "UNNUKE"} {
                        set multi [lindex $recordSplit 3]
                    }
                }
                {} {}
                default {
                    ErrorLog NukeRecord "invalid nuke record for \"$realPath\": \"$record\""
                }
            }
            if {!$isNuke && ![info exists multi]} {
                ErrorReturn "Unable to find the nuke record."
            }

            set nukeType 0
            if {[string first $nuke(GroupFlag) $flags] != -1} {
                # Find the group suffix in the release name (Something-GRP).
                if {[set groupPos [string last "-" [file tail $virtualPath]]] == -1} {
                    ErrorReturn "Unable to verify the release's group, does it end with -<group>?"
                }
                set allowNuke 0; set nukeType 1
                set rlsSuffix [string range [file tail $virtualPath] [incr groupPos] end]
                foreach nukerGroup $groups {
                    if {$rlsSuffix eq $nukerGroup} {set allowNuke 1; break}
                }
                if {!$allowNuke} {ErrorReturn "You are not allowed to nuke other groups."}
            }
            set diskCount 0; set files 0; set totalSize 0
            set nukeTime [clock seconds]
            set reason [StripChars $reason]
            set release [GetName $virtualPath]
            set parentPath [file dirname $realPath]
            ListAssign [GetCreditStatSections $virtualPath] creditSection statSection

            # Count disk sub-directories.
            foreach entry [glob -nocomplain -types d -directory $realPath "*"] {
                if {[IsDiskPath $entry]} {incr diskCount}
            }

            # Count files and total size.
            GetDirList $realPath dirlist ".ioFTPD*"
            foreach entry $dirlist(FileList) {
                incr files; set fileSize [file size $entry]
                set totalSize [expr {wide($totalSize) + wide($fileSize)}]

                catch {lindex [vfs read $entry] 0} userId
                if {[set nukeeUser [resolve uid $userId]] ne ""} {
                    if {[info exists nukefiles($nukeeUser)]} {
                        incr nukefiles($nukeeUser)
                    } else {set nukefiles($nukeeUser) 1}

                    if {[info exists nukesize($nukeeUser)]} {
                        set nukesize($nukeeUser) [expr {wide($nukesize($nukeeUser)) + wide($fileSize)}]
                    } else {set nukesize($nukeeUser) $fileSize}
                }
            }
            set totalSize [expr {wide($totalSize) / 1024}]

            # Check if Release is an empty nuke (less than 5KB).
            if {$totalSize < 5 || ![array exists nukesize]} {
                set nukeType 2
                unset -nocomplain nukefiles nukesize
                catch {lindex [vfs read $realPath] 0} userId
                if {[set nukeeUser [resolve uid $userId]] ne ""} {
                    set nukefiles($nukeeUser) 0
                    set nukesize($nukeeUser) [expr {wide($nuke(EmptyNuke)) * 1024 * 1024}]
                } else {ErrorReturn "Unable to find the directory owner."}
            }
            LinePuts "Release : $release"
            LinePuts "Multi   : ${multi}x"
            LinePuts "Reason  : $reason"
            if {$nukeType} {
                # 0=Normal, 1=Group, 2=Empty
                switch -- $nukeType {
                    2 {set typeMsg "Empty"}
                    1 {set typeMsg "Group"}
                    default {set typeMsg "Unknown"}
                }
                LinePuts "Type    : $typeMsg Nuke"
            }
            LinePuts "Files   : [format %-16s ${files}F] Size: [format %-16s [FormatSize $totalSize]] CDs: $diskCount"
            if {$isNuke} {
                iputs "|------------------------------------------------------------------------|"
                iputs "|    User    |   Group    |    Ratio    |  Stats Lost   |  Credits Lost  |"
                iputs "|------------------------------------------------------------------------|"
            } else {
                iputs "|------------------------------------------------------------------------|"
                iputs "|    User    |   Group    |    Ratio    | Stats Gained  | Credits Gained |"
                iputs "|------------------------------------------------------------------------|"
            }

            # Change the credits and stats of nukees.
            set nukeeLog [list]
            foreach nukeeUser [lsort -ascii [array names nukesize]] {
                set nukeCredits [expr {wide($nukesize($nukeeUser)) / 1024}]
                set nukeStats [expr {$nukeType == 2 ? 0 : $nukeCredits}]
                set result [UpdateUser $isNuke $nukeeUser $multi $nukeCredits $nukefiles($nukeeUser) $nukeStats $creditSection $statSection]
                foreach {nukeeGroup ratio creditsOld creditsNew creditsDiff} $result {break}

                set ratio [expr {$ratio != 0 ? "1:$ratio" : "Unlimited"}]
                iputs [format "| %-10s | %-10s | %11s | %13s | %14s |" $nukeeUser $nukeeGroup $ratio [FormatSize $nukeStats] [FormatSize $creditsDiff]]
                lappend nukeeLog [list $nukeeUser $nukeeGroup $creditsDiff $nukeStats]
            }
            set nukeeLog [join [lsort -decreasing -integer -index 2 $nukeeLog]]

            if {$isNuke} {
                set mapList [list %(user) $user %(group) $group %(multi) $multi %(reason) $reason]
                set nukeTag [file join $realPath [string map $mapList $nuke(InfoTag)]]
                CreateTag $nukeTag $uid $gid 555
                RemoveParentLinks $realPath $virtualPath
                set dirChmod 555
                set logType "NUKE"
                set nukeStatus 0
                set newName "$nuke(Prefix)[file tail $virtualPath]"
            } else {
                foreach entry [FindTags $realPath $nuke(InfoTag)] {
                    RemoveTag $entry
                }
                set dirChmod 777
                set logType "UNNUKE"
                set nukeStatus 1
                set virtualPath [TrimTag $virtualPath]
                set newName [file tail $virtualPath]
            }

            set newPath [file join $parentPath $newName]
            if {![string equal -nocase $realPath $newPath]} {
                # In order to prevent users from re-entering the
                # directory while nuking, it will be chmodded to 000.
                catch {vfs read $realPath} owner
                ListAssign $owner userId groupId
                if {![string is digit -strict $userId]} {set userId [lindex $misc(DirOwner) 0]}
                if {![string is digit -strict $groupId]} {set groupId [lindex $misc(DirOwner) 1]}
                catch {vfs write $realPath $userId $groupId 000}

                KickUsers [file join $virtualPath "*"]
                if {[catch {file rename -force -- $realPath $newPath} error]} {
                    set renameFail 1
                    ErrorLog NukeRename $error
                    iputs "|------------------------------------------------------------------------|"
                    LinePuts "Unable to rename directory, ask a siteop to rename it manually."
                } else {
                    set renameFail 0
                }
            }

            GetDirList $newPath dirlist ".ioFTPD*"
            foreach entry $dirlist(DirList) {
                catch {vfs read $entry} owner
                ListAssign $owner userId groupId
                if {![string is digit -strict $userId]} {set userId [lindex $misc(DirOwner) 0]}
                if {![string is digit -strict $groupId]} {set groupId [lindex $misc(DirOwner) 1]}
                catch {vfs write $entry $userId $groupId $dirChmod}
            }
            catch {vfs flush $parentPath}
            putlog "${logType}: \"$virtualPath\" \"$user\" \"$group\" \"$multi\" \"$reason\" \"$files\" \"$totalSize\" \"$diskCount\" \"[join $nukeeLog]\""

            if {![catch {DbOpenFile [namespace current]::NukeDb "Nukes.db"} error]} {
                # To pass a NULL value to TclSQLite, the variable must be unset.
                if {![string is digit -strict $nukeId]} {unset nukeId}
                NukeDb eval {INSERT OR REPLACE INTO
                    Nukes(NukeId,TimeStamp,UserName,GroupName,Status,Release,Reason,Multi,Files,Size)
                    VALUES($nukeId,$nukeTime,$user,$group,$nukeStatus,$release,$reason,$multi,$files,$totalSize)
                }
                set nukeId [NukeDb last_insert_rowid]

                NukeDb eval {BEGIN}
                foreach {nukeeUser nukeeGroup nukeeCredits nukeeStats} $nukeeLog {
                    NukeDb eval {INSERT OR REPLACE INTO
                        Users(NukeId,UserName,GroupName,Amount)
                        VALUES($nukeId,$nukeeUser,$nukeeGroup,$nukeeStats)
                    }
                }
                NukeDb eval {COMMIT}

                NukeDb close
            } else {ErrorLog NukeDb $error}

            # Save the nuke ID and multiplier for later use (ie. unnuke).
            UpdateRecord [expr {$renameFail ? $realPath : $newPath}] "2|$nukeStatus|$nukeId|$user|$group|$multi|$reason"
            iputs "'------------------------------------------------------------------------'"
        }
        NUKES - UNNUKES {
            if {![GetOptions [lrange $argList 1 end] limit pattern]} {
                iputs "Syntax: SITE $event \[-max <limit>\] \[release\]"
                return 0
            }
            if {$event eq "NUKES"} {
                iputs ".-\[Nukes\]----------------------------------------------------------------."
                iputs "|    Age    |    Nuker     |   Multi   |  Reason                         |"
                iputs "|------------------------------------------------------------------------|"
                set nukeStatus 0
            } elseif {$event eq "UNNUKES"} {
                iputs ".-\[UnNukes\]--------------------------------------------------------------."
                iputs "|    Age    |   UnNuker    |   Multi   |  Reason                         |"
                iputs "|------------------------------------------------------------------------|"
                set nukeStatus 1
            }
            set count 0
            set pattern [SqlGetPattern $pattern]

            if {![catch {DbOpenFile [namespace current]::NukeDb "Nukes.db"} error]} {
                NukeDb eval "SELECT * FROM Nukes WHERE Status=$nukeStatus AND Release \
                        LIKE '$pattern' ESCAPE '\\' ORDER BY TimeStamp DESC LIMIT $limit" values {
                    incr count
                    set nukeAge [FormatDuration [expr {[clock seconds] - $values(TimeStamp)}]]
                    iputs [format "| %-9.9s | %-12.12s | %-9.9s | %-31.31s |" [lrange $nukeAge 0 1] $values(UserName) $values(Multi)x $values(Reason)]
                    iputs [format "| Dir: %-65.65s |" $values(Release)]
                    iputs "|------------------------------------------------------------------------|"
                }
                NukeDb close
            } else {ErrorLog NukeLatest $error}

            if {!$count} {
                LinePuts "There are no nukes or unnukes to display."
            } else {
                LinePuts "Read the rules to avoid being nuked."
            }
            iputs "'------------------------------------------------------------------------'"
        }
        NUKETOP {
            if {![GetOptions [lrange $argList 1 end] limit pattern]} {
                iputs "Syntax: SITE NUKETOP \[-max <limit>\] \[group\]"
                return 0
            }
            iputs ".-\[NukeTop\]--------------------------------------------------------------."
            iputs "|    User    |   Group    | Times Nuked |  Amount                        |"
            iputs "|------------------------------------------------------------------------|"

            if {[string length $pattern]} {
                set groupMatch "GroupName LIKE '[SqlWildToLike $pattern]' ESCAPE '\\' AND"
            } else {
                set groupMatch ""
            }
            set count 0

            if {![catch {DbOpenFile [namespace current]::NukeDb "Nukes.db"} error]} {
                NukeDb eval "SELECT UserName, GroupName, count(*) AS Nuked, sum(Amount) AS Amount FROM Users \
                        WHERE $groupMatch (SELECT count(*) FROM Nukes WHERE NukeId=Users.NukeId AND Status=0) \
                        GROUP BY UserName ORDER BY Nuked DESC LIMIT $limit" values {
                    incr count
                    iputs [format "| %-10.10s | %-10.10s | %11d | %30.30s |" $values(UserName) $values(GroupName) $values(Nuked) [FormatSize $values(Amount)]]
                }
                NukeDb close
            } else {ErrorLog NukeTop $error}

            if {!$count} {LinePuts "There are no nukees to display."}
            iputs "'------------------------------------------------------------------------'"
        }
        default {
            ErrorLog InvalidArgs "unknown event \"[info script] $event\": check your ioFTPD.ini for errors"
        }
    }
    return 0
}

::nxTools::Nuke::Main [expr {[info exists args] ? $args : ""}]
