################################################################################
# nxTools - Database Utilities                                                 #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} ErrorMsg]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $ErrorMsg
}

namespace eval ::nxTools::Db {
    namespace import -force ::nxLib::*
    variable dbschema
    variable dbtables

    # Table Formats
    set dbschema(Approves) 0
    set dbtables(Approves) {
        Approves {CREATE TABLE Approves(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        Release   TEXT default '')}
    }

    set dbschema(DupeDirs) 0
    set dbtables(DupeDirs) {
        DupeDirs {CREATE TABLE DupeDirs(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        DirPath   TEXT default '',
        DirName   TEXT default '')}
    }

    set dbschema(DupeFiles) 0
    set dbtables(DupeFiles) {
        DupeFiles {CREATE TABLE DupeFiles(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        FileName  TEXT default '')}
    }

    set dbschema(Links) 0
    set dbtables(Links) {
        Links {CREATE TABLE Links(
        TimeStamp INTEGER default 0,
        LinkType  INTEGER default 0,
        DirName   TEXT default '')}
    }

    set dbschema(Nukes) 2
    set dbtables(Nukes) {
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

    set dbschema(OneLines) 0
    set dbtables(OneLines) {
        OneLines {CREATE TABLE OneLines(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        Message   TEXT default '')}
    }

    set dbschema(Pres) 0
    set dbtables(Pres) {
        Pres {CREATE TABLE Pres(
        TimeStamp INTEGER default 0,
        UserName  TEXT default '',
        GroupName TEXT default '',
        Area      TEXT default '',
        Release   TEXT default '',
        Files     INTEGER default 0,
        Size      INTEGER default 0)}
    }

    set dbschema(Requests) 1
    set dbtables(Requests) {
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

proc ::nxTools::Db::Create {DbList} {
    global misc
    variable dbschema
    variable dbtables

    if {![file exists $misc(DataPath)]} {
        catch {file mkdir $misc(DataPath)}
    }
    foreach DbName $DbList {
        set DbPath [file join $misc(DataPath) ${DbName}.db]
        set DbFile [file tail $DbPath]
        LinePuts "Creating database: $DbFile"

        set Exists [file exists $DbPath]
        if {[catch {sqlite3 SqliteDb $DbPath} ErrorMsg]} {
            LinePuts " - Unable to open file: $ErrorMsg"
            continue
        }
        set CurrentVer [SqliteDb eval {PRAGMA user_version}]

        if {$Exists && $CurrentVer != $dbschema($DbName)} {
            LinePuts "- Invalid schema version (current: v$CurrentVer, required: v$dbschema($DbName))."
            SqliteDb close

            # Rename the old database to <current name>.old-v<current version>.
            set DbOld "${DbPath}.old-v$CurrentVer"
            if {[catch {file rename -- $DbPath $DbOld} ErrorMsg]} {
                LinePuts "- $ErrorMsg"
                continue
            } else {
                LinePuts "- Renamed current database to [file tail $DbOld]."
            }

            # Re-open the database to create the tables.
            if {[catch {sqlite3 SqliteDb $DbPath} ErrorMsg]} {
                LinePuts "- Unable to re-open file: $ErrorMsg"
                continue
            }
        }

        SqliteDb eval "PRAGMA user_version=$dbschema($DbName)"
        foreach {TableName TableFormat} $dbtables($DbName) {
            if {[SqliteDb eval "SELECT count(*) FROM sqlite_master WHERE name='$TableName' AND type='table'"]} {
                LinePuts "- Table $TableName: exists."
            } else {
                LinePuts "- Creating table $TableName."
                SqliteDb eval $TableFormat
            }
        }
        SqliteDb close
    }
}

proc ::nxTools::Db::Check {DbList} {
    global misc
    foreach DbName $DbList {
        set DbPath [file join $misc(DataPath) ${DbName}.db]
        LinePuts "Checking database: [file tail $DbPath]"

        if {[catch {sqlite3 SqliteDb $DbPath} ErrorMsg]} {
            LinePuts " - Unable to open file: $ErrorMsg"
            continue
        }
        set Status [SqliteDb eval {PRAGMA integrity_check}]
        LinePuts " - Status: $Status"
        SqliteDb close
    }
}

proc ::nxTools::Db::Optimize {DbList} {
    global misc
    foreach DbName $DbList {
        set DbPath [file join $misc(DataPath) ${DbName}.db]
        LinePuts "Optimizing database: [file tail $DbPath]"

        if {[catch {sqlite3 SqliteDb $DbPath} ErrorMsg]} {
            LinePuts " - Unable to open file: $ErrorMsg"
            continue
        }
        SqliteDb eval {VACUUM}
        SqliteDb close
    }
}

# Database Main
######################################################################

proc ::nxTools::Db::Main {ArgV} {
    global misc
    variable dbschema
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}

    set ArgLength [llength [set ArgList [ArgList $ArgV]]]
    set Event [string toupper [lindex $ArgList 0]]
    if {[lsearch -exact {CREATE CHECK OPTIMIZE} $Event] == -1} {
        iputs "Syntax: SITE DB CHECK \[database\]"
        iputs "        SITE DB CREATE \[database\]"
        iputs "        SITE DB OPTIMIZE \[database\]"
        return 1
    }

    iputs ".-\[DB\]-------------------------------------------------------------------."
    set DbList [lsort -ascii [array names dbschema]]
    if {$ArgLength > 1} {
        set DbArg [lindex $ArgList 1]
        set ValidName 0
        foreach DbName $DbList {
            if {[string equal -nocase $DbArg $DbName]} {
                set DbList $DbName
                set ValidName 1; break
            }
        }
        if {!$ValidName} {
            LinePuts "Invalid database name \"$DbArg\", must be:"
            ErrorReturn [JoinLiteral $DbList]
        }
    }

    set Result 0
    switch -- $Event {
        {CREATE} {
            LinePuts "Creating [llength $DbList] database(s)."; LinePuts ""
            set Result [Create $DbList]
        }
        {CHECK} {
            LinePuts "Checking [llength $DbList] database(s)."; LinePuts ""
            set Result [Check $DbList]
        }
        {OPTIMIZE} {
            LinePuts "Optimizing [llength $DbList] database(s)."; LinePuts ""
            set Result [Optimize $DbList]
        }
    }
    iputs "'------------------------------------------------------------------------'"
    return $Result
}

::nxTools::Db::Main [expr {[info exists args] ? $args : ""}]
