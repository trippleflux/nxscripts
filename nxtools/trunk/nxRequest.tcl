#
# nxTools - Dupe Checker, nuker, pre, request and utilities.
# Copyright (c) 2004-2007 neoxed
#
# Module Name:
#   Request
#
# Author:
#   neoxed (neoxed@gmail.com)
#
# Abstract:
#   Implements a request script.
#

if {[IsTrue $misc(ReloadConfig)] && [catch {source "../scripts/init.itcl"} error]} {
    iputs "Unable to load script configuration, contact a siteop."
    return -code error $error
}

namespace eval ::nxTools::Req {
    namespace import -force ::nxLib::*
}

# Request Procedures
######################################################################

proc ::nxTools::Req::CheckLimit {userName groupName} {
    global req
    foreach reqLimit $req(Limits) {
        if {[llength $reqLimit] != 5} {
            ErrorLog ReqLimit "wrong number of options in line: \"$reqLimit\""; continue
        }
        foreach {target groupLimit userLimit timeLimit timePeriod} $reqLimit {break}

        set query "SELECT COUNT(*) FROM Requests WHERE Status=0 AND UserName='[DbEscape $userName]'"
        if {[string index $target 0] eq "="} {
            set target [string range $target 1 end]
            if {$target ne $groupName} {continue}

            # Group request limits are only checked when 'target' has a group prefix (=).
            if {$groupLimit >= 0 && [ReqDb eval {SELECT COUNT(*) FROM Requests WHERE Status=0 AND GroupName=$target}] >= $groupLimit} {
                LinePuts "You have reached your group's request limit of $groupLimit request(s)."
                return 0
            }
            append query " AND GroupName='[DbEscape $target]'"
        } elseif {$target ne $userName && $target ne "*"} {
            continue
        }

        if {$userLimit >= 0 && [ReqDb onecolumn $query] >= $userLimit} {
            LinePuts "You have reached your individual request limit of $userLimit request(s)."
            return 0
        }

        if {$timeLimit >= 0 && $timePeriod >= 0} {
            set timeStamp [expr {[clock seconds] - ($timePeriod * 86400)}]

            if {[ReqDb onecolumn {SELECT COUNT(*) FROM Requests WHERE TimeStamp > $timeStamp AND UserName=$userName}] >= $timeLimit} {
                LinePuts "Only $timeLimit request(s) can be made every $timePeriod day(s)."
                set lastReq [ReqDb onecolumn {SELECT TimeStamp FROM Requests WHERE UserName=$userName ORDER BY TimeStamp DESC LIMIT 1}]
                set timeLeft [FormatDuration [expr {$lastReq - $timeStamp}]]
                LinePuts "You have to wait $timeLeft until you can request again."
                return 0
            }
        }
        break
    }
    return 1
}

proc ::nxTools::Req::UpdateDir {event request {userId 0} {groupId 0}} {
    global req
    if {![string length $req(RequestPath)]} {
        return
    } elseif {![file exists $req(RequestPath)]} {
        ErrorLog ReqUpdateDir "The requests directory \"$req(RequestPath)\" does not exist."
        return
    }
    set mapList [list %(request) $request]
    set reqPath [file join $req(RequestPath) [string map $mapList $req(RequestTag)]]

    switch -- $event {
        ADD {CreateTag $reqPath $userId $groupId 777}
        DEL {
            if {[file isdirectory $reqPath]} {
                KickUsers [file join $reqPath "*"] True
                if {[catch {file delete -force -- $reqPath} error]} {
                    ErrorLog ReqDelete $error
                }
            }
        }
        FILL {
            if {[file isdirectory $reqPath]} {
                set fillPath [file join $req(RequestPath) [string map $mapList $req(FilledTag)]]
                KickUsers [file join $reqPath "*"] True
                if {[catch {file rename -force -- $reqPath $fillPath} error]} {
                    ErrorLog ReqFill $error
                }
            }
        }
        default {
            ErrorLog ReqUpdateDir "unknown event \"$event\""
        }
    }
    catch {vfs flush $req(RequestPath)}
    return
}

# Request Events
######################################################################

proc ::nxTools::Req::Add {userName groupName request} {
    global misc req
    iputs ".-\[Request\]--------------------------------------------------------------."
    set result 1

    set request [StripChars $request]
    if {[IsTrue $req(ReleaseNames)] && ![regexp -- {^\S+-\S+$} $request]} {
        LinePuts "Invalid release format, must be: \"Release.Name-Group\"."
    } elseif {[ReqDb exists {SELECT 1 FROM Requests WHERE Status IN (0,1) AND StrCaseEq(Request,$request)}]} {
        LinePuts "This release is already filled or requested."
    } elseif {[CheckLimit $userName $groupName]} {
        set requestId 1
        ReqDb eval {SELECT (MAX(RequestId)+1) AS NextId FROM Requests WHERE Status=0} values {
            # The MAX() function returns NULL if there are no matching records.
            if {[string length $values(NextId)]} {
                set requestId $values(NextId)
            }
        }
        set timeStamp [clock seconds]
        ReqDb eval {INSERT INTO Requests(TimeStamp,UserName,GroupName,Status,RequestId,Request) VALUES($timeStamp,$userName,$groupName,0,$requestId,$request)}

        putlog "REQUEST: \"$userName\" \"$groupName\" \"$request\" \"$requestId\""
        LinePuts "Added your request of $request (#$requestId)."
        UpdateDir ADD $request [resolve user $userName] [resolve group $groupName]
        set result 0
    }

    iputs "'------------------------------------------------------------------------'"
    return $result
}

