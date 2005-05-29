################################################################################
# nxTools - Request Script                                                     #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} ErrorMsg]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $ErrorMsg
}

namespace eval ::nxTools::Req {
    namespace import -force ::nxLib::*
}

# Request Procedures
######################################################################

proc ::nxTools::Req::CheckLimit {UserName GroupName} {
    global req
    foreach ReqLimit $req(Limits) {
        if {[llength $ReqLimit] != 5} {
            ErrorLog ReqLimit "wrong number of options in line: \"$ReqLimit\""; continue
        }
        foreach {Target GroupLimit UserLimit TimeLimit TimePeriod} $ReqLimit {break}

        if {[string index $Target 0] eq "="} {
            set Target [string range $Target 1 end]
            if {$Target ne $GroupName} {continue}

            ## Group request limits are only checked when 'Target' has a group prefix (=).
            if {$GroupLimit >= 0 && [ReqDb eval {SELECT count(*) FROM Requests WHERE Status=0 AND GroupName=$Target}] >= $GroupLimit} {
                LinePuts "You have reached your group's request limit of $GroupLimit request(s)."
                return 0
            }
            set GroupMatch "AND GroupName='[SqlEscape $Target]'"
        } elseif {$Target eq $UserName || $Target eq "*"} {
            set GroupMatch ""
        } else {continue}

        if {$UserLimit >= 0 && [ReqDb eval "SELECT count(*) FROM Requests WHERE Status=0 AND UserName='[SqlEscape $UserName]' $GroupMatch"] >= $UserLimit} {
            LinePuts "You have reached your individual request limit of $UserLimit request(s)."
            return 0
        }
        if {$TimeLimit >= 0 && $TimePeriod >= 0} {
            set TimeStamp [expr {[clock seconds] - ($TimePeriod * 86400)}]
            if {[ReqDb eval {SELECT count(*) FROM Requests WHERE TimeStamp > $TimeStamp AND UserName=$UserName}] >= $TimeLimit} {
                LinePuts "Only $TimeLimit request(s) can be made every $TimePeriod day(s)."
                set LastReq [ReqDb eval {SELECT TimeStamp FROM Requests WHERE UserName=$UserName ORDER BY TimeStamp DESC LIMIT 1}]
                set TimeLeft [FormatDuration [expr {$LastReq - $TimeStamp}]]
                LinePuts "You have to wait $TimeLeft until you can request again."
                return 0
            }
        }
        break
    }
    return 1
}

proc ::nxTools::Req::UpdateDir {Event Request {UserId 0} {GroupId 0}} {
    global req
    if {![string length $req(RequestPath)]} {
        return
    } elseif {![file exists $req(RequestPath)]} {
        ErrorLog ReqUpdateDir "The requests directory \"$req(RequestPath)\" does not exist."
        return
    }
    set ReMap [list %(request) $Request]
    set ReqPath [file join $req(RequestPath) [string map $ReMap $req(RequestTag)]]
    switch -- $Event {
        {ADD} {CreateTag $ReqPath $UserId $GroupId 777}
        {DEL} {
            if {[file isdirectory $ReqPath]} {
                KickUsers [file join $ReqPath "*"] True
                if {[catch {file delete -force -- $ReqPath} ErrorMsg]} {
                    ErrorLog ReqDelete $ErrorMsg
                }
            }
        }
        {FILL} {
            if {[file isdirectory $ReqPath]} {
                set FillPath [file join $req(RequestPath) [string map $ReMap $req(FilledTag)]]
                KickUsers [file join $ReqPath "*"] True
                if {[catch {file rename -force -- $ReqPath $FillPath} ErrorMsg]} {
                    ErrorLog ReqFill $ErrorMsg
                }
            }
        }
        default {
            ErrorLog ReqUpdateDir "unknown event \"$Event\""
        }
    }
    catch {vfs flush $req(RequestPath)}
    return
}

# Request Events
######################################################################

