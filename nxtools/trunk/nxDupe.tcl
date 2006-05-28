#
# nxTools - Dupe Checker, nuker, pre, request and utilities.
# Copyright (c) 2004-2006 neoxed
#
# Module Name:
#   Dupe and File Checks
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements a duplicate file and directory checker, pre time checks,
#   and a release approval script.
#

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Dupe {
    namespace import -force ::nxLib::*
}

# Dupe Procedures
######################################################################

proc ::nxTools::Dupe::CheckDirs {virtualPath} {
    global dupe
    if {[ListMatch $dupe(CheckExempts) $virtualPath] || [ListMatchI $dupe(IgnoreDirs) $virtualPath]} {return 0}
    set result 0
    if {![catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
        set dirName [file tail $virtualPath]

        DirDb eval {SELECT * FROM DupeDirs WHERE StrCaseEq(DirName,$dirName) LIMIT 1} values {
            set age [FormatDuration [expr {[clock seconds] - $values(TimeStamp)}]]
            set path [file join $values(DirPath) $values(DirName)]
            iputs -noprefix "553-.-\[DupeCheck\]-------------------------------------------------."
            iputs -noprefix "553-| [format %-59s "Dupe: $path"] |"
            iputs -noprefix "553-| [format %-59s "Created $age ago by $values(UserName)."] |"
            iputs -noprefix "553 '-------------------------------------------------------------'"
            set result 1
        }
        DirDb close
    } else {ErrorLog DupeCheckDirs $error}
    return $result
}

proc ::nxTools::Dupe::CheckFiles {virtualPath} {
    global dupe
    if {[ListMatch $dupe(CheckExempts) $virtualPath] || [ListMatchI $dupe(IgnoreFiles) $virtualPath]} {return 0}
    set result 0
    if {![catch {DbOpenFile [namespace current]::FileDb "DupeFiles.db"} error]} {
        set fileName [file tail $virtualPath]

        FileDb eval {SELECT * FROM DupeFiles WHERE StrCaseEq(FileName,$fileName) LIMIT 1} values {
            set age [FormatDuration [expr {[clock seconds] - $values(TimeStamp)}]]
            iputs -noprefix "553-.-\[DupeCheck\]-------------------------------------------------."
            iputs -noprefix "553-| [format %-59s "Dupe: $values(FileName)"] |"
            iputs -noprefix "553-| [format %-59s "Uploaded $age ago by $values(UserName)."] |"
            iputs -noprefix "553 '-------------------------------------------------------------'"
            set result 1
        }
        FileDb close
    } else {ErrorLog DupeCheckFiles $error}
    return $result
}

proc ::nxTools::Dupe::UpdateLog {command virtualPath} {
    global dupe
    if {[ListMatch $dupe(LoggingExempts) $virtualPath]} {return 0}
    set command [string toupper $command]

    # Check if the virtual path is a file or directory.
    if {$command eq "UPLD" || $command eq "DELE" || [file isfile [resolve pwd $virtualPath]]} {
        if {[IsTrue $dupe(CheckFiles)] && ![ListMatchI $dupe(IgnoreFiles) $virtualPath]} {
            return [UpdateFiles $command $virtualPath]
        }
    } elseif {[IsTrue $dupe(CheckDirs)] && ![ListMatchI $dupe(IgnoreDirs) $virtualPath]} {
        return [UpdateDirs $command $virtualPath]
    }
    return 0
}

proc ::nxTools::Dupe::UpdateDirs {command virtualPath} {
    global dupe group user
    if {[catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
        ErrorLog DupeUpdateDirs $error
        return 1
    }
    set dirName [file tail $virtualPath]
    set dirPath [string range $virtualPath 0 [string last "/" $virtualPath]]

    if {$command eq "MKD"  || $command eq "RNTO"} {
        set timeStamp [clock seconds]
        DirDb eval {INSERT INTO DupeDirs(TimeStamp,UserName,GroupName,DirPath,DirName) VALUES($timeStamp,$user,$group,$dirPath,$dirName)}
    } elseif {[lsearch -sorted {RMD RNFR WIPE} $command] != -1} {
        # Append a slash to improve the accuracy of StrCaseEqN.
        # For example, /Dir/Blah matches /Dir/Blah.Blah but /Dir/Blah/ does not.
        append virtualPath "/"
        DirDb eval {DELETE FROM DupeDirs WHERE StrCaseEqN(DirPath,$virtualPath,length($virtualPath)) OR (StrCaseEq(DirPath,$dirPath) AND StrCaseEq(DirName,$dirName))}
    }
    DirDb close
    return 0
}

proc ::nxTools::Dupe::UpdateFiles {command virtualPath} {
    global dupe group user
    if {[catch {DbOpenFile [namespace current]::FileDb "DupeFiles.db"} error]} {
        ErrorLog DupeUpdateFiles $error
        return 1
    }
    set fileName [file tail $virtualPath]

    if {$command eq "UPLD" || $command eq "RNTO"} {
        set timeStamp [clock seconds]
        FileDb eval {INSERT INTO DupeFiles(TimeStamp,UserName,GroupName,FileName) VALUES($timeStamp,$user,$group,$fileName)}
    } elseif {$command eq "DELE" || $command eq "RNFR"} {
        FileDb eval {DELETE FROM DupeFiles WHERE StrCaseEq(FileName,$fileName)}
    }
    FileDb close
    return 0
}

proc ::nxTools::Dupe::CleanDb {} {
    global dupe misc user group
    if {![info exists user] && ![info exists group]} {
        if {[userfile open $misc(MountUser)] != 0} {
            ErrorLog DupeClean "unable to open the user \"$misc(MountUser)\"";
            return 1
        } elseif {[mountfile open $misc(MountFile)] != 0} {
            ErrorLog DupeClean "unable to mount the VFS-file \"$misc(MountFile)\""
            return 1
        }
    }
    iputs -nobuffer ".-\[Clean\]----------------------------------------------------------------."

    if {$dupe(CleanFiles) < 1} {
        LinePuts -nobuffer "File database cleaning disabled, skipping."
    } elseif {[catch {DbOpenFile [namespace current]::FileDb "DupeFiles.db"} error]} {
        LinePuts -nobuffer "Unable to open the file database."
        ErrorLog CleanFiles $error
    } else {
        LinePuts -nobuffer "Cleaning the file database."
        set maxAge [expr {[clock seconds] - ($dupe(CleanFiles) * 86400)}]
        FileDb eval {DELETE FROM DupeFiles WHERE TimeStamp < $maxAge}
        LinePuts -nobuffer " - Removed [FileDb changes] files."
        FileDb close
    }

    if {$dupe(CleanDirs) < 1} {
        LinePuts -nobuffer "Directory database cleaning disabled, skipping."
    } elseif {[catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
        LinePuts -nobuffer "Unable to open the directory database."
        ErrorLog CleanDirs $error
    } else {
        LinePuts -nobuffer "Cleaning the directory database."
        set maxAge [expr {[clock seconds] - ($dupe(CleanDirs) * 86400)}]
        set rowIds [list]

        set statusCount 0
        set statusTime [clock seconds]
        set totalCount 0
        DirDb eval {SELECT DirPath,DirName,rowid FROM DupeDirs WHERE TimeStamp < $maxAge} values {
            incr totalCount
            set fullPath [file join $values(DirPath) $values(DirName)]
            if {![file isdirectory [resolve pwd $fullPath]]} {
                lappend rowIds $values(rowid)
            }

            # Check if 60 seconds have elapsed since the last
            # status output every 50 entries processed.
            if {[incr statusCount] >= 50 && ([clock seconds] - $statusTime) >= 60} {
                set statusCount 0
                set statusTime [clock seconds]
                LinePuts -nobuffer " - Processed $totalCount directories..."
            }
        }

        if {[llength $rowIds]} {
            DirDb eval "DELETE FROM DupeDirs WHERE rowid IN ([join $rowIds ,])"
        }
        LinePuts -nobuffer " - Removed [llength $rowIds] directories."
        DirDb close
    }
    iputs -nobuffer "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Dupe::RebuildDb {} {
    global dupe misc
    iputs -nobuffer ".-\[Rebuild\]--------------------------------------------------------------."
    if {![llength $dupe(RebuildPaths)]} {
        ErrorReturn "There are no paths defined, check your configuration."
    }
    if {[catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
        ErrorLog RebuildDirs $error
        return 1
    }
    if {[catch {DbOpenFile [namespace current]::FileDb "DupeFiles.db"} error]} {
        ErrorLog RebuildFiles $error
        DirDb close
        return 1
    }
    DirDb eval {BEGIN; DELETE FROM DupeDirs;}
    FileDb eval {BEGIN; DELETE FROM DupeFiles;}

    foreach rebuildPath $dupe(RebuildPaths) {
        if {[llength $rebuildPath] != 4} {
            ErrorLog DupeRebuild "wrong number of parameters in line: \"$rebuildPath\""; continue
        }
        foreach {virtualPath realPath updateDirs updateFiles} $rebuildPath {break}
        set realPath [file normalize $realPath]
        set trimLength [string length $realPath]; incr trimLength

        LinePuts -nobuffer "Updating from: $realPath"
        GetDirList $realPath dirlist $dupe(RebuildIgnore)

        foreach dirMode {0 1} doUpdate [list $updateDirs $updateFiles] {
            if {![IsTrue $doUpdate]} {continue}

            if {$dirMode} {
                set defOwner $misc(DirOwner)
                set entries  "DirList"
                set ignore   $dupe(IgnoreDirs)
                set maxAge   0
            } else {
                set defOwner $misc(FileOwner)
                set entries  "FileList"
                set ignore   $dupe(IgnoreFiles)
                set maxAge   [expr {$dupe(CleanFiles) * 86400}]
            }
            set defUser [resolve uid [lindex $defOwner 0]]
            set defGroup [resolve gid [lindex $defOwner 1]]

            foreach entry $dirlist($entries) {
                if {[ListMatchI $ignore $entry]} {continue}

                if {[catch {file stat $entry fstat}]} {continue}
                if {$maxAge > 0 && ([clock seconds] - $fstat(ctime)) > $maxAge} {continue}
                catch {vfs read $entry} owner
                if {[set userName [resolve uid [lindex $owner 0]]] eq ""} {set userName $defUser}
                if {[set groupName [resolve gid [lindex $owner 1]]] eq ""} {set groupName $defGroup}

                set baseName [file tail $entry]
                if {$dirMode} {
                    set dirPath [file join $virtualPath [string range [file dirname $entry] $trimLength end]]
                    append dirPath "/"
                    DirDb eval {INSERT INTO DupeDirs(TimeStamp,UserName,GroupName,DirPath,DirName) VALUES($fstat(ctime),$userName,$groupName,$dirPath,$baseName)}
                } else {
                    FileDb eval {INSERT INTO DupeFiles(TimeStamp,UserName,GroupName,FileName) VALUES($fstat(ctime),$userName,$groupName,$baseName)}
                }
            }
        }
    }

    DirDb eval {COMMIT}
    DirDb close
    FileDb eval {COMMIT}
    FileDb close
    iputs -nobuffer "'------------------------------------------------------------------------'"
    return 0
}

# Other Procedures
######################################################################

proc ::nxTools::Dupe::ApproveCheck {virtualPath createTag} {
    set approved 0
    if {![catch {DbOpenFile [namespace current]::ApproveDb "Approves.db"} error]} {
        set release [file tail $virtualPath]
        ApproveDb eval {SELECT UserName,GroupName,rowid FROM Approves WHERE StrCaseEq(Release,$release) LIMIT 1} values {set approved 1}
        if {$approved && $createTag} {
            ApproveRelease $virtualPath $values(UserName) $values(GroupName)
            ApproveDb eval {DELETE FROM Approves WHERE rowid=$values(rowid)}
        }
        ApproveDb close
    } else {ErrorLog ApproveCheck $error}
    return $approved
}

proc ::nxTools::Dupe::ApproveRelease {virtualPath userName groupName} {
    global approve
    set realPath [resolve pwd $virtualPath]
    if {[file isdirectory $realPath]} {
        putlog "APPROVE: \"$virtualPath\" \"$userName\" \"$groupName\""
        set tagPath [file join $realPath [string map [list %(user) $userName %(group) $groupName] $approve(DirTag)]]
        CreateTag $tagPath [resolve user $userName] [resolve group $groupName] 555
    } else {
        ErrorLog ApproveRelease "invalid vpath \"$virtualPath\", \"$realPath\" doesn't exist."
        return 0
    }
    return 1
}

proc ::nxTools::Dupe::ForceCheck {virtualPath} {
    global force
    set fileExt [file extension $virtualPath]
    set matchPath [file dirname $virtualPath]

    if {![string equal -nocase ".nfo" $fileExt] && ![string equal -nocase ".sfv" $fileExt] && ![ListMatchI $force(Exempts) $matchPath]} {
        set releasePath [resolve pwd [expr {[IsDiskPath $matchPath] ? [file dirname $matchPath] : $matchPath}]]
        set checkFile [ListMatch $force(FilePaths) $matchPath]

        if {$checkFile && [IsTrue $force(NfoFirst)]} {
            if {![llength [glob -nocomplain -types f -directory $releasePath "*.nfo"]]} {
                iputs -noprefix "553-.-\[ForceNFO\]------------------------------------."
                iputs -noprefix "553-| You must upload the NFO first.                |"
                iputs -noprefix "553 '-----------------------------------------------'"
                return 1
            }
        }
        if {$checkFile && [IsTrue $force(SfvFirst)]} {
            set realPath [resolve pwd $matchPath]
            if {![llength [glob -nocomplain -types f -directory $realPath "*.sfv"]]} {
                iputs -noprefix "553-.-\[ForceSFV\]------------------------------------."
                iputs -noprefix "553-| You must upload the SFV first.                |"
                iputs -noprefix "553 '-----------------------------------------------'"
                return 1
            }
        }
        if {[IsTrue $force(SampleFirst)] && [ListMatch $force(SamplePaths) $matchPath]} {
            set sampleFiles "sample/{*.avi,*.mpeg,*.mpg,*.vob}"
            if {![llength [glob -nocomplain -types f -directory $releasePath $sampleFiles]]} {
                iputs -noprefix "553-.-\[ForceSample\]---------------------------------."
                iputs -noprefix "553-| You must upload the sample first.             |"
                iputs -noprefix "553 '-----------------------------------------------'"
                return 1
            }
        }
    }
    return 0
}

proc ::nxTools::Dupe::RaceLinks {virtualPath} {
    global latest
    if {[ListMatch $latest(Exempts) $virtualPath] || [ListMatchI $latest(Ignores) $virtualPath]} {
        return 0
    }
    if {[catch {DbOpenFile [namespace current]::LinkDb "Links.db"} error]} {
        ErrorLog RaceLinks $error
        return 1
    }
    # Format and create link directory.
    set section [GetSectionName $virtualPath]
    set release [file tail $virtualPath]
    if {$latest(MaxLength) > 0 && [string length $release] > $latest(MaxLength)} {
        set release [string trimright [string range $release 0 $latest(MaxLength)] "."]
    }
    set tag [string map [list %(section) $section %(release) $release] $latest(RaceTag)]
    set timeStamp [clock seconds]
    LinkDb eval {INSERT INTO Links(TimeStamp,LinkType,DirName) VALUES($timeStamp,0,$tag)}

    set tag [file join $latest(SymPath) $tag]
    if {![catch {file mkdir $tag} error]} {
        catch {vfs chattr $tag 1 $virtualPath}
    } else {ErrorLog RaceLinksMkDir $error}

    # Remove older links.
    set linkCount [LinkDb onecolumn {SELECT COUNT(*) FROM Links WHERE LinkType=0}]
    if {$linkCount > $latest(RaceLinks)} {
        set linkCount [expr {$linkCount - $latest(RaceLinks)}]
        # The link type for pre tags is "0".
        LinkDb eval {SELECT DirName,rowid FROM Links WHERE LinkType=0 ORDER BY TimeStamp ASC LIMIT $linkCount} values {
            RemoveTag [file join $latest(SymPath) $values(DirName)]
            LinkDb eval {DELETE FROM Links WHERE rowid=$values(rowid)}
        }
    }
    LinkDb close
    return 0
}

# Site Commands
######################################################################

proc ::nxTools::Dupe::SiteApprove {event argList} {
    global approve misc flags group user

    if {$event eq "BOT"} {
        # Make sure the user (or bot) has siteop flags.
        if {![MatchFlags $misc(SiteopFlags) $flags]} {
            iputs "You do not have access to this command."
            return 1
        }
        set event [string toupper [lindex $argList 0]]
        set user [lindex $argList 1]
        set release [join [lrange $argList 2 end]]
        GetUserInfo $user group flags
    } else {
        set release [join $argList]
    }

    if {[catch {DbOpenFile [namespace current]::ApproveDb "Approves.db"} error]} {
        ErrorLog SiteApprove $error
        return 1
    }
    set release [file tail $release]
    switch -- $event {
        ADD {
            iputs ".-\[Approve\]--------------------------------------------------------------."
            if {![MatchFlags $approve(Flags) $flags]} {
                LinePuts "Only siteops may approve releases."
            } elseif {[ApproveDb exists {SELECT 1 FROM Approves WHERE StrCaseEq(Release,$release)}]} {
                LinePuts "This release is already approved."
            } else {
                # If the release already exists in the dupe database, approve that one.
                set approved 0
                if {![catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
                    DirDb eval {SELECT DirName,DirPath FROM DupeDirs WHERE StrCaseEq(DirName,$release) ORDER BY TimeStamp DESC LIMIT 1} values {
                        set virtualPath [file join $values(DirPath) $values(DirName)]
                        set approved [ApproveRelease $virtualPath $user $group]
                    }
                    DirDb close
                } else {ErrorLog SiteApprove $error}

                if {$approved} {
                    LinePuts "Found the release, approved \"$virtualPath\"."
                } else {
                    set timeStamp [clock seconds]
                    ApproveDb eval {INSERT INTO Approves(TimeStamp,UserName,GroupName,Release) VALUES($timeStamp,$user,$group,$release)}
                    putlog "APPROVEADD: \"$user\" \"$group\" \"$release\""
                    LinePuts "Added \"$release\" to the approve list."
                }
            }
            iputs "'------------------------------------------------------------------------'"
        }
        DEL {
            iputs ".-\[Approve\]--------------------------------------------------------------."
            if {![MatchFlags $approve(Flags) $flags]} {
                LinePuts "Only siteops may deleted approved releases."
            } else {
                set exists 0
                ApproveDb eval {SELECT rowid,* FROM Approves WHERE StrCaseEq(Release,$release) LIMIT 1} values {set exists 1}

                if {$exists} {
                    ApproveDb eval {DELETE FROM Approves WHERE rowid=$values(rowid)}
                    putlog "APPROVEDEL: \"$user\" \"$group\" \"$values(Release)\""
                    LinePuts "Removed \"$values(Release)\" from the approve list."
                } else {
                    LinePuts "Invalid release, use \"SITE APPROVE LIST\" to view approved releases."
                }
            }
            iputs "'------------------------------------------------------------------------'"
        }
        LIST {
            foreach fileExt {Header Body None Footer} {
                set template($fileExt) [ReadFile [file join $misc(Templates) "Approves.$fileExt"]]
            }
            OutputText $template(Header)
            set count 0

            ApproveDb eval {SELECT * FROM Approves ORDER BY Release ASC} values {
                incr count
                set age [FormatDuration [expr {[clock seconds] - $values(TimeStamp)}]]
                set valueList [list $count [lrange $age 0 1] $values(UserName) $values(GroupName) $values(Release)]
                OutputText [ParseCookies $template(Body) $valueList {num age user group release}]
            }

            if {!$count} {OutputText $template(None)}
            OutputText $template(Footer)
        }
        default {
            ErrorLog SiteApprove "unknown event \"$event\""
        }
    }
    ApproveDb close
    return 0
}

proc ::nxTools::Dupe::SiteDupe {fileRoot limit pattern} {
    global misc
    if {[catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
        ErrorLog SiteDupe $error
        return 1
    }
    foreach fileExt {Header Body None Footer} {
        set template($fileExt) [ReadFile [file join $misc(Templates) "$fileRoot.$fileExt"]]
    }
    OutputText $template(Header)
    set count 0
    set pattern [DbPattern $pattern]

    DirDb eval "SELECT * FROM DupeDirs WHERE DirName LIKE '$pattern' ESCAPE '\\' ORDER BY TimeStamp DESC LIMIT $limit" values {
        incr count
        set valueList [clock format $values(TimeStamp) -format {{%S} {%M} {%H} {%d} {%m} {%y} {%Y}} -gmt [IsTrue $misc(UtcTime)]]
        lappend valueList $count $values(UserName) $values(GroupName) $values(DirName) [file join $values(DirPath) $values(DirName)]
        OutputText [ParseCookies $template(Body) $valueList {sec min hour day month year2 year4 num user group release path}]
    }

    if {!$count} {OutputText $template(None)}
    if {$count == $limit} {
        set total [DirDb eval "SELECT COUNT(*) FROM DupeDirs WHERE DirName LIKE '$pattern' ESCAPE '\\'"]
    } else {
        set total $count
    }

    OutputText [ParseCookies $template(Footer) [list $count $total] {found total}]
    DirDb close
    return 0
}

proc ::nxTools::Dupe::SiteFileDupe {limit pattern} {
    global misc
    if {[catch {DbOpenFile [namespace current]::FileDb "DupeFiles.db"} error]} {
        ErrorLog SiteFileDupe $error
        return 1
    }
    foreach fileExt {Header Body None Footer} {
        set template($fileExt) [ReadFile [file join $misc(Templates) "FileDupe.$fileExt"]]
    }
    OutputText $template(Header)
    set count 0
    set pattern [DbPattern $pattern]

    FileDb eval "SELECT * FROM DupeFiles WHERE FileName LIKE '$pattern' ESCAPE '\\' ORDER BY TimeStamp DESC LIMIT $limit" values {
        incr count
        set valueList [clock format $values(TimeStamp) -format {{%S} {%M} {%H} {%d} {%m} {%y} {%Y}} -gmt [IsTrue $misc(UtcTime)]]
        lappend valueList $count $values(UserName) $values(GroupName) $values(FileName)
        OutputText [ParseCookies $template(Body) $valueList {sec min hour day month year2 year4 num user group file}]
    }

    if {!$count} {OutputText $template(None)}
    if {$count == $limit} {
        set total [FileDb eval "SELECT COUNT(*) FROM DupeFiles WHERE FileName LIKE '$pattern' ESCAPE '\\'"]
    } else {
        set total $count
    }

    OutputText [ParseCookies $template(Footer) [list $count $total] {found total}]
    FileDb close
    return 0
}

proc ::nxTools::Dupe::SiteNew {limit showSection} {
    global misc new
    if {[catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
        ErrorLog SiteNew $error
        return 1
    }
    foreach fileExt {Header Error Body None Footer} {
        set template($fileExt) [ReadFile [file join $misc(Templates) "New.$fileExt"]]
    }
    OutputText $template(Header)
    set sectionList [GetSectionList]

    if {$showSection ne ""} {
        set sectionNameList [list]
        set validSection 0

        # Validate the section name.
        foreach {sectionName creditSection statSection matchPath} $sectionList {
            if {[string equal -nocase $showSection $sectionName]} {
                set showSection $sectionName
                set validSection 1; break
            }
            lappend sectionNameList $sectionName
        }
        if {!$validSection} {
            set sectionNameList [join [lsort -ascii $sectionNameList]]
            OutputText [ParseCookies $template(Error) [list $sectionNameList] {sections}]
            OutputText $template(Footer)
            DirDb close
            return 1
        }

        set showAll 0
        set whereClause "WHERE DirPath LIKE '[DbToLike $matchPath]' ESCAPE '\\'"
    } else {
        set showAll 1
        set whereClause ""
    }

    set count 0
    DirDb eval "SELECT * FROM DupeDirs $whereClause ORDER BY TimeStamp DESC LIMIT $limit" values {
        incr count
        # Find section name and check the match path.
        if {$showAll} {
            set sectionName "DEFAULT"
            foreach {sectionName creditSection statSection matchPath} $sectionList {
                if {[string match -nocase $matchPath $values(DirPath)]} {
                    set showSection $sectionName; break
                }
            }
        }

        set age [FormatDuration [expr {[clock seconds] - $values(TimeStamp)}]]
        set valueList [list $count [lrange $age 0 1] $values(UserName) $values(GroupName) $showSection $values(DirName) [file join $values(DirPath) $values(DirName)]]
        OutputText [ParseCookies $template(Body) $valueList {num age user group section release path}]
    }

    if {!$count} {OutputText $template(None)}
    OutputText $template(Footer)
    DirDb close
    return 0
}

proc ::nxTools::Dupe::SiteUndupe {argList} {
    global dupe misc
    iputs ".-\[Undupe\]---------------------------------------------------------------."
    if {[string equal -nocase "-d" [lindex $argList 0]]} {
        set colName "DirName"
        set dbName "DupeDirs"
        set pattern [join [lrange $argList 1 end]]
    } else {
        set colName "FileName"
        set dbName "DupeFiles"
        set pattern [join [lrange $argList 0 end]]
    }

    if {[regexp {[\*\?]} $pattern] && [regexp -all -- {[[:alnum:]]} $pattern] < $dupe(AlphaNumChars)} {
        ErrorReturn "There must be at $dupe(AlphaNumChars) least alphanumeric chars when wildcards are used."
        return 1
    }
    LinePuts "Searching for: $pattern"
    set removed 0; set total 0
    set pattern [DbToLike $pattern]

    if {![catch {DbOpenFile [namespace current]::DupeDb "${dbName}.db"} error]} {
        set rowIds [list]
        set total [DupeDb eval "SELECT COUNT(*) FROM $dbName"]

        DupeDb eval "SELECT $colName,rowid FROM $dbName WHERE $colName LIKE '$pattern' ESCAPE '\\' ORDER BY $colName ASC" values {
            incr removed
            LinePuts "Unduped: $values($colName)"
            lappend rowIds $values(rowid)
        }
        if {[llength $rowIds]} {
            DupeDb eval "DELETE FROM $dbName WHERE rowid IN ([join $rowIds ,])"
        }
        DupeDb close
    }

    iputs "|------------------------------------------------------------------------|"
    LinePuts "Unduped $removed of $total dupe entries."
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Dupe::SiteWipe {virtualPath} {
    global dupe misc wipe group user
    iputs ".-\[Wipe\]-----------------------------------------------------------------."

    # Resolving a symlink returns its target path, which could have unwanted
    # results. To avoid such issues, we'll resolve the parent path instead.
    set parentPath [resolve pwd [file dirname $virtualPath]]
    set realPath [file join $parentPath [file tail $virtualPath]]
    if {![file exists $realPath]} {
        ErrorReturn "The specified file or directory does not exist."
    }

    set matchPath [string range $virtualPath 0 [string last "/" $virtualPath]]
    if {[ListMatch $wipe(NoPaths) $matchPath]} {
        ErrorReturn "Not allowed to wipe from here."
    }
    GetDirStats $realPath stats ".ioFTPD*"
    set stats(TotalSize) [expr {wide($stats(TotalSize)) / 1024}]

    set isDir [file isdirectory $realPath]
    KickUsers [expr {$isDir ? "$virtualPath/*" : $virtualPath}]
    if {[catch {file delete -force -- $realPath} error]} {
        ErrorLog SiteWipe $error
        LinePuts "Unable to delete the specified file or directory."
    }
    catch {vfs flush [resolve pwd $parentPath]}
    LinePuts "Wiped $stats(FileCount) File(s), $stats(DirCount) Directory(s), [FormatSize $stats(TotalSize)]"
    iputs "'------------------------------------------------------------------------'"

    # Update the file and directory databases.
    if {$isDir} {
        RemoveParentLinks $realPath
        putlog "WIPE: \"$virtualPath\" \"$user\" \"$group\" \"$stats(DirCount)\" \"$stats(FileCount)\" \"$stats(TotalSize)\""
        if {[IsTrue $dupe(CheckDirs)]} {
            UpdateLog "WIPE" $virtualPath
        }
    } elseif {[IsTrue $dupe(CheckFiles)]} {
        UpdateLog "DELE" $virtualPath
    }
    return 0
}

# Dupe Main
######################################################################

proc ::nxTools::Dupe::Main {argv} {
    global approve dupe force latest misc group ioerror pwd user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set result 0

    set argLength [llength [set argList [ListParse $argv]]]
    set event [string toupper [lindex $argList 0]]
    switch -- $event {
        DUPELOG {
            set virtualPath [GetPath [join [lrange $argList 2 end]] $pwd]
            if {[IsTrue $dupe(CheckDirs)] || [IsTrue $dupe(CheckFiles)]} {
                set result [UpdateLog [lindex $argList 1] $virtualPath]
            }
        }
        POSTMKD {
            set virtualPath [GetPath [join [lrange $argList 2 end]] $pwd]
            if {[IsTrue $dupe(CheckDirs)]} {
                set result [UpdateLog [lindex $argList 1] $virtualPath]
            }
            if {$latest(RaceLinks) > 0} {
                set result [RaceLinks $virtualPath]
            }
            if {[IsTrue $approve(CheckMkd)]} {ApproveCheck $virtualPath 1}
        }
        PREMKD {
            set virtualPath [GetPath [join [lrange $argList 2 end]] $pwd]
            if {!([IsTrue $approve(CheckMkd)] && [ApproveCheck $virtualPath 0])} {
                if {[IsTrue $dupe(CheckDirs)]} {
                    set result [CheckDirs $virtualPath]
                }
            }
        }
        PRESTOR {
            set virtualPath [GetPath [join [lrange $argList 2 end]] $pwd]
            if {[IsTrue $force(NfoFirst)] || [IsTrue $force(SfvFirst)] || [IsTrue $force(SampleFirst)]} {
                set result [ForceCheck $virtualPath]
            }
            if {!$result && [IsTrue $dupe(CheckFiles)]} {
                set result [CheckFiles $virtualPath]
            }
        }
        UPLOAD {
            if {[IsTrue $dupe(CheckFiles)]} {
                UpdateLog "UPLD" [lindex $argList 3]
            }
        }
        UPLOADERROR {
            if {[IsTrue $dupe(CheckFiles)]} {
                UpdateLog "DELE" [lindex $argList 3]
            }
        }
        APPROVE {
            array set params [list ADD 3 BOT 5 DEL 3 LIST 2]
            set subEvent [string toupper [lindex $argList 1]]

            if {[info exists params($subEvent)] && $argLength == $params($subEvent)} {
                set result [SiteApprove $subEvent [lrange $argList 2 end]]
            } else {
                iputs "Syntax: SITE APPROVE ADD <release>"
                iputs "        SITE APPROVE DEL <release>"
                iputs "        SITE APPROVE LIST"
            }
        }
        CLEAN {
            set result [CleanDb]
        }
        DUPE {
            if {$argLength > 1 && [GetOptions [lrange $argList 1 end] limit pattern]} {
                set result [SiteDupe "Dupe" $limit $pattern]
            } else {
                iputs "Syntax: SITE DUPE \[-max <limit>\] <release>"
            }
        }
        FDUPE {
            if {$argLength > 1 && [GetOptions [lrange $argList 1 end] limit pattern]} {
                set result [SiteFileDupe $limit $pattern]
            } else {
                iputs "Syntax: SITE FDUPE \[-max <limit>\] <filename>"
            }
        }
        NEW {
            if {[GetOptions [lrange $argList 1 end] limit sectionName]} {
                set result [SiteNew $limit $sectionName]
            } else {
                iputs "Syntax: SITE NEW \[-max <limit>\] \[section\]"
            }
        }
        REBUILD {
            set result [RebuildDb]
        }
        SEARCH {
            if {$argLength > 1 && [GetOptions [lrange $argList 1 end] limit pattern]} {
                set result [SiteDupe "Search" $limit $pattern]
            } else {
                iputs "Syntax: SITE SEARCH \[-max <limit>\] <release>"
            }
        }
        UNDUPE {
            if {$argLength > 1} {
                set result [SiteUndupe [lrange $argList 1 end]]
            } else {
                iputs "Syntax: SITE UNDUPE <filename>"
                iputs "        SITE UNDUPE -d <directory>"
            }
        }
        WIPE {
            if {$argLength > 1} {
                set virtualPath [GetPath [join [lrange $argList 1 end]] $pwd]
                set result [SiteWipe $virtualPath]
            } else {
                iputs " Usage: SITE WIPE <file/directory>"
            }
        }
        default {
            ErrorLog InvalidArgs "unknown event \"[info script] $event\": check your ioFTPD.ini for errors"
            set result 1
        }
    }
    return [set ioerror $result]
}

::nxTools::Dupe::Main [expr {[info exists args] ? $args : ""}]
