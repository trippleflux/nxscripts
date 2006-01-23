#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Bot Core
#
# Author:
#   neoxed (neoxed@gmail.com) Apr 16, 2005
#
# Abstract:
#   Implements the event handler, module system, and initialisation procedures.
#

namespace eval ::alcoholicz {
    variable debugMode 0
    variable ftpDaemon 0
    variable localTime 0
    variable scriptPath [file dirname [file normalize [info script]]]

    namespace export b c u r o \
        LogDebug LogInfo LogError LogWarning GetFtpDaemon \
        CmdCreate CmdGetFlags CmdGetList CmdSendHelp CmdRemove \
        FlagGetValue FlagExists FlagIsDisabled FlagIsEnabled FlagCheckEvent FlagCheckSection \
        ModuleFind ModuleHash ModuleInfo ModuleLoad ModuleUnload ModuleRead \
        ScriptExecute ScriptRegister ScriptUnregister \
        GetFlagsFromSection GetSectionFromEvent GetSectionFromPath \
        SendSection SendSectionTheme SendTarget SendTargetTheme
}

#
# Context Array Variables
#
# chanSections  - Channel sections.
# cmdFlags      - Command flags.
# cmdNames      - Commands created with "CmdCreate".
# colours       - Section colour mappings.
# events        - Events grouped by their function.
# format        - Text formatting definitions.
# modules       - Loaded modules.
# pathSections  - Path sections.
# replace       - Static variable replacements.
# scripts       - Callback scripts.
# theme         - Event theme definitions.
# variables     - Event variable definitions.
#
# Context Scalar Variables
#
# configFile    - Fully qualified path to the configuration file.
# configHandle  - Handle to the configuration file, valid only during init.
# debugMode     - Boolean value to indicate if we're running in debug mode.
# ftpDaemon     - FTP daemon identifier: 1=glFTPD and 2=ioFTPD.
# localTime     - Format time values in local time, otherwise UTC is used.
#

interp alias {} IsTrue {} string is true -strict
interp alias {} IsFalse {} string is false -strict

####
# Control Codes
#
# b - Bold
# c - Colour
# u - Underline
# r - Reverse
# o - Reset
#
proc ::alcoholicz::b {} {return \002}
proc ::alcoholicz::c {} {return \003}
proc ::alcoholicz::u {} {return \037}
proc ::alcoholicz::r {} {return \026}
proc ::alcoholicz::o {} {return \015}

####
# Logging
#
# Logging facility for debug, error, and warning messages.
#
proc ::alcoholicz::LogInfo {message} {
    putlog "\[[b]AlcoBot[b]\] $message"
}

proc ::alcoholicz::LogDebug {function message} {
    if {$::alcoholicz::debugMode} {
        putlog "\[[b]AlcoBot[b]\] Debug :: $function - $message"
    }
}

proc ::alcoholicz::LogError {function message} {
    putlog "\[[b]AlcoBot[b]\] Error :: $function - $message"
}

proc ::alcoholicz::LogWarning {function message} {
    putlog "\[[b]AlcoBot[b]\] Warning :: $function - $message"
}

####
# GetFtpDaemon
#
# Retrieve the FTP daemon name.
#
proc ::alcoholicz::GetFtpDaemon {} {
    variable ftpDaemon
    switch -- $ftpDaemon {
        1 {return "glFTPD"}
        2 {return "ioFTPD"}
        default {error "unknown FTP daemon ID \"$ftpDaemon\""}
    }
}

####
# SetFtpDaemon
#
# Define the FTP daemon to use, only glFTPD and ioFTPD are supported. This
# procedure is not exported because it's meant for internal use only.
#
proc ::alcoholicz::SetFtpDaemon {name} {
    variable ftpDaemon
    switch -glob -- [string tolower $name] {
        {gl*} {set ftpDaemon 1}
        {io*} {set ftpDaemon 2}
        default {error "unknown FTP daemon \"$name\""}
    }
    return $ftpDaemon
}

################################################################################
# Commands                                                                     #
################################################################################

####
# CmdCreate
#
# Create a channel command.
#
proc ::alcoholicz::CmdCreate {type name script args} {
    variable cmdNames
    variable cmdPrefix

    # Check arguments.
    if {$type ne "channel" && $type ne "message"} {
        error "invalid command type \"$type\""
    }
    if {[info exists cmdNames([list $type $name])]} {
        error "the $type command \"$name\" already exists"
    }

    set argDesc  ""
    set cmdDesc  ""
    set category ""
    set names  [list $name]
    set prefix $cmdPrefix

    foreach {option value} $args {
        switch -- $option {
            -aliases  {eval lappend names $value}
            -args     {set argDesc $value}
            -category {set category $value}
            -desc     {set cmdDesc $value}
            -prefix   {set prefix $value}
            default   {error "invalid option \"$option\""}
        }
    }

    # Bind the command and its aliases.
    set binds [list]
    foreach command $names {
        set command [string tolower "$prefix$command"]

        if {$type eq "channel"} {
            bind pub -|- $command [list [namespace current]::CmdChannelProc $name]
        } elseif {$type eq "message"} {
            bind msg -|- $command [list [namespace current]::CmdMessageProc $name]
        }
        lappend binds $command
    }

    # List: argDesc cmdDesc category binds script
    set cmdNames([list $type $name]) [list $argDesc $cmdDesc $category $binds $script]
    return
}

