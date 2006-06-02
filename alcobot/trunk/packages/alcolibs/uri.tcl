#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   URI Library
#
# Author:
#   neoxed (neoxed@gmail.com) May 2, 2006
#
# Abstract:
#   Implements a URI handling library.
#
# Procedures:
#   Uri::Parse   <url>
#   Uri::Quote   <value>
#   Uri::Unquote <value>
#

package require alco::util 1.2

namespace eval ::Uri {
    variable trans {a-zA-Z0-9$_.+!*'(,):=@;-}
}

####
# Uri::Parse
#
# Splits the given URL into its constituents.
#
proc ::Uri::Parse {url} {
    # RFC 1738:	scheme = 1*[ lowalpha | digit | "+" | "-" | "." ]
    if {![regexp -- {^([a-z0-9+.-]+):} $url dummy uri(scheme)]} {
        throw URI "invalid URI scheme \"$url\""
    }
    set index [string first ":" $url]
    set rest [string range $url [incr index] end]

    # Count the slashes after "scheme:".
    set slashes 0
    while {[string index $rest $slashes] eq "/"} {incr slashes}

    if {$slashes == 0} {
        # scheme:path
        set uri(path) $rest
    } elseif {$slashes == 1 || $slashes == 3} {
        # scheme:/path
        # scheme:///path
        set rest [string range $rest $slashes end]
    } else {
        # scheme://host
        # scheme://host/path
        set rest [string range $rest 2 end]
        if {![GetTuple $rest "/" uri(host) rest]} {
            set uri(host) $rest; set rest ""
        }
    }

    if {[info exists uri(host)]} {
        # scheme://user@host
        # scheme://user:password@host
        if {[GetTuple $uri(host) "@" uri(user) uri(host)]} {
            GetTuple $uri(user) ":" uri(user) uri(password)
        }
        # scheme://host:port
        GetTuple $uri(host) ":" uri(host) uri(port)
    }

    if {![info exists uri(path)]} {
        set uri(path) "/$rest"
    }
    if {[GetTuple $uri(path) "?" uri(path) params]} {
        foreach param [split $params "&"] {
            if {[GetTuple $param "=" name value]} {
                lappend uri(params) $name [Unquote $value]
            }
        }
    }
    return [array get uri]
}

####
# Uri::Quote
#
# Replaces special characters using the "%xx" escape.
#
proc ::Uri::Quote {value} {
    variable trans
    set index 0; set result ""
    while {[regexp -indices -- "\[^$trans\]" $value indices]} {
        set index [lindex $indices 0]
        scan [string index $value $index] %c char
        if {$char == 0} {
            error "invalid null character"
        }
        append result [string range $value 0 [incr index -1]] %[format "%.2X" $char]
        set value [string range $value [incr index 2] end]
    }
    return [append result $value]
}

####
# Uri::Unquote
#
# Replaces "%xx" escapes by their single-character equivalent.
#
proc ::Uri::Unquote {value} {
    set start 0; set result ""
    while {[regexp -start $start -indices {%[0-9a-fA-F]{2}} $value match]} {
        foreach {first last} $match {break}
        append result [string range $value $start [expr {$first - 1}]]
        append result [format "%c" 0x[string range $value [incr first] $last]]
        set start [incr last]
    }
    append result [string range $value $start end]
    return $result
}

################################################################################
# Internal Procedures                                                          #
################################################################################

####
# GetTuple
#
# Parses a name and value pair.
#
proc ::Uri::GetTuple {input separator nameVar valueVar} {
    upvar $nameVar name $valueVar value
    set index [string first $separator $input]
    if {$index != -1} {
        set name  [string range $input 0 [expr {$index - 1}]]
        set value [string range $input [expr {$index + 1}] end]
        return 1
    }
    return 0
}

package provide alco::uri 1.2.0
