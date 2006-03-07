#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Sample Module
#
# Author:
#   neoxed (neoxed@gmail.com) Apr 16, 2005
#
# Abstract:
#   Implements a example module to give developers a starting point.
#

namespace eval ::Bot::Sample {
    namespace import -force ::Bot::*
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Sample::Load {firstLoad} {
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Sample::Unload {} {
}
