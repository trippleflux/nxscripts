################################################################################
# nxTools - Pre Script                                                         #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

namespace eval ::nxTools::Pre {
    namespace import -force ::nxLib::*
}

# Pre Procedures
######################################################################

proc ::nxTools::Pre::ConfigLoader {ConfigFile} {
    upvar ConfigComments ConfigComments prearea prearea pregrp pregrp prepath prepath
    set ConfMode 0; set ConfigComments ""
    if {![catch {set Handle [open $ConfigFile r]} ErrorMsg]} {
        while {![eof $Handle]} {
            if {[set FileLine [string trim [gets $Handle]]] == ""} {continue}
            ## Check config section
            if {[string index $FileLine 0] == "#"} {append ConfigComments $FileLine "\n"; continue
            } elseif {[string equal {[AREAS]} $FileLine]} {set ConfMode 1; continue
            } elseif {[string equal {[GROUPS]} $FileLine]} {set ConfMode 2; continue
            } elseif {[string equal {[PATHS]} $FileLine]} {set ConfMode 3; continue
            } elseif {[string match {\[*\]} $FileLine]} {set ConfMode 0; continue}
            switch -exact -- $ConfMode {
                1 {set prearea([lindex $FileLine 0]) [lindex $FileLine 1]}
                2 {set pregrp([lindex $FileLine 0]) [lindex $FileLine 1]}
                3 {set prepath([lindex $FileLine 0]) [lrange $FileLine 1 end]}
            }
        }
        close $Handle
    } else {
        ErrorReturn "Unable to load the pre configuration, contact a siteop."
        ErrorLog PreConfigLoader $ErrorMsg
    }
}

proc ::nxTools::Pre::ConfigWriter {ConfigFile} {
    upvar ConfigComments ConfigComments prearea prearea pregrp pregrp prepath prepath
    if {![catch {set Handle [open $ConfigFile w]} ErrorMsg]} {
        puts $Handle $ConfigComments
        puts $Handle "\[AREAS\]"
        foreach {Name Value} [array get prearea] {
            puts $Handle "$Name \"$Value\""
        }
        puts $Handle "\n\[GROUPS\]"
        foreach {Name Value} [array get pregrp] {
            puts $Handle "$Name \"[lsort -ascii $Value]\""
        }
        puts $Handle "\n\[PATHS\]"
        foreach {Name Value} [array get prepath] {
            puts $Handle "$Name \"[join $Value {" "}]\""
        }
        close $Handle
    } else {ErrorLog PreConfigWriter $ErrorMsg}
}

proc ::nxTools::Pre::Links {VirtualPath} {
    global latest
    if {[ListMatch $latest(Exempts) $VirtualPath]} {
        return 0
    }
    if {[catch {DbOpenFile LinkDb "Links.db"} ErrorMsg]} {
        ErrorLog PreLinks $ErrorMsg
        return 1
    }
    ## Format and create the link directory
    set TagName [file tail $VirtualPath]
    if {$latest(MaxLength) > 0 && [string length $TagName] > $latest(MaxLength)} {
        set TagName [string trimright [string range $TagName 0 $latest(MaxLength)] "."]
    }
    set TagName [string map [list %(release) $TagName] $latest(PreTag)]
    set TimeStamp [clock seconds]
    LinkDb eval {INSERT INTO Links (TimeStamp,LinkType,DirName) VALUES($TimeStamp,1,$TagName)}
    set TagName [file join $latest(SymPath) $TagName]
    if {![catch {file mkdir $TagName} ErrorMsg]} {
        catch {vfs chattr $TagName 1 $VirtualPath}
    } else {ErrorLog PreLinksMkDir $ErrorMsg}

    ## Remove older links
    if {[set LinkCount [LinkDb eval {SELECT count(*) FROM Links WHERE LinkType=1}]] > $latest(PreLinks)} {
        set LinkCount [expr {$LinkCount - $latest(PreLinks)}]
        LinkDb eval "SELECT DirName,rowid FROM Links WHERE LinkType=1 ORDER BY TimeStamp ASC LIMIT $LinkCount" values {
            RemoveTag [file join $latest(SymPath) $values(DirName)]
            LinkDb eval {DELETE FROM Links WHERE rowid=$values(rowid)}
        }
    }
    LinkDb close
    return 0
}

