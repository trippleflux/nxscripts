################################################################################
# FakeLogs - Generate Fake Dupe Logs                                           #
################################################################################
# Author  : neoxed                                                             #
# Date    : 02/09/2004                                                         #
# Version : 1.0.0                                                              #
################################################################################

#### Settings ##############################################
#
# - List of file extensions to use when generating a file log.
# - List of user names which are chosen at random.
# - List of group names which are chosen at random.
# - Log files to generate, format: {<database> <file mode> <# of lines>}
#
set fake(ExtList)	".zip .rar .sfv .avi .mpg .r00 .r01 .r02 .r03 .r04 .r05 .r06 .r07 .r08 .r09 .r10 .r11 .r12 .r13 .r14 .r15 .r16 .r17 .r18 .r19 .r20 .r21 .r22 .r23 .r24 .r25 .r26 .r27 .r28 .r29 .r30 .r31 .r32 .r33 .r34 .r35 .r36 .r37 .r38 .r39 .r40"
set fake(UserList)	"Bob Chris Henry Sue Marge Darryl Guy Brad Shawn James Paul Billy Luke Wayne Richard Stanley Peter Timothy John Homer Bart Lisa Jerry George Bjorn Lois Max Ted"
set fake(GroupList)	"Admins Clowns DipShits Morons Fags Freaks AssHoles Friends Aussies Fools Nerds Bitches Nobodies Loaners Canucks JackAsses Losers"
set fake(LogFiles) {
	{"DupeDirs.db" 0 50000}
	{"DupeFiles.db" 1 50000}
}

#### End of Settings #######################################

load {E:\ioFTPD\system\tclsqlite3.dll}

proc ListRandom {List} {
	lindex $List [Random [llength $List]]
}

proc Random {MaxInt} {
	expr {int(rand() * $MaxInt)}
}

proc RandomString {MaxLength {CharSet "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"}} {
	set CharLength [string length $CharSet]
	set RanString ""
	for {set Index 0} {$Index < $MaxLength} {incr Index} {
		append RanString [string index $CharSet [Random $CharLength]]
	}
	return $RanString
}

proc FakeLogs {} {
	global fake
	set TimeOps {+ -}
	foreach FakeLog $fake(LogFiles) {
		set i 0
		set TimeStamp [clock seconds]
		foreach {DbFile FileMode LogLines} $FakeLog {break}
		sqlite3 db $DbFile

		if {$FileMode} {
db eval {CREATE TABLE DupeFiles(
TimeStamp INTEGER default 0,
UserName  TEXT default '',
GroupName TEXT default '',
FileName  TEXT default '')}
		} else {
db eval {CREATE TABLE DupeDirs(
TimeStamp INTEGER default 0,
UserName  TEXT default '',
GroupName TEXT default '',
DirPath   TEXT default '',
DirName   TEXT default '')}
		}

		db eval {BEGIN}

		puts "\[*\] Generating fake db: $DbFile"
		while {$i < $LogLines} {
			set Segments [expr {1 + [Random 4]}]

			set TempStamp [expr $TimeStamp [ListRandom $TimeOps] [Random 99999999]]
			set UserName [ListRandom $fake(UserList)]
			set GroupName [ListRandom $fake(GroupList)]

			if {$FileMode} {
				set FileName [RandomString [expr {7 + [Random 15]}]]
				append FileName [ListRandom $fake(ExtList)]

                db eval {INSERT INTO DupeFiles (TimeStamp,UserName,GroupName,FileName) VALUES($TempStamp,$UserName,$GroupName,$FileName)}
			} else {
				set DirPath "/"
				for {set s 0} {$s < $Segments} {incr s} {
					append DirPath [RandomString [expr {4 + [Random 15]}]] "/"
				}
				set DirName [RandomString [expr {25 + [Random 25]}]]

				db eval {INSERT INTO DupeDirs (TimeStamp,UserName,GroupName,DirPath,DirName) VALUES($TempStamp,$UserName,$GroupName,$DirPath,$DirName)}
			}

			incr i
		}

		db eval {COMMIT}
		db close
		puts "\[*\] Done, $i entries."
	}
}

FakeLogs
