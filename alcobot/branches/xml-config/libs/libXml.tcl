#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   XML Library
#
# Author:
#   neoxed (neoxed@gmail.com) Feb 13, 2006
#
# Abstract:
#   Implements a XML file parser, wrapped around tDOM.
#
# Exported Procedures:
#   XmlOpen  <filePath>
#   XmlClose <handle>
#   XmlRead  <handle>
#   XmlWrite <handle>
#

namespace eval ::xml {
    variable nextHandle
    if {![info exists nextHandle]} {
        set nextHandle 0
    }
    namespace export XmlOpen XmlClose XmlRead XmlWrite
}

####
# Acquire
#
# Validate and acquire a XML file handle.
#
proc ::xml::Acquire {handle handleVar} {
    if {![regexp -- {xml\d+} $handle] || ![array exists [namespace current]::$handle]} {
        error "invalid xml handle \"$handle\""
    }
    uplevel 1 [list upvar [namespace current]::$handle $handleVar]
}

####
# XmlOpen
#
# TODO
#
proc ::xml::XmlOpen {filePath} {
}

####
# XmlClose
#
# TODO
#
proc ::xml::XmlClose {handle} {
    Acquire $handle xml
}

####
# XmlRead
#
# TODO
#
proc ::xml::XmlRead {handle} {
    Acquire $handle xml
}

####
# XmlWrite
#
# TODO
#
proc ::xml::XmlWrite {handle} {
    Acquire $handle xml
}
