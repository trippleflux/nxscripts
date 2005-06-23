#
# AlcoExt - Alcoholicz Tcl extension.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# File Name:
#   all.tcl
#
# Author:
#   neoxed (neoxed@gmail.com) May 25, 2005
#
# Abstract:
#   Run all test suites.
#

package require tcltest 2
namespace import -force tcltest::*

if {[info exists tcl_platform(debug)] || [llength [info commands memory]]} {
    tcltest::singleProcess 1
}

set testPath [file dirname [info script]]
tcltest::workingDirectory $testPath
tcltest::testsDirectory $testPath
tcltest::runAllTests

return
