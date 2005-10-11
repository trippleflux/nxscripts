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
#     FtpOpen       <host> <port> <user> <passwd> [-secure <type>]
#     FtpClose      <handle>
#     FtpGetError   <handle>
#     FtpGetStatus  <handle>
#     FtpConnect    <handle>
#     FtpDisconnect <handle>
#     FtpCommand    <handle> <command> [callback]
#

namespace eval ::alcoholicz {
    if {![info exists ftpNextHandle]} {
        variable ftpNextHandle 0
    }
    catch {package require tls 1.5}
    namespace export FtpOpen FtpClose FtpGetError FtpGetStatus \
        FtpConnect FtpDisconnect FtpCommand
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
proc ::alcoholicz::FtpOpen {host port user passwd args} {
    variable ftpNextHandle

    set secure ""
    foreach {option value} $args {
        if {$option eq "-secure"} {
            switch -- $value {
                none {}
                implicit - ssl - tls {
                    # Make sure the TLS package is present (http://tls.sourceforge.net).
                    package present tls
                }
                default {
                    error "invalid value \"$value\": must be none, implicit, ssl, or tls"
                }
            }
            set secure $value
        } elseif {$option eq "-timeout"} {
            # TODO: connection timeout
            error "not implemented"
        } else {
            error "invalid switch \"$option\": must be -secure"
        }
    }

    set handle "ftp$ftpNextHandle"
    upvar [namespace current]::$handle ftp

    #
    # FTP Handle Contents
    #
    # ftp(host)   - Remote servet host.
    # ftp(port)   - Remote servet port.
    # ftp(user)   - Client user name.
    # ftp(passwd) - Client password.
    # ftp(secure) -
    # ftp(event)  - Event handler state.
    # ftp(error)  - Last error message.
    # ftp(sock)   - Socket channel.
    # ftp(status) - Connection status (0=disconnected, 1=connecting, 2=connected).
    #
    array set ftp [list \
        host   $host    \
        port   $port    \
        user   $user    \
        passwd $passwd  \
        secure $secure  \
        error  ""       \
        event  ""       \
        sock   ""       \
        status 0        \
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
    FtpShutdown $handle
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
# FtpGetStatus
#
# Returns the connection status.
#
proc ::alcoholicz::FtpGetStatus {handle} {
    FtpAcquire $handle
    return $ftp(status)
}

####
# FtpConnect
#
# Connect to the FTP server.
#
proc ::alcoholicz::FtpConnect {handle} {
    FtpAcquire $handle

    if {$ftp(sock) ne ""} {
        error "ftp connection open, disconnect first"
    }
    set ftp(status) 1

    # Asynchronous sockets in Tcl are created immediately but may not be
    # connected yet. The writable channel event callback is executed when
    # the socket is connected or if the connection failed.
    set ftp(sock) [socket -async $ftp(host) $ftp(port)]

    fileevent $ftp(sock) writable [list [namespace current]::FtpVerify $handle]
    return
}

####
# FtpDisconnect
#
# Disconnects from the FTP server.
#
proc ::alcoholicz::FtpDisconnect {handle} {
    FtpAcquire $handle
    FtpShutdown $handle
    return
}

####
# FtpCommand
#
# Sends a command to the FTP server.
#
proc ::alcoholicz::FtpCommand {handle command {callback ""}} {
    FtpAcquire $handle

    # TODO: Queue command.
    return
}

####
# FtpShutdown
#
# Shuts down the FTP connection.
#
proc ::alcoholicz::FtpShutdown {handle} {
    upvar [namespace current]::$handle ftp
    if {[info exists ftp]} {
        catch {close $ftp(sock)}
        set ftp(sock) ""
        set ftp(status) 0
    }
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
        LogDebug FtpVerify "Handle \"$handle\" does not exist."
        return
    }

    # Disable the writeable channel event.
    fileevent $ftp(sock) writable {}

    set ftp(error) [fconfigure $ftp(sock) -error]
    if {$ftp(error) ne ""} {
        LogDebug FtpVerify "Unable to connect to $ftp(host):$ftp(port) ($handle): $ftp(error)"
        FtpShutdown $handle
        return
    }
    set ftp(status) 2

    set peer [fconfigure $ftp(sock) -peername]
    LogDebug FtpVerify "Connected to [lindex $peer 0]:[lindex $peer 2] ($handle)."

    # Perform SSL negotiation for FTP servers using implicit SSL.
    if {$ftp(secure) eq "implicit"} {
        set ftp(sock) [tls::import $ftp(sock) -require 0 -ssl2 1 -ssl3 1 -tls1 1]
    }

    # Set socket options and event handlers.
    fconfigure $ftp(sock) -buffering line -blocking 0 -translation {auto crlf}
    fileevent $ftp(sock) readable [list [namespace current]::FtpHandler $handle]
    return
}

####
# FtpHandler
#
# FTP client event handler.
#
proc ::alcoholicz::FtpHandler {handle} {
    upvar [namespace current]::$handle ftp
    if {![info exists ftp]} {
        LogDebug FtpHandler "Handle \"$handle\" does not exist."
        return
    }

    # TODO: everything

    return
}
