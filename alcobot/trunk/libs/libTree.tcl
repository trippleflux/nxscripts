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
# Procedures:
#   tree::create [<key> <value> ...]
#   tree::exists <tree> <key> [<key> ...]
#   tree::for    {keyVar valueVar} <tree> <body>
#   tree::keys   <tree> [pattern]
#   tree::values <tree> [pattern]
#   tree::get    <tree> <key> [<key> ...]
#   tree::get2   <tree> <key> [<key> ...]
#   tree::set    <tree> <key> [<key> ...] <value>
#   tree::unset  <tree> <key> [<key> ...]
#

namespace eval ::tree {}

####
# tree::create
#
# Creates a new tree that contains each of the key and value mappings listed
# as arguments. Keys and values must be in alternating order.
#
proc ::tree::create {args} {
    if {[llength $args] & 1} {
        throw TREE "unbalanced list"
    }
    return $args
}

####
# tree::exists
#
# Returns a boolean value indicating whether the given key exists in the
# specified tree.
#
proc ::tree::exists {tree key args} {
    foreach {name node} $tree {
        if {$key eq $name} {
            if {![llength $args]} {return 1}
            return [eval ::tree::exists [list $node] $args]
        }
    }
    return 0
}

####
# tree::for
#
# Iterates through a tree, in the same manner as foreach. Two variable names
# must be specified for the first parameter, which are mapped to the key and
# value for each iteration.
#
proc ::tree::for {vars tree body} {
    if {[llength $vars] != 2} {
        throw TREE "must have exactly two variable names"
    }
    uplevel 1 [list foreach $vars $tree $body]
}

####
# tree::keys
#
# Returns a list of all keys in the given tree. If the "pattern" argument
# is specified, only matching keys are returned.
#
proc ::tree::keys {tree {pattern "*"}} {
    ::set keys [list]
    foreach {name node} $tree {
        if {[string match $pattern $name]} {
             lappend keys $name
        }
    }
    return $keys
}

####
# tree::values
#
# Returns a list of all values in the given tree. If the "pattern" argument
# is specified, only matching values are returned.
#
proc ::tree::values {tree {pattern "*"}} {
    ::set values [list]
    foreach {name node} $tree {
        if {[string match $pattern $node]} {
             lappend values $node
        }
    }
    return $values
}

####
# tree::get
#
# Retrieves the value of a key within the specified tree. Multiple keys can be
# specified to retrieve values of nested keys. An error is raised if you attempt
# to retrieve a value for a key that does not exist.
#
proc ::tree::get {tree key args} {
    foreach {name node} $tree {
        if {$key eq $name} {
            if {![llength $args]} {return $node}
            return [eval ::tree::get [list $node] $args]
        }
    }
    throw TREE "key \"$key\" not known in tree"
}

####
# tree::get2
#
# A variation of ::tree::get that returns an empty string when a nonexistent key is
# specified. This simplifies implementations that consider the key's existence to
# be irrelevant.
#
proc ::tree::get2 {tree key args} {
    foreach {name node} $tree {
        if {$key eq $name} {
            if {![llength $args]} {return $node}
            return [eval ::tree::get2 [list $node] $args]
        }
    }
    return {}
}

####
# tree::set
#
# Creates or updates the value for a key within a tree. When multiple keys are
# specified, it creates or updates a chain of nested keys. Note that this
# procedure takes name of a tree variable, not the actual tree.
#
proc ::tree::set {treeVar key value args} {
    upvar 1 $treeVar tree
    if {![info exists tree]} {
        ::set tree [list]
    }

    ::set exists 0
    ::set nodeIndex 1
    foreach {name node} $tree {
        if {$key eq $name} {::set exists 1; break}
        incr nodeIndex 2
    }

    if {$exists} {
        ::set tree [lreplace $tree $nodeIndex $nodeIndex]
    } else {
        lappend tree $key
        ::set node [list]
        ::set nodeIndex end
    }

    if {[llength $args]} {
        eval ::tree::set node [list $value] $args
    } else {
        ::set node $value
    }
    ::set tree [linsert $tree $nodeIndex $node]
}

####
# tree::unset
#
# Removes a key and its value from a tree. When multiple keys are specified,
# it removes the deepest existing key. Note that this procedure takes name of
# a tree variable, not the actual tree.
#
proc ::tree::unset {treeVar key args} {
    upvar 1 $treeVar tree
    if {![info exists tree]} {return [list]}

    ::set index 0
    foreach {name node} $tree {
        if {$key ne $name} {incr index 2; continue}
        if {[llength $args]} {
            incr index
            eval ::tree::unset node $args
            ::set tree [lreplace $tree $index $index $node]
        } else {
            ::set tree [lreplace $tree $index [incr index]]
        }
        break
    }
    return $tree
}