proc ::nxTools::Pre::ResolvePath {UserName GroupName RealPath} {
    set BestMatch 0
    set ResolvePath "/"; set VfsFile ""
    set RealPath [string map {\\ /} $RealPath]

    ## Find the user VFS-File
    if {[userfile open $UserName] == 0} {
        set UserFile [userfile bin2ascii]
        foreach UserLine [split $UserFile "\r\n"] {
            set LineType [string tolower [lindex $UserLine 0]]
            if {[string equal "vfsfile" $LineType]} {
                set VfsFile [ArgRange $UserLine 1 end]; break
            }
        }
    }
    ## Use the group VFS-File (if the user vfs file doesn't exist)
    if {![file isfile $VfsFile] && [groupfile open $GroupName] == 0} {
        set GroupFile [groupfile bin2ascii]
        foreach GroupLine [split $GroupFile "\r\n"] {
            set LineType [string tolower [lindex $GroupLine 0]]
            if {[string equal "vfsfile" $LineType]} {
                set VfsFile [ArgRange $GroupLine 1 end]; break
            }
        }
    }
    ## Use the default VFS-File (if the user and group vfs file don't exist)
    if {![file isfile $VfsFile]} {
        set VfsFile [config read "Locations" "Default_Vfs"]
    }
    if {![catch {set Handle [open $VfsFile r]} ErrorMsg]} {
        while {![eof $Handle]} {
            if {[set FileLine [string trim [gets $Handle]]] == ""} {continue}
            foreach {VfsRealPath VfsVirtualPath} [string map {\\ /} $FileLine] {break}
            if {[string first [string tolower $VfsRealPath] [string tolower $RealPath]] == 0} {
                ## Use the longest available mount path (more accurate)
                if {[set Length [string length $VfsRealPath]] > $BestMatch} {
                    set ResolvePath [string range $RealPath [set BestMatch $Length] end]
                    set ResolvePath [file join $VfsVirtualPath [string trimleft $ResolvePath "/"]]
                }
            }
        }
        close $Handle
    } else {
        ErrorLog PreResolvePath $ErrorMsg
        ErrorReturn "Unable to resolve virtual path, contact a siteop."
    }
    return $ResolvePath
}

