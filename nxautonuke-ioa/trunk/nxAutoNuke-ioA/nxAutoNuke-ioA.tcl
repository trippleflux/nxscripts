################################################################################
# nxAutoNuke - Auto-Nuke Banned, Empty, Incomplete, IMDB, and MP3 Releases     #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxAutoNuke {
    namespace import -force ::nxLib::*
}

# Nuke Procedures
######################################################################

proc ::nxAutoNuke::GetName {VirtualPath} {
    set Release [file tail $VirtualPath]
    if {[IsDiskPath $Release]} {
        set ParentPath [file tail [file dirname $VirtualPath]]
        if {[string length $ParentPath]} {set Release "$ParentPath ($Release)"}
    }
    return $Release
}

proc ::nxAutoNuke::Nuke {RealPath VirtualPath UserName GroupName Multi Reason} {
    global misc
    # Find credit and stats section
    foreach {CreditSection StatSection} [GetCreditStatSections $VirtualPath] {break}

    # Borrowed this portion from Harm's ioAUTONUKE, since
    # these features are undocumented for ioA and ioBanana.
    if {[string length $misc(ioBPath)]} {
        set ParentVirtual [string map {/ \\} [file dirname $VirtualPath]]
        set ParentReal [string map {/ \\} [file dirname $RealPath]]
        catch {exec $misc(ioBPath) KICKNUKE NUKE [file tail $VirtualPath] $ParentVirtual $ParentReal [resolve user $UserName]}
    }

    set RealPath [string map {/ \\} $RealPath]
    if {[catch {exec $misc(IoAPath) NUKE $RealPath $VirtualPath $Multi $StatSection $CreditSection $UserName $GroupName $Reason} OutputMsg]} {
        ErrorLog AutoNuke "ioA Output:\n$OutputMsg"
        return 0
    }
    foreach OutLine [split $OutputMsg "\r\n"] {
        if {![string first "NUKE:" $OutLine]} {putlog $OutLine}
    }
    return 1
}

# Auto Nuke Checks
######################################################################

proc ::nxAutoNuke::CheckAllowed {GroupList ReleaseName} {
    variable check
    if {[set GroupPos [string last "-" $ReleaseName]] == -1} {return 0}
    set GroupName [string range $ReleaseName [incr GroupPos] end]

    foreach AllowGroup $GroupList {
        if {[string match -nocase $AllowGroup $GroupName]} {
            return 0
        }
    }
    set check(Cookies) [list %(group) $GroupName]
    set check(WarnData) "\"$GroupName\" "
    return 1
}

proc ::nxAutoNuke::CheckBanned {PatternList ReleaseName} {
    variable check
    foreach Pattern $PatternList {
        if {[string match -nocase $Pattern $ReleaseName]} {
            set check(Cookies) [list %(banned) $Pattern]
            set check(WarnData) "\"$Pattern\" "
            return 1
        }
    }
    return 0
}

proc ::nxAutoNuke::CheckDisks {DiskMax DiskCount} {
    variable check
    if {$DiskCount > $DiskMax} {
        set check(Cookies) [list %(disks) $DiskCount %(maxdisks) $DiskMax]
        set check(WarnData) "\"$DiskCount\" \"$DiskMax\" "
        return 1
    }
    return 0
}

proc ::nxAutoNuke::CheckEmpty {RealPath} {
    foreach FileName [glob -nocomplain -types f -directory $RealPath "*"] {
        if {![string equal -nocase -length 7 ".ioFTPD" [file tail $FileName]]} {
            return 0
        }
    }
    return 1
}

proc ::nxAutoNuke::CheckInc {RealPath} {
    global anuke
    foreach ListItem [glob -nocomplain -types f -directory $RealPath "*"] {
        set FileName [file tail $ListItem]
        if {[string match $anuke(BadExt) $FileName] || [string match $anuke(MissingExt) $FileName]} {
            return 1
        }
    }
    return 0
}

