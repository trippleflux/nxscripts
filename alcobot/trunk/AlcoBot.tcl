#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005-2006 Alcoholicz Scripting Team
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

namespace eval ::Bot {
    variable debugMode 0
    variable ftpDaemon 0
    variable localTime 0
    variable scriptPath [file dirname [file normalize [info script]]]

    namespace export b c u r o \
        LogDebug LogInfo LogError LogWarning GetFtpDaemon \
        CmdCreate CmdGetList CmdGetOptions CmdSendHelp CmdRemove \
        FlagGetValue FlagExists FlagIsDisabled FlagIsEnabled FlagCheckEvent FlagCheckSection \
        ModuleFind ModuleHash ModuleInfo ModuleLoad ModuleUnload ModuleRead \
        ScriptExecute ScriptRegister ScriptUnregister \
        GetFlagsFromSection GetSectionFromEvent GetSectionFromPath \
        SendSection SendSectionTheme SendTarget SendTargetTheme \
        VarLoad VarReplace VarReplaceBase VarReplaceCommon
}

#
# Context Array Variables
#
# chanSections  - Channel sections.
# cmdNames      - Commands created with "CmdCreate".
# cmdOptions    - Command options.
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

####
# Control Codes
#
# b - Bold
# c - Colour
# u - Underline
# r - Reverse
# o - Reset
#
proc ::Bot::b {} {return \002}
proc ::Bot::c {} {return \003}
proc ::Bot::u {} {return \037}
proc ::Bot::r {} {return \026}
proc ::Bot::o {} {return \015}

####
# Logging
#
# Logging facility for debug, error, and warning messages.
#
proc ::Bot::LogInfo {message} {
    putlog "\[[b]AlcoBot[b]\] $message"
}

proc ::Bot::LogDebug {function message} {
    if {$::Bot::debugMode} {
        putlog "\[[b]AlcoBot[b]\] Debug :: $function - $message"
    }
}

proc ::Bot::LogError {function message} {
    putlog "\[[b]AlcoBot[b]\] Error :: $function - $message"
}

proc ::Bot::LogWarning {function message} {
    putlog "\[[b]AlcoBot[b]\] Warning :: $function - $message"
}

