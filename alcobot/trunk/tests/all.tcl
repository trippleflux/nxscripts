#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2006 Alcoholicz Scripting Team
#
# Module Name:
#   All Tests
#
# Author:
#   neoxed (neoxed@gmail.com) May 2, 2006
#
# Abstract:
#   Run all tests.
#

set currentPath [file dirname [file normalize [info script]]]
source [file join $currentPath "loader.tcl"]

tcltest::runAllTests
return