####
# CmdGetFlags
#
# Retrieve a list of flags for the given command.
#
proc ::alcoholicz::CmdGetFlags {command} {
    variable cmdFlags
    if {[info exists cmdFlags($command)]} {
        return $cmdFlags($command)
    }
    return [list]
}

####
# CmdGetList
#
# Retrieve a list of commands created with "CmdCreate".
# List: {type command} {argDesc cmdDesc category binds script} ...
#
proc ::alcoholicz::CmdGetList {typePattern namePattern} {
    variable cmdNames
    return [array get cmdNames [list $typePattern $namePattern]]
}

####
# CmdSendHelp
#
# Send a command help message to the specified target.
#
proc ::alcoholicz::CmdSendHelp {target type name {message ""}} {
    global lastbind
    variable cmdNames

    if {![info exists cmdNames([list $type $name])]} {
        error "invalid command type \"$type\" or name \"$name\""
    }

    set argDesc [lindex $cmdNames([list $type $name]) 0]
    if {$message ne ""} {
        SendTargetTheme "PRIVMSG $target" commandHelp  [list $argDesc $lastbind $message]
    } else {
        SendTargetTheme "PRIVMSG $target" commandUsage [list $argDesc $lastbind]
    }
}

####
# CmdRemove
#
# Remove a channel command created with "CmdCreate".
#
proc ::alcoholicz::CmdRemove {type name} {
    variable cmdNames

    if {![info exists cmdNames([list $type $name])]} {
        # The caller could have specified an invalid command type, a
        # command name that was already removed, or a command name that
        # was not created with "CmdCreate".
        error "invalid command type \"$type\" or name \"$name\""
    }

    # Remove the command bindings.
    set binds [lindex $cmdNames([list $type $name]) 3]
    foreach command $binds {
        if {$type eq "channel"} {
            catch {unbind pub -|- $command [list [namespace current]::CmdChannelProc $name]}
        } elseif {$type eq "message"} {
            catch {unbind msg -|- $command [list [namespace current]::CmdMessageProc $name]}
        }
    }

    unset cmdNames([list $type $name])
    return
}

####
# CmdChannelProc
#
# Wrapper for Eggdrop "bind pub" commands. Parses the text argument into a
# Tcl list and provides more information when a command evaluation fails.
#
proc ::alcoholicz::CmdChannelProc {command user host handle channel text} {
    variable cmdFlags
    variable cmdNames
    set target "PRIVMSG $channel"

    foreach {enabled name value} [CmdGetFlags $command] {
        set result 0
        switch -- $name {
            all     {set result 1}
            channel {set result [string equal -nocase $value $channel]}
            flags   {set result [matchattr $user $value]}
            host    {set result [string match -nocase $value $host]}
            target  {
                if {$value eq "notice"} {
                    set target "NOTICE $user"
                } elseif {$value eq "private"} {
                    set target "PRIVMSG $user"
                }
            }
            user    {set result [string equal -nocase $value $user]}
        }
        if {$result} {
            if {!$enabled} {
                LogDebug CmdChannel "Matched negation flag \"$name\" for command \"$::lastbind\", returning."
                return
            }
            break
        }
    }
    set argv [ArgsToList $text]

    # Eval is used to expand arguments to the callback procedure,
    # e.g. "CmdCreate channel !foo [list ChanFooCmd abc 123]".
    set script [lindex $cmdNames([list "channel" $command]) 4]

    if {[catch {eval $script [list $command $target $user $host $handle $channel $argv]} message]} {
        LogError CmdChannel "Error evaluating \"$script\":\n$::errorInfo"
    }
    return
}

####
# CmdMessageProc
#
# Wrapper for Eggdrop "bind msg" commands. Parses the text argument into a
# Tcl list and provides more information when a command evaluation fails.
#
proc ::alcoholicz::CmdMessageProc {command user host handle text} {
    variable cmdFlags
    variable cmdNames

    foreach {enabled name value} [CmdGetFlags $command] {
        set result 0
        switch -- $name {
            all   {set result 1}
            flags {set result [matchattr $user $value]}
            host  {set result [string match -nocase $value $host]}
            user  {set result [string equal -nocase $value $user]}
        }
        if {$result} {
            if {!$enabled} {
                LogDebug CmdMessage "Matched negation flag \"$name\" for command \"$::lastbind\", returning."
                return
            }
            break
        }
    }
    set argv [ArgsToList $text]
    set target "PRIVMSG $user"

    # Eval is used to expand arguments to the callback procedure,
    # e.g. "CmdCreate message !foo [list MessageFooCmd abc 123]".
    set script [lindex $cmdNames([list "message" $command]) 4]

    if {[catch {eval $script [list $command $target $user $host $handle $argv]} message]} {
        LogError CmdMessage "Error evaluating \"$script\":\n$::errorInfo"
    }
    return
}

