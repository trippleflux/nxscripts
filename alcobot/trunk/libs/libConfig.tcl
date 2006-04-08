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
# Procedures:
#   Config::Open     <filePath> [-align <int>] [-comment <char>]
#   Config::Change   <handle> <-align | -comment | -path> [value]
#   Config::Close    <handle>
#   Config::Free     <handle>
#   Config::Read     <handle>
#   Config::Write    <handle>
#   Config::Keys     <handle> [pattern]
#   Config::Sections <handle> <section> [pattern]
#   Config::Exists   <handle> <section> [key]
#   Config::Get      <handle> <section> <key>
#   Config::GetEx    <handle> <section> [pattern]
#   Config::GetMulti <handle> <section> <key> ...
#   Config::Set      <handle> <section> [<key> <value>]
#   Config::Unset    <handle> <section> [key]
#

namespace eval ::Config {
    variable nextHandle
    if {![info exists nextHandle]} {
        set nextHandle 0
    }
}

####
# Config::Open
#
# Create a new configuration library handle. This handle is used by every config
# procedure and must be closed by ::Config::Close. The "-align int" switch determines
# whether values will be aligned when writing the configuration file. If -align
# is greater than one, additional padding is used (n-1). The "-comment char"
# switch sets the comment character, "#" by default.
#
proc ::Config::Open {filePath args} {
    variable nextHandle
    set align 0
    set comment "#"

    foreach {option value} $args {
        switch -- $option {
            -align {
                if {![string is digit -strict $value]} {
                    throw CONFIG "expected digit but got \"$value\""
                }
                set align $value
            }
            -comment {
                if {[string length $value] != 1} {
                    throw CONFIG "invalid comment \"$value\": must be one character"
                }
                set comment $value
            }
            default {throw CONFIG "invalid switch \"$option\": must be -align or -comment"}
        }
    }

    set handle "config$nextHandle"
    upvar [namespace current]::$handle config
    array set config [list     \
        align   $align         \
        comment $comment       \
        tree    [Tree::Create] \
        path    $filePath      \
    ]

    incr nextHandle
    return $handle
}

####
# Config::Change
#
# Retrieve and modify options for a given configuration handle.
#
proc ::Config::Change {handle option args} {
    Acquire $handle config
    if {[lsearch -exact {-align -comment -path} $option] == -1} {
        throw CONFIG "invalid switch \"$option\": must be -align, -comment, or -path"
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
                    throw CONFIG "expected digit but got \"$value\""
                }
                set config(align) $value
            }
            comment {
                if {[string length $value] != 1} {
                    throw CONFIG "invalid comment \"$value\": must be one character"
                }
                set config(comment) $value
            }
            path {set config(path) $value}
        }
        return $value
    } else {
        throw CONFIG "wrong # args: must be \"Config::Change handle option ?value?\""
    }
    return
}

####
# Config::Close
#
# Closes and invalidates the specified handle.
#
proc ::Config::Close {handle} {
    Acquire $handle config
    unset -nocomplain config
    return
}

####
# Config::Free
#
# Clears the internal tree structure, which contains all configuration data.
#
proc ::Config::Free {handle} {
    Acquire $handle config
    set config(tree) [Tree::Create]
    return
}

####
# Config::Read
#
# Reads the configuration file from disk. An error is raised if the file
# cannot be opened for reading.
#
proc ::Config::Read {handle} {
    Acquire $handle config
    set fileHandle [open $config(path) r]

    # Clear the tree structure before reading new data.
    set config(tree) [Tree::Create]
    set comments [list]
    set section ""

    foreach line [split [read $fileHandle] "\n"] {
        set line [string trim $line]

        if {[string index $line 0] eq $config(comment)} {
            lappend comments $line
        } elseif {[string match {\[*\]} $line]} {
            set section [string range $line 1 end-1]
            if {$section eq ""} {continue}

            # The section key must only be created once,
            # in case a section is defined multiple times.
            if {![Tree::Exists $config(tree) $section]} {
                Tree::Set config(tree) $section [Tree::Create data [Tree::Create] length 0]
            }

            if {[llength $comments]} {
                set comments [concat [Tree::GetNaive $config(tree) $section comments] $comments]
                Tree::Set config(tree) $section comments $comments
                set comments [list]
            }
        } elseif {[set index [string first "=" $line]] != -1} {
            set key [string trimright [string range $line 0 [expr {$index - 1}]]]
            if {$key eq "" || $section eq ""} {continue}

            # The length of the longest key is used to align all
            # values when commiting the configuration file to disk.
            if {[string length $key] > [Tree::Get $config(tree) $section length]} {
                Tree::Set config(tree) $section length [string length $key]
            }

            # Key comments must be placed before the line, not at the end.
            set keyTree [Tree::Create value [string trimleft [string range $line [incr index] end]]]

            if {[llength $comments]} {
                Tree::Set keyTree comments $comments
                set comments [list]
            }
            Tree::Set config(tree) $section data $key $keyTree
        }
    }

    close $fileHandle
    return
}

