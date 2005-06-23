#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# File Name:
#   all.tcl
#
# Author:
#   neoxed (neoxed@gmail.com) May 15, 2005
#
# Abstract:
#   Run all test suites.
#

package require tcltest 2
namespace import -force tcltest::*

set testPath [file dirname [info script]]
tcltest::workingDirectory $testPath
tcltest::testsDirectory $testPath
tcltest::runAllTests

return
