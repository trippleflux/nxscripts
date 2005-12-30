#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
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

namespace eval ::alcoholicz::Sample {
    namespace import -force ::alcoholicz::*
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::Sample::Load {firstLoad} {
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::Sample::Unload {} {
}
