################################################################################
# nxTools - Dupe Checking and File Forcing                                     #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

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
            set dupeAge [FormatDuration [expr {[clock seconds] - $values(TimeStamp)}]]
            set dupePath [file join $values(DirPath) $values(DirName)]
            iputs -noprefix "553-.-\[DupeCheck\]-------------------------------------------------."
            iputs -noprefix "553-| [format %-59s "Dupe: $dupePath"] |"
            iputs -noprefix "553-| [format %-59s "Created $dupeAge ago by $values(UserName)."] |"
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
            set dupeAge [FormatDuration [expr {[clock seconds] - $values(TimeStamp)}]]
            iputs -noprefix "553-.-\[DupeCheck\]-------------------------------------------------."
            iputs -noprefix "553-| [format %-59s "Dupe: $values(FileName)"] |"
            iputs -noprefix "553-| [format %-59s "Uploaded $dupeAge ago by $values(UserName)."] |"
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
            ErrorLog DupeClean "unable to open the user \"$misc(MountUser)\""; return 1
        } elseif {[mountfile open $misc(MountFile)] != 0} {
            ErrorLog DupeClean "unable to mount the VFS-file \"$misc(MountFile)\""; return 1
        }
    }
    iputs ".-\[DupeClean\]------------------------------------------------------------."
    if {$dupe(CleanFiles) < 1} {
        LinePuts "File database cleaning disabled, skipping."
    } elseif {[catch {DbOpenFile [namespace current]::FileDb "DupeFiles.db"} error]} {
        LinePuts "Unable to open the file database."
        ErrorLog CleanFiles $error
    } else {
        LinePuts "Cleaning the file database."
        set maxAge [expr {[clock seconds] - ($dupe(CleanFiles) * 86400)}]
        FileDb eval {DELETE FROM DupeFiles WHERE TimeStamp < $maxAge}
        FileDb close
    }

    if {$dupe(CleanFiles) < 1} {
        LinePuts "Directory database cleaning disabled, skipping."
    } elseif {[catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
        LinePuts "Unable to open the directory database."
        ErrorLog CleanDirs $error
    } else {
        LinePuts "Cleaning the directory database."
        set maxAge [expr {[clock seconds] - ($dupe(CleanDirs) * 86400)}]
        DirDb eval {BEGIN}
        DirDb eval {SELECT DirPath,DirName,rowid FROM DupeDirs WHERE TimeStamp < $maxAge ORDER BY TimeStamp DESC} values {
            set fullPath [file join $values(DirPath) $values(DirName)]
            if {![file isdirectory [resolve pwd $fullPath]]} {
                DirDb eval {DELETE FROM DupeDirs WHERE rowid=$values(rowid)}
            }
        }
        DirDb eval {COMMIT}
        DirDb close
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Dupe::RebuildDb {} {
    global dupe misc
    iputs ".-\[DupeUpdate\]-----------------------------------------------------------."
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
        set trimLength [expr {[string length [file normalize $realPath]] + 1}]

        LinePuts "Updating dupe database from: $realPath"
        GetDirList $realPath dirlist $dupe(RebuildIgnore)

        foreach listName {DirList FileList} defOwner [list $misc(DirOwner) $misc(FileOwner)] {
            if {[set fileMode [string equal "FileList" $listName]]} {
                if {![IsTrue $updateFiles]} {continue}
                set ignoreList $dupe(IgnoreFiles)
                set maxAge [expr {$dupe(CleanFiles) * 86400}]
            } else {
                if {![IsTrue $updateDirs]} {continue}
                set ignoreList $dupe(IgnoreDirs)
                set maxAge 0
            }
            set defUser [resolve uid [lindex $defOwner 0]]
            set defGroup [resolve gid [lindex $defOwner 1]]

            foreach listItem $dirlist($listName) {
                if {[ListMatchI $ignoreList $listItem]} {continue}

                if {[catch {file stat $listItem fstat}]} {continue}
                if {$maxAge != 0 && ([clock seconds] - $fstat(ctime)) > $maxAge} {continue}
                catch {vfs read $listItem} owner
                if {[set userName [resolve uid [lindex $owner 0]]] eq ""} {set userName $defUser}
                if {[set groupName [resolve gid [lindex $owner 1]]] eq ""} {set groupName $defGroup}

                set baseName [file tail $listItem]
                if {$fileMode} {
                    FileDb eval {INSERT INTO DupeFiles(TimeStamp,UserName,GroupName,FileName) VALUES($fstat(ctime),$userName,$groupName,$baseName)}
                } else {
                    set dirPath [file join $virtualPath [string range [file dirname $listItem] $trimLength end]]
                    append dirPath "/"
                    DirDb eval {INSERT INTO DupeDirs(TimeStamp,UserName,GroupName,DirPath,DirName) VALUES($fstat(ctime),$userName,$groupName,$dirPath,$baseName)}
                }
            }
        }
    }
    DirDb eval {COMMIT}
    DirDb close
    FileDb eval {COMMIT}
    FileDb close
    iputs "'------------------------------------------------------------------------'"
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

proc ::nxTools::Dupe::PreTimeCheck {virtualPath} {
    global misc mysql pretime
    if {[ListMatchI $pretime(Ignores) $virtualPath]} {return 0}
    set check 0; set result 0
    foreach preCheck $pretime(CheckPaths) {
        if {[llength $preCheck] != 4} {
            ErrorLog PreTimeCheck "wrong number of parameters in line: \"$preCheck\""; continue
        }
        foreach {checkPath denyLate logInfo lateMins} $preCheck {break}
        if {[string match $checkPath $virtualPath]} {set check 1; break}
    }
    if {$check && [MySqlConnect]} {
        set releaseName [::mysql::escape [file tail $virtualPath]]
        set timeStamp [::mysql::sel $mysql(ConnHandle) "SELECT pretime FROM $mysql(TableName) WHERE release='$releaseName' LIMIT 1" -flatlist]

        if {[string is digit -strict $timeStamp]} {
            if {[set releaseAge [expr {[clock seconds] - $timeStamp}]] > [set lateSecs [expr {$lateMins * 60}]]} {
                if {[IsTrue $denyLate]} {
                    set errCode 553; set logPrefix "DENYPRE"; set result 1
                    set errMsg "Release not allowed by pre rules, older than [FormatDurationLong $lateSecs]."
                } else {
                    set errCode 257; set logPrefix "WARNPRE"
                    set errMsg "Release older than [FormatDurationLong $lateSecs], possible nuke."
                }
                iputs -noprefix "${ErrCode}-.-\[PreCheck\]--------------------------------------------------."
                iputs -noprefix "${ErrCode}-| [format %-59s $errMsg] |"
                iputs -noprefix "${ErrCode}-| [format %-59s "Pre'd [FormatDurationLong $releaseAge] ago."] |"
                iputs -noprefix "${ErrCode} '-------------------------------------------------------------'"
                if {[IsTrue $logInfo]} {
                    if {[IsTrue $misc(dZSbotLogging)]} {
                        set lateSecs $lateMins
                        set releaseAge [FormatDuration $releaseAge]
                        set timeStamp [clock format $timeStamp -format "%m/%d/%y %H:%M:%S" -gmt 1]
                    }
                    putlog "${LogPrefix}: \"$virtualPath\" \"$lateSecs\" \"$releaseAge\" \"$timeStamp\""
                }
            } elseif {[IsTrue $logInfo]} {
                if {[IsTrue $misc(dZSbotLogging)]} {
                    set releaseAge [FormatDuration $releaseAge]
                    set timeStamp [clock format $timeStamp -format "%m/%d/%y %H:%M:%S" -gmt 1]
                }
                putlog "PRETIME: \"$virtualPath\" \"$releaseAge\" \"$timeStamp\""
            }
        }
        MySqlClose
    }
    return $result
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
    set tagName [file tail $virtualPath]
    if {$latest(MaxLength) > 0 && [string length $tagName] > $latest(MaxLength)} {
        set tagName [string trimright [string range $tagName 0 $latest(MaxLength)] "."]
    }
    set tagName [string map [list %(release) $tagName] $latest(RaceTag)]
    set timeStamp [clock seconds]
    LinkDb eval {INSERT INTO Links(TimeStamp,LinkType,DirName) VALUES($timeStamp,0,$tagName)}
    set tagName [file join $latest(SymPath) $tagName]
    if {![catch {file mkdir $tagName} error]} {
        catch {vfs chattr $tagName 1 $virtualPath}
    } else {ErrorLog RaceLinksMkDir $error}

    # Remove older links.
    if {[set linkCount [LinkDb eval {SELECT count(*) FROM Links WHERE LinkType=0}]] > $latest(RaceLinks)} {
        set linkCount [expr {$linkCount - $latest(RaceLinks)}]
        LinkDb eval "SELECT DirName,rowid FROM Links WHERE LinkType=0 ORDER BY TimeStamp ASC LIMIT $linkCount" values {
            RemoveTag [file join $latest(SymPath) $values(DirName)]
            LinkDb eval {DELETE FROM Links WHERE rowid=$values(rowid)}
        }
    }
    LinkDb close
    return 0
}

# Site Commands
######################################################################

proc ::nxTools::Dupe::SiteApprove {event release} {
    global IsSiteBot approve misc flags group user
    if {[catch {DbOpenFile [namespace current]::ApproveDb "Approves.db"} error]} {
        ErrorLog SiteApprove $error
        return 1
    }
    set release [file tail $release]
    switch -- $event {
        {ADD} {
            iputs ".-\[Approve\]--------------------------------------------------------------."
            if {![MatchFlags $approve(Flags) $flags]} {
                LinePuts "Only siteops may approve releases."
            } elseif {[ApproveDb eval {SELECT count(*) FROM Approves WHERE StrCaseEq(Release,$release)}]} {
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
        {DEL} {
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
        {LIST} {
            if {!$isSiteBot} {
                foreach fileExt {Header Body None Footer} {
                    set template($fileExt) [ReadFile [file join $misc(Templates) "Approves.$fileExt"]]
                }
                OutputText $template(Header)
                set count 0
            }
            ApproveDb eval {SELECT * FROM Approves ORDER BY Release ASC} values {
                set approveAge [expr {[clock seconds] - $values(TimeStamp)}]
                if {$isSiteBot} {
                    iputs [list APPROVE $values(TimeStamp) $approveAge $values(UserName) $values(GroupName) $values(Release)]
                } else {
                    incr count
                    set approveAge [lrange [FormatDuration $approveAge] 0 1]
                    set valueList [list $count $approveAge $values(UserName) $values(GroupName) $values(Release)]
                    OutputText [ParseCookies $template(Body) $valueList {num age user group release}]
                }
            }
            if {!$isSiteBot} {
                if {!$count} {OutputText $template(None)}
                OutputText $template(Footer)
            }
        }
    }
    ApproveDb close
    return 0
}

proc ::nxTools::Dupe::SiteDupe {maxResults pattern} {
    global IsSiteBot misc
    if {[catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
        ErrorLog SiteDupe $error
        return 1
    }
    if {!$isSiteBot} {
        foreach fileExt {Header Body None Footer} {
            set template($fileExt) [ReadFile [file join $misc(Templates) "Dupe.$fileExt"]]
        }
        OutputText $template(Header)
    }
    set pattern [SqlWildToLike [regsub -all {[\s\*]+} "*$pattern*" "*"]]
    set count 0
    DirDb eval "SELECT * FROM DupeDirs WHERE DirName LIKE '$pattern' ESCAPE '\\' ORDER BY TimeStamp DESC LIMIT $maxResults" values {
        incr count
        if {$isSiteBot} {
            iputs [list DUPE $count $values(TimeStamp) $values(UserName) $values(GroupName) $values(DirPath) $values(DirName)]
        } else {
            set valueList [clock format $values(TimeStamp) -format {{%S} {%M} {%H} {%d} {%m} {%y} {%Y}} -gmt [IsTrue $misc(UtcTime)]]
            lappend valueList $count $values(UserName) $values(GroupName) $values(DirName) [file join $values(DirPath) $values(DirName)]
            OutputText [ParseCookies $template(Body) $valueList {sec min hour day month year2 year4 num user group release path}]
        }
    }
    if {!$isSiteBot} {
        if {!$count} {OutputText $template(None)}
        if {$count == $maxResults} {
            set total [DirDb eval "SELECT count(*) FROM DupeDirs WHERE DirName LIKE '$pattern' ESCAPE '\\'"]
        } else {
            set total $count
        }
        OutputText [ParseCookies $template(Footer) [list $count $total] {found total}]
    }
    DirDb close
    return 0
}

proc ::nxTools::Dupe::SiteFileDupe {maxResults pattern} {
    global IsSiteBot misc
    if {[catch {DbOpenFile [namespace current]::FileDb "DupeFiles.db"} error]} {
        ErrorLog SiteFileDupe $error
        return 1
    }
    if {!$isSiteBot} {
        foreach fileExt {Header Body None Footer} {
            set template($fileExt) [ReadFile [file join $misc(Templates) "FileDupe.$fileExt"]]
        }
        OutputText $template(Header)
    }
    set pattern [SqlWildToLike [regsub -all {[\s\*]+} "*$pattern*" "*"]]
    set count 0
    FileDb eval "SELECT * FROM DupeFiles WHERE FileName LIKE '$pattern' ESCAPE '\\' ORDER BY TimeStamp DESC LIMIT $maxResults" values {
        incr count
        if {$isSiteBot} {
            iputs [list DUPE $count $values(TimeStamp) $values(UserName) $values(GroupName) $values(FileName)]
        } else {
            set valueList [clock format $values(TimeStamp) -format {{%S} {%M} {%H} {%d} {%m} {%y} {%Y}} -gmt [IsTrue $misc(UtcTime)]]
            lappend valueList $count $values(UserName) $values(GroupName) $values(FileName)
            OutputText [ParseCookies $template(Body) $valueList {sec min hour day month year2 year4 num user group file}]
        }
    }
    if {!$isSiteBot} {
        if {!$count} {OutputText $template(None)}
        if {$count == $maxResults} {
            set total [FileDb eval "SELECT count(*) FROM DupeFiles WHERE FileName LIKE '$pattern' ESCAPE '\\'"]
        } else {
            set total $count
        }
        OutputText [ParseCookies $template(Footer) [list $count $total] {found total}]
    }
    FileDb close
    return 0
}

proc ::nxTools::Dupe::SiteNew {maxResults showSection} {
    global IsSiteBot misc new
    if {[catch {DbOpenFile [namespace current]::DirDb "DupeDirs.db"} error]} {
        ErrorLog SiteNew $error
        return 1
    }
    if {!$isSiteBot} {
        foreach fileExt {Header Error Body None Footer} {
            set template($fileExt) [ReadFile [file join $misc(Templates) "New.$fileExt"]]
        }
        OutputText $template(Header)
    }
    set sectionList [GetSectionList]
    if {![set showAll [string equal "" $showSection]]} {
        # Validate the section name.
        set sectionNameList ""; set validSection 0
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
        set matchPath [SqlWildToLike $matchPath]
        set whereClause "WHERE DirPath LIKE '$matchPath' ESCAPE '\\'"
    } else {set whereClause ""}

    set count 0
    DirDb eval "SELECT * FROM DupeDirs $whereClause ORDER BY TimeStamp DESC LIMIT $maxResults" values {
        incr count
        # Find section name and check the match path.
        if {$showAll} {
            set sectionName "Default"
            foreach {sectionName creditSection statSection matchPath} $sectionList {
                if {[string match -nocase $matchPath $values(DirPath)]} {
                    set showSection $sectionName; break
                }
            }
        }
        set releaseAge [expr {[clock seconds] - $values(TimeStamp)}]
        if {$isSiteBot} {
            iputs [list NEW $count $releaseAge $values(UserName) $values(GroupName) $showSection $values(DirPath) $values(DirName)]
        } else {
            set releaseAge [lrange [FormatDuration $releaseAge] 0 1]
            set valueList [list $count $releaseAge $values(UserName) $values(GroupName) $showSection $values(DirName) [file join $values(DirPath) $values(DirName)]]
            OutputText [ParseCookies $template(Body) $valueList {num age user group section release path}]
        }
    }
    if {!$isSiteBot} {
        if {!$count} {OutputText $template(None)}
        OutputText $template(Footer)
    }
    DirDb close
    return 0
}

proc ::nxTools::Dupe::SitePreTime {maxResults pattern} {
    global IsSiteBot misc mysql
    if {!$isSiteBot} {
        foreach fileExt {Header Body BodyInfo BodyNuke None Footer} {
            set template($fileExt) [ReadFile [file join $misc(Templates) "PreTime.$fileExt"]]
        }
        OutputText $template(Header)
    }
    set pattern [SqlWildToLike [regsub -all {[\s\*]+} "*$pattern*" "*"]]
    set count 0
    if {[MySqlConnect]} {
        set queryResults [::mysql::sel $mysql(ConnHandle) "SELECT * FROM $mysql(TableName) WHERE release LIKE '$pattern' ORDER BY pretime DESC LIMIT $maxResults" -list]
        set singleResult [expr {[llength $queryResults] == 1}]
        set timeNow [clock seconds]

        foreach queryLine $queryResults {
            incr count
            foreach {preId preTime section release files kiloBytes disks isNuked nukeTime nukeReason} $queryLine {break}
            set releaseAge [expr {$timeNow - $preTime}]
            if {$isSiteBot} {
                iputs [list PRETIME $count $releaseAge $preTime $section $release $files $kBytes $disks $isNuked $nukeTime $nukeReason]
            } else {
                set bodyTemplate [expr {$singleResult ? ($isNuked != 0 ? $template(BodyNuke) : $template(BodyInfo)) : $template(Body)}]
                set releaseAge [FormatDuration $releaseAge]

                # The pre time should always been in UTC (GMT).
                set valueList [clock format $preTime -format {{%S} {%M} {%H} {%d} {%m} {%y} {%Y}} -gmt 1]
                set valueList [concat $valueList [clock format $nukeTime -format {{%S} {%M} {%H} {%d} {%m} {%y} {%Y}} -gmt 1]]
                lappend valueList $releaseAge $count $section $release $files [FormatSize $kBytes] $disks $nukeReason
                OutputText [ParseCookies $bodyTemplate $valueList {sec min hour day month year2 year4 nukesec nukemin nukehour nukeday nukemonth nukeyear2 nukeyear4 age num section release files size disks reason}]
            }
        }
        MySqlClose
    }
    if {!$isSiteBot} {
        if {!$count} {OutputText $template(None)}
        OutputText $template(Footer)
    }
    return 0
}

proc ::nxTools::Dupe::SiteUndupe {argList} {
    global IsSiteBot dupe misc
    if {!$isSiteBot} {iputs ".-\[Undupe\]---------------------------------------------------------------."}
    if {[string equal -nocase "-d" [lindex $argList 0]]} {
        set colName "DirName"; set dbName "DupeDirs"
        set pattern [join [lrange $argList 1 end]]
    } else {
        set colName "FileName"; set dbName "DupeFiles"
        set pattern [join [lrange $argList 0 end]]
    }
    if {[regexp {[\*\?]} $pattern] && [regexp -all {[[:alnum:]]} $pattern] < $dupe(AlphaNumChars)} {
        if {!$isSiteBot} {ErrorReturn "There must be at $dupe(AlphaNumChars) least alpha-numeric chars when wildcards are used."}
        return 1
    }
    if {!$isSiteBot} {LinePuts "Searching for: $pattern"}
    set removed 0; set total 0
    set pattern [SqlWildToLike $pattern]

    if {![catch {DbOpenFile [namespace current]::DupeDb "${DbName}.db"} error]} {
        set total [DupeDb eval "SELECT count(*) FROM $dbName"]
        DupeDb eval {BEGIN}
        DupeDb eval "SELECT $colName,rowid FROM $dbName WHERE $colName LIKE '$pattern' ESCAPE '\\' ORDER BY $colName ASC" values {
            incr removed
            if {$isSiteBot} {
                iputs [list UNDUPE $values($colName)]
            } else {
                LinePuts "Unduped: $values($colName)"
            }
            DupeDb eval "DELETE FROM $dbName WHERE rowid=$values(rowid)"
        }
        DupeDb eval {COMMIT}
        DupeDb close
    }
    if {!$isSiteBot} {
        iputs "|------------------------------------------------------------------------|"
        LinePuts "Unduped $removed of $total dupe entries."
        iputs "'------------------------------------------------------------------------'"
    }
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

    if {$isDir} {
        RemoveParentLinks $realPath $virtualPath
        if {[IsTrue $misc(dZSbotLogging)]} {
            set stats(TotalSize) [expr {wide($stats(TotalSize)) / 1024}]
        }
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
    global IsSiteBot approve dupe force latest misc pretime group ioerror pwd user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set isSiteBot [expr {[info exists user] && $misc(SiteBot) eq $user}]
    set result 0

    set argLength [llength [set argList [ArgList $argv]]]
    set event [string toupper [lindex $argList 0]]
    switch -- $event {
        {DUPELOG} {
            set virtualPath [GetPath $pwd [join [lrange $argList 2 end]]]
            if {[IsTrue $dupe(CheckDirs)] || [IsTrue $dupe(CheckFiles)]} {
                set result [UpdateLog [lindex $argList 1] $virtualPath]
            }
        }
        {POSTMKD} {
            set virtualPath [GetPath $pwd [join [lrange $argList 2 end]]]
            if {[IsTrue $dupe(CheckDirs)]} {
                set result [UpdateLog [lindex $argList 1] $virtualPath]
            }
            if {$latest(RaceLinks) > 0} {
                set result [RaceLinks $virtualPath]
            }
            if {[IsTrue $approve(CheckMkd)]} {ApproveCheck $virtualPath 1}
        }
        {PREMKD} {
            set virtualPath [GetPath $pwd [join [lrange $argList 2 end]]]
            if {!([IsTrue $approve(CheckMkd)] && [ApproveCheck $virtualPath 0])} {
                if {[IsTrue $dupe(CheckDirs)]} {
                    set result [CheckDirs $virtualPath]
                }
                if {$result == 0 && [IsTrue $pretime(CheckMkd)]} {
                    set result [PreTimeCheck $virtualPath]
                }
            }
        }
        {PRESTOR} {
            set virtualPath [GetPath $pwd [join [lrange $argList 2 end]]]
            if {[IsTrue $force(NfoFirst)] || [IsTrue $force(SfvFirst)] || [IsTrue $force(SampleFirst)]} {
                set result [ForceCheck $virtualPath]
            }
            if {$result == 0 && [IsTrue $dupe(CheckFiles)]} {
                set result [CheckFiles $virtualPath]
            }
        }
        {UPLOAD} {
            if {[IsTrue $dupe(CheckFiles)]} {
                foreach {dummy realPath crc virtualPath} $argv {break}
                UpdateLog "UPLD" $virtualPath
            }
        }
        {UPLOADERROR} {
            if {[IsTrue $dupe(CheckFiles)]} {
                foreach {dummy realPath crc virtualPath} $argv {break}
                UpdateLog "DELE" $virtualPath
            }
        }
        {CLEAN} {
            set result [CleanDb]
        }
        {APPROVE} {
            array set params [list ADD 2 DEL 2 LIST 0]
            set subEvent [string toupper [lindex $argList 1]]

            if {[info exists params($subEvent)] && $argLength > $params($subEvent)} {
                set result [SiteApprove $subEvent [join [lrange $argList 2 end]]]
            } else {
                iputs "Syntax: SITE APPROVE ADD <release>"
                iputs "        SITE APPROVE DEL <release>"
                iputs "        SITE APPROVE LIST"
            }
        }
        {DUPE} {
            if {$argLength > 1 && [GetOptions [lrange $argList 1 end] maxResults Pattern]} {
                set result [SiteDupe $maxResults $pattern]
            } else {
                iputs "Syntax: SITE DUPE \[-max <limit>\] <release>"
            }
        }
        {FDUPE} {
            if {$argLength > 1 && [GetOptions [lrange $argList 1 end] maxResults Pattern]} {
                set result [SiteFileDupe $maxResults $pattern]
            } else {
                iputs "Syntax: SITE FDUPE \[-max <limit>\] <filename>"
            }
        }
        {NEW} {
            if {[GetOptions [lrange $argList 1 end] maxResults SectionName]} {
                set result [SiteNew $maxResults $sectionName]
            } else {
                iputs "Syntax: SITE NEW \[-max <limit>\] \[section\]"
            }
        }
        {PRETIME} {
            if {$argLength > 1 && [GetOptions [lrange $argList 1 end] maxResults Pattern]} {
                set result [SitePreTime $maxResults $pattern]
            } else {
                iputs "Syntax: SITE PRETIME \[-max <limit>\] <release>"
            }
        }
        {REBUILD} {
            set result [RebuildDb]
        }
        {UNDUPE} {
            if {$argLength > 1} {
                set result [SiteUndupe [lrange $argList 1 end]]
            } else {
                iputs "Syntax: SITE UNDUPE <filename>"
                iputs "        SITE UNDUPE -d <directory>"
            }
        }
        {WIPE} {
            if {$argLength > 1} {
                set virtualPath [GetPath $pwd [join [lrange $argList 1 end]]]
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
