################################################################################
# nxAutoNuke - Auto-Nuke Banned, Empty, Incomplete, IMDB, and MP3 Releases     #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[catch {source "../scripts/init.itcl"} ErrorMsg]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $ErrorMsg
}

namespace eval ::nxAutoNuke {
    namespace import -force ::nxLib::*
}

# Nuke Procedures
######################################################################

proc ::nxAutoNuke::GetName {VirtualPath} {
    set Release [file tail $VirtualPath]
    if {[IsMultiDisk $Release]} {
        set ParentPath [file tail [file dirname $VirtualPath]]
        if {![string equal "" $ParentPath]} {set Release "$ParentPath ($Release)"}
    }
    return $Release
}

proc ::nxAutoNuke::UpdateRecord {RealPath {Buffer ""}} {
    set Record ""
    set RealPath [file join $RealPath ".ioFTPD.nxNuke"]
    set OpenMode [expr {$Buffer != "" ? "w" : "r"}]

    ## Tcl can't open hidden files, quite lame.
    catch {file attributes $RealPath -hidden 0}
    if {[catch {set Handle [open $RealPath $OpenMode]} ErrorMsg]} {
        ErrorLog NukeRecord $ErrorMsg
    } elseif {[string equal "" $Buffer]} {
        set Record [read $Handle]
        close $Handle
    } else {
        puts $Handle $Buffer
        close $Handle
    }
    ## Set the file's attributes to hidden
    catch {file attributes $RealPath -hidden 1}
    return [string trim $Record]
}

proc ::nxAutoNuke::UpdateUser {UserName Multi Size Files Stats CreditSection StatSection} {
    set CreditSection [expr {$CreditSection + 1}]
    set StatSection [expr {$StatSection * 3 + 1}]
    set GroupName "NoGroup"
    set NewUserFile ""

    if {[userfile open $UserName] == 0} {
        userfile lock
        set UserFile [split [userfile bin2ascii] "\r\n"]
        foreach UserLine $UserFile {
            set LineType [string tolower [lindex $UserLine 0]]
            if {[string equal "credits" $LineType]} {
                set OldCredits [lindex $UserLine $CreditSection]
            } elseif {[string equal "groups" $LineType]} {
                set GroupName [GetGroupName [lindex $UserLine 1]]
            } elseif {[string equal "ratio" $LineType]} {
                set Ratio [lindex $UserLine $CreditSection]
            }
        }

        ## Change credits if the user is not leech
        if {$Ratio != 0} {
            set DiffCredits [expr {(wide($Size) * $Ratio) + (wide($Size) * ($Multi - 1))}]
            set NewCredits [expr {wide($OldCredits) - wide($DiffCredits)}]
        } else {
            set DiffCredits 0
            set NewCredits $OldCredits
        }
        foreach UserLine $UserFile {
            set LineType [string tolower [lindex $UserLine 0]]
            if {[lsearch -exact {allup dayup monthup wkup} $LineType] != -1} {
                set NewFiles [expr {wide([lindex $UserLine $StatSection]) - $Files}]
                set NewStats [expr {wide([lindex $UserLine [expr {$StatSection + 1}]]) - wide($Stats)}]
                set UserLine [lreplace $UserLine $StatSection [expr {$StatSection + 1}] $NewFiles $NewStats]
            } elseif {[string equal "credits" $LineType]} {
                set UserLine [lreplace $UserLine $CreditSection $CreditSection $NewCredits]
            }
            append NewUserFile $UserLine "\r\n"
        }
        userfile ascii2bin $NewUserFile
        userfile unlock
    }
    return [list $GroupName $Ratio $OldCredits $NewCredits $DiffCredits]
}

