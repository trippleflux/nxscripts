################################################################################
# nxTools - Pre Script                                                         #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Pre {
    namespace import -force ::nxLib::*
}

# Pre Procedures
######################################################################

proc ::nxTools::Pre::ConfigRead {configFile} {
    upvar ConfigComments ConfigComments prearea prearea pregrp pregrp prepath prepath
    if {[catch {set handle [open $configFile r]} error]} {
        ErrorLog PreConfigRead $error
        LinePuts "Unable to load the pre configuration, contact a siteop."
        return 0
    }
    set configComments ""
    set configSection -1

    while {![eof $handle]} {
        set line [string trim [gets $handle]]
        if {![string length $line]} {continue}

        if {[string index $line 0] eq "#"} {
            append configComments $line "\n"; continue
        }
        if {[string match {\[*\]} $line]} {
            set configSection [lsearch -exact {[AREAS] [GROUPS] [PATHS]} $line]
        } else {
            switch -- $configSection {
                0 {set prearea([lindex $line 0]) [lindex $line 1]}
                1 {set pregrp([lindex $line 0]) [lindex $line 1]}
                2 {set prepath([lindex $line 0]) [lrange $line 1 end]}
            }
        }
    }
    close $handle
    return 1
}

proc ::nxTools::Pre::ConfigWrite {configFile} {
    upvar ConfigComments ConfigComments prearea prearea pregrp pregrp prepath prepath
    if {![catch {set handle [open $configFile w]} error]} {
        puts $handle $configComments
        puts $handle "\[AREAS\]"
        foreach {vame value} [array get prearea] {
            puts $handle "$name \"$value\""
        }
        puts $handle "\n\[GROUPS\]"
        foreach {vame value} [array get pregrp] {
            puts $handle "$name \"[lsort -ascii $value]\""
        }
        puts $handle "\n\[PATHS\]"
        foreach {vame value} [array get prepath] {
            puts $handle "$name \"[join $value {" "}]\""
        }
        close $handle
    } else {ErrorLog PreConfigWrite $error}
}

proc ::nxTools::Pre::ResolvePath {userName groupName realPath} {
    set bestMatch 0
    set resolvePath "/"; set vfsFile ""
    set realPath [string map {\\ /} $realPath]

    # Find the user VFS file.
    if {[userfile open $userName] == 0} {
        set userFile [userfile bin2ascii]
        foreach line [split $userFile "\r\n"] {
            if {[string equal -nocase "vfsfile" [lindex $line 0]]} {
                set vfsFile [ArgRange $line 1 end]; break
            }
        }
    }
    # Use the group VFS file if the user VFS file does not exist.
    if {![file isfile $vfsFile] && [groupfile open $groupName] == 0} {
        set groupFile [groupfile bin2ascii]
        foreach line [split $groupFile "\r\n"] {
            if {[string equal -nocase "vfsfile" [lindex $line 0]]} {
                set vfsFile [ArgRange $line 1 end]; break
            }
        }
    }
    # Use the default VFS file if both the user and group VFS file do not exist.
    if {![file isfile $vfsFile]} {
        set vfsFile [config read "Locations" "Default_Vfs"]
    }
    if {![catch {set handle [open $vfsFile r]} error]} {
        while {![eof $handle]} {
            set line [string trim [gets $handle]]
            if {![string length $line]} {continue}
            foreach {basePath mountPath} [string map {\\ /} $line] {break}

            if {[string first [string tolower $basePath] [string tolower $realPath]] == 0} {
                # Use the longest available mount path, improves more accuracy.
                if {[set length [string length $basePath]] > $bestMatch} {
                    set resolvePath [string range $realPath [set bestMatch $length] end]
                    set resolvePath [file join $mountPath [string trimleft $resolvePath "/"]]
                }
            }
        }
        close $handle
    } else {
        ErrorLog PreResolvePath $error
        ErrorReturn "Unable to resolve virtual path, contact a siteop."
    }
    return $resolvePath
}

