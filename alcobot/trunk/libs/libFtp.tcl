#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   FTP Client Library
#
# Author:
#   neoxed (neoxed@gmail.com) Oct 10, 2005
#
# Abstract:
#   Implements a FTP client library to interact with FTP servers.
#

namespace eval ::alcoholicz {
    if {![info exists ftpNextHandle]} {
        set ftpNextHandle 0
    }
    catch {package require tls 1.5}
    namespace export FtpOpen FtpClose
}

####
# FtpAcquire
#
# Validate and acquire a FTP handle. This procedure is for internal use only,
# hence why it is not exported.
#
proc ::alcoholicz::FtpAcquire {handle {handleVar "ftp"}} {
    if {![regexp -- {ftp\d+} $handle] || ![array exists [namespace current]::$handle]} {
        error "invalid ftp handle \"$handle\""
    }
    uplevel 1 [list upvar [namespace current]::$handle $handleVar]
}

####
# FtpOpen
#
# Create a new FTP client handle. This handle is used by every FTP procedure
# and must be closed using FtpClose.
#
# Secure Options:
# none     - Regular connection.
# implicit - Implicit SSL connection.
# ssl      - Explicit SSL connection (AUTH SSL).
# tls      - Explicit TLS connection (AUTH TLS).
#
proc ::alcoholicz::FtpOpen {server port user passwd {secure "none"}} {
    variable ftpNextHandle

    switch -- $secure {
        {} {}
        none {set secure ""}
        implicit - ssl - tls {
            # Make sure the TLS package is present (http://tls.sourceforge.net).
            package present tls
        }
        default {
            error "invalid secure option \"$secure\": must be none, implicit, ssl, or tls"
        }
    }

    set handle "ftp$ftpNextHandle"
    upvar [namespace current]::$handle ftp

    array set ftp [list \
        server $user    \
        port   $port    \
        user   $user    \
        passwd $passwd  \
        secure $secure  \
    ]

    incr ftpNextHandle
    return $handle
}

####
# FtpClose
#
# Closes and invalidates the specified handle.
#
proc ::alcoholicz::FtpClose {handle} {
    FtpAcquire $handle
    unset -nocomplain ftp
    return
}
