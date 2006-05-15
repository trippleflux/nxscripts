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

# Location of the input files.
set inputFiles {
    "include/nxsdk.h"
    "src/lib/*.c"
    "src/lib/*.h"
}

################################################################################

proc BufferFile {path} {
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

################################################################################

proc ListTrim {items} {
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

proc ListToArgs {items} {
    # Group arguments.
    set count 0
    foreach line [ListTrim $items] {
        set line [string trimleft $line]

        if {[regexp -- {^(\w+)\s+-\s+(.+)$} $line dummy name line]} {
            incr count
            lappend arg($count) $name $line
        } elseif {$count} {
            lappend arg($count) $line
        } else {
            puts "    - Found argument description before definition."
        }
    }

    # Format the result: argOne {paraOne paraTwo} argTwo {paraOne paraTwo} ...
    set count 1
    set result [list]
    while {[info exists arg($count)]} {
        set name [lindex $arg($count) 0]
        set text [lrange $arg($count) 1 end]
        lappend result $name [ListToText $text]
        incr count
    }
    return $result
}

proc ListToText {items} {
    # Group paragraphs.
    set count 0
    foreach line [ListTrim $items] {
        set line [string trimleft $line]

        if {$line eq ""} {
            incr count
        } else {
            lappend para($count) $line
        }
    }

    # Format the result: paraOne paraTwo paraThree ...
    set count 0
    set result [list]
    while {[info exists para($count)]} {
        lappend result [join $para($count)]
        incr count
    }
    return $result
}

proc ListToHtml {textList {join "\n"}} {
    set result [list]
    foreach para $textList {
        lappend result "<p>$para</p>"
    }
    return [join $result $join]
}

################################################################################

proc EscapeChars {text} {
    # Special HTML characters.
    string map [list "&" "&amp" ">" "&gt;" "<" "&lt" "\"" "&quot;"] $text
}

proc MapArgs {mapping argList} {
    set result [list]
    foreach {argName textList} $argList {
        lappend result $argName [MapText $mapping $textList]
    }
    return $result
}

proc MapText {mapping textList} {
    set result [list]
    foreach para $textList {
        lappend result [string map $mapping [EscapeChars $para]]
    }
    return $result
}

################################################################################

proc WriteHeader {handle title} {
    puts $handle "<html>"
    puts $handle "<head>"
    puts $handle "  <title>$title</title>"
    puts $handle {
  <style>
    body {
      font-family:verdana,arial,helvetica;
      margin:0;
    }
  </style>

  <link rel="stylesheet" type="text/css" href="ie4.css" />
  <link rel="stylesheet" type="text/css" href="ie5.css" />
</head>

<body topmargin="0" leftmargin="0" marginheight="0" marginwidth="0" bgcolor="#FFFFFF" text="#000000">
<table class="clsContainer" cellpadding="10" cellspacing="0" float="left" width="100%" border="1">}

    puts $handle "<tr>"
    puts $handle "<td valign=\"top\">"
    puts $handle "  <center><h1>$title</h1></center>"
}

proc WriteIndex {handle anchors} {
    puts $handle "  <p><ul>"
    foreach {name target} $anchors {
    puts $handle "  <li>$target</li>"
    }
    puts $handle "  </ul></p>"
    puts $handle "</td>"
    puts $handle "</tr>"
    puts $handle ""
}

proc WriteEntry {handle name anchor intro code args} {
    array set opt $args
    set joinStr "\n  "

    puts $handle "<!-- Start $name -->"
    puts $handle "<tr>"
    puts $handle "<td valign=\"top\">"
    puts $handle "<h2><a name=\"$anchor\"></a>$name</h2>"
    puts $handle ""

    # Intro
    puts $handle "<p>"
    puts $handle [ListToHtml $intro]
    puts $handle "</p>"
    puts $handle ""

    # Code
    puts $handle "<pre class=\"syntax\">"
    puts $handle [join $code \n]
    puts $handle "</pre>"
    puts $handle ""

    # Arguments
    if {[info exists opt(-args)] && [llength $opt(-args)]} {
        puts $handle "<h4>Arguments</h4>"
        puts $handle "<dl>"
        foreach {argName argText} $opt(-args) {
            puts $handle "  <dt><i>$argName</i></dt>"
            puts $handle "  <dd>[ListToHtml $argText $joinStr]</dd>"
        }
        puts $handle "</dl>"
        puts $handle ""
    }

    # Members
    if {[info exists opt(-members)] && [llength $opt(-members)]} {
        puts $handle "<h4>Members</h4>"
        puts $handle "<dl>"
        foreach {memberName memberText} $opt(-members) {
            puts $handle "  <dt><i>$memberName</i></dt>"
            puts $handle "  <dd>[ListToHtml $memberText $joinStr]</dd>"
        }
        puts $handle "</dl>"
        puts $handle ""
    }

    # Return values
    if {[info exists opt(-retvals)] && [llength $opt(-retvals)]} {
        puts $handle "<h4>Return Values</h4>"
        puts $handle [ListToHtml $opt(-retvals)]
        puts $handle ""
    }

    # Remarks
    if {[info exists opt(-remarks)] && [llength $opt(-remarks)]} {
        puts $handle "<h4>Remarks</h4>"
        puts $handle [ListToHtml $opt(-remarks)]
        puts $handle ""
    }

    puts $handle "</td>"
    puts $handle "</tr>"
    puts $handle "<!-- End $name -->"
    puts $handle ""
}

proc WriteFooter {handle} {
    puts $handle "</table>"
    puts $handle "</body>"
    puts $handle "</html>"
}

################################################################################

puts "\n\tGenerating Source Documents\n"

puts "- Changing to base directory"
set currentDir [pwd]
cd $baseDir

puts "- Reading source files"
set funcList [list]
set structList [list]

foreach pattern $inputFiles {
    foreach path [glob -nocomplain $pattern] {
        puts "  - Reading file: $path"

        foreach {desc code} [ParseText [BufferFile $path]] {
            # Count and remove outer empty lines.
            set beforeCount [llength $desc]
            set desc [ListTrim $desc]
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
            if {[lsearch -exact $desc "Arguments:"] != -1 && [lsearch -exact $desc "Return Values:"] != -1} {
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

################################################################################

puts "- Parsing data"
set funcLinks [list]
set structLinks [list]
unset -nocomplain funcs structs

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

    # Parse comment
    set section "intro"
    array set text [list intro "" args "" remarks "" retvals ""]
    foreach line [lrange $desc 1 end] {
        switch -regexp -- $line {
            {^Arguments:$}     {set section "args"}
            {^Remarks:$}       {set section "remarks"}
            {^Return Values:$} {set section "retvals"}
            {^[\s\w]+:$}       {puts "    - Unknown comment section \"$line\"."}
            default            {lappend text($section) $line}
        }
    }
    set text(intro)   [ListToText $text(intro)]
    set text(args)    [ListToArgs $text(args)]
    set text(remarks) [ListToText $text(remarks)]
    set text(retvals) [ListToText $text(retvals)]

    # Parse code
    set result [list]
    foreach line $code {
        set check [string trimleft $line]
        if {$check eq ")" || $check eq ");"} {
            lappend result [EscapeChars ");"]
            break
        }
        lappend result [EscapeChars $line]
    }
    set code $result

    set anchor "#[string tolower $name]_func"
    lappend funcLinks $name "<a href=\"functions.htm$anchor\"><b>$name</b></a>"
    set funcs($name) [list $anchor $code $text(intro) $text(args) $text(remarks) $text(retvals)]
}

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

    # Parse comment
    set section "intro"
    array set text [list intro "" members "" remarks ""]
    foreach line [lrange $desc 1 end] {
        switch -regexp -- $line {
            {^Members:$} {set section "members"}
            {^Remarks:$} {set section "remarks"}
            {^[\s\w]+:$} {puts "    - Unknown comment section \"$line\"."}
            default      {lappend text($section) $line}
        }
    }
    set text(intro)   [ListToText $text(intro)]
    set text(members) [ListToArgs $text(members)]
    set text(remarks) [ListToText $text(remarks)]

    # Parse code
    set result [list]
    foreach line $code {
        lappend result [EscapeChars $line]
        set check [string trimleft $line]
        if {[string match "\} *;" $check]} {break}
    }
    set code $result

    set anchor "#[string tolower $name]_struct"
    lappend structLinks $name "<a href=\"structures.htm$anchor\"><b>$name</b></a>"
    set structs($name) [list $anchor $code $text(intro) $text(members) $text(remarks)]
}

################################################################################

puts "- Transforming data"

set funcNames [lsort [array names funcs]]
set structNames [lsort [array names structs]]

# Bold names and link references to other functions and structures.
foreach name $funcNames {
    foreach {anchor code intro args remarks retvals} $funcs($name) {break}

    set mapping [list $name "<b>$name</b>"]
    eval lappend mapping $funcLinks $structLinks

    set intro   [MapText $mapping $intro]
    set args    [MapArgs $mapping $args]
    set remarks [MapText $mapping $remarks]
    set retvals [MapText $mapping $retvals]

    set funcs($name) [list $anchor $code $intro $args $remarks $retvals]
}

foreach name $structNames {
    foreach {anchor code intro members remarks} $structs($name) {break}

    set mapping [list $name "<b>$name</b>"]
    eval lappend mapping $funcLinks $structLinks

    set intro   [MapText $mapping $intro]
    set members [MapArgs $mapping $members]
    set remarks [MapText $mapping $remarks]

    set structs($name) [list $anchor $code $intro $members $remarks]
}

################################################################################

puts "- Writing functions"

set handle [open "functions.htm" "w"]
WriteHeader $handle "Functions"
WriteIndex $handle $funcLinks

foreach name $funcNames {
    foreach {anchor code intro args remarks retvals} $funcs($name) {break}
    WriteEntry $handle $name $anchor $intro $code \
        -args $args -remarks $remarks -retvals $retvals
}

WriteFooter $handle
close $handle

puts "- Writing structures"
set handle [open "structures.htm" "w"]
WriteHeader $handle "Structures"
WriteIndex $handle $structLinks

foreach name $structNames {
    foreach {anchor code intro members remarks} $structs($name) {break}
    WriteEntry $handle $name $anchor $intro $code \
        -members $members -remarks $remarks
}

WriteFooter $handle
close $handle

puts "- Finished"
return 0