proc ::nxAutoNuke::Nuke {RealPath VirtualPath NukerUser NukerGroup Multi Reason} {
    global misc nuke
    if {![file isdirectory $RealPath]} {
        ErrorLog AutoNuke "unable to nuke \"$VirtualPath\": directory does not exist."
        return 0
    } elseif {![string is digit -strict $Multi]} {
        ErrorLog AutoNuke "unable to nuke \"$VirtualPath\": invalid multiplier value ($Multi)."
        return 0
    }

    ## Check if there is an available nuke record.
    set NukeId ""
    set Record [UpdateRecord $RealPath]
    set RecordSplit [split $Record "|"]

    ## Record versions:
    ## v1: nuke-type|user|group|multi|reason
    ## v2: 2|status|id|user|group|multi|reason
    switch -regexp -- $Record {
        {^2\|(0|1)\|\d+\|\S+\|\d+\|.+$} {
            set NukeId [lindex $RecordSplit 2]
        }
        {^(NUKE|UNNUKE)\|\S+\|\S+\|\d+\|.+$} {}
        {} {}
        default {
            ErrorLog AutoNuke "invalid nuke record for \"$RealPath\": $Record"
        }
    }
    set DiskCount 0; set Files 0; set TotalSize 0
    set NukeTime [clock seconds]
    set Release [GetName $VirtualPath]
    set ParentPath [file dirname $RealPath]
    ListAssign [GetCreditStatSections $VirtualPath] CreditSection StatSection

    ## Count CDs/Discs/DVDs
    foreach ListItem [glob -nocomplain -types d -directory $RealPath "*"] {
        if {[IsMultiDisk $ListItem]} {incr DiskCount}
    }

    ## Count files and total size
    GetDirList $RealPath dirlist ".ioFTPD*"
    foreach ListItem $dirlist(FileList) {
        incr Files; set FileSize [file size $ListItem]
        set TotalSize [expr {wide($TotalSize) + wide($FileSize)}]
        catch {lindex [vfs read $ListItem] 0} UserId
        if {[set NukeeUser [resolve uid $UserId]] != ""} {
            ## Increase file Count
            if {[info exists nukefiles($NukeeUser)]} {
                incr nukefiles($NukeeUser)
            } else {set nukefiles($NukeeUser) 1}

            ## Add total size
            if {[info exists nukesize($NukeeUser)]} {
                set nukesize($NukeeUser) [expr {wide($nukesize($NukeeUser)) + wide($FileSize)}]
            } else {set nukesize($NukeeUser) $FileSize}
        }
    }
    set TotalSize [expr {wide($TotalSize) / 1024}]

    ## Check if Release is an empty nuke (less then 5KB)
    if {$TotalSize < 5 || ![array exists nukesize]} {
        unset -nocomplain nukefiles nukesize
        set EmptyNuke 1
        catch {lindex [vfs read $RealPath] 0} UserId
        if {[set NukeeUser [resolve uid $UserId]] != ""} {
            set nukefiles($NukeeUser) 0
            set nukesize($NukeeUser) [expr {wide($nuke(EmptyNuke)) * 1024 * 1024}]
        } else {
            ErrorLog AutoNuke "unable to nuke \"$VirtualPath\": could not find directory owner."
            return 0
        }
    } else {set EmptyNuke 0}

    ## Change the credits and stats of nukees
    set NukeeLog ""
    foreach NukeeUser [lsort -ascii [array names nukesize]] {
        set NukeCredits [expr {wide($nukesize($NukeeUser)) / 1024}]
        set NukeStats [expr {$EmptyNuke ? 0 : $NukeCredits}]
        set Result [UpdateUser $NukeeUser $Multi $NukeCredits $nukefiles($NukeeUser) $NukeStats $CreditSection $StatSection]
        foreach {NukeeGroup Ratio OldCredits NewCredits DiffCredits} $Result {break}
        lappend NukeeLog [list $NukeeUser $NukeeGroup $NukeStats]
    }
    ## Join the list twice because of the sublist used in "lsort -index".
    set NukeeLog [join [join [lsort -decreasing -integer -index 2 $NukeeLog]]]

    ## Create nuke tag
    set ReMap [list %(user) $NukerUser %(group) $NukerGroup %(multi) $Multi %(reason) $Reason]
    set NukeTag [file join $RealPath [string map $ReMap $nuke(InfoTag)]]
    CreateTag $NukeTag [resolve user $NukerUser] [resolve group $NukerGroup] 555
    RemoveParentLinks $RealPath $VirtualPath

    ## Rename nuke directory
    set NewName "$nuke(Prefix)[file tail $VirtualPath]"
    set NewPath [file join $ParentPath $NewName]
    if {![string equal -nocase $RealPath $NewPath]} {
        ## In order to prevent users from re-entering the
        ## directory while nuking, it will be chmodded to 000.
        catch {vfs read $RealPath} VfsOwner
        ListAssign $VfsOwner UserId GroupId
        if {![string is digit -strict $UserId]} {set UserId [lindex $misc(DirOwner) 0]}
        if {![string is digit -strict $GroupId]} {set GroupId [lindex $misc(DirOwner) 1]}
        catch {vfs write $RealPath $UserId $GroupId 000}

        KickUsers [file join $VirtualPath "*"]
        if {[catch {file rename -force -- $RealPath $NewPath} ErrorMsg]} {
            set RenameFail 1
            ErrorLog AutoNukeRename $ErrorMsg
        } else {
            set RenameFail 0
        }
    }

    ## Chmod directories
    GetDirList $NewPath dirlist ".ioFTPD*"
    foreach ListItem $dirlist(DirList) {
        catch {vfs read $ListItem} VfsOwner
        ListAssign $VfsOwner UserId GroupId
        if {![string is digit -strict $UserId]} {set UserId [lindex $misc(DirOwner) 0]}
        if {![string is digit -strict $GroupId]} {set GroupId [lindex $misc(DirOwner) 1]}
        catch {vfs write $ListItem $UserId $GroupId 555}
    }
    catch {vfs flush $ParentPath}
    if {[IsTrue $misc(dZSbotLogging)]} {
        foreach {NukeeUser NukeeGroup Amount} $NukeeLog {
            set Amount [format "%.2f" [expr {double($Amount) / 1024.0}]]
            putlog "NUKE: \"$VirtualPath\" \"$NukerUser@$NukerGroup\" \"$NukeeUser@$NukeeGroup\" \"$Multi $Amount\" \"$Reason\""
        }
    } else {
        putlog "NUKE: \"$VirtualPath\" \"$NukerUser\" \"$NukerGroup\" \"$Multi\" \"$Reason\" \"$Files\" \"$TotalSize\" \"$DiskCount\" \"$NukeeLog\""
    }

    if {![catch {DbOpenFile NukeDb "Nukes.db"} ErrorMsg]} {
        ## In order to pass a NULL value to TclSQLite, the variable must be unset.
        if {![string is digit -strict $NukeId]} {unset NukeId}
        NukeDb eval {INSERT OR REPLACE INTO Nukes (NukeId,TimeStamp,UserName,GroupName,Status,Release,Reason,Multi,Files,Size) VALUES($NukeId,$NukeTime,$NukerUser,$NukerGroup,0,$Release,$Reason,$Multi,$Files,$TotalSize)}
        set NukeId [NukeDb last_insert_rowid]

        NukeDb eval {BEGIN}
        foreach {NukeeUser NukeeGroup Amount} $NukeeLog {
            NukeDb eval {INSERT OR REPLACE INTO Users (NukeId,Status,UserName,GroupName,Amount) VALUES($NukeId,0,$NukeeUser,$NukeeGroup,$Amount)}
        }
        NukeDb eval {COMMIT}

        NukeDb close
    } else {ErrorLog NukeDb $ErrorMsg}

    ## Save the nuke ID and multiplier for later use (ie. unnuke).
    UpdateRecord [expr {$RenameFail ? $RealPath : $NewPath}] "2|0|$NukeId|$NukerUser|$NukerGroup|$Multi|$Reason"
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
            default {continue}
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
            default {continue}
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
    ## Check if the release is an empty nuke
    if {[array names uploader] == ""} {
        catch {vfs read $RealPath} VfsOwner
        ListAssign $VfsOwner UserId GroupId
        if {[set UserName [resolve uid $UserId]] != ""} {
            set uploader($UserName) [GetGroupName $GroupId]
        } else {return ""}
    }
    ## Format uploaders list
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
    global anuke misc
    variable check
    variable NukedList
    variable WarnedList

    ## Skip the release if it was already nuked.
    set CheckPath [string tolower $RealPath]
    if {[lsearch -exact $NukedList $CheckPath] != -1} {return}

    set NukeSecs [expr {$check(NukeMins) * 60}]
    set WarnSecs [expr {$check(WarnMins) * 60}]

    if {$DirAge >= $NukeSecs} {
        ## Nuke the release
        lappend check(Cookies) %(age) [expr {$DirAge / 60}]
        set check(Reason) [StripChars [string map $check(Cookies) $check(Reason)]]

        ## Nuke the entire release if anuke(SubDirs) is false.
        if {![IsTrue $anuke(SubDirs)] && [IsMultiDisk $VirtualPath]} {
            set RealPath [file dirname $RealPath]
            set VirtualPath [file dirname $VirtualPath]
        }

        LinePuts "- Nuking: [GetName $VirtualPath] - $check(Reason)"
        if {![Nuke $RealPath $VirtualPath $anuke(UserName) $anuke(GroupName) $check(Multi) $check(Reason)]} {
            LinePuts "- Unable to nuke the the release, check nxError.log for details."
        }
        lappend NukedList $CheckPath
    } elseif {$DirAge >= $WarnSecs && [lsearch -exact $WarnedList $CheckPath] == -1} {
        ## Obtain a list of nuked users
        if {[IsTrue $anuke(WarnUsers)]} {
            set UserList [GetUserList $RealPath]
        } else {
            set UserList "Disabled"
        }

        ## Log the warning
        lappend check(Cookies) %(age) [expr {$DirAge / 60}]
        set check(Reason) [StripChars [string map $check(Cookies) $check(Reason)]]

        LinePuts "- Warning: [GetName $VirtualPath] - $check(Reason)"
        if {[IsTrue $misc(dZSbotLogging)]} {
            set DirAge [expr {$DirAge / 60}]
            putlog "$check(WarnType): \"$VirtualPath\" $check(WarnData)\"$DirAge\" \"[expr {$NukeSecs - $DirAge}]\" \"$NukeSecs\" \"$check(Multi)\" \"$UserList\""
        } else {
            putlog "$check(WarnType): \"$VirtualPath\" $check(WarnData)\"$DirAge\" \"[expr {$NukeSecs - $DirAge}]\" \"$NukeSecs\" \"$check(Multi)\" \"$UserList\""
        }
        lappend WarnedList $CheckPath
    }
    return
}