################################################################################
# Flags                                                                        #
################################################################################

####
# FlagGetValue
#
# Retrieve a flag's value from a list of flags. If the flag and it's value are
# present, the value is stored in the given variable and non-zero is returned.
# If the flag is not listed or does not have a value, zero is returned.
#
proc ::alcoholicz::FlagGetValue {flagList flagName valueVar} {
    upvar $valueVar value
    foreach flag $flagList {
        # Parse: +|-<name>=<value>
        if {[regexp -- {^(?:\+|\-)(\w+)=(.+)$} $flag dummy name result] && $name eq $flagName} {
            set value $result; return 1
        }
    }
    return 0
}

####
# FlagExists
#
# Check if the given flag exists.
#
proc ::alcoholicz::FlagExists {flagList flagName} {
    foreach flag $flagList {
        # Parse: +|-<name>[=<value>]
        if {[regexp -- {^(?:\+|\-)(\w+)} $flag dummy name] && $name eq $flagName} {
            return 1
        }
    }
    return 0
}

####
# FlagIsDisabled
#
# Check if the given flag exists and is disabled.
#
proc ::alcoholicz::FlagIsDisabled {flagList flagName} {
    foreach flag $flagList {
        # Parse: +|-<name>[=<value>]
        if {![regexp -- {^(\+|\-)(\w+)} $flag dummy prefix name]} {continue}

        if {$name eq "all" || $name eq $flagName} {
            return [string equal $prefix "-"]
        }
    }
    return 0
}

####
# FlagIsEnabled
#
# Check if the given flag exists and is enabled.
#
proc ::alcoholicz::FlagIsEnabled {flagList flagName} {
    foreach flag $flagList {
        # Parse: +|-<name>[=<value>]
        if {![regexp -- {^(\+|\-)(\w+)} $flag dummy prefix name]} {continue}

        if {$name eq "all" || $name eq $flagName} {
            return [string equal $prefix "+"]
        }
    }
    return 0
}

####
# FlagCheckEvent
#
# Check if the given event is enabled in the flag list.
#
proc ::alcoholicz::FlagCheckEvent {flagList event} {
    variable events
    foreach flag $flagList {
        # Parse: +|-<name>[=<value>]
        if {![regexp -- {^(\+|\-)(\w+)} $flag dummy prefix name]} {continue}

        if {$name eq "all" || $name eq $event || ([info exists events($name)] &&
            [lsearch -sorted $events($name) $event] != -1)} {
            return [string equal $prefix "+"]
        }
    }
    return 0
}

####
# FlagCheckSection
#
# Check if the given event is enabled in a channel or path section.
#
proc ::alcoholicz::FlagCheckSection {section event} {
    return [FlagCheckEvent [GetFlagsFromSection $section] $event]
}

################################################################################
# Modules                                                                      #
################################################################################

####
# ModuleFind
#
# Returns the directory path of where a given module resides. An
# error is raised if the module cannot be located.
#
proc ::alcoholicz::ModuleFind {modName} {
    variable ftpDaemon
    variable scriptPath

    # Generic modules take precedence over daemon specific modules.
    set dirList [list generic [lindex {{} glftpd ioftpd} $ftpDaemon]]

    foreach modDir $dirList {
        # Directory tree: ./modules/<type>/<module>/
        set modPath [file join $scriptPath modules $modDir $modName]
        if {[file isdirectory $modPath]} {
            return $modPath
        }
    }

    error "module not found"
}

####
# ModuleHash
#
# Calculate the MD5 check-sum of a given file. Even though the MD5 hash
# algorithm is considered broken, it is still suitable for this purpose.
#
proc ::alcoholicz::ModuleHash {filePath} {
    set fileHandle [open $filePath r]
    fconfigure $fileHandle -translation binary
    set hash [crypt start md5]

    while {![eof $fileHandle]} {
        # Process 8KB of data each iteration.
        set data [read $fileHandle 8192]
        if {[string length $data]} {
            crypt update $hash $data
        }
    }
    close $fileHandle
    return [crypt end $hash]
}