proc ::nxTools::Pre::UpdateUser {UserName Files Size CreditSection StatsSection} {
    set CreditSection [expr {$CreditSection + 1}]
    set StatsSection [expr {$StatsSection * 3 + 1}]
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
            set DiffCredits [expr {wide($Size) * $Ratio}]
            set NewCredits [expr {wide($OldCredits) + wide($DiffCredits)}]
        } else {
            set DiffCredits 0
            set NewCredits $OldCredits
        }
        foreach UserLine $UserFile {
            set LineType [string tolower [lindex $UserLine 0]]
            if {[lsearch -exact "allup dayup monthup wkup" $LineType] != -1} {
                set NewFiles [expr {wide([lindex $UserLine $StatsSection]) + $Files}]
                set NewStats [expr {wide([lindex $UserLine [expr {$StatsSection + 1}]]) + wide($Size)}]
                set UserLine [lreplace $UserLine $StatsSection [expr {$StatsSection + 1}] $NewFiles $NewStats]
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

# Pre Main
######################################################################

proc ::nxTools::Pre::Main {ArgV} {
    global dupe latest misc mysql pre pretime group groups pwd user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    ## Safe argument handling
    set ArgList [ArgList $ArgV]
    foreach {Action Option Target Value Other} $ArgV {break}
    set Action [string tolower $Action]

    if {[string equal "pre" $Action]} {
        if {[string equal "" $Option] || [string equal -nocase "help" $Option]} {
            iputs ".-\[Pre\]------------------------------------------------------------------."
            LinePuts "Syntax: SITE PRE <area> <directory>"
            LinePuts "        SITE PRE HISTORY \[-max <limit>\] \[group\]"
            LinePuts "        SITE PRE STATS \[-max <limit>\] \[group\]"
            LinePuts ""
            LinePuts "Areas : [lsort -ascii [array names prearea]]"
        } elseif {[string equal -nocase "history" $Option]} {
            if {![GetOptions [lrange $ArgList 2 end] MaxResults Pattern]} {
                iputs "Syntax: SITE PRE HISTORY \[-max <limit>\] \[group\]"
                return 0
            }
            if {![string equal "" $Pattern]} {
                set WhereClause "WHERE GroupName LIKE '[SqlWildToLike $Pattern]' ESCAPE '\\'"
            } else {
                set WhereClause ""
            }
            iputs ".-\[PreHistory\]-----------------------------------------------------------."
            iputs "| ## |  Release                                              |  Amount   |"
            iputs "|------------------------------------------------------------------------|"
            set Count 0
            if {![catch {DbOpenFile PreDb "Pres.db"} ErrorMsg]} {
                PreDb eval "SELECT Release,Size FROM Pres $WhereClause ORDER BY TimeStamp DESC LIMIT $MaxResults" values {
                    iputs [format "| %02d | %-53.53s | %9s |" [incr Count] $values(Release) [FormatSize $values(Size)]]
                }
                PreDb close
            } else {ErrorLog PreHistory $ErrorMsg}
            if {!$Count} {LinePuts "There are no pres to display."}
        } elseif {[string equal -nocase "stats" $Option]} {
            if {![GetOptions [lrange $ArgList 2 end] MaxResults Pattern]} {
                iputs "Syntax: SITE PRE STATS \[-max <limit>\] \[group\]"
                return 0
            }
            if {![string equal "" $Pattern]} {
                set WhereClause "WHERE GroupName LIKE '[SqlWildToLike $Pattern]' ESCAPE '\\'"
            } else {
                set WhereClause ""
            }
            iputs ".-\[PreStats\]-------------------------------------------------------------."
            iputs "| ## |  Group                        |   Pres    |   Files   |  Amount   |"
            iputs "|------------------------------------------------------------------------|"
            set Count 0
            if {![catch {DbOpenFile PreDb "Pres.db"} ErrorMsg]} {
                PreDb eval "SELECT GroupName, count(*) AS Pres, round(sum(Files)) AS Files, sum(Size) AS Amount FROM Pres $WhereClause GROUP BY GroupName ORDER BY Pres DESC LIMIT $MaxResults" values {
                    iputs [format "| %02d | %-29.29s | %9d | %8dF | %9s |" [incr Count] $values(GroupName) $values(Pres) $values(Files) [FormatSize $values(Amount)]]
                }
                PreDb close
            } else {ErrorLog PreStats $ErrorMsg}
            if {!$Count} {LinePuts "There are no statistics to display."}
        } else {
            iputs ".-\[Pre\]------------------------------------------------------------------."
            ConfigLoader $pre(ConfigFile)
            set PreArea [string toupper $Option]

            ## Check area and group paths
            if {![info exists prearea($PreArea)] || [string equal "" $prearea($PreArea)]} {
                ErrorReturn "Invalid area, try \"SITE PRE HELP\" to view available areas."
            } elseif {![info exists pregrp($PreArea)]} {
                ErrorReturn "The specified area has no group list defined."
            }

            ## Check if group is allowed to pre to the area and from this path
            set AllowPath 0; set PreGroup ""
            set VirtualPath [GetPath $pwd $Target]
            foreach GroupName $groups {
                if {[lsearch -exact $pregrp($PreArea) $GroupName] != -1} {
                    set PreGroup $GroupName
                    if {![info exists prepath($GroupName)]} {continue}
                    foreach PathEntry $prepath($GroupName) {
                        set PathEntry [string trimright $PathEntry "/"]
                        set PathLength [string length $PathEntry]
                        if {[string equal -length $PathLength -nocase $PathEntry $VirtualPath]} {
                            set AllowPath 1; break
                        }
                    }
                    if {$AllowPath} {break}
                }
            }
            if {[string equal "" $PreGroup]} {
                ErrorReturn "Your group(s) are not allowed to pre to the \"$PreArea\" area."
            } elseif {!$AllowPath} {
                ErrorReturn "Your group \"$PreGroup\" is not allowed to pre from this path."
            }

            ## Check if the specified directory exists
            set RealPath [resolve pwd $VirtualPath]
            if {[string equal "" $Target] || ![file isdirectory $RealPath]} {
                ErrorReturn "The specified directory does not exist."
            }
            set DiskCount 0; set Files 0; set TotalSize 0
            set Release [file tail $VirtualPath]

            ## Find the destination path
            set DestRealPath $prearea($PreArea)
            set PreTime [clock seconds]
            set DestRealPath [clock format $PreTime -format $DestRealPath -gmt [IsTrue $misc(UTC_Time)]]
            if {![file isdirectory $DestRealPath]} {
                LinePuts "The pre destination path for the \"$PreArea\" area does not exist."
                ErrorReturn "Note: If the area uses dated dirs, check that today's date dir exists."
            }
            set DestRealPath [file join $DestRealPath $Release]
            if {[file exists $DestRealPath]} {ErrorReturn "A file or directory by that name already exists in the target area."}

            ## Find the credit and stats section
            set DestVirtualPath [PreResolvePath $user $group $DestRealPath]
            ListAssign [GetCreditsAndStats $DestVirtualPath] CreditSection StatsSection

            ## Count CDs/Discs/DVDs
            foreach ListItem [glob -nocomplain -types d -directory $RealPath "*"] {
                if {[IsMultiDisk $ListItem]} {incr DiskCount}
            }

            ## Count files and total size
            GetDirList $RealPath dirlist ".ioFTPD*"
            foreach ListItem $dirlist(FileList) {
                incr Files; set FileSize [file size $ListItem]
                set FileSize [file size $ListItem]
                set TotalSize [expr {wide($TotalSize) + wide($FileSize)}]
                if {[IsTrue $pre(Uploaders)]} {
                    catch {lindex [vfs read $ListItem] 0} UserId
                    if {$FileSize > 0 && [set UserName [resolve uid $UserId]] != ""} {
                        ## Increase file Count
                        if {[info exists prefiles($UserName)]} {
                            incr prefiles($UserName)
                        } else {set prefiles($UserName) 1}

                        ## Add total size
                        if {[info exists presize($UserName)]} {
                            set presize($UserName) [expr {wide($presize($UserName)) + wide($FileSize)}]
                        } else {set presize($UserName) $FileSize}
                    }
                }
            }
            set TotalSize [expr {wide($TotalSize) / 1024}]

            ## If the pre destination is a different drive, ensure that there
            ## is adequate space for the release (and a 10MB "safety" buffer).
            set SourceDrive [lindex [file split $RealPath] 0]
            set DestDrive [lindex [file split $DestRealPath] 0]

            if {![string equal -nocase $SourceDrive $DestDrive]} {
                if {![::nx::volume info $DestDrive volume]} {
                    ErrorLog PreCheckSpace "unable to retrieve volume information for \"$DestDrive\""
                    ErrorReturn "Unable to check available space on target drive, contact a siteop."
                }
                set CheckSize [expr {wide($TotalSize) + (10*1024)}]
                set volume(free) [expr {wide($volume(free)) / 1024}]

                if {$CheckSize > $volume(free)} {
                    ErrorLog PreLowSpace "unable to pre \"$RealPath\": insufficient disk space on $DestDrive ([FormatSize $volume(free)] free, needed [FormatSize $CheckSize])"
                    ErrorReturn "Insufficient disk space on target drive, contact a siteop."
                }
            }
            LinePuts "Area    : $PreArea"
            LinePuts "Release : $Release"
            LinePuts "Files   : [format %-16s ${Files}F] Size: [format %-16s [FormatSize $TotalSize]] CDs: $DiskCount"

            ## Move release to the destination path
            KickUsers [file join $VirtualPath "*"]
            if {[catch {file rename -force -- $RealPath $DestRealPath} ErrorMsg]} {
                ErrorLog PreMove $ErrorMsg
                ErrorReturn "Error   : Unable to move directory, aborting."
            }

            set IsMP3 0
            set FilePath [expr {!$DiskCount ? "*.mp3" : "*/*.mp3"}]
            set MP3Files [glob -nocomplain -types f -directory $DestRealPath $FilePath]

            ## Attempt to parse every MP3 file until successful
            foreach FilePath $MP3Files {
                if {[::nx::mp3 $FilePath mp3]} {
                    set Codec [format "MPEG %.1f Layer %d" $mp3(version) $mp3(layer)]
                    iputs "|------------------------------------------------------------------------|"
                    iputs [format "| Artist  : %-60s |" $mp3(artist)]
                    iputs [format "| Album   : %-60s |" $mp3(album)]
                    iputs [format "| Genre   : %-16s Year: %-11s Bitrate: %-16s |" $mp3(genre) $mp3(year) "$mp3(bitrate) Kbit"]
                    iputs [format "| Channel : %-16s Type: %-13s Codec: %-16s |" $mp3(mode) $mp3(type) $Codec]
                    set IsMP3 1; break
                } else {
                    ErrorLog PreMP3 "unable to read ID3/MP3 headers from \"$FilePath\": invalid MP3 file"
                }
            }

            ## Hide the user's name
            if {[string is digit -strict $pre(ChownUID)]} {
                GetDirList $DestRealPath dirlist ".ioFTPD*"
                foreach ListItem $dirlist(DirList) {
                    catch {vfs read $ListItem} VfsOwner
                    ListAssign $VfsOwner UserId GroupId Chmod
                    ## Verify the group and chmod the directory.
                    if {![string is digit -strict $GroupId]} {set GroupId [lindex $misc(DirOwner) 1]}
                    if {![string is digit -strict $Chmod]} {set Chmod $misc(DirChmod)}
                    catch {vfs write $ListItem $pre(ChownUID) $GroupId $Chmod}
                }
                foreach ListItem $dirlist(FileList) {
                    catch {vfs read $ListItem} VfsOwner
                    ListAssign $VfsOwner UserId GroupId Chmod
                    ## Verify the group and chmod the file.
                    if {![string is digit -strict $GroupId]} {set GroupId [lindex $misc(FileOwner) 1]}
                    if {![string is digit -strict $Chmod]} {set Chmod $misc(FileChmod)}
                    catch {vfs write $ListItem $pre(ChownUID) $GroupId $Chmod}
                }
            }
            ## Touch file and directory times
            if {[IsTrue $pre(TouchTimes)]} {
                if {![::nx::touch -recurse $DestRealPath $PreTime]} {
                    ErrorLog PreTouch "unable to touch file times for \"$DestRealPath\""
                }
            }
            catch {vfs flush [file dirname $RealPath]}
            catch {vfs flush [file dirname $DestRealPath]}

            if {[IsTrue $pre(PreUser)]} {
                PreCredits $user 0 0 $CreditSection $StatsSection
            }
            if {[IsTrue $pre(Uploaders)]} {
                iputs "|------------------------------------------------------------------------|"
                iputs "|    User    |   Group    |   Ratio    |  Files   |  Amount  |  Credits  |"
                iputs "|------------------------------------------------------------------------|"
                foreach UserName [lsort -ascii [array names presize]] {
                    set UploadAmount [expr {wide($presize($UserName)) / 1024}]
                    set Result [UpdateUser $UserName $prefiles($UserName) $UploadAmount $CreditSection $StatsSection]
                    foreach {GroupName Ratio OldCredits NewCredits DiffCredits} $Result {break}
                    set Ratio [expr {$Ratio != 0 ? "1:$Ratio" : "Unlimited"}]
                    iputs [format "| %-10s | %-10s | %10s | %7d\F | %8s | %9s |" $UserName $GroupName $Ratio $prefiles($UserName) [FormatSize $UploadAmount] [FormatSize $DiffCredits]]
                }
            }
            if {[IsTrue $misc(dZSbotLogging)]} {
                set TotalMB [format "%.2f" [expr {double($TotalSize) / 1024.0}]]
                set LogLine "PRE: \"$DestVirtualPath\" \"$PreGroup\" \"$user\" \"$group\" \"$PreArea\" \"$Files\" \"$TotalMB\" \"$DiskCount\""
                if {$IsMP3} {append LogLine " \"$mp3(genre)\" \"$mp3(bitrate)\" \"$mp3(year)\""}
            } else {
                set LogLine "PRE: \"$DestVirtualPath\" \"$PreGroup\" \"$user\" \"$group\" \"$PreArea\" \"$Files\" \"$TotalSize\" \"$DiskCount\""
                if {$IsMP3} {append LogLine " \"$mp3(artist)|$mp3(album)|$mp3(genre)|$mp3(year)|$mp3(bitrate)|$mp3(type)\""}
            }
            putlog $LogLine

            if {![catch {DbOpenFile PreDb "Pres.db"} ErrorMsg]} {
                PreDb eval {INSERT INTO Pres (TimeStamp,UserName,GroupName,Area,Release,Files,Size) VALUES($PreTime,$user,$PreGroup,$PreArea,$Release,$Files,$TotalSize)}
                PreDb close
            } else {ErrorLog PreDb $ErrorMsg}

            if {[IsTrue $dupe(AddOnPre)]} {
                if {![catch {DbOpenFile DirDb "DupeDirs.db"} ErrorMsg]} {
                    set LogUser [expr {$pre(ChownUID) != "" ? [resolve uid $pre(ChownUID)] : $user}]
                    set LogPath [string range $DestVirtualPath 0 [string last "/" $DestVirtualPath]]
                    DirDb eval {INSERT INTO DupeDirs (TimeStamp,UserName,GroupName,DirPath,DirName) VALUES($PreTime,$LogUser,$PreGroup,$LogPath,$Release)}
                    DirDb close
                } else {ErrorLog PreDupeDb $ErrorMsg}
            }

            if {[IsTrue $pretime(AddOnPre)] && [MySqlConnect]} {
                set PreArea [::mysql::escape $PreArea]
                set Release [::mysql::escape $Release]
                if {[catch {::mysql::exec $mysql(ConnHandle) "INSERT INTO $mysql(TableName) (pretime,section,release,files,kbytes,disks) VALUES('$PreTime','$PreArea','$Release','$Files','$TotalSize','$DiskCount')"} ErrorMsg]} {
                    if {[string first "Duplicate entry" $ErrorMsg] == -1} {ErrorLog PreAddToDb $ErrorMsg}
                }
                MySqlClose
            }
            ## Create latest pre symlinks
            if {$latest(PreLinks) > 0} {PreLinks $DestVirtualPath}
        }
    } elseif {[string equal "edit" $Action]} {
        iputs ".-\[EditPre\]--------------------------------------------------------------."
        ConfigLoader $pre(ConfigFile)

        set PreArea [string toupper $Target]
        set Option [string tolower $Option]
        switch -exact -- $Option {
            {addarea} {
                if {[string equal "" $Target]} {
                    ErrorReturn "Invalid area, you must specify an area to add."
                } elseif {[info exists prearea($PreArea)]} {
                    ErrorReturn "The \"$PreArea\" area already exists, delete it first."
                } elseif {[string equal "" $Value]} {
                    ErrorReturn "The \"$PreArea\" area must have a destination path."
                }

                ## Check pre destination path
                set RealPath [resolve pwd $Value]
                if {[string index $Value 0] != "/" || ![file isdirectory $RealPath]} {
                    LinePuts "The destination path \"$Value\" is invalid or does not exist."
                    ErrorReturn "Note: Make sure the path begins with a forward slash."
                } elseif {[string index $RealPath end] != "/"} {append RealPath "/"}

                ## Check date cookie(s)
                if {[string first "%" $Other] != -1} {
                    set Other [string trim $Other "/"]
                    append RealPath $Other "/"
                } elseif {![string equal "" $Other]} {
                    LinePuts "Invalid destination date cookie(s) \"$Other\"."
                    ErrorReturn "Note: Try \"SITE EDITPRE HELP ADDAREA\" to view valid date cookies."
                }
                set prearea($PreArea) $RealPath
                set pregrp($PreArea) ""
                LinePuts "Created area \"$PreArea\", destination set to \"$Value$Other\"."
                LinePuts "Note: Add groups to the area so others may pre to it."
                ConfigWriter $pre(ConfigFile)
            }
            {delarea} {
                if {[string equal "" $Target] || ![info exists prearea($PreArea)]} {
                    ErrorReturn "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
                }
                unset -nocomplain prearea($PreArea) pregrp($PreArea)
                LinePuts "Removed the area \"$PreArea\" and all related settings."
                ConfigWriter $pre(ConfigFile)
            }
            {addgrp} {
                if {[string equal "" $Target] || ![info exists prearea($PreArea)]} {
                    ErrorReturn "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
                } elseif {[string equal "" $Value]} {
                    ErrorReturn "Invalid group, you must specify a group to add."
                }

                ## Check if the group already exists in the area
                if {![info exists pregrp($PreArea)]} {set pregrp($PreArea) ""}
                foreach GroupEntry $pregrp($PreArea) {
                    if {[string equal -nocase $GroupEntry $Value]} {
                        ErrorReturn "The group \"$Value\" already exists in the \"$PreArea\" allow list."
                    }
                }
                lappend pregrp($PreArea) $Value
                LinePuts "Added the group \"$Value\" to the \"$PreArea\" allow list."
                ConfigWriter $pre(ConfigFile)
            }
            {delgrp} {
                if {[string equal "" $Target] || ![info exists prearea($PreArea)]} {
                    ErrorReturn "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
                } elseif {[string equal "" $Value]} {
                    ErrorReturn "Invalid group, you must specify a group to remove."
                }
                set Deleted 0; set Index 0

                ## Remove the group from the area list
                if {![info exists pregrp($PreArea)]} {set pregrp($PreArea) ""}
                foreach GroupEntry $pregrp($PreArea) {
                    if {[string equal -nocase $GroupEntry $Value]} {
                        set pregrp($PreArea) [lreplace $pregrp($PreArea) $Index $Index]
                        set Deleted 1; break
                    }
                    incr Index
                }
                if {$Deleted} {
                    LinePuts "Removed the group \"$Value\" from the \"$PreArea\" allow list."
                    ConfigWriter $pre(ConfigFile)
                } else {
                    LinePuts "The group \"$Value\" does not exist in the \"$PreArea\" allow list."
                }
            }
            {addpath} {
                if {[string equal "" $Target]} {
                    ErrorReturn "Invalid group, try \"SITE EDITPRE HELP ADDPATH\" for help."
                } elseif {[string equal "" $Value]} {
                    ErrorReturn "Invalid path, you must specify a path where group can pre from."
                }

                ## Check group path
                set RealPath [resolve pwd $Value]
                if {[string index $Value 0] != "/" || ![file isdirectory $RealPath]} {
                    LinePuts "The path \"$Value\" is invalid or does not exist."
                    ErrorReturn "Note: Make sure the path has both leading and trailing slashes."
                } elseif {[string index $Value end] != "/"} {append Value "/"}

                ## Check if the path already defined for that group
                if {![info exists prepath($Target)]} {set prepath($Target) ""}
                foreach PathEntry $prepath($Target) {
                    if {[string equal -nocase $PathEntry $Value]} {
                        ErrorReturn "The path \"$Value\" is already defined for the \"$Target\" group."
                    }
                }
                lappend prepath($Target) $Value
                catch {vfs chattr $RealPath 0 [string map [list %(group) $Target] $pre(PrivatePath)]}
                LinePuts "Added path \"$Value\" to the \"$Target\" group."
                ConfigWriter $pre(ConfigFile)
            }
            {delpath} {
                if {[string equal "" $Target]} {
                    ErrorReturn "Invalid group, try \"SITE EDITPRE HELP DELPATH\" for help."
                } elseif {[string equal "" $Value]} {
                    ErrorReturn "Invalid path, you must specify a path to delete."
                }

                ## Remove the path from the group
                set Deleted 0; set Index 0
                if {[info exists prepath($Target)]} {
                    foreach PathEntry $prepath($Target) {
                        if {[string equal -nocase $PathEntry $Value]} {
                            set prepath($Target) [lreplace $prepath($Target) $Index $Index]
                            set Deleted 1; break
                        }
                        incr Index
                    }
                    if {[string equal "" $prepath($Target)]} {unset prepath($Target)}
                }
                if {$Deleted} {
                    LinePuts "Removed path \"$Value\" for the \"$Target\" group."
                    ConfigWriter $pre(ConfigFile)
                } else {
                    LinePuts "The path \"$Value\" is not defined for the \"$Target\" group."
                }
            }
            {hidepath} - {hidepaths} {
                if {[string equal "" $Target]} {
                    set Target "*"
                } elseif {![info exists prepath($Target)]} {
                    ErrorReturn "Invalid group, try \"SITE EDITPRE HELP HIDEPATH\" for help."
                }
                foreach Name [lsort -ascii [array names prepath]] {
                    if {![string match $Target $Name]} {continue}
                    LinePuts "Hiding pre paths for: $Name"
                    set PrivPath [string map [list %(group) $Name] $pre(PrivatePath)]

                    foreach PathEntry $prepath($Name) {
                        set RealPath [resolve pwd $PathEntry]
                        if {[file exists $RealPath]} {
                            catch {vfs chattr $RealPath 0 $PrivPath}
                        } else {
                            LinePuts "- The vpath \"$PathEntry\" does not exist."
                        }
                    }
                }
            }
            {view} {
                LinePuts "Areas:"
                foreach Name [lsort -ascii [array names prearea]] {
                    LinePuts [format "%-10s - %s" $Name $prearea($Name)]
                }
                LinePuts ""; LinePuts "Groups:"
                foreach Name [lsort -ascii [array names pregrp]] {
                    LinePuts [format "%-10s - %s" $Name $pregrp($Name)]
                }
                LinePuts ""; LinePuts "Paths:"
                foreach Name [lsort -ascii [array names prepath]] {
                    LinePuts [format "%-10s - %s" $Name $prepath($Name)]
                }
            }
            default {
                set Option [string tolower $Target]
                switch -exact -- $Option {
                    {addarea} {
                        LinePuts "Description:"
                        LinePuts " - Create a pre area and destination path."
                        LinePuts " - Date cookies can be given for dated dirs."
                        LinePuts " - Some FTP clients may interpret cookies as commands (FlashFXP & %d)."
                        LinePuts "Date Cookies:"
                        LinePuts " %d - Day     %y - Year (04)"
                        LinePuts " %W - Week    %Y - Year (2004)"
                        LinePuts " %m - Month   %% - Percent Sign"
                        LinePuts "Syntax : SITE EDITPRE ADDAREA <area> <path> \[date cookie(s)\]"
                        LinePuts "Example: SITE EDITPRE ADDAREA 0DAY /0DAY/ %m%d"
                    }
                    {delarea} {
                        LinePuts "Description:"
                        LinePuts " - Delete a pre area and related settings."
                        LinePuts "Syntax : SITE EDITPRE DELAREA <area>"
                        LinePuts "Example: SITE EDITPRE DELAREA 0DAY"
                    }
                    {addgrp} {
                        LinePuts "Description:"
                        LinePuts " - Allow a group to pre to the specified area."
                        LinePuts "Syntax : SITE EDITPRE ADDGRP <area> <group>"
                        LinePuts "Example: SITE EDITPRE ADDGRP 0DAY NX"
                    }
                    {delgrp} {
                        LinePuts "Description:"
                        LinePuts " - Disallow a group to pre to the specified area."
                        LinePuts "Syntax : SITE EDITPRE DELGRP <area> <group>"
                        LinePuts "Example: SITE EDITPRE DELGRP 0DAY NX"
                    }
                    {addpath} {
                        LinePuts "Description:"
                        LinePuts " - Add a path to be used as a pre area for the specified group."
                        LinePuts "Syntax : SITE EDITPRE ADDPATH <group> <path>"
                        LinePuts "Example: SITE EDITPRE ADDPATH NX /GROUPS/NX/"
                    }
                    {delpath} {
                        LinePuts "Description:"
                        LinePuts " - Remove a path from the specified group."
                        LinePuts "Syntax : SITE EDITPRE DELPATH <group> <path>"
                        LinePuts "Example: SITE EDITPRE DELPATH NX /GROUPS/NX/"
                    }
                    {hidepath} - {hidepaths} {
                        LinePuts "Description:"
                        LinePuts " - Re-hides all group pre paths using private paths (chattr +h)."
                        LinePuts " - Useful for when the chattr value is accidentally removed."
                        LinePuts " - If the group is not specified, all pre paths will be hidden."
                        LinePuts "Syntax : SITE EDITPRE HIDEPATH \[group\]"
                        LinePuts "Example: SITE EDITPRE HIDEPATH NX"
                    }
                    default {
                        LinePuts "Description:"
                        LinePuts " - Used to edit pre areas, groups and paths."
                        LinePuts " - For more detailed help, try \"SITE EDITPRE HELP\" <option>"
                        LinePuts "Syntax: SITE EDITPRE <option> <area> \[value\]"
                        LinePuts "Option: addarea delarea addgrp delgrp addpath delpath hidepath view"
                        LinePuts "Areas : [lsort -ascii [array names prearea]]"
                    }
                }
            }
        }
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

PreMain [expr {[info exists args] ? $args : ""}]
