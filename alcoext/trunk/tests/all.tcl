#
# AlcoExt - Alcoholicz Tcl extension.
# Copyright (c) 2005-2008 Alcoholicz Scripting Team
#
# Module Name:
#   All Tests
#
# Author:
#   neoxed (neoxed@gmail.com) May 25, 2005
#
# Abstract:
#   Run all test suites.
#

set currentPath [file dirname [file normalize [info script]]]
source [file join $currentPath "loader.tcl"]

tcltest::runAllTests
return
