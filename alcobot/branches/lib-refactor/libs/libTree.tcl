#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Tree Library
#
# Author:
#   neoxed (neoxed@gmail.com) May 13, 2005
#
# Abstract:
#   Tree data structure, implemented using Tcl lists and recursion. Unlike
#   Tcl's arrays and dictionaries, this method retains the natural order of
#   its keys and nodes.
#
# Exported Procedures:
#   TreeCreate   [<key> <value> ...]
#   TreeExists   <tree> <key> [<key> ...]
#   TreeFor      {keyVar valueVar} <tree> <body>
#   TreeKeys     <tree> [pattern]
#   TreeValues   <tree> [pattern]
#   TreeGet      <tree> <key> [<key> ...]
#   TreeGetNaive <tree> <key> [<key> ...]
#   TreeSet      <tree> <key> [<key> ...] <value>
#   TreeUnset    <tree> <key> [<key> ...]
#

namespace eval ::tree {
    namespace export TreeCreate TreeExists TreeFor TreeKeys TreeValues \
        TreeGet TreeGetNaive TreeSet TreeUnset
}

####
# TreeCreate
#
# Creates a new tree that contains each of the key and value mappings listed
# as arguments. Keys and values must be in alternating order.
#
proc ::tree::TreeCreate {args} {
    if {[llength $args] & 1} {
        error "unbalanced list"
    }
    return $args
}

####
# TreeExists
#
# Returns a boolean value indicating whether the given key exists in the
# specified tree.
#
proc ::tree::TreeExists {tree key args} {
    foreach {name node} $tree {
        if {$key eq $name} {
            if {![llength $args]} {return 1}
            return [eval TreeExists [list $node] $args]
        }
    }
    return 0
}

####
# TreeFor
#
# Iterates through a tree, in the same manner as foreach. Two variable names
# must be specified for the first parameter, which are mapped to the key and
# value for each iteration.
#
proc ::tree::TreeFor {vars tree body} {
    if {[llength $vars] != 2} {
        error "must have exactly two variable names"
    }
    uplevel 1 [list foreach $vars $tree $body]
}

####
# TreeKeys
#
# Returns a list of all keys in the given tree. If the "pattern" argument
# is specified, only matching keys are returned.
#
proc ::tree::TreeKeys {tree {pattern "*"}} {
    set keys [list]
    foreach {name node} $tree {
        if {[string match $pattern $name]} {
             lappend keys $name
        }
    }
    return $keys
}

####
# TreeValues
#
# Returns a list of all values in the given tree. If the "pattern" argument
# is specified, only matching values are returned.
#
proc ::tree::TreeValues {tree {pattern "*"}} {
    set values [list]
    foreach {name node} $tree {
        if {[string match $pattern $node]} {
             lappend values $node
        }
    }
    return $values
}

####
# TreeGet
#
# Retrieves the value of a key within the specified tree. Multiple keys can be
# specified to retrieve values of nested keys. An error is raised if you attempt
# to retrieve a value for a key that does not exist.
#
proc ::tree::TreeGet {tree key args} {
    foreach {name node} $tree {
        if {$key eq $name} {
            if {![llength $args]} {return $node}
            return [eval TreeGet [list $node] $args]
        }
    }
    error "key \"$key\" not known in tree"
}

####
# TreeGetNaive
#
# A variation of TreeGet that returns an empty string when a nonexistent key is
# specified. This simplifies implementations that consider the key's existence to
# be irrelevant.
#
proc ::tree::TreeGetNaive {tree key args} {
    foreach {name node} $tree {
        if {$key eq $name} {
            if {![llength $args]} {return $node}
            return [eval TreeGetNaive [list $node] $args]
        }
    }
    return {}
}

####
# TreeSet
#
# Creates or updates the value for a key within a tree. When multiple keys are
# specified, it creates or updates a chain of nested keys. Note that this
# procedure takes name of a tree variable, not the actual tree.
#
proc ::tree::TreeSet {treeVar key value args} {
    upvar 1 $treeVar tree
    if {![info exists tree]} {set tree [list]}

    set exists 0; set nodeIndex 1
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
        eval TreeSet node [list $value] $args
    } else {
        set node $value
    }
    set tree [linsert $tree $nodeIndex $node]
}

####
# TreeUnset
#
# Removes a key and its value from a tree. When multiple keys are specified,
# it removes the deepest existing key. Note that this procedure takes name of
# a tree variable, not the actual tree.
#
proc ::tree::TreeUnset {treeVar key args} {
    upvar 1 $treeVar tree
    if {![info exists tree]} {return [list]}

    set index 0
    foreach {name node} $tree {
        if {$key ne $name} {incr index 2; continue}
        if {[llength $args]} {
            incr index
            eval TreeUnset node $args
            set tree [lreplace $tree $index $index $node]
        } else {
            set tree [lreplace $tree $index [incr index]]
        }
        break
    }
    return $tree
}