####
# ModuleInfo
#
# Retrieve information about loaded modules.
#
proc ::alcoholicz::ModuleInfo {option args} {
    variable modules

    set argc [llength $args]
    switch -- $option {
        loaded {
            if {$argc != 1} {
                error "wrong # args: must be \"ModuleInfo loaded moduleName\""
            }
            return [info exists modules([lindex $args 0])]
        }
        list {
            if {$argc == 0} {
                return [array names modules]
            }
            if {$argc == 1} {
                return [array names modules [lindex $args 0]]
            }
            error "wrong # args: must be \"ModuleInfo list ?pattern?\""
        }
        query {
            if {$argc != 1} {
                error "wrong # args: must be \"ModuleInfo query moduleName\""
            }
            set modName [lindex $args 0]
            if {![info exists modules($modName)]} {
                error "module not loaded"
            }
            return $modules($modName)
        }
    }
    error "unknown option \"$option\": must be loaded, list, or query"
}

####
# ModuleLoad
#
# Read and initialise the given module. An error is raised if the module
# cannot be found or an unexpected error occurs while loading it.
#
proc ::alcoholicz::ModuleLoad {modName} {
    variable modules

    # Locate the module and read its definition file.
    set modPath [ModuleFind $modName]
    array set modInfo [ModuleRead [file join $modPath "module.def"]]

    # Refuse to load the module if a dependency is not present.
    foreach modDepend $modInfo(depends) {
        if {![info exists modules($modDepend)]} {
            error "missing module dependency \"$modDepend\""
        }
    }

    ModuleLoadEx $modName [array get modInfo]
}

####
# ModuleLoadEx
#
# Performs the actual module initialisation. This procedure leaves dependency
# and parameter checks up to the caller. Module writers are strongly encouraged
# to use ModuleLoad instead.
#
proc ::alcoholicz::ModuleLoadEx {modName modInfoList} {
    variable modules
    array set modInfo $modInfoList

    set firstLoad 1
    set fileHash [ModuleHash $modInfo(tclFile)]

    if {[info exists modules($modName)]} {
        set currentHash [lindex $modules($modName) 4]

        # If the file's check-sum has changed, it's assumed that there were
        # code changes and a full initialisation is performed. Otherwise a
        # basic initialisation is performed.
        if {$fileHash ne $currentHash} {
            catch {ModuleUnload $modName}
        } else {
            set firstLoad 0
        }
    }

    if {[catch {source $modInfo(tclFile)} message]} {
        error "can't source file \"$modInfo(tclFile)\": $message"
    }

    # Remove trailing namespace identifiers from the context.
    set modInfo(context) [string trimright $modInfo(context) ":"]

    if {[catch {${modInfo(context)}::Load $firstLoad} message]} {
        # If the module initialisation failed, call the unload procedure
        # in case the module did not clean up after failing.
        catch {${modInfo(context)}::Unload}

        error "initialisation failed: $message"
    }

    set modules($modName) [list $modInfo(desc) $modInfo(context) \
        $modInfo(depends) $modInfo(tclFile) $modInfo(varFile) $fileHash]

    return
}

####
# ModuleUnload
#
# Unload and finalise the given module. An error is raised if the
# module is not loaded or an unexpected error occurs while unloading it.
#
proc ::alcoholicz::ModuleUnload {modName} {
    variable modules
    if {![info exists modules($modName)]} {
        error "module not loaded"
    }

    foreach {desc context depends tclFile varFile hash} $modules($modName) {break}
    set failed [catch {${context}::Unload} message]

    # The namespace context must only be deleted if it's
    # not the global namespace or the bot's namespace.
    if {$context ne "" && $context ne [namespace current]} {
        catch {namespace delete $context}
    }

    unset modules($modName)

    # Raise an error after cleaning up the module.
    if {$failed} {error "finalisation failed: $message"}
    return
}

####
# ModuleRead
#
# Read a module definition file. The module information is returned in a key
# and value paired list, in the same manner that "array get" does. An error is
# raised if the module definition file cannot be read or is missing critical
# information.
#
proc ::alcoholicz::ModuleRead {filePath} {
    set dirPath [file dirname $filePath]
    set required [list desc context depends tclFile varFile]

    set handle [open $filePath r]
    foreach line [split [read -nonewline $handle] "\n"] {
        if {[string index $line 0] eq "#" || [set index [string first "=" $line]] == -1} {
            continue
        }
        set option [string range $line 0 [expr {$index - 1}]]

        if {[lsearch -exact $required $option] != -1} {
            set modInfo($option) [string range $line [incr index] end]
        }
    }
    close $handle

    # Perform all module information checks after the file was
    # read, in case the same option was defined multiple times.
    set index 0
    while {$index < [llength $required]} {
        if {[lsearch -exact [array names modInfo] [lindex $required $index]] != -1} {
            set required [lreplace $required $index $index]
            continue
        }
        incr index
    }
    if {[llength $required]} {
        error "missing required module information: [JoinLiteral $required]"
    }

    set modInfo(tclFile) [file join $dirPath $modInfo(tclFile)]
    if {![file isfile $modInfo(tclFile)]} {
        error "the source file \"$modInfo(tclFile)\" does not exist"
    }

    if {$modInfo(varFile) ne ""} {
        set modInfo(varFile) [file join $dirPath $modInfo(varFile)]
        if {![file isfile $modInfo(varFile)]} {
            error "the variable file \"$modInfo(varFile)\" does not exist"
        }
    }

    return [array get modInfo]
}

