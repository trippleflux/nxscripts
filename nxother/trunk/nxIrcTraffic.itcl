################################################################################
# nxIrcTraffic - Announce Traffic                                              #
################################################################################
# Author  : neoxed                                                             #
# Date    : 18/09/2004                                                         #
# Version : 1.0.0 (rev 003)                                                    #
################################################################################
#
# Installation:
# 1. Copy the nxIrcTraffic.itcl file to x:\ioFTPD\scripts
# 2. Configure the script options.
# 3. Add the following to the ioFTPD.ini:
#
# [FTP_Custom_Commands]
# irctraffic = TCL ..\scripts\nxIrcTraffic.itcl
#
# [FTP_SITE_Permissions]
# irctraffic = 1M
#
# 4. You may also schedule this script to automate the procedure.
# 5. Rehash ioFTPD for the changes to take affect (SITE CONFIG REHASH).
# 6. Open dZSbot.tcl or ioBanana.tcl in a text editor, and add the following lines:
#
# set disable(IRCTRAFFIC)    0
# set variables(IRCTRAFFIC) "%traffic"
# set announce(IRCTRAFFIC)  "-%sitename- \[Traffic\] + %traffic"
#
# Find: "msgtypes(DEFAULT)", and add "IRCTRAFFIC" in it.
# set msgtypes(DEFAULT)	"IRCTRAFFIC INVITE LOGIN LOGOUT ..."
#
# 7. Rehash Eggdrop for the changes to take effect (.rehash in the party-line).
# 8. Login to your FTP and run 'SITE IRCTRAFFIC <type>'.
# 9. Good luck! ;)
#
# Change Log:
#
# v1.0.0 - Initial release.
#
################################################################################

#### Traffic Log Types
#
# - Format: traffic(<type>) <log text>
# - Usage:
#    Site Cmd : SITE IRCTRAFFIC <type>
#    Scheduler: nxTraffic<type> = 59 23 * * TCL ..\scripts\nxIrcTraffic.itcl <type>
# - Cookies:
#    %B = Bold, %C = Colour, %U = Underline, %O = Reset Codes
#
#    Amount:          Files:          Speed:
#    %amount_alldn   %files_alldn   %speed_alldn
#    %amount_allup   %files_allup   %speed_allup
#    %amount_daydn   %files_daydn   %speed_daydn
#    %amount_dayup   %files_dayup   %speed_dayup
#    %amount_monthdn %files_monthdn %speed_monthdn
#    %amount_monthup %files_monthup %speed_monthup
#    %amount_wkdn    %files_wkdn    %speed_wkdn
#    %amount_wkup    %files_wkup    %speed_wkup
#
set traffic(day)	"Another fine day has ended on %BNX%B. Traffic today was %B%amount_dayup%B uploaded and %B%amount_daydn%B downloaded."
set traffic(week)	"Another fine week has ended on %BNX%B. Traffic this week was %B%amount_wkup%B uploaded and %B%amount_wkdn%B downloaded."

## End of Settings
################################################################################

proc FormatSize {Size} {
	if {$Size >= 1073741824} {
		return [format "%.2f TB" [expr {double($Size) / 1073741824.0}]]
	} elseif {$Size >= 1048576} {
		return [format "%.2f GB" [expr {double($Size) / 1048576.0}]]
	} elseif {$Size >= 1024} {
		return [format "%.1f MB" [expr {double($Size) / 1024.0}]]
	} else {
		return [format "%.0f KB" [expr {double($Size)}]]
	}
}

proc FormatSpeed {Speed {Seconds "0"}} {
	if {$Seconds > 0} {set Speed [expr {double($Speed) / $Seconds}]}
	if {$Speed >= 1048576} {
		return [format "%.2f GB/s" [expr {double($Speed) / 1048576.0}]]
	} elseif {$Speed >= 1024} {
		return [format "%.2f MB/s" [expr {double($Speed) / 1024.0}]]
	} else {
		return [format "%.0f KB/s" [expr {double($Speed)}]]
	}
}

proc IrcTraffic {} {
	global args traffic
	if {![info exists args] || [string equal "" $args]} {
		iputs "Usage: SITE IRCTRAFFIC <type>"; return 1
	}
	set TrafficType [lindex $args 0]; set TrafficLog ""
	foreach {Name Value} [array get traffic] {
		if {[string equal -nocase $Name $TrafficType]} {
			set TrafficLog $Value; break
		}
	}
	if {[string equal "" $TrafficLog]} {
		iputs "Unknown traffic type \"$TrafficType\"."
		iputs "Valid traffic types: [array names traffic]"; return 1
	}
	iputs "Traffic: $TrafficType"

	## Add stats
	iputs "Adding user stats..."
	array set file [list alldn 0 allup 0 daydn 0 dayup 0 monthdn 0 monthup 0 wkdn 0 wkup 0]
	array set size [list alldn 0 allup 0 daydn 0 dayup 0 monthdn 0 monthup 0 wkdn 0 wkup 0]
	array set time [list alldn 0 allup 0 daydn 0 dayup 0 monthdn 0 monthup 0 wkdn 0 wkup 0]
	foreach UserID [user list] {
		set UserName [resolve uid $UserID]
		if {[userfile open $UserName] != 0} {continue}
		set UserFile [userfile bin2ascii]
		foreach UserLine [split $UserFile "\n"] {
			set LineType [string tolower [lindex $UserLine 0]]
			if {[lsearch -exact "alldn allup daydn dayup monthdn monthup wkdn wkup" $LineType] != -1} {
				for {set Index 1} {$Index < 31} {incr Index} {
					set file($LineType) [expr {wide($file($LineType)) + wide([lindex $UserLine $Index])}]
					incr Index
					set size($LineType) [expr {wide($size($LineType)) + wide([lindex $UserLine $Index])}]
					incr Index
					set time($LineType) [expr {wide($time($LineType)) + wide([lindex $UserLine $Index])}]
				}
			}
		}
	}

	## Build cookie map list
	iputs "Building cookie list..."
	set CookieMap [list %B \002 %C \003 %U \037 %O \015]
	foreach StatsType "alldn allup daydn dayup monthdn monthup wkdn wkup" {
		lappend CookieMap "%files_$StatsType" $file($StatsType)
		lappend CookieMap "%amount_$StatsType" [FormatSize $size($StatsType)]
		lappend CookieMap "%speed_$StatsType" [FormatSpeed $size($StatsType) $time($StatsType)]
	}

	## Log line
	putlog "IRCTRAFFIC: \"[string map $CookieMap $TrafficLog]\""
	iputs "Finished."
	return 0
}

IrcTraffic
