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
#   ::config::open     <filePath> [-align <int>] [-comment <char>]
#   ::config::change   <handle> <-align | -comment | -path> [value]
#   ::config::close    <handle>
#   ::config::free     <handle>
#   ::config::read     <handle>
#   ::config::write    <handle>
#   ::config::keys     <handle> [pattern]
#   ::config::sections <handle> <section> [pattern]
#   ::config::exists   <handle> <section> [key]
#   ::config::get      <handle> <section> <key>
#   ::config::getex    <handle> <section> [pattern]
#   ::config::set      <handle> <section> [<key> <value>]
#   ::config::unset    <handle> <section> [key]
#

namespace eval ::config {
    variable nextHandle
    if {![info exists nextHandle]} {
        ::set nextHandle 0
    }
}

####
# ::config::open
#
# Create a new configuration library handle. This handle is used by every config
# procedure and must be closed by ::config::close. The "-align int" switch determines
# whether values will be aligned when writing the configuration file. If -align
# is greater than one, additional padding is used (n-1). The "-comment char"
# switch sets the comment character, "#" by default.
#
proc ::config::open {filePath args} {
    variable nextHandle
    ::set align 0
    ::set comment "#"

    foreach {option value} $args {
        switch -- $option {
            -align {
                if {![string is digit -strict $value]} {
                    error "expected digit but got \"$value\""
                }
                ::set align $value
            }
            -comment {
                if {[string length $value] != 1} {
                    error "invalid comment \"$value\": must be one character"
                }
                ::set comment $value
            }
            default {error "invalid switch \"$option\": must be -align or -comment"}
        }
    }

    ::set handle "config$nextHandle"
    upvar [namespace current]::$handle config
    array set config [list   \
        align   $align       \
        comment $comment     \
        tree    [::tree::create] \
        path    $filePath    \
    ]

    incr nextHandle
    return $handle
}

####
# ::config::change
#
# Retrieve and modify options for a given configuration handle.
#
proc ::config::change {handle option args} {
    Acquire $handle config
    if {[lsearch -exact {-align -comment -path} $option] == -1} {
        error "invalid switch \"$option\": must be -align, -comment, or -path"
    }
    regexp -- {-(\w+)} $option result option

    ::set argc [llength $args]
    if {$argc == 0} {
        return $config($option)
    } elseif {$argc == 1} {
        ::set value [lindex $args 0]
        switch -- $option {
            align {
                if {![string is digit -strict $value]} {
                    error "expected digit but got \"$value\""
                }
                ::set config(align) $value
            }
            comment {
                if {[string length $value] != 1} {
                    error "invalid comment \"$value\": must be one character"
                }
                ::set config(comment) $value
            }
            path {::set config(path) $value}
        }
        return $value
    } else {
        error "wrong # args: must be \"::config::change handle option ?value?\""
    }
    return
}

####
# ::config::close
#
# Closes and invalidates the specified handle.
#
proc ::config::close {handle} {
    Acquire $handle config
    ::unset -nocomplain config
    return
}

####
# ::config::free
#
# Clears the internal tree structure, which contains all configuration data.
#
proc ::config::free {handle} {
    Acquire $handle config
    ::set config(tree) [::tree::create]
    return
}

####
# ::config::read
#
# Reads the configuration file from disk. An error is raised if the file
# cannot be opened for reading.
#
proc ::config::read {handle} {
    Acquire $handle config
    ::set fileHandle [::open $config(path) r]

    # Clear the tree structure before reading new data.
    ::set config(tree) [::tree::create]
    ::set comments [list]
    ::set section ""

    foreach line [split [::read -nonewline $fileHandle] "\n"] {
        ::set line [string trim $line]

        if {[string index $line 0] eq $config(comment)} {
            lappend comments $line
        } elseif {[string match {\[*\]} $line]} {
            ::set section [string range $line 1 end-1]
            if {![string length $section]} {continue}

            # The section key must only be created once,
            # in case a section is defined multiple times.
            if {![::tree::exists $config(tree) $section]} {
                ::tree::set config(tree) $section [::tree::create data [::tree::create] length 0]
            }

            if {[llength $comments]} {
                ::set comments [concat [::tree::get2 $config(tree) $section comments] $comments]
                ::tree::set config(tree) $section comments $comments
                ::set comments [list]
            }
        } elseif {[::set index [string first "=" $line]] != -1} {
            ::set key [string trimright [string range $line 0 [expr {$index - 1}]]]
            if {![string length $key] || ![string length $section]} {continue}

            # The length of the longest key is used to align all
            # values when commiting the configuration file to disk.
            if {[string length $key] > [::tree::get $config(tree) $section length]} {
                ::tree::set config(tree) $section length [string length $key]
            }

            # Key comments must be placed before the line, not at the end.
            ::set keyTree [::tree::create value [string trimleft [string range $line [incr index] end]]]

            if {[llength $comments]} {
                ::tree::set keyTree comments $comments
                ::set comments [list]
            }
            ::tree::set config(tree) $section data $key $keyTree
        }
    }

    ::close $fileHandle
    return
}

