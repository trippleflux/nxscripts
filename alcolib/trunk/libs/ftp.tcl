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
# Error Handling:
#   Errors thrown with the "FTP" error code indicate user errors, e.g. trying
#   to send commands to a closed connection. Errors thrown without the "FTP"
#   error code indicate a caller implementation problem.
#
# Procedures:
#   Ftp::Open       <connString> [options ...]
#   Ftp::Change     <handle> [options ...]
#   Ftp::Close      <handle>
#   Ftp::GetError   <handle>
#   Ftp::GetStatus  <handle>
#   Ftp::Connect    <handle>
#   Ftp::Disconnect <handle>
#   Ftp::Command    <handle> <command> [callback]
#

package require alco::getopt 1.2
package require alco::uri 1.2
package require alco::util 1.2

namespace eval ::Ftp {
    variable nextHandle
    variable schemeMap
    if {![info exists nextHandle]} {
        set nextHandle 0
    }
    array set schemeMap [list \
        ftp    "off"          \
        ftps   "implicit"     \
        ftpssl "ssl"          \
        ftptls "tls"          \
    ]
}

####
# Ftp::Open
#
# Creates a new FTP client handle. This handle is used by every library
# procedure and must be closed using Ftp::Close.
#
# Connection String:
#  <scheme>://<username>:<password>@<host>:<port>
#
# Options:
#  -debug   <callback> - Debug logging callback.
#  -notify  <callback> - Connection notification callback.
#  -retries <count>    - Number of attempts while waiting for a complete reply.
#  -sleep   <msecs>    - Milliseconds to sleep after each attempt.
#
proc ::Ftp::Open {connString args} {
    variable nextHandle
    variable schemeMap

    # Set default values.
    array set uri [list user "anonymous" password "anonymous" host "localhost" port 21]
    array set option [list debug "" notify "" retries 100 sleep 100]

    # Parse arguments.
    array set uri [Uri::Parse $connString]
    if {![info exists schemeMap($uri(scheme))]} {
        throw FTP "unknown scheme \"$uri(scheme)\""
    }
    GetOpt::Parse $args {{debug arg} {notify arg} {retries integer} {sleep integer}} option

    # Load the TLS extension.
    set secure $schemeMap($uri(scheme))
    if {$secure ne "off" && [catch {package require tls 1.5} message]} {
        throw FTP "SSL/TLS not available: $message"
    }

    set handle "ftp$nextHandle"
    upvar ::Ftp::$handle ftp
    #
    # FTP Handle Contents
    #
    # ftp(secure)   - Security method.
    # ftp(user)     - Client user name.
    # ftp(password) - Client password.
    # ftp(host)     - Remote server host.
    # ftp(port)     - Remote server port.
    #
    # ftp(debug)    - Debug logging callback.
    # ftp(notify)   - Connection notification callback.
    # ftp(retries)  - Number of attempts while waiting for a complete reply.
    # ftp(sleep)    - Milliseconds to sleep after each attempt.
    #
    # ftp(error)    - Last error message.
    # ftp(queue)    - Event queue (FIFO).
    # ftp(sock)     - Socket channel.
    # ftp(status)   - Connection status (0=disconnected, 1=connecting, 2=connected).
    #
    array set ftp [list           \
        secure   $secure          \
        user     $uri(user)       \
        password $uri(password)   \
        host     $uri(host)       \
        port     $uri(port)       \
        debug    $option(debug)   \
        notify   $option(notify)  \
        retries  $option(retries) \
        sleep    $option(sleep)   \
        error    ""               \
        queue    [list]           \
        sock     ""               \
        status   0                \
    ]

    incr nextHandle
    return $handle
}