####
# GetFtpDaemon
#
# Retrieve the FTP daemon name.
#
proc ::Bot::GetFtpDaemon {} {
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
proc ::Bot::SetFtpDaemon {name} {
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
proc ::Bot::CmdCreate {type name script args} {
    variable cmdNames
    variable cmdPrefix

    # Check arguments.
    if {$type ne "channel" && $type ne "private"} {
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
            default   {error "invalid option \"$option\""}
        }
    }

    # Look up the user-defined prefix.
    foreach {enabled option value} [CmdGetOptions $type $name] {
        if {$option eq "prefix"} {
            set prefix $value; break
        }
    }

    # Bind the command and its aliases.
    set binds [list]
    foreach command $names {
        set command [string tolower "$prefix$command"]

        if {$type eq "channel"} {
            bind pub -|- $command [list [namespace current]::CmdChannelProc $name]
        } elseif {$type eq "private"} {
            bind msg -|- $command [list [namespace current]::CmdPrivateProc $name]
        }
        lappend binds $command
    }

    # List: argDesc cmdDesc category binds script
    set cmdNames([list $type $name]) [list $argDesc $cmdDesc $category $binds $script]
    return
}

####
# CmdGetList
#
# Retrieve a list of commands created with "CmdCreate".
# List: {type command} {argDesc cmdDesc category binds script} ...
#
proc ::Bot::CmdGetList {typePattern namePattern} {
    variable cmdNames
    return [array get cmdNames [list $typePattern $namePattern]]
}

####
# CmdGetOptions
#
# Retrieve a list of flags for the given command.
#
proc ::Bot::CmdGetOptions {type name} {
    variable cmdOptions
    if {[info exists cmdOptions([list $type $name])]} {
        return $cmdOptions([list $type $name])
    }
    return [list]
}

####
# CmdSendHelp
#
# Send a command help message to the specified target.
#
proc ::Bot::CmdSendHelp {dest type name {message ""}} {
    global lastbind
    variable cmdNames

    if {![info exists cmdNames([list $type $name])]} {
        error "invalid command type \"$type\" or name \"$name\""
    }

    set argDesc [lindex $cmdNames([list $type $name]) 0]
    if {$message ne ""} {
        SendTargetTheme "PRIVMSG $dest" commandHelp  [list $argDesc $lastbind $message]
    } else {
        SendTargetTheme "PRIVMSG $dest" commandUsage [list $argDesc $lastbind]
    }
}

####
# CmdRemove
#
# Remove a channel command created with "CmdCreate".
#
proc ::Bot::CmdRemove {type name} {
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
        } elseif {$type eq "private"} {
            catch {unbind msg -|- $command [list [namespace current]::CmdPrivateProc $name]}
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
proc ::Bot::CmdChannelProc {command user host handle channel text} {
    variable cmdNames
    set target "PRIVMSG $channel"

    foreach {enabled name value} [CmdGetOptions "channel" $command] {
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
            # Break on first match.
            break
        }
    }
    set argv [ListParse $text]

    # Eval is used to expand arguments to the callback procedure,
    # e.g. "CmdCreate channel !foo [list ChanFooCmd abc 123]".
    set script [lindex $cmdNames([list "channel" $command]) 4]

    if {[catch {eval $script [list $target $user $host $channel $argv]} message]} {
        global errorCode errorInfo

        if {$errorCode eq "CMDHELP" || $errorCode eq "GETOPT"} {
            CmdSendHelp $channel "channel" $command $message
        } else {
            # Unhandled error code.
            LogError CmdChannel "Error evaluating \"$script\":\n$errorInfo"
        }
    }
    return
}

####
# CmdPrivateProc
#
# Wrapper for Eggdrop "bind msg" commands. Parses the text argument into a
# Tcl list and provides more information when a command evaluation fails.
#
proc ::Bot::CmdPrivateProc {command user host handle text} {
    variable cmdNames
    set target "PRIVMSG $user"

    foreach {enabled name value} [CmdGetOptions "private" $command] {
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
            # Break on first match.
            break
        }
    }
    set argv [ListParse $text]

    # Eval is used to expand arguments to the callback procedure,
    # e.g. "CmdCreate private !foo [list MessageFooCmd abc 123]".
    set script [lindex $cmdNames([list "private" $command]) 4]

    if {[catch {eval $script [list $target $user $host $argv]} message]} {
        global errorCode errorInfo

        if {$errorCode eq "CMDHELP" || $errorCode eq "GETOPT"} {
            CmdSendHelp $user "private" $command $message
        } else {
            # Unhandled error code.
            LogError CmdMessage "Error evaluating \"$script\":\n$errorInfo"
        }
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
proc ::Bot::FlagGetValue {flagList flagName valueVar} {
    upvar $valueVar value
    foreach flag $flagList {
        # Parse: [!]<name>=<value>
        if {[regexp -- {^(?:!?)(\w+)=(.+)$} $flag dummy name result] && $name eq $flagName} {
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
proc ::Bot::FlagExists {flagList flagName} {
    foreach flag $flagList {
        # Parse: [!]<name>[=<value>]
        if {[regexp -- {^(?:!?)(\w+)} $flag dummy name] && $name eq $flagName} {
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
proc ::Bot::FlagIsDisabled {flagList flagName} {
    foreach flag $flagList {
        # Parse: [!]<name>[=<value>]
        if {![regexp -- {^(!?)(\w+)} $flag dummy prefix name]} {continue}

        if {$name eq "all" || $name eq $flagName} {
            return [string equal $prefix "!"]
        }
    }
    return 0
}

####
# FlagIsEnabled
#
# Check if the given flag exists and is enabled.
#
proc ::Bot::FlagIsEnabled {flagList flagName} {
    foreach flag $flagList {
        # Parse: [!]<name>[=<value>]
        if {![regexp -- {^(!?)(\w+)} $flag dummy prefix name]} {continue}

        if {$name eq "all" || $name eq $flagName} {
            return [string equal $prefix ""]
        }
    }
    return 0
}

####
# FlagCheckEvent
#
# Check if the given event is enabled in the flag list.
#
proc ::Bot::FlagCheckEvent {flagList event} {
    variable events
    foreach flag $flagList {
        # Parse: [!]<name>[=<value>]
        if {![regexp -- {^(!?)(\w+)} $flag dummy prefix name]} {continue}

        if {$name eq "all" || $name eq $event || ([info exists events($name)] &&
                [lsearch -exact $events($name) $event] != -1)} {
            return [string equal $prefix ""]
        }
    }
    return 0
}

####
# FlagCheckSection
#
# Check if the given event is enabled in a channel or path section.
#
proc ::Bot::FlagCheckSection {section event} {
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
proc ::Bot::ModuleFind {modName} {
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
proc ::Bot::ModuleHash {filePath} {
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
proc ::Bot::ModuleInfo {option args} {
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
proc ::Bot::ModuleLoad {modName} {
    variable modules

    # Locate the module and read its definition file.
    set modPath [ModuleFind $modName]
    ModuleLoadEx $modName [ModuleRead [file join $modPath "module.def"]]
}

####
# ModuleLoadEx
#
# Performs the actual module initialisation. This procedure leaves dependency
# and parameter checks up to the caller. Module writers are strongly encouraged
# to use ModuleLoad instead.
#
proc ::Bot::ModuleLoadEx {modName modInfoList} {
    variable modules
    array set modInfo $modInfoList

    # Refuse to load the module if a dependency is not present.
    foreach modDepend $modInfo(depends) {
        if {![info exists modules($modDepend)]} {
            error "missing module dependency \"$modDepend\""
        }
    }

    set firstLoad 1
    if {[llength $modInfo(tclFiles)]} {
        # Check if a full module initialisation is required.
        if {[info exists modules($modName)]} {
            if {$modInfo(tclFiles) eq [lindex $modules($modName) 4]} {
                set firstLoad 0
            } else {
                LogDebug ModuleLoadEx "File checksums changed, reloading module."
                catch {ModuleUnload $modName}
            }
        }

        # Source all script files.
        foreach {name hash} $modInfo(tclFiles) {
            set path [file join $modInfo(location) $name]
            if {[catch {source $path} message]} {
                error "can't source file \"$path\": $message"
            }
        }
    }

    # Remove trailing namespace identifiers from the context.
    set modInfo(context) [string trimright $modInfo(context) ":"]

    set modules($modName) [list $modInfo(desc) $modInfo(context) $modInfo(depends) \
        $modInfo(location) $modInfo(tclFiles) $modInfo(varFiles)]

    if {[llength $modInfo(tclFiles)]} {
        if {[catch {${modInfo(context)}::Load $firstLoad} message]} {
            # Unload the module since initialisation failed.
            if {[catch {ModuleUnload $modName} unloadMsg]} {
                LogDebug ModuleLoadEx "Unable to unload \"$modName\": $message"
            }
            error "initialisation failed: $message"
        }
    }

    foreach {name hash} $modInfo(varFiles) {
        VarLoad [file join $modInfo(location) $name]
    }
}

####
# ModuleUnload
#
# Unload and finalise the given module. An error is raised if the
# module is not loaded or an unexpected error occurs while unloading it.
#
proc ::Bot::ModuleUnload {modName} {
    variable modules

    if {![info exists modules($modName)]} {
        error "module not loaded"
    }
    foreach {desc context depends location tclFiles varFiles} $modules($modName) {break}

    if {[llength $tclFiles]} {
        set failed [catch {${context}::Unload} message]

        # The namespace context must only be deleted if it's
        # not the global namespace or the bot's namespace.
        if {$context ne "" && $context ne [namespace current]} {
            catch {namespace delete $context}
        }
    } else {
        set failed 0
    }

    unset modules($modName)

    # Raise an error after cleaning up the module.
    if {$failed} {error "finalisation failed: $message"}
}

####
# ModuleRead
#
# Read a module definition file. The module information is returned in a key
# and value paired list, in the same manner that "array get" does. An error is
# raised if the module definition file cannot be read or is missing critical
# information.
#
proc ::Bot::ModuleRead {filePath} {
    set location [file dirname $filePath]
    set required [list desc context depends tclFiles varFiles]

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
    foreach option [array names modInfo] {
        set required [ListRemove $required $option]
    }
    if {[llength $required]} {
        error "missing required module information: [ListConvert $required]"
    }

    # Resolve and hash all script files.
    set tclFiles [list]
    foreach name [ListParse $modInfo(tclFiles)] {
        set path [file join $location $name]
        if {![file isfile $path]} {
            error "the script file \"$path\" does not exist"
        }
        lappend tclFiles $name [ModuleHash $path]
    }

    # Resolve and hash all variable files.
    set varFiles [list]
    foreach name [ListParse $modInfo(varFiles)] {
        set path [file join $location $name]
        if {![file isfile $path]} {
            error "the variable file \"$path\" does not exist"
        }
        lappend varFiles $name [ModuleHash $path]
    }

    # Update module information.
    set modInfo(location) $location
    set modInfo(tclFiles) $tclFiles
    set modInfo(varFiles) $varFiles

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
proc ::Bot::ScriptExecute {type event args} {
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
proc ::Bot::ScriptRegister {type event script {alwaysExec 0}} {
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
proc ::Bot::ScriptUnregister {type event script} {
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
proc ::Bot::GetFlagsFromSection {section} {
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
proc ::Bot::GetSectionFromEvent {section event} {
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
proc ::Bot::GetSectionFromPath {fullPath} {
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
proc ::Bot::SendSection {section text} {
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
}

####
# SendSectionTheme
#
# Replace theme values and send the text to all channels for the given
# channel or path section.
#
proc ::Bot::SendSectionTheme {section type {valueList ""}} {
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
proc ::Bot::SendTarget {target text} {
    foreach line [split $text "\n"] {
        putserv "$target :$line"
    }
}

####
# SendTargetTheme
#
# Replace theme values and send the text to the given target.
#
proc ::Bot::SendTargetTheme {target type {valueList ""} {section ""}} {
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
# Variables                                                                    #
################################################################################

####
# VarLoad
#
# Loads a variable definition file.
#
proc ::Bot::VarLoad {filePath} {
    variable events
    variable replace
    variable variables

    set handle [Config::Open $filePath]
    Config::Read $handle

    foreach {name value} [Config::GetEx $handle Events] {
        # Allow underscores for convenience.
        if {![string is wordchar -strict $name]} {
            LogError VarLoad "Invalid event group name \"$name\" in \"$filePath\": must be alphanumeric."
            continue
        }

        # Several categories have multiple events defined. Therefore,
        # the config value must be appended to, not replaced.
        eval lappend [list events($name)] $value
    }

    array set replace [Config::GetEx $handle Replace]
    array set variables [Config::GetEx $handle Variables]
    Config::Close $handle
}

####
# VarFormat
#
# Formats a given variable according to its substitution type.
#
proc ::Bot::VarFormat {valueVar name type width precision} {
    variable sizeDivisor
    upvar $valueVar value

    # Type of variable substitution. Error checking is not performed
    # for all types because of the performance implications in doing so.
    switch -- $type {
        b {set value [FormatSize [expr {double($value) / double($sizeDivisor)}]]}
        d {set value [FormatDuration $value]}
        e {set value [subst -nocommands -novariables $value]}
        k {set value [FormatSize $value]}
        m {set value [FormatSize [expr {double($value) * double($sizeDivisor)}]]}
        s {set value [FormatSpeed $value]}
        p -
        P -
        t {return 0}
        n {
            # Integer or floating point.
            if {[string is integer -strict $value]} {
                set value [format "%${width}d" $value]
                return 1
            } elseif {[string is double -strict $value]} {
                set value [format "%${width}.${precision}f" $value]
                return 1
            }
            LogWarning VarFormat "Invalid numerical value \"$value\" for \"$name\"."
        }
        z {
            variable replace
            if {![string length $value] && [info exists replace($name)]} {
                set value $replace($name)
            }
        }
        default {LogWarning VarFormat "Unknown substitution type \"$type\" for \"$name\"."}
    }

    set value [format "%${width}.${precision}s" $value]
    return 1
}

####
# VarReplace
#
# Substitute a list of variables and values in a given string.
#
proc ::Bot::VarReplace {input varList valueList} {
    set inputLength [string length $input]
    set output ""

    set index 0
    foreach varName $varList value $valueList {
        if {[string match {*:[Ppt]} $varName]} {
            set type [string index $varName end]
            set varName [string range $varName 0 end-2]

            if {$type eq "P" || $type eq "p"} {
                lappend varList ${varName}Dir:z ${varName}Full:z ${varName}Name:z ${varName}Path:z
                eval lappend valueList [PathParseSection $value [string equal $type "P"]]
            } elseif {$type eq "t"} {
                lappend varList ${varName}Date:z ${varName}Time:z
                lappend valueList [FormatDate $value] [FormatTime $value]
            }
        } elseif {[llength $varName] > 1} {
            variable theme
            variable variables

            set value [ListParse $value]
            foreach {varName loopName} $varName {
                set joinName "${loopName}_JOIN"
                if {![info exists theme($loopName)] || ![info exists theme($joinName)]} {
                    LogError VarReplace "Missing theme definition for \"$loopName\" or \"$joinName\"."
                    continue
                }

                set data [list]
                set varCount [llength $variables($loopName)]
                set valueCount [llength $value]
                for {set i 0} {$i < $valueCount} {incr i $varCount} {
                    set values [lrange $value $i [expr {$i + $varCount - 1}]]
                    lappend data [VarReplace $theme($loopName) $variables($loopName) $values]
                }
                lappend varList $varName
                lappend valueList [join $data $theme($joinName)]
            }
        } else {
            incr index; continue
        }

        # Remove the original variable and its value.
        set varList [lreplace $varList $index $index]
        set valueList [lreplace $valueList $index $index]
    }

    for {set inputIndex 0} {$inputIndex < $inputLength} {incr inputIndex} {
        if {[string index $input $inputIndex] ne "%"} {
            append output [string index $input $inputIndex]
        } else {
            set startIndex $inputIndex

            # Find the width field (before dot).
            set beforeIndex [incr inputIndex]
            if {[string index $input $inputIndex] eq "-"} {
                # Ignore the negative sign if a number does not follow, e.g. %-(variable).
                if {[string is digit -strict [string index $input [incr inputIndex]]]} {
                    incr inputIndex
                } else {incr beforeIndex}
            }
            while {[string is digit -strict [string index $input $inputIndex]]} {incr inputIndex}
            if {$beforeIndex != $inputIndex} {
                set width [string range $input $beforeIndex [expr {$inputIndex - 1}]]
            } else {
                set width 0
            }

            # Find the precision field (after dot).
            if {[string index $input $inputIndex] eq "."} {
                set beforeIndex [incr inputIndex]
                # Ignore the negative sign, ex: %.-(variable).
                if {[string index $input $inputIndex] eq "-"} {incr beforeIndex; incr inputIndex}
                while {[string is digit -strict [string index $input $inputIndex]]} {incr inputIndex}
                if {$beforeIndex != $inputIndex} {
                    set precision [string range $input $beforeIndex [expr {$inputIndex - 1}]]
                } else {
                    set precision 0
                }
            } else {
                # Tcl's format function does not accept -1 for the precision field
                # like printf() does, so a reasonably large number will suffice.
                set precision 9999
            }

            # Find the variable name.
            if {[string index $input $inputIndex] eq "("} {
                set beforeIndex [incr inputIndex]
                while {[string index $input $inputIndex] ne ")" && $inputIndex <= $inputLength} {incr inputIndex}
                set varName [string range $input $beforeIndex [expr {$inputIndex - 1}]]
            } else {
                # Invalid variable format, an opening parenthesis is expected.
                append output [string range $input $startIndex $inputIndex]
                continue
            }

            # Variable names must be composed of alphanumeric
            # and/or connector characters (a-z, A-Z, 0-9, and _).
            if {[string is wordchar -strict $varName] && [set valueIndex [lsearch -glob $varList ${varName}:?]] != -1} {
                set value [lindex $valueList $valueIndex]
                set type [string index [lindex $varList $valueIndex] end]

                if {[VarFormat value $varName $type $width $precision]} {
                    append output $value; continue
                }
            }

            # Unknown variable name, the string will still appended to
            # notify the user of their mistake (most likely a typo).
            append output [string range $input $startIndex $inputIndex]
        }
    }
    return $output
}

####
# VarReplaceBase
#
# Replaces static content and control codes (i.e. bold, colour, and underline).
#
proc ::Bot::VarReplaceBase {text {doPrefix 1}} {
    # Replace static variables.
    set vars {siteName:z siteTag:z}
    set values [list $::Bot::siteName $::Bot::siteTag]

    if {$doPrefix} {
        lappend vars prefix:z
        lappend values $::Bot::format(prefix)
    }
    set text [VarReplace $text $vars $values]

    # Mapping for control code replacement.
    set map [list {[b]} [b] {[c]} [c] {[o]} [o] {[r]} [r] {[u]} [u]]
    return [subst -nocommands -novariables [string map $map $text]]
}

####
# VarReplaceCommon
#
# Replace dynamic content, such as the current date, time, and section colours.
#
proc ::Bot::VarReplaceCommon {text section} {
    variable colours
    variable defaultSection

    set vars [list now:t]
    set values [list [clock seconds]]
    if {$section eq ""} {
        # If no section is specified, use the default section.
        set section $defaultSection
    } else {
        lappend vars "section:z"
        lappend values $section

        if {![info exists colours($section)]} {
            # If there are no colours available for the current
            # section, use the default section's colours.
            set section $defaultSection
        }
    }

    if {[info exists colours($section)]} {
        set text [string map $colours($section) $text]
    } else {
        LogDebug VarReplaceCommon "No section colours defined for \"$section\"."
    }
    return [VarReplace $text $vars $values]
}

################################################################################
# DCC Admin Command                                                            #
################################################################################

# Command aliases.
bind dcc n "alc"        ::Bot::DccAdmin
bind dcc n "alco"       ::Bot::DccAdmin
bind dcc n "alcobot"    ::Bot::DccAdmin

####
# DccAdmin
#
# Bot administration command, used from Eggdrop's party-line.
#
proc ::Bot::DccAdmin {handle idx text} {
    variable scriptPath

    set argv [ListParse $text]
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
            foreach {desc context depends location tclFiles varFiles} $modules($name) {break}
            putdcc $idx "$name - [b]Info:[b] $desc [b]Depends:[b] [ListConvert $depends] [b]Path:[b] $location"
        }

        putdcc $idx "[b]Sections:[b]"
        foreach name [lsort [array names chanSections]] {
            foreach {channels flags} $chanSections($name) {break}
            putdcc $idx "$name - [b]Channels:[b] [ListConvert $channels] [b]Flags:[b] $flags"
        }
        foreach name [lsort [array names pathSections]] {
            foreach {path channels flags} $pathSections($name) {break}
            putdcc $idx "$name - [b]Channels:[b] [ListConvert $channels] [b]Flags:[b] $flags [b]Path:[b] $path"
        }

        putdcc $idx "[b]Scripts:[b]"
        foreach name [lsort [array names scripts]] {
            set scriptList [list]
            foreach {proc always} $scripts($name) {
                lappend scriptList "$proc ([expr {$always ? {True} : {False}}])"
            }

            foreach {type name} $name {break}
            putdcc $idx "$type $name - [ListConvert $scriptList]"
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
proc ::Bot::InitConfig {filePath} {
    variable configFile
    variable configHandle
    variable defaultSection
    variable cmdOptions
    variable chanSections
    variable pathSections
    unset -nocomplain cmdOptions chanSections pathSections

    # Update configuration path before reading the file.
    set configFile $filePath
    if {[info exists configHandle]} {
        Config::Change $configHandle -path $configFile
    } else {
        set configHandle [Config::Open $configFile -align 2]
    }
    Config::Read $configHandle

    foreach option {cmdPrefix debugMode localTime ftpDaemon siteName siteTag} {
        variable $option [Config::Get $configHandle General $option]
    }
    set debugMode [IsTrue $debugMode]
    set localTime [IsTrue $localTime]

    if {[catch {SetFtpDaemon $ftpDaemon} message]} {
        error "Unable to set FTP daemon: $message"
    }

    # Read command options.
    foreach {name value} [Config::GetEx $configHandle Commands] {
        set nameSplit [split $name "."]
        if {[llength $nameSplit] != 2} {
            LogWarning Config "Invalid command definition \"$name\"."

        } elseif {[lsearch -exact {channel private} [lindex $nameSplit 0]] == -1} {
            LogWarning Config "Invalid command type \"[lindex $nameSplit 0]\" for \"$name\"."

        } else {
            set options [list]
            foreach entry [ListParse $value] {
                # Parse options into a list.
                if {[regexp -- {^(!?)(\w+)=?(.*)$} $entry dummy prefix option value]} {
                    set enabled [expr {$prefix eq "!" ? 0 : 1}]
                    lappend options $enabled $option $value
                } else {
                    LogWarning Config "Invalid option for command \"$name\": $entry"
                }
            }
            set cmdOptions($nameSplit) $options
        }
    }

    # Read channel and path sections.
    foreach {name value} [Config::GetEx $configHandle Sections] {
        set options [ListParse $value]
        if {[llength $options] == 2} {
            set chanSections($name) $options

        } elseif {[llength $options] == 3} {
            set pathSections($name) $options

        } else {
            LogWarning Config "Wrong number of options for section \"$name\"."
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
proc ::Bot::InitLibraries {rootPath} {
    global auto_path

    # Some users reported that "auto_path" was not always set,
    # which is bizarre considering Tcl initialises this variable.
    if {![info exists auto_path] || [lsearch -exact $auto_path $libPath] == -1} {
        # Add the libs directory to the package search path.
        lappend auto_path $libPath
    }

    set libPath [file join $rootPath "libs"]
    foreach script {constants.tcl libUtil.tcl libConfig.tcl libFtp.tcl libGetOpt.tcl libTree.tcl} {
        set script [file join $libPath $script]
        if {[catch {source $script} message]} {
            error "couldn't source script \"$script\": $message"
        }
    }

    package require AlcoExt 0.5
}

####
# InitModules
#
# Load the given modules. Any pre-existing modules that are not listed
# in the "modList" parameter will be unloaded.
#
proc ::Bot::InitModules {modList} {
    variable cmdNames
    variable events
    variable modules
    variable replace
    variable variables

    set prevModules [array names modules]
    array set prevCommands [array get cmdNames]
    unset -nocomplain cmdNames events replace variables

    # Locate and read all module definition files.
    set modInfo [Tree::Create]
    foreach modName $modList {
        if {[catch {
            set defFile [file join [ModuleFind $modName] "module.def"]
            Tree::Set modInfo $modName [ModuleRead $defFile]
        } message]} {
            LogInfo "Unable to load module \"$modName\": $message"
        }
    }

    # Reorder the modules based on their dependencies.
    set loadOrder [Tree::Keys $modInfo]
    foreach modName $loadOrder {
        # The module's index changes as the list is reordered.
        set modIndex [lsearch -exact $loadOrder $modName]

        foreach depName [Tree::Get $modInfo $modName depends] {
            set depIndex [lsearch -exact $loadOrder $depName]

            if {$depIndex > $modIndex} {
                # Move the dependency before the module.
                set loadOrder [lreplace $loadOrder $depIndex $depIndex]
                set loadOrder [linsert $loadOrder $modIndex $depName]
            }
        }

        # The module was requested to be loaded again, remove it
        # from the list of previous modules so its not unloaded.
        set prevModules [ListRemove $prevModules $modName]
    }

    # Remove unreferenced modules.
    foreach modName $prevModules {
        if {[catch {ModuleUnload $modName} message]} {
            LogInfo "Unable to unload module \"$modName\": $message"
        } else {
            LogInfo "Module Unloaded: $modName"
        }
    }

    # Load all modules listed in the configuration file.
    foreach modName $loadOrder {
        if {[catch {ModuleLoadEx $modName [Tree::Get $modInfo $modName]} message]} {
            LogInfo "Unable to load module \"$modName\": $message"
        } else {
            LogInfo "Module Loaded: $modName"
        }
    }

    # Remove unreferenced commands.
    foreach name [array names prevCommands] {
        if {![info exists cmdNames($name)]} {
            set cmdNames($name) $prevCommands($name)
            CmdRemove [lindex $name 0] [lindex $name 1]
        }
    }
}

####
# InitTheme
#
# Read the given theme file.
#
proc ::Bot::InitTheme {themeFile} {
    variable colours
    variable format
    variable theme
    variable scriptPath
    variable variables
    unset -nocomplain colours format theme

    set themeFile [file join $scriptPath "themes" $themeFile]
    set handle [Config::Open $themeFile]
    Config::Read $handle

    # Process colour entries.
    foreach {name value} [Config::GetEx $handle Colour] {
        if {![regexp -- {^(\d+)\.(\S+)$} $name result num section]} {
            LogWarning Theme "Invalid colour entry \"$name\"."

        } elseif {![string is digit -strict $value] || $value < 0 || $value > 15} {
            LogWarning Theme "Invalid colour value \"$value\" for \"$name\", must be from 0 to 15."

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
    foreach {name value} [Config::GetEx $handle Format] {
        set index [lsearch -exact $known $name]
        if {$index != -1} {
            set known [lreplace $known $index $index]

            # Remove quotes around the format value, if present.
            if {[string index $value 0] eq "\"" && [string index $value end] eq "\""} {
                set value [string range $value 1 end-1]
            }
            set format($name) [VarReplaceBase $value 0]
        } else {
            LogDebug Theme "Unknown format type \"$name\"."
        }
    }
    if {[llength $known]} {
        foreach name $known {set format($name) ""}
        LogWarning Theme "Missing format entries: [ListConvert $known]."
    }

    # Process theme entries.
    set known [array names variables]
    foreach {name value} [Config::GetEx $handle Theme] {
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
        LogWarning Theme "Missing theme entries: [ListConvert [lsort $known]]."
    }

    Config::Close $handle
}

####
# InitMain
#
# Loads all bot subsystems. Eggdrop is terminated if a critical initialisation
# stage fails; an error message will be displayed explaining the failure.
#
proc ::Bot::InitMain {} {
    variable configHandle
    variable scriptPath
    LogInfo "Starting..."

    LogInfo "Loading libraries..."
    if {[catch {InitLibraries $scriptPath} message]} {
        LogError Libraries $message
        die
    }

    LogInfo "Loading configuration..."
    set configFile [file join $scriptPath "AlcoBot.conf"]
    if {[catch {InitConfig $configFile} message]} {
        LogError Config $message; die
    }

    LogInfo "Loading modules..."
    set modules [ListParse [Config::Get $configHandle General modules]]
    if {[catch {InitModules $modules} message]} {
        LogError Modules $message; die
    }
    set varFile [file join $scriptPath "AlcoBot.vars"]
    if {[catch {VarLoad $varFile} message]} {
        LogError Variables $message; die
    }
    set themeFile [Config::Get $configHandle General themeFile]
    if {[catch {InitTheme $themeFile} message]} {
        LogError Theme $message; die
    }

    Config::Free $configHandle
    LogInfo "Sitebot loaded, configured for [GetFtpDaemon]."
    return
}

# This Tcl version check is here for a reason - do not remove it.
if {[catch {package require Tcl 8.4} error]} {
    LogError TclVersion "You must be using Tcl v8.4, or newer."
    die
}

::Bot::InitMain
