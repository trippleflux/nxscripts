################################################################################
# nxImport - Import Users Credits and Stats                                    #
################################################################################
# Author  : neoxed                                                             #
# Date    : 01/07/2004                                                         #
# Version : 1.1.0                                                              #
################################################################################
#
# Requirements:
#
# - Backup your user and group ioFTPD before using this script, just in case.
# - You must have already imported the users and groups from another install.
# - This script must be installed on the ioFTPD you wish to have the credits and
#   stats of users added too.
#
# Notes:
#
# - This script only updates credits and stats, it does not create users or groups.
# - If the user does not exist on the ioFTPD that you're importing to, that user will be
#   skipped and you'll have to manually create and update that user.
# - These user files must not be in use, or you will have problems. Make sure that
#   the ioFTPD process from the previous install is NOT running.
#
# Installation:
#
# 1. Copy the nxImportStats.tcl file to x:\ioFTPD\scripts
# 2. Configure the script options.
# 3. Add the following to the ioFTPD.ini
#
# [FTP_Custom_Commands]
# importstats = TCL ..\scripts\nxImportStats.tcl
#
# [FTP_SITE_Permissions]
# importstats = M
#
# 4. Rehash or restart for the changes to take effect.
# 5. Login to your FTP and run 'SITE IMPORTSTATS'.
# 6. Check the output to your client or ../logs/nxImport.log for info.
# 7. Good luck! ;)
#
# Change Log:
#
# v1.1.0 - Added a credits and stats updating script.
# v1.0.1 - Documentation changes, updated a few of the requirements.
# v1.0.0 - Initial release.
#
################################################################################

## Path to the old UserIdTable and user files(use foward slashes)
set IU(UserIdTable)	"D:/ioFTPD/etc/UserIdTable"
set IU(UserFiles)	"D:/ioFTPD/users"

## End of Settings
################################################################################

set CurrUsers ""
set UserList ""
set IU(Version) "1.1.0"

## Log and output text
proc DebugLog {LogText} {
	iputs -nobuffer $LogText
set LogTime [clock format [clock seconds] -format "%m-%d-%Y %H:%M:%S"]
	if {![catch {open "../logs/nxImport.log" a} fp]} {
		puts $fp "$LogTime - $LogText"
		close $fp
	} else {iputs -nobuffer "Error opening file \"../logs/nxImport.log\""}
}
DebugLog "nxImportStats v$IU(Version) starting..."

## Read users from the old UserIdTable
DebugLog "Reading users from \"$IU(UserIdTable)\""
if {![catch {open $IU(UserIdTable) r} fp]} {
	while {![eof $fp]} {
		set Line [string trim [gets $fp]]
		if {[string equal "" $Line]} {continue}
		foreach {Name ID Type} [split $Line ":"] {break}
		## Validate the user line
		if {![string is digit -strict $ID] || [string equal "" $Name]} {
			DebugLog "Invalid UserIdTable line \"$Line\""
		} else {lappend UserList $ID $Name}
	}
	close $fp
} else {DebugLog "Unable to open \"$IU(UserIdTable)\" for reading."; return}

## Create a list of current users and groups
foreach UID [user list] {lappend CurrUsers [resolve uid $UID]}

## Add the old users
foreach {OldUID UserName} $UserList {
	## Check if the user already exists
	if {[lsearch -exact $CurrUsers $UserName] == -1} {
		## The user doesn't exist
		DebugLog "The user \"$UserName\" does not exist, you'll have to manually create and update this user."
		continue
	}
	## Create the stats array
	set StatTypes "alldn allup monthdn monthup wkdn wkup daydn dayup"
	set UStats(credits) "0 0 0 0 0 0 0 0 0 0"
	foreach StatType $StatTypes {
		set UStats($StatType) "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"
	}
	## Read the old user file
	set UFilePath [file join $IU(UserFiles) $OldUID]
	if {![catch {open $UFilePath r} fp]} {
		while {![eof $fp]} {
			set ULine [string trim [gets $fp]]
			if {[string equal "" $ULine]} {continue}
			set ULineType [string tolower [lindex $ULine 0]]
			## Save the users stats
			if {[string equal "credits" $ULineType]} {
				set UStats(credits) $ULine
			} elseif {[lsearch -exact $StatTypes $ULineType] != -1} {
				set UStats($ULineType) $ULine
			}
		}
		close $fp
		## Update the user file
		set NewUFile ""
		if {[userfile open $UserName] == 0} {
			userfile lock
			set UFileList [split [userfile bin2ascii] "\n"]
			foreach ULine $UFileList {
				set ULineType [string tolower [lindex $ULine 0]]
				if {[string equal "credits" $ULineType]} {
					## Update credits
					for {set x 1} {$x < 11} {incr x} {
						set Value [expr {wide([lindex $ULine $x]) + wide([lindex $UStats(credits) $x])}]
						set ULine [lreplace $ULine $x $x $Value]
					}
				} elseif {[lsearch -exact $StatTypes $ULineType] != -1} {
					## Update stats
					for {set x 1} {$x < 31} {incr x} {
						set Value [expr {wide([lindex $ULine $x]) + wide([lindex $UStats($ULineType) $x])}]
						set ULine [lreplace $ULine $x $x $Value]
					}
				}
				append NewUFile $ULine "\n"
			}
			userfile ascii2bin $NewUFile
			userfile unlock
			DebugLog "Updated the credits and stats for \"$UserName\" with the contents of \"$UFilePath\""
		} else {DebugLog "Unable to update the credits and stats for \"$UserName\""}

	} else {
		DebugLog "Unable to read the file \"$UFilePath\" for the user \"$UserName\""
	}
}

DebugLog "nxImportStats v$IU(Version) finished."

## EOF
