################################################################################
# nxTools - Display Rules                                                      #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

namespace eval ::nxTools::Rules {
    namespace import -force ::nxTools::Lib::*
}

# Rules Procedures
######################################################################

proc ::nxTools::Rules::WordWrap {WrapText TextWidth} {
    set Result ""
    while {[string length $WrapText] > $TextWidth} {
        set Index [string last { } $WrapText $TextWidth]
        if {$Index < 0} {set Index $TextWidth}
        lappend Result [string trim [string range $WrapText 0 $Index]]
        set WrapText [string range $WrapText [expr {$Index + 1}] end]
    }
    if {![string equal "" $WrapText]} {lappend Result $WrapText}
    return $Result
}

# Rules Main
######################################################################

proc ::nxTools::Rules::Main {ShowSection} {
    global misc rules
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set SectionList ""; set SectionName ""; set ShowList ""

    ## Read rules configuration
    if {![catch {set Handle [open $rules(ConfigFile) r]} ErrorMsg]} {
        while {![eof $Handle]} {
            set FileLine [string trim [gets $Handle]]
            ## Check config section
            if {[string equal "" $FileLine] || [string index $FileLine 0] == "#"} {continue
            } elseif {[string match {\[*\]} $FileLine]} {
                set SectionName [string range $FileLine 1 end-1]
                lappend SectionList $SectionName
            } elseif {[set Pos [string first "|" $FileLine]] != -1} {
                lappend rule($SectionName) [string range $FileLine 0 [incr Pos -1]] [string range $FileLine [incr Pos 2] end]
            }
        }
        close $Handle
    } else {ErrorLog RulesRead $ErrorMsg; return 1}

    ## Find specified section, show all areas if is there no match
    foreach SectionName $SectionList {
        if {[string equal -nocase $SectionName $ShowSection]} {
            set ShowList [list $SectionName]; break
        }
    }
    if {[string equal "" $ShowList]} {set ShowList $SectionList}

    foreach MessageType {Header Section SingleLine MultiLine Footer} {
        set template($MessageType) [ReadFile [file join $misc(Templates) "Rules.$MessageType"]]
    }
    OutputData [ParseCookies $template(Header) [list $SectionList] {sections}]

    foreach SectionName $ShowList {
        set RuleList [lindex [array get rule $SectionName] 1]
        if {[string equal "" $RuleList]} {continue}
        set Count 0
        OutputData [ParseCookies $template(Section) [list $SectionName $SectionList] {section sections}]

        foreach {PunishText RuleText} $RuleList {
            incr Count; set MultiLine 0
            ## Wrap each line before displaying
            foreach RuleLine [WordWrap $RuleText $rules(LineWidth)] {
                set ValueList [list $Count $PunishText $RuleLine $SectionName]
                if {!$MultiLine} {
                    OutputData [ParseCookies $template(SingleLine) $ValueList {num punishment rule section}]
                } else {
                    OutputData [ParseCookies $template(MultiLine) $ValueList {num punishment rule section}]
                }
                incr MultiLine
            }
        }
    }
    OutputData [ParseCookies $template(Footer) [list $SectionList] {sections}]
    return 0
}

::nxTools::Rules::Main [expr {[info exists args] ? $args : ""}]
