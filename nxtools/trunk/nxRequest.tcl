################################################################################
# nxTools - Request Script                                                     #
################################################################################
# Author  : $-66(AUTHOR) #
# Date    : $-66(TIMESTAMP) #
# Version : $-66(VERSION) #
################################################################################

# Load Libraries
######################################################################

if {[catch {source [file join [file dirname [info script]] "nxLib.itcl"]} ErrorMsg]} {
    iputs "Error loading nxLib: $ErrorMsg"; return
}
if {[catch {load "tclsqlite3.dll" Tclsqlite3} ErrorMsg]} {
    iputs "Error loading TclSQLite: $ErrorMsg"; return
}

# Request Functions
######################################################################

proc ReqCheckLimit {DbProc UserName} {
    global req
    if {[$DbProc eval {SELECT count(*) FROM Requests WHERE Status=0}] >= $req(TotalLimit)} {
        LinePuts "The maximum request limit has been reached, $req(TotalLimit) requests."
        return 0
    }
    if {[$DbProc eval {SELECT count(*) FROM Requests WHERE Status=0 AND UserName=$UserName}] >= $req(UserLimit)} {
        LinePuts "You have made to many requests, the limit is $req(UserLimit) request(s) per user."
        return 0
    }
    if {$req(TimeDays) > 0 && $req(TimeLimit) > 0} {
        set TimeStamp [expr {[clock seconds] - $req(TimeDays) * 86400}]
        if {[$DbProc eval {SELECT count(*) FROM Requests WHERE TimeStamp > $TimeStamp AND UserName=$UserName}] >= $req(TimeLimit)} {
            LinePuts "You have reached your request limit for the time being."
            LinePuts "Only $req(TimeLimit) request(s) are allowed every $req(TimeDays) day(s)."
            return 0
        }
    }
    return 1
}

proc ReqUpdateDir {Action Request {UserId 0} {GroupId 0}} {
    global req
    if {[string equal "" $req(RequestPath)]} {
        return
    } elseif {[file exists $req(RequestPath)]} {
        set ReMap [list %(request) $Request]
        set ReqPath [file join $req(RequestPath) [string map $ReMap $req(RequestTag)]]
        switch -exact -- $Action {
            {del} {
                if {[file isdirectory $ReqPath]} {
                    ## Kick users in the directory
                    KickUsers [file join $ReqPath "*"] True
                    if {[catch {file delete -force -- $ReqPath} ErrorMsg]} {
                        ErrorLog ReqDelete $ErrorMsg
                    }
                }
            }
            {fill} {
                if {[file isdirectory $ReqPath]} {
                    set FillPath [file join $req(RequestPath) [string map $ReMap $req(FilledTag)]]
                    ## Kick users in the directory
                    KickUsers [file join $ReqPath "*"] True
                    if {[catch {file rename -force -- $ReqPath $FillPath} ErrorMsg]} {
                        ErrorLog ReqFill $ErrorMsg
                    }
                }
            }
            {req} {
                CreateTag $ReqPath $UserId $GroupId 777
            }
            default {
                ErrorLog ReqUpdateDir "Unknown event \"$Action\"."
            }
        }
        catch {vfs flush $req(RequestPath)}
    } else {
        ErrorLog ReqUpdateDir "The requests directory \"$req(RequestPath)\" does not exist."
    }
}

# Request Main
######################################################################

