#
# AlcoExt - Alcoholicz Tcl extension.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   loader.tcl
#
# Author:
#   neoxed (neoxed@gmail.com) Feb 14, 2005
#
# Abstract:
#   Test suite loader.
#

# Load required packages.
package require Tcl 8.4
package require tcltest 2

# Load the AlcoExt library.
if {$tcl_platform(platform) eq "windows"} {
    set libFile "AlcoExt*"
} else {
    set libFile "libAlcoExt*"
}
append libFile [info sharedlibextension]

set parentPath [file dirname [file dirname [file normalize [info script]]]]
set libFile [lindex [glob -dir $parentPath -types f $libFile] 0]
load $libFile

# Import commonly used namespace functions.
namespace import -force ::alcoholicz::*
namespace import -force ::tcltest::*
