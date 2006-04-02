#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
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
# Procedures:
#   Ftp::Init       <logCallback>
#   Ftp::Open       <host> <port> <user> <passwd> [-notify <callback>] [-secure <type>]
#   Ftp::Close      <handle>
#   Ftp::GetError   <handle>
#   Ftp::GetStatus  <handle>
#   Ftp::Connect    <handle>
#   Ftp::Disconnect <handle>
#   Ftp::Command    <handle> <command> [callback]
#

namespace eval ::Ftp {
    variable logCallback
    variable nextHandle
    if {![info exists logCallback]} {
        set logCallback ""
        set nextHandle 0
    }
}

####
# Ftp::Init
#
# Initialises the FTP library.
#
proc ::Ftp::Init {logProc} {
    # Log callback (disabled if "").
    variable logCallback $logProc

    # Required for SSL/TLS support.
    if {[catch {package require tls 1.5} message]} {
        Debug FtpInit "SSL/TLS support disabled: $message"
    }
}

####
# Ftp::Open
#
# Creates a new FTP client handle. This handle is used by every FTP procedure
# and must be closed using ::Ftp::Close.
#
# Secure Options:
# none     - Regular connection.
# implicit - Implicit SSL connection.
# ssl      - Explicit SSL connection (AUTH SSL).
# tls      - Explicit TLS connection (AUTH TLS).
#
proc ::Ftp::Open {host port user passwd args} {
    variable nextHandle

    set notify ""; set secure ""
    foreach {option value} $args {
        if {$option eq "-notify"} {
            set notify $value

        } elseif {$option eq "-secure"} {
            switch -- $value {
                {} {}
                none {set value ""}
                implicit - ssl - tls {
                    # Make sure the TLS package (http://tls.sf.net) is present.
                    if {[catch {package present tls} message]} {
                        throw FTP "SSL/TLS not available"
                    }
                }
                default {
                    throw FTP "invalid value \"$value\": must be none, implicit, ssl, or tls"
                }
            }
            set secure $value

        } elseif {$option eq "-timeout"} {
            # TODO: Connection timeout.
            throw FTP "timeout not implemented"

        } else {
            throw FTP "invalid switch \"$option\": must be -notify, -secure, or -timeout"
        }
    }

    set handle "ftp$nextHandle"
    upvar [namespace current]::$handle ftp

    #
    # FTP Handle Contents
    #
    # ftp(host)   - Remote server host.
    # ftp(port)   - Remote server port.
    # ftp(user)   - Client user name.
    # ftp(passwd) - Client password.
    # ftp(notify) - Callback to notify when connected.
    # ftp(secure) - Connection security.
    # ftp(error)  - Last error message.
    # ftp(queue)  - Event queue (FIFO).
    # ftp(sock)   - Socket channel.
    # ftp(status) - Connection status (0=disconnected, 1=connecting, 2=connected).
    #
    array set ftp [list \
        host   $host    \
        port   $port    \
        user   $user    \
        passwd $passwd  \
        notify $notify  \
        secure $secure  \
        error  ""       \
        queue  [list]   \
        sock   ""       \
        status 0        \
    ]

    incr nextHandle
    return $handle
}

####
# Ftp::Close
#
# Closes and invalidates the specified handle.
#
proc ::Ftp::Close {handle} {
    Acquire $handle ftp
    Shutdown $handle
    unset -nocomplain ftp
    return
}

####
# Ftp::GetError
#
# Returns the last error message.
#
proc ::Ftp::GetError {handle} {
    Acquire $handle ftp
    return $ftp(error)
}

####
# Ftp::GetStatus
#
# Returns the connection status.
#
proc ::Ftp::GetStatus {handle} {
    Acquire $handle ftp
    return $ftp(status)
}

####
# Ftp::Connect
#
# Connects to the FTP server.
#
proc ::Ftp::Connect {handle} {
    Acquire $handle ftp

    if {$ftp(sock) ne ""} {
        throw FTP "ftp connection open, disconnect first"
    }
    set ftp(error) ""
    set ftp(status) 1

    # Asynchronous sockets in Tcl are created immediately but may not be
    # connected yet. The writable channel event callback is executed when
    # the socket is connected or if the connection failed.
    set ftp(sock) [socket -async $ftp(host) $ftp(port)]

    fileevent $ftp(sock) writable [list [namespace current]::Verify $handle]
    return
}

####
# Ftp::Disconnect
#
# Disconnects from the FTP server.
#
proc ::Ftp::Disconnect {handle} {
    Acquire $handle ftp
    Shutdown $handle
    return
}