proc ::nxTools::Req::Add {UserName GroupName Request} {
    global misc req
    iputs ".-\[Request\]--------------------------------------------------------------."
    set Request [StripChars $Request]

    if {[ReqDb eval {SELECT count(*) FROM Requests WHERE Status=0 AND StrCaseEq(Request,$Request)}]} {
        LinePuts "This item is already requested."
    } elseif {[CheckLimit $UserName $GroupName]} {
        set RequestId 1
        ReqDb eval {SELECT (max(RequestId)+1) AS NextId FROM Requests WHERE Status=0} values {
            ## The max() function returns NULL if there are no matching records.
            if {[string length $values(NextId)]} {
                set RequestId $values(NextId)
            }
        }
        set TimeStamp [clock seconds]
        ReqDb eval {INSERT INTO Requests(TimeStamp,UserName,GroupName,Status,RequestId,Request) VALUES($TimeStamp,$UserName,$GroupName,0,$RequestId,$Request)}

        set RequestId [format "%03s" $RequestId]
        putlog "REQUEST: \"$UserName\" \"$GroupName\" \"$Request\" \"$RequestId\""
        LinePuts "Added your request of $Request (#$RequestId)."
        UpdateDir ADD $Request
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Req::Update {Event UserName GroupName Request} {
    global misc req
    iputs ".-\[Request\]--------------------------------------------------------------."
    set Exists 0
    set Request [StripChars $Request]

    ReqDb eval {SELECT rowid,* FROM Requests WHERE Status=0 AND (RequestId=$Request OR StrCaseEq(Request,$Request)) LIMIT 1} values {set Exists 1}
    if {!$Exists} {
        LinePuts "Invalid request, use \"SITE REQUESTS\" to view current requests."
    } else {
        if {$Event eq "FILL"} {
            ReqDb eval {UPDATE Requests SET Status=1 WHERE rowid=$values(rowid)}
            LinePuts "Filled request $values(Request) for $values(UserName)/$values(GroupName)."
            set LogPrefix "REQFILL"

        } elseif {$Event eq "DEL"} {
            ## Only siteops or the owner may delete a request.
            if {$UserName ne $values(UserName) && ![regexp "\[$misc(SiteopFlags)\]" $flags]} {
                ReqDb close
                ErrorReturn "You are not allowed to delete another user's request."
            }
            ReqDb eval {DELETE FROM Requests WHERE rowid=$values(rowid)}
            LinePuts "Deleted request $values(Request) for $values(UserName)/$values(GroupName)."
            set LogPrefix "REQDEL"
        }

        set RequestAge [expr {[clock seconds] - $values(TimeStamp)}]
        set RequestId [format "%03s" $values(RequestId)]
        if {[IsTrue $misc(dZSbotLogging)]} {
            set RequestAge [FormatDuration $RequestAge]
        }
        putlog "${LogPrefix}: \"$UserName\" \"$GroupName\" \"$values(Request)\" \"$values(UserName)\" \"$values(GroupName)\" \"$RequestId\" \"$RequestAge\""
        UpdateDir $Event $values(Request)
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

proc ::nxTools::Req::List {IsSiteBot} {
    global misc req
    if {!$IsSiteBot} {
        foreach MessageType {Header Body None Footer} {
            set template($MessageType) [ReadFile [file join $misc(Templates) "Requests.$MessageType"]]
        }
        OutputData $template(Header)
        set Count 0
    }
    ReqDb eval {SELECT * FROM Requests WHERE Status=0 ORDER BY RequestId DESC} values {
        set RequestAge [expr {[clock seconds] - $values(TimeStamp)}]
        set RequestId [format "%03s" $values(RequestId)]
        if {$IsSiteBot} {
            iputs [list REQS $values(TimeStamp) $RequestAge $RequestId $values(UserName) $values(GroupName) $values(Request)]
        } else {
            incr Count
            set RequestAge [lrange [FormatDuration $RequestAge] 0 1]
            set ValueList [list $RequestAge $RequestId $values(UserName) $values(GroupName) $values(Request)]
            OutputData [ParseCookies $template(Body) $ValueList {age id user group request}]
        }
    }
    if {!$IsSiteBot} {
        if {!$Count} {OutputData $template(None)}
        OutputData $template(Footer)
    }
}

proc ::nxTools::Req::Wipe {} {
    global misc req
    iputs ".-\[Request\]--------------------------------------------------------------."
    if {$req(MaximumAge) < 1} {
        LinePuts "Request wiping is disabled, check your configuration."
    } else {
        LinePuts "Wiping filled requests older then $req(MaximumAge) day(s)..."
        set MaxAge [expr {[clock seconds] - $req(MaximumAge) * 86400}]

        ReqDb eval {SELECT rowid,* FROM Requests WHERE Status=1 AND TimeStamp < $MaxAge ORDER BY RequestId DESC} values {
            set RequestAge [expr {[clock seconds] - $values(TimeStamp)}]
            set RequestId [format "%03s" $values(RequestId)]

            ## Wipe the directory if it exists.
            set FillPath [string map [list %(request) $values(Request)] $req(FilledTag)]
            set FillPath [file join $req(RequestPath) $FillPath]
            if {[file isdirectory $FillPath]} {
                KickUsers [file join $FillPath "*"] True
                if {[catch {file delete -force -- $FillPath} ErrorMsg]} {
                    ErrorLog ReqWipe $ErrorMsg
                }
                LinePuts "Wiped: $values(Request) by $values(UserName)/$values(GroupName) (#$RequestId)."
                if {[IsTrue $misc(dZSbotLogging)]} {
                    set RequestAge [FormatDuration $RequestAge]
                }
                putlog "REQWIPE: \"$values(UserName)\" \"$values(GroupName)\" \"$values(Request)\" \"$RequestId\" \"$RequestAge\" \"$req(MaximumAge)\""
            }
            ReqDb eval {UPDATE Requests SET Status=2 WHERE rowid=$values(rowid)}
        }
        catch {vfs flush $req(RequestPath)}
    }
    iputs "'------------------------------------------------------------------------'"
    return 0
}

# Request Main
######################################################################

proc ::nxTools::Req::Main {ArgV} {
    global misc flags group user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set IsSiteBot [expr {[info exists user] && $misc(SiteBot) eq $user}]

    if {[catch {DbOpenFile [namespace current]::ReqDb "Requests.db"} ErrorMsg]} {
        ErrorLog RequestDb $ErrorMsg
        if {!$IsSiteBot} {iputs "Unable to open requests database."}
        return 1
    }

    set ArgLength [llength [set ArgList [ArgList $ArgV]]]
    set Event [string toupper [lindex $ArgList 0]]
    switch -- $Event {
        {ADD} {
            if {$ArgLength > 1} {
                Add $user $group [join [lrange $ArgList 1 end]]
            } else {
                iputs "Syntax: SITE REQUEST <request>"
            }
        }
        {DEL} - {FILL} {
            if {$ArgLength > 1} {
                Update $Event $user $group [join [lrange $ArgList 1 end]]
            } else {
                iputs "Syntax: SITE REQ$Event <id/request>"
            }
        }
        {LIST} {List $IsSiteBot}
        {WIPE} {Wipe}
        default {
            ErrorLog InvalidArgs "unknown event \"[info script] $Event\": check your ioFTPD.ini for errors"
        }
    }

    ReqDb close
    return 0
}

::nxTools::Req::Main [expr {[info exists args] ? $args : ""}]