proc ReqMain {ArgV} {
    global misc req flags group user
    if {[IsTrue $misc(DebugMode)]} {DebugLog -state [info script]}
    set IsSiteBot [expr {[info exists user] && [string equal $misc(SiteBot) $user]}]

    ## Safe argument handling
    set ArgList [ArgList $ArgV]
    set Action [string tolower [lindex $ArgList 0]]

    set Request [join [lrange $ArgList 1 end]]

    if {[string equal "view" $Action]} {
        set Action "view"; set ShowText 0
    } else {
        iputs ".-\[Request\]--------------------------------------------------------------."
        set ShowText 1
    }

    if {[catch {DbOpenFile ReqDb "Requests.db"} ErrorMsg]} {
        ErrorLog RequestDb $ErrorMsg
        if {$ShowText && !$IsSiteBot} {ErrorReturn "Unable to open requests database."}
        return 1
    }
    ReqDb function StrEq {string equal -nocase}

    switch -exact -- $Action {
        {add} {
            set Request [StripChars $Request]
            if {[ReqDb eval {SELECT count(*) FROM Requests WHERE Status=0 AND StrEq(Request,$Request)}]} {
                LinePuts "This item is already requested."
            } elseif {[ReqCheckLimit ReqDb $user]} {
                set RequestId 1
                ReqDb eval {SELECT (max(RequestId)+1) AS NextId FROM Requests WHERE Status=0} values {
                    ## The max() function returns NULL if there are no matching records.
                    if {[llength $values(NextId)]} {
                        set RequestId $values(NextId)
                    }
                }
                set TimeStamp [clock seconds]
                ReqDb eval {INSERT INTO Requests (TimeStamp,UserName,GroupName,Status,RequestId,Request) VALUES($TimeStamp,$user,$group,0,$RequestId,$Request)}

                set RequestId [format "%03s" $RequestId]
                putlog "REQUEST: \"$user\" \"$group\" \"$Request\" \"$RequestId\""
                LinePuts "Added your request of $Request (#$RequestId)."
                ReqUpdateDir $Action $Request
            }
        }
        {del} - {fill} {
            set Exists 0
            ReqDb eval {SELECT rowid,* FROM Requests WHERE Status=0 AND (RequestId=$RequestId OR StrEq(Request,$Request)) LIMIT 1} values {set Exists 1}
            if {!$Exists} {
                ReqDb close
                ErrorReturn "Invalid request, use \"SITE REQUEST\" to view current requests."
            }
            if {[string equal "fill" $Action]} {
                ReqDb eval {UPDATE Requests SET Status=1 WHERE rowid=$values(rowid)}
                LinePuts "Filled request $values(Request) by $values(UserName)/$values(GroupName)."
                set LogPrefix "REQFILL"
            } elseif {[string equal "del" $Action]} {
                ## Only siteops or the requester may delete a request
                if {![string equal $user $values(UserName)] && ![regexp "\[$misc(SiteopFlags)\]" $flags]} {
                    ReqDb close
                    ErrorReturn "You are not allowed to delete another user's request."
                }
                ReqDb eval {DELETE FROM Requests WHERE rowid=$values(rowid)}
                LinePuts "Deleted request $values(Request) by $values(UserName)/$values(GroupName)."
                set LogPrefix "REQDEL"
            }

            set RequestAge [expr {[clock seconds] - $values(TimeStamp)}]
            set RequestId [format "%03s" $values(RequestId)]
            if {[IsTrue $misc(dZSbotLogging)]} {
                set RequestAge [FormatDuration $RequestAge]
            }
            putlog "${LogPrefix}: \"$user\" \"$group\" \"$values(Request)\" \"$values(UserName)\" \"$values(GroupName)\" \"$RequestId\" \"$RequestAge\""
            ReqUpdateDir $Action $values(Request)
        }
        {view} {
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
                    iputs "REQS|$values(TimeStamp)|$RequestAge|$RequestId|$values(UserName)|$values(GroupName)|$values(Request)"
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
        {wipe} {
            if {$req(MaximumAge) == 0} {
                LinePuts "Request wiping is disabled, check your configuration."
            } else {
                LinePuts "Wiping filled requests older then $req(MaximumAge) day(s)..."
                set MaxAge [expr {[clock seconds] - $req(MaximumAge) * 86400}]

                ReqDb eval {SELECT rowid,* FROM Requests WHERE Status=1 AND TimeStamp < $MaxAge ORDER BY RequestId DESC} values {
                    set RequestAge [expr {[clock seconds] - $values(TimeStamp)}]
                    set RequestId [format "%03s" $values(RequestId)]

                    ## Wipe the directory if it exists
                    set FillPath [string map [list %(request) $values(Request)] $req(FilledTag)]
                    set FillPath [file join $req(RequestPath) $FillPath]
                    if {[file isdirectory $FillPath]} {
                        ## Kick users in the directory
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
        }
        default {
            ErrorLog "Invalid function \"[info script] $Action\", check your ioFTPD.ini for errors."
        }
    }
    if {$ShowText} {iputs "'------------------------------------------------------------------------'"}
    ReqDb close
    return 0
}

ReqMain [expr {[info exists args] ? $args : ""}]
