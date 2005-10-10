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
#   Exported Procedures:
#     FtpOpen        <host> <port> <user> <passwd> [secure]
#     FtpClose       <handle>
#     FtpGetError    <handle>
#     FtpConnect     <handle>
#     FtpDisconnect  <handle>
#     FtpIsConnected <handle>
#

namespace eval ::alcoholicz {
    if {![info exists ftpNextHandle]} {
        variable ftpNextHandle 0
    }
    catch {package require tls 1.5}
    namespace export FtpOpen FtpClose FtpGetError \
        FtpConnect FtpDisconnect FtpIsConnected
}

####
# bgerror
#
# Display background errors.
#
proc bgerror {message} {
    ::alcoholicz::LogDebug BgError $message
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
# Creates a new FTP client handle. This handle is used by every FTP procedure
# and must be closed using FtpClose.
#
# Secure Options:
# none     - Regular connection.
# implicit - Implicit SSL connection.
# ssl      - Explicit SSL connection (AUTH SSL).
# tls      - Explicit TLS connection (AUTH TLS).
#
proc ::alcoholicz::FtpOpen {host port user passwd {secure "none"}} {
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
        host   $host    \
        port   $port    \
        user   $user    \
        passwd $passwd  \
        secure $secure  \
        error  ""       \
        sock   ""       \
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

####
# FtpGetError
#
# Returns the last error message.
#
proc ::alcoholicz::FtpGetError {handle} {
    FtpAcquire $handle
    return $ftp(error)
}

####
# FtpConnect
#
# Connect to the FTP server.
#
proc ::alcoholicz::FtpConnect {handle} {
    FtpAcquire $handle

    if {$ftp(sock) ne ""} {
        error "ftp connection already open, disconnect first"
    }

    # Asynchronous sockets in Tcl are created immediately but may not be
    # connected yet. The writable channel event callback is executed when
    # the socket is connected or if the connection failed.
    set ftp(sock) [socket -async $ftp(host) $ftp(port)]

    fileevent $ftp(sock) writable [list [namespace current]::FtpVerify $handle]
    return
}

####
# FtpVerify
#
# Verifies the connection's state and begins the SSL negotiation for
# FTP servers using implicit SSL.
#
proc ::alcoholicz::FtpVerify {handle} {
    upvar [namespace current]::$handle ftp
    if {![info exists ftp]} {
        LogDebug FtpVerify "Handle \"$handle\" closed before connection succeeded."
        return
    }

    # Disable the writeable channel event.
    fileevent $ftp(sock) writable {}

    set ftp(error) [fconfigure $ftp(sock) -error]
    if {$ftp(error) ne ""} {
        LogDebug FtpVerify "Unable to connect to $ftp(host):$ftp(port) ($handle): $ftp(error)"

        # The socket must be closed and set to an empty string
        # in order for FtpIsConnected to return false.
        catch {close $ftp(sock)}
        set ftp(sock) ""
        return
    }

    set peer [fconfigure $ftp(sock) -peername]
    LogDebug FtpVerify "Connected to [lindex $peer 0]:[lindex $peer 2] ($handle)."

    # Perform SSL negotiation for FTP servers using implicit SSL.
    if {$ftp(secure) eq "implicit"} {
        set ftp(sock) [tls::import $ftp(sock) -require 0 -ssl2 1 -ssl3 1 -tls1 1]
    }

    # Set socket options and event handlers.
    fconfigure $ftp(sock) -buffering line -blocking 0 -translation {auto crlf}
    # TODO: State handler.
    #fileevent $ftp(sock) readable [list [namespace current]::FtpHandler $handle]
    return
}

####
# FtpDisconnect
#
# Disconnects from the FTP server.
#
proc ::alcoholicz::FtpDisconnect {handle} {
    FtpAcquire $handle

    # TODO: Send QUIT
    catch {close $ftp(sock)}
    set ftp(sock) ""
    return
}

####
# FtpIsConnected
#
# Checks if the current handle is connected.
#
proc ::alcoholicz::FtpIsConnected {handle} {
    FtpAcquire $handle
    return [expr {$ftp(sock) ne ""}]
}
