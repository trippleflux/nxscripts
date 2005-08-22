################################################################################
# nxImport - Import Users and Groups to your Shared DB                         #
################################################################################
# Author  : neoxed                                                             #
# Date    : 01/07/2004                                                         #
# Version : 1.1.0                                                              #
################################################################################
#
# Requirements:
#
# - Backup your user and group ioFTPD before using this script, just in case.
# - The NoGroup (GID 1) group must exist on the ioFTPD that you're importing to.
# - This script must be installed on the ioFTPD you wish to have the users and
#   groups imported to.
# - You must have the GroupIdTable, UserIdTable, user files, and group files
#   from your previous ioFTPD install. They should reside in another directory.
#
# Notes:
#
# - This script may be slow due to extensive logging.
# - If the user or group already exists, the userfile/groupfile is updated with the
#   old one anyway. (In case the user already exists.)
# - These user and group files must not be in use, or you will have problems. Make
#   sure that the ioFTPD process from the previous install is NOT running.
#
# Installation:
#
# 1. Copy the nxImport.tcl file to x:\ioFTPD\scripts
# 2. Configure the script options.
# 3. Add the following to the ioFTPD.ini
#
# [FTP_Custom_Commands]
# import = TCL ..\scripts\nxImport.tcl
#
# [FTP_SITE_Permissions]
# import = M
#
# 4. Rehash or restart for the changes to take effect.
# 5. Login to your FTP and run 'SITE IMPORT'.
# 6. Check the output to your client or ../logs/nxImport.log for info.
# 7. Good luck! ;)
#
# Change Log:
#
# v1.1.0 - Added a credits and stats updating script.
# v1.1.0 - Documentation changes, updated a few of the requirements.
# v1.0.0 - Initial release.
#
################################################################################

## Path to the old GroupIdTable and UserIdTable (use foward slashes)
set IU(GroupIdTable)	"D:/ioFTPD/etc/GroupIdTable"
set IU(UserIdTable)	"D:/ioFTPD/etc/UserIdTable"

## Path to the user and group files (use foward slashes)
set IU(GroupFiles)	"D:/ioFTPD/groups"
set IU(UserFiles)	"D:/ioFTPD/users"

## End of Settings
################################################################################

set GroupList ""
set UserList ""
set GIDList ""
set IU(Version) "1.0.1"

## Log and output text
proc DebugLog {LogText} {
	iputs -nobuffer $LogText
	set LogTime [clock format [clock seconds] -format "%m-%d-%Y %H:%M:%S"]
	if {![catch {open "../logs/nxImport.log" a} fp]} {
		puts $fp "$LogTime - $LogText"
		close $fp
	} else {iputs -nobuffer "Error opening file \"../logs/nxImport.log\""}
}
DebugLog "nxImport v$IU(Version) starting..."

## Convert a text file to a list
proc FileToList {FilePath} {
	set FileLines ""
	if {![catch {open $FilePath r} fp]} {
		while {![eof $fp]} {
			set Line [string trim [gets $fp]]
			if {[string equal "" $Line]} {continue}
			lappend FileLines $Line
		}
		close $fp
	}
	return $FileLines
}

## Read groups from the old GroupIdTable
DebugLog "Reading groups from \"$IU(GroupIdTable)\""
if {![catch {open $IU(GroupIdTable) r} fp]} {
	while {![eof $fp]} {
		set Line [string trim [gets $fp]]
		if {[string equal "" $Line]} {continue}
		foreach {Name ID Type} [split $Line ":"] {break}
		## Skip GID 1 aka NoGroup
		if {$ID == 1} {continue}
		## Validate the group line
		if {![string is digit -strict $ID] || [string equal "" $Name]} {
			DebugLog "Invalid GroupIdTable line \"$Line\""
		} else {lappend GroupList $ID $Name}
	}
	close $fp
} else {DebugLog "Unable to open \"$IU(GroupIdTable)\" for reading."; return}

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
set CurrGroups ""; set CurrUsers ""
foreach GID [group list] {lappend CurrGroups [resolve gid $GID]}
foreach UID [user list] {lappend CurrUsers [resolve uid $UID]}