################################################################################
# Scripts                                                                      #
################################################################################

####
# ScriptExecute
#
# Run all registered scripts for the specified event type.
#
proc ::alcoholicz::ScriptExecute {type event args} {
    variable scripts
    set varName scripts([list $type $event])

    if {![info exists $varName]} {return 1}
    set result 1
    foreach {script alwaysExec} [set $varName] {
        if {!$alwaysExec && !$result} {continue}

        if {[catch {set scriptResult [eval [list $script $event] $args]} message]} {
            LogError ScriptExecute "Error evaluating callback \"$script\" for $type $event: $::errorInfo"

        } elseif {[IsFalse $scriptResult]} {
            LogDebug ScriptExecute "The callback \"$script\" for $type $event returned false."
            set result 0

        } elseif {![IsTrue $scriptResult]} {
            LogWarning ScriptExecute "The callback \"$script\" for $type $event did not return a boolean value."
        }
    }
    return $result
}

####
# ScriptRegister
#
# Register an event callback script.
#
proc ::alcoholicz::ScriptRegister {type event script {alwaysExec 0}} {
    variable scripts
    set varName scripts([list $type $event])

    if {[info exists $varName]} {
        # Check the script is already registered.
        set scriptList [set $varName]
        foreach {name always} $scriptList {
            if {$name eq $script} {return $scriptList}
        }
    }
    return [lappend $varName $script [IsTrue $alwaysExec]]
}

####
# ScriptUnregister
#
# Unregister an event callback script.
#
proc ::alcoholicz::ScriptUnregister {type event script} {
    variable scripts
    set varName scripts([list $type $event])

    if {![info exists $varName]} {return [list]}
    set index 0
    foreach {name always} [set $varName] {
        if {$name eq $script} {
            return [set $varName [lreplace [set $varName] $index [incr index]]]
        }
        incr index 2
    }
    return [set $varName]
}

################################################################################
# Sections                                                                     #
################################################################################

####
# GetFlagsFromSection
#
# Retrieve the flags from a channel or path section.
#
proc ::alcoholicz::GetFlagsFromSection {section} {
    variable chanSections
    variable pathSections

    if {[info exists chanSections($section)]} {
        return [lindex $chanSections($section) 1]
    }
    if {[info exists pathSections($section)]} {
        return [lindex $pathSections($section) 2]
    }
    return [list]
}

####
# GetSectionFromEvent
#
# Retrieve the destination section name for an event. An empty string
# is returned if no matching section is found.
#
proc ::alcoholicz::GetSectionFromEvent {section event} {
    variable chanSections

    if {$section ne "" && [FlagCheckSection $section $event]} {
        return $section
    }

    # Check if the event is enabled in another channel-section. This allows users
    # to redirect events without requiring additional configuration options. For
    # example, one could restrict the WIPE event in all path-sections, but allow
    # it in the STAFF channel-section.
    foreach section [array names chanSections] {

        # First match wins, so users must restrict the event from in all
        # sections except the one they want it redirected to.
        if {[FlagCheckEvent [lindex $chanSections($section) 1] $event]} {
            return $section
        }
    }

    # Event disabled.
    return ""
}

####
# GetSectionFromPath
#
# Retrieve the path-section name from a given virtual path. An empty string
# is returned if no matching section path is found.
#
proc ::alcoholicz::GetSectionFromPath {fullPath} {
    variable pathSections
    set bestMatch 0
    set pathSection ""

    foreach name [array names pathSections] {
        set path [lindex $pathSections($name) 0]
        set pathLength [string length $path]

        # Best match wins (longest section path).
        if {$pathLength > $bestMatch && [string equal -length $pathLength $path $fullPath]} {
            set bestMatch $pathLength
            set pathSection $name
        }
    }
    return $pathSection
}

################################################################################
# Output                                                                       #
################################################################################

####
# SendSection
#
# Send text to all channels for the given channel or path section.
#
proc ::alcoholicz::SendSection {section text} {
    variable chanSections
    variable pathSections

    if {[info exists chanSections($section)]} {
        set channels [lindex $chanSections($section) 0]
    } elseif {[info exists pathSections($section)]} {
        set channels [lindex $pathSections($section) 1]
    } else {
        LogError SendSection "Unknown section \"$section\"."
        return
    }

    foreach line [split $text "\n"] {
        foreach channel $channels {
            putserv "PRIVMSG $channel :$line"
        }
    }
    return
}

####
# SendSectionTheme
#
# Replace theme values and send the text to all channels for the given
# channel or path section.
#
proc ::alcoholicz::SendSectionTheme {section type {valueList ""}} {
    variable theme
    variable variables

    if {![info exists theme($type)] || ![info exists variables($type)]} {
        LogError SendSectionTheme "Missing theme or variable definition for \"$type\"."
        return
    }

    set text [VarReplaceCommon $theme($type) $section]
    SendSection $section [VarReplace $text $variables($type) $valueList]
}

