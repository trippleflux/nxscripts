################################################################################
# nxTools - Common Functions Library                                           #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

namespace eval ::nxLib {
    variable LogPath "../logs"
    namespace export *
}

# General Procedures
######################################################################

interp alias {} IsTrue {} string is true -strict
interp alias {} IsFalse {} string is false -strict

## Safe argument handling
proc ::nxLib::ArgList {ArgV} {
    split [string trim [regsub -all {\s+} $ArgV { }]]
}
proc ::nxLib::ArgIndex {ArgV Index} {
    lindex [ArgList $ArgV] $Index
}
proc ::nxLib::ArgLength {ArgV} {
    llength [ArgList $ArgV]
}
proc ::nxLib::ArgRange {ArgV Start End} {
    join [lrange [ArgList $ArgV] $Start $End]
}

proc ::nxLib::GetOptions {ArgList MaxVar StringVar} {
    upvar $MaxVar MaxResults $StringVar String
    set Switch [string tolower [lindex $ArgList 0]]
    if {[string index $Switch 0] == "-"} {
        set Switch [string range $Switch 1 end]
        switch -exact -- $Switch {
            {m} -
            {max} {
                set MaxResults [lindex $ArgList 1]
                set String [join [lrange $ArgList 2 end]]
                if {![string is digit -strict $MaxResults] || $MaxResults < 1} {set MaxResults 10}
            }
            default {return 0}
        }
    } else {
        set MaxResults 10
        set String [join $ArgList]
    }
    return 1
}

proc ::nxLib::ErrorReturn {ErrorMsg} {
    LinePuts $ErrorMsg
    iputs "'------------------------------------------------------------------------'"
    return -code return
}

proc ::nxLib::LinePuts {String} {iputs [format "| %-70s |" $String]}

proc ::nxLib::StripChars {String} {
    regsub -all {[\(\<\{]+} $String {(} String
    regsub -all {[\)\>\}]+} $String {)} String
    regsub -all {[^A-Za-z0-9_\-\(\)]+} $String {.} String
    return [string trim $String "."]
}

proc ::nxLib::Sleep {MilliSeconds} {
    set UniqueKey [UniqueKey]
    set ::Sleep_$UniqueKey 0
    after $MilliSeconds [list set ::Sleep_$UniqueKey 1]
    vwait ::Sleep_$UniqueKey
    unset ::Sleep_$UniqueKey
}

proc ::nxLib::UniqueKey {} {
    set Key [expr {pow(2,31) + [clock clicks]}]
    set Key [string range $Key end-8 end-3]
    return "[clock seconds]$Key"
}

# DataBase Procedures
######################################################################

