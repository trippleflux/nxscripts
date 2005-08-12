#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Bot Core
#
# Author:
#   neoxed (neoxed@gmail.com) April 16, 2005
#
# Abstract:
#   Implements the event handler, module system, and initialisation procedures.
#

namespace eval ::alcoholicz {
    if {![info exists debugMode]} {
        variable debugMode 0
        variable ftpDaemon 0
    }
    variable scriptPath [file dirname [info script]]

    namespace export b c u r o \
        LogDebug LogInfo LogError LogWarning GetFtpDaemon \
        CmdCreate CmdRemove EventExecute EventRegister EventUnregister \
        ModuleFind ModuleHash ModuleInfo ModuleLoad ModuleLoadEx ModuleUnload ModuleRead \
        SendSection SendSectionTheme SendTarget SendTargetTheme
}

################################################################################
# Utilities                                                                    #
################################################################################

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
proc ::alcoholicz::CmdCreate {flags command args} {
    variable cmds
    bind pub $flags $command [list [namespace current]::CmdProc $args]

    set cmds($command) [list $flags $args]
    return
}

####
# CmdRemove
#
# Remove a channel command created with 'CmdCreate'.
#
proc ::alcoholicz::CmdRemove {command} {
    variable cmds
    if {![info exists cmds($command)]} {
        error "unknown command \"$command\""
    }
    foreach {flags script} $cmds($command) {break}
    unbind pub $flags $command [list [namespace current]::CmdProc $script]

    unset cmds($command)
    return
}

####
# CmdProc
#
# Wrapper for Eggdrop 'bind pub' commands. Parses the 'text' argument into a
# Tcl list and provides more information when a command evaluation fails.
#
proc ::alcoholicz::CmdProc {script user host handle channel text} {
    variable targets
    set target "PRIVMSG $channel"

    # Check if the invoked command has a predefined target.
    foreach name [array names targets] {
        if {[string match $name $::lastbind]} {
            LogDebug CmdProc "Matched command \"$::lastbind\" to \"$name\", using target \"$targets($name)\"."
            switch -- $targets($name) {
                notice {set target "NOTICE $user"}
                user   {set target "PRIVMSG $user"}
                default {return}
            }
            break
        }
    }

    set argv [ArgsToList $text]
    if {[catch {eval $script [list $user $host $handle $channel $target [llength $argv] $argv]} message]} {
        LogError CmdProc "Error evaluating \"$script\":\n$::errorInfo"
    }
    return
}

################################################################################
# Events                                                                       #
################################################################################

####
# EventExecute
#
# Run all registered scripts for the specified event type.
#
proc ::alcoholicz::EventExecute {type event args} {
    variable [append varName event_ $type]
    append varName ($event)
    if {![info exists $varName]} {return 1}

    set result 1
    foreach {script alwaysExec} [set $varName] {
        if {!$alwaysExec && !$result} {continue}

        if {[catch {set scriptResult [eval [list $script $event] $args]} message]} {
            LogError EventExecute "Error evaluating callback \"$script\" for $type $event: $message"

        } elseif {[IsFalse $scriptResult]} {
            LogDebug EventExecute "The callback \"$script\" for $type $event returned false."
            set result 0

        } elseif {![IsTrue $scriptResult]} {
            LogWarning EventExecute "The callback \"$script\" for $type $event did not return a boolean value."
        }
    }
    return $result
}

####
# EventRegister
#
# Register an event callback script.
#
proc ::alcoholicz::EventRegister {type event script {alwaysExec 0}} {
    variable [append varName event_ $type]
    append varName ($event)

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
# EventUnregister
#
# Unregister an event callback script.
#
proc ::alcoholicz::EventUnregister {type event script} {
    variable [append varName event_ $type]
    append varName ($event)
    if {![info exists $varName]} {return [list]}
    set scriptList [set $varName]

    set index 0
    foreach {name always} $scriptList {
        if {$name eq $script} {
            return [set $varName [lreplace $scriptList $index [incr index]]]
        }
        incr index 2
    }
    return $scriptList
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
# algorithm has been broken, it is still more than suitable for this purpose.
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
            if {$argc == 1} {
                return [array names modules]
            } elseif {$argc == 2} {
                return [array names modules [lindex $args 0]]
            } else {
                error "wrong # args: must be \"ModuleInfo list ?pattern?\""
            }
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
        default {error "unknown option \"$option\": must be loaded, list, or query"}
    }
}

