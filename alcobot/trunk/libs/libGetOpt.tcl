#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Option Parsing Library
#
# Author:
#   neoxed (neoxed@gmail.com) Sep 8, 2005
#
# Abstract:
#   Implements a command-line option parser.
#

namespace eval ::alcoholicz {
    if {![info exists charClasses]} {
        # Create a list of known character classes.
        variable charClasses
        catch {string is . .} charClasses
        regexp -- {must be (.+)$} $charClasses dummy charClasses
        regsub -all -- {, (or )?} $charClasses { } charClasses
    }
    namespace export GetIndexFromList GetOptions
}

####
# GetIndexFromList
#
# Returns the index of an element in a list. A partial match is performed
# if there is no exact match. If the element exists in the list, the index
# is returned. If there is no unique match, -1 is returned.
#
proc ::alcoholicz::GetIndexFromList {list element} {
    # Check for an exact match.
    set index [lsearch -exact $list $element]
    if {$index == -1} {
        # Check for a partial match (e.g. -foo will match -foobar).
        set element [string map {* \\* ? \\? \\ \\\\ \[ \\\[ \] \\\]} $element]
        append element "*"
        set index [lsearch -all -glob $list $element]

        # If there is more than one partial match, a more specific
        # list element must be provided for a unique match.
        if {[llength $index] != 1} {
            return -1
        }
    }
    return $index
}

####
# GetOptions
#
# TODO
#
proc ::alcoholicz::GetOptions {argList optList} {

    set optNames [list help ?]
    foreach option $optList {


        if {[regsub -- {\.multi$} $option {} option]} {
            set multi
        # Remove this extension before passing to typedGetopt.

        regsub -- {\..*$} $name {} temp
        set multi($temp) 1
        }

    }
}

#
# Definitions:
#
# option         - Switch with no argument (e.g. "-foo").
# option.arg     - Switch with any type of argument (e.g. "-foo bar").
# option.integer - Switch with a specific class of argument (e.g. "-foo 5").
# option.(exact|glob|regexp) - Switch with a argument in the list (e.g. "-foo exact").
#
# Quantifiers:
#
# ? - Single optional argument.
# * - List of arguments, terminated by "--".
# + - Optional list of arguments, terminated by "--".
#
# Defaults:
# -help and -?
#
# [-limit <#>] [-match exact|glob|regexp]
# limit.integer match.(exact|glob|regexp)
#
