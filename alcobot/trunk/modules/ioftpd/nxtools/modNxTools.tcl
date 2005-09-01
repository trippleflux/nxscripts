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
#   Implements a module to interact with nxTools databases.
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
# ReadConfig
#
# Reads required options from the nxTools configuration file.
#
proc ::alcoholicz::NxTools::ReadConfig {configFile} {
    variable oneLines 5
    variable undupeChars 5

    if {![file readable $configFile]} {
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

    # Create related commands.
    if {[ConfigExists $configHandle Module::NxTools cmdPrefix]} {
        set prefix [ConfigGet $configHandle Module::NxTools cmdPrefix]
    } else {
        set prefix $::alcoholicz::cmdPrefix
    }

#    CmdCreate channel ${prefix}approved [namespace current]::Command
#    CmdCreate channel ${prefix}dupe     [namespace current]::Command
#    CmdCreate channel ${prefix}new      [namespace current]::Command
#    CmdCreate channel ${prefix}nukes    [namespace current]::Command
#    CmdCreate channel ${prefix}nuketop  [namespace current]::Command
#    CmdCreate channel ${prefix}prestats [namespace current]::Command
#    CmdCreate channel ${prefix}requests [namespace current]::Command
#    CmdCreate channel ${prefix}search   [namespace current]::Command
#    CmdCreate channel ${prefix}undupe   [namespace current]::Command
#    CmdCreate channel ${prefix}unnukes  [namespace current]::Command

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
