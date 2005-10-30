#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Filter Module
#
# Author:
#   neoxed (neoxed@gmail.com) Oct 29, 2005
#
# Abstract:
#   Implements a module to filter out events, announces, and commands
#   based on user-defined logic.
#

namespace eval ::alcoholicz::Filter {
    namespace import -force ::alcoholicz::*
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::Filter::Load {firstLoad} {
    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Filter::Unload {} {
    return
}
