#
# nxMissingTags - Create tags for missing files.
# Copyright (c) 2005 neoxed
#
# Author:
#   neoxed (neoxed@gmail.com) Aug 12, 2005
#
# Abstract:
#   This script creates tags for missing NFOs and SFVs.
#
# Installation:
#   1. Copy the nxMissingTags.tcl file to x:\ioFTPD\scripts\.
#   2. Configure the script.
#   3. Add the following to your ioFTPD.ini:
#
#   [Events]
#   OnDelDir         = TCL ..\scripts\nxMissingTags.tcl DELDIR
#   OnUploadComplete = TCL ..\scripts\nxMissingTags.tcl UPLOAD
#
#   4. If you're using ioA, you may want to add the following lines to remove
#      tags after nukes and wipes. For those of you who are using nxTools, this
#      step is not necessary, since nxTools will automatically remove them.
#
#   [FTP_Custom_Commands]
#   nuke = EXEC ..\scripts\ioA.exe NUKE
#   nuke = TCL  ..\scripts\nxMissingTags.tcl SITE
#
#   wipe = EXEC ..\scripts\ioA.exe WIPE
#   wipe = TCL  ..\scripts\nxMissingTags.tcl SITE
#
#   Note: nxMissingTags must go AFTER ioA.
#
#   5. Rehash or restart ioFTPD for the changes to take effect.
#

namespace eval ::nxMissing {
    # Symlink tag formats, cookies: %release and %disk.
    variable tagNfo      "(NoNFO)-%release"
    variable tagSfv      "(NoSFV)-%release"
    variable tagSfvDisk  "(NoSFV)-%release-%disk"

    # Show warning if the NFO or SFV is missing.
    variable showWarning True

    # Sub-directories to check for SFVs.
    variable subDirs     {cd[0-9] dis[ck][0-9] dvd[0-9] extra extras sub subs vobsub vobsubs}

    # Paths to exempt from tag creation.
    variable exemptPaths {/0DAY/* /REQS/* /STAFF/* */codec */cover */covers */sample}
}

proc ::nxMissing::ArgList {argv} {
    set argList [list]
    set length [string length $argv]

    for {set index 0} {$index < $length} {incr index} {
        # Ignore leading white-space.
        while {[string is space -strict [string index $argv $index]]} {incr index}
        if {$index >= $length} {break}

        if {[string index $argv $index] eq "\""} {
            # Find the next quote character.
            set startIndex [incr index]
            while {[string index $argv $index] ne "\"" && $index < $length} {incr index}
        } else {
            # Find the next white-space character.
            set startIndex $index
            while {![string is space -strict [string index $argv $index]] && $index < $length} {incr index}
        }
        lappend argList [string range $argv $startIndex [expr {$index - 1}]]
    }
    return $argList
}

proc ::nxMissing::GetPath {path workingPath} {
    # Absolute path or relative path.
    if {[string index $path 0] ne "/"} {
        set path "$workingPath/$path"
    }
    set path [string trim $path "/\\"]

    # Resolve the "." and ".." path components.
    set components [list]
    foreach component [SplitPath $path] {
        if {$component eq ".."} {
            set components [lreplace $components end end]
        } elseif {$component ne "."} {
            lappend components $component
        }
    }
    return "/[join $components /]"
}

proc ::nxMissing::ListMatch {patternList string} {
    foreach pattern $patternList {
        if {[string match -nocase $pattern $string]} {return 1}
    }
    return 0
}

proc ::nxMissing::IsSubDir {path} {
    variable subDirs
    return [ListMatch $subDirs [file tail $path]]
}

proc ::nxMissing::GetNfoTag {realPath} {
    variable tagNfo

    if {[IsSubDir $realPath]} {
        set realPath [file dirname $realPath]
    }
    set release [file tail $realPath]
    set tag [string map [list "%release" $release] $tagNfo]

    return [file join [file dirname $realPath] $tag]
}