####
# Config::Write
#
# Writes the configuration file to disk. An error is raised if the file
# cannot be opened for writing.
#
proc ::Config::Write {handle} {
    Acquire $handle config
    set fileHandle [open $config(path) w]

    Tree::For {section sectionTree} $config(tree) {
        if {$config(align)} {
            set length [Tree::Get $sectionTree length]
            incr length [expr {$config(align) - 1}]
        }

        if {[Tree::Exists $sectionTree comments]} {
            puts $fileHandle [join [Tree::Get $sectionTree comments] "\n"]
        }
        puts $fileHandle "\[$section\]"

        Tree::For {key keyTree} [Tree::Get $sectionTree data] {
            if {[Tree::Exists $keyTree comments]} {
                puts $fileHandle [join [Tree::Get $keyTree comments] "\n"]
            }

            set value [Tree::Get $keyTree value]
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
# Config::Keys
#
# Returns a list of all keys within a given section. If the "pattern" argument
# is specified, only matching keys are returned.
#
proc ::Config::Keys {handle section {pattern "*"}} {
    Acquire $handle config
    return [Tree::Keys [Tree::GetNaive $config(tree) $section data] $pattern]
}

####
# Config::Sections
#
# Returns a list of all configuration sections. If the "pattern" argument is
# specified, only matching sections are returned.
#
proc ::Config::Sections {handle {pattern "*"}} {
    Acquire $handle config
    return [Tree::Keys $config(tree) $pattern]
}

####
# Config::Exists
#
# Test for the existence of a section or a key within a given section.
#
proc ::Config::Exists {handle section {key ""}} {
    Acquire $handle config
    if {[string length $key]} {
        return [Tree::Exists $config(tree) $section data $key]
    } else {
        return [Tree::Exists $config(tree) $section]
    }
}

####
# Config::Get
#
# Returns the value of the named key from the specified section.
#
proc ::Config::Get {handle section key} {
    Acquire $handle config
    return [Tree::GetNaive $config(tree) $section data $key value]
}

####
# Config::GetEx
#
# Returns a list of key and value pairs from the specified section. If the
# "pattern" argument is specified, only matching keys are returned.
#
proc ::Config::GetEx {handle section {pattern "*"}} {
    Acquire $handle config
    set result [list]
    Tree::For {key keyTree} [Tree::GetNaive $config(tree) $section data] {
        if {[string match $pattern $key]} {
            lappend result $key [Tree::Get $keyTree value]
        }
    }
    return $result
}

####
# Config::GetMulti
#
# Returns a list of key and value pairs from the specified section.
#
proc ::Config::GetMulti {handle section args} {
    Acquire $handle config
    set result [list]
    set keyTree [Tree::GetNaive $config(tree) $section data]
    foreach key $args {
        lappend result $key [Tree::GetNaive $keyTree $key value]
    }
    return $result
}

####
# Config::Set
#
# Sets the value of the key in the specified section. If the section does not
# exist, a new one is created.
#
proc ::Config::Set {handle section args} {
    Acquire $handle config
    set argc [llength $args]
    if {$argc != 0  && $argc != 2} {
        throw CONFIG "wrong # args: must be \"Config::Set handle section ?key value?\""
    }

    # Initialise the section if it does not exist.
    if {![Tree::Exists $config(tree) $section]} {
        Tree::Set config(tree) $section [Tree::Create data [Tree::Create] length 0]
    }

    # Set the key's value and update the length if necessary.
    if {$argc == 2} {
        foreach {key value} $args {break}
        if {[string length $key] > [Tree::Get $config(tree) $section length]} {
            Tree::Set config(tree) $section length [string length $key]
        }
        Tree::Set config(tree) $section data $key value $value
        return $value
    }
    return
}

####
# Config::Unset
#
# Removes the key or the entire section and all its keys.
#
proc ::Config::Unset {handle section {key ""}} {
    Acquire $handle config
    if {$key eq ""} {
        Tree::Unset config(tree) $section
    } else {
        Tree::Unset config(tree) $section data $key
    }
    return
}

####
# Acquire
#
# Validate and acquire a configuration handle.
#
proc ::Config::Acquire {handle handleVar} {
    if {![regexp -- {config\d+} $handle] || ![array exists [namespace current]::$handle]} {
        throw CONFIG "invalid config handle \"$handle\""
    }
    uplevel 1 [list upvar [namespace current]::$handle $handleVar]
}
