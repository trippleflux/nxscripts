################################################################################
#  nxHelp v1.10                                                                #
################################################################################
# Author  : neoxed                                                             #
# Date    : 05-03-2004                                                         #
# Version : 1.1.0 (rev 4)                                                      #
################################################################################

# Path to the help files.
# - Shown on 'SITE HELP'

set help(HelpFiles) "../scripts/nxHelp/help"

# Path to site command help files.
# - Shown on 'SITE HELP <command>'

set help(SiteFiles) "../scripts/nxHelp/site"

# Permissions to display help files.
# - The files will be shown in the defined order.
# - If you leave the 'flags' directive empty, the file will be shown to all users.
# - The file must be located in the 'help(HelpFiles)' path.
# - Format: "<flags>|<file to display>"

set help(Permissions) {
  "M1G|user.help"
  "M1|siteop.help"
  "M1V|vfs.help"
  "M1N|nuke.help"
}

# End of Settings
##############################################################################

proc CheckFlags {need have} {
    if {[regexp "\[$need\]" $have]} {return 1}
    return 0
}

proc ShowFile {FilePath} {
    if {![catch {set Handle [open $FilePath r]} ErrorMsg]} {
        while {![eof $Handle]} {
            if {[gets $Handle Line] > 0} {iputs $Line}
        }
        close $Handle
    }
}

if {![info exists args] || [string equal "" $args]} {
    ShowFile [file join $help(HelpFiles) "header.help"]
    foreach perm $help(Permissions) {
        set perm [split $perm "|"]
        if {[string equal "" [lindex $perm 0]] || [CheckFlags [lindex $perm 0] $flags]} {
            ShowFile [file join $help(HelpFiles) [lindex $perm 1]]
        }
    }
    ShowFile [file join $help(HelpFiles) "footer.help"]
} else {
    set topic [string tolower [lindex $args 0]]
    if {[file readable [file join $help(SiteFiles) $topic\.site]]} {
        ShowFile [file join $help(SiteFiles) $topic\.site]
    } else {
        ShowFile [file join $help(HelpFiles) "notfound.help"]
    }
}