proc ::nxMissing::GetSfvTag {realPath} {
    variable tagSfv
    variable tagSfvDisk

    set mapping [list]
    if {[IsSubDir $realPath]} {
        set tagFormat $tagSfvDisk
        lappend mapping "%disk" [file tail $realPath]
        set realPath [file dirname $realPath]
    } else {
        set tagFormat $tagSfv
    }
    lappend mapping "%release" [file tail $realPath]
    set tag [string map $mapping $tagFormat]

    return [file join [file dirname $realPath] $tag]
}

proc ::nxMissing::UpdateTags {realPath virtualPath} {
    variable showWarning

    # NFO files reside in the release root directory, not subdirs.
    if {[IsSubDir $realPath]} {
        set nfoRealPath    [file dirname $realPath]
        set nfoVirtualPath [file dirname $virtualPath]
    } else {
        set nfoRealPath    $realPath
        set nfoVirtualPath $virtualPath
    }

    if {![llength [glob -directory $nfoRealPath -nocomplain "*.nfo"]]} {
        if {[string is true -strict $showWarning]} {
            iputs " Upload the NFO first!"
        }
        # Create no-NFO tag.
        set tagPath [GetNfoTag $realPath]
        if {![file exists $tagPath]} {
            catch {file mkdir $tagPath}
            catch {vfs chattr $tagPath 1 $nfoVirtualPath}
        }
    }

    if {![llength [glob -directory $realPath -nocomplain "*.sfv"]]} {
        if {[string is true -strict $showWarning]} {
            iputs " Upload the SFV first!"
        }
        # Create no-SFV tag.
        set tagPath [GetSfvTag $realPath]
        if {![file exists $tagPath]} {
            catch {file mkdir $tagPath}
            catch {vfs chattr $tagPath 1 $virtualPath}
        }
    }
}

proc ::nxMissing::RemoveTag {realPath} {
    catch {file delete -- [file join $realPath ".ioFTPD"]}
    catch {file delete -- $realPath}
    catch {vfs flush [file dirname $realPath]}
}

proc ::nxMissing::RemoveParentLinks {realPath} {
    if {[IsDiskPath $realPath]} {
        set realPath [file dirname $realPath]
    }
    set realPath [file dirname $realPath]
    set realPathLen [string length $realPath]

    foreach symPath [glob -nocomplain -types d -directory $realPath "*"] {
        if {[catch {vfs chattr $symPath 1} symTarget] || $symTarget eq ""} {continue}
        set symRealTarget [resolve pwd $symTarget]

        if {[string equal -nocase -length $realPathLen $realPath $symRealTarget]} {
            RemoveTag $symPath
        }
    }
}

proc ::nxMissing::SplitPath {path} {
    regsub -all -- {[\\/]+} $path {/} path
    return [file split [string trim $path "/"]]
}

proc ::nxMissing::Main {argv} {
    variable exemptPaths
    set argList [ArgList $argv]

    set event [string toupper [lindex $argList 0]]
    if {$event eq "DELDIR"} {
        set realPath [lindex $argList 1]

        # Remove tags on directory deletion.
        RemoveTag [GetSfvTag $realPath]
        if {![IsSubDir $realPath]} {
            RemoveTag [GetNfoTag $realPath]
        }
    } elseif {$event eq "SITE"} {
        set virtualPath [GetPath [lindex $argList 1]]
        RemoveParentLinks [resolve pwd $virtualPath]
    } elseif {$event eq "UPLOAD"} {
        set virtualPath [file dirname [lindex $argList 3]]

        if {![ListMatch $exemptPaths $virtualPath]} {
            set filePath [lindex $argList 1]
            set realPath [file dirname $filePath]
            set fileExt [string toupper [file extension $filePath]]

            if {$fileExt eq ".NFO"} {
                RemoveTag [GetNfoTag $realPath]
            } elseif {$fileExt eq ".SFV"} {
                RemoveTag [GetSfvTag $realPath]
            } else {
                UpdateTags $realPath $virtualPath
            }
        }
    }

    return 0
}

::nxMissing::Main [expr {[info exists args] ? $args : ""}]