proc ::nxLib::DbOpenFile {DbProc FileName} {
    set DbPath [file join $::misc(DataPath) $FileName]
    if {![file exists $DbPath]} {
        return -code error "the database \"$DbPath\" does not exist: please run \"SITE DB CREATE\""
    if {[catch {sqlite3 $DbProc $DbPath} ErrorMsg]} {
        return -code error "unable to open \"$DbPath\": $ErrorMsg"
    }
    return 1
}

proc ::nxLib::MySqlConnect {} {
    global mysql
    if {[catch {set mysql(ConnHandle) [::mysql::connect -host $mysql(Host) -user $mysql(Username) -password $mysql(Password) -port $mysql(Port) -db $mysql(DataBase)]} ErrorMsg]} {
        ErrorLog MySqlConnect $ErrorMsg
    } elseif {[lsearch -exact [::mysql::info $mysql(ConnHandle) tables] $mysql(TableName)] != -1} {
        return 1
    } else {
        ErrorLog MySqlConnect "the table \"$mysql(TableName)\" does not exist in the database \"$mysql(DataBase)\""
        MySqlClose
    }
    set mysql(ConnHandle) -1
    return 0
}

proc ::nxLib::MySqlClose {} {
    global mysql
    if {$mysql(ConnHandle) != -1} {
        catch {::mysql::close $mysql(ConnHandle)}
        set mysql(ConnHandle) -1
    }
    return
}

proc ::nxLib::SqlEscape {String} {
    return [string map {\\ \\\\ \' \\\' \" \\\"} $String]
}

proc ::nxLib::SqlWildToLike {Pattern} {
    return [string map {* % ? _} [string map {% \\% _ \\_ \\ \\\\ \' \\\' \" \\\"} $Pattern]]
}

# File and Directory Procedures
######################################################################

proc ::nxLib::ArchiveFile {FilePath {FormatStyle "%Y-%m-%d"}} {
    global log
    if {![file isdirectory $log(ArchivePath)]} {
        if {[catch {file mkdir $log(ArchivePath)} ErrorMsg]} {
            ErrorLog ArchiveMkDir $ErrorMsg; return 0
        }
    }
    set DatePrefix [clock format [clock seconds] -format $FormatStyle -gmt 0]
    set ArchiveFile [file join $log(ArchivePath) "$DatePrefix.[file tail $FilePath]"]
    if {[catch {file copy -- $FilePath $ArchiveFile} ErrorMsg]} {
        ErrorLog ArchiveFile $ErrorMsg; return 0
    }
    return 1
}

proc ::nxLib::CreateTag {RealPath UserId GroupId Chmod} {
    if {[catch {file mkdir $RealPath} ErrorMsg]} {
        ErrorLog CreateTag $ErrorMsg
    }
    catch {vfs write $RealPath $UserId $GroupId $Chmod}
}

proc ::nxLib::RemoveTag {RealPath} {
    ## Safely remove a directory tag, in case there is data inside it.
    catch {file delete -- [file join $RealPath ".ioFTPD"]}
    if {[catch {file delete -- $RealPath} ErrorMsg]} {
        ErrorLog RemoveTag $ErrorMsg
    }
}

proc ::nxLib::GetDirList {RealPath VarName {IgnoreList ""} {FirstCall 1}} {
    upvar $VarName list
    if {$FirstCall} {
        array set list [list DirList "" FileList ""]
    }
    if {[file isdirectory $RealPath]} {
        lappend list(DirList) $RealPath
        set Listing [glob -nocomplain -directory $RealPath "*"]
    } elseif {[file isfile $RealPath]} {
        set Listing [list $RealPath]
    } else {return}
    foreach ListItem $Listing {
        if {[file readable $ListItem] && ![ListMatchI $IgnoreList [file tail $ListItem]]} {
            if {[file isdirectory $ListItem]} {
                GetDirList $ListItem list $IgnoreList 0
            } else {
                lappend list(FileList) $ListItem
            }
        }
    }
    return
}

proc ::nxLib::GetDirStats {RealPath VarName {IgnoreList ""} {FirstCall 1}} {
    upvar $VarName stats
    if {$FirstCall} {
        array set stats [list DirCount 0 FileCount 0 TotalSize 0]
    }
    if {[file isdirectory $RealPath]} {
        incr stats(DirCount)
        set Listing [glob -nocomplain -directory $RealPath "*"]
    } elseif {[file isfile $RealPath]} {
        set Listing [list $RealPath]
    } else {return}
    foreach ListItem $Listing {
        if {[file readable $ListItem] && ![ListMatchI $IgnoreList [file tail $ListItem]]} {
            if {[file isdirectory $ListItem]} {
                GetDirStats $ListItem stats $IgnoreList 0
            } else {
                incr stats(FileCount)
                set stats(TotalSize) [expr wide($stats(TotalSize)) + wide([file size $ListItem])]
            }
        }
    }
    return
}

proc ::nxLib::GetPath {PWD Path} {
    if {[string index $Path 0] == "/"} {set VirtualPath $Path} else {set VirtualPath "$PWD$Path"}
    regsub -all {[\\/]+} $Path {/} Path
    ## A few "security checks", in case $Path is "." or ".."
    if {[file tail $VirtualPath] == "." || [file tail $VirtualPath] == ".."} {
        set VirtualPath [file dirname $VirtualPath]
    } elseif {![string equal "/" $VirtualPath]} {
        set VirtualPath [string trimright $VirtualPath "/"]
    }
    return $VirtualPath
}

proc ::nxLib::IsMultiDisk {DiskPath} {
    set DiskPath [string tolower [file tail $DiskPath]]
    return [regexp {^(cd|dis[ck]|dvd)\d{1,2}$} $DiskPath]
}

proc ::nxLib::RemoveParentLinks {RealPath VirtualPath} {
    if {[IsMultiDisk $RealPath]} {
        set RealPath [file dirname $RealPath]
    }
    set RealPath [file dirname $RealPath]
    set VirtualPath [string trimright $VirtualPath "/"]
    set VirtualLen [string length $VirtualPath]
    foreach LinkPath [glob -nocomplain -types d -directory $RealPath "*"] {
        if {[catch {vfs chattr $LinkPath 1} LinkTarget] || [string equal "" $LinkTarget]} {continue}
        regsub -all {[\\/]+} $LinkTarget {/} LinkTarget
        set LinkTarget "/[string trim $LinkTarget {/}]"
        if {[string equal -nocase -length $VirtualLen $VirtualPath $LinkTarget]} {
            RemoveTag $LinkPath
        }
    }
    return
}

# ioFTPD Related Procedures
######################################################################

proc ::nxLib::GetSectionList {} {
    set IsSections 0
    set SectionList ""
    if {![catch {set Handle [open "ioFTPD.ini" r]} ErrorMsg]} {
        while {![eof $Handle]} {
            set ConfLine [string trim [gets $Handle]]
            if {[string index $ConfLine 0] == ";" || [string index $ConfLine 0] == "#"} {continue}
            if {[string equal {[Sections]} $ConfLine]} {
                set IsSections 1
            } elseif {$IsSections} {
                if {[string match {\[*\]} $ConfLine]} {
                    set IsSections 0
                } elseif {[set Items [llength $ConfLine]]} {
                    ## Check if the user was to lazy to define the stats section
                    foreach {SectionName EqSign CreditSection Param1 Param2} $ConfLine {break}
                    switch -exact -- $Items {
                        5 {lappend SectionList $SectionName $CreditSection $Param1 $Param2}
                        4 {lappend SectionList $SectionName $CreditSection 0 $Param1}
                        default {ErrorLog GetSectionList "invalid ioFTPD.ini \[Sections\] line: \"$ConfLine\""}
                    }
                }
            }
        }
        close $Handle
    } else {ErrorLog GetSectionList $ErrorMsg}
    return $SectionList
}

proc ::nxLib::GetSectionPath {FindSection {SectionList ""}} {
    if {![llength $SectionList]} {set SectionList [GetSectionList]}
    foreach {SectionName CreditSection StatSection MatchPath} $SectionList {
        if {[string equal -nocase $FindSection $SectionName]} {
            return [list $SectionName $MatchPath]
        }
    }
    return [list "DEFAULT" "*"]
}

proc ::nxLib::GetCreditStatSections {VirtualPath {SectionList ""}} {
    if {![llength $SectionList]} {set SectionList [GetSectionList]}
    foreach {SectionName CreditSection StatSection MatchPath} $SectionList {
        if {[string match -nocase $MatchPath $VirtualPath]} {
            return [list $CreditSection $StatSection]
        }
    }
    return [list 0 0]
}

proc ::nxLib::KickUsers {KickPath {RealPaths "False"}} {
    if {[IsTrue $RealPaths]} {
        catch {client kill realpath $KickPath}
    } else {
        catch {client kill virtualpath $KickPath}
    }
    set KickPath [string map {\[ \\\[ \] \\\]} $KickPath]
    Sleep 100

    ## Repeat the kicking process 20 times to ensure users were disconnected
    for {set Count 0} {$Count < 20} {incr Count} {
        if {[client who init "CID" "STATUS" "VIRTUALPATH" "VIRTUALDATAPATH"] == 0} {
            set UsersOnline 0
            while {[set WhoData [client who fetch]] != ""} {
                foreach {ClientId Status VfsPath DataPath} $WhoData {break}

                ## Resolve virtual paths if needed
                if {[IsTrue $RealPaths]} {
                    set VfsPath [resolve pwd $VfsPath]
                    set DataPath [resolve pwd $DataPath]
                }
                ## Following a transfer, the user's data path will be the last file
                ## transfered; however, their status will be IDLE. Bug?
                if {$Status == 1 || $Status == 2} {
                    set MatchPath $DataPath
                } else {
                    if {[string index $VfsPath end] != "/"} {append VfsPath "/"}
                    set MatchPath $VfsPath
                }
                ## Attempt to kick the client ID
                if {[string match -nocase $KickPath $MatchPath]} {
                    incr UsersOnline
                    catch {client kill clientid $ClientId}
                }
            }
            ## If there are no longer any users in that dir, we can return
            if {!$UsersOnline} {return}
        }
        Sleep 100
    }
}

# List Procedures
######################################################################

proc ::nxLib::ListAssign {ValueList args} {
    while {[llength $ValueList] < [llength $args]} {
        lappend ValueList {}
    }
    uplevel [list foreach $args $ValueList break]
}

proc ::nxLib::ListMatch {PatternList String} {
    foreach ListItem $PatternList {
        if {[string match $ListItem $String]} {return 1}
    }
    return 0
}

proc ::nxLib::ListMatchI {PatternList String} {
    foreach ListItem $PatternList {
        if {[string match -nocase $ListItem $String]} {return 1}
    }
    return 0
}

# Logging Procedures
######################################################################

proc ::nxLib::DebugLog {LogType LogMsg} {
    global args flags group groups path pwd user
    variable LogPath

    set LogFile [file join $LogPath "nxDebug.log"]
    if {![catch {set Handle [open $LogFile a]} ErrorMsg]} {
        set TimeNow [clock format [clock seconds] -format "%m-%d-%Y %H:%M:%S"]
        if {[string equal "-state" $LogType]} {
            puts $Handle "$TimeNow -------------------------------------------------------------------"
            puts $Handle "$TimeNow - [format %-12s Script] : $LogMsg"
            foreach EnvVar {args user group groups flags path pwd} {
                set LogMsg [expr {[info exists $EnvVar] ? [set $EnvVar] : ""}]
                puts $Handle "$TimeNow - [format %-12s $EnvVar] : $LogMsg"
            }
        } else {
            puts $Handle "$TimeNow - [format %-12s $LogType] : $LogMsg"
        }
        close $Handle
    } else {iputs $ErrorMsg}
}

proc ::nxLib::ErrorLog {LogType LogMsg} {
    variable LogPath

    set LogFile [file join $LogPath "nxError.log"]
    if {![catch {set Handle [open $LogFile a]} ErrorMsg]} {
        set TimeNow [clock format [clock seconds] -format "%m-%d-%Y %H:%M:%S"]
        puts $Handle "$TimeNow - [format %-12s $LogType] : $LogMsg"
        close $Handle
    } else {iputs $ErrorMsg}
}

# Formatting Procedures
######################################################################

proc ::nxLib::FormatDuration {Seconds} {
    set Duration ""
    foreach Div {31536000 604800 86400 3600 60 1} Mod {0 52 7 24 60 60} Unit {y w d h m s} {
        set Num [expr {$Seconds / $Div}]
        if {$Mod > 0} {set Num [expr {$Num % $Mod}]}
        if {$Num > 0} {lappend Duration "$Num$Unit"}
    }
    if {[llength $Duration]} {return [join $Duration]} else {return "0s"}
}

proc ::nxLib::FormatDurationLong {Seconds} {
    set Duration ""
    foreach Div {31536000 604800 86400 3600 60 1} Mod {0 52 7 24 60 60} Unit {year week day hour min sec} {
        set Num [expr {$Seconds / $Div}]
        if {$Mod > 0} {set Num [expr {$Num % $Mod}]}
        if {$Num > 1} {lappend Duration "$Num ${Unit}s"} elseif {$Num == 1} {lappend Duration "$Num $Unit"}
    }
    if {[llength $Duration]} {return [join $Duration {, }]} else {return "0 secs"}
}

proc ::nxLib::FormatSize {KBytes} {
    foreach Dec {0 1 2 2} Unit {KB MB GB TB} {
        if {abs($KBytes) >= 1024} {
            set KBytes [expr {double($KBytes) / 1024.0}]
        } else {break}
    }
    return [format "%.*f%s" $Dec $KBytes $Unit]
}

proc ::nxLib::FormatSpeed {Speed {Seconds 0}} {
    if {$Seconds > 0} {set Speed [expr {double($Speed) / $Seconds}]}
    foreach Dec {0 2 2} Unit {KB/s MB/s GB/s} {
        if {abs($Speed) >= 1024} {
            set Speed [expr {double($Speed) / 1024.0}]
        } else {break}
    }
    return [format "%.*f%s" $Dec $Speed $Unit]
}

# User and Group Procedures
######################################################################

proc ::nxLib::GetUserList {} {
    set UserList ""
    foreach UserId [user list] {lappend UserList [resolve uid $UserId]}
    return [lsort -ascii $UserList]
}

proc ::nxLib::GetGroupList {} {
    set GroupList ""
    foreach GroupId [group list] {lappend GroupList [resolve gid $GroupId]}
    return [lsort -ascii $GroupList]
}

proc ::nxLib::GetGroupName {GroupId} {
    if {[set GroupName [resolve gid $GroupId]] != ""} {
        return $GroupName
    }
    return "NoGroup"
}

proc ::nxLib::GetGroupUsers {GroupId} {
    set UserList ""
    foreach UserName [GetUserList] {
        if {[userfile open $UserName] != 0} {continue}
        set UserFile [userfile bin2ascii]
        if {[regexp -nocase {groups ([\s\d]+)} $UserFile Result GroupIdList]} {
            if {[lsearch -exact $GroupIdList $GroupId] != -1} {lappend UserList $UserName}
        }
    }
    return $UserList
}

proc ::nxLib::MergeStats {StatsLine FileVar SizeVar TimeVar} {
    upvar $FileVar FileStats $SizeVar SizeStats $TimeVar TimeStats
    foreach {File Size Time} $StatsLine {
        set FileStats [expr {wide($FileStats) + wide($File)}]
        set SizeStats [expr {wide($SizeStats) + wide($Size)}]
        set TimeStats [expr {wide($TimeStats) + wide($Time)}]
    }
}

# Cookie Parsing Procedures
######################################################################

proc ::nxLib::OutputData {OutputData} {
    foreach Output [split $OutputData "\r\n"] {
        if {![string equal "" $Output]} {iputs $Output}
    }
}

proc ::nxLib::ReadFile {FilePath} {
    set FileData ""
    if {![catch {set Handle [open $FilePath r]} ErrorMsg]} {
        set FileData [read -nonewline $Handle]
        close $Handle
    } else {ErrorLog ReadFile $ErrorMsg}
    return $FileData
}

proc ::nxLib::ParseCookies {InputStr ValueList CookieList} {
    set InputLen [string length $InputStr]
    set OutputStr ""

    for {set InputIdx 0} {$InputIdx < $InputLen} {incr InputIdx} {
        if {[string index $InputStr $InputIdx] == "%"} {
            ## Save this index for invalid cookies
            set StartIdx $InputIdx

            ## Find position field
            set BeforeIdx [incr InputIdx]
            if {[string index $InputStr $InputIdx] == "-"} {
                ## Ignore the negative sign if a does not number follow, for example: %-(cookie)
                if {[string is digit -strict [string index $InputStr [incr InputIdx]]]} {incr InputIdx} else {incr BeforeIdx}
            }
            while {[string is digit -strict [string index $InputStr $InputIdx]]} {incr InputIdx}
            if {$BeforeIdx != $InputIdx} {
                set RightPos [string range $InputStr $BeforeIdx [expr {$InputIdx - 1}]]
            } else {
                set RightPos 0
            }

            ## Find minimum/precision field
            if {[string index $InputStr $InputIdx] == "."} {
                set BeforeIdx [incr InputIdx]
                ## Ignore the negative sign, for example: %.-(cookie)
                if {[string index $InputStr $InputIdx] == "-"} {incr BeforeIdx; incr InputIdx}
                while {[string is digit -strict [string index $InputStr $InputIdx]]} {incr InputIdx}
                if {$BeforeIdx != $InputIdx} {
                    set LeftPos [string range $InputStr $BeforeIdx [expr {$InputIdx - 1}]]
                } else {
                    set LeftPos 0
                }
            } else {
                ## Tcl's [format ...] function doesn't accept -1 for the minimum field
                ## like printf() does, so a reasonably large number will suffice.
                set LeftPos 999999
            }

            ## Find cookie name
            if {[string index $InputStr $InputIdx] == "("} {
                set BeforeIdx [incr InputIdx]
                while {[string index $InputStr $InputIdx] != ")" && $InputIdx <= $InputLen} {incr InputIdx}
                set CookieName [string range $InputStr $BeforeIdx [expr {$InputIdx - 1}]]
            } else {
                ## Invalid cookie format, an open parenthesis is expected
                append OutputStr [string range $InputStr $StartIdx $InputIdx]
                continue
            }

            if {[set CookiePos [lsearch -exact $CookieList $CookieName]] != -1} {
                set Value [lindex $ValueList $CookiePos]
                ## Type of cookie substitution to perform
                if {[string is integer -strict $Value]} {
                    append OutputStr [format "%${RightPos}i" $Value]
                } elseif {[regexp {^-?[0-9]+\.[0-9]+$} $Value]} {
                    append OutputStr [format "%${RightPos}.${LeftPos}f" $Value]
                } else {
                    append OutputStr [format "%${RightPos}.${LeftPos}s" $Value]
                }
            } else {
                ## Append the starting point of the cookie to the current index in hope that
                ## the user will notice that he or she has made an error in the template line.
                append OutputStr [string range $InputStr $StartIdx $InputIdx]
            }
        } else {
            append OutputStr [string index $InputStr $InputIdx]
        }
    }
    return $OutputStr
}

# Reload Configuration
######################################################################
#
#if {[IsTrue $misc(ReloadConfig)]} {
#    if {[catch {source "../scripts/init.itcl"} ErrorMsg]} {
#        iputs "Unable to load script configuration, contact a siteop."
#        return -code error $ErrorMsg
#    }
#}
