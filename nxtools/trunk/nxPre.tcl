#
# nxTools - Dupe Checker, nuker, pre, request and utilities.
# Copyright (c) 2004-2006 neoxed
#
# Module Name:
#   Pre
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements a pre script and related statistical commands.
#

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
    upvar comments comments preArea preArea preGrps preGrps prePath prePath
    if {[catch {set handle [open $configFile r]} error]} {
        ErrorLog PreConfigRead $error
        LinePuts "Unable to load the pre configuration, contact a siteop."
        return 0
    }
    set comments ""
    set section -1

    while {![eof $handle]} {
        set line [string trim [gets $handle]]
        if {![string length $line]} {continue}

        if {[string index $line 0] eq "#"} {
            append comments $line "\n"; continue
        }
        if {[string match {\[*\]} $line]} {
            set section [lsearch -exact {[AREAS] [GROUPS] [PATHS]} $line]
        } else {
            switch -- $section {
                0 {set preArea([lindex $line 0]) [lindex $line 1]}
                1 {set preGrps([lindex $line 0]) [lindex $line 1]}
                2 {set prePath([lindex $line 0]) [lrange $line 1 end]}
            }
        }
    }
    close $handle
    return 1
}

proc ::nxTools::Pre::ConfigWrite {configFile} {
    upvar comments comments preArea preArea preGrps preGrps prePath prePath
    if {[catch {set handle [open $configFile w]} error]} {
        ErrorLog PreConfigWrite $error
        return 0
    }
    puts $handle $comments

    puts $handle "\[AREAS\]"
    foreach {name value} [array get preArea] {
        puts $handle "$name \"$value\""
    }

    puts $handle "\n\[GROUPS\]"
    foreach {name value} [array get preGrps] {
        puts $handle "$name \"[lsort -ascii $value]\""
    }

    puts $handle "\n\[PATHS\]"
    foreach {name value} [array get prePath] {
        puts $handle "$name \"[join $value {" "}]\""
    }
    close $handle
    return 1
}

proc ::nxTools::Pre::DisplayAreas {areaList} {
    set areas [JoinLiteral [lsort -ascii $areaList]]
    set prefix "Areas : "
    set prefixLen [string length $prefix]

    foreach line [WordWrap $areas [expr {70 - $prefixLen}]] {
        LinePuts "$prefix$line"
        set prefix [string repeat " " $prefixLen]
    }
}

