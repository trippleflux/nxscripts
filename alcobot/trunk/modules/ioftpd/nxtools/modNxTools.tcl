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

    db close
    return
}

####
# Latest
#
# Display recent releases .
#
proc ::alcoholicz::NxTools::Latest {user host handle channel target argc argv} {
    if {![DbOpenFile "DupeDirs.db"]} {return}

    db close
    return
}

####
# Search
#
# Search for a release.
#
proc ::alcoholicz::NxTools::Search {user host handle channel target argc argv} {
    if {![DbOpenFile "DupeDirs.db"]} {return}

    db close
    return
}

####
# Nukes
#
# Display recent nukes.
#
proc ::alcoholicz::NxTools::Nukes {user host handle channel target argc argv} {
    if {![DbOpenFile "Nukes.db"]} {return}

    db close
    return
}

####
# NukeTop
#
# Display top nuked users.
#
proc ::alcoholicz::NxTools::NukeTop {user host handle channel target argc argv} {
    if {![DbOpenFile "Nukes.db"]} {return}

    db close
    return
}

####
# Unnukes
#
# Display recent unnukes.
#
proc ::alcoholicz::NxTools::Unnukes {user host handle channel target argc argv} {
    if {![DbOpenFile "Nukes.db"]} {return}

    db close
    return
}

####
# OneLines
#
# Display recent one-lines.
#
proc ::alcoholicz::NxTools::OneLines {user host handle channel target argc argv} {
    if {![DbOpenFile "OneLines.db"]} {return}

    db close
    return
}

####
# PreStats
#
# Display group pre statistics.
#
proc ::alcoholicz::NxTools::PreStats {user host handle channel target argc argv} {
    if {![DbOpenFile "Pres.db"]} {return}

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

    db close
    return
}

####
# Undupe
#
# Remove a file or directory from the dupe database.
#
proc ::alcoholicz::NxTools::Undupe {user host handle channel target argc argv} {
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

    CmdCreate channel ${prefix}news     [namespace current]::OneLines
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
