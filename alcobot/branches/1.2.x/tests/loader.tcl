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
::tcltest::singleProcess 1
::tcltest::testsDirectory $testPath
::tcltest::workingDirectory $testPath

# Add the "packages" directory to the search path.
global auto_path
set pkgPath [file join [file dirname $testPath] "packages"]
if {![info exists auto_path] || [lsearch -exact $auto_path $pkgPath] == -1} {
    lappend auto_path $pkgPath
}

# Detect if we're running from an Eggdrop Tcl interpreter.
proc IsEggdrop {} {
    foreach var {botnick botname config handlen server serveraddress version} {
        if {![info exists ::$var]} {return 0}
    }
    set cmds [info commands]
    foreach cmd {bind binds channel die putlog timer utimer unbind} {
        if {[lsearch -exact $cmds $cmd] == -1} {return 0}
    }
    return 1
}
::tcltest::testConstraint eggdrop [IsEggdrop]

# Import commonly used namespace functions.
namespace import -force ::tcltest::*
