################################################################################
# nxTools - Nuke Script                                                        #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} ErrorMsg]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $ErrorMsg
}

namespace eval ::nxTools::Nuke {
    namespace import -force ::nxLib::*
}

# Nuke Procedures
######################################################################

proc ::nxTools::Nuke::FindTags {RealPath TagFormat} {
    regsub -all {%\(\w+\)} $TagFormat {*} TagFormat
    set TagFormat [string map {\[ \\\[ \] \\\] \{ \\\{ \} \\\}} $TagFormat]
    return [glob -nocomplain -types d -directory $RealPath $TagFormat]
}

proc ::nxTools::Nuke::GetName {VirtualPath} {
    set Release [file tail [TrimTag $VirtualPath]]
    if {[IsMultiDisk $Release]} {
        set ParentPath [file tail [file dirname $VirtualPath]]
        if {![string equal "" $ParentPath]} {set Release "$ParentPath ($Release)"}
    }
    return $Release
}

proc ::nxTools::Nuke::TrimTag {VirtualPath} {
    global nuke
    set ParentPath [file dirname $VirtualPath]
    set Release [file tail $VirtualPath]
    if {![string first $nuke(Prefix) $Release]} {
        set Release [string range $Release [string length $nuke(Prefix)] end]
    }
    return [file join $ParentPath $Release]
}

