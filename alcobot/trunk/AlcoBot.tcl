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
    variable cmdCount
    if {![info exists cmdCount]} {set cmdCount 0}
    variable defaultSection "DEFAULT"
    variable ftpDaemon ""
    variable localTime 0
    variable scriptPath [file dirname [file normalize [info script]]]

    namespace export b c u r o LogDebug LogInfo LogError LogWarning GetFtpDaemon \
        CmdCreate CmdGetList CmdGetOptions CmdSendHelp CmdRemove CmdRemoveByToken \
        FlagGetValue FlagExists FlagIsDisabled FlagIsEnabled FlagCheckEvent FlagCheckSection \
        ModuleFind ModuleHash ModuleInfo ModuleLoad ModuleUnload ModuleRead \
        ScriptExecute ScriptRegister ScriptUnregister \
        GetFlagsFromSection GetSectionFromEvent GetSectionFromPath \
        SendSection SendSectionTheme SendTarget SendTargetTheme \
        FormatDate FormatTime FormatDuration FormatDurationLong FormatSize FormatSpeed \
        VarGetEntry VarGetGroups VarGetNames VarFileLoad VarFileUnload VarReplace VarReplaceDynamic VarReplaceStatic
}

#
# Context Arrays:
#
#   chanSections  - Channel sections.
#   cmdNames      - Commands created with "CmdCreate".
#   cmdOptions    - Command options.
#   colours       - Section colour mappings.
#   events        - Events grouped by their function.
#   format        - Text formatting definitions.
#   pathSections  - Path sections.
#   replace       - Static variable replacements.
#   scripts       - Callback scripts.
#   themes        - Theme definitions.
#   variables     - Variable definitions.
#
# Context Variables:
#
#   cmdCount      - Counter incremented each time a command is created.
#   configFile    - Fully qualified path to the configuration file.
#   configHandle  - Handle to the configuration file, valid only during init.
#   ftpDaemon     - FTP daemon name.
#   localTime     - Format time values in local time, otherwise UTC is used.
#   modules       - Loaded modules.
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
    putlog "\[[b]AlcoBot[b]\] Debug :: $function - $message"
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
# Retrieves the FTPD name.
#
proc ::Bot::GetFtpDaemon {} {
    variable ftpDaemon
    return $ftpDaemon
}

####
# SetFtpDaemon
#
# Sets the FTPD module directory (internal use only).
#
proc ::Bot::SetFtpDaemon {name} {
    variable ftpDaemon
    variable scriptPath

    # Create a list of known FTPDs.
    set path [file join $scriptPath modules]
    set known [glob -directory $path -nocomplain -tails -types d "*"]
    set known [ListRemove $known "generic"]

    # Make sure the FTPD has a module directory.
    set name [string tolower $name]
    set ftpDaemon [GetOpt::Element $known $name "FTP daemon"]
}

################################################################################
# Commands                                                                     #
################################################################################

####
# CmdCreate
#
# Create and register a command.
#
proc ::Bot::CmdCreate {type name script args} {
    variable cmdCount
    variable cmdNames
    variable cmdPrefix

    # Check arguments.
    if {$type ne "channel" && $type ne "private"} {
        error "invalid command type \"$type\""
    }

    set argDesc  ""
    set cmdDesc  ""
    set category ""
    set prefixes [list]
    set suffixes [list $name]

    foreach {option value} $args {
        switch -- $option {
            -args     {set argDesc $value}
            -category {set category $value}
            -desc     {set cmdDesc $value}
            default   {error "invalid option \"$option\""}
        }
    }

    # Look up user-defined command prefixes.
    foreach {enabled option value} [CmdGetOptions $type $name] {
        if {$option eq "prefix"} {
            lappend prefixes $value
        } elseif {$option eq "suffix"} {
            lappend suffixes $value
        }
    }
    if {![llength $prefixes]} {
        # No custom prefixes defined, use the default one.
        set prefixes [list $cmdPrefix]
    }

    # Bind the command and its aliases.
    set binds [list]
    foreach prefix $prefixes {
        foreach suffix $suffixes {
            set command [string tolower "$prefix$suffix"]

            if {$type eq "channel"} {
                bind pub -|- $command [list [namespace current]::CmdChannelProc $name]
            } elseif {$type eq "private"} {
                bind msg -|- $command [list [namespace current]::CmdPrivateProc $name]
            }
            lappend binds $command
        }
    }

    # List: argDesc cmdDesc category binds script token
    set token [format "cmd%d" $cmdCount]; incr cmdCount
    set cmdNames([list $type $name]) [list $argDesc $cmdDesc $category $binds $script $token]
    return $token
}