proc ::nxAutoNuke::CheckImdb {CheckList RealPath} {
    global anuke
    variable check

    set FoundMatch 0
    set genre ""; set rating ""; set year ""
    foreach TagName [FindTags $RealPath] {
        set ReMatch [regexp -inline -nocase -- $anuke(ImdbMatch) [file tail $TagName]]
        if {[llength $ReMatch]} {
            foreach $anuke(ImdbOrder) [lrange $ReMatch 1 end] {break}
            set FoundMatch 1; break
        }
    }
    if {!$FoundMatch} {return 0}

    foreach {Type Value} $CheckList {
        set Nuke 0
        switch -- $Type {
            {genre} {
                if {[string match -nocase $Value $genre]} {set Nuke 1}
            }
            {rating} {
                if {[string is double -strict $rating] && $rating < $Value} {set Nuke 1}
            }
            {year} {
                if {![string match -nocase $Value $year]} {set Nuke 1}
            }
        }

        if {$Nuke} {
            set Value [set $Type]
            set check(Cookies) [list %(type) $Type %(banned) $Value]
            set check(WarnData) "\"$Type\" \"$Value\" "
            return 1
        }
    }
    return 0
}

proc ::nxAutoNuke::CheckKeyword {KeywordList ReleaseName} {
    variable check
    if {[set GroupPos [string last "-" $ReleaseName]] != -1} {
        set ReleaseName [string range $ReleaseName 0 [incr GroupPos -1]]
    }
    set ReleaseSplit [string tolower [split $ReleaseName "()-._"]]

    foreach Keyword $KeywordList {
        if {[lsearch -exact $ReleaseSplit $Keyword] != -1} {
            set check(Cookies) [list %(banned) $Keyword]
            set check(WarnData) "\"$Keyword\" "
            return 1
        }
    }
	return 0
}

proc ::nxAutoNuke::CheckMP3 {CheckList RealPath} {
    global anuke
    variable check

    set FoundMatch 0
    set genre ""; set year ""
    foreach TagName [FindTags $RealPath] {
        set ReMatch [regexp -inline -nocase -- $anuke(MP3Match) [file tail $TagName]]
        if {[llength $ReMatch]} {
            foreach $anuke(MP3Order) [lrange $ReMatch 1 end] {break}
            set FoundMatch 1; break
        }
    }
    if {!$FoundMatch} {return 0}

    foreach {Type Value} $CheckList {
        set Nuke 0
        switch -- $Type {
            {genre} {
                if {[string match -nocase $Value $genre]} {set Nuke 1}
            }
            {year} {
                if {![string match -nocase $Value $year]} {set Nuke 1}
            }
        }

        if {$Nuke} {
            set Value [set $Type]
            set check(Cookies) [list %(type) $Type %(banned) $Value]
            set check(WarnData) "\"$Type\" \"$Value\" "
            return 1
        }
    }
    return 0
}

# Auto Nuke Procedures
######################################################################

proc ::nxAutoNuke::GetUserList {RealPath} {
    global anuke
    GetDirList $RealPath dirlist ".ioFTPD*"
    foreach ListItem $dirlist(FileList) {
        if {[file size $ListItem] < 1} {continue}
        catch {vfs read $ListItem} VfsOwner
        ListAssign $VfsOwner UserId GroupId
        if {[set UserName [resolve uid $UserId]] != "" && ![info exists uploader($UserName)]} {
            set uploader($UserName) [GetGroupName $GroupId]
        }
    }
    # Check if the release is an empty nuke
    if {![array exists uploader]} {
        catch {vfs read $RealPath} VfsOwner
        ListAssign $VfsOwner UserId GroupId
        if {[set UserName [resolve uid $UserId]] != ""} {
            set uploader($UserName) [GetGroupName $GroupId]
        } else {return ""}
    }
    # Format uploaders list
    set FormatList ""
    foreach {UserName GroupName} [array get uploader] {
        set ReMap [list %b \002 %c \003 %u \031 %(user) $UserName %(group) $GroupName]
        lappend FormatList [string map $ReMap $anuke(UserFormat)]
    }
    return [join [lsort -ascii $FormatList] $anuke(UserSplit)]
}

