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
# Error Handling:
#   Errors thrown with the "DB" error code indicate user errors, e.g. using
#   an unknown parameter in the URI string. Errors thrown without the "DB"
#   error code indicate a caller implementation problem.
#
# Procedures:
#   Db::Open        <connString> [options ...]
#   Db::Close       <handle>
#   Db::GetError    <handle>
#   Db::GetStatus   <handle>
#   Db::Connect     <handle>
#   Db::Disconnect  <handle>
#   Db::Info        <handle> <option>
#   Db::Exec        <handle> <statement>
#   Db::Select      <handle> <option> <statement>
#   Db::Escape      <handle> <value>
#   Db::Pattern     <handle> <value>
#   Db::QuoteName   <handle> <value> [value ...]
#   Db::QuoteString <handle> <value> [value ...]
#
# Statement Functions:
#   Like      <value> <pattern> [escape char]
#   NotLike   <value> <pattern> [escape char]
#   Regexp    <value> <pattern>
#   NotRegexp <value> <pattern>
#   Escape    <value>
#   Name      <value> [value ...]
#   String    <value> [value ...]
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
#  -debug  <callback> - Debug logging callback.
#  -notify <callback> - Connection notification callback.
#  -ping   <minutes>  - Check the database connection.
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
    if {![info exists uri(params)]} {set uri(params) ""}
    array set option [list debug "" notify "" ping 0]
    GetOpt::Parse $args {{debug arg} {notify arg} {ping integer}} option

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
    # db(error)   - Last error message.
    # db(notify)  - Connection notification callback.
    # db(object)  - Database object, used by the database extension.
    # db(options) - List of connection options (host, params, password, path, port, scheme, and user).
    # db(ping)    - How often to ping the database, in minutes.
    # db(timerId) - Timer identifier.
    #
    array set db [list          \
        debug   $option(debug)  \
        driver  $driver         \
        error   ""              \
        notify  $option(notify) \
        object  ""              \
        options [array get uri] \
        ping    $option(ping)   \
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
    if {$db(object) ne ""} {ConnClose $handle}
    unset -nocomplain db
    return
}

####
# Db::GetError
#
# Returns the last error message.
#
proc ::Db::GetError {handle} {
    Acquire $handle db
    return $db(error)
}

####
# Db::GetStatus
#
# Returns the connection status.
#
proc ::Db::GetStatus {handle} {
    Acquire $handle db
    return [expr {$db(object) ne ""}]
}

