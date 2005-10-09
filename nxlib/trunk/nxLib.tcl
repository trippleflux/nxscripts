#
# nxScripts - Scripts by neoxed.
# Copyright (c) 2004-2005 neoxed
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

proc ::nxLib::ArgList {argv} {
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

proc ::nxLib::StringRange {string start end} {
    regsub -all -- {\s+} $string { } string
    join [lrange [split [string trim $string]] $start $end]
}

proc ::nxLib::GetOptions {argList limitVar stringVar} {
    upvar $limitVar limit $stringVar string
    set option [string tolower [lindex $argList 0]]
    if {[string index $option 0] eq "-"} {
        set option [string range $option 1 end]
        if {$option eq "limit" || $option eq "max"} {
            set limit [lindex $argList 1]
            set string [join [lrange $argList 2 end]]
            if {![string is digit -strict $limit] || $limit < 1} {set limit 10}
        } else {
            return 0
        }
    } else {
        set limit 10
        set string [join $argList]
    }
    return 1
}

proc ::nxLib::JoinLiteral {list {word "and"}} {
    if {[llength $list] < 2} {return [join $list]}
    set listLiteral [join [lrange $list 0 end-1] ", "]
    if {[llength $list] > 2} {
        append listLiteral ","
    }
    return [append listLiteral " " $word " " [lindex $list end]]
}

proc ::nxLib::ErrorReturn {message} {
    LinePuts $message
    iputs "'------------------------------------------------------------------------'"
    return -code return
}

proc ::nxLib::LinePuts {message} {
    iputs [format "| %-70s |" $message]
}

proc ::nxLib::StripChars {string} {
    regsub -all -- {[\(\<\{]+} $string {(} string
    regsub -all -- {[\)\>\}]+} $string {)} string
    regsub -all -- {[^\w\-\(\)]+} $string {.} string
    return [string trim $string "."]
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
    ::nx::sleep 200
    return 0
}

proc ::nxLib::SqlGetPattern {pattern} {
    set pattern "*$pattern*"
    regsub -all {[\s\*]+} $pattern "*" pattern
    return [SqlWildToLike $pattern]
}

proc ::nxLib::SqlEscape {string} {
    return [string map {\\ \\\\ ` \\` ' \\' \" \\\"} $string]
}

proc ::nxLib::SqlWildToLike {pattern} {
    set pattern [SqlEscape $pattern]
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

proc ::nxLib::GetDirList {realPath varName {ignoreList ""} {firstCall 1}} {
    upvar $varName list
    if {$firstCall} {
        array set list [list DirList [list] FileList [list]]
    }
    if {[file isdirectory $realPath]} {
        lappend list(DirList) $realPath
        set listing [glob -nocomplain -directory $realPath "*"]
    } elseif {[file isfile $realPath]} {
        set listing [list $realPath]
    } else {return}

    foreach element $listing {
        if {[file readable $element] && ![ListMatchI $ignoreList [file tail $element]]} {
            if {[file isdirectory $element]} {
                GetDirList $element list $ignoreList 0
            } else {
                lappend list(FileList) $element
            }
        }
    }
    return
}

proc ::nxLib::GetDirStats {realPath varName {ignoreList ""} {firstCall 1}} {
    upvar $varName stats
    if {$firstCall} {
        array set stats [list DirCount 0 FileCount 0 TotalSize 0]
    }
    if {[file isdirectory $realPath]} {
        incr stats(DirCount)
        set listing [glob -nocomplain -directory $realPath "*"]
    } elseif {[file isfile $realPath]} {
        set listing [list $realPath]
    } else {return}

    foreach element $listing {
        if {[file readable $element] && ![ListMatchI $ignoreList [file tail $element]]} {
            if {[file isdirectory $element]} {
                GetDirStats $element stats $ignoreList 0
            } else {
                incr stats(FileCount)
                set stats(TotalSize) [expr {wide($stats(TotalSize)) + wide([file size $element])}]
            }
        }
    }
    return
}

proc ::nxLib::GetPath {currentPath path} {
    if {[string index $path 0] eq "/"} {
        set virtualPath $path
    } else {
        set virtualPath "/$currentPath$path"
    }
    regsub -all -- {[\\/]+} $virtualPath {/} virtualPath

    # Ignore "." and "..".
    set tail [file tail $virtualPath]
    if {$tail eq "." || $tail eq ".."} {
        set virtualPath [file dirname $virtualPath]
    } elseif {$virtualPath ne "/"} {
        set virtualPath [string trimright $virtualPath "/"]
    }
    return $virtualPath
}

proc ::nxLib::IsDiskPath {path} {
    set path [string tolower [file tail $path]]
    return [regexp {^(cd|dis[ck]|dvd)\d{1,2}$} $path]
}

proc ::nxLib::RemoveParentLinks {realPath virtualPath} {
    if {[IsDiskPath $realPath]} {
        set realPath [file dirname $realPath]
    }
    set realPath [file dirname $realPath]
    set virtualPath [string trimright $virtualPath "/"]

    foreach linkPath [glob -nocomplain -types d -directory $realPath "*"] {
        if {[catch {vfs chattr $linkPath 1} linkTarget] || ![string length $linkTarget]} {continue}
        regsub -all -- {[\\/]+} $linkTarget {/} linkTarget
        set linkTarget "/[string trim $linkTarget {/}]"
        if {[string equal -nocase -length [string length $virtualPath] $virtualPath $linkTarget]} {
            RemoveTag $linkPath
        }
    }
    return
}

# ioFTPD Related Procedures
######################################################################

proc ::nxLib::GetSectionList {} {
    set isSections 0
    set sectionList [list]
    if {![catch {set handle [open "ioFTPD.ini" r]} error]} {
        while {![eof $handle]} {
            set line [string trim [gets $handle]]
            if {[string index $line 0] eq ";" || [string index $line 0] eq "#"} {continue}
            if {$line eq {[Sections]}} {
                set isSections 1
            } elseif {$isSections} {
                if {[string match {\[*\]} $line]} {
                    set isSections 0
                } elseif {[set elements [llength $line]]} {
                    # Check if the user was to lazy to define the stats section
                    foreach {sectionName eqSign creditSection argOne argTwo} $line {break}
                    switch -- $elements {
                        5 {lappend sectionList $sectionName $creditSection $argOne $argTwo}
                        4 {lappend sectionList $sectionName $creditSection 0 $argOne}
                        default {ErrorLog GetSectionList "invalid ioFTPD.ini \[Sections\] line: \"$line\""}
                    }
                }
            }
        }
        close $handle
    } else {ErrorLog GetSectionList $error}
    return $sectionList
}

proc ::nxLib::GetSectionPath {findSection {sectionList ""}} {
    if {![llength $sectionList]} {set sectionList [GetSectionList]}
    foreach {sectionName creditSection statSection matchPath} $sectionList {
        if {[string equal -nocase $findSection $sectionName]} {
            return [list $sectionName $matchPath]
        }
    }
    return [list "DEFAULT" "*"]
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

proc ::nxLib::ListAssign {valueList args} {
    while {[llength $valueList] < [llength $args]} {
        lappend valueList {}
    }
    uplevel [list foreach $args $valueList break]
}

proc ::nxLib::ListMatch {patternList string} {
    foreach pattern $patternList {
        if {[string match $pattern $string]} {return 1}
    }
    return 0
}

proc ::nxLib::ListMatchI {patternList string} {
    foreach pattern $patternList {
        if {[string match -nocase $pattern $string]} {return 1}
    }
    return 0
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
    foreach divisor {31536000 604800 86400 3600 60 1} mod {0 52 7 24 60 60} unit {y w d h m s} {
        set num [expr {$seconds / $divisor}]
        if {$mod > 0} {set num [expr {$num % $mod}]}
        if {$num > 0} {lappend duration "$num$unit"}
    }
    if {[llength $duration]} {return [join $duration]} else {return "0s"}
}

proc ::nxLib::FormatDurationLong {seconds} {
    set duration [list]
    foreach divisor {31536000 604800 86400 3600 60 1} mod {0 52 7 24 60 60} unit {year week day hour min sec} {
        set num [expr {$seconds / $divisor}]
        if {$mod > 0} {set num [expr {$num % $mod}]}
        if {$num > 1} {
            lappend duration "$num ${unit}s"
        } elseif {$num == 1} {
            lappend duration "$num $unit"
        }
    }
    if {[llength $duration]} {return [join $duration {, }]} else {return "0 secs"}
}

proc ::nxLib::FormatSize {kiloBytes} {
    foreach decimals {0 1 2 2} unit {KB MB GB TB} {
        if {abs($kiloBytes) < 1024} {break}
        set kiloBytes [expr {double($kiloBytes) / 1024.0}]
    }
    return [format "%.*f%s" $decimals $kiloBytes $unit]
}

proc ::nxLib::FormatSpeed {speed {seconds 0}} {
    if {$seconds > 0} {set speed [expr {double($speed) / $seconds}]}
    foreach decimals {0 2 2} unit {KB/s MB/s GB/s} {
        if {abs($speed) < 1024} {break}
        set speed [expr {double($speed) / 1024.0}]
    }
    return [format "%.*f%s" $decimals $speed $unit]
}

# User and Group Procedures
######################################################################

proc ::nxLib::GetUserList {} {
    set userList [list]
    foreach userId [user list] {lappend userList [resolve uid $userId]}
    return [lsort -ascii $userList]
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
        if {[regexp -nocase {groups ([\s\d]+)} $userFile result groupIdList]} {
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
                } elseif {[regexp {^-?[0-9]+\.[0-9]+$} $value]} {
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