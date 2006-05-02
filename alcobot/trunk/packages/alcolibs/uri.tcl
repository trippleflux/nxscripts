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
#   Uri::Parse <url>
#

package require alco::util 1.2

namespace eval ::Uri {}

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
                # Design Note:
                # The "value" may contain escapes (e.g. %xx). To keep the code
                # small and simple, I won't implement an un-escape routine.
                lappend uri(params) $name $value
            }
        }
    }
    return [array get uri]
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