proc ::nxTools::Pre::UpdateLinks {virtualPath} {
    global latest
    if {[ListMatch $latest(Exempts) $virtualPath]} {
        return 0
    }
    if {[catch {DbOpenFile [namespace current]::LinkDb "Links.db"} error]} {
        ErrorLog PreLinks $error
        return 1
    }
    # Format and create link directory.
    set tagName [file tail $virtualPath]
    if {$latest(MaxLength) > 0 && [string length $tagName] > $latest(MaxLength)} {
        set tagName [string trimright [string range $tagName 0 $latest(MaxLength)] "."]
    }
    set tagName [string map [list %(release) $tagName] $latest(PreTag)]
    set timeStamp [clock seconds]
    LinkDb eval {INSERT INTO Links(TimeStamp,LinkType,DirName) VALUES($timeStamp,1,$tagName)}
    set tagName [file join $latest(SymPath) $tagName]
    if {![catch {file mkdir $tagName} error]} {
        catch {vfs chattr $tagName 1 $virtualPath}
    } else {ErrorLog PreLinksMkDir $error}

    # Remove older links.
    if {[set linkCount [LinkDb eval {SELECT count(*) FROM Links WHERE LinkType=1}]] > $latest(PreLinks)} {
        set linkCount [expr {$linkCount - $latest(PreLinks)}]
        LinkDb eval "SELECT DirName,rowid FROM Links WHERE LinkType=1 ORDER BY TimeStamp ASC LIMIT $linkCount" values {
            RemoveTag [file join $latest(SymPath) $values(DirName)]
            LinkDb eval {DELETE FROM Links WHERE rowid=$values(rowid)}
        }
    }
    LinkDb close
    return 0
}

