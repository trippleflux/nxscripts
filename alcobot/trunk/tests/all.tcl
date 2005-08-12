#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Run Tests
#
# Author:
#   neoxed (neoxed@gmail.com) May 15, 2005
#
# Abstract:
#   Run all test suites in the current directory.
#

package require tcltest 2
namespace import -force tcltest::*

set testPath [file dirname [file normalize [info script]]]
tcltest::workingDirectory $testPath
tcltest::testsDirectory $testPath
tcltest::runAllTests

return