proc ::nxAutoNuke::FindTags {RealPath {TagTemplate "*"}} {
    set TagTemplate [string map {\[ \\\[ \] \\\] \{ \\\{ \} \\\}} $TagTemplate]
    return [glob -nocomplain -types d -directory $RealPath $TagTemplate]
}

proc ::nxAutoNuke::SplitSettings {CheckSettings} {
    set CheckList ""
    foreach ListItem $CheckSettings {
        foreach {Name Value} [split $ListItem ":"] {break}
        lappend CheckList [string tolower $Name] $Value
    }
    return $CheckList
}

proc ::nxAutoNuke::NukeCheck {RealPath VirtualPath DirAge} {
    global anuke
    variable check
    variable NukedList
    variable WarnedList

    # Skip the release if it was already nuked.
    set CheckPath [string tolower $RealPath]
    if {[lsearch -exact $NukedList $CheckPath] != -1} {return}

    set NukeSecs [expr {$check(NukeMins) * 60}]
    set WarnSecs [expr {$check(WarnMins) * 60}]

    if {$DirAge >= $NukeSecs} {
        # Nuke the release
        lappend check(Cookies) %(age) [expr {$DirAge / 60}]
        set check(Reason) [StripChars [string map $check(Cookies) $check(Reason)]]

        # Nuke the entire release if anuke(SubDirs) is false.
        if {![IsTrue $anuke(SubDirs)] && [IsDiskPath $VirtualPath]} {
            set RealPath [file dirname $RealPath]
            set VirtualPath [file dirname $VirtualPath]
        }

        LinePuts "- Nuking: [GetName $VirtualPath] - $check(Reason)"
        if {![Nuke $RealPath $VirtualPath $anuke(UserName) $anuke(GroupName) $check(Multi) $check(Reason)]} {
            LinePuts "- Unable to nuke the the release, check nxError.log for details."
        }
        lappend NukedList $CheckPath
    } elseif {$DirAge >= $WarnSecs && [lsearch -exact $WarnedList $CheckPath] == -1} {
        # Obtain a list of nuked users
        if {[IsTrue $anuke(WarnUsers)]} {
            set UserList [GetUserList $RealPath]
        } else {
            set UserList "Disabled"
        }

        # Log the warning
        set DirAge [expr {$DirAge / 60}]
        lappend check(Cookies) %(age) $DirAge
        set check(Reason) [StripChars [string map $check(Cookies) $check(Reason)]]

        LinePuts "- Warning: [GetName $VirtualPath] - $check(Reason)"
        putlog "$check(WarnType): \"$VirtualPath\" $check(WarnData)\"$DirAge\" \"[expr {$check(NukeMins) - $DirAge}]\" \"$check(NukeMins)\" \"$check(Multi)\" \"$UserList\""
        lappend WarnedList $CheckPath
    }
    return
}

# AutoNuke Main
######################################################################