####
# Ftp::Command
#
# Sends a command to the FTP server. The server's response can be retrieved
# by specifying a callback, since this library operates asynchronously.
# For example:
#
# proc SiteWhoCallback {handle response} {
#     foreach {code text} $response {
#         putlog "$code: $text"
#     }
# }
#
# set handle [Ftp::Open localhost 21 user pass]
# Ftp::Connect $handle
# Ftp::Command $handle "SITE WHO" SiteWhoCallback
# Ftp::Close $handle
#
proc ::Ftp::Command {handle command {callback ""}} {
    Acquire $handle ftp

    if {$ftp(status) != 2} {
        throw FTP "not connected"
    }
    lappend ftp(queue) [list quote $command $callback]

    # If there's only event in queue, invoke the handler directly.
    if {[llength $ftp(queue)] == 1} {
        Handler $handle 1
    }
    return
}

####
# Acquire
#
# Validate and acquire a FTP handle.
#
proc ::Ftp::Acquire {handle handleVar} {
    if {![regexp -- {ftp\d+} $handle] || ![array exists [namespace current]::$handle]} {
        throw FTP "invalid ftp handle \"$handle\""
    }
    uplevel 1 [list upvar [namespace current]::$handle $handleVar]
}

####
# Debug
#
# Logs a debug message.
#
proc ::Ftp::Debug {function message} {
    variable logCallback
    if {$logCallback ne ""} {
        eval [list $logCallback $function $message]
    }
}

####
# Evaluate
#
# Evaluates a callback script.
#
proc ::Ftp::Evaluate {script args} {
    if {$script ne "" && [catch {eval $script $args} message]} {
        Debug FtpEvaluate $message
    }
}

####
# Send
#
# Sends a command to the FTP control channel.
#
proc ::Ftp::Send {handle command} {
    upvar [namespace current]::$handle ftp
    Debug FtpSend "Sending command \"$command\" ($handle)."

    if {[info exists ftp]} {
        if {[catch {puts $ftp(sock) $command} message]} {
            Shutdown $handle "unable to send command - $message"
        } else {
            catch {flush $ftp(sock)}
        }
    }
}

####
# Shutdown
#
# Shuts down the FTP connection. The error parameter is an empty string
# when the connection is closed intentionally with ::Ftp::Close or ::Ftp::Disconnect.
#
proc ::Ftp::Shutdown {handle {error ""}} {
    upvar [namespace current]::$handle ftp
    Debug FtpShutdown "Connection closed ($handle): $error"

    if {[info exists ftp]} {
        # Remove channel events before closing the channel.
        catch {fileevent $ftp(sock) readable {}}
        catch {fileevent $ftp(sock) writable {}}

        # Send the QUIT command and terminate the socket.
        catch {puts $ftp(sock) "QUIT"}
        catch {flush $ftp(sock)}
        catch {close $ftp(sock)}
        set ftp(sock) ""

        # Update connection status, error message, and evaluate the notify callback.
        set ftp(status) 0
        if {$error ne ""} {
            set ftp(error) $error
            Evaluate $ftp(notify) $handle 0
        }
    }
}

####
# Verify
#
# Verifies the connection's state and begins the SSL negotiation for
# FTP servers using implicit SSL.
#
proc ::Ftp::Verify {handle} {
    upvar [namespace current]::$handle ftp
    if {![info exists ftp]} {
        Debug FtpVerify "Handle \"$handle\" does not exist."
        return
    }

    # Disable the writable channel event.
    fileevent $ftp(sock) writable {}

    set message [fconfigure $ftp(sock) -error]
    if {$message ne ""} {
        Shutdown $handle "unable to connect - $message"
        return
    }

    set peer [fconfigure $ftp(sock) -peername]
    Debug FtpVerify "Connected to [lindex $peer 0]:[lindex $peer 2] ($handle)."

    # Perform SSL negotiation for FTP servers using implicit SSL.
    # TODO: Implicit is broken.
    if {$ftp(secure) eq "implicit" && [catch {tls::import $ftp(sock) -ssl2 1 -ssl3 1 -tls1 1} message]} {
        Shutdown $handle "SSL negotiation failed - $message"
        return
    }

    # Initialise event queue.
    if {$ftp(secure) eq "ssl" || $ftp(secure) eq "tls"} {
        set ftp(queue) auth
    } else {
        set ftp(queue) user
    }

    # Set channel options and event handlers.
    fconfigure $ftp(sock) -buffering line -blocking 0 -translation {auto crlf}
    fileevent $ftp(sock) readable [list [namespace current]::Handler $handle]
}

