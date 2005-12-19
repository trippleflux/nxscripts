#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
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
    variable logsPath
    if {![info exists logsPath]} {
        set logsPath ""
    }
    namespace import -force ::alcoholicz::*
    namespace import -force ::alcoholicz::FtpDaemon::*
}

####
# OpenBinaryFile
#
# Opens a binary file located in glFTPD's log directory.
#
proc ::alcoholicz::GlData::OpenBinaryFile {name {mode "r"}} {
    variable logsPath
    set filePath [file join $logsPath $name]
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
proc ::alcoholicz::GlData::StructOpen {name handleVar {backwards 1}} {
    variable structHandles
    variable structLength
    upvar $handleVar handle

    # Sanity check.
    if {![info exists structLength($name)]} {
        error "invalid structure name \"$name\""
    }
    set handle [OpenBinaryFile $name]
    if {$handle eq ""} {return 0}

    # Set the file access pointer to the end if we are reading
    # the file backwards (newer entries are at the end).
    set backwards [IsTrue $backwards]
    if {$backwards} {
        seek $handle -$structLength($name) end
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

    set data [read $handle $structLength]
    if {[string length $data] != $structLength} {
        return 0
    }
    if {$backwards} {
        # Move two entries back.
        seek $handle [expr {$structLength * -2}] current
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
# Search for a release, command: !dupe [-limit <num>] [-section <name>] <pattern>.
#
proc ::alcoholicz::GlData::Dupe {command target user host handle channel argv} {
    variable structFormat
    upvar ::alcoholicz::pathSections pathSections

    # Parse command options.
    set option(limit) -1
    set optList [list {limit integer} [list section arg [lsort [array names pathSections]]]]

    if {[catch {set pattern [GetOptions $argv $optList option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    if {[set pattern [join $pattern]] eq ""} {
        CmdSendHelp $channel channel $command "you must specify a pattern"
        return
    }
    set option(limit) [GetResultLimit $option(limit)]

    if {[info exists option(section)]} {
        set section $option(section)
        set matchPath [lindex $pathSections($section) 0]
    } else {
        set section ""
        set matchPath ""
    }
    SendTargetTheme $target dupeHead [list $pattern]

    set count 0
    if {[StructOpen "dirlog" handle FALSE]} {
        while {$count < $option(limit) && [StructRead $handle data]} {
            if {[binary scan $data $structFormat(dirlog) status {} timeStamp userId groupId files {} bytes release]} {
                incr count
                putlog "\[$count\] $userId/$groupId at $files\F, $bytes\B for $release"
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target dupeNone [list $pattern]}
    SendTargetTheme $target dupeFoot
    return
}

####
# New
#
# Display recent releases, command: !new [-limit <num>] [section].
#
proc ::alcoholicz::GlData::New {command target user host handle channel argv} {
    variable structFormat
    upvar ::alcoholicz::pathSections pathSections

    # Parse command options.
    set option(limit) -1
    if {[catch {set section [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    set option(limit) [GetResultLimit $option(limit)]

    if {[set section [join $section]] eq ""} {
        set matchPath ""
    } else {
        # Validate the specified section name.
        set names [lsort [array names pathSections]]
        if {[catch {set section [GetElementFromList $names $section section]} message]} {
            CmdSendHelp $channel channel $command $message
            return
        }
        set matchPath [lindex $pathSections($section) 0]
    }
    SendTargetTheme $target newHead

    set count 0
    if {[StructOpen "dirlog" handle]} {
        while {$count < $option(limit) && [StructRead $handle data]} {
            if {[binary scan $data $structFormat(dirlog) status {} timeStamp userId groupId files {} bytes release]} {
                incr count
                putlog "\[$count\] $userId/$groupId at $files\F, $bytes\B for $release"
            }
        }
        StructClose $handle
    }

    if {!$count} {SendTargetTheme $target newNone}
    SendTargetTheme $target newFoot
    return
}

####
# Undupe
#
# Remove a file or directory from the dupe database, command: !undupe [-directory] <pattern>.
#
proc ::alcoholicz::GlData::Undupe {command target user host handle channel argv} {
    variable undupeChars

    # Parse command options.
    if {[set pattern [join $argv]] eq ""} {
        CmdSendHelp $channel channel $command "you must specify a pattern"
        return
    }
    if {[regexp -- {[\*\?]} $pattern] && [regexp -all -- {[[:alnum:]]} $pattern] < $undupeChars} {
        CmdSendHelp $channel channel $command "you must specify at least $undupeChars alphanumeric chars with wildcards"
        return
    }
    SendTargetTheme $target undupeHead [list $pattern]

    set count 0
    if {[set handle [OpenFile "dupefile"]] ne ""} {
        # TODO:
        # - Read log file.
        # - Parse binary data with "binary scan".
        # - Output data.
        close $handle
    }

    if {!$count} {SendTargetTheme $target undupeNone [list $pattern]}
    SendTargetTheme $target undupeFoot
    return
}

####
# Nukes
#
# Display recent nukes, command: !nukes [-limit <num>] [pattern].
#
proc ::alcoholicz::GlData::Nukes {command target user host handle channel argv} {
    # Parse command options.
    set option(limit) -1
    if {[catch {set pattern [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    set option(limit) [GetResultLimit $option(limit)]
    set pattern [join $pattern]
    SendTargetTheme $target nukesHead

    set count 0
    if {[set handle [OpenFile "nukelog"]] ne ""} {
        # TODO:
        # - Read log file.
        # - Parse binary data with "binary scan".
        # - Output data.
        close $handle
    }

    if {!$count} {SendTargetTheme $target nukesNone}
    SendTargetTheme $target nukesFoot
    return
}

####
# NukeTop
#
# Display top nuked users, command: !nukes [-limit <num>] [group].
#
proc ::alcoholicz::GlData::NukeTop {command target user host handle channel argv} {
    # Parse command options.
    set option(limit) -1
    if {[catch {set group [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    set option(limit) [GetResultLimit $option(limit)]
    set group [join $group]
    SendTargetTheme $target nuketopHead

    set count 0
    if {[set handle [OpenFile "nukelog"]] ne ""} {
        # TODO:
        # - Read log file.
        # - Parse binary data with "binary scan".
        # - Output data.
        close $handle
    }

    if {!$count} {SendTargetTheme $target nuketopNone}
    SendTargetTheme $target nuketopFoot
    return
}

####
# Unnukes
#
# Display recent unnukes, command: !unnukes [-limit <num>] [pattern].
#
proc ::alcoholicz::GlData::Unnukes {command target user host handle channel argv} {
    # Parse command options.
    set option(limit) -1
    if {[catch {set pattern [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $command $message
        return
    }
    set option(limit) [GetResultLimit $option(limit)]
    set pattern [join $pattern]
    SendTargetTheme $target unnukesHead

    set count 0
    if {[set handle [OpenFile "nukelog"]] ne ""} {
        # TODO:
        # - Read log file.
        # - Parse binary data with "binary scan".
        # - Output data.
        close $handle
    }

    if {!$count} {SendTargetTheme $target unnukesNone}
    SendTargetTheme $target unnukesFoot
    return
}

####
# OneLines
#
# Display recent one-lines, command: !onel.
#
proc ::alcoholicz::GlData::OneLines {command target user host handle channel argv} {
    variable oneLines
    SendTargetTheme $target oneLinesHead

    set count 0
    if {[set handle [OpenFile "oneliner"]] ne ""} {
        # TODO:
        # - Read log file.
        # - Parse binary data with "binary scan".
        # - Output data.
        close $handle
    }

    if {!$count} {SendTargetTheme $target oneLinesNone}
    SendTargetTheme $target oneLinesFoot
    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::GlData::Load {firstLoad} {
    variable logsPath
    variable structFormat
    variable structLength
    upvar ::alcoholicz::configHandle configHandle

    # For 32-bit little endian systems.
    array set structFormat {
        dirlog   ssisssswA255
        dupefile A256iA25
        nukelog  ssiA12A12A12ssfA60A255
        oneliner A24A24A64iA100
    }
    array set structLength {
        dirlog   288
        dupefile 288
        nukelog  376
        oneliner 216
    }

    # Check defined directory paths.
    set logsPath [file join [ConfigGet $configHandle GlFtpd dataPath] "logs"]
    if {![file isdirectory $logsPath]} {
        error "the directory \"$logsPath\" does not exist"
    }

    if {[ConfigExists $configHandle Module::GlData cmdPrefix]} {
        set prefix [ConfigGet $configHandle Module::GlData cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    # Directory commands.
    CmdCreate channel dupe   [namespace current]::Dupe \
        -category "Data"  -args "\[-limit <num>\] \[-section <name>\] <pattern>" \
        -prefix   $prefix -desc "Search for a release."

    CmdCreate channel new    [namespace current]::New \
        -category "Data"  -args "\[-limit <num>\] \[section\]" \
        -prefix   $prefix -desc "Display new releases."

    CmdCreate channel undupe [namespace current]::Undupe \
        -category "Data"  -args "\[-directory\] <pattern>" \
        -prefix   $prefix -desc "Undupe files and directories."

    # Nuke commands.
    CmdCreate channel nukes   [namespace current]::Nukes \
        -category "Data"  -args "\[-limit <num>\] \[pattern\]" \
        -prefix   $prefix -desc "Display recent nukes."

    CmdCreate channel nuketop [namespace current]::NukeTop \
        -category "Data"  -args "\[-limit <num>\] \[group\]" \
        -prefix   $prefix -desc "Display top nuked users."

    CmdCreate channel unnukes [namespace current]::Unnukes \
        -category "Data"  -args "\[-limit <num>\] \[pattern\]" \
        -prefix   $prefix -desc "Display recent unnukes."

    # Other commands.
    CmdCreate channel onel    [namespace current]::OneLines \
        -category "General" -desc "Display recent one-lines." -prefix $prefix

    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::GlData::Unload {} {
    return
}
