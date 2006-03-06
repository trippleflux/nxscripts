#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   glFTPD Data Module
#
# Author:
#   neoxed (neoxed@gmail.com) Nov 13, 2005
#
# Abstract:
#   Implements a module to interact with glFTPD's binary data files.
#

namespace eval ::alcoholicz::GlData {
    if {![info exists [namespace current]::logsPath]} {
        variable logsPath ""
        variable tempPath ""
        variable undupeChars 5
        variable undupeWild 0
    }
    namespace import -force ::alcoholicz::*
    namespace import -force ::alcoholicz::FtpDaemon::*
}

####
# OpenBinaryFile
#
# Opens a binary file located in glFTPD's log directory.
#
proc ::alcoholicz::GlData::OpenBinaryFile {filePath {mode "r"}} {
    variable logsPath
    if {[catch {set handle [open $filePath $mode]} message]} {
        LogError ModGlData $message
        return ""
    }
    fconfigure $handle -translation binary
    return $handle
}

####
# StructOpen
#
# Opens glFTPD binary structure file for reading.
#
proc ::alcoholicz::GlData::StructOpen {fileName handleVar {backwards 1}} {
    variable logsPath
    variable structHandles
    variable structLength
    upvar $handleVar handle

    # Sanity check.
    set name [file rootname $fileName]
    if {![info exists structLength($name)]} {
        error "invalid structure name \"$name\""
    }
    set handle [OpenBinaryFile [file join $logsPath $fileName]]
    if {$handle eq ""} {return 0}

    # Set the file access pointer to the end if we are reading
    # the file backwards (newer entries are at the end).
    if {[set backwards [IsTrue $backwards]]} {
        seek $handle 0 end
    }

    # Format: backwards structName structLength
    set structHandles($handle) [list $backwards $name $structLength($name)]
    return 1
}

####
# StructRead
#
# Reads an entry from a glFTPD binary file.
#
proc ::alcoholicz::GlData::StructRead {handle dataVar} {
    variable structHandles
    upvar $dataVar data
    foreach {backwards structName structLength} $structHandles($handle) {break}

    if {$backwards && [catch {seek $handle -$structLength current}]} {
        # We've reached the beginning of the file.
        return 0
    }
    set data [read $handle $structLength]
    if {[string length $data] != $structLength} {
        return 0
    }
    if {$backwards} {
        seek $handle -$structLength current
    }
    return 1
}

####
# StructClose
#
# Closes a glFTPD binary file.
#
proc ::alcoholicz::GlData::StructClose {handle} {
    variable structHandles
    unset structHandles($handle)
    close $handle
}

####
# Dupe
#
# Search the dupelog for a release, command: !dupe [-limit <num>] <pattern>.
#
proc ::alcoholicz::GlData::Dupe {command target user host handle channel argv} {
    variable logsPath

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target dupeHead [list $pattern]
    set data [list]
    if {![catch {set handle [open [file join $logsPath "dupelog"] r]} message]} {
        set range [expr {$limit - 1}]

        while {![eof $handle]} {
            # Every line in the dupelog file should be at least eight
            # characters (timestamp is 6 characters, a space, and a path).
            if {[gets $handle line] < 8} {continue}
            set release [string range $line 7 end]

            if {[string match -nocase $pattern $release]} {
                set data [lrange [linsert $data 0 $line] 0 $range]
            }
        }
        close $handle
    } else {
        LogError ModGlData $message
    }

    # Display results.
    if {[llength $data]} {
        set count 0
        foreach item $data {
            incr count
            set month [string range $item 0 1]
            set day   [string range $item 2 3]
            set year  [string range $item 4 5]
            set time  [clock scan "$month/$day/$year"]
            set release [string range $item 7 end]

            SendTargetTheme $target dupeBody [list $count $release $time]
        }
    } else {
        SendTargetTheme $target dupeNone [list $pattern]
    }
    SendTargetTheme $target dupeFoot
}

