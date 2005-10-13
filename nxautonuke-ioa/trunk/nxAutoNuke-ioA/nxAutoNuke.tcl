#
# nxAutoNuke - Extensive auto-nuker for ioFTPD.
# Copyright (c) 2004-2005 neoxed
#
# Module Name:
#   Auto-nuker
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements an auto-nuker using ioA.
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

proc ::nxAutoNuke::Nuke {realPath virtualPath userName groupName multi reason} {
    global misc
    # Find credit and stats section
    foreach {creditSection statSection} [GetCreditStatSections $virtualPath] {break}

    # Borrowed this portion from Harm's ioAUTONUKE, since
    # these features are undocumented for ioA and ioBanana.
    if {[string length $misc(IoBPath)]} {
        set parentVirtual [file dirname $virtualPath]
        set parentReal [string map {/ \\} [file dirname $realPath]]
        catch {exec $misc(IoBPath) KICKNUKE NUKE [file tail $virtualPath] $parentVirtual $parentReal [resolve user $userName]}
    }

    set realPath [string map {/ \\} $realPath]
    if {[catch {exec $misc(IoAPath) NUKE $realPath $virtualPath $multi $statSection $creditSection $userName $groupName $reason} output]} {
        ErrorLog AutoNuke "ioA Output:\n$output"
        return 0
    }
    foreach line [split $output "\r\n"] {
        if {![string first "NUKE:" $line]} {putlog $line}
    }
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
    foreach entry [glob -nocomplain -types f -directory $realPath "*"] {
        set fileName [file tail $entry]
        if {[string match $anuke(BadExt) $fileName] || [string match $anuke(MissingExt) $fileName]} {
            return 1
        }
    }
    return 0
}

