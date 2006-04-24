#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   Database Library
#
# Author:
#   neoxed (neoxed@gmail.com) Apr 11, 2006
#
# Abstract:
#   Implements a database abstraction layer.
#
# Procedures:
#   Db::Open        <connString> [options ...]
#   Db::Close       <handle>
#   Db::Connect     <handle>
#   Db::Disconnect  <handle>
#   Db::Exec        <handle> <statement>
#   Db::Select      <handle> <type> <statement>
#   Db::GenSQL      <handle> <script>
#   Db::Escape      <handle> <value>
#   Db::QuoteName   <handle> <value>
#   Db::QuoteString <handle> <value>
#

namespace eval ::Db {
    variable nextHandle
    variable driverMap
    if {![info exists nextHandle]} {
        set nextHandle 0
    }
    array set driverMap [list  \
        mysql       MySQL      \
        pgsql       PostgreSQL \
        postgres    PostgreSQL \
        postgresql  PostgreSQL \
        sqlite      SQLite     \
        sqlite3     SQLite     \
    ]
}

####
# Db::Open
#
# Creates a new database client handle. This handle is used by every library
# procedure and must be closed using Db::Close.
#
# Connection String:
#  <driver>://<user>:<password>@<host>:<port>/<dbname>
#
# Options:
#  -debug <callback>
#  -ping  <minutes>
#
# Example:
#  sqlite3:data/my.db
#  sqlite3:/home/user/data/my.db
#  postgres://user:pass@localhost:5432/alcoholicz
#
proc ::Db::Open {connString args} {
    variable nextHandle
    variable driverMap

    # Parse arguments.
    array set uri [ParseURI $connString]
    if {![info exists driverMap($uri(scheme))]} {
        throw DB "unknown driver \"$uri(scheme)\""
    }
    set debug ""; set ping 0

    foreach {name value} $args {
        if {$name eq "-debug"} {
            set debug $value
        } elseif {$name eq "-ping"} {
            if {![string is digit -strict $value]} {
                throw DB "expected digit but got \"$value\""
            }
            set ping $value
        } else {
            throw DB "invalid switch \"$name\": must be -debug or -ping"
        }
    }

    # Make sure the "params" element always exists.
    if {![info exists uri(params)]} {
        set uri(params) [list]
    }

    # Initialise the driver.
    set driver $driverMap($uri(scheme))
    ::Db::${driver}::Init

    set handle "db$nextHandle"
    upvar ::Db::$handle db
    #
    # Database Handle Contents
    #
    # db(debug)   - Debug logging callback.
    # db(driver)  - Driver namespace context.
    # db(object)  - Database object, used by the database extension.
    # db(options) - List of connection options (host, params, password, path, port, scheme, and user).
    # db(ping)    - How often to ping the database, in minutes.
    # db(timerId) - Timer identifier.
    #
    array set db [list          \
        debug   $debug          \
        driver  $driver         \
        object  ""              \
        options [array get uri] \
        ping    $ping           \
        timerId ""              \
    ]

    incr nextHandle
    return $handle
}

####
# Db::Close
#
# Closes and invalidates the specified handle.
#
proc ::Db::Close {handle} {
    Acquire $handle db
    if {$db(timerId) ne ""} {
        catch {killtimer $db(timerId)}
    }
    if {$db(object) ne ""} {
        Debug $db(debug) DbDisconnect "Closing the $db(driver) connection."
        ::Db::$db(driver)::Disconnect $db(object)
    }
    unset -nocomplain db
    return
}

####
# Db::Connect
#
# Connects to the database.
#
proc ::Db::Connect {handle} {
    Acquire $handle db
    if {$db(object) ne ""} {
        throw DB "already connected, disconnect first"
    }
    Debug $db(debug) DbConnect "Attempting to open a $db(driver) connection."
    set db(object) [::Db::$db(driver)::Connect $db(options)]

    if {$db(ping) > 0} {
        # Check the connection every few minutes.
        set db(timerId) [timer $db(ping) [list ::Db::Ping $handle]]
    }
    return
}

####
# Db::Disconnect
#
# Disconnects from the database.
#
proc ::Db::Disconnect {handle} {
    Acquire $handle db
    if {$db(timerId) ne ""} {
        catch {killtimer $db(timerId)}
        set db(timerId) ""
    }
    if {$db(object) ne ""} {
        Debug $db(debug) DbDisconnect "Closing the $db(driver) connection."
        ::Db::$db(driver)::Disconnect $db(object)
        set db(object) ""
    }
    return
}