## Add the old groups
foreach {OldGID GroupName} $GroupList {
	## Check if the group already exists
	if {[lsearch -exact $CurrGroups $GroupName] == -1} {
		## The group doesn't exist, create it
		set NewGID [group create $GroupName]
		## Check if the group was created
		if {$NewGID == -1} {DebugLog "Unable to create the group \"$GroupName\""; continue}
		## Group was created
		DebugLog "Created the group \"$GroupName\" with a GID of $NewGID (Old GID: $OldGID)"
	} else {
		set NewGID [resolve group $GroupName]
		DebugLog "The group \"$GroupName\" already exists with a GID of $NewGID (Old GID: $OldGID)"
	}
	## Read the old group file
	set GFilePath [file join $IU(GroupFiles) $OldGID]
	set GFileList [FileToList $GFilePath]
	## Was the group file read?
	if {[string equal "" $GFileList]} {
		DebugLog "Unable to read the file \"$GFilePath\" for the group \"$GroupName\""
	} else {
		## Update group file
		if {[groupfile open $GroupName] == 0} {
			groupfile lock
			groupfile ascii2bin [join $GFileList "\n"]
			groupfile unlock
			DebugLog "Updated the group file for \"$GroupName\" with the contents of \"$GFilePath\""
		} else {DebugLog "Unable to update the group file for \"$GroupName\""}
	}
	## Save the new and old GIDs for later use
	lappend GIDList $OldGID $NewGID
}

## Find the new GID for a group
proc UpdateGID {FindOld} {
	global GIDList
	foreach {OldGID NewGID} $GIDList {
		if {$FindOld == $OldGID} {return $NewGID}
	}
	return ""
}

## Add the old users
foreach {OldUID UserName} $UserList {
	## Check if the user already exists
	if {[lsearch -exact $CurrUsers $UserName] == -1} {
		## The user doesn't exist, create it
		set NewUID [user create $UserName]
		## Check if the user was created
		if {$NewUID == -1} {DebugLog "Unable to create the user \"$UserName\""; continue}
		## User was created
		DebugLog "Created the user \"$UserName\" with a UID of $NewUID (Old UID: $OldUID)"
	} else {
		set NewUID [resolve user $UserName]
		DebugLog "The user \"$UserName\" already exists with a UID of $NewUID (Old UID: $OldUID)"
	}
	## Read the old user file
	set UFilePath [file join $IU(UserFiles) $OldUID]
	set UFileList [FileToList $UFilePath]
	## Was the user file read?
	if {[string equal "" $UFileList]} {
		DebugLog "Unable to read the file \"$UFilePath\" for the user \"$UserName\""
	} else {
		## Update the user file
		set NewUFile ""
		if {[userfile open $UserName] == 0} {
			userfile lock
			foreach Line $UFileList {
				if {[string match -nocase "admingroups*" $Line]} {
					set NewLine [lindex $Line 0]
					## Update the old GIDs with the new ones
					foreach OldGID [lrange $Line 1 end] {
						## Skip GID 1 aka NoGroup
						if {$OldGID == 1} {continue}
						set NewGID [UpdateGID $OldGID]
						if {![string equal "" $NewGID]} {append NewLine " " $NewGID}
					}
					set Line $NewLine
				} elseif {[string match -nocase "groups*" $Line]} {
					set NewLine [lindex $Line 0]
					## Update the old GIDs with the new ones
					foreach OldGID [lrange $Line 1 end] {
						## Skip GID 1 aka NoGroup
						if {$OldGID == 1} {continue}
						set NewGID [UpdateGID $OldGID]
						if {![string equal "" $NewGID]} {append NewLine " " $NewGID}
					}
					## If no there are no GIDs, add GID 1 for NoGroup
					if {[llength $NewLine] == 1} {append NewLine " " 1}
					set Line $NewLine
				}
				append NewUFile $Line "\n"
			}
			userfile ascii2bin $NewUFile
			userfile unlock
			DebugLog "Updated the user file for \"$UserName\" with the contents of \"$UFilePath\""
		} else {DebugLog "Unable to update the user file for \"$UserName\""}
	}
}

DebugLog "nxImport v$IU(Version) finished."

## EOF
