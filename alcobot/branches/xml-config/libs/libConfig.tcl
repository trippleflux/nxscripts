#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Config Library
#
# Author:
#   neoxed (neoxed@gmail.com) May 13, 2005
#
# Abstract:
#   Implements a configuration file library to read from and write to
#   INI-style configuration files.
#
# Exported Procedures:
#   ConfigOpen     <filePath> [-align <int>] [-comment <char>]
#   ConfigChange   <handle> <-align | -comment | -path> [value]
#   ConfigClose    <handle>
#   ConfigFree     <handle>
#   ConfigRead     <handle>
#   ConfigWrite    <handle>
#   ConfigKeys     <handle> [pattern]
#   ConfigSections <handle> <section> [pattern]
#   ConfigExists   <handle> <section> [key]
#   ConfigGet      <handle> <section> <key>
#   ConfigGetEx    <handle> <section> [pattern]
#   ConfigSet      <handle> <section> [<key> <value>]
#   ConfigUnset    <handle> <section> [key]
#

namespace eval ::config {
    variable nextHandle
    if {![info exists nextHandle]} {
        set nextHandle 0
    }
    namespace import -force ::tree::*
    namespace export ConfigOpen ConfigChange ConfigClose ConfigFree ConfigRead ConfigWrite \
        ConfigKeys ConfigSections ConfigExists ConfigGet ConfigGetEx ConfigSet ConfigUnset
}

####
# ConfigOpen
#
# Create a new configuration library handle. This handle is used by every config
# procedure and must be closed by ConfigClose. The "-align int" switch determines
# whether values will be aligned when writing the configuration file. If -align
# is greater than one, additional padding is used (n-1). The "-comment char"
# switch sets the comment character, "#" by default.
#
proc ::config::ConfigOpen {filePath args} {
    variable nextHandle
    set align 0
    set comment "#"

    foreach {option value} $args {
        switch -- $option {
            -align {
                if {![string is digit -strict $value]} {
                    error "expected digit but got \"$value\""
                }
                set align $value
            }
            -comment {
                if {[string length $value] != 1} {
                    error "invalid comment \"$value\": must be one character"
                }
                set comment $value
            }
            default {error "invalid switch \"$option\": must be -align or -comment"}
        }
    }

    set handle "config$nextHandle"
    upvar [namespace current]::$handle config
    array set config [list   \
        align   $align       \
        comment $comment     \
        tree    [TreeCreate] \
        path    $filePath    \
    ]

    incr nextHandle
    return $handle
}

####
# ConfigChange
#
# Retrieve and modify options for a given configuration handle.
#
proc ::config::ConfigChange {handle option args} {
    Acquire $handle config
    if {[lsearch -exact {-align -comment -path} $option] == -1} {
        error "invalid switch \"$option\": must be -align, -comment, or -path"
    }
    regexp -- {-(\w+)} $option result option

    set argc [llength $args]
    if {$argc == 0} {
        return $config($option)
    } elseif {$argc == 1} {
        set value [lindex $args 0]
        switch -- $option {
            align {
                if {![string is digit -strict $value]} {
                    error "expected digit but got \"$value\""
                }
                set config(align) $value
            }
            comment {
                if {[string length $value] != 1} {
                    error "invalid comment \"$value\": must be one character"
                }
                set config(comment) $value
            }
            path {set config(path) $value}
        }
        return $value
    } else {
        error "wrong # args: must be \"ConfigChange handle option ?value?\""
    }
    return
}

####
# ConfigClose
#
# Closes and invalidates the specified handle.
#
proc ::config::ConfigClose {handle} {
    Acquire $handle config
    unset -nocomplain config
    return
}

####
# ConfigFree
#
# Clears the internal tree structure, which contains all configuration data.
#
proc ::config::ConfigFree {handle} {
    Acquire $handle config
    set config(tree) [TreeCreate]
    return
}

####
# ConfigRead
#
# Reads the configuration file from disk. An error is raised if the file
# cannot be opened for reading.
#
proc ::config::ConfigRead {handle} {
    Acquire $handle config
    set fileHandle [open $config(path) r]

    # Clear the tree structure before reading new data.
    set config(tree) [TreeCreate]
    set comments [list]
    set section ""

    foreach line [split [read -nonewline $fileHandle] "\n"] {
        set line [string trim $line]

        if {[string index $line 0] eq $config(comment)} {
            lappend comments $line
        } elseif {[string match {\[*\]} $line]} {
            set section [string range $line 1 end-1]
            if {![string length $section]} {continue}

            # The section key must only be created once,
            # in case a section is defined multiple times.
            if {![TreeExists $config(tree) $section]} {
                TreeSet config(tree) $section [TreeCreate data [TreeCreate] length 0]
            }

            if {[llength $comments]} {
                set comments [concat [TreeGetNaive $config(tree) $section comments] $comments]
                TreeSet config(tree) $section comments $comments
                set comments [list]
            }
        } elseif {[set index [string first "=" $line]] != -1} {
            set key [string trimright [string range $line 0 [expr {$index - 1}]]]
            if {![string length $key] || ![string length $section]} {continue}

            # The length of the longest key is used to align all
            # values when commiting the configuration file to disk.
            if {[string length $key] > [TreeGet $config(tree) $section length]} {
                TreeSet config(tree) $section length [string length $key]
            }

            # Key comments must be placed before the line, not at the end.
            set keyTree [TreeCreate value [string trimleft [string range $line [incr index] end]]]

            if {[llength $comments]} {
                TreeSet keyTree comments $comments
                set comments [list]
            }
            TreeSet config(tree) $section data $key $keyTree
        }
    }

    close $fileHandle
    return
}

