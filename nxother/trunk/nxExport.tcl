################################################################################
# nxExport - Export Users and Groups from your Shared DB                       #
################################################################################
# Author  : neoxed                                                             #
# Date    : 10/08/2004                                                         #
# Version : 1.0.0                                                              #
################################################################################
#
# Notes:
#
# - This script may be slow due to extensive logging.
# - Backup your user and group ioFTPD before using this script, just in case.
# - Make sure there are not any existing user or group files in the export path,
#   because there's a possibility they may be overwritten.
#
# Installation:
#
# 1. Copy the nxExport.tcl file to x:\ioFTPD\scripts
# 2. Configure the script options.
# 3. Add the following to the ioFTPD.ini
#
# [FTP_Custom_Commands]
# export = TCL ..\scripts\nxExport.tcl
#
# [FTP_SITE_Permissions]
# export = M
#
# 4. Rehash or restart for the changes to take effect.
# 5. Login to your FTP and run 'SITE EXPORT'.
# 6. Check the output to your client or ../logs/nxExport.log for info.
# 7. Good luck! ;)
#
# Change Log:
#
# v1.0.0 - Initial release.
#
################################################################################

# Path to export the GroupIdTable and UserIdTable to (use foward slashes).
set IE(GroupIdTable) "D:/ioFTPD/export/GroupIdTable"
set IE(UserIdTable)  "D:/ioFTPD/export/UserIdTable"

# Path to export the user and group files to (use foward slashes).
set IE(GroupFiles)   "D:/ioFTPD/export/groups"
set IE(UserFiles)    "D:/ioFTPD/export/users"

################################################################################

proc DebugLog {message} {
    iputs -nobuffer $message
    set now [clock format [clock seconds] -format "%m-%d-%Y %H:%M:%S"]

    if {![catch {set handle [open "../logs/nxExport.log" a]} error]} {
        puts $handle "$now - $message"
        close $handle
    } else {iputs $error}
}

set GroupList [list]
set UserList  [list]

DebugLog "nxExport starting..."

# Create export directories (if they don't exist)
set CreateDirs [list [file dirname $IE(GroupIdTable)] [file dirname $IE(GroupIdTable)] $IE(GroupFiles) $IE(UserFiles)]
foreach DirPath $CreateDirs {
    if {[file isfile $DirPath]} {
        DebugLog "Error, the file \"$DirPath\" should be a directory."
        return
    } elseif {![file isdirectory $DirPath]} {
        DebugLog "Creating the directory \"$DirPath\""
        catch {file mkdir $DirPath}
    }
}

# Retrieve a list of current users and groups
DebugLog "Retrieving a list of current users and groups."
foreach GID [group list] {lappend GroupList $GID [resolve gid $GID]}
foreach UID [user list] {lappend UserList $UID [resolve uid $UID]}

# Write the group table
DebugLog "Writing group table to \"$IE(GroupIdTable)\""
if {![catch {open $IE(GroupIdTable) w} fp]} {
    foreach {GID GroupName} $GroupList {
        if {![string equal "" $GroupName]} {
            puts $fp "${GroupName}:${GID}:STANDARD"
        } else {
            # This shouldn't happen, however, doesn't hurt to check
            DebugLog "Unable to resolve the GID $GID to it's corresponding group name."
        }
    }
    close $fp
} else {DebugLog "Unable to open \"$IE(GroupIdTable)\" for writing."; return}

# Write the user table
DebugLog "Writing user table to \"$IE(UserIdTable)\""
if {![catch {open $IE(UserIdTable) w} fp]} {
    foreach {UID UserName} $UserList {
        if {![string equal "" $UserName]} {
            puts $fp "${UserName}:${UID}:STANDARD"
        } else {
            # This shouldn't happen, however, doesn't hurt to check
            DebugLog "Unable to resolve the UID $UID to it's corresponding user name."
        }
    }
    close $fp
} else {DebugLog "Unable to open \"$IE(UserIdTable)\" for writing."; return}

# Write group files
if {![file isdirectory $IE(GroupFiles)]} {
    DebugLog "Error, the directory \"$IE(GroupFiles)\" does not exist."; return
} else {
    DebugLog "Writing group files to \"$IE(GroupFiles)\""
    foreach {GID GroupName} $GroupList {
        # Open the group file for writing
        set GFilePath [file join $IE(GroupFiles) $GID]
        if {![catch {open $GFilePath w} fp]} {
            # Retrieve the group file contents
            if {[groupfile open $GroupName] == 0} {
                set GFileContents [groupfile bin2ascii]
                # Write the group file
                foreach GLine [split $GFileContents "\n"] {
                    if {![string equal "" $GLine]} {puts $fp $GLine}
                }
                close $fp
                DebugLog "Successfully wrote the group file for \"$GroupName\" to \"$GFilePath\""
            } else {DebugLog "Unable to update the group file for \"$GroupName\""}

        } else {DebugLog "Unable open the file \"$GFilePath\" for writing for the group \"$GroupName\""}
    }
}

# Write user files
if {![file isdirectory $IE(UserFiles)]} {
    DebugLog "Error, the directory \"$IE(UserFiles)\" does not exist."; return
} else {
    DebugLog "Writing user files to \"$IE(UserFiles)\""
    foreach {UID UserName} $UserList {
        # Open the user file for writing
        set UFilePath [file join $IE(UserFiles) $UID]
        if {![catch {open $UFilePath w} fp]} {
            # Retrieve the user file contents
            if {[userfile open $UserName] == 0} {
                set UFileContents [userfile bin2ascii]
                # Write the user file
                foreach ULine [split $UFileContents "\n"] {
                    if {![string equal "" $ULine]} {puts $fp $ULine}
                }
                close $fp
                DebugLog "Successfully wrote the user file for \"$UserName\" to \"$UFilePath\""
            } else {DebugLog "Unable to update the user file for \"$UserName\""}

        } else {DebugLog "Unable open the file \"$UFilePath\" for writing for the user \"$UserName\""}
    }
}

DebugLog "nxExport finished."

# EOF
