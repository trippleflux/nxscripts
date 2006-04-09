#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
#
# Module Name:
#   nxTools Module
#
# Author:
#   neoxed (neoxed@gmail.com) Aug 30, 2005
#
# Abstract:
#   Implements a module to interact with nxTools' databases.
#

namespace eval ::Bot::Mod::NxTools {
    if {![info exists [namespace current]::cmdTokens]} {
        variable cmdTokens [list]
        variable dataPath ""
        variable undupeChars 5
        variable undupeWild 0
    }
    namespace import -force ::Bot::*
    namespace import -force ::Bot::Mod::FtpConn::GetFtpConnection
    namespace import -force ::Bot::Mod::Invite::GetFtpUser
}

####
# DbOpenFile
#
# Open a nxTools SQLite database file.
#
proc ::Bot::Mod::NxTools::DbOpenFile {fileName} {
    variable dataPath

    set filePath [file join $dataPath $fileName]
    if {![file isfile $filePath]} {
        LogError ModNxTools "Unable to open \"$filePath\": the file does not exist"
        return 0
    }
    if {[catch {sqlite3 [namespace current]::db $filePath} message]} {
        LogError ModNxTools "Unable to open \"$filePath\": $message"
        return 0
    }

    db busy [namespace current]::DbBusyHandler
    return 1
}

####
# DbBusyHandler
#
# Callback invoked by SQLite if it tries to open a locked database.
#
proc ::Bot::Mod::NxTools::DbBusyHandler {tries} {
    # Give up after 50 attempts, although it should succeed after 1-5.
    if {$tries > 50} {return 1}
    after 200
    return 0
}

