#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Tree Library
#
# Author:
#   neoxed (neoxed@gmail.com) May 13, 2005
#
# Abstract:
#   Tree data structure, implemented using Tcl lists and recursion. Unlike
#   Tcl's arrays and dictionaries, this method retains the original order of
#   its keys and nodes.
#
# Error Handling:
#   Errors thrown with the "TREE" error code indicate user errors, e.g.
#   requesting the node of an unknown key. Errors thrown without the "TREE"
#   error code indicate a caller implementation problem.
#
# Procedures:
#   Tree::Create   [<key> <value> ...]
#   Tree::Exists   <tree> <key> [<key> ...]
#   Tree::For      {keyVar valueVar} <tree> <body>
#   Tree::Keys     <tree> [pattern]
#   Tree::Values   <tree> [pattern]
#   Tree::Get      <tree> <key> [<key> ...]
#   Tree::GetQuiet <tree> <key> [<key> ...]
#   Tree::Set      <tree> <key> [<key> ...] <value>
#   Tree::Unset    <tree> <key> [<key> ...]
#

package require alco::util 1.2

namespace eval ::Tree {}

####
# Tree::Create
#
# Creates a new tree that contains each of the key and value mappings listed
# as arguments. Keys and values must be in alternating order.
#
proc ::Tree::Create {args} {
    if {[llength $args] & 1} {
        error "unbalanced list"
    }
    return $args
}

####
# Tree::Exists
#
# Returns a boolean value indicating whether the given key exists in the
# specified tree.
#
proc ::Tree::Exists {tree key args} {
    foreach {name node} $tree {
        if {$key eq $name} {
            if {![llength $args]} {return 1}
            return [eval ::Tree::Exists [list $node] $args]
        }
    }
    return 0
}

####
# Tree::For
#
# Iterates through a tree, in the same manner as foreach. Two variable names
# must be specified for the first parameter, which are mapped to the key and
# value for each iteration.
#
proc ::Tree::For {vars tree body} {
    if {[llength $vars] != 2} {
        error "must have exactly two variable names"
    }
    uplevel 1 [list foreach $vars $tree $body]
}

####
# Tree::Keys
#
# Returns a list of all keys in the given tree. If the "pattern" argument
# is specified, only matching keys are returned.
#
proc ::Tree::Keys {tree {pattern "*"}} {
    set keys [list]
    foreach {name node} $tree {
        if {[string match $pattern $name]} {
             lappend keys $name
        }
    }
    return $keys
}

####
# Tree::Values
#
# Returns a list of all values in the given tree. If the "pattern" argument
# is specified, only matching values are returned.
#
proc ::Tree::Values {tree {pattern "*"}} {
    set values [list]
    foreach {name node} $tree {
        if {[string match $pattern $node]} {
             lappend values $node
        }
    }
    return $values
}

####
# Tree::Get
#
# Retrieves the value of a key within the specified tree. Multiple keys can be
# specified to retrieve values of nested keys. An error is raised if you attempt
# to retrieve a value for a key that does not exist.
#
proc ::Tree::Get {tree key args} {
    foreach {name node} $tree {
        if {$key eq $name} {
            if {![llength $args]} {return $node}
            return [eval ::Tree::Get [list $node] $args]
        }
    }
    throw TREE "key \"$key\" not known in tree"
}

####
# Tree::GetQuiet
#
# A variation of Tree::Get that returns an empty string when a nonexistent key is
# specified. This simplifies implementations that consider the key's existence to
# be irrelevant.
#
proc ::Tree::GetQuiet {tree key args} {
    foreach {name node} $tree {
        if {$key eq $name} {
            if {![llength $args]} {return $node}
            return [eval ::Tree::GetQuiet [list $node] $args]
        }
    }
    return {}
}

####
# Tree::Set
#
# Creates or updates the value for a key within a tree. When multiple keys are
# specified, it creates or updates a chain of nested keys. Note that this
# procedure takes name of a tree variable, not the actual tree.
#
proc ::Tree::Set {treeVar key value args} {
    upvar 1 $treeVar tree
    if {![info exists tree]} {
        set tree [list]
    }

    set exists 0
    set nodeIndex 1
    foreach {name node} $tree {
        if {$key eq $name} {set exists 1; break}
        incr nodeIndex 2
    }

    if {$exists} {
        set tree [lreplace $tree $nodeIndex $nodeIndex]
    } else {
        lappend tree $key
        set node [list]
        set nodeIndex end
    }

    if {[llength $args]} {
        eval ::Tree::Set node [list $value] $args
    } else {
        set node $value
    }
    set tree [linsert $tree $nodeIndex $node]
}

####
# Tree::Unset
#
# Removes a key and its value from a tree. When multiple keys are specified,
# it removes the deepest existing key. Note that this procedure takes name of
# a tree variable, not the actual tree.
#
proc ::Tree::Unset {treeVar key args} {
    upvar 1 $treeVar tree
    if {![info exists tree]} {return [list]}

    set index 0
    foreach {name node} $tree {
        if {$key ne $name} {incr index 2; continue}
        if {[llength $args]} {
            incr index
            eval ::Tree::Unset node $args
            set tree [lreplace $tree $index $index $node]
        } else {
            set tree [lreplace $tree $index [incr index]]
        }
        break
    }
    return $tree
}

package provide alco::tree 1.2.0
