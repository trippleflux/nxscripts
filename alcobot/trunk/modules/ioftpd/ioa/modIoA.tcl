#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   ioA Module
#
# Author:
#   neoxed (neoxed@gmail.com) Dec 17, 2005
#
# Abstract:
#   Implements a module to interact with ioA's data files.
#

namespace eval ::alcoholicz::IoA {
    namespace import -force ::alcoholicz::*
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::IoA::Load {firstLoad} {
    upvar ::alcoholicz::configHandle configHandle

    # Open ioA's configuration file.
    set ioaFile [ConfigGet $configHandle Module::IoA configFile]
    if {![file isfile $ioaFile]} {
        error "the file \"$ioaFile\" does not exist"
    }
    set ioaHandle [ConfigOpen $ioaFile]
    ConfigRead $ioaHandle

    foreach {varName section key} {
        searchFile   Search  Search_Log_File
        requestsFile Request Request_File
        onelineFile  Oneline Oneline_File
        nukesFile    Nuke    Nuke_Log_File
        unnukesFile  Unnuke  UnNuke_Log_File
    } {
        set value [ConfigGet $ioaHandle $section $key]
        variable $varName [string trim $value " \t\""]
    }
    ConfigClose $ioaHandle

    # TODO: Created related commands.
    #
    # !search   ($searchFile)
    # !requests ($requestsFile)
    # !onel     ($onelineFile)
    # !nukes    ($nukesFile)
    # !unnukes  ($unnukesFile)

    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::IoA::Unload {} {
    return
}
