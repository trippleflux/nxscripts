################################################################################
# ForceSFV - Force SFV Files First                                             #
################################################################################
# Author  : neoxed                                                             #
# Date    : 16/05/2004                                                         #
# Version : 1.0.2                                                              #
################################################################################
#
# Installation:
#
# 1. Copy the ForceSFV.itcl file to x:\ioFTPD\scripts
# 2. Configure the script options.
# 3. Add the following to the ioFTPD.ini
#
# [FTP_Pre-Command_Events]
# stor	= TCL ..\scripts\ForceSFV.itcl
#
# 4. Good luck! ;)
#
# Change Log:
#
# v1.0.2 - Added user, group, and flag excemptions.
# v1.0.1 - Fixed a few spelling errors and updated the config comments.
# v1.0.0 - Initial release.
#
################################################################################

## Extensions to Ignore
# - List of extensions not to check, include the preceeding period.
# - It's recommended to leave .nfo and .sfv in the ignore list.
# - Wildcards may be used if needed.

set IgnoreExts		".nfo .sfv .log .txt .jpg"

## No Extension
# - Block files with no extension? (ex. README)
# - 1 = Block / 0 = Allow

set BlockNoExt		0

## Paths to Ignore
# - Virtual paths to ignore for the SFV check (case insensitive).
# - Include the trailing forward slash.

set IgnorePaths		"*/codec/ */cover/ */covers/ */extra/ */sample/ */subs/ */extras/ */vobsub/ */vobsubs/"

## Paths to Check
# - Virtual paths to force for the SFV first (case sensitive).
# - Wildcards may be used if needed.

set CheckPaths		"/APPS/* /DVDR/* /GAMES/* /MP3/* /SVCD/* /TV/* /VCD/* /XVID/* /XXX/*"

## Exempt Users/Groups/Flags
# - Exempt certain users based on usernames, groups or flags.
# - To disable exempts, set to "".

set ExemptUsers		"someuser someguy"
set ExemptGroups	"FRiENDS STAFF"
set ExemptFlags		"M1"

## End of Settings
################################################################################

## Find the virtual path, handles both relative and absolute paths.
proc GetPath {pwd path} {
	if {[string index $path 0] == "/"} {set vpath $path} else {set vpath "$pwd$path"}
	regsub -all {[\/]+} $vpath {/} vpath
	if {![string equal "/" $vpath]} {set vpath [string trimright $vpath "/"]}
	return $vpath
}

## Handle arguments with unclosed brackets.
proc ArgRange {list start end} {
	regsub -all {[\s]+} $list { } list
	return [join [lrange [split [string trim $list] { }] $start $end] { }]
}

if {[info exists args] && ![string equal "" $args]} {
	## Set file and directory paths
	set checksfv 0
	set fpath [GetPath $pwd [ArgRange $args 1 end]]
	set mpath [string range $fpath 0 [string last "/" $fpath]]
	set filext [file extension $fpath]
	## Check if file has extension
	if {[string equal "" $filext] && $BlockNoExt == 1} {
		iputs -noprefix "553-.-\[NoExt\]---------------------------------------."
		iputs -noprefix "553-| No extension found, file not allowed.         |"
		iputs -noprefix "553 '-----------------------------------------------'"
		set ioerror 1; return 1
	}
	## Check for exempts/ignores
	if {[lsearch $ExemptUsers $user] != -1 || [lsearch $ExemptGroups $group] != -1} {return 0}
	if {![string equal "" $ExemptFlags] && [regexp "\[$ExemptFlags\]" $flags]} {return 0}
	foreach ignore $IgnoreExts {
		if {[string match -nocase $ignore $filext]} {return 0}
	}
	foreach ignore $IgnorePaths {
		if {[string match -nocase $ignore $mpath]} {return 0}
	}
	## Do we check this path?
	foreach check $CheckPaths {
		if {[string match $check $mpath]} {set checksfv 1; break}
	}
	if {$checksfv == 1} {
		set realpath [resolve pwd $mpath]
		set sfvfiles [glob -nocomplain -types f -directory $realpath {*.[sS][fF][vV]}]
		if {[llength $sfvfiles] == 0} {
			iputs -noprefix "553-.-\[NoSFV\]---------------------------------------."
			iputs -noprefix "553-| You must upload the SFV first.                |"
			iputs -noprefix "553 '-----------------------------------------------'"
			set ioerror 1; return 1
		}
	}
}
