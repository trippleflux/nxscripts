#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   nxTools Module
#
# Author:
#   neoxed (neoxed@gmail.com) Aug 30, 2005
#
# Abstract:
#   Implements a module to interact with nxTools' databases.
#

namespace eval ::alcoholicz::NxTools {
    if {![info exists dataPath]} {
        variable dataPath ""
        variable defResults 5
        variable maxResults 15
        variable oneLines 5
        variable undupeChars 5
    }
    namespace import -force ::alcoholicz::*
}

####
# DbOpenFile
#
# Open a nxTools SQLite database file.
#
proc ::alcoholicz::NxTools::DbOpenFile {fileName} {
    variable dataPath

    set filePath [file join $dataPath $fileName]
    if {![file isfile $filePath]} {
        LogError ModNxTools "Unable to open \"$filePath\": the file does not exist"
        return 0
    }
    if {[catch {sqlite3 [namespace current]::db $filePath} error]} {
        LogError ModNxTools "Unable to open \"$filePath\": $error"
        return 0
    }

    db busy [namespace current]::DbBusyHandler
    return 1
}

####
# DbBusyHandler
#
# Callback invoked by SQLite if it tries to open a locked database.
#
proc ::alcoholicz::NxTools::DbBusyHandler {tries} {
    # Give up after 50 attempts, although it should succeed after 1-5.
    if {$tries > 50} {return 1}
    after 200
    return 0
}

####
# Approved
#
# Display approved releases.
#
proc ::alcoholicz::NxTools::Approved {user host handle channel target argc argv} {
    if {![DbOpenFile "Approves.db"]} {return}
    SendTargetTheme $target approveHead

    set count 0
    db eval {SELECT * FROM Approves ORDER BY Release ASC} values {
        incr count
        set age [expr {[clock seconds] - $values(TimeStamp)}]
        set items [list $values(UserName) $values(GroupName) $values(Release) $age $count]
        SendTargetTheme $target approveBody $items
    }
    db close

    if {!$count} {SendTargetTheme $target approveNone}
    SendTargetTheme $target approveFoot
    return
}

