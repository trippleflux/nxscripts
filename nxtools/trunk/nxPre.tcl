################################################################################
# nxTools - Pre Script                                                         #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} ErrorMsg]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $ErrorMsg
}

namespace eval ::nxTools::Pre {
    namespace import -force ::nxLib::*
}

# Pre Procedures
######################################################################

proc ::nxTools::Pre::ConfigRead {ConfigFile} {
    upvar ConfigComments ConfigComments prearea prearea pregrp pregrp prepath prepath
    if {[catch {set Handle [open $ConfigFile r]} ErrorMsg]} {
        ErrorLog PreConfigRead $ErrorMsg
        LinePuts "Unable to load the pre configuration, contact a siteop."
        return 0
    }
    set ConfigComments ""
    set ConfigSection -1

    while {![eof $Handle]} {
        set FileLine [string trim [gets $Handle]]
        if {![string length $FileLine]} {continue}

        if {[string index $FileLine 0] eq "#"} {
            append ConfigComments $FileLine "\n"; continue
        }
        if {[string match {\[*\]} $FileLine]} {
            set ConfigSection [lsearch -exact {[AREAS] [GROUPS] [PATHS]} $FileLine]
        } else {
            switch -- $ConfigSection {
                0 {set prearea([lindex $FileLine 0]) [lindex $FileLine 1]}
                1 {set pregrp([lindex $FileLine 0]) [lindex $FileLine 1]}
                2 {set prepath([lindex $FileLine 0]) [lrange $FileLine 1 end]}
            }
        }
    }
    close $Handle
    return 1
}

proc ::nxTools::Pre::ConfigWrite {ConfigFile} {
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
    } else {ErrorLog PreConfigWrite $ErrorMsg}
}