####
# ::config::write
#
# Writes the configuration file to disk. An error is raised if the file
# cannot be opened for writing.
#
proc ::config::write {handle} {
    Acquire $handle config
    ::set fileHandle [::open $config(path) w]

    ::tree::for {section sectionTree} $config(tree) {
        if {$config(align)} {
            ::set length [::tree::get $sectionTree length]
            incr length [expr {$config(align) - 1}]
        }

        if {[::tree::exists $sectionTree comments]} {
            puts $fileHandle [join [::tree::get $sectionTree comments] "\n"]
        }
        puts $fileHandle "\[$section\]"

        ::tree::for {key keyTree} [::tree::get $sectionTree data] {
            if {[::tree::exists $keyTree comments]} {
                puts $fileHandle [join [::tree::get $keyTree comments] "\n"]
            }

            ::set value [::tree::get $keyTree value]
            if {$config(align)} {
                # Align values to the longest key name.
                puts $fileHandle [format "%-*s=%s" $length $key $value]
            } else {
                puts $fileHandle "$key=$value"
            }
        }
        puts $fileHandle ""
    }

    ::close $fileHandle
    return
}

####
# ::config::keys
#
# Returns a list of all keys within a given section. If the "pattern" argument
# is specified, only matching keys are returned.
#
proc ::config::keys {handle section {pattern "*"}} {
    Acquire $handle config
    return [::tree::keys [::tree::get2 $config(tree) $section data] $pattern]
}

####
# ::config::sections
#
# Returns a list of all configuration sections. If the "pattern" argument is
# specified, only matching sections are returned.
#
proc ::config::sections {handle {pattern "*"}} {
    Acquire $handle config
    return [::tree::keys $config(tree) $pattern]
}

####
# ::config::exists
#
# Test for the existence of a section or a key within a given section.
#
proc ::config::exists {handle section {key ""}} {
    Acquire $handle config
    if {[string length $key]} {
        return [::tree::exists $config(tree) $section data $key]
    } else {
        return [::tree::exists $config(tree) $section]
    }
}

####
# ::config::get
#
# Returns the value of the named key from the specified section.
#
proc ::config::get {handle section key} {
    Acquire $handle config
    return [::tree::get2 $config(tree) $section data $key value]
}

####
# ::config::getex
#
# Returns a list of key and value pairs from the specified section. If the
# "pattern" argument is specified, only matching keys are returned.
#
proc ::config::getex {handle section {pattern "*"}} {
    Acquire $handle config
    ::set pairList [list]
    ::tree::for {key keyTree} [::tree::get2 $config(tree) $section data] {
        if {[string match $pattern $key]} {
            lappend pairList $key [::tree::get $keyTree value]
        }
    }
    return $pairList
}

####
# ::config::set
#
# Sets the value of the key in the specified section. If the section does not
# exist, a new one is created.
#
proc ::config::set {handle section args} {
    Acquire $handle config
    ::set argc [llength $args]
    if {$argc != 0  && $argc != 2} {
        error "wrong # args: must be \"::config::set handle section ?key value?\""
    }

    # Initialise the section if it does not exist.
    if {![::tree::exists $config(tree) $section]} {
        ::tree::set config(tree) $section [::tree::create data [::tree::create] length 0]
    }

    # Set the key's value and update the length if necessary.
    if {$argc == 2} {
        foreach {key value} $args {break}
        if {[string length $key] > [::tree::get $config(tree) $section length]} {
            ::tree::set config(tree) $section length [string length $key]
        }
        ::tree::set config(tree) $section data $key value $value
        return $value
    }
    return
}

####
# ::config::unset
#
# Removes the key or the entire section and all its keys.
#
proc ::config::unset {handle section {key ""}} {
    Acquire $handle config
    if {[string length $key]} {
        ::tree::unset config(tree) $section data $key
    } else {
        ::tree::unset config(tree) $section
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