proc ::nxTools::Req::Update {event userName groupName flags request} {
    global misc req
    iputs ".-\[Request\]--------------------------------------------------------------."
    set exists 0; set result 0
    set request [StripChars $request]

    ReqDb eval {SELECT rowid,* FROM Requests WHERE Status=0 AND (RequestId=$request OR StrCaseEq(Request,$request)) LIMIT 1} values {set exists 1}
    if {!$exists} {
        LinePuts "Invalid request, use \"SITE REQUESTS\" to view current requests."
        set result 1
    } else {
        if {$event eq "FILL"} {
            if {$userName eq $values(UserName)} {
                LinePuts "You cannot fill your own requests."
                iputs "'------------------------------------------------------------------------'"
                return 1
            }

            ReqDb eval {UPDATE Requests SET Status=1 WHERE rowid=$values(rowid)}
            LinePuts "Filled request $values(Request) for $values(UserName)/$values(GroupName)."
            set logType "REQFILL"
        } elseif {$event eq "DEL"} {
            # Only siteops or the owner may delete a request.
            if {$userName ne $values(UserName) && ![MatchFlags $misc(SiteopFlags) $flags]} {
                LinePuts "You are not allowed to delete another user's request."
                iputs "'------------------------------------------------------------------------'"
                return 1
            }
            ReqDb eval {DELETE FROM Requests WHERE rowid=$values(rowid)}
            LinePuts "Deleted request $values(Request) for $values(UserName)/$values(GroupName)."
            set logType "REQDEL"
        }

        set age [expr {[clock seconds] - $values(TimeStamp)}]
        putlog "${logType}: \"$userName\" \"$groupName\" \"$values(Request)\" \"$values(UserName)\" \"$values(GroupName)\" \"$values(RequestId)\" \"$age\""
        UpdateDir $event $values(Request)
    }

    iputs "'------------------------------------------------------------------------'"
    return $result
}

proc ::nxTools::Req::List {} {
    global misc req

    foreach fileExt {Header Body None Footer} {
        set template($fileExt) [ReadFile [file join $misc(Templates) "Requests.$fileExt"]]
    }
    OutputText $template(Header)
    set count 0

    ReqDb eval {SELECT * FROM Requests WHERE Status=0 ORDER BY RequestId ASC} values {
        incr count
        set age [FormatDuration [expr {[clock seconds] - $values(TimeStamp)}]]
        set valueList [list [lrange $age 0 1] $values(RequestId) $values(UserName) $values(GroupName) $values(Request)]
        OutputText [ParseCookies $template(Body) $valueList {age id user group request}]
    }

    if {!$count} {OutputText $template(None)}
    OutputText $template(Footer)
    return 0
}

proc ::nxTools::Req::Wipe {} {
    global misc req
    iputs ".-\[Request\]--------------------------------------------------------------."

    if {$req(MaximumAge) < 1} {
        LinePuts "Request wiping is disabled, check your configuration."
    } else {
        LinePuts "Wiping filled requests older than $req(MaximumAge) day(s)..."
        set maxAge [expr {$req(MaximumAge) * 86400}]
        set timeStamp [expr {[clock seconds] - $maxAge}]

        ReqDb eval {SELECT rowid,* FROM Requests WHERE Status=1 AND TimeStamp < $timeStamp ORDER BY RequestId ASC} values {
            set age [expr {[clock seconds] - $values(TimeStamp)}]

            # Wipe the directory if it exists.
            set fillPath [string map [list %(request) $values(Request)] $req(FilledTag)]
            set fillPath [file join $req(RequestPath) $fillPath]
            if {[file isdirectory $fillPath]} {
                KickUsers [file join $fillPath "*"] True
                if {[catch {file delete -force -- $fillPath} error]} {
                    ErrorLog ReqWipe $error
                }
            }

            LinePuts "Wiped: $values(Request) by $values(UserName)/$values(GroupName) (#$values(RequestId))."
            putlog "REQWIPE: \"$values(UserName)\" \"$values(GroupName)\" \"$values(Request)\" \"$values(RequestId)\" \"$age\" \"$maxAge\""
            ReqDb eval {UPDATE Requests SET Status=2 WHERE rowid=$values(rowid)}
        }
        catch {vfs flush $req(RequestPath)}
    }

    iputs "'------------------------------------------------------------------------'"
    return 0
}

# Request Main
######################################################################

proc ::nxTools::Req::Main {argv} {
    global misc ioerror group user flags
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set result 0

    if {[catch {DbOpenFile [namespace current]::ReqDb "Requests.db"} error]} {
        ErrorLog RequestDb $error
        iputs "Unable to open requests database."
        return 1
    }

    set argLength [llength [set argList [ListParse $argv]]]
    set event [string toupper [lindex $argList 0]]
    set request [join [lrange $argList 1 end]]

    if {$event eq "BOT"} {
        if {$argLength != 4} {
            iputs "Syntax: SITE REQBOT <event> <user> <request>"
        } else {
            foreach {event user request} [lrange $argList 1 end] {break}
            set event [string toupper $event]
            GetUserInfo $user group flags
        }
    }

    switch -- $event {
        ADD {
            if {$argLength > 1} {
                set result [Add $user $group $request]
            } else {
                iputs "Syntax: SITE REQUEST <request>"
            }
        }
        BOT {}
        DEL - FILL {
            if {$argLength > 1} {
                set result [Update $event $user $group $flags $request]
            } else {
                iputs "Syntax: SITE REQ$event <id/request>"
            }
        }
        LIST {set result [List]}
        WIPE {set result [Wipe]}
        default {
            ErrorLog InvalidArgs "unknown event \"[info script] $event\": check your ioFTPD.ini for errors"
            set result 1
        }
    }

    ReqDb close
    return [set ioerror $result]
}

::nxTools::Req::Main [expr {[info exists args] ? $args : ""}]