####
# Handler
#
# FTP client event handler.
#
proc ::Ftp::Handler {handle {direct 0}} {
    upvar [namespace current]::$handle ftp
    if {![info exists ftp]} {
        Debug FtpHandler "Handle \"$handle\" does not exist."
        return
    }
    set replyCode 0
    set replyBase 0
    set buffer [list]
    set message ""

    if {[gets $ftp(sock) line] > 0} {
        #
        # Multi-line responses have a hyphen after the reply code for
        # each line until the last line is reached. For example:
        #
        # 200-blah
        # 200-blah
        # 200-blah
        # 200 Command successful.
        #
        if {[regexp -- {^(\d+)( |-)?(.*)$} $line result replyCode multi message]} {
            lappend buffer $replyCode $message
        } else {
            Debug FtpHandler "Invalid server response \"$line\"."
            set multi ""
        }

        #
        # The "STAT -al" response differs substantially, all subsequent lines
        # after the initial response do not have a reply code until the last line.
        #
        # 211-Status of .:
        # drwxrwxrwx  22 user         group               0 Oct 07 03:49 .
        # drwxrwxrwx   5 user         group               0 Apr 02 02:59 blah1
        # drwxrwxrwx  25 user         group               0 Oct 11 00:00 blah2
        # drwxrwxrwx  22 user         group               0 Sep 27 22:52 blah3
        # drwxrwxrwx  37 user         group               0 Jun 06 03:39 blah4
        # 211 End of Status
        #
        # Because of this, the line is appended to the response buffer
        # regardless of whether or not it matches the regular expression.
        #
        while {$multi eq "-" && [gets $ftp(sock) line] > 0} {
            regexp -- {^(\d+)( |-)?(.*)$} $line result replyCode multi line
            lappend buffer $replyCode $line
        }
    } elseif {[eof $ftp(sock)]} {
        # The remote server has closed the control connection.
        Shutdown $handle "server closed connection"
        return
    } elseif {!$direct} {
        # No response from the server. Return if the handler was not
        # invoked directly (i.e. not by a channel writable event).
        return
    }
    Debug FtpHandler "Reply code \"$replyCode\" and message \"$message\" ($handle)."

    #
    # Variables:
    # replyCode - Reply code (e.g. 200).
    # replyBase - Base reply code (e.g. 2).
    # buffer    - List of reply codes and text messages.
    # message   - Text from the first line (lindex $buffer 1).
    #
    set replyBase [string index $replyCode 0]

    while {[llength $ftp(queue)]} {
        set nextEvent 0

        # The first list element of an event must be its name, the
        # remaining elements are optional and vary between event types.
        set event [lindex $ftp(queue) 0]
        set eventName [lindex $event 0]

        # Pop the event from queue.
        set ftp(queue) [lrange $ftp(queue) 1 end]

        Debug FtpHandler "Event: $eventName ($handle)"
        switch -- $eventName {
            auth {
                # Receive hello response and send AUTH.
                if {$replyBase == 2} {
                    Send $handle "AUTH [string toupper $ftp(secure)]"
                    set ftp(queue) [linsert $ftp(queue) 0 auth_sent]
                } else {
                    Shutdown $handle "unable to login - $message"
                    return
                }
            }
            auth_sent {
                # Receive AUTH response and send PBSZ.
                if {$replyBase == 2} {
                    if {[catch {tls::import $ftp(sock) -ssl2 1 -ssl3 1 -tls1 1} message]} {
                        Shutdown $handle "SSL negotiation failed - $message"
                        return
                    }
                    # Set channel options again, in case the TLS module changes them.
                    fconfigure $ftp(sock) -buffering line -blocking 0 -translation {auto crlf}

                    Send $handle "PBSZ 0"
                    set ftp(queue) [linsert $ftp(queue) 0 user]
                } else {
                    Shutdown $handle "unable to login - $message"
                    return
                }
            }
            user {
                # Receive hello or PBSZ response and send USER.
                if {$replyBase == 2} {
                    Send $handle "USER $ftp(user)"
                    set ftp(queue) [linsert $ftp(queue) 0 user_sent]
                } else {
                    Shutdown $handle "unable to login - $message"
                    return
                }
            }
            user_sent {
                # Receive USER response and send PASS.
                if {$replyBase == 3} {
                    Send $handle "PASS $ftp(passwd)"
                    set ftp(queue) [linsert $ftp(queue) 0 pass_sent]
                } else {
                    Shutdown $handle "unable to login - $message"
                    return
                }
            }
            pass_sent {
                # Receive PASS response.
                if {$replyBase == 2} {
                    set ftp(status) 2
                    Evaluate $ftp(notify) $handle 1
                    set nextEvent 1
                } else {
                    Shutdown $handle "unable to login - $message"
                    return
                }
            }
            quote {
                # Send command.
                Send $handle [lindex $event 1]
                set ftp(queue) [linsert $ftp(queue) 0 [list quote_sent [lindex $event 2]]]
            }
            quote_sent {
                # Receive command.
                Evaluate [lindex $event 1] $handle $buffer
                set nextEvent 1
            }
            default {
                Debug FtpHandler "Invalid event name \"$eventName\"."
            }
        }

        # Proceed to the next event?
        if {!$nextEvent} {break}
    }
}
