#
# nxScripts - Scripts by neoxed.
# Copyright (c) 2004-2006 neoxed
#
# Module Name:
#   Library
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements commonly used functions.
#

namespace eval ::nxLib {
    variable logPath "../logs"
    namespace export *
}

# General Procedures
######################################################################

interp alias {} IsTrue {} string is true -strict
interp alias {} IsFalse {} string is false -strict

proc ::nxLib::GetOptions {argList limitVar stringVar} {
    global misc
    upvar $limitVar limit $stringVar string

    set option [string tolower [lindex $argList 0]]
    if {[string index $option 0] eq "-"} {
        set option [string range $option 1 end]
        if {$option eq "limit" || $option eq "max"} {
            set limit [lindex $argList 1]
            set string [join [lrange $argList 2 end]]
            if {![string is digit -strict $limit] || $limit < 1} {
                set limit $misc(DefaultLimit)
            }
        } else {
            return 0
        }
    } else {
        set limit $misc(DefaultLimit)
        set string [join $argList]
    }
    return 1
}

proc ::nxLib::ErrorReturn {message} {
    LinePuts $message
    iputs "'------------------------------------------------------------------------'"
    return -code return
}

proc ::nxLib::LinePuts {args} {
    set message [format "| %-70s |" [lindex $args end]]
    set args [lreplace $args end end $message]
    eval iputs $args
}

proc ::nxLib::StripChars {value} {
    regsub -all -- {[\(\<\{]+} $value {(} value
    regsub -all -- {[\)\>\}]+} $value {)} value
    regsub -all -- {[^\w\-\(\)]+} $value {.} value
    return [string trim $value "."]
}

proc ::nxLib::WordWrap {text width} {
    set result [list]
    while {[string length $text] > $width} {
        set index [string last { } $text $width]
        if {$index < 0} {set index $width}
        lappend result [string trim [string range $text 0 $index]]
        set text [string range $text [expr {$index + 1}] end]
    }
    if {[string length $text]} {lappend result $text}
    return $result
}

# DataBase Procedures
######################################################################

proc ::nxLib::DbOpenFile {dbProc fileName} {
    set filePath [file join $::misc(DataPath) $fileName]
    if {![file exists $filePath]} {
        return -code error "the database \"$filePath\" does not exist: please run \"SITE DB CREATE\""
    } elseif {[catch {sqlite3 $dbProc $filePath} error]} {
        return -code error "unable to open \"$filePath\": $error"
    }
    $dbProc busy ::nxLib::DbBusyHandler
    return
}

proc ::nxLib::DbBusyHandler {tries} {
    # Give up after 50 attempts, although it should succeed after 1-5.
    if {$tries > 50} {return 1}
    ::nx::sleep 250
    return 0
}