proc ::nxTools::Pre::ResolvePath {UserName GroupName RealPath} {
    set BestMatch 0
    set ResolvePath "/"; set VfsFile ""
    set RealPath [string map {\\ /} $RealPath]

    ## Find the user VFS file.
    if {[userfile open $UserName] == 0} {
        set UserFile [userfile bin2ascii]
        foreach UserLine [split $UserFile "\r\n"] {
            if {[string equal -nocase "vfsfile" [lindex $UserLine 0]]} {
                set VfsFile [ArgRange $UserLine 1 end]; break
            }
        }
    }
    ## Use the group VFS file if the user vfs file does not exist.
    if {![file isfile $VfsFile] && [groupfile open $GroupName] == 0} {
        set GroupFile [groupfile bin2ascii]
        foreach GroupLine [split $GroupFile "\r\n"] {
            if {[string equal -nocase "vfsfile" [lindex $UserLine 0]]} {
                set VfsFile [ArgRange $GroupLine 1 end]; break
            }
        }
    }
    ## Use the default VFS file if both the user and group VFS file do not exist.
    if {![file isfile $VfsFile]} {
        set VfsFile [config read "Locations" "Default_Vfs"]
    }
    if {![catch {set Handle [open $VfsFile r]} ErrorMsg]} {
        while {![eof $Handle]} {
            set FileLine [string trim [gets $Handle]]
            if {![string length $FileLine]} {continue}
            foreach {VfsRealPath VfsVirtualPath} [string map {\\ /} $FileLine] {break}

            if {[string first [string tolower $VfsRealPath] [string tolower $RealPath]] == 0} {
                ## Use the longest available mount path, improves more accuracy.
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

proc ::nxTools::Pre::UpdateLinks {VirtualPath} {
    global latest
    if {[ListMatch $latest(Exempts) $VirtualPath]} {
        return 0
    }
    if {[catch {DbOpenFile [namespace current]::LinkDb "Links.db"} ErrorMsg]} {
        ErrorLog PreLinks $ErrorMsg
        return 1
    }
    ## Format and create link directory.
    set TagName [file tail $VirtualPath]
    if {$latest(MaxLength) > 0 && [string length $TagName] > $latest(MaxLength)} {
        set TagName [string trimright [string range $TagName 0 $latest(MaxLength)] "."]
    }
    set TagName [string map [list %(release) $TagName] $latest(PreTag)]
    set TimeStamp [clock seconds]
    LinkDb eval {INSERT INTO Links(TimeStamp,LinkType,DirName) VALUES($TimeStamp,1,$TagName)}
    set TagName [file join $latest(SymPath) $TagName]
    if {![catch {file mkdir $TagName} ErrorMsg]} {
        catch {vfs chattr $TagName 1 $VirtualPath}
    } else {ErrorLog PreLinksMkDir $ErrorMsg}

    ## Remove older links.
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

proc ::nxTools::Pre::UpdateUser {UserName Files Size CreditSection StatSection} {
    set CreditSection [expr {$CreditSection + 1}]
    set StatSection [expr {$StatSection * 3 + 1}]
    set GroupName "NoGroup"
    set NewUserFile ""

    if {[userfile open $UserName] == 0} {
        userfile lock
        set UserFile [split [userfile bin2ascii] "\r\n"]
        foreach UserLine $UserFile {
            set LineType [string tolower [lindex $UserLine 0]]
            if {$LineType eq "credits"} {
                set OldCredits [lindex $UserLine $CreditSection]
            } elseif {$LineType eq "groups"} {
                set GroupName [GetGroupName [lindex $UserLine 1]]
            } elseif {$LineType eq "ratio"} {
                set Ratio [lindex $UserLine $CreditSection]
            }
        }

        if {$Ratio != 0} {
            set DiffCredits [expr {wide($Size) * $Ratio}]
            set NewCredits [expr {wide($OldCredits) + wide($DiffCredits)}]
        } else {
            set DiffCredits 0
            set NewCredits $OldCredits
        }
        foreach UserLine $UserFile {
            set LineType [string tolower [lindex $UserLine 0]]
            if {[lsearch -exact {allup dayup monthup wkup} $LineType] != -1} {
                set NewFiles [expr {wide([lindex $UserLine $StatSection]) + $Files}]
                set NewStats [expr {wide([lindex $UserLine [expr {$StatSection + 1}]]) + wide($Size)}]
                set UserLine [lreplace $UserLine $StatSection [expr {$StatSection + 1}] $NewFiles $NewStats]
            } elseif {$LineType eq "credits"} {
                set UserLine [lreplace $UserLine $CreditSection $CreditSection $NewCredits]
            }
            append NewUserFile $UserLine "\r\n"
        }
        userfile ascii2bin $NewUserFile
        userfile unlock
    }
    return [list $GroupName $Ratio $OldCredits $NewCredits $DiffCredits]
}

# Pre Events
######################################################################

proc ::nxTools::Pre::Edit {ArgList} {
    global pre
    if {![ConfigRead $pre(ConfigFile)]} {return 1}
    if {![llength $ArgList]} {
        LinePuts "Syntax: SITE EDITPRE <option> <area> \[value\]"
        LinePuts "Option: addarea delarea addgrp delgrp addpath delpath hidepath view"
        LinePuts "Areas : [lsort -ascii [array names prearea]]"
        return 1
    }
    foreach {Option Target Value} $ArgList {break}

    set PreArea [string toupper $Target]
    set Option [string tolower $Option]
    switch -- $Option {
        {addarea} {
            if {![string length $Target]} {
                ErrorReturn "Invalid area, you must specify an area to add."
            } elseif {[info exists prearea($PreArea)]} {
                ErrorReturn "The \"$PreArea\" area already exists, delete it first."
            } elseif {![string length $Value]} {
                ErrorReturn "The \"$PreArea\" area must have a destination path."
            }

            set RealPath [resolve pwd $Value]
            if {[string index $Value 0] ne "/" || ![file isdirectory $RealPath]} {
                LinePuts "The destination path \"$Value\" is invalid or does not exist."
                ErrorReturn "Note: Make sure the path begins with a forward slash."
            } elseif {[string index $RealPath end] ne "/"} {append RealPath "/"}

            if {[string first "%" $Other] != -1} {
                set Other [string trim $Other "/"]
                append RealPath $Other "/"
            } elseif {[string length $Other]} {
                LinePuts "Invalid destination date cookie(s) \"$Other\"."
                ErrorReturn "Note: Try \"SITE EDITPRE HELP ADDAREA\" to view valid date cookies."
            }
            set prearea($PreArea) $RealPath
            set pregrp($PreArea) ""
            LinePuts "Created area \"$PreArea\", destination set to \"$Value$Other\"."
            LinePuts "Note: Add groups to the area so others may pre to it."
            ConfigWrite $pre(ConfigFile)
        }
        {delarea} {
            if {![string length $Target] || ![info exists prearea($PreArea)]} {
                ErrorReturn "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
            }
            unset -nocomplain prearea($PreArea) pregrp($PreArea)
            LinePuts "Removed the area \"$PreArea\" and all related settings."
            ConfigWrite $pre(ConfigFile)
        }
        {addgrp} {
            if {![string length $Target] || ![info exists prearea($PreArea)]} {
                ErrorReturn "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
            } elseif {![string length $Value]} {
                ErrorReturn "Invalid group, you must specify a group to add."
            }

            ## Check if the group already exists in the area.
            if {![info exists pregrp($PreArea)]} {set pregrp($PreArea) ""}
            foreach GroupEntry $pregrp($PreArea) {
                if {[string equal -nocase $GroupEntry $Value]} {
                    ErrorReturn "The group \"$Value\" already exists in the \"$PreArea\" allow list."
                }
            }
            lappend pregrp($PreArea) $Value
            LinePuts "Added the group \"$Value\" to the \"$PreArea\" allow list."
            ConfigWrite $pre(ConfigFile)
        }
        {delgrp} {
            if {![string length $Target] || ![info exists prearea($PreArea)]} {
                ErrorReturn "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
            } elseif {![string length $Value]} {
                ErrorReturn "Invalid group, you must specify a group to remove."
            }
            set Deleted 0; set Index 0
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
                ConfigWrite $pre(ConfigFile)
            } else {
                LinePuts "The group \"$Value\" does not exist in the \"$PreArea\" allow list."
            }
        }
        {addpath} {
            if {![string length $Target]} {
                ErrorReturn "Invalid group, try \"SITE EDITPRE HELP ADDPATH\" for help."
            } elseif {![string length $Value]} {
                ErrorReturn "Invalid path, you must specify a path where group can pre from."
            }

            set RealPath [resolve pwd $Value]
            if {[string index $Value 0] ne "/" || ![file isdirectory $RealPath]} {
                LinePuts "The path \"$Value\" is invalid or does not exist."
                ErrorReturn "Note: Make sure the path has both leading and trailing slashes."
            } elseif {[string index $Value end] ne "/"} {append Value "/"}

            ## Check if the path already defined for that group.
            if {![info exists prepath($Target)]} {set prepath($Target) ""}
            foreach PathEntry $prepath($Target) {
                if {[string equal -nocase $PathEntry $Value]} {
                    ErrorReturn "The path \"$Value\" is already defined for the \"$Target\" group."
                }
            }
            lappend prepath($Target) $Value
            catch {vfs chattr $RealPath 0 [string map [list %(group) $Target] $pre(PrivatePath)]}
            LinePuts "Added path \"$Value\" to the \"$Target\" group."
            ConfigWrite $pre(ConfigFile)
        }
        {delpath} {
            if {![string length $Target]} {
                ErrorReturn "Invalid group, try \"SITE EDITPRE HELP DELPATH\" for help."
            } elseif {![string length $Value]} {
                ErrorReturn "Invalid path, you must specify a path to delete."
            }
            set Deleted 0; set Index 0
            if {[info exists prepath($Target)]} {
                foreach PathEntry $prepath($Target) {
                    if {[string equal -nocase $PathEntry $Value]} {
                        set prepath($Target) [lreplace $prepath($Target) $Index $Index]
                        set Deleted 1; break
                    }
                    incr Index
                }
                if {![string length $prepath($Target)]} {unset prepath($Target)}
            }
            if {$Deleted} {
                LinePuts "Removed path \"$Value\" for the \"$Target\" group."
                ConfigWrite $pre(ConfigFile)
            } else {
                LinePuts "The path \"$Value\" is not defined for the \"$Target\" group."
            }
        }
        {hidepath} - {hidepaths} {
            if {![string length $Target]} {
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
            switch -- $Option {
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
    return 0
}

proc ::nxTools::Pre::History {ArgList} {
    if {![GetOptions $ArgList MaxResults Pattern]} {
        iputs "Syntax: SITE PRE HISTORY \[-max <limit>\] \[group\]"
        return 1
    }
    if {[string length $Pattern]} {
        set WhereClause "WHERE GroupName LIKE '[SqlWildToLike $Pattern]' ESCAPE '\\'"
    } else {
        set WhereClause ""
    }

    iputs ".-\[PreHistory\]-----------------------------------------------------------."
    iputs "| ## |  Release                                              |  Amount   |"
    iputs "|------------------------------------------------------------------------|"
    set Count 0
    if {![catch {DbOpenFile [namespace current]::PreDb "Pres.db"} ErrorMsg]} {
        PreDb eval "SELECT Release,Size FROM Pres $WhereClause ORDER BY TimeStamp DESC LIMIT $MaxResults" values {
            iputs [format "| %02d | %-53.53s | %9s |" [incr Count] $values(Release) [FormatSize $values(Size)]]
        }
        PreDb close
    } else {ErrorLog PreHistory $ErrorMsg}

    if {!$Count} {LinePuts "There are no pres to display."}
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Pre::Stats {ArgList} {
    if {![GetOptions $ArgList MaxResults Pattern]} {
        iputs "Syntax: SITE PRE STATS \[-max <limit>\] \[group\]"
        return 1
    }
    if {[string length $Pattern]} {
        set WhereClause "WHERE GroupName LIKE '[SqlWildToLike $Pattern]' ESCAPE '\\'"
    } else {
        set WhereClause ""
    }

    iputs ".-\[PreStats\]-------------------------------------------------------------."
    iputs "| ## |  Group                        |   Pres    |   Files   |  Amount   |"
    iputs "|------------------------------------------------------------------------|"
    set Count 0
    if {![catch {DbOpenFile [namespace current]::PreDb "Pres.db"} ErrorMsg]} {
        PreDb eval "SELECT GroupName, count(*) AS Pres, round(sum(Files)) AS Files, sum(Size) AS Amount FROM Pres $WhereClause GROUP BY GroupName ORDER BY Pres DESC LIMIT $MaxResults" values {
            iputs [format "| %02d | %-29.29s | %9d | %8dF | %9s |" [incr Count] $values(GroupName) $values(Pres) $values(Files) [FormatSize $values(Amount)]]
        }
        PreDb close
    } else {ErrorLog PreStats $ErrorMsg}

    if {!$Count} {LinePuts "There are no statistics to display."}
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Pre::Release {ArgList} {
    global dupe latest misc mysql pre pretime group groups pwd user
    if {![ConfigRead $pre(ConfigFile)]} {return 1}
    set PreArea [string toupper [lindex $ArgList 0]]
    set Target [lindex $ArgList 1]

    ## Check area and group paths.
    if {![info exists prearea($PreArea)] || ![string length $prearea($PreArea)]} {
        ErrorReturn "Invalid area, try \"SITE PRE HELP\" to view available areas."
    } elseif {![info exists pregrp($PreArea)]} {
        ErrorReturn "The specified area has no group list defined."
    }

    ## Check if group is allowed to pre to the area and from this path.
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
    if {![string length $PreGroup]} {
        ErrorReturn "Your group(s) are not allowed to pre to the \"$PreArea\" area."
    } elseif {!$AllowPath} {
        ErrorReturn "Your group \"$PreGroup\" is not allowed to pre from this path."
    }

    ## Check if the specified directory exists.
    set RealPath [resolve pwd $VirtualPath]
    if {![string length $Target] || ![file isdirectory $RealPath]} {
        ErrorReturn "The specified directory does not exist."
    }
    set DiskCount 0; set Files 0; set TotalSize 0
    set Release [file tail $VirtualPath]

    ## Find destination path.
    set DestRealPath $prearea($PreArea)
    set PreTime [clock seconds]
    set DestRealPath [clock format $PreTime -format $DestRealPath -gmt [IsTrue $misc(UtcTime)]]
    if {![file isdirectory $DestRealPath]} {
        LinePuts "The pre destination path for the \"$PreArea\" area does not exist."
        ErrorReturn "Note: If the area uses dated dirs, check that today's date dir exists."
    }
    set DestRealPath [file join $DestRealPath $Release]
    if {[file exists $DestRealPath]} {ErrorReturn "A file or directory by that name already exists in the target area."}

    ## Find the credit and stats section.
    set DestVirtualPath [ResolvePath $user $group $DestRealPath]
    ListAssign [GetCreditStatSections $DestVirtualPath] CreditSection StatSection

    ## Count disk sub-directories.
    foreach ListItem [glob -nocomplain -types d -directory $RealPath "*"] {
        if {[IsMultiDisk $ListItem]} {incr DiskCount}
    }

    ## Count files and total size.
    GetDirList $RealPath dirlist ".ioFTPD*"
    foreach ListItem $dirlist(FileList) {
        incr Files; set FileSize [file size $ListItem]
        set FileSize [file size $ListItem]
        set TotalSize [expr {wide($TotalSize) + wide($FileSize)}]
        if {[IsTrue $pre(Uploaders)]} {
            catch {lindex [vfs read $ListItem] 0} UserId
            if {$FileSize > 0 && [set UserName [resolve uid $UserId]] ne ""} {
                if {[info exists prefiles($UserName)]} {
                    incr prefiles($UserName)
                } else {set prefiles($UserName) 1}

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
        if {[catch {::nx::volume info $DestDrive volume} ErrorMsg]} {
            ErrorLog PreCheckSpace $ErrorMsg
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

    ## Move release to the destination path.
    KickUsers [file join $VirtualPath "*"]
    if {[catch {file rename -force -- $RealPath $DestRealPath} ErrorMsg]} {
        ErrorLog PreMove $ErrorMsg
        ErrorReturn "Error   : Unable to move directory, aborting."
    }

    set IsMP3 0
    set FilePath [expr {!$DiskCount ? "*.mp3" : "*/*.mp3"}]
    set MP3Files [glob -nocomplain -types f -directory $DestRealPath $FilePath]

    ## Attempt to parse every MP3 file until successful.
    foreach FilePath $MP3Files {
        if {[catch {::nx::mp3 $FilePath mp3} ErrorMsg]} {
            ErrorLog PreMP3 $ErrorMsg
        } else {
            set Codec [format "MPEG %.1f Layer %d" $mp3(version) $mp3(layer)]
            iputs "|------------------------------------------------------------------------|"
            iputs [format "| Artist  : %-60s |" $mp3(artist)]
            iputs [format "| Album   : %-60s |" $mp3(album)]
            iputs [format "| Genre   : %-16s Year: %-11s Bitrate: %-16s |" $mp3(genre) $mp3(year) "$mp3(bitrate) Kbit"]
            iputs [format "| Channel : %-16s Type: %-13s Codec: %-16s |" $mp3(mode) $mp3(type) $Codec]
            set IsMP3 1; break
        }
    }

    ## Change file and directory ownership.
    if {[string is digit -strict $pre(ChownUserId)]} {
        GetDirList $DestRealPath dirlist ".ioFTPD*"
        foreach ListItem $dirlist(DirList) {
            catch {vfs read $ListItem} VfsOwner
            ListAssign $VfsOwner UserId GroupId Chmod
            ## Verify the group and chmod the directory.
            if {![string is digit -strict $GroupId]} {set GroupId [lindex $misc(DirOwner) 1]}
            if {![string is digit -strict $Chmod]} {set Chmod $misc(DirChmod)}
            catch {vfs write $ListItem $pre(ChownUserId) $GroupId $Chmod}
        }
        foreach ListItem $dirlist(FileList) {
            catch {vfs read $ListItem} VfsOwner
            ListAssign $VfsOwner UserId GroupId Chmod
            ## Verify the group and chmod the file.
            if {![string is digit -strict $GroupId]} {set GroupId [lindex $misc(FileOwner) 1]}
            if {![string is digit -strict $Chmod]} {set Chmod $misc(FileChmod)}
            catch {vfs write $ListItem $pre(ChownUserId) $GroupId $Chmod}
        }
    }
    ## Update file and directory times.
    if {[IsTrue $pre(TouchTimes)]} {
        if {[catch {::nx::touch -recurse $DestRealPath $PreTime} ErrorMsg]} {
            ErrorLog PreTouch $ErrorMsg
        }
    }
    catch {vfs flush [file dirname $RealPath]}
    catch {vfs flush [file dirname $DestRealPath]}

    if {[IsTrue $pre(PreUser)]} {
        UpdateUser $user 0 0 $CreditSection $StatSection
    }
    if {[IsTrue $pre(Uploaders)]} {
        iputs "|------------------------------------------------------------------------|"
        iputs "|    User    |   Group    |   Ratio    |  Files   |  Amount  |  Credits  |"
        iputs "|------------------------------------------------------------------------|"
        foreach UserName [lsort -ascii [array names presize]] {
            set UploadAmount [expr {wide($presize($UserName)) / 1024}]
            set Result [UpdateUser $UserName $prefiles($UserName) $UploadAmount $CreditSection $StatSection]
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

    if {![catch {DbOpenFile [namespace current]::PreDb "Pres.db"} ErrorMsg]} {
        PreDb eval {INSERT INTO Pres(TimeStamp,UserName,GroupName,Area,Release,Files,Size) VALUES($PreTime,$user,$PreGroup,$PreArea,$Release,$Files,$TotalSize)}
        PreDb close
    } else {ErrorLog PreDb $ErrorMsg}

    if {[IsTrue $dupe(AddOnPre)]} {
        if {![catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} ErrorMsg]} {
            set LogUser [expr {$pre(ChownUserId) ne "" ? [resolve uid $pre(ChownUserId)] : $user}]
            set LogPath [string range $DestVirtualPath 0 [string last "/" $DestVirtualPath]]
            DirDb eval {INSERT INTO DupeDirs(TimeStamp,UserName,GroupName,DirPath,DirName) VALUES($PreTime,$LogUser,$PreGroup,$LogPath,$Release)}
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
    ## Create latest pre symlinks.
    if {$latest(PreLinks) > 0} {UpdateLinks $DestVirtualPath}
    return 0
}

# Pre Main
######################################################################

proc ::nxTools::Pre::Main {ArgV} {
    global misc pre ioerror
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set Result 0

    set ArgLength [llength [set ArgList [ArgList $ArgV]]]
    set Event [string toupper [lindex $ArgList 0]]
    if {$Event eq "PRE"} {
        set SubEvent [string toupper [lindex $ArgList 1]]
        if {$SubEvent eq "HELP" || $ArgLength < 2} {
            iputs ".-\[Pre\]------------------------------------------------------------------."
            if {[ConfigRead $pre(ConfigFile)]} {
                LinePuts "Syntax: SITE PRE <area> <directory>"
                LinePuts "        SITE PRE HISTORY \[-max <limit>\] \[group\]"
                LinePuts "        SITE PRE STATS \[-max <limit>\] \[group\]"
                LinePuts "Areas : [lsort -ascii [array names prearea]]"
            }
            iputs "'------------------------------------------------------------------------'"
        } elseif {$SubEvent eq "HISTORY"} {
            set Result [History [lrange $ArgList 2 end]]
        } elseif {$SubEvent eq "STATS"} {
            set Result [Stats [lrange $ArgList 2 end]]
        } else {
            iputs ".-\[Pre\]------------------------------------------------------------------."
            set Result [Release [lrange $ArgList 1 end]]
            iputs "'------------------------------------------------------------------------'"
        }
    } elseif {$Event eq "EDIT"} {
        iputs ".-\[EditPre\]--------------------------------------------------------------."
        set Result [Edit [lrange $ArgList 1 end]]
        iputs "'------------------------------------------------------------------------'"
    } else {
        ErrorLog InvalidArgs "unknown event \"[info script] $Event\": check your ioFTPD.ini for errors"
        set Result 1
    }
    return [set ioerror $Result]
}

::nxTools::Pre::Main [expr {[info exists args] ? $args : ""}]
