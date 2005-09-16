#
# nxTools - Dupe Checker, nuker, pre, request and utilities.
# Copyright (c) 2004-2005 neoxed
#
# Module Name:
#   Database Utilities
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements commands to maintain the databases used by nxTools.
#

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Db {
    namespace import -force ::nxLib::*
    variable dbSchema
    variable dbTables

    # Table Formats
    set dbSchema(Approves) 0
    set dbTables(Approves) {
        Approves {CREATE TABLE Approves(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        Release   TEXT default '')}
    }

    set dbSchema(DupeDirs) 0
    set dbTables(DupeDirs) {
        DupeDirs {CREATE TABLE DupeDirs(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        DirPath   TEXT default '',
        DirName   TEXT default '')}
    }

    set dbSchema(DupeFiles) 0
    set dbTables(DupeFiles) {
        DupeFiles {CREATE TABLE DupeFiles(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        FileName  TEXT default '')}
    }

    set dbSchema(Links) 0
    set dbTables(Links) {
        Links {CREATE TABLE Links(
        TimeStamp INTEGER default 0,
        LinkType  INTEGER default 0,
        DirName   TEXT default '')}
    }

    set dbSchema(Nukes) 2
    set dbTables(Nukes) {
        Nukes {CREATE TABLE Nukes(
        NukeId    INTEGER PRIMARY KEY AUTOINCREMENT,
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        Status    INTEGER default 0,
        Release   TEXT default '',
        Reason    TEXT default '',
        Multi     INTEGER default 0,
        Files     INTEGER default 0,
        Size      INTEGER default 0)}

        Users {CREATE TABLE Users(
        NukeId    INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        Amount    INTEGER default 0,
        UNIQUE    (NukeId,UserName))}
    }

    set dbSchema(OneLines) 0
    set dbTables(OneLines) {
        OneLines {CREATE TABLE OneLines(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        Message   TEXT default '')}
    }

    set dbSchema(Pres) 0
    set dbTables(Pres) {
        Pres {CREATE TABLE Pres(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        Area      TEXT default '',
        Release   TEXT default '',
        Files     INTEGER default 0,
        Size      INTEGER default 0)}
    }

    set dbSchema(Requests) 1
    set dbTables(Requests) {
        Requests {CREATE TABLE Requests(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        Status    INTEGER default 0,
        RequestId INTEGER default 0,
        Request   TEXT default '')}
    }
}

# Database Procedures
######################################################################

proc ::nxTools::Db::Create {dbList} {
    global misc
    variable dbSchema
    variable dbTables

    if {![file exists $misc(DataPath)]} {
        catch {file mkdir $misc(DataPath)}
    }
    foreach dbName $dbList {
        set filePath [file join $misc(DataPath) ${dbName}.db]
        set fileName [file tail $filePath]
        LinePuts "Creating database: $fileName"

        set exists [file exists $filePath]
        if {[catch {sqlite3 db $filePath} error]} {
            LinePuts " - Unable to open file: $error"
            continue
        }
        set currentVer [db eval {PRAGMA user_version}]

        if {$exists && $currentVer != $dbSchema($dbName)} {
            LinePuts "- Invalid schema version (current: v$currentVer, required: v$dbSchema($dbName))."
            db close

            # Rename the old database file.
            set oldPath "${filePath}.old-v$currentVer"
            if {[catch {file rename -- $filePath $oldPath} error]} {
                LinePuts "- $error"
                continue
            } else {
                LinePuts "- Renamed current database to [file tail $oldPath]."
            }

            # Re-open the database for table creation.
            if {[catch {sqlite3 db $filePath} error]} {
                LinePuts "- Unable to re-open file: $error"
                continue
            }
        }

        db eval "PRAGMA user_version=$dbSchema($dbName)"
        foreach {table query} $dbTables($dbName) {
            if {[db eval "SELECT count(*) FROM sqlite_master WHERE name='$table' AND type='table'"]} {
                LinePuts "- Table \"$table\" exists."
            } else {
                LinePuts "- Creating table \"$table\"."
                db eval $query
            }
        }
        db close
    }
}

proc ::nxTools::Db::Check {dbList} {
    global misc
    foreach dbName $dbList {
        set filePath [file join $misc(DataPath) ${dbName}.db]
        LinePuts "Checking database: [file tail $filePath]"

        if {[catch {sqlite3 db $filePath} error]} {
            LinePuts " - Unable to open file: $error"
            continue
        }
        set status [db eval {PRAGMA integrity_check}]
        LinePuts " - Status: $status"
        db close
    }
}

proc ::nxTools::Db::Optimize {dbList} {
    global misc
    foreach dbName $dbList {
        set filePath [file join $misc(DataPath) ${dbName}.db]
        LinePuts "Optimizing database: [file tail $filePath]"

        if {[catch {sqlite3 db $filePath} error]} {
            LinePuts " - Unable to open file: $error"
            continue
        }
        db eval {VACUUM}
        db close
    }
}

# Database Main
######################################################################

proc ::nxTools::Db::Main {argv} {
    global misc
    variable dbSchema
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}

    set argLength [llength [set argList [ArgList $argv]]]
    set event [string toupper [lindex $argList 0]]
    if {[lsearch -exact {CREATE CHECK OPTIMIZE} $event] == -1} {
        iputs "Syntax: SITE DB CHECK \[database\]"
        iputs "        SITE DB CREATE \[database\]"
        iputs "        SITE DB OPTIMIZE \[database\]"
        return 1
    }

    iputs ".-\[DB\]-------------------------------------------------------------------."
    set dbList [lsort -ascii [array names dbSchema]]
    set result 0

    if {$argLength > 1} {
        set arg [lindex $argList 1]
        set valid 0
        foreach dbName $dbList {
            if {[string equal -nocase $arg $dbName]} {
                set dbList [list $dbName]
                set valid 1; break
            }
        }
        if {!$valid} {
            LinePuts "Invalid database name \"$arg\", must be:"
            LinePuts [JoinLiteral $dbList "or"]
            set result 1
        }
    }

    if {!$result} {
        switch -- $event {
            CREATE {
                LinePuts "Creating [llength $dbList] database(s)."; LinePuts ""
                set result [Create $dbList]
            }
            CHECK {
                LinePuts "Checking [llength $dbList] database(s)."; LinePuts ""
                set result [Check $dbList]
            }
            OPTIMIZE {
                LinePuts "Optimizing [llength $dbList] database(s)."; LinePuts ""
                set result [Optimize $dbList]
            }
        }
    }

    iputs "'------------------------------------------------------------------------'"
    return $result
}

::nxTools::Db::Main [expr {[info exists args] ? $args : ""}]