####
# New
#
# Display recent releases, command: !new [-limit <num>] [pattern].
#
proc ::alcoholicz::GlData::New {command target user host handle channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]
    SendTargetTheme $target newHead

    set count 0
    if {[StructOpen "dirlog" handle]} {
        while {$count < $limit && [StructRead $handle data]} {
            if {[binary scan $data $structFormat(dirlog) status {} time userId groupId files {} bytes path]} {
                # Status Values:
                # 0 = NEWDIR
                # 1 = NUKE
                # 2 = UNNUKE
                # 3 = DELETED
                if {$status == 0 && ($pattern eq "" || [string match -nocase $pattern [file tail $path]])} {
                    incr count
                    set age [expr {[clock seconds] - $time}]
                    set user [UserIdToName $userId]
                    set group [GroupIdToName $groupId]

                    SendTargetTheme $target newBody [list $count \
                        $user $group $path $time $age $files $bytes]
                }
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target newNone}
    SendTargetTheme $target newFoot
}

####
# Search
#
# Search the dirlog for a release, command: !search [-limit <num>] <pattern>.
#
proc ::alcoholicz::GlData::Search {command target user host handle channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target searchHead [list $pattern]
    set count 0
    if {[StructOpen "dirlog" handle]} {
        while {$count < $limit && [StructRead $handle data]} {
            if {[binary scan $data $structFormat(dirlog) status {} time userId groupId files {} bytes path]} {
                if {[string match -nocase $pattern [file tail $path]]} {
                    incr count
                    set age [expr {[clock seconds] - $time}]
                    set user [UserIdToName $userId]
                    set group [GroupIdToName $groupId]

                    SendTargetTheme $target newBody [list $count \
                        $user $group $path $time $age $files $bytes]
                }
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target searchNone [list $pattern]}
    SendTargetTheme $target searchFoot
}

####
# Undupe
#
# Remove a file the dupefile log, command: !undupe <pattern>.
#
proc ::alcoholicz::GlData::Undupe {command target user host handle channel argv} {
    variable logsPath
    variable tempPath
    variable undupeChars
    variable undupeWild
    variable structFormat

    # Parse command options.
    if {[set pattern [join $argv]] eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    if {[string first "?" $pattern] != -1 || [string first "*" $pattern] != -1 } {
        if {!$undupeWild} {
            throw CMDHELP "wildcards are not allowed"
        }
        if {[regexp -all -- {[[:alnum:]]} $pattern] < $undupeChars} {
            throw CMDHELP "you must specify at least $undupeChars alphanumeric chars with wildcards"
        }
    }
    set patternEsc [string map {[ \\[ ] \\]} $pattern]

    SendTargetTheme $target undupeHead [list $pattern]
    set count 0
    if {[StructOpen "dupefile" handle 0]} {
        # Open a temporary file for writing.
        set tempFile [file join $tempPath "dupefile-[clock seconds]"]
        set tempHandle [OpenBinaryFile $tempFile "w"]

        if {$tempHandle ne ""} {
            while {[StructRead $handle data]} {
                if {[binary scan $data $structFormat(dupefile) file time user]} {
                    # Write all non-matching entries to the temporary file.
                    if {[string match -nocase $patternEsc $file]} {
                        incr count
                        SendTargetTheme $target undupeBody [list $count $file $time]
                    } else {
                        puts -nonewline $tempHandle $data
                    }
                }
            }
            close $tempHandle
        }
        StructClose $handle

        if {$count} {
            # Replace the existing dupe file with the temporary file.
            set dupeFile [file join $logsPath "dupefile"]
            if {[catch {file rename -force -- $tempFile $dupeFile} message]} {
                LogError ModGlData $message
            } else {
                # Chmod the new dupefile to 0666.
                catch {file attributes $dupeFile -permissions 0666}
            }
        }
        catch {file delete -- $tempFile}
    }

    if {!$count} {SendTargetTheme $target undupeNone [list $pattern]}
    SendTargetTheme $target undupeFoot
}

####
# Nukes
#
# Display recent nukes, command: !nukes [-limit <num>] [pattern].
#
proc ::alcoholicz::GlData::Nukes {command target user host handle channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target nukesHead
    set count 0
    if {[StructOpen "nukelog" handle]} {
        while {$count < $limit && [StructRead $handle data]} {
            if {[binary scan $data $structFormat(nukelog) status {} time nuker unnuker nukee multi {} bytes reason path]} {
                # Status Values:
                # 0 = NUKED
                # 1 = UNNUKED
                if {$status == 0 && ($pattern eq "" || [string match -nocase $pattern [file tail $path]])} {
                    incr count
                    set age [expr {[clock seconds] - $time}]

                    SendTargetTheme $target nukesBody [list $count \
                        $nuker $path $time $age $multi $reason $bytes]
                }
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target nukesNone}
    SendTargetTheme $target nukesFoot
}

####
# Unnukes
#
# Display recent unnukes, command: !unnukes [-limit <num>] [pattern].
#
proc ::alcoholicz::GlData::Unnukes {command target user host handle channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target unnukesHead
    set count 0
    if {[StructOpen "nukelog" handle]} {
        while {$count < $limit && [StructRead $handle data]} {
            if {[binary scan $data $structFormat(nukelog) status {} time nuker unnuker nukee multi {} bytes reason path]} {
                # Status Values:
                # 0 = NUKED
                # 1 = UNNUKED
                if {$status == 1 && ($pattern eq "" || [string match -nocase $pattern [file tail $path]])} {
                    incr count
                    set age [expr {[clock seconds] - $time}]

                    SendTargetTheme $target unnukesBody [list $count \
                        $unnuker $path $time $age $multi $reason $bytes]
                }
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target unnukesNone}
    SendTargetTheme $target unnukesFoot
}

####
# OneLines
#
# Display recent one-lines, command: !onel [-limit <num>].
#
proc ::alcoholicz::GlData::OneLines {command target user host handle channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    GetOpt::Parse $argv {{limit integer}} option
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target oneLinesHead
    set count 0
    if {[StructOpen "oneliners.log" handle]} {
        while {$count < $limit && [StructRead $handle data]} {
            if {[binary scan $data $structFormat(oneliners) user group tagline time message]} {
                incr count
                set age [expr {[clock seconds] - $time}]

                SendTargetTheme $target oneLinesBody [list $count \
                    $user $group $message $time $age]
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target oneLinesNone}
    SendTargetTheme $target oneLinesFoot
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::GlData::Load {firstLoad} {
    variable logsPath
    variable tempPath
    variable undupeChars
    variable undupeWild
    variable structFormat
    variable structLength
    upvar ::alcoholicz::configHandle configHandle

    # For 32-bit little endian systems.
    array set structFormat {
        dirlog    ssisssswA255
        dupefile  A256iA25
        nukelog   ssiA12A12A12ssfA60A255
        oneliners A24A24A64iA100
    }
    array set structLength {
        dirlog    288
        dupefile  288
        nukelog   376
        oneliners 216
    }

    # Retrieve configuration options.
    foreach option {tempPath undupeChars undupeWild} {
        set $option [Config::Get $configHandle Module::GlData $option]
    }
    if {![file isdirectory $tempPath]} {
        error "the directory \"$tempPath\" does not exist"
    }
    set undupeWild [IsTrue $undupeWild]

    set logsPath [file join [Config::Get $configHandle Ftpd dataPath] "logs"]
    if {![file isdirectory $logsPath]} {
        error "the directory \"$logsPath\" does not exist"
    }

    if {[Config::Exists $configHandle Module::GlData cmdPrefix]} {
        set prefix [Config::Get $configHandle Module::GlData cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    # Directory commands.
    CmdCreate channel dupe   [namespace current]::Dupe \
        -category "Data"  -args "\[-limit <num>\] <pattern>" \
        -prefix   $prefix -desc "Search the dupe database for a release."

    CmdCreate channel new    [namespace current]::New \
        -category "Data"  -args "\[-limit <num>\] \[pattern\]" \
        -prefix   $prefix -desc "Display new releases."

    CmdCreate channel search [namespace current]::Search \
        -category "Data"  -args "\[-limit <num>\] <pattern>" \
        -prefix   $prefix -desc "Search the site for a release."

    CmdCreate channel undupe [namespace current]::Undupe \
        -category "Data"  -args "<pattern>" \
        -prefix   $prefix -desc "Undupe files and directories."

    # Nuke commands.
    CmdCreate channel nukes   [namespace current]::Nukes \
        -category "Data"  -args "\[-limit <num>\] \[pattern\]" \
        -prefix   $prefix -desc "Display recent nukes."

    CmdCreate channel unnukes [namespace current]::Unnukes \
        -category "Data"  -args "\[-limit <num>\] \[pattern\]" \
        -prefix   $prefix -desc "Display recent unnukes."

    # Other commands.
    CmdCreate channel onel    [namespace current]::OneLines \
        -category "General" -args "\[-limit <num>\]" \
        -prefix   $prefix   -desc "Display recent one-lines."
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::GlData::Unload {} {
}
