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
#   Config::Open     <path> [options ...]
#   Config::Change   <handle> [options ...]
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

package require alco::getopt 1.2
package require alco::tree 1.2
package require alco::util 1.2

namespace eval ::Config {
    variable nextHandle
    if {![info exists nextHandle]} {
        set nextHandle 0
    }
}

####
# Config::Open
#
# Create a new configuration file handle. This handle is used by every
# library procedure and must be closed using Config::Close.
#
# Options:
#  -align   <num>  - Aligns values when writing the configuration file.
#  -comment <char> - Sets the comment character, "#" is used by default.
#
proc ::Config::Open {path args} {
    variable nextHandle

    # Parse arguments.
    array set option [list align 0 comment "#"]
    GetOpt::Parse $args {{align integer} {comment arg}} option
    if {[string length $option(comment)] != 1} {
        error "invalid comment \"$option(comment)\": must be one character"
    }
    set handle "config$nextHandle"
    upvar ::Config::$handle config

    #
    # Config Handle Contents
    #
    # ftp(align)   - Key alignment.
    # ftp(comment) - Comment character.
    # ftp(tree)    - Config data tree.
    # ftp(path)    - Path to the config file.
    #
    array set config [list       \
        align   $option(align)   \
        comment $option(comment) \
        tree    [Tree::Create]   \
        path    $path            \
    ]

    incr nextHandle
    return $handle
}

####
# Config::Change
#
# Retrieve and modify options for a given configuration handle.
#
proc ::Config::Change {handle args} {
    Acquire $handle config
    set options {-align -comment -path}

    # Retrieve all options.
    if {[llength $args] == 0} {
        set result [list]
        foreach option $options {
            lappend result $option $config([string range $option 1 end])
        }
        return $result
    }

    # Retrieve only the specified option.
    if {[llength $args] == 1} {
        set option [GetOpt::Element $options [lindex $args 0]]
        return $config([string range $option 1 end])
    }

    # Modify options.
    GetOpt::Parse $args {{align integer} {comment arg} {path arg}} option
    if {[info exists option(comment)] && [string length $option(comment)] != 1} {
        error "invalid comment \"$option(comment)\": must be one character"
    }
    array set config [array get option]
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
                set comments [concat [Tree::GetQuiet $config(tree) $section comments] $comments]
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
    return [Tree::Keys [Tree::GetQuiet $config(tree) $section data] $pattern]
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
    return [Tree::GetQuiet $config(tree) $section data $key value]
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
    Tree::For {key keyTree} [Tree::GetQuiet $config(tree) $section data] {
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
    set keyTree [Tree::GetQuiet $config(tree) $section data]
    foreach key $args {
        lappend result $key [Tree::GetQuiet $keyTree $key value]
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
        error "wrong # args: must be \"Config::Set handle section ?key value?\""
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

################################################################################
# Internal Procedures                                                          #
################################################################################

####
# Acquire
#
# Validate and acquire a configuration handle.
#
proc ::Config::Acquire {handle handleVar} {
    if {![regexp -- {^config\d+$} $handle] || ![array exists ::Config::$handle]} {
        error "invalid config handle \"$handle\""
    }
    uplevel 1 [list upvar ::Config::$handle $handleVar]
}

package provide alco::config 1.2.0
