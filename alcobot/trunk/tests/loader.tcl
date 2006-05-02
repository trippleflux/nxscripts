#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2006 Alcoholicz Scripting Team
#
# Module Name:
#   Test Loader
#
# Author:
#   neoxed (neoxed@gmail.com) May 2, 2006
#
# Abstract:
#   Test suite loader.
#

# Load required packages.
package require Tcl 8.4
package require tcltest 2

# Setup test package.
set testPath [file dirname [file normalize [info script]]]
tcltest::singleProcess 1
tcltest::testsDirectory $testPath
tcltest::workingDirectory $testPath

# Load the AlcoExt library.
set pkgPath [file join [file dirname $testPath] "packages"]
if {![info exists ::auto_path] || [lsearch -exact $::auto_path $pkgPath] == -1} {
    # Add the "packages" directory to the search path.
    lappend ::auto_path $pkgPath
}

# Import commonly used namespace functions.
namespace import -force ::tcltest::*
