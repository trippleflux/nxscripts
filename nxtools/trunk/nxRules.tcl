#
# nxTools - Dupe Checker, nuker, pre, request and utilities.
# Copyright (c) 2004-2005 neoxed
#
# Module Name:
#   Rules
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements a rules script.
#

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Rules {
    namespace import -force ::nxLib::*
}

# Rules Procedures
######################################################################

proc ::nxTools::Rules::WordWrap {text width} {
    set result ""
    while {[string length $text] > $width} {
        set index [string last { } $text $width]
        if {$index < 0} {set index $width}
        lappend result [string trim [string range $text 0 $index]]
        set text [string range $text [expr {$index + 1}] end]
    }
    if {[string length $text]} {lappend result $text}
    return $result
}

# Rules Main
######################################################################

proc ::nxTools::Rules::Main {display} {
    global misc rules
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set displayList ""; set sectionList ""; set sectionName ""

    # Read rules configuration.
    if {![catch {set handle [open $rules(ConfigFile) r]} error]} {
        while {![eof $handle]} {
            set line [string trim [gets $handle]]
            if {![string length $line] || [string index $line 0] eq "#"} {continue}

            if {[string match {\[*\]} $line]} {
                set sectionName [string range $line 1 end-1]
                lappend sectionList $sectionName
            } elseif {[set index [string first "|" $line]] != -1} {
                lappend rule($sectionName) [string range $line 0 [incr index -1]] [string range $line [incr index 2] end]
            }
        }
        close $handle
    } else {ErrorLog RulesRead $error; return 1}

    # Find the specified section, display all areas if is there no match.
    foreach sectionName $sectionList {
        if {[string equal -nocase $sectionName $display]} {
            set displayList [list $sectionName]; break
        }
    }
    if {![string length $displayList]} {set displayList $sectionList}

    foreach fileExt {Header Section SingleLine MultiLine Footer} {
        set template($fileExt) [ReadFile [file join $misc(Templates) "Rules.$fileExt"]]
    }
    OutputText [ParseCookies $template(Header) [list $sectionList] {sections}]

    foreach sectionName $displayList {
        set ruleList [lindex [array get rule $sectionName] 1]
        if {![llength $ruleList]} {continue}
        set count 0
        OutputText [ParseCookies $template(Section) [list $sectionName $sectionList] {section sections}]

        foreach {punishment text} $ruleList {
            incr count; set isMultiLine 0

            # Wrap each line before displaying.
            foreach line [WordWrap $text $rules(LineWidth)] {
                set valueList [list $count $punishment $line $sectionName]
                if {!$isMultiLine} {
                    OutputText [ParseCookies $template(SingleLine) $valueList {num punishment rule section}]
                } else {
                    OutputText [ParseCookies $template(MultiLine) $valueList {num punishment rule section}]
                }
                incr isMultiLine
            }
        }
    }
    OutputText [ParseCookies $template(Footer) [list $sectionList] {sections}]
    return 0
}

::nxTools::Rules::Main [expr {[info exists args] ? $args : ""}]