proc ::nxAutoNuke::Main {} {
    global anuke misc user group
    # A userfile and VFS file will have to be opened so that resolve works under ioFTPD's scheduler
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
    variable NukedList ""
    variable WarnedList ""

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
    array set ReleaseChecks [list \
        allowed    [list ANUKEALLOWED $anuke(ReasonAllowed) {CheckAllowed $check(Settings) $release(Name)}] \
        banned     [list ANUKEBANNED  $anuke(ReasonBanned)  {CheckBanned  $check(Settings) $release(Name)}] \
        disks      [list ANUKEDISKS   $anuke(ReasonDisks)   {CheckDisks   $check(Settings) [llength $release(PathList)]}] \
        imdb       [list ANUKEIMDB    $anuke(ReasonImdb)    {CheckImdb    $check(Settings) $release(RealPath)}] \
        keyword    [list ANUKEKEYWORD $anuke(ReasonKeyword) {CheckKeyword $check(Settings) $release(Name)}] \
    ]
    array set DiskChecks [list \
        empty      [list ANUKEEMPTY   $anuke(ReasonEmpty)   {CheckEmpty $disk(RealPath)}] \
        incomplete [list ANUKEINC     $anuke(ReasonInc)     {CheckInc   $disk(RealPath)}] \
        mp3        [list ANUKEMP3     $anuke(ReasonMP3)     {CheckMP3   $check(Settings) $disk(RealPath)}] \
    ]

    # Timestamp used to format date cookies
    set TimeNow [clock seconds]
    set MaxAge [expr {$anuke(MaximumAge) * 60}]

    foreach {check(VirtualPath) check(DayOffset) check(SettingsList)} $anuke(Sections) {
        # Sort the check settings so the earliest nuke time is processed first.
        if {[catch {llength $check(SettingsList)} error] || \
        [catch {set check(SettingsList) [lsort -increasing -integer -index 4 $check(SettingsList)]} error]} {
            ErrorLog AutoNuke "invalid check settings for \"$VirtualPath\": $error"
            continue
        }

        # Convert virtual path date cookies.
        set FormatTime [expr {$TimeNow + ($check(DayOffset) * 86400)}]
        set check(VirtualPath) [clock format $FormatTime -format $check(VirtualPath) -gmt [IsTrue $misc(UtcTime)]]
        LinePuts ""
        LinePuts "Checking path: $check(VirtualPath) (day offset: $check(DayOffset))"
        set check(RealPath) [resolve pwd $check(VirtualPath)]
        if {![file isdirectory $check(RealPath)]} {
            LinePuts "- The directory does not exist."
            continue
        }

        foreach ConfigLine $check(SettingsList) {
            foreach {CheckType check(Settings) check(Multi) check(WarnMins) check(NukeMins)} $ConfigLine {break}
            set CheckType [string tolower $CheckType]

            # Split IMDB and MP3 check settings.
            if {$CheckType eq "imdb" || $CheckType eq "mp3"} {
                set check(Settings) [SplitSettings $check(Settings)]
            } elseif {$CheckType eq "keyword"} {
                set check(Settings) [string tolower $check(Settings)]
            }

            foreach release(RealPath) [glob -nocomplain -types d -directory $check(RealPath) "*"] {
                set release(Name) [file tail $release(RealPath)]

                # Ignore exempted, approved, and old releases.
                if {[ListMatchI $anuke(Exempts) $release(Name)] || [llength [FindTags $release(RealPath) $anuke(ApproveTag)]] || \
                [catch {file stat $release(RealPath) stat}] || [set release(Age) [expr {[clock seconds] - $stat(ctime)}]] > $MaxAge} {
                    continue
                }

                # Find release subdirectories.
                set release(PathList) ""
                foreach DiskDir [glob -nocomplain -types d -directory $release(RealPath) "*"] {
                    if {![ListMatchI $anuke(Exempts) [file tail $DiskDir]] && [IsDiskPath $DiskDir]} {
                        lappend release(PathList) $DiskDir
                    }
                }
                # If there are no subdirectories present, check the release's root directory.
                if {![llength $release(PathList)]} {
                    lappend release(PathList) $release(RealPath)
                }

                array set check [list Cookies "" Reason "" WarnType "" WarnData ""]
                set release(VirtualPath) [file join $check(VirtualPath) $release(Name)]

                if {[info exists ReleaseChecks($CheckType)]} {
                    foreach {check(WarnType) check(Reason) CheckProc} $ReleaseChecks($CheckType) {break}

                    if {[eval $CheckProc]} {
                        NukeCheck $release(RealPath) $release(VirtualPath) $release(Age)
                    }
                } elseif {[info exists DiskChecks($CheckType)]} {
                    foreach {check(WarnType) check(Reason) CheckProc} $DiskChecks($CheckType) {break}

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
                        if {[eval $CheckProc]} {
                            NukeCheck $disk(RealPath) $disk(VirtualPath) $disk(Age)
                        }
                    }
                    set CheckProc $DiskChecks($CheckType)
                } else {
                    ErrorLog AutoNuke "Invalid auto-nuke type \"$CheckType\" in line: \"$ConfigLine\""
                    break
                }
            }
        }
    }
    unset -nocomplain check NukedList WarnedList

    iputs "'------------------------------------------------------------------------'"
    return 0
}

::nxAutoNuke::Main
