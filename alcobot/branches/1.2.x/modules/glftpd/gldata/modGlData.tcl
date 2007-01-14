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

namespace eval ::Bot::Mod::GlData {
    if {![info exists [namespace current]::cmdTokens]} {
        variable cmdTokens [list]
        variable logsPath ""
        variable tempPath ""
        variable undupeChars 5
        variable undupeWild 0
    }
    namespace import -force ::Bot::*
    namespace import -force ::Bot::Mod::Ftpd::*
}

####
# OpenBinaryFile
#
# Opens a binary file located in glFTPD's log directory.
#
proc ::Bot::Mod::GlData::OpenBinaryFile {filePath {mode "r"}} {
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
proc ::Bot::Mod::GlData::StructOpen {fileName handleVar {backwards 1}} {
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
proc ::Bot::Mod::GlData::StructRead {handle dataVar} {
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
proc ::Bot::Mod::GlData::StructClose {handle} {
    variable structHandles
    unset structHandles($handle)
    close $handle
}

####
# Dupe
#
# Search the dupelog for a release, command: !dupe [-limit <num>] <pattern>.
#
proc ::Bot::Mod::GlData::Dupe {target user host channel argv} {
    variable logsPath

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target Module::GlData dupeHead [list $pattern]
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

            SendTargetTheme $target Module::GlData dupeBody [list $count $release $time]
        }
    } else {
        SendTargetTheme $target Module::GlData dupeNone [list $pattern]
    }
    SendTargetTheme $target Module::GlData dupeFoot
}

####
# New
#
# Display recent releases, command: !new [-limit <num>] [pattern].
#
proc ::Bot::Mod::GlData::New {target user host channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]
    SendTargetTheme $target Module::GlData newHead

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

                    SendTargetTheme $target Module::GlData newBody [list $count \
                        $user $group $path $time $age $files $bytes]
                }
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target Module::GlData newNone}
    SendTargetTheme $target Module::GlData newFoot
}

####
# Search
#
# Search the dirlog for a release, command: !search [-limit <num>] <pattern>.
#
proc ::Bot::Mod::GlData::Search {target user host channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target Module::GlData searchHead [list $pattern]
    set count 0
    if {[StructOpen "dirlog" handle]} {
        while {$count < $limit && [StructRead $handle data]} {
            if {[binary scan $data $structFormat(dirlog) status {} time userId groupId files {} bytes path]} {
                if {[string match -nocase $pattern [file tail $path]]} {
                    incr count
                    set age [expr {[clock seconds] - $time}]
                    set user [UserIdToName $userId]
                    set group [GroupIdToName $groupId]

                    SendTargetTheme $target Module::GlData newBody [list $count \
                        $user $group $path $time $age $files $bytes]
                }
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target Module::GlData searchNone [list $pattern]}
    SendTargetTheme $target Module::GlData searchFoot
}

####
# Undupe
#
# Remove a file the dupefile log, command: !undupe <pattern>.
#
proc ::Bot::Mod::GlData::Undupe {target user host channel argv} {
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

    SendTargetTheme $target Module::GlData undupeHead [list $pattern]
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
                        SendTargetTheme $target Module::GlData undupeBody [list $count $file $time]
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

    if {!$count} {SendTargetTheme $target Module::GlData undupeNone [list $pattern]}
    SendTargetTheme $target Module::GlData undupeFoot
}

####
# Nukes
#
# Display recent nukes, command: !nukes [-limit <num>] [pattern].
#
proc ::Bot::Mod::GlData::Nukes {target user host channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target Module::GlData nukesHead
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

                    SendTargetTheme $target Module::GlData nukesBody [list $count \
                        $nuker $path $time $age $multi $reason $bytes]
                }
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target Module::GlData nukesNone}
    SendTargetTheme $target Module::GlData nukesFoot
}

####
# Unnukes
#
# Display recent unnukes, command: !unnukes [-limit <num>] [pattern].
#
proc ::Bot::Mod::GlData::Unnukes {target user host channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target Module::GlData unnukesHead
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

                    SendTargetTheme $target Module::GlData unnukesBody [list $count \
                        $unnuker $path $time $age $multi $reason $bytes]
                }
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target Module::GlData unnukesNone}
    SendTargetTheme $target Module::GlData unnukesFoot
}

####
# OneLines
#
# Display recent one-lines, command: !onel [-limit <num>].
#
proc ::Bot::Mod::GlData::OneLines {target user host channel argv} {
    variable structFormat

    # Parse command options.
    set option(limit) -1
    GetOpt::Parse $argv {{limit integer}} option
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target Module::GlData oneLinesHead
    set count 0
    if {[StructOpen "oneliners.log" handle]} {
        while {$count < $limit && [StructRead $handle data]} {
            if {[binary scan $data $structFormat(oneliners) user group tagline time message]} {
                incr count
                set age [expr {[clock seconds] - $time}]

                SendTargetTheme $target Module::GlData oneLinesBody [list $count \
                    $user $group $message $time $age]
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target Module::GlData oneLinesNone}
    SendTargetTheme $target Module::GlData oneLinesFoot
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::GlData::Load {firstLoad} {
    variable cmdTokens
    variable logsPath
    variable tempPath
    variable undupeChars
    variable undupeWild
    variable structFormat
    variable structLength
    upvar ::Bot::configHandle configHandle

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
    set cmdTokens [list]

    # Directory commands.
    lappend cmdTokens [CmdCreate channel dupe [namespace current]::Dupe \
        -args "\[-limit <num>\] <pattern>" \
        -category "Data" -desc "Search the dupe database for a release."]

    lappend cmdTokens [CmdCreate channel new [namespace current]::New \
        -args "\[-limit <num>\] \[pattern\]" \
        -category "Data" -desc "Display new releases."]

    lappend cmdTokens [CmdCreate channel search [namespace current]::Search \
        -args "\[-limit <num>\] <pattern>" \
        -category "Data" -desc "Search the site for a release."]

    lappend cmdTokens [CmdCreate channel undupe [namespace current]::Undupe \
        -args "<pattern>" \
        -category "Data" -desc "Undupe files and directories."]

    # Nuke commands.
    lappend cmdTokens [CmdCreate channel nukes [namespace current]::Nukes \
        -args "\[-limit <num>\] \[pattern\]" \
        -category "Data" -desc "Display recent nukes."]

    lappend cmdTokens [CmdCreate channel unnukes [namespace current]::Unnukes \
        -args "\[-limit <num>\] \[pattern\]" \
        -category "Data" -desc "Display recent unnukes."]

    # Other commands.
    lappend cmdTokens [CmdCreate channel onel [namespace current]::OneLines \
        -args "\[-limit <num>\]" \
        -category "General" -desc "Display recent one-lines."]
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::GlData::Unload {} {
    variable cmdTokens
    foreach token $cmdTokens {
        CmdRemoveByToken $token
    }
}
