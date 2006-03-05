#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
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
# Procedures:
#   ::getopt::element <list> <element> [type]
#   ::getopt::index   <list> <element>
#   ::getopt::parse   <argList> <optList> <resultVar>
#

namespace eval ::getopt {
    variable charClasses
    if {![info exists charClasses]} {
        # Create a list of known character classes.
        catch {string is . .} charClasses
        regexp -- {must be (.+)$} $charClasses dummy charClasses
        regsub -all -- {, (or )?} $charClasses { } charClasses
    }
    namespace import -force ::alcoholicz::ListConvert
}

####
# ::getopt::element
#
# Simple wrapper around ::getopt::index, an error is thrown if no
# match is found.
#
proc ::getopt::element {list element {type "option"}} {
    set index [::getopt::index $list $element]
    if {$index == -1} {
        error "invalid $type \"$element\", must be [ListConvert $list or]"
    }
    return [lindex $list $index]
}

####
# ::getopt::index
#
# Returns the index of an element in a list. A partial match is performed
# if there is no exact match. If the element exists in the list, the index
# is returned. If there is no unique match, -1 is returned.
#
proc ::getopt::index {list element} {
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
# ::getopt::parse
#
# Parses command line options from an argument list.
#
# Option List:
#
# option             - Option with no argument (e.g. "-option").
#
# option arg         - Option with any type of argument (e.g. "-option anything").
#
# option arg {a b c} - Option with predefined value (e.g. "-option b").
#
# option class       - Option with a specific type of value, the "class" can
#                      be any character class supported by Tcl's "string is"
#                      command. Such as alnum, alpha, ascii, control, boolean,
#                      digit, double, false, graph, integer, lower, print, punct,
#                      space, true, upper, wordchar, or xdigit
#
# Example:
#
# set argList "-limit 5 -match glob *some pattern*"
# set optList {{limit integer} {match arg {exact glob regexp}}}
# set pattern [::getopt::parse $argList $optList result]
#
# $result(limit) = 5
# $result(match) = glob
# $pattern       = *some pattern*"
#
proc ::getopt::parse {argList optList resultVar} {
    variable charClasses
    upvar $resultVar result

    if {[info exists result] && ![array exists result]} {
        error "the variable \"$resultVar\" is not an array"
    }

    # Create a list of option names.
    set optNames [list]
    set types [lsort [concat $charClasses arg]]

    foreach option $optList {
        switch -- [llength $option] {
            1 {}
            2 {
                if {[lsearch -exact $types [lindex $option 1]] == -1} {
                    error "invalid option definition \"$option\": \
                        bad type \"[lindex $option 1]\", must be [ListConvert $types or]"
                }
            }
            3 {
                if {[lindex $option 1] ne "arg"} {
                    error "invalid option definition \"$option\": \
                        value lists can only be used with the \"arg\" type"
                }
            }
            default {
                error "invalid option definition \"$option\": wrong number of arguments"
            }
        }
        lappend optNames [lindex $option 0]
    }

    #
    # Errors thrown before this point indicate implementation problems, errors
    # thrown after this point indicate user errors. For example, the argument
    # list may contain an option that is invalid or undefined.
    #

    while {[set argCount [llength $argList]]} {
        set arg [lindex $argList 0]

        # The "--" argument indicates the end of options.
        if {$arg eq "--"} {
            set argList [lrange $argList 1 end]
            break
        }

        if {[string index $arg 0] eq "-"} {
            set index [::getopt::index $optNames [string range $arg 1 end]]
            if {$index == -1} {
                error "invalid option \"$arg\""
            }

            set optCount [llength [lindex $optList $index]]
            foreach {optName optType optValues} [lindex $optList $index] {break}

            if {$optCount > 1} {
                if {$argCount < 2} {
                    error "the option \"$arg\" requires a value"
                }
                set value [lindex $argList 1]

                if {$optType eq "arg"} {
                    if {$optCount > 2} {
                        set value [::getopt::element $optValues $value "value"]
                    }
                } elseif {![string is $optType -strict $value]} {
                    error "the option \"$arg\" requires a $optType type value"
                }

                set result($optName) $value
                set argList [lrange $argList 2 end]
            } else {
                # Since this option does not require an argument, an empty
                # string is used to notify the caller that it was specified.
                set result($optName) {}
                set argList [lrange $argList 1 end]
            }
        } else {
            break
        }
    }

    return $argList
}
