#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# File Name:
#   sample.tcl
#
# Author:
#   neoxed (neoxed@gmail.com) April 16, 2005
#
# Abstract:
#   An example module to give developers a starting point.
#

namespace eval ::alcoholicz::Sample {
    namespace import -force ::alcoholicz::*
}

####
# Load
#
# Module initialization procedure, called when the module is loaded.
#
proc ::alcoholicz::Sample::Load {firstLoad} {
    return
}

####
# Unload
#
# Module clean-up procedure, called before the module is unloaded.
#
proc ::alcoholicz::Sample::Unload {} {
    return
}
