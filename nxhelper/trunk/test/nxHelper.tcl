#
# nxHelper - Tcl extension for nxTools.
# Copyright (c) 2004-2008 neoxed
#
# File Name:
#   nxHelper.tcl
#
# Author:
#   neoxed (neoxed@gmail.com) May 22, 2005
#
# Abstract:
#   Basic regression tests for nxHelper.
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

foreach type {gzip zlib zlib-raw} {
    foreach test $testStrings {
        set compressed [::nx::zlib compress $type $test]
        set decompressed [::nx::zlib decompress $type $compressed]
        if {$test ne $decompressed} {
            error "zlib: compress/decompress on $type failed for \"$test\""
        }
    }
}

foreach test $testStrings {
    ::nx::zlib adler32 $test
    ::nx::zlib crc32 $test
}

# Volume
::nx::volume type "C:\\"
::nx::volume info "C:\\" volC
::nx::volume info "D:\\" volD

unset volC volD

# MP3
::nx::mp3 test.mp3 mp3
unset mp3

# Key
::nx::key set world {kind of big}
::nx::key set universe {really big}

if {![::nx::key exists world] || ![::nx::key exists universe]} {
    error "these keys exist"
}
if {[::nx::key exists fake]} {
    error "this key does not exist"
}

if {[::nx::key get world] ne [list kind of big]} {
    error "value is wrong"
}

::nx::key unset -nocomplain universe
if {[::nx::key exists universe]} {
    error "this key was unset"
}

if {[::nx::key list] ne [list world]} {
    error "list is wrong"
}

::nx::key set world {the biggest now}
if {[::nx::key get world] ne [list the biggest now]} {
    error "new value is wrong"
}

::nx::key unset -nocomplain invalid
::nx::key unset world

::nx::key set leak1 {blah blah}
::nx::key set leak2 {blah blah}
