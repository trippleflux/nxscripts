################################################################################
# nxHelp - SITE HELP Script                                                    #
################################################################################
# Author  : neoxed                                                             #
# Date    : 18-07-2005                                                         #
# Version : 2.0.0 (rev 5)                                                      #
################################################################################

namespace eval ::nxHelp {
    # Path to SITE command files.
    variable cmdPath    "../scripts/nxHelp/site"

    # Path to help files.
    variable helpPath   "../scripts/nxHelp/help"

    # Permissions for help files displayed on 'SITE HELP'.
    # - If the 'flags' field is left empty, the file is shown to all users.
    # - The 'file to display' must be located in 'helpPath'.
    # - Format: <flags>|<file to display>
    variable permissions {
        {M1G|user.help}
        {M1|siteop.help}
        {M1V|vfs.help}
        {M1N|nuke.help}
    }
}

# End of Settings
##############################################################################

proc ::nxHelp::ArgsToList {ArgV} {
    split [string trim [regsub -all {\s+} $ArgV { }]]
}

proc ::nxHelp::CheckFlags {required current} {
    foreach flag [split $required {}] {
        if {[string first $flag $required] != -1} {
            return 1
        }
    }
    return 0
}

proc ::nxHelp::ShowFile {path} {
    if {![catch {set handle [open $path r]} message]} {
        while {![eof $handle]} {
            if {[gets $handle line] > 0} {iputs $line}
        }
        close $handle
    }
}

proc ::nxHelp::Main {argv} {
    global flags
    variable cmdPath
    variable helpPath
    variable permissions

    # Safe argument handling.
    set argList [ArgsToList $argv]

    if {![llength $argList]} {
        ShowFile [file join $helpPath "header.help"]
        foreach entry $permissions {
            foreach {neededFlags filePath} [split $entry "|"] {break}

            # An empty flag field implies all users.
            if {$neededFlags eq "" || [CheckFlags $neededFlags $flags]} {
                ShowFile [file join $helpPath $filePath]
            }
        }
        ShowFile [file join $helpPath "footer.help"]
    } else {
        set topic [string tolower [lindex $argList 0]]
        set cmdFile [join [list $cmdPath ${topic}.site] "/"]

        if {[file readable $cmdFile]} {
            ShowFile $cmdFile
        } else {
            ShowFile [file join $helpPath "notfound.help"]
        }
    }
    return 0
}

::nxHelp::Main [expr {[info exists args] ? $args : ""}]