####
# Latest
#
# Display recent releases .
#
proc ::alcoholicz::NxTools::Latest {user host handle channel target argc argv} {
    variable defResults
    variable maxResults

    # Parse command options.
    set option(limit) $defResults
    if {[catch {set section [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $::lastbind $message
        return
    }
    if {$option(limit) < 0 || $option(limit) > $maxResults} {
        set option(limit) $maxResults
    }

    if {![DbOpenFile "DupeDirs.db"]} {return}

    # TODO

    db close
    return
}

####
# Search
#
# Search for a release.
#
proc ::alcoholicz::NxTools::Search {user host handle channel target argc argv} {
    variable defResults
    variable maxResults

    # Parse command options.
    set option(limit) $defResults
    set option(section) ""
    if {[catch {set pattern [GetOptions $argv {{limit integer} {section arg}} option]} message]} {
        CmdSendHelp $channel channel $::lastbind $message
        return
    } elseif {$pattern eq ""} {
        CmdSendHelp $channel channel $::lastbind $message "you must specify a pattern"
        return
    }
    if {$option(limit) < 0 || $option(limit) > $maxResults} {
        set option(limit) $maxResults
    }

    if {![DbOpenFile "DupeDirs.db"]} {return}

    # TODO

    db close
    return
}

####
# Nukes
#
# Display recent nukes.
#
proc ::alcoholicz::NxTools::Nukes {user host handle channel target argc argv} {
    variable defResults
    variable maxResults

    # Parse command options.
    set option(limit) $defResults
    if {[catch {set pattern [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $::lastbind $message
        return
    }
    if {$option(limit) < 0 || $option(limit) > $maxResults} {
        set option(limit) $maxResults
    }

    if {![DbOpenFile "Nukes.db"]} {return}

    # TODO

    db close
    return
}

####
# NukeTop
#
# Display top nuked users.
#
proc ::alcoholicz::NxTools::NukeTop {user host handle channel target argc argv} {
    variable defResults
    variable maxResults

    # Parse command options.
    set option(limit) $defResults
    if {[catch {set group [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $::lastbind $message
        return
    }
    if {$option(limit) < 0 || $option(limit) > $maxResults} {
        set option(limit) $maxResults
    }

    if {![DbOpenFile "Nukes.db"]} {return}

    # TODO

    db close
    return
}

####
# Unnukes
#
# Display recent unnukes.
#
proc ::alcoholicz::NxTools::Unnukes {user host handle channel target argc argv} {
    variable defResults
    variable maxResults

    # Parse command options.
    set option(limit) $defResults
    if {[catch {set pattern [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $::lastbind $message
        return
    }
    if {$option(limit) < 0 || $option(limit) > $maxResults} {
        set option(limit) $maxResults
    }

    if {![DbOpenFile "Nukes.db"]} {return}

    # TODO

    db close
    return
}

####
# OneLines
#
# Display recent one-lines.
#
proc ::alcoholicz::NxTools::OneLines {user host handle channel target argc argv} {
    variable oneLines
    if {![DbOpenFile "OneLines.db"]} {return}
    SendTargetTheme $target oneLinesHead

    set count 0
    db eval {SELECT * FROM OneLines ORDER BY TimeStamp DESC LIMIT $oneLines} values {
        incr count
        set age [expr {[clock seconds] - $values(TimeStamp)}]
        set items [list $values(UserName) $values(GroupName) $values(Message) $values(TimeStamp) $age $count]
        SendTargetTheme $target oneLinesBody $items
    }
    db close

    if {!$count} {SendTargetTheme $target oneLinesNone}
    SendTargetTheme $target oneLinesFoot
    return
}

####
# PreStats
#
# Display group pre statistics.
#
proc ::alcoholicz::NxTools::PreStats {user host handle channel target argc argv} {
    variable defResults
    variable maxResults

    # Parse command options.
    set option(limit) $defResults
    if {[catch {set group [GetOptions $argv {{limit integer}} option]} message]} {
        CmdSendHelp $channel channel $::lastbind $message
        return
    }
    if {$option(limit) < 0 || $option(limit) > $maxResults} {
        set option(limit) $maxResults
    }

    if {![DbOpenFile "Pres.db"]} {return}

    # TODO

    db close
    return
}

####
# Requests
#
# Display current requests.
#
proc ::alcoholicz::NxTools::Requests {user host handle channel target argc argv} {
    if {![DbOpenFile "Requests.db"]} {return}
    SendTargetTheme $target requestsHead

    set count 0
    db eval {SELECT * FROM Requests WHERE Status=0 ORDER BY RequestId DESC} values {
        incr count
        set age [expr {[clock seconds] - $values(TimeStamp)}]
        set items [list $values(UserName) $values(GroupName) $values(Request) $values(RequestId) $age $count]
        SendTargetTheme $target requestsBody $items
    }
    db close

    if {!$count} {SendTargetTheme $target requestsNone}
    SendTargetTheme $target requestsFoot
    return
}

####
# Undupe
#
# Remove a file or directory from the dupe database.
#
proc ::alcoholicz::NxTools::Undupe {user host handle channel target argc argv} {
    variable undupeChars

    # Parse command options.
    if {[catch {set pattern [GetOptions $argv {directory} option]} message]} {
        CmdSendHelp $channel channel $::lastbind $message
        return
    } elseif {[regexp {[\*\?]} $pattern] && [regexp -all {[[:alnum:]]} $pattern] < $undupeChars} {
        CmdSendHelp $channel channel $::lastbind $message "you must specify at least $undupeChars alphanumeric chars"
        return
    }

    if {[info exists option(directory)]} {
        set colName "DirName"
        set tableName "DupeDirs"
    } else {
        set colName "FileName"
        set tableName "DupeFiles"
    }

    if {![DbOpenFile "${tableName}.db"]} {return}

    # TODO

    return
}

####
# ReadConfig
#
# Reads required options from the nxTools configuration file.
#
proc ::alcoholicz::NxTools::ReadConfig {configFile} {
    variable oneLines 5
    variable undupeChars 5

    if {![file isfile $configFile]} {
        error "the file \"$configFile\" does not exist"
    }

    # Evaluate the source file in a slave interpreter
    # to retrieve the required configuration options.
    set slave [interp create -safe]
    interp invokehidden $slave source $configFile

    foreach varName {oneLines undupeChars} option {misc(OneLines) dupe(AlphaNumChars)} {
        if {[$slave eval info exists $option]} {
            set $varName [$slave eval set $option]
        } else {
            LogWarning ModNxTools "The option \"$option\" is not defined in \"$configFile\"."
        }
    }

    interp delete $slave
    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::NxTools::Load {firstLoad} {
    variable dataPath
    variable defResults
    variable maxResults
    upvar ::alcoholicz::configHandle configHandle

    if {$firstLoad} {
        package require sqlite3
    }

    foreach option {configFile dataPath defResults maxResults} {
        set $option [ConfigGet $configHandle Module::NxTools $option]
    }

    # Check defined file and directory paths.
    if {[catch {ReadConfig $configFile} message]} {
        LogError ModNxTools "Unable to read nxTools configuration: $message"
    }
    if {![file isdirectory $dataPath]} {
        LogError ModNxTools "The database directory \"$dataPath\" does not exist."
    }

    if {[ConfigExists $configHandle Module::NxTools cmdPrefix]} {
        set prefix [ConfigGet $configHandle Module::NxTools cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

    # Directory commands.
    CmdCreate channel ${prefix}latest   [namespace current]::Latest
    CmdCreate channel ${prefix}new      [namespace current]::Latest \
        Stats "Display new releases." "\[-limit <num>\] \[section\]"

    CmdCreate channel ${prefix}dupe     [namespace current]::Search
    CmdCreate channel ${prefix}search   [namespace current]::Search \
        Stats "Search for a release." "\[-limit <num>\] \[-section <name>\] <pattern>"

    CmdCreate channel ${prefix}undupe   [namespace current]::Undupe \
        Stats "Undupe files and directories." "\[-directory\] <pattern>"

    # Nuke commands.
    CmdCreate channel ${prefix}nukes    [namespace current]::Nukes \
        Stats "Display recent nukes." "\[-limit <num>\] \[pattern\]"

    CmdCreate channel ${prefix}nuketop  [namespace current]::NukeTop \
        Stats "Display top nuked users." "\[-limit <num>\] \[group\]"

    CmdCreate channel ${prefix}unnukes  [namespace current]::Unnukes \
        Stats "Display recent unnukes." "\[-limit <num>\] \[pattern\]"

    # Other commands.
    CmdCreate channel ${prefix}approved [namespace current]::Approved \
        General "Display approved releases."

    CmdCreate channel ${prefix}onelines [namespace current]::OneLines
    CmdCreate channel ${prefix}onel     [namespace current]::OneLines \
        General "Display recent one-lines."

    CmdCreate channel ${prefix}prestats [namespace current]::PreStats \
        Stats "Display pre statistics." "\[-limit <num>\] \[group\]"

    CmdCreate channel ${prefix}reqs     [namespace current]::Requests
    CmdCreate channel ${prefix}requests [namespace current]::Requests \
        General "Display current requests."

    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::NxTools::Unload {} {
    return
}
