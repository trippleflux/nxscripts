#
# nxHelper - Tcl extension for nxTools.
# Copyright (c) 2005 neoxed
#
# File Name:
#   nxHelper.tcl
#
# Author:
#   neoxed (neoxed@gmail.com) May 22, 2005
#
# Abstract:
#   Test exercises for nxHelper functions.
#

load nxHelper.dll

# Base64 and Zlib
set testStrings {
    {adam}
    {adam rulez with nxhelper}
    {\nad]akpda0-dajk3wr5aqq54ra}
}

foreach test $testStrings {
    set encoded [::nx::base64 encode $test]
    set decoded [::nx::base64 decode $encoded]
    if {$test ne $decoded} {
        puts "base64: encode/decode failed for \"$test\""
    }
}

foreach test $testStrings {
    set deflated [::nx::zlib deflate $test]
    set inflated [::nx::zlib inflate $deflated]
    if {$test ne $inflated} {
        error "zlib: deflate/inflate failed for \"$test\""
    }
}

foreach test $testStrings {
    ::nx::zlib adler32 $test
    ::nx::zlib crc32 $test
}

::nx::volume type "C:\\"
::nx::volume info "C:\\" volC
::nx::volume info "D:\\" volD

::nx::mp3 test.mp3 mp3A
::nx::mp3 test.mp3 mp3B

unset mp3A mp3B volC volD