proc ::nxLib::DbEscape {value} {
    return [string map {' ''} $value]
}

proc ::nxLib::DbPattern {pattern} {
    set pattern "*$pattern*"
    regsub -all -- {[\s\*]+} $pattern "*" pattern
    return [DbToLike $pattern]
}

proc ::nxLib::DbToLike {pattern} {
    set pattern [DbEscape $pattern]
    return [string map {* % ? _} [string map {% \\% _ \\_} $pattern]]
}

# File and Directory Procedures
######################################################################

proc ::nxLib::ArchiveFile {filePath {formatStyle "%Y-%m-%d"}} {
    global log
    if {![file isdirectory $log(ArchivePath)]} {
        if {[catch {file mkdir $log(ArchivePath)} error]} {
            ErrorLog ArchiveMkDir $error; return 0
        }
    }
    set datePrefix [clock format [clock seconds] -format $formatStyle -gmt 0]
    set archivePath [file join $log(ArchivePath) "$datePrefix.[file tail $filePath]"]
    if {[catch {file rename -- $filePath $archivePath} error]} {
        ErrorLog ArchiveFile $error; return 0
    }
    return 1
}

proc ::nxLib::CreateTag {realPath userId groupId chmod} {
    if {[catch {file mkdir $realPath} error]} {
        ErrorLog CreateTag $error
    }
    catch {vfs write $realPath $userId $groupId $chmod}
}

proc ::nxLib::RemoveTag {realPath} {
    # Safely remove a directory tag, in case there are other files inside it.
    catch {file delete -- [file join $realPath ".ioFTPD"]}
    if {[catch {file delete -- $realPath} error]} {
        ErrorLog RemoveTag $error
    }
}

proc ::nxLib::GetDirList {realPath varName {ignoreList ""}} {
    upvar $varName data
    array set data [list DirList [list] FileList [list]]

    GetDirListRecurse $realPath data $ignoreList
    return
}

proc ::nxLib::GetDirListRecurse {realPath varName {ignoreList ""}} {
    upvar $varName data

    if {[file isdirectory $realPath]} {
        lappend data(DirList) $realPath
        set listing [glob -nocomplain -directory $realPath "*"]
    } elseif {[file isfile $realPath]} {
        set listing [list $realPath]
    } else {return}

    foreach entry $listing {
        if {[file readable $entry] && ![ListMatchI $ignoreList [file tail $entry]]} {
            if {[file isdirectory $entry]} {
                GetDirListRecurse $entry data $ignoreList
            } else {
                lappend data(FileList) $entry
            }
        }
    }
}

proc ::nxLib::GetDirStats {realPath varName {ignoreList ""}} {
    upvar $varName stats
    array set stats [list DirCount 0 FileCount 0 TotalSize 0]

    GetDirStatsRecurse $realPath data $ignoreList
    return
}

proc ::nxLib::GetDirStatsRecurse {realPath varName {ignoreList ""}} {
    upvar $varName stats

    if {[file isdirectory $realPath]} {
        incr stats(DirCount)
        set listing [glob -nocomplain -directory $realPath "*"]
    } elseif {[file isfile $realPath]} {
        set listing [list $realPath]
    } else {return}

    foreach entry $listing {
        if {[file readable $entry] && ![ListMatchI $ignoreList [file tail $entry]]} {
            if {[file isdirectory $entry]} {
                GetDirStatsRecurse $entry stats $ignoreList
            } else {
                incr stats(FileCount)
                set stats(TotalSize) [expr {wide($stats(TotalSize)) + wide([file size $entry])}]
            }
        }
    }
}

proc ::nxLib::GetPath {path workingPath} {
    # Absolute path or relative path.
    if {[string index $path 0] ne "/"} {
        set path "$workingPath/$path"
    }
    set path [string trim $path "/\\"]

    # Resolve the "." and ".." path components.
    set components [list]
    foreach component [SplitPath $path] {
        if {$component eq ".."} {
            set components [lreplace $components end end]
        } elseif {$component ne "."} {
            lappend components $component
        }
    }
    return "/[join $components /]"
}

proc ::nxLib::IsDiskPath {path} {
    set path [string tolower [file tail $path]]
    return [regexp -- {^(cd|dis[ck]|dvd)\d{1,2}$} $path]
}

proc ::nxLib::RemoveParentLinks {realPath args} {
    if {[IsDiskPath $realPath]} {
        set realPath [file dirname $realPath]
    }
    set realPath [file dirname $realPath]
    set realPathLen [string length $realPath]

    foreach symPath [glob -nocomplain -types d -directory $realPath "*"] {
        if {[catch {vfs chattr $symPath 1} symTarget] || $symTarget eq ""} {continue}
        set symRealTarget [resolve pwd $symTarget]

        if {[string equal -nocase -length $realPathLen $realPath $symRealTarget]} {
            RemoveTag $symPath
        }
    }
}

proc ::nxLib::ResolvePath {userName groupName realPath} {
    set bestMatch 0
    set resolvePath "/"
    set vfsFile ""
    set realPath [string map {\\ /} $realPath]

    # Find the user VFS file.
    if {[userfile open $userName] == 0} {
        set userFile [userfile bin2ascii]
        foreach line [split $userFile "\r\n"] {
            if {[string equal -nocase "vfsfile" [lindex $line 0]]} {
                set vfsFile [ListRange $line 1 end]; break
            }
        }
    }
    # Use the group VFS file if the user VFS file does not exist.
    if {![file isfile $vfsFile] && [groupfile open $groupName] == 0} {
        set groupFile [groupfile bin2ascii]
        foreach line [split $groupFile "\r\n"] {
            if {[string equal -nocase "vfsfile" [lindex $line 0]]} {
                set vfsFile [ListRange $line 1 end]; break
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
            set char [string index $line 0]
            if {$char eq "" || $char eq ";" || $char eq "#"} {continue}

            # Parse the VFS line: "<base path>" <mount path>
            if {![regexp -- {^\"(.+)\"\s+(.+)$} $line result basePath mountPath]} {continue}
            set basePath [string map {\\ /} $basePath]
            set mountPath [string map {\\ /} $mountPath]

            # Compare only the length of the current base path
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

proc ::nxLib::SplitPath {path} {
    regsub -all -- {[\\/]+} $path {/} path
    return [file split [string trim $path "/"]]
}

# ioFTPD Related Procedures
######################################################################

proc ::nxLib::GetSectionList {} {
    set inSections 0
    set sectionList [list]
    if {![catch {set handle [open "ioFTPD.ini" r]} error]} {
        while {![eof $handle]} {
            set line [string trim [gets $handle]]
            set char [string index $line 0]
            if {$char eq "" || $char eq ";" || $char eq "#"} {continue}

            if {$line eq "\[Sections\]"} {
                set inSections 1
            } elseif {$inSections} {
                if {[string match {\[*\]} $line]} {
                    set inSections 0
                } else {
                    foreach {sectionName dummy creditSection argOne argTwo argThree} $line {break}
                    switch -- [llength $line] {
                        4 {lappend sectionList $sectionName $creditSection 0 $argOne}
                        5 {lappend sectionList $sectionName $creditSection $argOne $argTwo}
                        6 {lappend sectionList $sectionName $creditSection $argOne $argThree}
                        default {ErrorLog GetSectionList "invalid ioFTPD.ini \[Sections\] line: \"$line\""}
                    }
                }
            }
        }
        close $handle
    } else {ErrorLog GetSectionList $error}
    return $sectionList
}

proc ::nxLib::GetSectionName {virtualPath {sectionList ""}} {
    if {![llength $sectionList]} {set sectionList [GetSectionList]}
    foreach {sectionName creditSection statSection matchPath} $sectionList {
        if {[string match -nocase $matchPath $virtualPath]} {
            return $sectionName
        }
    }
    return "DEFAULT"
}

proc ::nxLib::GetCreditStatSections {virtualPath {sectionList ""}} {
    if {![llength $sectionList]} {set sectionList [GetSectionList]}
    foreach {sectionName creditSection statSection matchPath} $sectionList {
        if {[string match -nocase $matchPath $virtualPath]} {
            return [list $creditSection $statSection]
        }
    }
    return [list 0 0]
}

proc ::nxLib::KickUsers {path {isRealPath "False"}} {
    if {[IsTrue $isRealPath]} {
        catch {client kill realpath $path}
    } else {
        catch {client kill virtualpath $path}
    }
    set path [string map {\[ \\\[ \] \\\]} $path]
    ::nx::sleep 200

    # Repeat the kicking process 20 times to ensure users were disconnected.
    for {set i 0} {$i < 20} {incr i} {
        if {[client who init "CID" "STATUS" "VIRTUALPATH" "VIRTUALDATAPATH"] == 0} {
            set online 0
            while {[set whoData [client who fetch]] != ""} {
                foreach {clientId status virtualPath dataPath} $whoData {break}
                if {[IsTrue $isRealPath]} {
                    set virtualPath [resolve pwd $virtualPath]
                    set dataPath [resolve pwd $dataPath]
                }

                # After a transfer the user's data path will be the last file
                # transfered; however, their status will be IDLE. Bug?
                if {$status == 1 || $status == 2} {
                    set matchPath $dataPath
                } else {
                    if {[string index $virtualPath end] ne "/"} {append virtualPath "/"}
                    set matchPath $virtualPath
                }

                # Attempt to kick the client ID.
                if {[string match -nocase $path $matchPath]} {
                    incr online
                    catch {client kill clientid $clientId}
                }
            }
            if {!$online} {return}
        }
        ::nx::sleep 250
    }
}

# List Procedures
######################################################################

proc ::nxLib::ListAssign {values args} {
    while {[llength $values] < [llength $args]} {
        lappend values {}
    }
    uplevel [list foreach $args $values break]
}

proc ::nxLib::ListConvert {list {word "and"}} {
    if {[llength $list] < 2} {return [join $list]}
    set result [join [lrange $list 0 end-1] ", "]
    if {[llength $list] > 2} {
        append result ","
    }
    return [append result " " $word " " [lindex $list end]]
}

proc ::nxLib::ListMatch {patterns value} {
    foreach pattern $patterns {
        if {[string match $pattern $value]} {return 1}
    }
    return 0
}

proc ::nxLib::ListMatchI {patterns value} {
    foreach pattern $patterns {
        if {[string match -nocase $pattern $value]} {return 1}
    }
    return 0
}

proc ::nxLib::ListParse {argv} {
    set argList [list]
    set length [string length $argv]

    for {set index 0} {$index < $length} {incr index} {
        # Ignore leading white-space.
        while {[string is space -strict [string index $argv $index]]} {incr index}
        if {$index >= $length} {break}

        if {[string index $argv $index] eq "\""} {
            # Find the next quote character.
            set startIndex [incr index]
            while {[string index $argv $index] ne "\"" && $index < $length} {incr index}
        } else {
            # Find the next white-space character.
            set startIndex $index
            while {![string is space -strict [string index $argv $index]] && $index < $length} {incr index}
        }
        lappend argList [string range $argv $startIndex [expr {$index - 1}]]
    }
    return $argList
}

proc ::nxLib::ListRange {value start end} {
    regsub -all -- {\s+} $value { } value
    join [lrange [split [string trim $value]] $start $end]
}

# Logging Procedures
######################################################################

proc ::nxLib::DebugLog {type message} {
    global args flags group groups path pwd user
    variable logPath

    set filePath [file join $logPath "nxDebug.log"]
    if {![catch {set handle [open $filePath a]} error]} {
        set now [clock format [clock seconds] -format "%m-%d-%Y %H:%M:%S"]
        if {$type eq "-state"} {
            puts $handle "$now -------------------------------------------------------------------"
            puts $handle "$now - [format %-12s Script] : $message"
            foreach varName {args user group groups flags path pwd} {
                set message [expr {[info exists $varName] ? [set $varName] : ""}]
                puts $handle "$now - [format %-12s $varName] : $message"
            }
        } else {
            puts $handle "$now - [format %-12s $type] : $message"
        }
        close $handle
    } else {iputs $error}
}

proc ::nxLib::ErrorLog {type message} {
    variable logPath

    set filePath [file join $logPath "nxError.log"]
    if {![catch {set handle [open $filePath a]} error]} {
        set now [clock format [clock seconds] -format "%m-%d-%Y %H:%M:%S"]
        puts $handle "$now - [format %-12s $type] : $message"
        close $handle
    } else {iputs $error}
}

# Formatting Procedures
######################################################################

proc ::nxLib::FormatDuration {seconds} {
    set duration [list]
    foreach div {31536000 604800 86400 3600 60 1} unit {y w d h m s} {
        set num [expr {$seconds / $div}]
        if {$num > 0} {lappend duration "$num$unit"}
        set seconds [expr {$seconds % $div}]
    }
    if {[llength $duration]} {return [join $duration]} else {return "0s"}
}

proc ::nxLib::FormatDurationLong {seconds} {
    set duration [list]
    foreach div {31536000 604800 86400 3600 60 1} unit {year week day hour min sec} {
        set num [expr {$seconds / $div}]
        if {$num > 1} {
            lappend duration "$num ${unit}s"
        } elseif {$num == 1} {
            lappend duration "$num $unit"
        }
        set seconds [expr {$seconds % $div}]
    }
    if {[llength $duration]} {return [join $duration {, }]} else {return "0 secs"}
}

proc ::nxLib::FormatSize {kiloBytes} {
    foreach decimals {0 1 2 2} unit {KB MB GB TB} {
        if {abs($kiloBytes) < 1024} {break}
        set kiloBytes [expr {double($kiloBytes) / 1024.0}]
    }
    return [format "%.${decimals}f%s" $kiloBytes $unit]
}

proc ::nxLib::FormatSpeed {speed {seconds 0}} {
    if {$seconds > 0} {set speed [expr {double($speed) / $seconds}]}
    foreach decimals {0 2 2} unit {KB/s MB/s GB/s} {
        if {abs($speed) < 1024} {break}
        set speed [expr {double($speed) / 1024.0}]
    }
    return [format "%.${decimals}f%s" $speed $unit]
}

# User and Group Procedures
######################################################################

proc ::nxLib::GetUserList {} {
    set userList [list]
    foreach userId [user list] {lappend userList [resolve uid $userId]}
    return [lsort -ascii $userList]
}

proc ::nxLib::GetUserInfo {userName groupVar flagsVar} {
    upvar $groupVar group $flagsVar flags
    set group "NoGroup"; set flags ""

    if {[userfile open $userName] == 0} {
        set userFile [userfile bin2ascii]
        foreach line [split $userFile "\r\n"] {
            set type [string tolower [lindex $line 0]]

            if {$type eq "flags"} {
                set flags [lindex $line 1]
            } elseif {$type eq "groups"} {
                set group [GetGroupName [lindex $line 1]]
            }
        }
    }
    return
}

proc ::nxLib::GetGroupList {} {
    set groupList [list]
    foreach groupId [group list] {lappend groupList [resolve gid $groupId]}
    return [lsort -ascii $groupList]
}

proc ::nxLib::GetGroupName {groupId} {
    set groupName [resolve gid $groupId]
    if {$groupName eq ""} {
        return "NoGroup"
    }
    return $groupName
}

proc ::nxLib::GetGroupUsers {groupId} {
    set userList [list]
    foreach userName [GetUserList] {
        if {[userfile open $userName] != 0} {continue}
        set userFile [userfile bin2ascii]
        if {[regexp -nocase -- {groups ([\s\d]+)} $userFile result groupIdList]} {
            if {[lsearch -exact $groupIdList $groupId] != -1} {lappend userList $userName}
        }
    }
    return $userList
}

proc ::nxLib::MatchFlags {required current} {
    set current [split $current {}]
    foreach flag [split $required {}] {
        if {[lsearch -exact $current $flag] != -1} {return 1}
    }
    return 0
}

proc ::nxLib::MergeStats {stats filesVar sizeVar timeVar} {
    upvar $filesVar files $sizeVar size $timeVar time
    foreach {sectionFiles sectionSize sectionTime} $stats {
        set files [expr {wide($files) + wide($sectionFiles)}]
        set size [expr {wide($size) + wide($sectionSize)}]
        set time [expr {wide($time) + wide($sectionTime)}]
    }
}

# Cookie Parsing Procedures
######################################################################

proc ::nxLib::OutputText {text} {
    foreach line [split $text "\r\n"] {
        if {[string length $line]} {iputs $line}
    }
}

proc ::nxLib::ReadFile {filePath} {
    set data ""
    if {![catch {set handle [open $filePath r]} error]} {
        set data [read -nonewline $handle]
        close $handle
    } else {ErrorLog ReadFile $error}
    return $data
}

proc ::nxLib::ParseCookies {input valueList cookieList} {
    set inputLen [string length $input]
    set output ""

    for {set inputIdx 0} {$inputIdx < $inputLen} {incr inputIdx} {
        if {[string index $input $inputIdx] eq "%"} {
            # Save this index for invalid cookies.
            set startIdx $inputIdx

            # Find position field
            set beforeIdx [incr inputIdx]
            if {[string index $input $inputIdx] eq "-"} {
                # Ignore the negative sign if a does not number follow, for example: %-(cookie).
                if {[string is digit -strict [string index $input [incr inputIdx]]]} {incr inputIdx} else {incr beforeIdx}
            }
            while {[string is digit -strict [string index $input $inputIdx]]} {incr inputIdx}
            if {$beforeIdx != $inputIdx} {
                set rightPos [string range $input $beforeIdx [expr {$inputIdx - 1}]]
            } else {
                set rightPos 0
            }

            # Find minimum/precision field.
            if {[string index $input $inputIdx] eq "."} {
                set beforeIdx [incr inputIdx]
                # Ignore the negative sign, for example: %.-(cookie).
                if {[string index $input $inputIdx] eq "-"} {incr beforeIdx; incr inputIdx}
                while {[string is digit -strict [string index $input $inputIdx]]} {incr inputIdx}
                if {$beforeIdx != $inputIdx} {
                    set leftPos [string range $input $beforeIdx [expr {$inputIdx - 1}]]
                } else {
                    set leftPos 0
                }
            } else {
                # Tcl's [format ...] function doesn't accept -1 for the minimum field
                # like printf() does, so a reasonably large number will suffice.
                set leftPos 999999
            }

            # Find cookie name.
            if {[string index $input $inputIdx] eq "("} {
                set beforeIdx [incr inputIdx]
                while {[string index $input $inputIdx] ne ")" && $inputIdx < $inputLen} {incr inputIdx}
                set cookie [string range $input $beforeIdx [expr {$inputIdx - 1}]]
            } else {
                # Invalid cookie format, an open parenthesis is expected.
                append output [string range $input $startIdx $inputIdx]
                continue
            }

            if {[set index [lsearch -exact $cookieList $cookie]] != -1} {
                set value [lindex $valueList $index]
                # Type of cookie substitution to perform.
                if {[string is integer -strict $value]} {
                    append output [format "%${rightPos}i" $value]
                } elseif {[regexp -- {^-?[0-9]+\.[0-9]+$} $value]} {
                    append output [format "%${rightPos}.${leftPos}f" $value]
                } else {
                    append output [format "%${rightPos}.${leftPos}s" $value]
                }
            } else {
                # Append the starting point of the cookie to the current index in hope that
                # the user will notice that he or she has made an error in the template line.
                append output [string range $input $startIdx $inputIdx]
            }
        } else {
            append output [string index $input $inputIdx]
        }
    }
    return $output
}