####
# ConfigWrite
#
# Writes the configuration file to disk. An error is raised if the file
# cannot be opened for writing.
#
proc ::config::ConfigWrite {handle} {
    Acquire $handle config
    set fileHandle [open $config(path) w]

    TreeFor {section sectionTree} $config(tree) {
        if {$config(align)} {
            set length [TreeGet $sectionTree length]
            incr length [expr {$config(align) - 1}]
        }

        if {[TreeExists $sectionTree comments]} {
            puts $fileHandle [join [TreeGet $sectionTree comments] "\n"]
        }
        puts $fileHandle "\[$section\]"

        TreeFor {key keyTree} [TreeGet $sectionTree data] {
            if {[TreeExists $keyTree comments]} {
                puts $fileHandle [join [TreeGet $keyTree comments] "\n"]
            }

            set value [TreeGet $keyTree value]
            if {$config(align)} {
                # Align values to the longest key name.
                puts $fileHandle [format "%-*s=%s" $length $key $value]
            } else {
                puts $fileHandle "$key=$value"
            }
        }
        puts $fileHandle ""
    }

    close $fileHandle
    return
}

####
# ConfigKeys
#
# Returns a list of all keys within a given section. If the "pattern" argument
# is specified, only matching keys are returned.
#
proc ::config::ConfigKeys {handle section {pattern "*"}} {
    Acquire $handle config
    return [TreeKeys [TreeGetNaive $config(tree) $section data] $pattern]
}

####
# ConfigSections
#
# Returns a list of all configuration sections. If the "pattern" argument is
# specified, only matching sections are returned.
#
proc ::config::ConfigSections {handle {pattern "*"}} {
    Acquire $handle config
    return [TreeKeys $config(tree) $pattern]
}

####
# ConfigExists
#
# Test for the existence of a section or a key within a given section.
#
proc ::config::ConfigExists {handle section {key ""}} {
    Acquire $handle config
    if {[string length $key]} {
        return [TreeExists $config(tree) $section data $key]
    } else {
        return [TreeExists $config(tree) $section]
    }
}

####
# ConfigGet
#
# Returns the value of the named key from the specified section.
#
proc ::config::ConfigGet {handle section key} {
    Acquire $handle config
    return [TreeGetNaive $config(tree) $section data $key value]
}

####
# ConfigGetEx
#
# Returns a list of key and value pairs from the specified section. If the
# "pattern" argument is specified, only matching keys are returned.
#
proc ::config::ConfigGetEx {handle section {pattern "*"}} {
    Acquire $handle config
    set pairList [list]
    TreeFor {key keyTree} [TreeGetNaive $config(tree) $section data] {
        if {[string match $pattern $key]} {
            lappend pairList $key [TreeGet $keyTree value]
        }
    }
    return $pairList
}

####
# ConfigSet
#
# Sets the value of the key in the specified section. If the section does not
# exist, a new one is created.
#
proc ::config::ConfigSet {handle section args} {
    Acquire $handle config
    set argc [llength $args]
    if {$argc != 0  && $argc != 2} {
        error "wrong # args: must be \"ConfigSet handle section ?key value?\""
    }

    # Initialise the section if it does not exist.
    if {![TreeExists $config(tree) $section]} {
        TreeSet config(tree) $section [TreeCreate data [TreeCreate] length 0]
    }

    # Set the key's value and update the length if necessary.
    if {$argc == 2} {
        foreach {key value} $args {break}
        if {[string length $key] > [TreeGet $config(tree) $section length]} {
            TreeSet config(tree) $section length [string length $key]
        }
        TreeSet config(tree) $section data $key value $value
        return $value
    }
    return
}

####
# ConfigUnset
#
# Removes the key or the entire section and all its keys.
#
proc ::config::ConfigUnset {handle section {key ""}} {
    Acquire $handle config
    if {[string length $key]} {
        TreeUnset config(tree) $section data $key
    } else {
        TreeUnset config(tree) $section
    }
    return
}

####
# Acquire
#
# Validate and acquire a configuration handle.
#
proc ::config::Acquire {handle handleVar} {
    if {![regexp -- {config\d+} $handle] || ![array exists [namespace current]::$handle]} {
        error "invalid config handle \"$handle\""
    }
    uplevel 1 [list upvar [namespace current]::$handle $handleVar]
}