####
# Ftp::Change
#
# Retrieve and modify options for a given FTP handle.
#
proc ::Ftp::Change {handle args} {
    Acquire $handle ftp
    set options {-debug -host -notify -password -port -retries -secure -sleep -user}

    # Retrieve all options.
    if {[llength $args] == 0} {
        set result [list]
        foreach option $options {
            lappend result $option $ftp([string range $option 1 end])
        }
        return $result
    }

    # Retrieve only the specified option.
    if {[llength $args] == 1} {
        set option [GetOpt::Element $options [lindex $args 0]]
        return $ftp([string range $option 1 end])
    }

    # Modify options.
    GetOpt::Parse $args {{debug arg} {host arg} {notify arg} {password arg} \
        {port integer} {retries integer} {secure arg {implicit off ssl tls}} \
        {sleep integer} {user arg}} option
    if {[info exists option(secure)] && $option(secure) ne "off" && [catch {package require tls 1.5} message]} {
        throw FTP "SSL/TLS not available: $message"
    }
    array set ftp [array get option]
    return
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
        error "already connected, disconnect first"
    }
    set ftp(error) ""
    set ftp(status) 1

    # Asynchronous sockets in Tcl are created immediately but may not be
    # connected yet. The writable channel event callback is executed when
    # the socket is connected or if the connection failed.
    set ftp(sock) [socket -async $ftp(host) $ftp(port)]

    fileevent $ftp(sock) writable [list ::Ftp::Verify $handle]
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

    # Queue the command event
    set event [list quote $command $callback]
    set queue [lappend ftp(queue) $event]

    # If the queue has only one event, invoke the handler directly.
    if {[llength $queue] == 1} {
        Handler $handle
    }
    return
}

################################################################################
# Internal Procedures                                                          #
################################################################################

####
# Acquire
#
# Validate and acquire a FTP handle.
#
proc ::Ftp::Acquire {handle handleVar} {
    if {![regexp -- {^ftp\d+$} $handle] || ![array exists ::Ftp::$handle]} {
        error "invalid ftp handle \"$handle\""
    }
    uplevel 1 [list upvar ::Ftp::$handle $handleVar]
}

####
# Debug
#
# Logs a debug message.
#
proc ::Ftp::Debug {script function message} {
    if {$script ne ""} {
        eval $script [list $function $message]
    }
}

####
# Evaluate
#
# Evaluates a callback script.
#
proc ::Ftp::Evaluate {debug script args} {
    if {$script ne "" && [catch {eval $script $args} message]} {
        Debug $debug FtpEvaluate $message
    }
}

####
# Send
#
# Sends a command to the FTP control channel.
#
proc ::Ftp::Send {handle command} {
    upvar ::Ftp::$handle ftp
    Debug $ftp(debug) FtpSend "Sending command \"$command\"."

    if {[catch {puts $ftp(sock) $command} message]} {
        Shutdown $handle "unable to send command - $message"
    } else {
        catch {flush $ftp(sock)}
    }
}

####
# Shutdown
#
# Shuts down the FTP connection. The error parameter is an empty string
# when the connection is closed intentionally with Ftp::Close or Ftp::Disconnect.
#
proc ::Ftp::Shutdown {handle {error ""}} {
    upvar ::Ftp::$handle ftp

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
        Debug $ftp(debug) FtpShutdown "Connection closed: $error"
        Evaluate $ftp(debug) $ftp(notify) $handle 0
    }
}

####
# Verify
#
# Verifies the connection's state and begins the SSL negotiation for
# FTP servers using implicit SSL.
#
proc ::Ftp::Verify {handle} {
    upvar ::Ftp::$handle ftp
    if {![info exists ftp]} {return}

    # Disable the writable channel event.
    fileevent $ftp(sock) writable {}

    set message [fconfigure $ftp(sock) -error]
    if {$message ne ""} {
        Shutdown $handle "unable to connect - $message"
        return
    }

    set peer [fconfigure $ftp(sock) -peername]
    Debug $ftp(debug) FtpVerify "Connected to [lindex $peer 0]:[lindex $peer 2]."

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
    fileevent $ftp(sock) readable [list ::Ftp::Handler $handle 1]
}