####
# ModuleLoad
#
# Read and initialise the given module. An error is raised if the module
# cannot be found or an unexpected error occurs while loading it.
#
proc ::alcoholicz::ModuleLoad {modName} {
    # Locate the module and read its definition file.
    set modPath [ModuleFind $modName]
    array set modInfo [ModuleRead [file join $modPath ".module"]]

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
# and parameter checks up to the caller, for reason it is strongly recommended
# that module writers use ModuleLoad instead.
#
proc ::alcoholicz::ModuleLoadEx {modName modInfoList} {
    variable modules
    array set modInfo $modInfoList

    set firstLoad 1
    set fileHash [ModuleHash $modInfo(file)]

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

    if {[catch {source $modInfo(file)} message]} {
        error "can't source file \"$modInfo(file)\": $message"
    }

    # Remove trailing namespace identifiers from the context.
    set modInfo(context) [string trimright $modInfo(context) ":"]
    if {[catch {${modInfo(context)}::Load $firstLoad} message]} {
        error "initialisation failed: $message"
    }

    set modules($modName) [list $modInfo(desc) $modInfo(file) \
        $modInfo(context) $modInfo(depends) $fileHash]

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

    foreach {modDesc modFile modContext modDepends modHash} $modules($modName) {break}
    set failed [catch {${modContext}::Unload} message]

    # The namespace context must only be deleted if it's
    # not the global namespace or the bot's namespace.
    if {$modContext ne "" && $modContext ne [namespace current]} {
        catch {namespace delete $modContext}
    }

    unset modules($modName)

    # Raise an error after cleaning up the module.
    if {$failed} {error "de-initialisation failed: $message"}
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
    set handle [open $filePath r]
    set required [list desc file context depends]

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
        error "missing required module information: [join $required {, }]"
    }
    set modInfo(file) [file join [file dirname $filePath] $modInfo(file)]
    if {![file isfile $modInfo(file)]} {
        error "the file \"$modInfo(file)\" does not exist"
    }

    return [array get modInfo]
}

################################################################################
# Output                                                                       #
################################################################################

####
# SendSection
#
# Send text to all channels for the given section.
#
proc ::alcoholicz::SendSection {section text} {
    variable sections
    if {![info exists sections($section)]} {
        LogError SendSection "Unknown section \"$section\"."
        return
    }
    set text [VarReplaceCommon $text $section]
    foreach channel [lindex $sections($section) 1] {
        putserv "PRIVMSG $channel :$text"
    }
    return
}

####
# SendSectionTheme
#
# Replace theme values and send the text to all channels for the given section.
#
proc ::alcoholicz::SendSectionTheme {section type {values {}}} {
    variable theme
    variable vars
    if {![info exists theme($type)] || ![info exists vars($type)]} {
        LogError SendSectionTheme "Missing theme or vars definition for \"$type\"."
        return
    }
    SendSection $section [VarReplace $theme($type) $vars($type) $values]
}

####
# SendTarget
#
# Send text to the given target.
#
proc ::alcoholicz::SendTarget {target text {section DEFAULT}} {
    putserv [append target " :" [VarReplaceCommon $text $section]]
}

####
# SendTargetTheme
#
# Replace theme values and send the text to the given target.
#
proc ::alcoholicz::SendTargetTheme {target type {values {}} {section DEFAULT}} {
    variable theme
    variable vars
    if {![info exists theme($type)] || ![info exists vars($type)]} {
        LogError SendTargetTheme "Missing theme or vars definition for \"$type\"."
        return
    }
    set text [VarReplace $theme($type) $vars($type) $values]
    SendTarget $target $text $section
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
    variable sections
    variable targets
    unset -nocomplain sections targets

    # Update configuration path before reading the file.
    set configFile $filePath
    if {[info exists configHandle]} {
        ConfigChange $configHandle -path $configFile
    } else {
        set configHandle [ConfigOpen $configFile -align 2]
    }
    ConfigRead $configHandle

    foreach option {cmdPrefix debugMode ftpDaemon siteName siteTag} {
        variable $option [ConfigGet $configHandle General $option]
    }
    set debugMode [IsTrue $debugMode]

    if {[catch {SetFtpDaemon $ftpDaemon} message]} {
        error "Unable to set FTP daemon: $message"
    }

    # Store sections and targets in an array since they are frequently used.
    array set sections [ConfigGetEx $configHandle Sections]

    foreach {command target} [ConfigGetEx $configHandle Targets] {
        # Lazy matching for command targets. Commands are sent to
        # the channel by default, so 'chan*' entries are ignored.
        switch -glob -- [string tolower $target] {
            {chan*} {continue}
            {disable*} {set target disable}
            {not*} {set target notice}
            {user} -
            {priv*} {set target user}
            default {
                LogWarning InitConfig "Invalid command target \"$target\" for \"$command\"."
                continue
            }
        }
        set targets($command) $target
    }
    return
}

####
# InitLibraries
#
# Load all library scripts and Tcl extensions. An error is raised if a
# required script or extension could not be loaded.
#
proc ::alcoholicz::InitLibraries {rootPath} {
    global auto_path
    global tcl_platform

    set libPath [file join $rootPath libs]
    foreach script {constants.tcl common.tcl config.tcl ftp.tcl tree.tcl} {
        set script [file join $libPath $script]
        if {[catch {source $script} message]} {
            error "couldn't source script \"$script\": $message"
        }
    }

    # Add the libs directory to the package search path.
    if {[lsearch -exact $auto_path $libPath] == -1} {
        lappend auto_path $libPath
    }

    # Load the Alcoholicz Tcl extension.
    package require AlcoExt 0.1

    # Load optional packages.
    foreach {name version} {mysqltcl 3.0 sqlite3 3.2 tls 1.5} {
        catch {package require $name $version}
    }
    return
}

