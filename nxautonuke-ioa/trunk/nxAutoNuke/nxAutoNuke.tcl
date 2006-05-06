#
# nxAutoNuke - Extensive auto-nuker for ioFTPD.
# Copyright (c) 2004-2006 neoxed
#
# Module Name:
#   Auto-nuker
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements an auto-nuker using nxTools.
#

if {[catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxAutoNuke {
    namespace import -force ::nxLib::*
}

# Nuke Procedures
######################################################################

proc ::nxAutoNuke::GetName {virtualPath} {
    set release [file tail $virtualPath]
    if {[IsDiskPath $release]} {
        set parentPath [file tail [file dirname $virtualPath]]
        if {[string length $parentPath]} {set release "$parentPath ($release)"}
    }
    return $release
}

proc ::nxAutoNuke::UpdateRecord {recordPath {data ""}} {
    set mode [expr {$data eq "" ? "RDONLY CREAT" : "w"}]
    set record ""

    # Tcl cannot open hidden files, so the
    # hidden attribute must be removed first.
    catch {file attributes $recordPath -hidden 0}

    if {[catch {set handle [open $recordPath $mode]} error]} {
        ErrorLog AutoNuke $error
    } elseif {![string length $data]} {
        gets $handle record
        close $handle
    } else {
        puts $handle $data
        close $handle
    }

    catch {file attributes $recordPath -hidden 1}
    return $record
}

proc ::nxAutoNuke::UpdateUser {userName multi size files stats creditSection statSection} {
    set creditSection [expr {$creditSection + 1}]
    set statSection [expr {$statSection * 3 + 1}]
    set newUserFile [list]

    # Default user values.
    set groupName "NoGroup"; set ratio -1
    set creditsOld 0; set creditsNew 0; set creditsDiff 0

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
            set creditsNew [expr {wide($creditsOld) - wide($creditsDiff)}]
        } else {
            set creditsDiff 0
            set creditsNew $creditsOld
        }
        foreach line $userFile {
            set type [string tolower [lindex $line 0]]
            if {[lsearch -exact {allup dayup monthup wkup} $type] != -1} {
                set newFiles [expr {wide([lindex $line $statSection]) - $files}]
                set newStats [expr {wide([lindex $line [expr {$statSection + 1}]]) - wide($stats)}]
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

proc ::nxAutoNuke::Nuke {realPath virtualPath nukerUser nukerGroup multi reason} {
    global misc nuke
    if {![file isdirectory $realPath]} {
        ErrorLog AutoNuke "unable to nuke \"$virtualPath\": directory does not exist."
        return 0
    } elseif {![string is digit -strict $multi]} {
        ErrorLog AutoNuke "unable to nuke \"$virtualPath\": invalid multiplier value ($multi)."
        return 0
    }

    # Check if there is an available nuke record.
    set nukeId ""
    set record [UpdateRecord [file join $realPath ".ioFTPD.nxNuke"]]
    set recordSplit [split $record "|"]

    # Record versions:
    # v1: nuke-type|user|group|multi|reason
    # v2: 2|status|id|user|group|multi|reason
    switch -regexp -- $record {
        {^2\|(0|1)\|\d+\|\S+\|\d+\|.+$} {
            set nukeId [lindex $recordSplit 2]
        }
        {^(NUKE|UNNUKE)\|\S+\|\S+\|\d+\|.+$} {}
        {} {}
        default {
            ErrorLog AutoNuke "invalid nuke record for \"$realPath\": \"$record\""
        }
    }
    set diskCount 0; set files 0; set totalSize 0
    set nukeTime [clock seconds]
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
        unset -nocomplain nukefiles nukesize
        set emptyNuke 1
        catch {lindex [vfs read $realPath] 0} userId
        if {[set nukeeUser [resolve uid $userId]] ne ""} {
            set nukefiles($nukeeUser) 0
            set nukesize($nukeeUser) [expr {wide($nuke(EmptyNuke)) * 1024 * 1024}]
        } else {
            ErrorLog AutoNuke "unable to nuke \"$virtualPath\": could not find directory owner."
            return 0
        }
    } else {set emptyNuke 0}

    # Change the credits and stats of nukees.
    set nukeeLog [list]
    foreach nukeeUser [lsort -ascii [array names nukesize]] {
        set nukeCredits [expr {wide($nukesize($nukeeUser)) / 1024}]
        set nukeStats [expr {$emptyNuke ? 0 : $nukeCredits}]
        set result [UpdateUser $nukeeUser $multi $nukeCredits $nukefiles($nukeeUser) $nukeStats $creditSection $statSection]
        foreach {nukeeGroup ratio creditsOld creditsNew creditsDiff} $result {break}
        lappend nukeeLog [list $nukeeUser $nukeeGroup $creditsDiff $nukeStats]
    }
    set nukeeLog [join [lsort -decreasing -integer -index 2 $nukeeLog]]

    # Create nuke tag.
    set mapList [list %(user) $nukerUser %(group) $nukerGroup %(multi) $multi %(reason) $reason]
    set nukeTag [file join $realPath [string map $mapList $nuke(InfoTag)]]
    CreateTag $nukeTag [resolve user $nukerUser] [resolve group $nukerGroup] 555
    RemoveParentLinks $realPath $virtualPath

    set newName "$nuke(Prefix)[file tail $virtualPath]"
    set newPath [file join $parentPath $newName]

    set renameFailed 0
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
            set renameFailed 1
            ErrorLog AutoNuke $error
        }
    }

    GetDirList $newPath dirlist ".ioFTPD*"
    foreach entry $dirlist(DirList) {
        catch {vfs read $entry} owner
        ListAssign $owner userId groupId
        if {![string is digit -strict $userId]} {set userId [lindex $misc(DirOwner) 0]}
        if {![string is digit -strict $groupId]} {set groupId [lindex $misc(DirOwner) 1]}
        catch {vfs write $entry $userId $groupId 555}
    }
    catch {vfs flush $parentPath}
    putlog "NUKE: \"$virtualPath\" \"$nukerUser\" \"$nukerGroup\" \"$multi\" \"$reason\" \"$files\" \"$totalSize\" \"$diskCount\" \"[join $nukeeLog]\""

    if {![catch {DbOpenFile [namespace current]::NukeDb "Nukes.db"} error]} {
        # To pass a NULL value to TclSQLite, the variable must be unset.
        if {![string is digit -strict $nukeId]} {unset nukeId}
        NukeDb eval {INSERT OR REPLACE INTO
            Nukes(NukeId,TimeStamp,UserName,GroupName,Status,Release,Reason,Multi,Files,Size)
            VALUES($nukeId,$nukeTime,$nukerUser,$nukerGroup,0,$release,$reason,$multi,$files,$totalSize)
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
    } else {ErrorLog AutoNuke $error}

    # Save the nuke ID and multiplier for later use (ie. unnuke).
    set record "2|0|$nukeId|$nukerUser|$nukerGroup|$multi|$reason"
    if {$renameFailed} {UpdateRecord [file join $realPath ".ioFTPD.nxNuke"] $record}
    UpdateRecord [file join $newPath ".ioFTPD.nxNuke"] $record
    return 1
}

# Auto Nuke Checks
######################################################################

proc ::nxAutoNuke::CheckAllowed {groupList releaseName} {
    variable check
    if {[set groupPos [string last "-" $releaseName]] == -1} {return 0}
    set groupName [string range $releaseName [incr groupPos] end]

    foreach entry $groupList {
        if {[string match -nocase $entry $groupName]} {return 0}
    }
    set check(Cookies) [list %(group) $groupName]
    set check(WarnData) "\"$groupName\" "
    return 1
}

proc ::nxAutoNuke::CheckBanned {patternList releaseName} {
    variable check
    foreach pattern $patternList {
        if {[string match -nocase $pattern $releaseName]} {
            set check(Cookies) [list %(banned) $pattern]
            set check(WarnData) "\"$pattern\" "
            return 1
        }
    }
    return 0
}

proc ::nxAutoNuke::CheckDisks {diskMax diskCount} {
    variable check
    if {$diskCount > $diskMax} {
        set check(Cookies) [list %(disks) $diskCount %(maxdisks) $diskMax]
        set check(WarnData) "\"$diskCount\" \"$diskMax\" "
        return 1
    }
    return 0
}

proc ::nxAutoNuke::CheckEmpty {realPath} {
    foreach fileName [glob -nocomplain -types f -directory $realPath "*"] {
        if {![string equal -nocase -length 7 ".ioFTPD" [file tail $fileName]]} {
            return 0
        }
    }
    return 1
}

proc ::nxAutoNuke::CheckInc {realPath} {
    global anuke
    # Check for incomplete tags.
    foreach entry [glob -nocomplain -types d -directory $realPath "*"] {
        set dirName [file tail $entry]
        if {[string match -nocase $anuke(IncTag) $dirName]} {
            return 1
        }
    }
    # Check for .bad or .missing files.
    foreach entry [glob -nocomplain -types f -directory $realPath "*"] {
        set fileName [file tail $entry]
        if {[string match -nocase $anuke(BadExt) $fileName] ||
                [string match -nocase $anuke(MissingExt) $fileName]} {
            return 1
        }
    }
    return 0
}

proc ::nxAutoNuke::CheckImdb {options realPath} {
    global anuke
    variable check
    set found 0; set genre ""; set rating ""; set year ""

    foreach tagName [FindTags $realPath] {
        set reMatch [regexp -inline -nocase -- $anuke(ImdbMatch) [file tail $tagName]]
        if {[llength $reMatch]} {
            foreach $anuke(ImdbOrder) [lrange $reMatch 1 end] {break}
            set found 1; break
        }
    }
    if {!$found} {return 0}

    foreach {type value} $options {
        set nuke 0
        switch -- $type {
            genre {
                # Banned genres.
                if {[string length $genre] && [string match -nocase $value $genre]} {
                    set nuke 1
                }
            }
            rating {
                # Minimum rating.
                if {[string is double -strict $rating] && $rating < $value} {
                    set nuke 1
                }
            }
            year {
                # Accepted years.
                if {[string length $year] && ![string match -nocase $value $year]} {
                    set nuke 1
                }
            }
            default {
                ErrorLog AutoNuke "Unknown IMDB setting \"$type\"."
            }
        }

        if {$nuke} {
            set value [set $type]
            set check(Cookies) [list %(type) $type %(banned) $value]
            set check(WarnData) "\"$type\" \"$value\" "
            return 1
        }
    }
    return 0
}

proc ::nxAutoNuke::CheckKeyword {keywordList releaseName} {
    variable check
    if {[set groupPos [string last "-" $releaseName]] != -1} {
        set releaseName [string range $releaseName 0 [incr groupPos -1]]
    }
    set releaseSplit [string tolower [split $releaseName "()-._"]]

    foreach keyword $keywordList {
        if {[lsearch -exact $releaseSplit $keyword] != -1} {
            set check(Cookies) [list %(banned) $keyword]
            set check(WarnData) "\"$keyword\" "
            return 1
        }
    }
    return 0
}

proc ::nxAutoNuke::CheckMP3 {options realPath} {
    global anuke
    variable check
    set found 0; set bitrate 0; set genre ""; set year ""

    foreach tagName [FindTags $realPath] {
        set reMatch [regexp -inline -nocase -- $anuke(MP3Match) [file tail $tagName]]
        if {[llength $reMatch]} {
            foreach $anuke(MP3Order) [lrange $reMatch 1 end] {break}
            set found 1; break
        }
    }
    if {!$found} {return 0}

    foreach {type value} $options {
        set nuke 0
        switch -- $type {
            bitrate {
                # Minimum and maximum bitrate.
                if {[string is double -strict $bitrate] && $bitrate > 0} {
                    set min [lindex $value 0]
                    set max [lindex $value 1]

                    if {![string is double -strict $min] || ![string is double -strict $max]} {
                        ErrorLog AutoNuke "Invalid MP3 bitrate setting \"$value\"."
                    } elseif {($min > 0 && $min > $bitrate) || ($max > 0 && $max < $bitrate)} {
                        set nuke 1
                    }
                }
            }
            genre {
                # Banned genres.
                if {[string length $genre] && [string match -nocase $value $genre]} {
                    set nuke 1
                }
            }
            year {
                # Accepted years.
                if {[string length $year] && ![string match -nocase $value $year]} {
                    set nuke 1
                }
            }
            default {
                ErrorLog AutoNuke "Unknown MP3 setting \"$type\"."
            }
        }

        if {$nuke} {
            set value [set $type]
            set check(Cookies) [list %(type) $type %(banned) $value]
            set check(WarnData) "\"$type\" \"$value\" "
            return 1
        }
    }
    return 0
}

proc ::nxAutoNuke::CheckSize {options realPath} {
    variable check

    set min [lindex $options 0]
    set max [lindex $options 1]
    if {![string is double -strict $min] || ![string is double -strict $max]} {
        ErrorLog AutoNuke "Invalid size option \"$options\"."
        return 1
    }
    GetDirStats $realPath stats ".ioFTPD*"
    set kiloBytes [expr {$stats(TotalSize) / 1024.0}]

    if {$min > 0 && ($min * 1024.0) > $kiloBytes} {
        set check(Cookies) [list %(type) "minimum" %(size) $min]
        set check(WarnData) "\"minimum\" \"$min\" "
        return 1
    }

    if {$max > 0 && ($max * 1024.0) < $kiloBytes} {
        set check(Cookies) [list %(type) "maximum" %(size) $max]
        set check(WarnData) "\"maximum\" \"$max\" "
        return 1
    }
    return 0
}

# Auto Nuke Procedures
######################################################################

proc ::nxAutoNuke::GetUserList {realPath} {
    global anuke
    GetDirList $realPath dirlist ".ioFTPD*"
    foreach entry $dirlist(FileList) {
        if {[file size $entry] < 1} {continue}
        catch {vfs read $entry} owner
        ListAssign $owner userId groupId
        if {[set userName [resolve uid $userId]] != "" && ![info exists uploader($userName)]} {
            set uploader($userName) [GetGroupName $groupId]
        }
    }

    # Check if the release is an empty nuke.
    if {![array exists uploader]} {
        catch {vfs read $realPath} owner
        ListAssign $owner userId groupId
        if {[set userName [resolve uid $userId]] != ""} {
            set uploader($userName) [GetGroupName $groupId]
        } else {return ""}
    }

    # Format uploaders list.
    set formatList [list]
    foreach {userName groupName} [array get uploader] {
        set mapList [list %b \002 %c \003 %u \031 %(user) $userName %(group) $groupName]
        lappend formatList [string map $mapList $anuke(UserFormat)]
    }
    return [join [lsort -ascii $formatList] $anuke(UserSplit)]
}

proc ::nxAutoNuke::FindTags {realPath {tagTemplate "*"}} {
    set tagTemplate [string map {\[ \\\[ \] \\\] \{ \\\{ \} \\\}} $tagTemplate]
    return [glob -nocomplain -types d -directory $realPath $tagTemplate]
}

proc ::nxAutoNuke::SplitOptions {type options} {
    set result [list]
    foreach entry $options {
        set value [split $entry ":"]

        if {[llength $value] == 2} {
            lappend result [string tolower [lindex $value 0]] [lindex $value 1]
        } else {
            ErrorLog AutoNuke "Invalid $type entry \"$entry\"."
        }
    }
    return $result
}

proc ::nxAutoNuke::NukeAllowed {realPath} {
    variable nukedList
    set check [string tolower $realPath]
    if {[lsearch -exact $nukedList $check] != -1} {
        return 0
    }
    lappend nukedList $check
    return 1
}

proc ::nxAutoNuke::WarnAllowed {type realPath} {
    global anuke
    variable nukedList
    variable warnedList

    # Check if the release was nuked or the warning type was announced.
    set check [string tolower $realPath]
    if {[lsearch -exact $nukedList $check] != -1 ||
            ([info exists warnedList($type)] &&
            [lsearch -exact $warnedList($type) $check] != -1)} {
        return 0
    }

    # Check if the warning was announced earlier (from a previous execution).
    if {[IsTrue $anuke(WarnOnce)]} {
        set recordPath [file join $realPath ".ioFTPD.nxAutoNuke"]
        set record [UpdateRecord $recordPath]
        if {[lsearch -exact $record $type] != -1} {
            return 0
        }
        UpdateRecord $recordPath [lappend record $type]
    }

    lappend warnedList($type) $check
    return 1
}

proc ::nxAutoNuke::NukeCheck {realPath virtualPath dirAge} {
    global anuke
    variable check

    # Format the reason.
    lappend check(Cookies) %(age) [expr {$dirAge / 60}]
    set check(Reason) [StripChars [string map $check(Cookies) $check(Reason)]]

    set nukeSecs [expr {$check(NukeMins) * 60}]
    set warnSecs [expr {$check(WarnMins) * 60}]

    if {$dirAge >= $nukeSecs && [NukeAllowed $realPath]} {
        # Nuke the entire release if anuke(SubDir) is false.
        if {![IsTrue $anuke(SubDir)] && [IsDiskPath $virtualPath]} {
            set realPath [file dirname $realPath]
            set virtualPath [file dirname $virtualPath]
        }

        LinePuts "- Nuking: [GetName $virtualPath] - $check(Reason)"
        if {![Nuke $realPath $virtualPath $anuke(UserName) $anuke(GroupName) $check(Multi) $check(Reason)]} {
            LinePuts "- Unable to nuke the the release, check nxError.log for details."
        }
    } elseif {$dirAge >= $warnSecs && [WarnAllowed $check(Type) $realPath]} {
        # Obtain a list of nuked users.
        if {[IsTrue $anuke(UserList)]} {
            set userList [GetUserList $realPath]
        } else {
            set userList "Disabled"
        }

        LinePuts "- Warning: [GetName $virtualPath] - $check(Reason)"
        putlog "$check(WarnType): \"$virtualPath\" $check(WarnData)\"$dirAge\" \"[expr {$nukeSecs - $dirAge}]\" \"$nukeSecs\" \"$check(Multi)\" \"$userList\""
    }
}

# AutoNuke Main
######################################################################

proc ::nxAutoNuke::Main {} {
    global anuke misc user group
    variable check
    variable nukedList
    variable warnedList

    # A userfile and VFS file will have to be opened so that resolve works under ioFTPD's scheduler.
    if {![info exists user] && ![info exists group]} {
        if {[userfile open $misc(MountUser)] != 0} {
            ErrorLog AutoNuke "error opening the user \"$misc(MountUser)\""
            return 1
        } elseif {[mountfile open $misc(MountFile)] != 0} {
            ErrorLog AutoNuke "error mounting the VFS-file \"$misc(MountFile)\""
            return 1
        }
    }
    iputs ".-\[AutoNuke\]-------------------------------------------------------------."
    LinePuts "Checking [expr {[llength $anuke(Sections)] / 3}] auto-nuke sections."

    # Initialise variables and options.
    set anuke(ImdbOrder) [string tolower $anuke(ImdbOrder)]
    set anuke(MP3Order)  [string tolower $anuke(MP3Order)]
    set maxAge [expr {$anuke(MaximumAge) * 60}]
    set nukedList [list]
    set timeNow [clock seconds]
    unset -nocomplain warnedList

    #
    # Check Variables:
    #
    # check(Type)     - Type of auto-nuke check.
    # check(Options)  - Options specific to the check type.
    # check(Multi)    - Nuke multiplier.
    # check(WarnMins) - Minutes until the warning is announced.
    # check(NukeMins) - Minutes until the release is nuked.
    # check(Cookies)  - List of reason cookies.
    # check(Reason)   - Nuke reason template.
    # check(WarnType) - Warning log event type.
    # check(WarnData) - Warning log check data.
    #
    # Release Specific Variables:
    #
    # release(Age)         - Age of release, in seconds.
    # release(Name)        - Release name.
    # release(RealPath)    - Release physical path.
    # release(VirtualPath) - Release virtual path.
    # release(PathList)    - Release sub-directory list.
    #
    # Disk Specific Variables:
    #
    # disk(Age)         - Age of disk sub-directory, in seconds.
    # disk(Name)        - Name of disk sub-directory.
    # disk(RealPath)    - Disk physical path.
    # disk(VirtualPath) - Disk virtual path.
    #
    array set releaseChecks [list \
        allowed    [list ANUKEALLOWED $anuke(ReasonAllowed) {CheckAllowed $check(Options) $release(Name)}] \
        banned     [list ANUKEBANNED  $anuke(ReasonBanned)  {CheckBanned  $check(Options) $release(Name)}] \
        disks      [list ANUKEDISKS   $anuke(ReasonDisks)   {CheckDisks   $check(Options) [llength $release(PathList)]}] \
        imdb       [list ANUKEIMDB    $anuke(ReasonImdb)    {CheckImdb    $check(Options) $release(RealPath)}] \
        keyword    [list ANUKEKEYWORD $anuke(ReasonKeyword) {CheckKeyword $check(Options) $release(Name)}] \
        size       [list ANUKESIZE    $anuke(ReasonSize)    {CheckSize    $check(Options) $release(RealPath)}] \
    ]
    array set diskChecks [list \
        empty      [list ANUKEEMPTY   $anuke(ReasonEmpty)   {CheckEmpty $disk(RealPath)}] \
        incomplete [list ANUKEINC     $anuke(ReasonInc)     {CheckInc   $disk(RealPath)}] \
        mp3        [list ANUKEMP3     $anuke(ReasonMP3)     {CheckMP3   $check(Options) $disk(RealPath)}] \
    ]

    foreach {sectionVirtualPath offset settings} $anuke(Sections) {
        # Convert virtual path date cookies.
        set formatTime [expr {$timeNow + ($offset * 86400)}]
        set sectionVirtualPath [clock format $formatTime -format $sectionVirtualPath -gmt [IsTrue $misc(UtcTime)]]
        LinePuts ""
        LinePuts "Checking path: $sectionVirtualPath (day offset: $offset)"
        set sectionRealPath [resolve pwd $sectionVirtualPath]
        if {![file isdirectory $sectionRealPath]} {
            LinePuts "- The directory does not exist."
            continue
        }

        # Process the earliest nuke times first.
        if {[catch {set settings [lsort -increasing -integer -index 4 $settings]} error]} {
            LinePuts "- Invalid settings, check your configuration."
            ErrorLog AutoNuke "invalid settings for \"$sectionVirtualPath\": $error"
            continue
        }

        # Parse the settings for each individual check.
        set settingsList [list]
        foreach entry $settings {
            foreach {type options multi warnMins nukeMins} $entry {break}
            set type [string tolower $type]

            if {$type eq "imdb" || $type eq "mp3"} {
                set options [SplitOptions $type $options]
            } elseif {$type eq "keyword"} {
                set options [string tolower $options]
            }
            lappend settingsList $type $options $multi $warnMins $nukeMins
        }

        foreach release(RealPath) [glob -nocomplain -types d -directory $sectionRealPath "*"] {
            set release(Name) [file tail $release(RealPath)]

            # Ignore exempted, approved, and old releases.
            if {[ListMatchI $anuke(Exempts) $release(Name)] ||
                    [llength [FindTags $release(RealPath) $anuke(ApproveTag)]] ||
                    [catch {file stat $release(RealPath) stat}] ||
                    [set release(Age) [expr {[clock seconds] - $stat(ctime)}]] > $maxAge} {
                continue
            }

            # Find all disk sub-directories.
            set release(PathList) [list]
            foreach diskDir [glob -nocomplain -types d -directory $release(RealPath) "*"] {
                if {![ListMatchI $anuke(Exempts) [file tail $diskDir]] && [IsDiskPath $diskDir]} {
                    lappend release(PathList) $diskDir
                }
            }

            # Use the root directory if there are no sub-directories.
            if {![llength $release(PathList)]} {
                lappend release(PathList) $release(RealPath)
            }
            set release(VirtualPath) [file join $sectionVirtualPath $release(Name)]

            foreach {check(Type) check(Options) check(Multi) check(WarnMins) check(NukeMins)} $settingsList {
                if {[info exists releaseChecks($check(Type))]} {
                    foreach {check(WarnType) check(Reason) checkProc} $releaseChecks($check(Type)) {break}

                    if {[eval $checkProc]} {
                        NukeCheck $release(RealPath) $release(VirtualPath) $release(Age)
                    }
                } elseif {[info exists diskChecks($check(Type))]} {
                    foreach {check(WarnType) check(Reason) checkProc} $diskChecks($check(Type)) {break}

                    # Check each release sub-directory.
                    foreach disk(RealPath) $release(PathList) {
                        if {[IsDiskPath $disk(RealPath)]} {
                            # Retrieve the sub-directory's age.
                            if {[catch {file stat $disk(RealPath) stat}]} {continue}
                            set disk(Age) [expr {[clock seconds] - $stat(ctime)}]

                            set disk(Name) [file tail $disk(RealPath)]
                            set disk(VirtualPath) [file join $release(VirtualPath) $disk(Name)]
                        } else {
                            array set disk [array get release]
                        }
                        if {[eval $checkProc]} {
                            NukeCheck $disk(RealPath) $disk(VirtualPath) $disk(Age)
                        }
                    }
                } else {
                    ErrorLog AutoNuke "Invalid auto-nuke type \"$check(Type)\"."
                    break
                }
            }
        }
    }
    unset -nocomplain check nukedList warnedList

    iputs "'------------------------------------------------------------------------'"
    return 0
}

::nxAutoNuke::Main
