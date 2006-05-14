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

proc ReadFile {path} {
    set handle [open $path "r"]
    set data [read $handle]
    close $handle
    return $data
}

proc ParseBlocks {text} {
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

puts "\n\tGenerating Source Documents\n"

puts "- Changing to base directory"
set currentDir [pwd]
cd $baseDir

puts "- Parsing source files"
set funcList [list]
set structList [list]

foreach pattern $inputFiles {
    foreach path [glob -nocomplain $pattern] {
        puts "  - Parsing file: $path"
        set text [ReadFile $path]

        foreach {desc code} [ParseBlocks $text] {
            # Detect between function and structure comments.
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

puts "- Transforming data"
unset -nocomplain funcs structs

foreach {desc code} $structList {
}

foreach {desc code} $funcList {
}

puts "- Writing output file"
set handle [open $outputFile "w"]
set funcNames [lsort [array names funcs]]
set structNames [lsort [array names structs]]

puts "- Writing index"
foreach name $funcNames {
}
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