proc ::nxTools::Pre::UpdateUser {userName files size creditSection statSection} {
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
            set creditsDiff [expr {wide($size) * $ratio}]
            set creditsNew [expr {wide($creditsOld) + wide($creditsDiff)}]
        } else {
            set creditsDiff 0
            set creditsNew $creditsOld
        }
        foreach line $userFile {
            set type [string tolower [lindex $line 0]]
            if {[lsearch -exact {allup dayup monthup wkup} $type] != -1} {
                set newFiles [expr {wide([lindex $line $statSection]) + $files}]
                set newStats [expr {wide([lindex $line [expr {$statSection + 1}]]) + wide($size)}]
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

# Pre Events
######################################################################

proc ::nxTools::Pre::Edit {argList} {
    global pre
    if {![ConfigRead $pre(ConfigFile)]} {return 1}
    if {![llength $argList]} {
        LinePuts "Syntax: SITE EDITPRE <option> <area> \[value\]"
        LinePuts "Option: addarea delarea addgrp delgrp addpath delpath hidepath view"
        LinePuts "Areas : [lsort -ascii [array names prearea]]"
        return 1
    }
    foreach {option target value} $argList {break}

    set preArea [string toupper $target]
    set option [string tolower $option]
    switch -- $option {
        {addarea} {
            if {![string length $target]} {
                ErrorReturn "Invalid area, you must specify an area to add."
            } elseif {[info exists prearea($preArea)]} {
                ErrorReturn "The \"$preArea\" area already exists, delete it first."
            } elseif {![string length $value]} {
                ErrorReturn "The \"$preArea\" area must have a destination path."
            }

            set realPath [resolve pwd $value]
            if {[string index $value 0] ne "/" || ![file isdirectory $realPath]} {
                LinePuts "The destination path \"$value\" is invalid or does not exist."
                ErrorReturn "Note: Make sure the path begins with a forward slash."
            } elseif {[string index $realPath end] ne "/"} {append realPath "/"}

            if {[string first "%" $other] != -1} {
                set other [string trim $other "/"]
                append realPath $other "/"
            } elseif {[string length $other]} {
                LinePuts "Invalid destination date cookie(s) \"$other\"."
                ErrorReturn "Note: Try \"SITE EDITPRE HELP ADDAREA\" to view valid date cookies."
            }
            set prearea($preArea) $realPath
            set pregrp($preArea) ""
            LinePuts "Created area \"$preArea\", destination set to \"$value$other\"."
            LinePuts "Note: Add groups to the area so others may pre to it."
            ConfigWrite $pre(ConfigFile)
        }
        {delarea} {
            if {![string length $target] || ![info exists prearea($preArea)]} {
                ErrorReturn "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
            }
            unset -nocomplain prearea($preArea) pregrp($preArea)
            LinePuts "Removed the area \"$preArea\" and all related settings."
            ConfigWrite $pre(ConfigFile)
        }
        {addgrp} {
            if {![string length $target] || ![info exists prearea($preArea)]} {
                ErrorReturn "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
            } elseif {![string length $value]} {
                ErrorReturn "Invalid group, you must specify a group to add."
            }

            # Check if the group already exists in the area.
            if {![info exists pregrp($preArea)]} {set pregrp($preArea) ""}
            foreach groupEntry $pregrp($preArea) {
                if {[string equal -nocase $groupEntry $value]} {
                    ErrorReturn "The group \"$value\" already exists in the \"$preArea\" allow list."
                }
            }
            lappend pregrp($preArea) $value
            LinePuts "Added the group \"$value\" to the \"$preArea\" allow list."
            ConfigWrite $pre(ConfigFile)
        }
        {delgrp} {
            if {![string length $target] || ![info exists prearea($preArea)]} {
                ErrorReturn "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
            } elseif {![string length $value]} {
                ErrorReturn "Invalid group, you must specify a group to remove."
            }
            set deleted 0; set index 0
            if {![info exists pregrp($preArea)]} {set pregrp($preArea) ""}
            foreach groupEntry $pregrp($preArea) {
                if {[string equal -nocase $groupEntry $value]} {
                    set pregrp($preArea) [lreplace $pregrp($preArea) $index $index]
                    set deleted 1; break
                }
                incr index
            }
            if {$deleted} {
                LinePuts "Removed the group \"$value\" from the \"$preArea\" allow list."
                ConfigWrite $pre(ConfigFile)
            } else {
                LinePuts "The group \"$value\" does not exist in the \"$preArea\" allow list."
            }
        }
        {addpath} {
            if {![string length $target]} {
                ErrorReturn "Invalid group, try \"SITE EDITPRE HELP ADDPATH\" for help."
            } elseif {![string length $value]} {
                ErrorReturn "Invalid path, you must specify a path where group can pre from."
            }

            set realPath [resolve pwd $value]
            if {[string index $value 0] ne "/" || ![file isdirectory $realPath]} {
                LinePuts "The path \"$value\" is invalid or does not exist."
                ErrorReturn "Note: Make sure the path has both leading and trailing slashes."
            } elseif {[string index $value end] ne "/"} {append value "/"}

            # Check if the path already defined for that group.
            if {![info exists prepath($target)]} {set prepath($target) ""}
            foreach pathEntry $prepath($target) {
                if {[string equal -nocase $pathEntry $value]} {
                    ErrorReturn "The path \"$value\" is already defined for the \"$target\" group."
                }
            }
            lappend prepath($target) $value
            catch {vfs chattr $realPath 0 [string map [list %(group) $target] $pre(PrivatePath)]}
            LinePuts "Added path \"$value\" to the \"$target\" group."
            ConfigWrite $pre(ConfigFile)
        }
        {delpath} {
            if {![string length $target]} {
                ErrorReturn "Invalid group, try \"SITE EDITPRE HELP DELPATH\" for help."
            } elseif {![string length $value]} {
                ErrorReturn "Invalid path, you must specify a path to delete."
            }
            set deleted 0; set index 0
            if {[info exists prepath($target)]} {
                foreach pathEntry $prepath($target) {
                    if {[string equal -nocase $pathEntry $value]} {
                        set prepath($target) [lreplace $prepath($target) $index $index]
                        set deleted 1; break
                    }
                    incr index
                }
                if {![string length $prepath($target)]} {unset prepath($target)}
            }
            if {$deleted} {
                LinePuts "Removed path \"$value\" for the \"$target\" group."
                ConfigWrite $pre(ConfigFile)
            } else {
                LinePuts "The path \"$value\" is not defined for the \"$target\" group."
            }
        }
        {hidepath} - {hidepaths} {
            if {![string length $target]} {
                set target "*"
            } elseif {![info exists prepath($target)]} {
                ErrorReturn "Invalid group, try \"SITE EDITPRE HELP HIDEPATH\" for help."
            }
            foreach name [lsort -ascii [array names prepath]] {
                if {![string match $target $name]} {continue}
                LinePuts "Hiding pre paths for: $name"
                set privPath [string map [list %(group) $name] $pre(PrivatePath)]

                foreach pathEntry $prepath($name) {
                    set realPath [resolve pwd $pathEntry]
                    if {[file exists $realPath]} {
                        catch {vfs chattr $realPath 0 $privPath}
                    } else {
                        LinePuts "- The vpath \"$pathEntry\" does not exist."
                    }
                }
            }
        }
        {view} {
            LinePuts "Areas:"
            foreach name [lsort -ascii [array names prearea]] {
                LinePuts [format "%-10s - %s" $name $prearea($name)]
            }
            LinePuts ""; LinePuts "Groups:"
            foreach name [lsort -ascii [array names pregrp]] {
                LinePuts [format "%-10s - %s" $name $pregrp($name)]
            }
            LinePuts ""; LinePuts "Paths:"
            foreach name [lsort -ascii [array names prepath]] {
                LinePuts [format "%-10s - %s" $name $prepath($name)]
            }
        }
        default {
            set option [string tolower $target]
            switch -- $option {
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

proc ::nxTools::Pre::History {argList} {
    if {![GetOptions $argList maxResults Pattern]} {
        iputs "Syntax: SITE PRE HISTORY \[-max <limit>\] \[group\]"
        return 1
    }
    if {[string length $pattern]} {
        set whereClause "WHERE GroupName LIKE '[SqlWildToLike $pattern]' ESCAPE '\\'"
    } else {
        set whereClause ""
    }

    iputs ".-\[PreHistory\]-----------------------------------------------------------."
    iputs "| # |  Release                                              |  Amount   |"
    iputs "|------------------------------------------------------------------------|"
    set count 0
    if {![catch {DbOpenFile [namespace current]::PreDb "Pres.db"} error]} {
        PreDb eval "SELECT Release,Size FROM Pres $whereClause ORDER BY TimeStamp DESC LIMIT $maxResults" values {
            iputs [format "| %02d | %-53.53s | %9s |" [incr count] $values(Release) [FormatSize $values(Size)]]
        }
        PreDb close
    } else {ErrorLog PreHistory $error}

    if {!$count} {LinePuts "There are no pres to display."}
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Pre::Stats {argList} {
    if {![GetOptions $argList maxResults Pattern]} {
        iputs "Syntax: SITE PRE STATS \[-max <limit>\] \[group\]"
        return 1
    }
    if {[string length $pattern]} {
        set whereClause "WHERE GroupName LIKE '[SqlWildToLike $pattern]' ESCAPE '\\'"
    } else {
        set whereClause ""
    }

    iputs ".-\[PreStats\]-------------------------------------------------------------."
    iputs "| # |  Group                        |   Pres    |   Files   |  Amount   |"
    iputs "|------------------------------------------------------------------------|"
    set count 0
    if {![catch {DbOpenFile [namespace current]::PreDb "Pres.db"} error]} {
        PreDb eval "SELECT GroupName, count(*) AS Pres, round(sum(Files)) AS Files, sum(Size) AS Amount FROM Pres $whereClause GROUP BY GroupName ORDER BY Pres DESC LIMIT $maxResults" values {
            iputs [format "| %02d | %-29.29s | %9d | %8dF | %9s |" [incr count] $values(GroupName) $values(Pres) $values(Files) [FormatSize $values(Amount)]]
        }
        PreDb close
    } else {ErrorLog PreStats $error}

    if {!$count} {LinePuts "There are no statistics to display."}
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Pre::Release {argList} {
    global dupe latest misc mysql pre pretime group groups pwd user
    if {![ConfigRead $pre(ConfigFile)]} {return 1}
    set preArea [string toupper [lindex $argList 0]]
    set target [lindex $argList 1]

    # Check area and group paths.
    if {![info exists prearea($preArea)] || ![string length $prearea($preArea)]} {
        ErrorReturn "Invalid area, try \"SITE PRE HELP\" to view available areas."
    } elseif {![info exists pregrp($preArea)]} {
        ErrorReturn "The specified area has no group list defined."
    }

    # Check if group is allowed to pre to the area and from this path.
    set allowPath 0; set preGroup ""
    set virtualPath [GetPath $pwd $target]
    foreach groupName $groups {
        if {[lsearch -exact $pregrp($preArea) $groupName] != -1} {
            set preGroup $groupName
            if {![info exists prepath($groupName)]} {continue}
            foreach pathEntry $prepath($groupName) {
                set pathEntry [string trimright $pathEntry "/"]
                set pathLength [string length $pathEntry]
                if {[string equal -length $pathLength -nocase $pathEntry $virtualPath]} {
                    set allowPath 1; break
                }
            }
            if {$allowPath} {break}
        }
    }
    if {![string length $preGroup]} {
        ErrorReturn "Your group(s) are not allowed to pre to the \"$preArea\" area."
    } elseif {!$allowPath} {
        ErrorReturn "Your group \"$preGroup\" is not allowed to pre from this path."
    }

    # Check if the specified directory exists.
    set realPath [resolve pwd $virtualPath]
    if {![string length $target] || ![file isdirectory $realPath]} {
        ErrorReturn "The specified directory does not exist."
    }
    set diskCount 0; set files 0; set totalSize 0
    set release [file tail $virtualPath]

    # Find destination path.
    set destRealPath $prearea($preArea)
    set preTime [clock seconds]
    set destRealPath [clock format $preTime -format $destRealPath -gmt [IsTrue $misc(UtcTime)]]
    if {![file isdirectory $destRealPath]} {
        LinePuts "The pre destination path for the \"$preArea\" area does not exist."
        ErrorReturn "Note: If the area uses dated dirs, check that today's date dir exists."
    }
    set destRealPath [file join $destRealPath $release]
    if {[file exists $destRealPath]} {ErrorReturn "A file or directory by that name already exists in the target area."}

    # Find the credit and stats section.
    set destVirtualPath [ResolvePath $user $group $destRealPath]
    ListAssign [GetCreditStatSections $destVirtualPath] creditSection statSection

    # Count disk sub-directories.
    foreach listItem [glob -nocomplain -types d -directory $realPath "*"] {
        if {[IsDiskPath $listItem]} {incr diskCount}
    }

    # Count files and total size.
    GetDirList $realPath dirlist ".ioFTPD*"
    foreach listItem $dirlist(FileList) {
        incr files; set fileSize [file size $listItem]
        set fileSize [file size $listItem]
        set totalSize [expr {wide($totalSize) + wide($fileSize)}]
        if {[IsTrue $pre(Uploaders)]} {
            catch {lindex [vfs read $listItem] 0} userId
            if {$fileSize > 0 && [set userName [resolve uid $userId]] ne ""} {
                if {[info exists prefiles($userName)]} {
                    incr prefiles($userName)
                } else {set prefiles($userName) 1}

                if {[info exists presize($userName)]} {
                    set presize($userName) [expr {wide($presize($userName)) + wide($fileSize)}]
                } else {set presize($userName) $fileSize}
            }
        }
    }
    set totalSize [expr {wide($totalSize) / 1024}]

    # If the pre destination is a different drive, ensure that there
    # is adequate space for the release (and a 10MB "safety" buffer).
    set sourceDrive [lindex [file split $realPath] 0]
    set destDrive [lindex [file split $destRealPath] 0]

    if {![string equal -nocase $sourceDrive $destDrive]} {
        if {[catch {::nx::volume info $destDrive volume} error]} {
            ErrorLog PreCheckSpace $error
            ErrorReturn "Unable to check available space on target drive, contact a siteop."
        }
        set checkSize [expr {wide($totalSize) + (10*1024)}]
        set volume(free) [expr {wide($volume(free)) / 1024}]

        if {$checkSize > $volume(free)} {
            ErrorLog PreLowSpace "unable to pre \"$realPath\": insufficient disk space on $destDrive ([FormatSize $volume(free)] free, needed [FormatSize $checkSize])"
            ErrorReturn "Insufficient disk space on target drive, contact a siteop."
        }
    }
    LinePuts "Area    : $preArea"
    LinePuts "Release : $release"
    LinePuts "Files   : [format %-16s ${Files}F] Size: [format %-16s [FormatSize $totalSize]] CDs: $diskCount"

    # Move release to the destination path.
    KickUsers [file join $virtualPath "*"]
    if {[catch {file rename -force -- $realPath $destRealPath} error]} {
        ErrorLog PreMove $error
        ErrorReturn "Error   : Unable to move directory, aborting."
    }

    set isMP3 0
    set filePath [expr {!$diskCount ? "*.mp3" : "*/*.mp3"}]
    set mP3Files [glob -nocomplain -types f -directory $destRealPath $filePath]

    # Attempt to parse every MP3 file until successful.
    foreach filePath $mP3Files {
        if {[catch {::nx::mp3 $filePath mp3} error]} {
            ErrorLog PreMP3 $error
        } else {
            set codec [format "MPEG %.1f Layer %d" $mp3(version) $mp3(layer)]
            iputs "|------------------------------------------------------------------------|"
            iputs [format "| Artist  : %-60s |" $mp3(artist)]
            iputs [format "| Album   : %-60s |" $mp3(album)]
            iputs [format "| Genre   : %-16s Year: %-11s Bitrate: %-16s |" $mp3(genre) $mp3(year) "$mp3(bitrate) Kbit"]
            iputs [format "| Channel : %-16s Type: %-13s Codec: %-16s |" $mp3(mode) $mp3(type) $codec]
            set isMP3 1; break
        }
    }

    # Change file and directory ownership.
    if {[string is digit -strict $pre(ChownUserId)]} {
        GetDirList $destRealPath dirlist ".ioFTPD*"
        foreach listItem $dirlist(DirList) {
            catch {vfs read $listItem} owner
            ListAssign $owner userId groupId Chmod
            # Verify the group and chmod the directory.
            if {![string is digit -strict $groupId]} {set groupId [lindex $misc(DirOwner) 1]}
            if {![string is digit -strict $chmod]} {set chmod $misc(DirChmod)}
            catch {vfs write $listItem $pre(ChownUserId) $groupId $chmod}
        }
        foreach listItem $dirlist(FileList) {
            catch {vfs read $listItem} owner
            ListAssign $owner userId groupId Chmod
            # Verify the group and chmod the file.
            if {![string is digit -strict $groupId]} {set groupId [lindex $misc(FileOwner) 1]}
            if {![string is digit -strict $chmod]} {set chmod $misc(FileChmod)}
            catch {vfs write $listItem $pre(ChownUserId) $groupId $chmod}
        }
    }
    # Update file and directory times.
    if {[IsTrue $pre(TouchTimes)]} {
        if {[catch {::nx::touch -recurse $destRealPath $preTime} error]} {
            ErrorLog PreTouch $error
        }
    }
    catch {vfs flush [file dirname $realPath]}
    catch {vfs flush [file dirname $destRealPath]}

    if {[IsTrue $pre(PreUser)]} {
        UpdateUser $user 0 0 $creditSection $statSection
    }
    if {[IsTrue $pre(Uploaders)]} {
        iputs "|------------------------------------------------------------------------|"
        iputs "|    User    |   Group    |   Ratio    |  Files   |  Amount  |  Credits  |"
        iputs "|------------------------------------------------------------------------|"
        foreach userName [lsort -ascii [array names presize]] {
            set uploadAmount [expr {wide($presize($userName)) / 1024}]
            set result [UpdateUser $userName $prefiles($userName) $uploadAmount $creditSection $statSection]
            foreach {groupName ratio creditsOld creditsNew creditsDiff} $result {break}
            set ratio [expr {$ratio != 0 ? "1:$ratio" : "Unlimited"}]
            iputs [format "| %-10s | %-10s | %10s | %7d\F | %8s | %9s |" $userName $groupName $ratio $prefiles($userName) [FormatSize $uploadAmount] [FormatSize $creditsDiff]]
        }
    }
    if {[IsTrue $misc(dZSbotLogging)]} {
        set totalMB [format "%.2f" [expr {double($totalSize) / 1024.0}]]
        set logLine "PRE: \"$destVirtualPath\" \"$preGroup\" \"$user\" \"$group\" \"$preArea\" \"$files\" \"$totalMB\" \"$diskCount\""
        if {$isMP3} {append logLine " \"$mp3(genre)\" \"$mp3(bitrate)\" \"$mp3(year)\""}
    } else {
        set logLine "PRE: \"$destVirtualPath\" \"$preGroup\" \"$user\" \"$group\" \"$preArea\" \"$files\" \"$totalSize\" \"$diskCount\""
        if {$isMP3} {append logLine " \"$mp3(artist)|$mp3(album)|$mp3(genre)|$mp3(year)|$mp3(bitrate)|$mp3(type)\""}
    }
    putlog $logLine

    if {![catch {DbOpenFile [namespace current]::PreDb "Pres.db"} error]} {
        PreDb eval {INSERT INTO Pres(TimeStamp,UserName,GroupName,Area,Release,Files,Size) VALUES($preTime,$user,$preGroup,$preArea,$release,$files,$totalSize)}
        PreDb close
    } else {ErrorLog PreDb $error}

    if {[IsTrue $dupe(AddOnPre)]} {
        if {![catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
            set logUser [expr {$pre(ChownUserId) ne "" ? [resolve uid $pre(ChownUserId)] : $user}]
            set logPath [string range $destVirtualPath 0 [string last "/" $destVirtualPath]]
            DirDb eval {INSERT INTO DupeDirs(TimeStamp,UserName,GroupName,DirPath,DirName) VALUES($preTime,$logUser,$preGroup,$logPath,$release)}
            DirDb close
        } else {ErrorLog PreDupeDb $error}
    }

    if {[IsTrue $pretime(AddOnPre)] && [MySqlConnect]} {
        set preArea [::mysql::escape $preArea]
        set release [::mysql::escape $release]
        if {[catch {::mysql::exec $mysql(ConnHandle) "INSERT INTO $mysql(TableName) (pretime,section,release,files,kbytes,disks) VALUES('$preTime','$preArea','$release','$files','$totalSize','$diskCount')"} error]} {
            if {[string first "Duplicate entry" $error] == -1} {ErrorLog PreAddToDb $error}
        }
        MySqlClose
    }
    # Create latest pre symlinks.
    if {$latest(PreLinks) > 0} {UpdateLinks $destVirtualPath}
    return 0
}

# Pre Main
######################################################################

proc ::nxTools::Pre::Main {argv} {
    global misc pre ioerror
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set result 0

    set argLength [llength [set argList [ArgList $argv]]]
    set event [string toupper [lindex $argList 0]]
    if {$event eq "PRE"} {
        set subEvent [string toupper [lindex $argList 1]]
        if {$subEvent eq "HELP" || $argLength < 2} {
            iputs ".-\[Pre\]------------------------------------------------------------------."
            if {[ConfigRead $pre(ConfigFile)]} {
                LinePuts "Syntax: SITE PRE <area> <directory>"
                LinePuts "        SITE PRE HISTORY \[-max <limit>\] \[group\]"
                LinePuts "        SITE PRE STATS \[-max <limit>\] \[group\]"
                LinePuts "Areas : [lsort -ascii [array names prearea]]"
            }
            iputs "'------------------------------------------------------------------------'"
        } elseif {$subEvent eq "HISTORY"} {
            set result [History [lrange $argList 2 end]]
        } elseif {$subEvent eq "STATS"} {
            set result [Stats [lrange $argList 2 end]]
        } else {
            iputs ".-\[Pre\]------------------------------------------------------------------."
            set result [Release [lrange $argList 1 end]]
            iputs "'------------------------------------------------------------------------'"
        }
    } elseif {$event eq "EDIT"} {
        iputs ".-\[EditPre\]--------------------------------------------------------------."
        set result [Edit [lrange $argList 1 end]]
        iputs "'------------------------------------------------------------------------'"
    } else {
        ErrorLog InvalidArgs "unknown event \"[info script] $event\": check your ioFTPD.ini for errors"
        set result 1
    }
    return [set ioerror $result]
}

::nxTools::Pre::Main [expr {[info exists args] ? $args : ""}]
