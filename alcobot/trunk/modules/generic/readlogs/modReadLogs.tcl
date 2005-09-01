#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Log Reading Module
#
# Author:
#   neoxed (neoxed@gmail.com) Auguest 22, 2005
#
# Abstract:
#   Implements a module to manage affiliated and banned groups.
#

namespace eval ::alcoholicz::ReadLogs {
    if {![info exists logList]} {
        variable excludePaths ""
        variable logCount 0
        variable logList [list]
        variable timerId ""
    }
    namespace import -force ::alcoholicz::*
}

####
# AddLog
#
# Add a log file to the monitoring list.
#
proc ::alcoholicz::ReadLogs::AddLog {logType logFile} {
    variable logCount
    variable logList
    variable logOffset

    # Sanity checks.
    switch -- $logType {
        main  {set logType 0}
        error {set logType 1}
        login {set logType 2}
        sysop {set logType 3}
        default {error "unknown log type \"logType\""}
    }

    if {![file isfile $logFile] || ![file readable $logFile]} {
        error "file not readable \"$logFile\""
    }

    set logOffset($logCount) [file size $logFile]
    lappend logList $logCount $logType [file normalize $logFile]

    incr logCount
    return
}

####
# IsPathExcluded
#
# Check if a given path is excluded from announcing.
#
proc ::alcoholicz::ReadLogs::IsPathExcluded {path} {
    variable excludePaths
    foreach pattern $excludePaths {
        if {[string match -nocase $pattern $path]} {return 1}
    }
    return 0
}

####
# ParseLogin
#
# Parse glFTPD login.log entries.
#
proc ::alcoholicz::ReadLogs::ParseLogin {line eventVar dataVar} {
    upvar $eventVar event $dataVar data

    # Note: In some glFTPD versions there is an extra space before
    # the host in BADPASSWORD, a typo by their developers I suppose.
    array set reLogin {
        LOGIN       {LOGOUT: (\S+)@(\S+) \((\S+)\) "(\S+)" "(\S+)" "(.+)"$}
        LOGOUT      {LOGIN: (\S+)@(\S+) \((\S+)\) \S+ "(\S+)" "(\S+)" "(.+)"$}
        TIMEOUT     {TIMEOUT: (\S+) \((\S+)@(\S+)\) timed out after being idle (\d+) seconds\.$}
        KILLGHOST   {^'(\S+)' killed a ghost with PID (\d+)\.$}
        BADHOSTMASK {^(\S+): (\S+)@(\S+) \((\S+)\): Bad user@host\.$}
        BANNEDHOST  {^(\S+): (\S+)@(\S+) \((\S+)\): Banned user@host\.$}
        DELETED     {^(\S+): (\S+)@(\S+) \((\S+)\): Deleted\.$}
        BADPASSWORD {^(\S+): (\S+)@(\S+) \s?\((\S+)\): (?:Login failure|Repeated login failures)\.$}
        UNKNOWNHOST {^(\S+)@(\S+) \((\S+)\): connection refused: ident@ip not added to any users\.}
    }
    foreach event [array names reLogin] {
        set result [regexp -inline -- $reLogin($event) $line]
        if {[llength $result]} {
            set data [lrange $result 1 end]
            return 1
        }
    }
    return 0
}

####
# ParseSysop
#
# Parse ioFTPD and glFTPD sysop.log entries.
#
proc ::alcoholicz::ReadLogs::ParseSysop {line eventVar dataVar} {
    variable reSysop
    upvar $eventVar event $dataVar data

    foreach event [array names reSysop] {
        set result [regexp -inline -- $reSysop($event) $line]
        if {[llength $result]} {
            set data [lrange $result 1 end]
            return 1
        }
    }
    return 0
}

####
# Timer
#
# Log timer, executed every second to check for new log file entries.
#
proc ::alcoholicz::ReadLogs::Timer {} {
    variable timerId
    if {[catch {Update}]} {
        LogError ModReadLogs "Unhandled error, please report to developers:\n$::errorInfo"
    }
    set timerId [utimer 1 [namespace current]::Timer]
    return
}