####
# SendTarget
#
# Send text to the given target.
#
proc ::alcoholicz::SendTarget {target text} {
    foreach line [split $text "\n"] {
        putserv "$target :$line"
    }
}

####
# SendTargetTheme
#
# Replace theme values and send the text to the given target.
#
proc ::alcoholicz::SendTargetTheme {target type {valueList ""} {section ""}} {
    variable defaultSection
    variable theme
    variable variables

    if {![info exists theme($type)] || ![info exists variables($type)]} {
        LogError SendTargetTheme "Missing theme or variable definition for \"$type\"."
        return
    }

    # Replace section colours and common items before the value list, in
    # case the values introduce colour codes (e.g. a user named "[b]ill",
    # since "[b]" is also the bold code).
    set text [VarReplaceCommon $theme($type) $section]
    SendTarget $target [VarReplace $text $variables($type) $valueList]
}

################################################################################
# DCC Admin Command                                                            #
################################################################################

# Command aliases.
bind dcc n "alc"        ::alcoholicz::DccAdmin
bind dcc n "alco"       ::alcoholicz::DccAdmin
bind dcc n "alcobot"    ::alcoholicz::DccAdmin
bind dcc n "alcohol"    ::alcoholicz::DccAdmin
bind dcc n "alcoholicz" ::alcoholicz::DccAdmin

####
# DccAdmin
#
# Bot administration command, used from Eggdrop's party-line.
#
proc ::alcoholicz::DccAdmin {handle idx text} {
    variable scriptPath

    set argv [ArgsToList $text]
    set event [string toupper [lindex $argv 0]]

    if {$event eq "DUMP"} {
        variable chanSections
        variable pathSections
        variable cmdNames
        variable configFile
        variable debugMode
        variable modules
        variable scripts

        putdcc $idx "[b]General:[b]"
        putdcc $idx "Config File: $configFile"
        putdcc $idx "Debug Mode: [expr {$debugMode ? {True} : {False}}]"
        putdcc $idx "FTP Daemon: [GetFtpDaemon]"
        putdcc $idx "Script Path: $scriptPath"

        putdcc $idx "[b]Commands:[b]"
        foreach name [lsort [array names cmdNames]] {
            foreach {argDesc cmdDesc category binds script} $cmdNames($name) {break}
            putdcc $idx "[join $name { }] - [b]Binds:[b] [join $binds {, }] [b]Category:[b] $category"
        }

        putdcc $idx "[b]Modules:[b]"
        foreach name [lsort [array names modules]] {
            foreach {desc context depends tclFile varFile hash} $modules($name) {break}
            putdcc $idx "$name - [b]Info:[b] $desc [b]Path:[b] [file dirname $tclFile] [b]MD5:[b] [encode hex $hash]"
        }

        putdcc $idx "[b]Sections:[b]"
        foreach name [lsort [array names chanSections]] {
            foreach {channels flags} $chanSections($name) {break}
            putdcc $idx "$name - [b]Channels:[b] [JoinLiteral $channels] [b]Flags:[b] $flags"
        }
        foreach name [lsort [array names pathSections]] {
            foreach {path channels flags} $pathSections($name) {break}
            putdcc $idx "$name - [b]Channels:[b] [JoinLiteral $channels] [b]Flags:[b] $flags [b]Path:[b] $path"
        }

        putdcc $idx "[b]Scripts:[b]"
        foreach name [lsort [array names scripts]] {
            set scriptList [list]
            foreach {proc always} $scripts($name) {
                lappend scriptList "$proc ([expr {$always ? {True} : {False}}])"
            }

            foreach {type name} $name {break}
            putdcc $idx "$type $name - [JoinLiteral $scriptList]"
        }
    } elseif {$event eq "REHASH" || $event eq "RELOAD"} {
        # Reload configuration file.
        InitMain

    } else {
        global lastbind
        putdcc $idx "[b]Alcoholicz Bot DCC Admin[b]"
        putdcc $idx ".$lastbind dump   - Dump configuration."
        putdcc $idx ".$lastbind help   - Command help."
        putdcc $idx ".$lastbind reload - Reload configuration."
    }
    return
}

################################################################################
# Start Up                                                                     #
################################################################################