####
# CmdGetList
#
# Retrieve a list of commands created with "CmdCreate".
# List: {type command} {argDesc cmdDesc category binds script token} ...
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
        SendTargetTheme "PRIVMSG $dest" Core commandHelp  [list $argDesc $lastbind $message]
    } else {
        SendTargetTheme "PRIVMSG $dest" Core commandUsage [list $argDesc $lastbind]
    }
}

####
# CmdRemove
#
# Remove a command.
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
# CmdRemoveByToken
#
# Remove a command by it's unique identifier.
#
proc ::Bot::CmdRemoveByToken {token} {
    variable cmdNames
    foreach {name data} [array get cmdNames] {
        if {$token eq [lindex $data 5]} {
            eval CmdRemove $name; return
        }
    }
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

    foreach modDir [list "generic" $ftpDaemon] {
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
    set handle [open $filePath r]
    fconfigure $handle -translation binary

    set hash [crypt start md5]
    while {![eof $handle]} {
        crypt update $hash [read $handle 8192]
    }
    close $handle
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
            return [Tree::Exists $modules [lindex $args 0]]
        }
        list {
            if {$argc == 0} {
                return [Tree::Keys $modules]
            }
            if {$argc == 1} {
                return [Tree::Keys $modules [lindex $args 0]]
            }
            error "wrong # args: must be \"ModuleInfo list ?pattern?\""
        }
        query {
            if {$argc != 1} {
                error "wrong # args: must be \"ModuleInfo query moduleName\""
            }
            set modName [lindex $args 0]
            if {![Tree::Exists $modules $modName]} {
                error "module not loaded"
            }
            return [Tree::Get $modules $modName]
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
    set modPath [ModuleFind $modName]
    ModuleLoadEx $modName [ModuleRead [file join $modPath "module.def"]]
}

####
# ModuleLoadEx
#
# Performs the actual module initialisation and loading procedures.
#
proc ::Bot::ModuleLoadEx {modName modDefinition} {
    variable modules
    array set module $modDefinition

    # Refuse to load the module if a dependency is not present.
    foreach depName $module(depends) {
        if {![Tree::Exists $modules $depName]} {
            error "missing module dependency \"$depName\""
        }
    }

    # Check if a full module initialisation is required.
    set firstLoad 1
    if {[Tree::Exists $modules $modName]} {
        array set prev [Tree::Get $modules $modName]

        if {$module(tclFiles) eq $prev(tclFiles) && $module(varFiles) eq $prev(varFiles)} {
            set firstLoad 0
            set module(refCount) $prev(refCount)
        } else {
            LogDebug ModuleLoadEx "File checksums changed, reloading module."
            if {[catch {ModuleUnload $modName} debug]} {
                LogDebug ModuleLoadEx "Unable to unload \"$modName\": debug"
            }
        }
    }

    if {$firstLoad} {
        set module(refCount) 0
    } elseif {$module(refCount) < 0} {
        # Reset the reference count if it's less than zero.
        set module(refCount) 0
    }
    set module(context) [string trimright $module(context) ":"]
    Tree::Set modules $modName [array get module]

    if {[catch {
        if {$firstLoad} {
            # Read all script files.
            foreach {name hash} $module(tclFiles) {
                source [file join $module(location) $name]
            }

            # Read all variable files.
            foreach {name hash} $module(varFiles) {
                VarFileLoad [file join $module(location) $name]
            }
        }

        # Initialise the module.
        if {$module(context) ne "" && [catch {${module(context)}::Load $firstLoad} message]} {
            error "initialisation failed: $message"
        }
    } message]} {
        if {[catch {ModuleUnload $modName} debug]} {
            LogDebug ModuleLoadEx "Unable to unload \"$modName\": $debug"
        }

        # Raise an error after cleaning up the module.
        error $message
    }

    # Increase the reference count of all dependencies.
    if {$firstLoad} {
        foreach depName $module(depends) {
            set refCount [Tree::Get $modules $depName refCount]
            Tree::Set modules $depName refCount [incr refCount]
        }
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

    if {![Tree::Exists $modules $modName]} {
        error "module not loaded"
    }
    array set module [Tree::Get $modules $modName]

    if {$module(refCount) > 0} {
        error "module still referenced"
    }

    # Decrease the reference count of all dependencies.
    foreach depName $module(depends) {
        if {[Tree::Exists $modules $depName]} {
            set refCount [Tree::Get $modules $depName refCount]
            Tree::Set modules $depName refCount [incr refCount -1]
        }
    }

    # Unload script files.
    if {$module(context) ne ""} {
        set failed [catch {${module(context)}::Unload} message]

        # Do not delete the bot's namespace.
        if {$module(context) ne [namespace current]} {
            catch {namespace delete $module(context)}
        }
    } else {
        set failed 0
    }

    # Unload variable files.
    foreach {name hash} $module(varFiles) {
        set path [file join $module(location) $name]
        if {[catch {VarFileUnload $path} debug]} {
            LogDebug ModuleUnload "Unable to unload variable file \"$name\": $debug"
        }
    }

    Tree::Unset modules $modName

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
    set required {desc context depends tclFiles varFiles}

    set handle [open $filePath r]
    foreach line [split [read $handle] "\n"] {
        if {[string index $line 0] eq "#" || [set index [string first "=" $line]] == -1} {
            continue
        }
        set option [string range $line 0 [expr {$index - 1}]]

        if {[lsearch -exact $required $option] != -1} {
            set module($option) [string range $line [incr index] end]
        }
    }
    close $handle

    # Perform all module information checks after the file was
    # read, in case the same option was defined multiple times.
    foreach option [array names module] {
        set required [ListRemove $required $option]
    }
    if {[llength $required]} {
        error "missing required module information: [ListConvert $required]"
    }

    # Resolve and hash all script files.
    set tclFiles [list]
    foreach name [ListParse $module(tclFiles)] {
        set path [file join $location $name]
        if {![file isfile $path]} {
            error "the script file \"$path\" does not exist"
        }
        lappend tclFiles $name [ModuleHash $path]
    }

    # Resolve and hash all variable files.
    set varFiles [list]
    foreach name [ListParse $module(varFiles)] {
        set path [file join $location $name]
        if {![file isfile $path]} {
            error "the variable file \"$path\" does not exist"
        }
        lappend varFiles $name [ModuleHash $path]
    }

    # Update module information.
    set module(location) $location
    set module(tclFiles) $tclFiles
    set module(varFiles) $varFiles

    return [array get module]
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
proc ::Bot::SendSectionTheme {section themeGroup themeName {valueList ""}} {
    variable themes
    variable variables

    set name [list $themeGroup $themeName]
    if {![info exists themes($name)] || ![info exists variables($name)]} {
        LogError SendSectionTheme "Missing theme or variable definition for \"$themeName\" in \"$themeGroup\"."
        return
    }

    set text [VarReplaceDynamic $themes($name) $section]
    SendSection $section [VarReplace $text $variables($name) $valueList]
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
proc ::Bot::SendTargetTheme {target themeGroup themeName {valueList ""}} {
    variable themes
    variable variables

    set name [list $themeGroup $themeName]
    if {![info exists themes($name)] || ![info exists variables($name)]} {
        LogError SendTargetTheme "Missing theme or variable definition for \"$themeName\" in \"$themeGroup\"."
        return
    }

    # Replace section colours and common items before the value list, in
    # case the values introduce colour codes (e.g. a user named "[b]ill",
    # since "[b]" is also the bold code).
    set text [VarReplaceDynamic $themes($name)]
    SendTarget $target [VarReplace $text $variables($name) $valueList]
}

################################################################################
# Formatting                                                                   #
################################################################################

####
# FormatDate
#
# Formats an integer time value into a human-readable date. If a time value
# is not given, the current date will be used.
#
proc ::Bot::FormatDate {{clockVal ""}} {
    variable format
    variable localTime

    if {![string is digit -strict $clockVal]} {
        set clockVal [clock seconds]
    }
    return [clock format $clockVal -format $format(date) -gmt [expr {!$localTime}]]
}

####
# FormatTime
#
# Formats an integer time value into a human-readable time. If a time value
# is not given, the current time will be used.
#
proc ::Bot::FormatTime {{clockVal ""}} {
    variable format
    variable localTime

    if {![string is digit -strict $clockVal]} {
        set clockVal [clock seconds]
    }
    return [clock format $clockVal -format $format(time) -gmt [expr {!$localTime}]]
}

####
# FormatDuration
#
# Formats a time duration into a human-readable format.
#
proc ::Bot::FormatDuration {seconds} {
    if {$seconds < 0} {
        set seconds [expr {-$seconds}]
    }
    set duration [list]
    foreach div {31536000 604800 86400 3600 60 1} unit {y w d h m s} {
        set num [expr {$seconds / $div}]
        if {$num > 0} {lappend duration "[b]$num[b]$unit"}
        set seconds [expr {$seconds % $div}]
    }
    if {[llength $duration]} {return [join $duration]} else {return "[b]0[b]s"}
}

####
# FormatDurationLong
#
# Formats a time duration into a human-readable format.
#
proc ::Bot::FormatDurationLong {seconds} {
    if {$seconds < 0} {
        set seconds [expr {-$seconds}]
    }
    set duration [list]
    foreach div {31536000 604800 86400 3600 60 1} unit {year week day hour min sec} {
        set num [expr {$seconds / $div}]
        if {$num > 1} {
            lappend duration "[b]$num[b] ${unit}s"
        } elseif {$num == 1} {
            lappend duration "[b]$num[b] $unit"
        }
        set seconds [expr {$seconds % $div}]
    }
    if {[llength $duration]} {return [join $duration {, }]} else {return "[b]0[b] secs"}
}

####
# FormatSize
#
# Formats a value in kilobytes into a human-readable amount.
#
proc ::Bot::FormatSize {size} {
    variable format
    variable sizeDivisor
    foreach unit {sizeKilo sizeMega sizeGiga sizeTera} {
        if {abs($size) < $sizeDivisor} {break}
        set size [expr {double($size) / double($sizeDivisor)}]
    }
    return [VarReplace $format($unit) "size:n" $size]
}

####
# FormatSpeed
#
# Formats a value in kilobytes per second into a human-readable speed.
#
proc ::Bot::FormatSpeed {speed {seconds 0}} {
    variable format
    variable speedDivisor
    if {$seconds > 0} {set speed [expr {double($speed) / $seconds}]}
    foreach unit {speedKilo speedMega speedGiga} {
        if {abs($speed) < $speedDivisor} {break}
        set speed [expr {double($speed) / double($speedDivisor)}]
    }
    return [VarReplace $format($unit) "speed:n" $speed]
}

################################################################################
# Variables                                                                    #
################################################################################

####
# VarGetEntry
#
# Retrieves a variable definition.
#
proc ::Bot::VarGetEntry {varGroup varName} {
    variable variables

    set name [list $varGroup $varName]
    if {![info exists variables($name)]} {
        error "no variable definition for \"$varName\" in \"$varGroup\""
    }
    return $variables($name)
}

####
# VarGetGroups
#
# Retrieves a list of variable groups.
#
proc ::Bot::VarGetGroups {{groupPattern "*"}} {
    variable variables

    set result [list]
    foreach name [array names variables [list $groupPattern "*"]] {
        set name [lindex $name 0]
        if {[lsearch -exact $result $name] == -1} {
            lappend result $name
        }
    }
    return $result
}

####
# VarGetNames
#
# Retrieves a list of variable names in a group.
#
proc ::Bot::VarGetNames {varGroup {namePattern "*"}} {
    variable variables

    set result [list]
    set varGroup [GlobEscape $varGroup]
    foreach name [array names variables [list $varGroup $namePattern]] {
        lappend result [lindex $name 1]
    }
    return $result
}

####
# VarFileLoad
#
# Loads a variable definition file.
#
proc ::Bot::VarFileLoad {filePath} {
    variable events
    variable replace
    variable variables

    set handle [Config::Open $filePath]
    Config::Read $handle

    foreach {name value} [Config::GetEx $handle Events] {
        # Allow underscores for convenience.
        if {![string is wordchar -strict $name]} {
            LogError VarFileLoad "Invalid event group name \"$name\" in \"$filePath\": must be alphanumeric."
            continue
        }

        # Several categories have multiple events defined. Therefore,
        # the config value must be appended to, not replaced.
        eval lappend [list events($name)] $value
    }

    array set replace [Config::GetEx $handle Replace]

    foreach section [Config::Sections $handle Variables::*] {
        set group [string range $section 11 end]
        foreach {name value} [Config::GetEx $handle $section] {
            set variables([list $group $name]) $value
        }
    }

    Config::Close $handle
}

####
# VarFileUnload
#
# Unloads a variable definition file.
#
proc ::Bot::VarFileUnload {filePath} {
    variable events
    variable replace
    variable variables

    set handle [Config::Open $filePath]
    Config::Read $handle

    foreach {name value} [Config::GetEx $handle Events] {
        if {![info exists events($name)]} {continue}
        foreach event $value {
            set events($name) [ListRemove $events($name) $event]
        }

        # Remove the event group if it's empty.
        if {![llength $events($name)]} {
            unset events($name)
        }
    }

    foreach name [Config::Keys $handle Replace] {
        unset -nocomplain replace($name)
    }

    foreach section [Config::Sections $handle Variables::*] {
        set group [string range $section 11 end]
        foreach name [Config::Keys $handle $section] {
            unset -nocomplain variables([list $group $name])
        }
    }

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
            variable themes
            variable variables

            set value [ListParse $value]
            foreach {varName loopName} $varName {
                set joinName "${loopName}_JOIN"
                if {![info exists themes($loopName)] || ![info exists themes($joinName)]} {
                    LogError VarReplace "Missing theme definition for \"$loopName\" or \"$joinName\"."
                    continue
                }

                set data [list]
                set varCount [llength $variables($loopName)]
                set valueCount [llength $value]
                for {set i 0} {$i < $valueCount} {incr i $varCount} {
                    set values [lrange $value $i [expr {$i + $varCount - 1}]]
                    lappend data [VarReplace $themes($loopName) $variables($loopName) $values]
                }
                lappend varList $varName
                lappend valueList [join $data $themes($joinName)]
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

            # Variable names must be consist of alphanumeric characters.
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
# VarReplaceDynamic
#
# Replace dynamic content, such as the current date, time, and section colours.
#
proc ::Bot::VarReplaceDynamic {text {section ""}} {
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
        LogDebug VarReplaceDynamic "No section colours defined for \"$section\"."
    }
    return [VarReplace $text $vars $values]
}

####
# VarReplaceStatic
#
# Replaces static content and control codes (i.e. bold, colour, and underline).
#
proc ::Bot::VarReplaceStatic {text {doPrefix 1}} {
    # Replace static variables.
    set vars {siteName:z siteTag:z}
    set values [list $::Bot::siteName $::Bot::siteTag]

    if {$doPrefix} {
        lappend vars prefix:z
        lappend values $::Bot::format(prefix)
    }
    set text [VarReplace $text $vars $values]

    # Mapping for control code replacement.
    set map [list {[b]} \002 {[c]} \003 {[o]} \015 {[r]} \026 {[u]} \037]
    return [subst -nocommands -novariables [string map $map $text]]
}

################################################################################
# DCC Admin Command                                                            #
################################################################################

# Command aliases.
bind dcc n "alc"     ::Bot::DccAdmin
bind dcc n "alco"    ::Bot::DccAdmin
bind dcc n "alcobot" ::Bot::DccAdmin

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
        variable modules
        variable scripts

        putdcc $idx "[b]General:[b]"
        putdcc $idx "Config File: $configFile"
        putdcc $idx "FTP Daemon: [GetFtpDaemon]"
        putdcc $idx "Script Path: $scriptPath"

        putdcc $idx "[b]Commands:[b]"
        foreach name [lsort [array names cmdNames]] {
            foreach {argDesc cmdDesc category binds script token} $cmdNames($name) {break}
            putdcc $idx "[join $name { }] - [b]Binds:[b] [join $binds {, }] [b]Category:[b] $category"
        }

        putdcc $idx "[b]Modules:[b]"
        foreach name [lsort [Tree::Keys $modules]] {
            array set module [Tree::Get $modules $name]
            putdcc $idx "$name - [b]Info:[b] $module(desc) [b]Depends:[b] [ListConvert $module(depends)] [b]Path:[b] $module(location)"
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

    # Retrieve configuration options.
    foreach {name value} [Config::GetMulti $configHandle General cmdPrefix siteName siteTag] {
        variable $name $value
    }
    array set option [Config::GetMulti $configHandle General localTime ftpDaemon]
    SetFtpDaemon $option(ftpDaemon)
    variable localTime [IsTrue $option(localTime)]

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
                if {[regexp -- {^(!?)(\w+)=?(.*)$} $entry dummy prefix optName value]} {
                    set enabled [expr {$prefix eq "!" ? 0 : 1}]
                    lappend options $enabled $optName $value
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
# InitModules
#
# Load the given modules. Any pre-existing modules that are not listed
# in the "modList" parameter will be unloaded.
#
proc ::Bot::InitModules {modList} {
    variable modules
    if {![info exists modules]} {
        set modules [Tree::Create]
    }
    set prevModules [Tree::Keys $modules]

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

    # Reset module reference counts and remove unreferenced modules.
    foreach modName [Tree::Keys $modules] {
        Tree::Set modules $modName refCount -1
    }
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

    # Remove modules with a reference count less than zero (garbage collecting).
    Tree::For {modName modData} $modules {
        if {[Tree::Get $modData refCount] >= 0} {continue}

        if {[catch {ModuleUnload $modName} message]} {
            LogInfo "Unable to unload module \"$modName\": $message"
        } else {
            LogInfo "Module Unloaded: $modName"
        }
    }
}

####
# InitPackages
#
# Load all required Tcl packages.
#
proc ::Bot::InitPackages {rootPath} {
    global auto_path
    set pkgPath [file join $rootPath "packages"]

    # Some users reported that "auto_path" was not always set,
    # which is bizarre considering Tcl initialises this variable.
    if {![info exists auto_path] || [lsearch -exact $auto_path $pkgPath] == -1} {
        # Add the "packages" directory to the search path.
        lappend auto_path $pkgPath
    }

    # Required libraries.
    package require alco::config 1.2
    package require alco::db     1.2
    package require alco::ftp    1.2
    package require alco::getopt 1.2
    package require alco::tree   1.2
    package require alco::uri    1.2
    package require alco::util   1.2

    # Required extensions.
    package require AlcoExt 0.6
}

####
# InitTheme
#
# Read the given theme file.
#
proc ::Bot::InitTheme {themeFile} {
    variable colours
    variable format
    variable themes
    variable variables
    variable scriptPath
    unset -nocomplain colours format themes

    set themeFile [file join $scriptPath "themes" $themeFile]
    set handle [Config::Open $themeFile]
    Config::Read $handle

    # Process colour entries.
    foreach {name value} [Config::GetEx $handle Colour] {
        if {![regexp -- {^(\d+)\.(.+)$} $name result num section]} {
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
        set known [ListRemove $known $name]

        # Remove quotes around the format value, if present.
        if {[string index $value 0] eq "\"" && [string index $value end] eq "\""} {
            set value [string range $value 1 end-1]
        }
        set format($name) [VarReplaceStatic $value 0]
    }
    if {[llength $known]} {
        foreach name $known {set format($name) ""}
        LogWarning Theme "Missing format entries: [ListConvert $known]."
    }

    # Process theme entries.
    set known [array names variables]
    foreach section [Config::Sections $handle Theme::*] {
        set group [string range $section 7 end]

        foreach {name value} [Config::GetEx $handle $section] {
            set name [list $group $name]
            set known [ListRemove $known $name]

            # Remove quotes around the theme value, if present.
            if {[string index $value 0] eq "\"" && [string index $value end] eq "\""} {
                set value [string range $value 1 end-1]
            }
            set themes($name) [VarReplaceStatic $value]
        }
    }
    if {[llength $known]} {
        foreach name $known {set themes($name) ""}
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

    LogInfo "Loading packages..."
    if {[catch {InitPackages $scriptPath} message]} {
        LogError Packages $message; die
    }

    LogInfo "Loading configuration..."
    set path [file join $scriptPath "AlcoBot.conf"]
    if {[catch {InitConfig $path} message]} {
        LogError Config $message; die
    }
    array set option [Config::GetMulti $configHandle General modules themeFile]

    LogInfo "Loading modules..."
    if {[catch {InitModules [ListParse $option(modules)]} message]} {
        LogError Modules $message; die
    }

    LogInfo "Loading variables..."
    if {[catch {VarFileLoad [file join $scriptPath "AlcoBot.vars"]} message]} {
        LogError Variables $message; die
    }

    LogInfo "Loading theme..."
    if {[catch {InitTheme $option(themeFile)} message]} {
        LogError Theme $message; die
    }

    LogInfo "Sitebot loaded, configured for [GetFtpDaemon]."
    return
}

# This Tcl version check is here for a reason - do not remove it.
if {![package vsatisfies [package provide Tcl] 8.4]} {
    LogError TclVersion "You must be using Tcl v8.4, or newer."; die
}

::Bot::InitMain