####
# Db::Connect
#
# Connects to the database.
#
proc ::Db::Connect {handle} {
    Acquire $handle db
    if {$db(object) ne ""} {
        error "already connected, disconnect first"
    }
    if {![ConnOpen $handle] && $db(notify) eq ""} {
        # Throw an error if the current handle doesn't
        # have a connection notification callback.
        throw DB $db(error)
    }
    if {$db(ping) > 0} {
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
    if {$db(object) ne ""} {ConnClose $handle}
    return
}

####
# Db::Info
#
# Retrieves information about the database connection.
#
# Options:
#  -client    - Client version.
#  -databases - List of all databases.
#  -server    - Server version.
#  -tables    - List of tables in the current database.
#
proc ::Db::Info {handle option} {
    Acquire $handle db
    if {$db(object) eq ""} {
        throw DB "not connected"
    }
    set option [GetOpt::Element {-client -databases -server -tables} $option]
    return [Db::$db(driver)::Info $db(object) [string range $option 1 end]]
}

####
# Db::Exec
#
# Executes the given SQL statement and returns the number of affected rows.
#
proc ::Db::Exec {handle statement} {
    Acquire $handle db
    if {$db(object) eq ""} {
        throw DB "not connected"
    }
    # Generate the SQL statement.
    regsub -all -- {([^\B]\[)} $statement "\\1::Db::$db(driver)::Func::" statement
    set statement [uplevel [list subst $statement]]

    return [Db::$db(driver)::Exec $db(object) $statement]
}

####
# Db::Select
#
# Executes the given SQL statement and returns the results.
#
# Options:
#  -list  - A list of values.
#  -llist - A list of lists; the outer lists contains rows and the inner
#           list contains the values for each column of the row.
#
proc ::Db::Select {handle option statement} {
    Acquire $handle db
    if {$db(object) eq ""} {
        throw DB "not connected"
    }
    set option [GetOpt::Element {-list -llist} $option]

    # Generate the SQL statement.
    regsub -all -- {([^\B]\[)} $statement "\\1::Db::$db(driver)::Func::" statement
    set statement [uplevel [list subst $statement]]

    if {$option eq "-list"} {
        return [Db::$db(driver)::SelectList $db(object) $statement]
    } elseif {$option eq "-llist"} {
        return [Db::$db(driver)::SelectNestedList $db(object) $statement]
    }
}

####
# Db::Escape
#
# Escapes the given value.
#
proc ::Db::Escape {handle value} {
    Acquire $handle db
    return [Db::$db(driver)::Func::Escape $value]
}

####
# Db::Pattern
#
# Converts a wild-card pattern into SQL LIKE pattern.
#
proc ::Db::Pattern {handle value} {
    Acquire $handle db
    # Replace spaces and multiple stars.
    set value "*$value*"
    regsub -all -- {[\s\*]+} $value "*" value

    # Map wild-card characters to LIKE characters.
    set value [Db::$db(driver)::Func::Escape $value]
    return [string map {* % ? _} [string map {% \\% _ \\_} $value]]
}

####
# Db::QuoteName
#
# Escapes and quotes an identifier (e.g. a column or row).
#
proc ::Db::QuoteName {handle args} {
    Acquire $handle db
    return [eval Db::$db(driver)::Func::Name $args]
}

####
# Db::QuoteString
#
# Escapes and quotes a string constant.
#
proc ::Db::QuoteString {handle args} {
    Acquire $handle db
    return [eval Db::$db(driver)::Func::String $args]
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
        error "invalid database handle \"$handle\""
    }
    uplevel 1 [list upvar ::Db::$handle $handleVar]
}

####
# ConnOpen
#
# Opens a new database connection.
#
proc ::Db::ConnOpen {handle} {
    upvar ::Db::$handle db
    if {[catch {set db(object) [Db::$db(driver)::Connect $db(options)]} message]} {
        set success 0
        Debug $db(debug) DbConnect "Unable to connect: $message"
    } else {
        set success 1; set message ""
        set server [Db::$db(driver)::Info $db(object) "server"]
        Debug $db(debug) DbConnect "Successfully connected to $server."
    }
    set db(error) $message
    Evaluate $db(debug) $db(notify) $handle $success
    return $success
}

####
# ConnClose
#
# Closes an existing database connection.
#
proc ::Db::ConnClose {handle} {
    upvar ::Db::$handle db
    Debug $db(debug) DbDisconnect "Closing the $db(driver) connection."

    if {[catch {::Db::$db(driver)::Disconnect $db(object)} message]} {
        Debug $db(debug) DbDisconnect "Disconnecting from $db(driver) failed: $message"
    }
    set db(object) ""
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
# Evaluate
#
# Evaluates a callback script.
#
proc ::Db::Evaluate {debug script args} {
    if {$script ne "" && [catch {eval $script $args} message]} {
        Debug $debug DbEvaluate $message
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
    } elseif {![Db::$db(driver)::Ping $db(object)]} {
        set retry 1
        Debug $db(debug) DbPing "Unable to ping server, attempting to re-connect."
        ConnClose $handle
    } else {
        set retry 0
    }
    if {$retry} {ConnOpen $handle}

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
    variable params {compress encoding interactive socket ssl sslca sslcapath sslcert sslcipher sslkey}
}

proc ::Db::MySQL::Init {} {
    package require mysqltcl 3
}

proc ::Db::MySQL::Connect {options} {
    variable params
    array set option $options

    # MySQL does not allow multiple statements by default.
    set connOptions [list "-multistatement" 1]
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
        set name [GetOpt::Element $params $name]
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

proc ::Db::MySQL::Info {object option} {
    set result ""
    switch -- $option {
        client {
            set result "MySQL Client v"
            append result [mysql::baseinfo clientversion]
        }
        databases {
            set result [mysql::info $object databases]
        }
        server {
            set result "MySQL Server v"
            append result [mysql::info $object serverversion]
        }
        tables {
            set result [mysql::info $object tables]
        }
    }
    return $result
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
    return "$value LIKE $pattern ESCAPE [String $escape]"
}

proc ::Db::MySQL::Func::NotLike {value pattern {escape "\\"}} {
    return "$value NOT LIKE $pattern ESCAPE [String $escape]"
}

proc ::Db::MySQL::Func::Regexp {value pattern} {
    return "$value REGEXP $pattern"
}

proc ::Db::MySQL::Func::NotRegexp {value pattern} {
    return "$value NOT REGEXP $pattern"
}

# Quoting Functions

proc ::Db::MySQL::Func::Escape {value} {
    return [mysql::escape $value]
}

proc ::Db::MySQL::Func::Name {args} {
    set result [list]
    foreach value $args {
        lappend result "`[mysql::escape $value]`"
    }
    return [join $result ","]
}

proc ::Db::MySQL::Func::String {args} {
    set result [list]
    foreach value $args {
        lappend result "'[mysql::escape $value]'"
    }
    return [join $result ","]
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

    set connInfo ""
    foreach {name value} $options {
        if {[lsearch -exact {host password port user} $name] != -1} {
            append connInfo " $name=" [pg_quote $value]
        } elseif {$name eq "path"} {
            # Remove the leading slash, if present.
            if {[string index $value 0] eq "/"} {
                set value [string range $value 1 end]
            }
            append connInfo " dbname=" [pg_quote $value]
        }
    }

    # Optional parameters.
    foreach {name value} $option(params) {
        set name [GetOpt::Element $params $name]
        append connInfo " $name=" [pg_quote $value]
    }
    return [pg_connect -conninfo $connInfo]
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

proc ::Db::PostgreSQL::Info {object option} {
    set result ""
    switch -- $option {
        client {
            set result "PostgreSQL Client"
        }
        databases {
            set result [GetResult $object -list "SELECT datname FROM pg_database"]
        }
        server {
            set result "PostgreSQL Server v"
            append result [lindex [GetResult $object -list "SELECT version()"] 0]
        }
        tables {
            set result [GetResult $object -list "SELECT tablename FROM pg_tables"]
        }
    }
    return $result
}

proc ::Db::PostgreSQL::GetResult {object option statement} {
    set handle [pg_exec $object $statement]

    # Check if the statement failed.
    if {[pg_result $handle -status] eq "PGRES_FATAL_ERROR"} {
        set result [pg_result $handle -error]
        pg_result $handle -clear
        error $result
    }

    # Retrieve the desired result option.
    set result [pg_result $handle $option]
    pg_result $handle -clear
    return $result
}

proc ::Db::PostgreSQL::Exec {object statement} {
    return [GetResult $object -cmdTuples $statement]
}

proc ::Db::PostgreSQL::SelectList {object statement} {
    return [GetResult $object -list $statement]
}

proc ::Db::PostgreSQL::SelectNestedList {object statement} {
    return [GetResult $object -llist $statement]
}

# Comparison Functions

proc ::Db::PostgreSQL::Func::Like {value pattern {escape "\\"}} {
    return "$value ILIKE $pattern ESCAPE [String $escape]"
}

proc ::Db::PostgreSQL::Func::NotLike {value pattern {escape "\\"}} {
    return "$value NOT ILIKE $pattern ESCAPE [String $escape]"
}

proc ::Db::PostgreSQL::Func::Regexp {value pattern} {
    return "$value ~* $pattern"
}

proc ::Db::PostgreSQL::Func::NotRegexp {value pattern} {
    return "$value !~* $pattern"
}

# Quoting Functions

proc ::Db::PostgreSQL::Func::Escape {value} {
    return [pg_escape_string $value]
}

proc ::Db::PostgreSQL::Func::Name {args} {
    set result [list]
    foreach value $args {
        lappend result "\"[pg_escape_string $value]\""
    }
    return [join $result ","]
}

proc ::Db::PostgreSQL::Func::String {args} {
    set result [list]
    foreach value $args {lappend result [pg_quote $value]}
    return [join $result ","]
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

proc ::Db::SQLite::Info {object option} {
    set result ""
    switch -- $option {
        client - server {
            set result "SQLite v"
            append result [sqlite3 -version]
        }
        databases {
            $object eval {PRAGMA database_list} row {
                lappend result $row(name)
            }
        }
        tables {
            set result [$object eval "SELECT name FROM sqlite_master \
                WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%'"]
        }
    }
    return $result
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
    return "$value LIKE $pattern ESCAPE [String $escape]"
}

proc ::Db::SQLite::Func::NotLike {value pattern {escape "\\"}} {
    return "$value NOT LIKE $pattern ESCAPE [String $escape]"
}

proc ::Db::SQLite::Func::Regexp {value pattern} {
    return "$value REGEXP $pattern"
}

proc ::Db::SQLite::Func::NotRegexp {value pattern} {
    return "$value NOT REGEXP $pattern"
}

# Quoting Functions

proc ::Db::SQLite::Func::Escape {value} {
    return [string map {\\ \\\\ ` \\` ' \\' \" \\\"} $value]
}

proc ::Db::SQLite::Func::Name {args} {
    set result [list]
    foreach value $args {
        lappend result "\"[Escape $value]\""
    }
    return [join $result ","]
}

proc ::Db::SQLite::Func::String {args} {
    set result [list]
    foreach value $args {
        lappend result "'[Escape $value]'"
    }
    return [join $result ","]
}