####
# InitModules
#
# Load the given modules. Any pre-existing modules that are not listed
# in the "modList" parameter will be unloaded.
#
proc ::alcoholicz::InitModules {modList} {
    variable cmds
    variable modules
    LogInfo "Loading modules..."

    array set prevCmds [array get cmds]
    array set prevModules [array get modules]
    unset -nocomplain cmds modules

    # TODO:
    # - The module load order should reflect their dependency requirements.
    # - Read all module definition files, re-order them, and call ModuleLoadEx.

    foreach modName $modList {
        if {[catch {ModuleLoad $modName} message]} {
            LogInfo "Unable to load module \"$modName\": $message"
        } else {
            LogInfo "Module Loaded: $modName"
            unset -nocomplain prevModules($modName)
        }
    }

    # Remove unreferenced modules.
    foreach modName [array names prevModules] {
        if {[catch {ModuleUnload $modName} message]} {
            LogInfo "Unable to unload module \"$modName\": $message"
        } else {
            LogInfo "Module Unloaded: $modName"
        }
    }

    # Remove unreferenced commands.
    foreach command [array names prevCmds] {
        if {![info exists cmds($command)]} {
            foreach {flags script} $prevCmds($command) {break}
            unbind pub $flags $command [list [namespace current]::CmdProc $script]
            unset prevCmds($command)
        }
    }
    return
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
    variable vars
    unset -nocomplain colours format theme

    set themeFile [file join $scriptPath $themeFile]
    set handle [ConfigOpen $themeFile]
    ConfigRead $handle

    # Process '[Colour]' entries.
    foreach {name value} [ConfigGetEx $handle Colour] {
        if {![regexp {^(\S+),(\d+)$} $name result section num]} {
            LogWarning InitTheme "Invalid colour entry \"$name\"."
        } elseif {![string is digit -strict $value] || $value < 0 || $value > 15} {
            LogWarning InitTheme "Invalid colour value \"$value\" for \"$name\", must be from 0 to 15."
        } else {
            # Create a mapping of section colours to use with 'string map'. The
            # colour index must be zero-padded to two digits to avoid conflicts
            # with other numerical chars. Note that the %s identifier is used
            # instead of %d to avoid octal interpretation (e.g. format %d 08).
            lappend colours($section) "\[c$num\]" [format "\003%02s" $value]
        }
    }

    # Process '[Format]' entries.
    set known {prefix date time sizeKilo sizeMega sizeGiga sizeTera speedKilo speedGiga speedMega}
    foreach {name value} [ConfigGetEx $handle Format] {
        set index [lsearch -exact $known $name]
        if {$index != -1} {
            set known [lreplace $known $index $index]
            set format($name) [VarReplaceBase $value 0]
        } else {
            LogDebug InitTheme "Unknown format type \"$name\"."
        }
    }
    if {[llength $known]} {
        foreach name $known {set format($name) {}}
        LogWarning InitTheme "Missing required format entries: [join $known {, }]."
    }

    # Process '[Theme]' entries.
    set known [array names vars]
    foreach {name value} [ConfigGetEx $handle Theme] {
        set index [lsearch -exact $known $name]
        if {$index != -1} {
            set known [lreplace $known $index $index]
            set theme($name) [VarReplaceBase $value]
        }
    }
    if {[llength $known]} {
        foreach name $known {set theme($name) {}}
        LogWarning InitTheme "Missing required theme entries: [join [lsort $known] {, }]."
    }

    ConfigClose $handle
    return
}

####
# InitVars
#
# Read the given variable definition files.
#
proc ::alcoholicz::InitVariables {varFiles} {
    variable flags
    variable replace
    variable scriptPath
    variable vars
    unset -nocomplain flags replace vars

    foreach filePath $varFiles {
        set handle [ConfigOpen [file join $scriptPath $filePath]]
        ConfigRead $handle

        foreach {name value} [ConfigGetEx $handle Flags] {
            # Several flags have multiple events defined. Therefore,
            # the config value must be appended, not 'set'.
            eval lappend [list flags($name)] $value
        }

        array set replace [ConfigGetEx $handle Replace]
        array set vars [ConfigGetEx $handle Variables]

        ConfigClose $handle
    }

    # The contents of flags array must be sorted to use 'lsearch -sorted'.
    foreach name [array name flags] {
        set flags($name) [lsort -unique $flags($name)]
    }
    return
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
        LogError InitMain $message
        die
    }

    # Initialise various subsystems.
    LogInfo "Loading configuration..."
    set configFile [file join $scriptPath "alcoBot.conf"]
    if {[catch {InitConfig $configFile} message]} {
        LogError InitMain $message
        die
    }

    InitVariables [ConfigGet $configHandle General varFiles]
    InitTheme     [ConfigGet $configHandle General themeFile]
    InitModules   [ConfigGet $configHandle General modules]

    ConfigFree $configHandle
    LogInfo "Sitebot loaded, configured for [GetFtpDaemon]."
    return
}

::alcoholicz::InitMain
