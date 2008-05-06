if {![package vsatisfies [package provide Tcl] 8.4]} {return}
package ifneeded alco::config 1.3.0 [list source [file join $dir "config.tcl"]]
package ifneeded alco::db     1.3.0 [list source [file join $dir "db.tcl"]]
package ifneeded alco::ftp    1.3.0 [list source [file join $dir "ftp.tcl"]]
package ifneeded alco::getopt 1.3.0 [list source [file join $dir "getopt.tcl"]]
package ifneeded alco::tree   1.3.0 [list source [file join $dir "tree.tcl"]]
package ifneeded alco::uri    1.3.0 [list source [file join $dir "uri.tcl"]]
package ifneeded alco::util   1.3.0 [list source [file join $dir "util.tcl"]]