####
# Handler
#
# FTP client event handler.
#
proc ::Ftp::Handler {handle {isFileEvent 0}} {
    upvar ::Ftp::$handle ftp
    if {![info exists ftp] || $ftp(sock) eq ""} {return 0}

    set replyCode 0
    set replyBase 0
    set buffer [list]
    set message ""

    if {[gets $ftp(sock) line] > 0} {
        # Multi-line responses have a hyphen after the reply code for
        # each line until the last line is reached. For example:
        # 200-blah
        # 200-blah
        # 200 Command successful.
        if {![regexp -- {^(\d+)( |-)?(.*)$} $line result replyCode multi message]} {
            Debug $ftp(debug) FtpHandler "Invalid server response \"$line\"."
        } else {
            lappend buffer $replyCode $message

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
            set attempts 0
            while {$multi eq "-" && $attempts < $ftp(retries)} {
                if {[gets $ftp(sock) line] > 0} {
                    regexp -- {^(\d+)( |-)?(.*)$} $line result replyCode multi line
                    lappend buffer $replyCode $line
                    set attempts 0
                } else {
                    incr attempts
                    Debug $ftp(debug) FtpHandler "No response in $attempts/$ftp(retries), sleeping for $ftp(sleep)ms."
                    after $ftp(sleep)
                }
            }
        }
    } elseif {[eof $ftp(sock)]} {
        # The remote server has closed the control connection.
        Shutdown $handle "server closed connection"
        return 0
    } elseif {$isFileEvent} {
        Debug $ftp(debug) FtpHandler "No response from server, returning."
        return 0
    }
    Debug $ftp(debug) FtpHandler "Reply code \"$replyCode\" and message \"$message\"."

    #
    # Variables:
    # replyCode - Reply code (e.g. 200).
    # replyBase - Base reply code (e.g. 2).
    # buffer    - Paired list of reply codes and messages.
    # message   - Message from the first buffer line (lindex $buffer 1).
    #
    set replyBase [string index $replyCode 0]

    if {![llength $ftp(queue)]} {
        Debug $ftp(debug) FtpHandler "Empty event queue, returning."
        return 0
    }
    set nextEvent 0

    # The first list element of an event must be its name, the
    # remaining elements are optional and vary between event types.
    set event [lindex $ftp(queue) 0]
    set eventName [lindex $event 0]

    # Pop the event from queue.
    set ftp(queue) [lreplace $ftp(queue) 0 0]

    Debug $ftp(debug) FtpHandler "Event: $eventName"
    switch -- $eventName {
        auth {
            # Receive hello response and send AUTH.
            if {$replyBase == 2} {
                Send $handle "AUTH [string toupper $ftp(secure)]"
                set ftp(queue) [linsert $ftp(queue) 0 auth_sent]
            } else {
                Shutdown $handle "unable to login - $message"
                return 0
            }
        }
        auth_sent {
            # Receive AUTH response and send PBSZ.
            if {$replyBase == 2} {
                if {[catch {tls::import $ftp(sock) -ssl2 1 -ssl3 1 -tls1 1} message]} {
                    Shutdown $handle "SSL negotiation failed - $message"
                    return 0
                }
                # Set channel options again, in case the TLS module changes them.
                fconfigure $ftp(sock) -buffering line -blocking 0 -translation {auto crlf}

                Send $handle "PBSZ 0"
                set ftp(queue) [linsert $ftp(queue) 0 user]
            } else {
                Shutdown $handle "unable to login - $message"
                return 0
            }
        }
        user {
            # Receive hello or PBSZ response and send USER.
            if {$replyBase == 2} {
                Send $handle "USER $ftp(user)"
                set ftp(queue) [linsert $ftp(queue) 0 user_sent]
            } else {
                Shutdown $handle "unable to login - $message"
                return 0
            }
        }
        user_sent {
            # Receive USER response and send PASS.
            if {$replyBase == 3} {
                Send $handle "PASS $ftp(password)"
                set ftp(queue) [linsert $ftp(queue) 0 pass_sent]
            } else {
                Shutdown $handle "unable to login - $message"
                return 0
            }
        }
        pass_sent {
            # Receive PASS response.
            if {$replyBase == 2} {
                set ftp(status) 2
                Evaluate $ftp(debug) $ftp(notify) $handle 1
                set nextEvent 1
            } else {
                Shutdown $handle "unable to login - $message"
                return 0
            }
        }
        quote {
            # Send command.
            Send $handle [lindex $event 1]
            set ftp(queue) [linsert $ftp(queue) 0 [list quote_sent [lindex $event 2]]]
        }
        quote_sent {
            # Receive command.
            Evaluate $ftp(debug) [lindex $event 1] $handle $buffer
            set nextEvent 1
        }
        default {
            Debug $ftp(debug) FtpHandler "Unknown event name \"$eventName\"."
        }
    }

    if {$nextEvent} {
        # Process the remaining events.
        while {[llength $ftp(queue)]} {
            if {[Handler $handle]} {
                Debug $ftp(debug) FtpHandler "Successfully processed queued event."
            } else {
                Debug $ftp(debug) FtpHandler "Unable to process queued event."
            }
        }
    }
    return 1
}

package provide alco::ftp 1.2.0
