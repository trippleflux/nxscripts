#
# nxSDK - ioFTPD Software Development Kit
# Copyright (c) 2006 neoxed
#
# Module Name:
#   Gen Docs
#
# Author:
#   neoxed (neoxed@gmail.com) May 13, 2006
#
# Abstract:
#   Generates source documentation.
#

# Base directory to change to.
set baseDir ".."

# Location for the output file.
set outputFile "docs.txt"

# Location of the input files.
set inputFiles {
    "src/nxsdk.h"
    "src/lib/*.c"
    "src/lib/*.h"
}

puts "\n\tGenerating Source Documents\n"

puts "- Changing to base directory"
set currentDir [pwd]
cd $baseDir

proc ReadFile {path} {
    set handle [open $path "r"]
    set data [read $handle]
    close $handle
    return $data
}

proc ParseText {text} {
    set status 0
    set result [list]

    foreach line [split $text "\n"] {
        set line [string trimright $line]

        # status values:
        # 0 - Ignore the line.
        # 1 - In a marked block comment (/*++ ... --*/).
        # 2 - After the end of a block comment.

        if {$line eq "/*++"} {
            if {$status == 2} {
                lappend result $desc $code
                set code [list]
                set desc [list]
                set status 0
            } elseif {$status == 1} {
                puts "    - Found comment start but we are already in a comment block."
            } else {
                set status 1
                set code [list]
                set desc [list]
            }

        } elseif {$line eq "--*/"} {
            if {$status != 1} {
                puts "    - Found comment end but we are not in a comment block."
            } else {
                set status 2
            }

        } elseif {$status == 2} {
            if {$line eq ""} {
                lappend result $desc $code
                set code [list]
                set desc [list]
                set status 0
            } else {
                lappend code $line
            }

        } elseif {$status == 1} {
            lappend desc $line
        }
    }

    return $result
}

proc ParseList {items} {
    # Remove empty leading elements.
    while {[llength $items] && [lindex $items 0] eq ""} {
        set items [lreplace $items 0 0]
    }

    # Remove empty trailing elements.
    while {[llength $items] && [lindex $items end] eq ""} {
        set items [lreplace $items end end]
    }
    return $items
}

puts "- Reading source files"
set funcList [list]
set structList [list]

foreach pattern $inputFiles {
    foreach path [glob -nocomplain $pattern] {
        puts "  - Reading file: $path"

        foreach {desc code} [ParseText [ReadFile $path]] {
            # Count and remove outer empty lines.
            set beforeCount [llength $desc]
            set desc [ParseList $desc]
            set afterCount [llength $desc]

            if {!$afterCount} {
                puts "    - Empty comment block."
                continue
            }
            set diffCount [expr {$beforeCount - $afterCount}]
            if {$diffCount != 2} {
                puts "    - Found $diffCount outer empty lines in a comment block, should be 2 empty lines."
            }

            # Detect function and structure comments.
            if {[lsearch -exact $desc "Arguments:"] != -1 && [lsearch -exact $desc "Return Value:"] != -1} {
                lappend funcList $desc $code

            } elseif {[lsearch -exact $desc "Members:"] != -1} {
                lappend structList $desc $code

            } else {
                set sections [join [lsearch -all -inline -regexp $desc {^[\s\w]+:$}] {, }]
                puts "    - Unknown comment type, sections are \"$sections\"."
            }
        }
    }
}

puts "- Changing back to original directory"
cd $currentDir

puts "- Parsing structures"
set structId 0
set structMap [list]
unset -nocomplain structs

foreach {desc code} $structList {
    set name [lindex $desc 0]
    puts "  - Structure: $name"

    if {![string is wordchar -strict $name]} {
        puts "    - Invalid structure name, skipping."
        continue
    }
    if {[info exists structs($name)]} {
        puts "    - Structure already defined, skipping."
        continue
    }

    set section "intro"
    array set text [list intro "" members "" remarks ""]
    foreach line $desc {
        switch -regexp -- $line {
            {^Members:$} {set section "members"}
            {^Remarks:$} {set section "remarks"}
            {^[\s\w]+:$} {puts "    - Unknown comment section \"$line\"."}
            default      {lappend text($section) $line}
        }
    }

    set target "struct[incr structId]"
    lappend structMap $name "structs.htm#$target"
    set structs($name) [list $target $text(intro) $text(members) $text(remarks)]
}

puts "- Parsing functions"
set funcId 0
set funcMap [list]
unset -nocomplain funcs

foreach {desc code} $funcList {
    set name [lindex $desc 0]
    puts "  - Function: $name"

    if {![string is wordchar -strict $name]} {
        puts "    - Invalid function name, skipping."
        continue
    }
    if {[info exists funcs($name)]} {
        puts "    - Function already defined, skipping."
        continue
    }

    set section "intro"
    array set text [list intro "" args "" remarks "" retval ""]
    foreach line $desc {
        switch -regexp -- $line {
            {^Arguments:$}    {set section "args"}
            {^Remarks:$}      {set section "remarks"}
            {^Return Value:$} {set section "retval"}
            {^[\s\w]+:$}      {puts "    - Unknown comment section \"$line\"."}
            default           {lappend text($section) $line}
        }
    }

    set target "func[incr structId]"
    lappend structMap $name "funcs.htm#$target"
    set funcs($name) [list]
}

puts "- Opening output file"
set handle [open $outputFile "w"]
puts $handle "Source Docs"
puts $handle ""

puts "- Writing index"
set funcNames [lsort [array names funcs]]
foreach name $funcNames {
}

set structNames [lsort [array names structs]]
foreach name $structNames {
}

puts "- Writing descriptions"
foreach name $funcNames {
}

foreach name $structNames {
}

puts "- Finished"
close $handle
return 0
