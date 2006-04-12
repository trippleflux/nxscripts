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
#   Db::Open
#   Db::Close       <handle>
#   Db::Connect     <handle>
#   Db::Disconnect  <handle>
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
proc ::Db::Open {connString args} {
    variable nextHandle
    variable driverMap

    # TODO: parse connString
    # <driver>://<user>:<passwd>@<host>:<port>/<dbname>
    #
    #
    #

    ::Db::${driver}::Init

    set handle "db$nextHandle"
    upvar [namespace current]::$handle db

    #
    # Database Handle Contents
    #
    # db(driver) - Driver name.
    # db(object) - Database object.
    #
    array set db [list \
        driver $driver \
        object ""      \
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
    if ($db(object) ne ""} {
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
    set db(object) [::Db::$db(driver)::Connect $db(options)]
    return
}

####
# Db::Disconnect
#
# Disconnects from the database.
#
proc ::Db::Disconnect {handle} {
    Acquire $handle db
    if ($db(object) ne ""} {
        ::Db::$db(driver)::Disconnect $db(object)
        set db(object) ""
    }
    return
}

####
# Db::GenSQL
#
# Generates a SQL statement
#
proc ::Db::GenSQL {handle statement} {
    Acquire $handle db
    regsub -all -- {([^\B]\[)} $statement "\\1::Db::$db(driver)::Func::" statement
    return [uplevel [list subst $statement]]
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
    if {![regexp -- {db\d+} $handle] || ![array exists [namespace current]::$handle]} {
        throw DB "invalid database handle \"$handle\""
    }
    uplevel 1 [list upvar [namespace current]::$handle $handleVar]
}

####
# SplitURI
#
# Splits the given URL into its constituents.
#
proc ::Db::SplitURI {url} {
    # RFC 1738:	scheme = 1*[ lowalpha | digit | "+" | "-" | "." ]
    if {![regexp -- {^([a-z0-9+.-]+):} $url result scheme]} {
        throw DB "invalid URI scheme \"$url\""
    }

    # TODO
}

################################################################################
# MySQL Driver                                                                 #
################################################################################

namespace eval ::Db::MySQL::Func {}

proc ::Db::MySQL::Init {} {
    package require mysqltcl 3
}

proc ::Db::MySQL::Connect {options} {
}

proc ::Db::MySQL::Disconnect {object} {
    ::mysql::close $object
}

proc ::Db::MySQL::Func::Escape {value} {
    return [::mysql::escape $value]
}

proc ::Db::MySQL::Func::QuoteName {value} {
    set value [::mysql::escape $value]
    return "`$value`"
}

proc ::Db::MySQL::Func::QuoteString {value} {
    set value [::mysql::escape $value]
    return "'$value'"
}

################################################################################
# PostgreSQL Driver                                                            #
################################################################################

namespace eval ::Db::PostgreSQL::Func {}

proc ::Db::PostgreSQL::Init {} {
    package require Pgtcl 1.5
}

proc ::Db::PostgreSQL::Connect {options} {
}

proc ::Db::PostgreSQL::Disconnect {object} {
    pg_disconnect $object
}

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
}

proc ::Db::SQLite::Disconnect {object} {
    $object close
}

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