####
# InitConfig
#
# Read the given configuration file. All previously existing configuration
# data is discarded, so modules may have to update internal values.
#
proc ::alcoholicz::InitConfig {filePath} {
    variable configFile
    variable configHandle
    variable cmdFlags
    variable defaultSection
    variable chanSections
    variable pathSections
    unset -nocomplain cmdFlags chanSections pathSections

    # TODO: Parse xml config.

    # Update configuration path before reading the file.
    set configFile $filePath
    if {[info exists configHandle]} {
        ConfigChange $configHandle -path $configFile
    } else {
        set configHandle [ConfigOpen $configFile -align 2]
    }
    ConfigRead $configHandle

    foreach option {cmdPrefix debugMode localTime ftpDaemon siteName siteTag} {
        variable $option [ConfigGet $configHandle General $option]
    }
    set debugMode [IsTrue $debugMode]
    set localTime [IsTrue $localTime]

    if {[catch {SetFtpDaemon $ftpDaemon} message]} {
        error "Unable to set FTP daemon: $message"
    }

    foreach {name value} [ConfigGetEx $configHandle Commands] {
        set flags [list]
        foreach flag [ArgsToList $value] {
            # Parse command flags into a list.
            if {[regexp -- {^(\+|\-)(\w+)=?(.*)$} $flag dummy prefix flag setting]} {
                lappend flags [string equal "+" $prefix] $flag $setting
            } else {
                LogWarning InitConfig "Invalid flag for command \"$name\": $flag"
            }
        }
        set cmdFlags($name) $flags
    }

    # Read channel and path sections.
    foreach {name value} [ConfigGetEx $configHandle Sections] {
        set options [ArgsToList $value]
        if {[llength $options] == 2} {
            set chanSections($name) $options
        } elseif {[llength $options] == 3} {
            set pathSections($name) $options
        } else {
            LogWarning InitConfig "Wrong number of options for section \"$name\": $value"
        }
    }

    if {![info exists chanSections($defaultSection)]} {
        error "No default channel section defined, must be named \"$defaultSection\"."
    }
}

####
# InitLibraries
#
# Load all library scripts and Tcl extensions. An error is raised if a
# required script or extension could not be loaded.
#
proc ::alcoholicz::InitLibraries {rootPath} {
    global auto_path

    set libPath [file join $rootPath "libs"]
    foreach script {constants.tcl libFtp.tcl libGetOpt.tcl libTree.tcl libConfig.tcl libUtil.tcl} {
        set script [file join $libPath $script]
        if {[catch {source $script} message]} {
            error "couldn't source script \"$script\": $message"
        }
    }

    # Some users reported that "auto_path" was not always set,
    # which is bizarre considering Tcl initialises this variable.
    if {![info exists auto_path] || [lsearch -exact $auto_path $libPath] == -1} {
        # Add the libs directory to the package search path.
        lappend auto_path $libPath
    }

    # The TLS extension is optional, but AlcoExt and tDOM are required.
    catch {package require tls 1.5}
    package require AlcoExt 0.4
    package require tdom 8.0
}

####
# InitModules
#
# Load the given modules. Any pre-existing modules that are not listed
# in the "modList" parameter will be unloaded.
#
proc ::alcoholicz::InitModules {modList} {
    variable cmdNames
    variable modules
    LogInfo "Loading modules..."

    set prevModList [array names modules]
    array set prevCmdNames [array get cmdNames]
    unset -nocomplain cmdNames

    # TODO:
    # - The module load order should reflect their dependency requirements.
    # - Read all module definition files, re-order them, and call ModuleLoadEx.

    foreach modName $modList {
        if {[catch {ModuleLoad $modName} message]} {
            LogInfo "Unable to load module \"$modName\": $message"
        } else {
            LogInfo "Module Loaded: $modName"
            set index [lsearch -exact $prevModList $modName]
            if {$index != -1} {
                set prevModList [lreplace $prevModList $index $index]
            }
        }
    }

    # Remove unreferenced modules.
    foreach modName $prevModList {
        if {[catch {ModuleUnload $modName} message]} {
            LogInfo "Unable to unload module \"$modName\": $message"
        } else {
            LogInfo "Module Unloaded: $modName"
        }
    }

    # Remove unreferenced commands.
    foreach name [array names prevCmdNames] {
        if {![info exists cmdNames($name)]} {
            set cmdNames($name) $prevCmdNames($name)
            CmdRemove [lindex $name 0] [lindex $name 1]
        }
    }
}