proc ::nxTools::Pre::ResolvePath {userName groupName realPath} {
    set bestMatch 0
    set resolvePath "/"
    set vfsFile ""
    set realPath [string map {\\ /} $realPath]

    # Find the user VFS file.
    if {[userfile open $userName] == 0} {
        set userFile [userfile bin2ascii]
        foreach line [split $userFile "\r\n"] {
            if {[string equal -nocase "vfsfile" [lindex $line 0]]} {
                set vfsFile [StringRange $line 1 end]; break
            }
        }
    }
    # Use the group VFS file if the user VFS file does not exist.
    if {![file isfile $vfsFile] && [groupfile open $groupName] == 0} {
        set groupFile [groupfile bin2ascii]
        foreach line [split $groupFile "\r\n"] {
            if {[string equal -nocase "vfsfile" [lindex $line 0]]} {
                set vfsFile [StringRange $line 1 end]; break
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
            set baseLength [string length $basePath]

            if {[string equal -length $baseLength -nocase $basePath $realPath]} {
                # Use the longest available mount path, improves accuracy.
                if {$baseLength > $bestMatch} {
                    set resolvePath [string range $realPath $baseLength end]
                    set resolvePath [file join $mountPath [string trimleft $resolvePath "/"]]
                    set bestMatch $baseLength
                }
            }
        }
        close $handle
    } else {
        ErrorLog PreResolvePath $error
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
    set linkCount [LinkDb eval {SELECT COUNT(*) FROM Links WHERE LinkType=1}]
    if {$linkCount > $latest(PreLinks)} {
        set linkCount [expr {$linkCount - $latest(PreLinks)}]
        # The link type for pre tags is "1".
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
        set argList "help"
    }
    foreach {option target value} $argList {break}

    set area [string toupper $target]
    set option [string tolower $option]
    set success 0

    switch -- $option {
        addarea {
            if {![string length $target]} {
                LinePuts "Invalid area, you must specify an area to add."
            } elseif {[info exists preArea($area)]} {
                LinePuts "The \"$area\" area already exists, delete it first."
            } elseif {![string length $value]} {
                LinePuts "The \"$area\" area must have a destination path."
            } else {
                set realPath [resolve pwd $value]
                if {[string index $value 0] ne "/" || ![file isdirectory $realPath]} {
                    LinePuts "The destination path \"$value\" is invalid or does not exist."
                    LinePuts "Note: Make sure the path begins with a forward slash."
                    return 1
                } elseif {[string index $realPath end] ne "/"} {append realPath "/"}

                if {[string first "%" $other] != -1} {
                    set other [string trim $other "/"]
                    append realPath $other "/"
                } elseif {[string length $other]} {
                    LinePuts "Invalid destination date cookie(s) \"$other\"."
                    LinePuts "Note: Try \"SITE EDITPRE HELP ADDAREA\" to view valid date cookies."
                    return 1
                }
                set preArea($area) $realPath
                set preGrps($area) ""

                LinePuts "Created area \"$area\", destination set to \"$value$other\"."
                LinePuts "Note: Add groups to the area so others may pre to it."
                set success [ConfigWrite $pre(ConfigFile)]
            }
        }
        delarea {
            if {![string length $target] || ![info exists preArea($area)]} {
                LinePuts "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
            } else {
                unset -nocomplain preArea($area) preGrps($area)
                LinePuts "Removed the area \"$area\" and all related settings."
                set success [ConfigWrite $pre(ConfigFile)]
            }
        }
        addgrp {
            if {![string length $target] || ![info exists preArea($area)]} {
                LinePuts "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
            } elseif {![string length $value]} {
                LinePuts "Invalid group, you must specify a group to add."
            } else {
                # Check if the group already exists in the area.
                if {![info exists preGrps($area)]} {set preGrps($area) ""}
                foreach entry $preGrps($area) {
                    if {$entry eq $value} {
                        LinePuts "The group \"$value\" already exists in the \"$area\" allow list."
                        return 1
                    }
                }
                lappend preGrps($area) $value
                LinePuts "Added the group \"$value\" to the \"$area\" allow list."
                set success [ConfigWrite $pre(ConfigFile)]
            }
        }
        delgrp {
            if {![string length $target] || ![info exists preArea($area)]} {
                LinePuts "Invalid area, try \"SITE EDITPRE HELP\" to view available areas."
            } elseif {![string length $value]} {
                LinePuts "Invalid group, you must specify a group to remove."
            } else {
                set deleted 0; set index 0
                if {![info exists preGrps($area)]} {set preGrps($area) ""}
                foreach entry $preGrps($area) {
                    if {$entry eq $value} {
                        set preGrps($area) [lreplace $preGrps($area) $index $index]
                        set deleted 1; break
                    }
                    incr index
                }

                if {$deleted} {
                    LinePuts "Removed the group \"$value\" from the \"$area\" allow list."
                    set success [ConfigWrite $pre(ConfigFile)]
                } else {
                    LinePuts "The group \"$value\" does not exist in the \"$area\" allow list."
                }
            }
        }
        addpath {
            if {![string length $target]} {
                LinePuts "Invalid group, try \"SITE EDITPRE HELP ADDPATH\" for help."
            } elseif {![string length $value]} {
                LinePuts "Invalid path, you must specify a path where group can pre from."
            } else {
                set realPath [resolve pwd $value]
                if {[string index $value 0] ne "/" || ![file isdirectory $realPath]} {
                    LinePuts "The path \"$value\" is invalid or does not exist."
                    LinePuts "Note: Make sure the path has both leading and trailing slashes."
                    return 1
                } elseif {[string index $value end] ne "/"} {append value "/"}

                # Check if the path already defined for that group.
                if {![info exists prePath($target)]} {set prePath($target) ""}
                foreach entry $prePath($target) {
                    if {[string equal -nocase $entry $value]} {
                        LinePuts "The path \"$value\" is already defined for the \"$target\" group."
                        return 1
                    }
                }
                lappend prePath($target) $value
                catch {vfs chattr $realPath 0 [string map [list %(group) $target] $pre(PrivatePath)]}

                LinePuts "Added path \"$value\" to the \"$target\" group."
                set success [ConfigWrite $pre(ConfigFile)]
            }
        }
        delpath {
            if {![string length $target]} {
                LinePuts "Invalid group, try \"SITE EDITPRE HELP DELPATH\" for help."
            } elseif {![string length $value]} {
                LinePuts "Invalid path, you must specify a path to delete."
            } else {
                set deleted 0; set index 0
                if {[info exists prePath($target)]} {
                    foreach entry $prePath($target) {
                        if {[string equal -nocase $entry $value]} {
                            set prePath($target) [lreplace $prePath($target) $index $index]
                            set deleted 1; break
                        }
                        incr index
                    }
                    if {![string length $prePath($target)]} {unset prePath($target)}
                }

                if {$deleted} {
                    LinePuts "Removed path \"$value\" for the \"$target\" group."
                    set success [ConfigWrite $pre(ConfigFile)]
                } else {
                    LinePuts "The path \"$value\" is not defined for the \"$target\" group."
                }
            }
        }
        hidepath - hidepaths {
            if {$target ne "" && ![info exists prePath($target)]} {
                LinePuts "Invalid group, try \"SITE EDITPRE HELP HIDEPATH\" for help."
            } else {
                foreach name [lsort -ascii [array names prePath]] {
                    if {$target ne "" && ![string match $target $name]} {continue}
                    LinePuts "Hiding pre paths for: $name"
                    set privPath [string map [list %(group) $name] $pre(PrivatePath)]

                    foreach entry $prePath($name) {
                        set realPath [resolve pwd $entry]
                        if {[file exists $realPath]} {
                            catch {vfs chattr $realPath 0 $privPath}
                        } else {
                            LinePuts "- The vpath \"$entry\" does not exist."
                        }
                    }
                }
                set success 1
            }
        }
        view {
            LinePuts "Areas:"
            foreach name [lsort -ascii [array names preArea]] {
                LinePuts [format "%-10s - %s" $name $preArea($name)]
            }
            LinePuts ""; LinePuts "Groups:"
            foreach name [lsort -ascii [array names preGrps]] {
                LinePuts [format "%-10s - %s" $name $preGrps($name)]
            }
            LinePuts ""; LinePuts "Paths:"
            foreach name [lsort -ascii [array names prePath]] {
                LinePuts [format "%-10s - %s" $name $prePath($name)]
            }
            set success 1
        }
        default {
            set option [string tolower $target]
            switch -- $option {
                addarea {
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
                delarea {
                    LinePuts "Description:"
                    LinePuts " - Delete a pre area and related settings."
                    LinePuts "Syntax : SITE EDITPRE DELAREA <area>"
                    LinePuts "Example: SITE EDITPRE DELAREA 0DAY"
                }
                addgrp {
                    LinePuts "Description:"
                    LinePuts " - Allow a group to pre to the specified area."
                    LinePuts "Syntax : SITE EDITPRE ADDGRP <area> <group>"
                    LinePuts "Example: SITE EDITPRE ADDGRP 0DAY NX"
                }
                delgrp {
                    LinePuts "Description:"
                    LinePuts " - Disallow a group to pre to the specified area."
                    LinePuts "Syntax : SITE EDITPRE DELGRP <area> <group>"
                    LinePuts "Example: SITE EDITPRE DELGRP 0DAY NX"
                }
                addpath {
                    LinePuts "Description:"
                    LinePuts " - Add a path to be used as a pre area for the specified group."
                    LinePuts "Syntax : SITE EDITPRE ADDPATH <group> <path>"
                    LinePuts "Example: SITE EDITPRE ADDPATH NX /GROUPS/NX/"
                }
                delpath {
                    LinePuts "Description:"
                    LinePuts " - Remove a path from the specified group."
                    LinePuts "Syntax : SITE EDITPRE DELPATH <group> <path>"
                    LinePuts "Example: SITE EDITPRE DELPATH NX /GROUPS/NX/"
                }
                hidepath - hidepaths {
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
                    DisplayAreas [array names preArea]
                }
            }
        }
    }
    return [expr {!$success}]
}

proc ::nxTools::Pre::History {argList} {
    if {![GetOptions $argList limit pattern]} {
        iputs "Syntax: SITE PRE HISTORY \[-max <limit>\] \[group\]"
        return 1
    }
    if {[string length $pattern]} {
        set whereClause "WHERE GroupName LIKE '[SqlWildToLike $pattern]' ESCAPE '\\'"
    } else {
        set whereClause ""
    }

    iputs ".-\[PreHistory\]-----------------------------------------------------------."
    iputs "| ## |  Release                                              |  Amount   |"
    iputs "|------------------------------------------------------------------------|"
    set count 0
    if {![catch {DbOpenFile [namespace current]::PreDb "Pres.db"} error]} {
        PreDb eval "SELECT Release,Size FROM Pres $whereClause ORDER BY TimeStamp DESC LIMIT $limit" values {
            iputs [format "| %02d | %-53.53s | %9s |" [incr count] $values(Release) [FormatSize $values(Size)]]
        }
        PreDb close
    } else {ErrorLog PreHistory $error}

    if {!$count} {LinePuts "There are no pres to display."}
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Pre::Stats {argList} {
    if {![GetOptions $argList limit pattern]} {
        iputs "Syntax: SITE PRE STATS \[-max <limit>\] \[group\]"
        return 1
    }
    if {[string length $pattern]} {
        set whereClause "WHERE GroupName LIKE '[SqlWildToLike $pattern]' ESCAPE '\\'"
    } else {
        set whereClause ""
    }

    iputs ".-\[PreStats\]-------------------------------------------------------------."
    iputs "| ## |  Group                        |   Pres    |   Files   |  Amount   |"
    iputs "|------------------------------------------------------------------------|"
    set count 0
    if {![catch {DbOpenFile [namespace current]::PreDb "Pres.db"} error]} {
        PreDb eval "SELECT GroupName, COUNT(*) AS Pres, CAST(SUM(Files) AS INT) AS Files,
                SUM(Size) AS Amount FROM Pres $whereClause
                GROUP BY GroupName ORDER BY Pres DESC LIMIT $limit" values {
            iputs [format "| %02d | %-29.29s | %9d | %8dF | %9s |" [incr count] $values(GroupName) $values(Pres) $values(Files) [FormatSize $values(Amount)]]
        }
        PreDb close
    } else {ErrorLog PreStats $error}

    if {!$count} {LinePuts "There are no statistics to display."}
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Pre::Release {argList} {
    global dupe latest misc pre group groups pwd user
    if {![ConfigRead $pre(ConfigFile)]} {return 1}
    set area [string toupper [lindex $argList 0]]
    set target [lindex $argList 1]

    # Check area and group paths.
    if {![info exists preArea($area)] || ![string length $preArea($area)]} {
        LinePuts "Invalid area, try \"SITE PRE HELP\" to view available areas."
        return 1
    } elseif {![info exists preGrps($area)]} {
        LinePuts "The specified area has no group list defined."
        return 1
    }

    # Check if group is allowed to pre to the area and from this path.
    set allowPath 0; set preGroup ""
    set virtualPath [GetPath $pwd $target]
    foreach groupName $groups {
        if {[lsearch -exact $preGrps($area) $groupName] != -1} {
            set preGroup $groupName
            if {![info exists prePath($groupName)]} {continue}

            foreach entry $prePath($groupName) {
                set entry [string trimright $entry "/"]
                set pathLength [string length $entry]
                if {[string equal -length $pathLength -nocase $entry $virtualPath]} {
                    set allowPath 1; break
                }
            }
            if {$allowPath} {break}
        }
    }
    if {![string length $preGroup]} {
        LinePuts "Your group(s) are not allowed to pre to the \"$area\" area."
        return 1
    } elseif {!$allowPath} {
        LinePuts "Your group \"$preGroup\" is not allowed to pre from this path."
        return 1
    }

    # Check if the specified directory exists.
    set realPath [resolve pwd $virtualPath]
    if {![string length $target] || ![file isdirectory $realPath]} {
        LinePuts "The specified directory does not exist."
        return 1
    }
    set diskCount 0; set files 0; set totalSize 0
    set release [file tail $virtualPath]

    # Find destination path.
    set destRealPath $preArea($area)
    set preTime [clock seconds]
    set destRealPath [clock format $preTime -format $destRealPath -gmt [IsTrue $misc(UtcTime)]]
    if {![file isdirectory $destRealPath]} {
        LinePuts "The pre destination path for the \"$area\" area does not exist."
        LinePuts "Note: If the area uses dated dirs, check that today's date dir exists."
        return 1
    }
    set destRealPath [file join $destRealPath $release]
    if {[file exists $destRealPath]} {
        LinePuts "A file or directory by that name already exists in the target area."
        return 1
    }

    # Find the credit and stats section.
    set destVirtualPath [ResolvePath $user $group $destRealPath]
    ListAssign [GetCreditStatSections $destVirtualPath] creditSection statSection

    # Count disk sub-directories.
    foreach entry [glob -nocomplain -types d -directory $realPath "*"] {
        if {[IsDiskPath $entry]} {incr diskCount}
    }

    # Count files and total size.
    GetDirList $realPath dirlist ".ioFTPD*"
    foreach entry $dirlist(FileList) {
        incr files; set fileSize [file size $entry]
        set fileSize [file size $entry]
        set totalSize [expr {wide($totalSize) + wide($fileSize)}]

        if {[IsTrue $pre(Uploaders)]} {
            catch {lindex [vfs read $entry] 0} userId
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
            LinePuts "Unable to check available space on target drive, contact a siteop."
            return 1
        }
        set checkSize [expr {wide($totalSize) + (10*1024)}]
        set volume(free) [expr {wide($volume(free)) / 1024}]

        if {$checkSize > $volume(free)} {
            ErrorLog PreLowSpace "unable to pre \"$realPath\": insufficient disk space on $destDrive ([FormatSize $volume(free)] free, needed [FormatSize $checkSize])"
            LinePuts "Insufficient disk space on target drive, contact a siteop."
            return 1
        }
    }
    LinePuts "Area    : $area"
    LinePuts "Release : $release"
    LinePuts "Files   : [format %-16s ${files}F] Size: [format %-16s [FormatSize $totalSize]] CDs: $diskCount"

    # Move release to the destination path.
    KickUsers [file join $virtualPath "*"]
    if {[catch {file rename -force -- $realPath $destRealPath} error]} {
        ErrorLog PreMove $error
        LinePuts "Error   : Unable to move directory, aborting."
        return 1
    }

    set isMP3 0
    set filePath [expr {!$diskCount ? "*.mp3" : "*/*.mp3"}]
    set mp3Files [glob -nocomplain -types f -directory $destRealPath $filePath]

    # Attempt to parse every MP3 file until successful.
    foreach filePath $mp3Files {
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
        foreach entry $dirlist(DirList) {
            catch {vfs read $entry} owner
            ListAssign $owner userId groupId chmod

            # Verify the group and chmod the directory.
            if {![string is digit -strict $groupId]} {set groupId [lindex $misc(DirOwner) 1]}
            if {![string is digit -strict $chmod]} {set chmod $misc(DirChmod)}
            catch {vfs write $entry $pre(ChownUserId) $groupId $chmod}
        }
        foreach entry $dirlist(FileList) {
            catch {vfs read $entry} owner
            ListAssign $owner userId groupId chmod

            # Verify the group and chmod the file.
            if {![string is digit -strict $groupId]} {set groupId [lindex $misc(FileOwner) 1]}
            if {![string is digit -strict $chmod]} {set chmod $misc(FileChmod)}
            catch {vfs write $entry $pre(ChownUserId) $groupId $chmod}
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
    set line [expr {$isMP3 ? "PRE-MP3" : "PRE"}]
    append line ": \"$destVirtualPath\" \"$preGroup\" \"$user\" \"$group\" \"$area\" \"$files\" \"$totalSize\" \"$diskCount\""
    if {$isMP3} {append line " \"$mp3(artist)\" \"$mp3(album)\" \"$mp3(genre)\" \"$mp3(year)\" \"$mp3(bitrate)\" \"$mp3(type)\""}
    putlog $line

    if {![catch {DbOpenFile [namespace current]::PreDb "Pres.db"} error]} {
        PreDb eval {INSERT INTO Pres(TimeStamp,UserName,GroupName,Area,Release,Files,Size)
            VALUES($preTime,$user,$preGroup,$area,$release,$files,$totalSize)}
        PreDb close
    } else {ErrorLog PreDb $error}

    if {[IsTrue $dupe(AddOnPre)]} {
        if {![catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
            if {$pre(ChownUserId) eq ""} {
                set logUser $user
            } else {
                set logUser [resolve uid $pre(ChownUserId)]
            }
            set logPath [string range $destVirtualPath 0 [string last "/" $destVirtualPath]]

            DirDb eval {INSERT INTO DupeDirs(TimeStamp,UserName,GroupName,DirPath,DirName)
                VALUES($preTime,$logUser,$preGroup,$logPath,$release)}
            DirDb close
        } else {ErrorLog PreDupeDb $error}
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
                DisplayAreas [array names preArea]
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