proc ::nxAutoNuke::CheckImdb {checkList realPath} {
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

    foreach {type value} $checkList {
        set nuke 0
        switch -- $type {
            genre {
                if {[string match -nocase $value $genre]} {set nuke 1}
            }
            rating {
                if {[string is double -strict $rating] && $rating < $value} {set nuke 1}
            }
            year {
                if {![string match -nocase $value $year]} {set nuke 1}
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

proc ::nxAutoNuke::CheckMP3 {checkList realPath} {
    global anuke
    variable check
    set found 0; set genre ""; set year ""

    foreach tagName [FindTags $realPath] {
        set reMatch [regexp -inline -nocase -- $anuke(MP3Match) [file tail $tagName]]
        if {[llength $reMatch]} {
            foreach $anuke(MP3Order) [lrange $reMatch 1 end] {break}
            set found 1; break
        }
    }
    if {!$found} {return 0}

    foreach {type value} $checkList {
        set nuke 0
        switch -- $type {
            genre {
                if {[string match -nocase $value $genre]} {set nuke 1}
            }
            year {
                if {![string match -nocase $value $year]} {set nuke 1}
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
    set formatList ""
    foreach {userName groupName} [array get uploader] {
        set reMap [list %b \002 %c \003 %u \031 %(user) $userName %(group) $groupName]
        lappend formatList [string map $reMap $anuke(UserFormat)]
    }
    return [join [lsort -ascii $formatList] $anuke(UserSplit)]
}

proc ::nxAutoNuke::FindTags {realPath {tagTemplate "*"}} {
    set tagTemplate [string map {\[ \\\[ \] \\\] \{ \\\{ \} \\\}} $tagTemplate]
    return [glob -nocomplain -types d -directory $realPath $tagTemplate]
}

proc ::nxAutoNuke::SplitSettings {settings} {
    set checkList [list]
    foreach entry $settings {
        foreach {name value} [split $entry ":"] {break}
        lappend checkList [string tolower $name] $value
    }
    return $checkList
}

proc ::nxAutoNuke::NukeCheck {realPath virtualPath dirAge} {
    global anuke
    variable check
    variable nukedList
    variable warnedList

    # Skip the release if it was already nuked.
    set checkPath [string tolower $realPath]
    if {[lsearch -exact $nukedList $checkPath] != -1} {return}

    set nukeSecs [expr {$check(NukeMins) * 60}]
    set warnSecs [expr {$check(WarnMins) * 60}]

    if {$dirAge >= $nukeSecs} {
        # Nuke the release.
        lappend check(Cookies) %(age) [expr {$dirAge / 60}]
        set check(Reason) [StripChars [string map $check(Cookies) $check(Reason)]]

        # Nuke the entire release if anuke(SubDirs) is false.
        if {![IsTrue $anuke(SubDirs)] && [IsDiskPath $virtualPath]} {
            set realPath [file dirname $realPath]
            set virtualPath [file dirname $virtualPath]
        }

        LinePuts "- Nuking: [GetName $virtualPath] - $check(Reason)"
        if {![Nuke $realPath $virtualPath $anuke(UserName) $anuke(GroupName) $check(Multi) $check(Reason)]} {
            LinePuts "- Unable to nuke the the release, check nxError.log for details."
        }
        lappend nukedList $checkPath
    } elseif {$dirAge >= $warnSecs && [lsearch -exact $warnedList $checkPath] == -1} {
        # Obtain a list of nuked users.
        if {[IsTrue $anuke(WarnUsers)]} {
            set userList [GetUserList $realPath]
        } else {
            set userList "Disabled"
        }

        # Log the warning.
        set dirAge [expr {$dirAge / 60}]
        lappend check(Cookies) %(age) $dirAge
        set check(Reason) [StripChars [string map $check(Cookies) $check(Reason)]]

        LinePuts "- Warning: [GetName $virtualPath] - $check(Reason)"
        putlog "$check(WarnType): \"$virtualPath\" $check(WarnData)\"$dirAge\" \"[expr {$check(NukeMins) - $dirAge}]\" \"$check(NukeMins)\" \"$check(Multi)\" \"$userList\""
        lappend warnedList $checkPath
    }
    return
}

# AutoNuke Main
######################################################################

proc ::nxAutoNuke::Main {} {
    global anuke misc user group
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
    if {![file isfile $misc(IoAPath)]} {
        ErrorLog AutoNuke "invalid path to ioA \"$misc(IoAPath)\": the file does not exist"
        ErrorReturn "Invalid path to the ioA executable, check your configuration."
    }
    if {[string length $misc(IoBPath)] && ![file isfile $misc(IoBPath)]} {
        ErrorLog AutoNuke "invalid path to ioBanana \"$misc(IoBPath)\": the file does not exist"
        ErrorReturn "Invalid path to the ioBanana executable, check your configuration."
    }

    LinePuts "Checking [expr {[llength $anuke(Sections)] / 3}] auto-nuke sections."

    variable check
    variable nukedList  [list]
    variable warnedList [list]

    set anuke(ImdbOrder) [string tolower $anuke(ImdbOrder)]
    set anuke(MP3Order) [string tolower $anuke(MP3Order)]

    # Check variables:
    #
    # check(Settings) - Check settings         (user defined).
    # check(Multi)    - Nuke multiplier        (user defined).
    # check(WarnMins) - Minutes until warning  (user defined).
    # check(NukeMins) - Minutes until nuke     (user defined).
    # check(Cookies)  - List of reason cookies (script defined).
    # check(Reason)   - Nuke reason template   (script defined).
    # check(WarnType) - Warning log event type (script defined).
    # check(WarnData) - Warning log check data (script defined).
    #
    # Release check variables:
    #
    # release(Age)         - Age of release, in seconds.
    # release(Name)        - Release name.
    # release(RealPath)    - Release physical path.
    # release(VirtualPath) - Release virtual path.
    # release(PathList)    - Release subdirectory list.
    #
    # Disk check variables:
    #
    # disk(Age)         - Age of disk subdirectory, in seconds.
    # disk(Name)        - Name of disk subdirectory.
    # disk(RealPath)    - Disk physical path.
    # disk(VirtualPath) - Disk virtual path.
    #
    array set releaseChecks [list \
        allowed    [list ANUKEALLOWED $anuke(ReasonAllowed) {CheckAllowed $check(Settings) $release(Name)}] \
        banned     [list ANUKEBANNED  $anuke(ReasonBanned)  {CheckBanned  $check(Settings) $release(Name)}] \
        disks      [list ANUKEDISKS   $anuke(ReasonDisks)   {CheckDisks   $check(Settings) [llength $release(PathList)]}] \
        imdb       [list ANUKEIMDB    $anuke(ReasonImdb)    {CheckImdb    $check(Settings) $release(RealPath)}] \
        keyword    [list ANUKEKEYWORD $anuke(ReasonKeyword) {CheckKeyword $check(Settings) $release(Name)}] \
    ]
    array set diskChecks [list \
        empty      [list ANUKEEMPTY   $anuke(ReasonEmpty)   {CheckEmpty $disk(RealPath)}] \
        incomplete [list ANUKEINC     $anuke(ReasonInc)     {CheckInc   $disk(RealPath)}] \
        mp3        [list ANUKEMP3     $anuke(ReasonMP3)     {CheckMP3   $check(Settings) $disk(RealPath)}] \
    ]

    # Timestamp used to format date cookies.
    set timeNow [clock seconds]
    set maxAge [expr {$anuke(MaximumAge) * 60}]

    foreach {check(VirtualPath) check(DayOffset) check(SettingsList)} $anuke(Sections) {
        # Sort the check settings so the earliest nuke time is processed first.
        if {[catch {llength $check(SettingsList)} error] || \
            [catch {set check(SettingsList) [lsort -increasing -integer -index 4 $check(SettingsList)]} error]} {
            ErrorLog AutoNuke "invalid check settings for \"$check(VirtualPath)\": $error"
            continue
        }

        # Convert virtual path date cookies.
        set formatTime [expr {$timeNow + ($check(DayOffset) * 86400)}]
        set check(VirtualPath) [clock format $formatTime -format $check(VirtualPath) -gmt [IsTrue $misc(UtcTime)]]
        LinePuts ""
        LinePuts "Checking path: $check(VirtualPath) (day offset: $check(DayOffset))"
        set check(RealPath) [resolve pwd $check(VirtualPath)]
        if {![file isdirectory $check(RealPath)]} {
            LinePuts "- The directory does not exist."
            continue
        }

        foreach configLine $check(SettingsList) {
            foreach {checkType check(Settings) check(Multi) check(WarnMins) check(NukeMins)} $configLine {break}
            set checkType [string tolower $checkType]

            # Split IMDB and MP3 check settings.
            if {$checkType eq "imdb" || $checkType eq "mp3"} {
                set check(Settings) [SplitSettings $check(Settings)]
            } elseif {$checkType eq "keyword"} {
                set check(Settings) [string tolower $check(Settings)]
            }

            foreach release(RealPath) [glob -nocomplain -types d -directory $check(RealPath) "*"] {
                set release(Name) [file tail $release(RealPath)]

                # Ignore exempted, approved, and old releases.
                if {[ListMatchI $anuke(Exempts) $release(Name)] || \
                        [llength [FindTags $release(RealPath) $anuke(ApproveTag)]] || \
                        [catch {file stat $release(RealPath) stat}] || \
                        [set release(Age) [expr {[clock seconds] - $stat(ctime)}]] > $maxAge} {
                    continue
                }

                # Find release subdirectories.
                set release(PathList) [list]
                foreach diskDir [glob -nocomplain -types d -directory $release(RealPath) "*"] {
                    if {![ListMatchI $anuke(Exempts) [file tail $diskDir]] && [IsDiskPath $diskDir]} {
                        lappend release(PathList) $diskDir
                    }
                }
                # If there are no subdirectories present, check the release's root directory.
                if {![llength $release(PathList)]} {
                    lappend release(PathList) $release(RealPath)
                }

                array set check [list Cookies "" Reason "" WarnType "" WarnData ""]
                set release(VirtualPath) [file join $check(VirtualPath) $release(Name)]

                if {[info exists releaseChecks($checkType)]} {
                    foreach {check(WarnType) check(Reason) checkProc} $releaseChecks($checkType) {break}

                    if {[eval $checkProc]} {
                        NukeCheck $release(RealPath) $release(VirtualPath) $release(Age)
                    }
                } elseif {[info exists diskChecks($checkType)]} {
                    foreach {check(WarnType) check(Reason) checkProc} $diskChecks($checkType) {break}

                    # Check each release subdirectory.
                    foreach disk(RealPath) $release(PathList) {
                        if {[IsDiskPath $disk(RealPath)]} {
                            # Retrieve the subdirectory's age.
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
                    ErrorLog AutoNuke "Invalid auto-nuke type \"$checkType\" in line: \"$configLine\""
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