####
# Db::Exec
#
# Executes the given SQL statement and returns the number of affected rows.
#
proc ::Db::Exec {handle statement} {
    Acquire $handle db
    return [::Db::$db(driver)::Exec $db(object) $statement]
}

####
# Db::Select
#
# Executes the given SQL statement and returns the results.
#
proc ::Db::Select {handle type statement} {
    Acquire $handle db
    if {$type eq "-list"} {
        return [::Db::$db(driver)::SelectList $db(object) $statement]
    } elseif {$type eq "-llist"} {
        return [::Db::$db(driver)::SelectNestedList $db(object) $statement]
    }
    throw DB "invalid type \"$type\": must be -list or -llist"
}

####
# Db::GenSQL
#
# Generates a SQL statement.
#
# Functions:
#  Like        <value> <pattern> [escape char]
#  NotLike     <value> <pattern> [escape char]
#  RegExp      <value> <pattern>
#  NotRegExp   <value> <pattern>
#  Escape      <value>
#  QuoteName   <value>
#  QuoteString <value>
#
# Example:
#  set match "a%z"
#  set input {SELECT [QuoteName group] WHERE [Like [QuoteName user] [QuoteString $match]];}
#  set query [Db::GenSQL $handle $input]
#
#  Would generate the following statement for MySQL:
#  SELECT `group` WHERE `user` LIKE 'a%z';
#
proc ::Db::GenSQL {handle script} {
    Acquire $handle db
    regsub -all -- {([^\B]\[)} $script "\\1::Db::$db(driver)::Func::" script
    return [uplevel [list subst $script]]
}

####
# Db::Escape
#
# Escapes the given value.
#
proc ::Db::Escape {handle value} {
    Acquire $handle db
    return [::Db::$db(driver)::Func::Escape $value]
}

####
# Db::QuoteName
#
# Escapes and quotes an identifier (e.g. a column or row).
#
proc ::Db::QuoteName {handle value} {
    Acquire $handle db
    return [::Db::$db(driver)::Func::QuoteName $value]
}

####
# Db::QuoteString
#
# Escapes and quotes a string constant.
#
proc ::Db::QuoteString {handle value} {
    Acquire $handle db
    return [::Db::$db(driver)::Func::QuoteString $value]
}

################################################################################
# Internal Procedures                                                          #
################################################################################

####
# Acquire
#
# Validate and acquire a database handle.
#
proc ::Db::Acquire {handle handleVar} {
    if {![regexp -- {db\d+} $handle] || ![array exists ::Db::$handle]} {
        throw DB "invalid database handle \"$handle\""
    }
    uplevel 1 [list upvar ::Db::$handle $handleVar]
}

####
# Debug
#
# Logs a debug message.
#
proc ::Db::Debug {script function message} {
    if {$script ne ""} {
        eval $script [list $function $message]
    }
}

####
# Ping
#
# Pings the server and re-connects if the connection closed.
#
proc ::Db::Ping {handle} {
    upvar ::Db::$handle db
    if {![info exists db]} {return}

    if {$db(object) eq ""} {
        set retry 1
    } elseif {![::Db::$db(driver)::Ping $db(object)]} {
        set retry 1
        Debug $db(debug) DbPing "Unable to ping server, attempting to re-connect."

        catch {::Db::$db(driver)::Disconnect $db(object)}
        set db(object) ""
    } else {
        set retry 0
    }

    if {$retry} {
        if {[catch {set db(object) [::Db::$db(driver)::Connect $db(options)]} message]} {
            Debug $db(debug) DbPing "Re-connect failed: $message"
        } else {
            Debug $db(debug) DbPing "Re-connect succeeded."
        }
    }

    # Restart the timer for the next ping interval.
    set db(timerId) [timer $db(ping) [list ::Db::Ping $handle]]
    return
}

####
# ParseTuple
#
# Parses a name and value tuple.
#
proc ::Db::ParseTuple {input separator nameVar valueVar} {
    upvar $nameVar name $valueVar value
    set index [string first $separator $input]
    if {$index != -1} {
        set name  [string range $input 0 [expr {$index - 1}]]
        set value [string range $input [expr {$index + 1}] end]
        return 1
    }
    return 0
}

####
# ParseURI
#
# Splits the given URL into its constituents.
#
proc ::Db::ParseURI {url} {
    # RFC 1738:	scheme = 1*[ lowalpha | digit | "+" | "-" | "." ]
    if {![regexp -- {^([a-z0-9+.-]+):} $url dummy uri(scheme)]} {
        throw DB "invalid URI scheme \"$url\""
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
        if {![ParseTuple $rest "/" uri(host) rest]} {
            set uri(host) $rest; set rest ""
        }
    }

    if {[info exists uri(host)]} {
        # scheme://user@host
        # scheme://user:password@host
        if {[ParseTuple $uri(host) "@" uri(user) uri(host)]} {
            ParseTuple $uri(user) ":" uri(user) uri(password)
        }
        # scheme://host:port
        ParseTuple $uri(host) ":" uri(host) uri(port)
    }

    if {![info exists uri(path)]} {
        set uri(path) "/$rest"
    }
    if {[ParseTuple $uri(path) "?" uri(path) params]} {
        foreach param [split $params "&"] {
            if {[ParseTuple $param "=" name value]} {
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
# MySQL Driver                                                                 #
################################################################################

namespace eval ::Db::MySQL {
    namespace eval Func {}
    variable params {compress encoding foundrows interactive localfiles multiresult multistatement noschema odbc socket ssl sslca sslcapath sslcert sslcipher sslkey}
}

proc ::Db::MySQL::Init {} {
    package require mysqltcl 3
}

proc ::Db::MySQL::Connect {options} {
    variable params
    array set option $options

    set connOptions [list]
    foreach {name value} $options {
        if {[lsearch -exact {host password port user} $name] != -1} {
            lappend connOptions "-$name" $value
        } elseif {$name eq "path"} {
            # Remove the leading slash, if present.
            if {[string index $value 0] eq "/"} {
                set value [string range $value 1 end]
            }
            lappend connOptions "-db" $value
        }
    }

    # Optional parameters.
    foreach {name value} $option(params) {
        if {[lsearch -exact $params $name] == -1} {
            throw DB "unsupported parameter \"$name\""
        }
        lappend connOptions "-$name" $value
    }

    return [eval mysql::connect $connOptions]
}

proc ::Db::MySQL::Disconnect {object} {
    mysql::close $object
}

proc ::Db::MySQL::Ping {object} {
    return [mysql::ping $object]
}

proc ::Db::MySQL::Exec {object statement} {
    return [mysql::exec $object $statement]
}

proc ::Db::MySQL::SelectList {object statement} {
    return [mysql::sel $object $statement -flatlist]
}

proc ::Db::MySQL::SelectNestedList {object statement} {
    return [mysql::sel $object $statement -list]
}

# Comparison Functions

proc ::Db::MySQL::Func::Like {value pattern {escape "\\"}} {
    return "$value LIKE $pattern ESCAPE '$escape'"
}

proc ::Db::MySQL::Func::NotLike {value pattern {escape "\\"}} {
    return "$value NOT LIKE $pattern ESCAPE '$escape'"
}

proc ::Db::MySQL::Func::RegExp {value pattern} {
    return "$value REGEXP $pattern"
}

proc ::Db::MySQL::Func::NotRegExp {value pattern} {
    return "$value NOT REGEXP $pattern"
}

# Quoting Functions

proc ::Db::MySQL::Func::Escape {value} {
    return [mysql::escape $value]
}

proc ::Db::MySQL::Func::QuoteName {value} {
    set value [mysql::escape $value]
    return "`$value`"
}

proc ::Db::MySQL::Func::QuoteString {value} {
    set value [mysql::escape $value]
    return "'$value'"
}

################################################################################
# PostgreSQL Driver                                                            #
################################################################################

namespace eval ::Db::PostgreSQL {
    namespace eval Func {}
    variable params {connect_timeout krbsrvname requiressl service sslmode}
}

proc ::Db::PostgreSQL::Init {} {
    package require Pgtcl 1.5
}

proc ::Db::PostgreSQL::Connect {options} {
    variable params
    array set option $options

    set connOptions [list]
    foreach {name value} $options {
        if {[lsearch -exact {host password port user} $name] != -1} {
            lappend connOptions "$name=[pg_quote $value]"
        } elseif {$name eq "path"} {
            # Remove the leading slash, if present.
            if {[string index $value 0] eq "/"} {
                set value [string range $value 1 end]
            }
            lappend connOptions "dbname=[pg_quote $value]"
        }
    }

    # Optional parameters.
    foreach {name value} $option(params) {
        if {[lsearch -exact $params $name] == -1} {
            throw DB "unsupported parameter \"$name\""
        }
        lappend connOptions "$name=[pg_quote $value]"
    }

    return [pg_connect -conninfo [join $connOptions]]
}

proc ::Db::PostgreSQL::Disconnect {object} {
    pg_disconnect $object
}

proc ::Db::PostgreSQL::Ping {object} {
    # If this query fails, it's probably safe to assume
    # that the connection to the server has closed.
    if {[catch {pg_execute $object "SELECT 1"} message]} {
        return 0
    }
    return 1
}

proc ::Db::PostgreSQL::GetResult {object statement option} {
    set handle [pg_exec $object $statement]

    # Check if the statement failed.
    if {[pg_result $handle -status] eq "PGRES_FATAL_ERROR"} {
        set result [pg_result $handle -error]
        pg_result $handle -clear
        throw DB $result
    }

    # Retrieve the desired result option.
    set result [pg_result $handle $option]
    pg_result $handle -clear
    return $result
}

proc ::Db::PostgreSQL::Exec {object statement} {
    return [GetResult $object $statement -cmdTuples]
}

proc ::Db::PostgreSQL::SelectList {object statement} {
    return [GetResult $object $statement -list]
}

proc ::Db::PostgreSQL::SelectNestedList {object statement} {
    return [GetResult $object $statement -llist]
}

# Comparison Functions

proc ::Db::PostgreSQL::Func::Like {value pattern {escape "\\"}} {
    return "$value LIKE $pattern ESCAPE '$escape'"
}

proc ::Db::PostgreSQL::Func::NotLike {value pattern {escape "\\"}} {
    return "$value NOT LIKE $pattern ESCAPE '$escape'"
}

proc ::Db::PostgreSQL::Func::RegExp {value pattern} {
    return "$value ~* $pattern"
}

proc ::Db::PostgreSQL::Func::NotRegExp {value pattern} {
    return "$value !~* $pattern"
}

# Quoting Functions

proc ::Db::PostgreSQL::Func::Escape {value} {
    return [pg_escape_string $value]
}

proc ::Db::PostgreSQL::Func::QuoteName {value} {
    set value [pg_escape_string $value]
    return "\"$value\""
}

proc ::Db::PostgreSQL::Func::QuoteString {value} {
    return [pg_quote $value]
}

################################################################################
# SQLite Driver                                                                #
################################################################################

namespace eval ::Db::SQLite::Func {}

proc ::Db::SQLite::Init {} {
    package require sqlite3
}

proc ::Db::SQLite::Connect {options} {
    array set option $options

    if {$option(path) ne ":memory:"} {
        set option(path) [file normalize $option(path)]
    }
    set object "::Db::SQLite::db[clock clicks]"
    sqlite3 $object $option(path)

    # The value and pattern arguments are passed to Tcl in reverse.
    #
    # SQL Query: 'value' REGEXP 'pattern'
    # Tcl Sees : REGEXP $pattern $value
    $object function REGEXP {regexp -nocase --}

    return $object
}

proc ::Db::SQLite::Disconnect {object} {
    $object close
}

proc ::Db::SQLite::Ping {object} {
    return 1
}

proc ::Db::SQLite::Exec {object statement} {
    $object eval $statement
    return [$object changes]
}

proc ::Db::SQLite::SelectList {object statement} {
    return [$object eval $statement]
}

proc ::Db::SQLite::SelectNestedList {object statement} {
    set result [list]
    $object eval $statement row {
        set rowList [list]
        foreach name $row(*) {lappend rowList $row($name)}
        lappend result $rowList
    }
    return $result
}

# Comparison Functions

proc ::Db::SQLite::Func::Like {value pattern {escape "\\"}} {
    return "$value LIKE $pattern ESCAPE '$escape'"
}

proc ::Db::SQLite::Func::NotLike {value pattern {escape "\\"}} {
    return "$value NOT LIKE $pattern ESCAPE '$escape'"
}

proc ::Db::SQLite::Func::RegExp {value pattern} {
    return "$value REGEXP $pattern"
}

proc ::Db::SQLite::Func::NotRegExp {value pattern} {
    return "$value NOT REGEXP $pattern"
}

# Quoting Functions

proc ::Db::SQLite::Func::Escape {value} {
    return [string map {\\ \\\\ ` \\` ' \\' \" \\\"} $value]
}

proc ::Db::SQLite::Func::QuoteName {value} {
    set value [string map {\\ \\\\ ` \\` ' \\' \" \\\"} $value]
    return "\"$value\""
}

proc ::Db::SQLite::Func::QuoteString {value} {
    set value [string map {\\ \\\\ ` \\` ' \\' \" \\\"} $value]
    return "'$value'"
}