####
# Dupe
#
# Search for a release, command: !dupe [-limit <num>] [-section <name>] <pattern>.
#
proc ::Bot::Mod::NxTools::Dupe {target user host channel argv} {
    upvar ::Bot::pathSections pathSections

    # Parse command options.
    set option(limit) -1
    set optList [list {limit integer} [list section arg [lsort [array names pathSections]]]]
    set pattern [join [GetOpt::Parse $argv $optList option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    set limit [GetResultLimit $option(limit)]

    if {[info exists option(section)]} {
        set section $option(section)
        set matchPath [SqlToLike [lindex $pathSections($section) 0]]
        set sectionQuery "AND DirPath LIKE '${matchPath}%' ESCAPE '\\'"
    } else {
        set sectionQuery ""
    }
    SendTargetTheme $target Module::NxTools dupeHead [list $pattern]

    set count 0
    if {[DbOpenFile "DupeDirs.db"]} {
        db eval "SELECT * FROM DupeDirs WHERE DirName LIKE '[SqlGetPattern $pattern]' ESCAPE '\\' \
                $sectionQuery ORDER BY TimeStamp DESC LIMIT $limit" values {
            # Retrieve the section name.
            set virtualPath [file join $values(DirPath) $values(DirName)]
            if {$sectionQuery eq "" && [set section [GetSectionFromPath $virtualPath]] eq ""} {
                set section $::Bot::defaultSection
            }

            incr count
            set age [expr {[clock seconds] - $values(TimeStamp)}]
            SendTargetTheme $target Module::NxTools dupeBody [list $values(UserName) $values(GroupName) \
                $section $virtualPath $values(TimeStamp) $age $count]
        }
        db close
    }

    if {!$count} {SendTargetTheme $target Module::NxTools dupeNone [list $pattern]}
    SendTargetTheme $target Module::NxTools dupeFoot
}

####
# New
#
# Display recent releases, command: !new [-limit <num>] [section].
#
proc ::Bot::Mod::NxTools::New {target user host channel argv} {
    upvar ::Bot::pathSections pathSections

    # Parse command options.
    set option(limit) -1
    set section [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    if {$section eq ""} {
        set sectionQuery ""
    } else {
        # Validate the specified section name.
        set names [lsort [array names pathSections]]
        set section [GetOpt::Element $names $section section]

        set matchPath [SqlToLike [lindex $pathSections($section) 0]]
        set sectionQuery "WHERE DirPath LIKE '${matchPath}%' ESCAPE '\\'"
    }
    SendTargetTheme $target Module::NxTools newHead

    set count 0
    if {[DbOpenFile "DupeDirs.db"]} {
        db eval "SELECT * FROM DupeDirs $sectionQuery ORDER BY TimeStamp DESC LIMIT $limit" values {
            # Retrieve the section name.
            set virtualPath [file join $values(DirPath) $values(DirName)]
            if {$sectionQuery eq "" && [set section [GetSectionFromPath $virtualPath]] eq ""} {
                set section $::Bot::defaultSection
            }

            incr count
            set age [expr {[clock seconds] - $values(TimeStamp)}]
            SendTargetTheme $target Module::NxTools newBody [list $values(UserName) $values(GroupName) \
                $section $virtualPath $values(TimeStamp) $age $count]
        }
        db close
    }

    if {!$count} {SendTargetTheme $target Module::NxTools newNone}
    SendTargetTheme $target Module::NxTools newFoot
}

####
# Undupe
#
# Remove a file or directory from the dupe database, command: !undupe [-directory] <pattern>.
#
proc ::Bot::Mod::NxTools::Undupe {target user host channel argv} {
    variable undupeChars
    variable undupeWild

    # Parse command options.
    set pattern [join [GetOpt::Parse $argv {directory} option]]
    if {$pattern eq ""} {
        throw CMDHELP "you must specify a pattern"
    }
    if {[string first "?" $pattern] != -1 || [string first "*" $pattern] != -1 } {
        if {!$undupeWild} {
            throw CMDHELP "wildcards are not allowed"
        }
        if {[regexp -all -- {[[:alnum:]]} $pattern] < $undupeChars} {
            throw CMDHELP "you must specify at least $undupeChars alphanumeric chars with wildcards"
        }
    }
    SendTargetTheme $target Module::NxTools undupeHead [list $pattern]

    if {[info exists option(directory)]} {
        set colName "DirName"
        set tableName "DupeDirs"
    } else {
        set colName "FileName"
        set tableName "DupeFiles"
    }

    set count 0
    if {[DbOpenFile "${tableName}.db"]} {
        set rowIds [list]
        db eval "SELECT $colName,rowid FROM $tableName WHERE $colName \
                LIKE '[SqlToLike $pattern]' ESCAPE '\\' ORDER BY $colName ASC" values {
            incr count
            SendTargetTheme $target Module::NxTools undupeBody [list $values($colName) $count]
            lappend rowIds $values(rowid)
        }
        if {[llength $rowIds]} {
            db eval "DELETE FROM $tableName WHERE rowid IN ([join $rowIds ,])"
        }
        db close
    }

    if {!$count} {SendTargetTheme $target Module::NxTools undupeNone [list $pattern]}
    SendTargetTheme $target Module::NxTools undupeFoot
}

####
# Nukes
#
# Display recent nukes, command: !nukes [-limit <num>] [pattern].
#
proc ::Bot::Mod::NxTools::Nukes {target user host channel argv} {
    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    if {$pattern eq ""} {
        set matchQuery ""
    } else {
        set matchQuery "AND Release LIKE '[SqlGetPattern $pattern]' ESCAPE '\\'"
    }
    SendTargetTheme $target Module::NxTools nukesHead

    set count 0
    if {[DbOpenFile "Nukes.db"]} {
        db eval "SELECT * FROM Nukes WHERE Status=0 $matchQuery \
                ORDER BY TimeStamp DESC LIMIT $limit" values {
            incr count
            set age [expr {[clock seconds] - $values(TimeStamp)}]
            SendTargetTheme $target Module::NxTools nukesBody [list $values(UserName) $values(GroupName) \
                $values(Release) $values(TimeStamp) $values(Multi) $values(Reason) \
                $values(Files) $values(Size) $age $count]
        }
        db close
    }

    if {!$count} {SendTargetTheme $target Module::NxTools nukesNone}
    SendTargetTheme $target Module::NxTools nukesFoot
}

####
# NukeTop
#
# Display top nuked users, command: !nukes [-limit <num>] [group].
#
proc ::Bot::Mod::NxTools::NukeTop {target user host channel argv} {
    # Parse command options.
    set option(limit) -1
    set group [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    if {$group eq ""} {
        set groupQuery ""
    } else {
        set groupQuery "GroupName='[SqlEscape $group]' AND"
    }
    SendTargetTheme $target Module::NxTools nuketopHead

    set count 0
    if {[DbOpenFile "Nukes.db"]} {
        db eval "SELECT UserName, GroupName, count(*) AS Nuked, sum(Amount) AS Credits FROM Users \
                WHERE $groupQuery (SELECT count(*) FROM Nukes WHERE NukeId=Users.NukeId AND Status=0) \
                GROUP BY UserName ORDER BY Nuked DESC LIMIT $limit" values {
            incr count
            SendTargetTheme $target Module::NxTools nuketopBody [list $values(UserName) \
                $values(GroupName) $values(Credits) $values(Nuked) $count]
        }
        db close
    }

    if {!$count} {SendTargetTheme $target Module::NxTools nuketopNone}
    SendTargetTheme $target Module::NxTools nuketopFoot
}

####
# Unnukes
#
# Display recent unnukes, command: !unnukes [-limit <num>] [pattern].
#
proc ::Bot::Mod::NxTools::Unnukes {target user host channel argv} {
    # Parse command options.
    set option(limit) -1
    set pattern [join [GetOpt::Parse $argv {{limit integer}} option]]
    set limit [GetResultLimit $option(limit)]

    if {$pattern eq ""} {
        set matchQuery ""
    } else {
        set matchQuery "AND Release LIKE '[SqlGetPattern $pattern]' ESCAPE '\\'"
    }
    SendTargetTheme $target Module::NxTools unnukesHead

    set count 0
    if {[DbOpenFile "Nukes.db"]} {
        db eval "SELECT * FROM Nukes WHERE Status=1 $matchQuery \
                ORDER BY TimeStamp DESC LIMIT $limit" values {
            incr count
            set age [expr {[clock seconds] - $values(TimeStamp)}]
            SendTargetTheme $target Module::NxTools unnukesBody [list $values(UserName) $values(GroupName) \
                $values(Release) $values(TimeStamp) $values(Multi) $values(Reason) \
                $values(Files) $values(Size) $age $count]
        }
        db close
    }

    if {!$count} {SendTargetTheme $target Module::NxTools unnukesNone}
    SendTargetTheme $target Module::NxTools unnukesFoot
}

####
# Approved
#
# Display approved releases, command: !approved.
#
proc ::Bot::Mod::NxTools::Approved {target user host channel argv} {
    SendTargetTheme $target Module::NxTools approveHead

    set count 0
    if {[DbOpenFile "Approves.db"]} {
        db eval {SELECT * FROM Approves ORDER BY Release ASC} values {
            incr count
            set age [expr {[clock seconds] - $values(TimeStamp)}]
            SendTargetTheme $target Module::NxTools approveBody [list $values(UserName) \
                $values(GroupName) $values(Release) $age $count]
        }
        db close
    }

    if {!$count} {SendTargetTheme $target Module::NxTools approveNone}
    SendTargetTheme $target Module::NxTools approveFoot
}

####
# OneLines
#
# Display recent one-lines, command: !onel [-limit <num>].
#
proc ::Bot::Mod::NxTools::OneLines {target user host channel argv} {
    # Parse command options.
    set option(limit) -1
    GetOpt::Parse $argv {{limit integer}} option
    set limit [GetResultLimit $option(limit)]

    SendTargetTheme $target Module::NxTools oneLinesHead
    set count 0
    if {[DbOpenFile "OneLines.db"]} {
        db eval {SELECT * FROM OneLines ORDER BY TimeStamp DESC LIMIT $limit} values {
            incr count
            set age [expr {[clock seconds] - $values(TimeStamp)}]
            SendTargetTheme $target Module::NxTools oneLinesBody [list $values(UserName) \
                $values(GroupName) $values(Message) $values(TimeStamp) $age $count]
        }
        db close
    }

    if {!$count} {SendTargetTheme $target Module::NxTools oneLinesNone}
    SendTargetTheme $target Module::NxTools oneLinesFoot
}

####
# Requests
#
# Display current requests, command: !requests.
#
proc ::Bot::Mod::NxTools::Requests {target user host channel argv} {
    SendTargetTheme $target Module::NxTools requestsHead

    set count 0
    if {[DbOpenFile "Requests.db"]} {
        db eval {SELECT * FROM Requests WHERE Status=0 ORDER BY RequestId DESC} values {
            incr count
            set age [expr {[clock seconds] - $values(TimeStamp)}]
            SendTargetTheme $target Module::NxTools requestsBody [list $values(UserName) \
                $values(GroupName) $values(Request) $values(RequestId) $age $count]
        }
        db close
    }

    if {!$count} {SendTargetTheme $target Module::NxTools requestsNone}
    SendTargetTheme $target Module::NxTools requestsFoot
}

####
# SiteCmd
#
# Send SITE commands to the FTP server and display the response.
#
proc ::Bot::Mod::NxTools::SiteCmd {event target user host channel argv} {
    if {[llength $argv] != 1} {
        throw CMDHELP
    }
    switch -- $event {
        APPROVE {
            set command "SITE APPROVE BOT ADD"
            set theme "approveAdd"
        }
        REQADD {
            set command "SITE REQBOT ADD"
            set theme "requestAdd"
        }
        REQDEL {
            set command "SITE REQBOT DEL"
            set theme "requestDel"
        }
        REQFILL {
            set command "SITE REQBOT FILL"
            set theme "requestFill"
        }
        default {
            LogError ModNxTools "Unknown status event \"$event\"."
            return
        }
    }
    if {[catch {set ftpUser [GetFtpUser $user]} message]} {
        SendTargetTheme $target Module::NxTools $theme [list $message]
        return
    }
    append command " $ftpUser \"[lindex $argv 0]\""

    # Send the SITE command.
    set connection [GetFtpConnection]
    if {[Ftp::GetStatus $connection] == 2} {
        Ftp::Command $connection $command [list [namespace current]::SiteCallback $target $theme]
    } else {
        SendTargetTheme $target Module::NxTools $theme [list "Not connected to the FTP server."]
    }
}

####
# SiteCallback
#
# SITE command callback, display the server's response.
#
proc ::Bot::Mod::NxTools::SiteCallback {target theme connection response} {
    # Ignore the header, foot, and the "command successful" message.
    foreach {code message} [lrange $response 2 end-4] {
        set message [string trim $message "| \t"]
        SendTargetTheme $target Module::NxTools $theme [list $message]
    }
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::Bot::Mod::NxTools::Load {firstLoad} {
    variable cmdTokens
    variable dataPath
    variable undupeChars
    variable undupeWild
    upvar ::Bot::configHandle configHandle

    if {$firstLoad} {
        package require sqlite3
    }

    # Retrieve configuration options.
    foreach option {dataPath undupeChars undupeWild} {
        set $option [Config::Get $configHandle Module::NxTools $option]
    }
    if {![file isdirectory $dataPath]} {
        error "The database directory \"$dataPath\" does not exist."
    }
    set undupeWild [IsTrue $undupeWild]

    # Directory commands.
    set cmdTokens [list]

    lappend cmdTokens [CmdCreate channel dupe [namespace current]::Dupe \
        -args "\[-limit <num>\] \[-section <name>\] <pattern>" \
        -category "Data" -desc "Search for a release."]

    lappend cmdTokens [CmdCreate channel new [namespace current]::New \
        -args "\[-limit <num>\] \[section\]" \
        -category "Data" -desc "Display new releases."]

    lappend cmdTokens [CmdCreate channel undupe [namespace current]::Undupe \
        -args "\[-directory\] <pattern>" \
        -category "Data" -desc "Undupe files and directories."]

    # Nuke commands.
    lappend cmdTokens [CmdCreate channel nukes [namespace current]::Nukes \
        -args "\[-limit <num>\] \[pattern\]" \
        -category "Data" -desc "Display recent nukes."]

    lappend cmdTokens [CmdCreate channel nuketop [namespace current]::NukeTop \
        -args "\[-limit <num>\] \[group\]" \
        -category "Data" -desc "Display top nuked users."]

    lappend cmdTokens [CmdCreate channel unnukes [namespace current]::Unnukes \
        -args "\[-limit <num>\] \[pattern\]" \
        -category "Data" -desc "Display recent unnukes."]

    # Request commands.
    lappend cmdTokens [CmdCreate channel requests [namespace current]::Requests \
        -category "Request" -desc "Display current requests."]

    lappend cmdTokens [CmdCreate channel request [list [namespace current]::SiteCmd REQADD] \
        -args "<request/id>" -category "Request" -desc "Add a request."]

    lappend cmdTokens [CmdCreate channel reqdel [list [namespace current]::SiteCmd REQDEL] \
        -args "<request/id>" -category "Request" -desc "Remove a request."]

    lappend cmdTokens [CmdCreate channel reqfill [list [namespace current]::SiteCmd REQFILL] \
        -args "<request/id>" -category "Request" -desc "Mark a request as filled."]

    # Other commands.
    lappend cmdTokens [CmdCreate channel approve [list [namespace current]::SiteCmd APPROVE] \
        -args "<release>" -category "General" -desc "Approve a release."]

    lappend cmdTokens [CmdCreate channel approved [namespace current]::Approved \
        -category "General" -desc "Display approved releases."]

    lappend cmdTokens [CmdCreate channel onel [namespace current]::OneLines \
        -args "\[-limit <num>\]" -category "General" -desc "Display recent one-lines."]
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::Bot::Mod::NxTools::Unload {} {
    variable cmdTokens
    foreach token $cmdTokens {
        CmdRemoveByToken $token
    }
}