# AutoNuke Main
######################################################################

proc ::nxAutoNuke::Main {} {
    global anuke misc user group
    ## A userfile and VFS file will have to be opened so that resolve works under ioFTPD's scheduler
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

    variable check
    variable NukedList ""
    variable WarnedList ""

    set anuke(ImdbOrder) [string tolower $anuke(ImdbOrder)]
    set anuke(MP3Order) [string tolower $anuke(MP3Order)]

    ## Check variables:
    ##
    ## check(Settings) - Check settings         (user defined)
    ## check(Multi)    - Nuke multiplier        (user defined)
    ## check(WarnMins) - Minutes until warning  (user defined)
    ## check(NukeMins) - Minutes until nuke     (user defined)
    ## check(Cookies)  - List of reason cookies (script defined)
    ## check(Reason)   - Nuke reason template   (script defined)
    ## check(WarnType) - Warning log event type (script defined)
    ## check(WarnData) - Warning log check data (script defined)
    ##
    ## Release check variables:
    ##
    ## release(Age)         - Age of release, in seconds.
    ## release(Name)        - Release name.
    ## release(RealPath)    - Release physical path.
    ## release(VirtualPath) - Release virtual path.
    ## release(PathList)    - Release sub-directory list.
    ##
    ## Disk check variables:
    ##
    ## disk(Age)         - Age of disk sub-directory, in seconds.
    ## disk(Name)        - Name of disk sub-directory.
    ## disk(RealPath)    - Disk physical path.
    ## disk(VirtualPath) - Disk virtual path.
    ##
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

    ## Timestamp used to format date cookies
    set TimeNow [clock seconds]
    set MaxAge [expr {$anuke(MaximumAge) * 60}]

    foreach {check(VirtualPath) check(DayOffset) check(SettingsList)} $anuke(Sections) {
        ## Sort the check settings so the earliest nuke time is processed first.
        if {[catch {llength $check(SettingsList)} ErrorMsg] || \
        [catch {set check(SettingsList) [lsort -increasing -integer -index 4 $check(SettingsList)]} ErrorMsg]} {
            ErrorLog AutoNuke "invalid check settings for \"$VirtualPath\": $ErrorMsg"
            continue
        }

        ## Convert virtual path date cookies.
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

            ## Split IMDB and MP3 check settings.
            if {[lsearch -exact {imdb mp3} $CheckType] != -1} {
                set check(Settings) [SplitSettings $check(Settings)]
            } elseif {[string equal "keyword" $CheckType]} {
                set check(Settings) [string tolower $check(Settings)]
            }

            foreach release(RealPath) [glob -nocomplain -types d -directory $check(RealPath) "*"] {
                set release(Name) [file tail $release(RealPath)]

                ## Ignore exempted, approved, and old releases.
                if {[ListMatchI $anuke(Exempts) $release(Name)] || [llength [FindTags $release(RealPath) $anuke(ApproveTag)]] || \
                [catch {file stat $release(RealPath) stat}] || [set release(Age) [expr {[clock seconds] - $stat(ctime)}]] > $MaxAge} {
                    continue
                }

                ## Find release sub-directories.
                set release(PathList) ""
                foreach DiskDir [glob -nocomplain -types d -directory $release(RealPath) "*"] {
                    if {![ListMatchI $anuke(Exempts) [file tail $DiskDir]] && [IsMultiDisk $DiskDir]} {
                        lappend release(PathList) $DiskDir
                    }
                }
                ## If there are no sub-directories present, check the release's root directory.
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

                    ## Check each release sub-directory.
                    foreach disk(RealPath) $release(PathList) {
                        if {[IsMultiDisk $disk(RealPath)]} {
                            ## Retrieve the age of the sub-directory.
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