####
# Update
#
# Check for new log entries.
#
proc ::alcoholicz::ReadLogs::Update {} {
    variable logList
    variable logOffset
    variable reBase
    upvar ::alcoholicz::variables variables

    # This log reading code was taken from Project-ZS-NG's sitebot,
    # which was coincidently also written by me (neoxed).
    set lines [list]
    foreach {logId logType logFile} $logList {
        if {$reBase($logType) eq ""} {continue}
        if {![file isfile $logFile] || ![file readable $logFile]} {
            LogError ModReadLogs "Unable to read log file \"$logFile\"."
            continue
        }

        set offset [file size $logFile]
        if {$logOffset($logId) < $offset} {
            if {![catch {set handle [open $logFile r]} error]} {
                seek $handle $logOffset($logId)
                set data [read -nonewline $handle]

                # Update offset with the current file position, in case
                # additional data was written since the 'file size' call.
                set offset [tell $handle]
                close $handle

                foreach line [split $data "\n"] {
                    # Remove the date and time from the log line.
                    if {[regexp -- $reBase($logType) $line result event line]} {
                        lappend lines $logType $event $line
                    } else {
                        LogWarning ModReadLogs "Invalid log line: $line"
                    }
                }
            } else {
                LogError ModReadLogs "Unable to open log file \"$logFile\": $error"
            }
        }
        set logOffset($logId) $offset
    }

    # Log Types:
    # 0 - Main log (ioFTPD.log or glftpd.log).
    # 1 - Error log.
    # 2 - Login log.
    # 3 - Sysop log.
    foreach {logType event line} $lines {
        # Handle unique log files.
        switch -- $logType {
            0 {set line [ArgsToList $line]}
            1 {set line [list $line]; set event "ERROR"}
            2 {
                if {![ParseLogin $line event line]} {
                    LogWarning ModReadLogs "Unknown login.log line: $line"
                    continue
                }
            }
            3 {
                if {![ParseSysop $line event line]} {
                    set event "SYSOP"
                    set line [list $line]
                }
            }
        }
        LogDebug ModReadLogs "Received event: $event (log: $logType)."

        if {![info exists variables($event)]} {
            LogDebug ModReadLogs "No variable definition for event, skipping announce."
            continue
        }

        # If the event's variable definition contains a section-path cookie,
        # assume it's a section oriented announce (e.g. NEWDIR and DELDIR).
        set index [lsearch -glob $variables($event) "*:P"]
        if {$index != -1} {
            set path "/[PathStrip [lindex $line $index]]"

            if {[IsPathExcluded $path]} {
                LogDebug ModReadLogs "Path \"$path\" excluded, skipping announce."
                continue
            }
            set pathSection [GetSectionFromPath $path]
        } else {
            set pathSection $::alcoholicz::defaultSection
        }

        set section [GetSectionFromEvent $pathSection $event]

        # If a pre-command event returns false or the event is disabled,
        # skip the announce. The pre-command event must always be executed,
        # so the section check comes after.
        if {![EventExecute pre $event $section $line] || $section eq ""} {
            LogDebug ModReadLogs "Event disabled or callback returned false, skipping announce."
            continue
        }
        SendSectionTheme $section $event $line

        # Post-command events are only executed if the announce was successful.
        EventExecute post $event $section $line
    }

    return
}