####
# InitTheme
#
# Read the given theme file.
#
proc ::alcoholicz::InitTheme {themeFile} {
    variable colours
    variable format
    variable theme
    variable scriptPath
    variable variables
    unset -nocomplain colours format theme

    set themeFile [file join $scriptPath "themes" $themeFile]
    set handle [ConfigOpen $themeFile]
    ConfigRead $handle

    # Process colour entries.
    foreach {name value} [ConfigGetEx $handle Colour] {
        if {![regexp -- {^(\S+),(\d+)$} $name result section num]} {
            LogWarning InitTheme "Invalid colour entry \"$name\"."
        } elseif {![string is digit -strict $value] || $value < 0 || $value > 15} {
            LogWarning InitTheme "Invalid colour value \"$value\" for \"$name\", must be from 0 to 15."
        } else {
            # Create a mapping of section colours to use with "string map". The
            # colour index must be zero-padded to two digits to avoid conflicts
            # with other numerical chars. Note that the %s identifier is used
            # instead of %d to avoid octal interpretation (e.g. "format %d 08").
            lappend colours($section) "\[c$num\]" [format "\003%02s" $value]
        }
    }

    # Process format entries.
    set known {prefix date time sizeKilo sizeMega sizeGiga sizeTera speedKilo speedGiga speedMega}
    foreach {name value} [ConfigGetEx $handle Format] {
        set index [lsearch -exact $known $name]
        if {$index != -1} {
            set known [lreplace $known $index $index]

            # Remove quotes around the format value, if present.
            if {[string index $value 0] eq "\"" && [string index $value end] eq "\""} {
                set value [string range $value 1 end-1]
            }
            set format($name) [VarReplaceBase $value 0]
        } else {
            LogDebug InitTheme "Unknown format type \"$name\"."
        }
    }
    if {[llength $known]} {
        foreach name $known {set format($name) ""}
        LogWarning InitTheme "Missing format entries: [JoinLiteral $known]."
    }

    # Process theme entries.
    set known [array names variables]
    foreach {name value} [ConfigGetEx $handle Theme] {
        set index [lsearch -exact $known $name]
        if {$index != -1} {
            set known [lreplace $known $index $index]

            # Remove quotes around the theme value, if present.
            if {[string index $value 0] eq "\"" && [string index $value end] eq "\""} {
                set value [string range $value 1 end-1]
            }
            set theme($name) [VarReplaceBase $value]
        }
    }
    if {[llength $known]} {
        foreach name $known {set theme($name) ""}
        LogWarning InitTheme "Missing theme entries: [JoinLiteral [lsort $known]]."
    }

    ConfigClose $handle
}

####
# InitVars
#
# Read the given variable definition files.
#
proc ::alcoholicz::InitVariables {fileNames} {
    variable events
    variable modules
    variable replace
    variable scriptPath
    variable variables
    unset -nocomplain events replace variables

    # Resolve file names to their full path.
    set varFiles [list]
    foreach fileName $fileNames {
        lappend varFiles [file join $scriptPath "vars" $fileName]
    }

    # Retrieve module variable definitions.
    foreach modName [array names modules] {
        foreach {desc context depends tclFile varFile hash} $modules($modName) {break}
        if {$varFile ne ""} {lappend varFiles $varFile}
    }

    # The "AlcoBot.vars" file enables users to define variables for third
    # party scripts or to redefine existing variable definitions.
    lappend varFiles [file join $scriptPath "AlcoBot.vars"]

    foreach filePath $varFiles {
        set handle [ConfigOpen $filePath]
        ConfigRead $handle

        foreach {name value} [ConfigGetEx $handle Events] {
            # Allow underscores for convenience.
            if {![string is wordchar -strict $name]} {
                LogError InitVariables "Invalid event group name \"$name\" in \"$filePath\": must be alphanumeric."
                continue
            }

            # Several categories have multiple events defined. Therefore,
            # the config value must be appended to, not replaced.
            eval lappend [list events($name)] $value
        }

        array set replace [ConfigGetEx $handle Replace]
        array set variables [ConfigGetEx $handle Variables]

        ConfigClose $handle
    }

    # The contents of "events" array must be sorted to use "lsearch -sorted".
    foreach name [array names events] {
        set events($name) [lsort -unique $events($name)]
    }
}

####
# InitMain
#
# Loads all bot subsystems. Eggdrop is terminated if a critical initialisation
# stage fails; an error message will be displayed explaining the failure.
#
proc ::alcoholicz::InitMain {} {
    variable configHandle
    variable scriptPath
    LogInfo "Starting..."

    LogInfo "Loading libraries..."
    if {[catch {InitLibraries $scriptPath} message]} {
        LogError Libraries $message
        die
    }

    # Initialise various subsystems.
    LogInfo "Loading configuration..."
    set configFile [file join $scriptPath "AlcoBot.conf"]
    if {[catch {InitConfig $configFile} message]} {
        LogError Config $message
        die
    }

    foreach funct {InitModules InitVariables InitTheme} option {modules varFiles themeFile} {
        set value [ArgsToList [ConfigGet $configHandle General $option]]
        if {[catch {$funct $value} message]} {
            LogError [string range $funct 4 end] $message
            die
        }
    }

    ConfigFree $configHandle
    LogInfo "Sitebot loaded, configured for [GetFtpDaemon]."
    return
}

# This Tcl version check is here for a reason - do not remove it. If you
# think removing this check in order for AlcoBot to work on an older Tcl is
# clever, it's not. In fact, you are a moron. AlcoBot uses several features
# which are specific to Tcl 8.4.
if {[catch {package require Tcl 8.4} error]} {
    LogError TclVersion "You must be using Tcl v8.4, or newer."
    die
}

::alcoholicz::InitMain