proc ::nxTools::Nuke::UpdateRecord {RealPath {Buffer ""}} {
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

proc ::nxTools::Nuke::UpdateUser {IsNuke UserName Multi Size Files Stats CreditSection StatSection} {
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
            set NewCredits [expr {wide($OldCredits) + ($IsNuke ? -wide($DiffCredits) : wide($DiffCredits))}]
        } else {
            set DiffCredits 0
            set NewCredits $OldCredits
        }
        if {$IsNuke} {
            set Files [expr {-wide($Files)}]
            set Size [expr {-wide($Size)}]
            set Stats [expr {-wide($Stats)}]
        }
        foreach UserLine $UserFile {
            set LineType [string tolower [lindex $UserLine 0]]
            if {[lsearch -exact {allup dayup monthup wkup} $LineType] != -1} {
                set NewFiles [expr {wide([lindex $UserLine $StatSection]) + $Files}]
                set NewStats [expr {wide([lindex $UserLine [expr {$StatSection + 1}]]) + wide($Stats)}]
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

# Nuke Main
######################################################################

proc ::nxTools::Nuke::Main {ArgV} {
    global approve misc nuke flags gid group groups pwd uid user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}

    ## Safe argument handling
    set ArgList [ArgList $ArgV]
    set Action [string tolower [lindex $ArgList 0]]
    switch -- $Action {
        {nuke} - {unnuke} {
            if {[string equal "nuke" $Action]} {
                iputs ".-\[Nuke\]-----------------------------------------------------------------."
                foreach {Tmp Target Multi Reason} $ArgV {break}
                if {[llength $ArgList] < 4 || ![string is digit -strict $Multi]} {
                    ErrorReturn "Syntax: SITE NUKE <directory> <multiplier> <reason>"
                }
                if {$Multi > $nuke(MultiMax)} {
                    ErrorReturn "The specified multiplier is to large, the max is $nuke(MultiMax)\x."
                }
                set IsNuke 1
            } elseif {[string equal "unnuke" $Action]} {
                iputs ".-\[UnNuke\]---------------------------------------------------------------."
                foreach {Tmp Target Reason} $ArgV {break}
                if {[llength $ArgList] < 3} {ErrorReturn "Syntax: SITE UNNUKE <directory> <reason>"}
                set IsNuke 0
            }
            set VirtualPath [GetPath $pwd $Target]
            set RealPath [resolve pwd $VirtualPath]
            if {![file isdirectory $RealPath]} {
                ErrorReturn "The specified directory does not exist."
            }
            set MatchPath [string range $VirtualPath 0 [string last "/" $VirtualPath]]
            if {[ListMatch $nuke(NoPaths) $MatchPath]} {
                ErrorReturn "Not allowed to nuke from here."
            }
            if {$IsNuke && [llength [FindTags $RealPath $approve(DirTag)]]} {
                ErrorReturn "Approved releases cannot be nuked."
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
                    ## Only use the multiplier if it's a nuke record.
                    if {!$IsNuke && ![lindex $RecordSplit 1]} {
                        set Multi [lindex $RecordSplit 5]
                    }
                }
                {^(NUKE|UNNUKE)\|\S+\|\S+\|\d+\|.+$} {
                    ## Only use the multiplier if it's a nuke record.
                    if {!$IsNuke && [string equal "UNNUKE" [lindex $RecordSplit 0]]} {
                        set Multi [lindex $RecordSplit 3]
                    }
                }
                {} {}
                default {
                    ErrorLog NukeRecord "invalid nuke record for \"$RealPath\": $Record"
                }
            }
            if {!$IsNuke && ![info exists Multi]} {
                ErrorReturn "Unable to find the nuke record."
            }

            set NukeType 0
            if {[string first $nuke(GroupFlag) $flags] != -1} {
                ## Find the group suffix in the release name (Something-GRP)
                if {[set GroupPos [string last "-" [file tail $VirtualPath]]] == -1} {
                    ErrorReturn "Unable to verify the release's group, is it suffixed with \"-<group>\"?"
                }
                set AllowNuke 0; set NukeType 1
                set RlsSuffix [string range [file tail $VirtualPath] [incr GroupPos] end]
                foreach NukeeGroup $groups {
                    if {[string equal $RlsSuffix $NukeeGroup]} {set AllowNuke 1; break}
                }
                if {!$AllowNuke} {ErrorReturn "You are not allowed to nuke other groups."}
            }
            set DiskCount 0; set Files 0; set TotalSize 0
            set NukeTime [clock seconds]
            set Reason [StripChars $Reason]
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
                set NukeType 2
                unset -nocomplain nukefiles nukesize
                catch {lindex [vfs read $RealPath] 0} UserId
                if {[set NukeeUser [resolve uid $UserId]] != ""} {
                    set nukefiles($NukeeUser) 0
                    set nukesize($NukeeUser) [expr {wide($nuke(EmptyNuke)) * 1024 * 1024}]
                } else {ErrorReturn "Unable to find the directory owner."}
            }
            LinePuts "Release : $Release"
            LinePuts "Multi   : ${Multi}x"
            LinePuts "Reason  : $Reason"
            if {$NukeType} {
                ## 0=Normal, 1=Group, 2=Empty
                switch -- $NukeType {
                    2 {set TypeMsg "Empty"}
                    1 {set TypeMsg "Group"}
                    default {set TypeMsg "Unknown"}
                }
                LinePuts "Type    : $TypeMsg Nuke"
            }
            LinePuts "Files   : [format %-16s ${Files}F] Size: [format %-16s [FormatSize $TotalSize]] CDs: $DiskCount"
            if {$IsNuke} {
                iputs "|------------------------------------------------------------------------|"
                iputs "|    User    |   Group    |    Ratio    |  Amount Lost  |  Credits Lost  |"
                iputs "|------------------------------------------------------------------------|"
            } else {
                iputs "|------------------------------------------------------------------------|"
                iputs "|    User    |   Group    |    Ratio    | Amount Gained | Credits Gained |"
                iputs "|------------------------------------------------------------------------|"
            }

            ## Change the credits and stats of nukees
            set NukeeLog ""
            foreach NukeeUser [lsort -ascii [array names nukesize]] {
                set NukeCredits [expr {wide($nukesize($NukeeUser)) / 1024}]
                set NukeStats [expr {$NukeType == 2 ? 0 : $NukeCredits}]
                set Result [UpdateUser $IsNuke $NukeeUser $Multi $NukeCredits $nukefiles($NukeeUser) $NukeStats $CreditSection $StatSection]
                foreach {NukeeGroup Ratio OldCredits NewCredits DiffCredits} $Result {break}

                set Ratio [expr {$Ratio != 0 ? "1:$Ratio" : "Unlimited"}]
                iputs [format "| %-10s | %-10s | %11s | %13s | %14s |" $NukeeUser $NukeeGroup $Ratio [FormatSize $NukeStats] [FormatSize $DiffCredits]]
                lappend NukeeLog [list $NukeeUser $NukeeGroup $NukeStats]
            }
            ## Join the list twice because of the sublist used in "lsort -index".
            set NukeeLog [join [join [lsort -decreasing -integer -index 2 $NukeeLog]]]

            if {$IsNuke} {
                set ReMap [list %(user) $user %(group) $group %(multi) $Multi %(reason) $Reason]
                set NukeTag [file join $RealPath [string map $ReMap $nuke(InfoTag)]]
                CreateTag $NukeTag $uid $gid 555
                RemoveParentLinks $RealPath $VirtualPath
                set DirChmod 555
                set LogPrefix "NUKE"
                set NukeStatus 0
                set NewName "$nuke(Prefix)[file tail $VirtualPath]"

            } else {
                foreach ListItem [FindTags $RealPath $nuke(InfoTag)] {
                    RemoveTag $ListItem
                }
                set DirChmod 777
                set LogPrefix "UNNUKE"
                set NukeStatus 1
                set VirtualPath [TrimTag $VirtualPath]
                set NewName [file tail $VirtualPath]
            }

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
                    ErrorLog NukeRename $ErrorMsg
                    iputs "|------------------------------------------------------------------------|"
                    LinePuts "Unable to rename directory, ask a siteop to rename it manually."
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
                catch {vfs write $ListItem $UserId $GroupId $DirChmod}
            }
            catch {vfs flush $ParentPath}
            if {[IsTrue $misc(dZSbotLogging)]} {
                foreach {NukeeUser NukeeGroup Amount} $NukeeLog {
                    set Amount [format "%.2f" [expr {double($Amount) / 1024.0}]]
                    putlog "${LogPrefix}: \"$VirtualPath\" \"$user@$group\" \"$NukeeUser@$NukeeGroup\" \"$Multi $Amount\" \"$Reason\""
                }
            } else {
                putlog "${LogPrefix}: \"$VirtualPath\" \"$user\" \"$group\" \"$Multi\" \"$Reason\" \"$Files\" \"$TotalSize\" \"$DiskCount\" \"$NukeeLog\""
            }

            if {![catch {DbOpenFile NukeDb "Nukes.db"} ErrorMsg]} {
                ## In order to pass a NULL value to TclSQLite, the variable must be unset.
                if {![string is digit -strict $NukeId]} {unset NukeId}
                NukeDb eval {INSERT OR REPLACE INTO Nukes (NukeId,TimeStamp,UserName,GroupName,Status,Release,Reason,Multi,Files,Size) VALUES($NukeId,$NukeTime,$user,$group,$NukeStatus,$Release,$Reason,$Multi,$Files,$TotalSize)}
                set NukeId [NukeDb last_insert_rowid]

                NukeDb eval {BEGIN}
                foreach {NukeeUser NukeeGroup Amount} $NukeeLog {
                    NukeDb eval {INSERT OR REPLACE INTO Users (NukeId,Status,UserName,GroupName,Amount) VALUES($NukeId,$NukeStatus,$NukeeUser,$NukeeGroup,$Amount)}
                }
                NukeDb eval {COMMIT}

                NukeDb close
            } else {ErrorLog NukeDb $ErrorMsg}

            ## Save the nuke ID and multiplier for later use (ie. unnuke).
            UpdateRecord [expr {$RenameFail ? $RealPath : $NewPath}] "2|$NukeStatus|$NukeId|$user|$group|$Multi|$Reason"
            iputs "'------------------------------------------------------------------------'"
        }
        {nukes} - {unnukes} {
            set IsSiteBot [string equal $misc(SiteBot) $user]
            if {![GetOptions [lrange $ArgList 1 end] MaxResults Pattern]} {
                iputs "Syntax: SITE [string toupper $Action] \[-max <limit>\] \[release\]"
                return 0
            }
            set Pattern [SqlWildToLike [regsub -all {[\s\*]+} "*$Pattern*" "*"]]
            if {[string equal "nukes" $Action]} {
                if {!$IsSiteBot} {
                    iputs ".-\[Nukes\]----------------------------------------------------------------."
                    iputs "|    Age    |    Nuker     |   Multi   |  Reason                         |"
                    iputs "|------------------------------------------------------------------------|"
                }
                set NukeStatus 0
            } elseif {[string equal "unnukes" $Action]} {
                if {!$IsSiteBot} {
                    iputs ".-\[UnNukes\]--------------------------------------------------------------."
                    iputs "|    Age    |   UnNuker    |   Multi   |  Reason                         |"
                    iputs "|------------------------------------------------------------------------|"
                }
                set NukeStatus 1
            }
            set Count 0
            if {![catch {DbOpenFile NukeDb "Nukes.db"} ErrorMsg]} {
                NukeDb eval "SELECT * FROM Nukes WHERE Status=$NukeStatus AND Release LIKE '$Pattern' ESCAPE '\\' ORDER BY TimeStamp DESC LIMIT $MaxResults" values {
                    incr Count
                    if {$IsSiteBot} {
                        iputs "NUKES|$Count|$values(TimeStamp)|$values(Release)|$values(UserName)|$values(GroupName)|$values(Multi)|$values(Reason)|$values(Files)|$values(Size)"
                    } else {
                        set NukeAge [expr {[clock seconds] - $values(TimeStamp)}]
                        iputs [format "| %-9.9s | %-12.12s | %-9.9s | %-31.31s |" [lrange [FormatDuration $NukeAge] 0 1] $values(UserName) $values(Multi)x $values(Reason)]
                        iputs [format "| Dir: %-65.65s |" $values(Release)]
                        iputs "|------------------------------------------------------------------------|"
                    }
                }
                NukeDb close
            } else {ErrorLog NukeLatest $ErrorMsg}

            if {!$IsSiteBot} {
                if {!$Count} {
                    LinePuts "There are no nukes or unnukes to display."
                } else {
                    LinePuts "Read the rules to avoid being nuked."
                }
                iputs "'------------------------------------------------------------------------'"
            }
        }
        {nuketop} {
            set IsSiteBot [string equal $misc(SiteBot) $user]
            if {![GetOptions [lrange $ArgList 1 end] MaxResults Pattern]} {
                iputs "Syntax: SITE NUKETOP \[-max <limit>\] \[group\]"
                return 0
            }
            if {![string equal "" $Pattern]} {
                set GroupMatch "AND GroupName LIKE '[SqlWildToLike $Pattern]' ESCAPE '\\'"
            } else {
                set GroupMatch ""
            }
            if {!$IsSiteBot} {
                iputs ".-\[NukeTop\]--------------------------------------------------------------."
                iputs "|    User    |   Group    | Times Nuked |  Amount                        |"
                iputs "|------------------------------------------------------------------------|"
            }
            set Count 0
            if {![catch {DbOpenFile NukeDb "Nukes.db"} ErrorMsg]} {
                NukeDb eval "SELECT UserName, GroupName, count(*) AS Nuked, sum(Amount) AS Amount FROM Users WHERE Status=0 $GroupMatch GROUP BY UserName ORDER BY Nuked DESC LIMIT $MaxResults" values {
                    incr Count
                    if {$IsSiteBot} {
                        iputs "NUKETOP|$Count|$values(UserName)|$values(GroupName)|$values(Nuked)|$values(Amount)"
                    } else {
                        iputs [format "| %-10.10s | %-10.10s | %11d | %30.30s |" $values(UserName) $values(GroupName) $values(Nuked) [FormatSize $values(Amount)]]
                    }
                }
                NukeDb close
            } else {ErrorLog NukeTop $ErrorMsg}
            if {!$IsSiteBot} {
                if {!$Count} {LinePuts "There are no nukees display."}
                iputs "'------------------------------------------------------------------------'"
            }
        }
        default {
            ErrorLog InvalidArgs "invalid parameter \"[info script] $Action\": check your ioFTPD.ini for errors"
        }
    }
    return 0
}

::nxTools::Nuke::Main [expr {[info exists args] ? $args : ""}]