####
# Load
#
# Module initialisation procedure, called when the module is loaded.
#
proc ::alcoholicz::ReadLogs::Load {firstLoad} {
    variable excludePaths
    variable logCount
    variable logList
    variable reBase
    variable reSysop
    variable timerId
    upvar ::alcoholicz::configHandle configHandle

    # Regular expression patterns used to remove the time-stamp
    # from log entries and extract meaningful data.
    unset -nocomplain reBase reSysop
    if {$::alcoholicz::ftpDaemon == 1} {
        # Base patterns for log types.
        set reBase(0) {^\w+ \w+ \s?\d+ \d+:\d+:\d+ \d{4} (\S+): (.+)}
        set reBase(1) {^\w+ \w+ \s?\d+ \d+:\d+:\d+ \d{4} \[(\d+)\s*\] (.+)}
        set reBase(2) $reBase(1)
        set reBase(3) $reBase(1)

        # Common sysop.log entries.
        set reSysop(ADDUSER)  {^'(\S+)' added user '(\S+)'\.$}
        set reSysop(GADDUSER) {^'(\S+)' added user '(\S+)' to group '(\S+)'\.$}
        set reSysop(RENUSER)  {^'(\S+)' renamed user '(\S+)' to '(\S+)'\.$}
        set reSysop(DELUSER)  {^'(\S+)' deleted user '(\S+)'\.$}
        set reSysop(READDED)  {^'(\S+)' readded '(\S+)'\.$}
        set reSysop(PURGED)   {^'(\S+)' purged '(\S+)'$}
        set reSysop(ADDIP)    {^'(\S+)' added ip '(\S+)' to '(\S+)'$}
        set reSysop(DELIP)    {^'(\S+)' .*removed ip '(\S+)' from '(\S+)'$}
        set reSysop(GRPADD)   {^'(\S+)' added group \((\S+)\)$}
        set reSysop(GRPDEL)   {^'(\S+)' deleted group \((\S+)\)$}
        set reSysop(CHGRPADD) {^'(\S+)': successfully added to '(\S+)' by (\S+)$}
        set reSysop(CHGRPDEL) {^'(\S+)': successfully removed from '(\S+)' by (\S+)$}
        set reSysop(GIVE)     {^'(\S+)' \S+ transferred (\d+)K to (\S+)$}
        set reSysop(TAKE)     {^'(\S+)' \S+ took (\d+)K from (\S+)$}
    } elseif {$::alcoholicz::ftpDaemon == 2} {
        # Base patterns for log types.
        set reBase(0) {^\d+-\d+-\d{4} \d+:\d+:\d+ (\S+): (.+)}
        set reBase(1) {^\d+-\d+-\d{4} \d+:\d+:\d+ ()(.+)}
        set reBase(2) {}
        set reBase(3) $reBase(1)

        # Common SysOp.log entries.
        set reSysop(GADDUSER) {^'(\S+)' created user '(\S+)' in group '(\S+)'\.$}
        set reSysop(RENUSER)  {^'(\S+)' renamed user '(\S+)' to '(\S+)'\.$}
        set reSysop(DELUSER)  {^'(\S+)' deleted user '(\S+)'\.$}
        set reSysop(READDED)  {^'(\S+)' readded user '(\S+)'\.$}
        set reSysop(PURGED)   {^'(\S+)' purged user '(\S+)'\.$}
        set reSysop(ADDIP)    {^'(\S+)' added ip '(\S+)' to user '(\S+)'\.$}
        set reSysop(DELIP)    {^'(\S+)' removed ip '(\S+)' from user '(\S+)'\.$}
        set reSysop(GRPADD)   {^'(\S+)' created group '(\S+)'\.$}
        set reSysop(GRPREN)   {^'(\S+)' renamed group '(\S+)' to '(\S+)'\.$}
        set reSysop(GRPDEL)   {^'(\S+)' deleted group '(\S+)'\.$}
        set reSysop(CHGRPADD) {^'(\S+)' added user '(\S+)' to group '(\S+)'\.$}
        set reSysop(CHGRPDEL) {^'(\S+)' removed user '(\S+)' from group '(\S+)'\.$}
    } else {
        error "unknown FTP daemon \"$::alcoholicz::ftpDaemon\""
    }

    # Reset the log count, list, and timer ID on rehash/reload.
    set logCount 0
    set logList [list]
    set timerId ""

    # Monitor all user-defined log files.
    foreach type {main error login sysop} option {mainLogs errorLogs loginLogs sysopLogs} {
        foreach filePath [ArgsToList [ConfigGet $configHandle Module::ReadLogs $option]] {
            if {[catch {AddLog $type $filePath} error]} {
                LogError ModReadLogs "Unable to add log file: $error"
            }
        }
    }

    # Paths to exclude from announcing.
    set excludePaths [ArgsToList [ConfigGet $configHandle Module::ReadLogs excludePaths]]

    if {$firstLoad} {
        set timerId [utimer 1 [namespace current]::Timer]
    }
    LogInfo "Monitoring $logCount log file(s)."
    return
}

####
# Unload
#
# Module finalisation procedure, called before the module is unloaded.
#
proc ::alcoholicz::ReadLogs::Unload {} {
    variable timerId
    if {$timerId ne ""} {
        catch {killutimer $timerId}
        set timerId ""
    }
    return
}
