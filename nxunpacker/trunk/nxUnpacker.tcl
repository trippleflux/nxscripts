puts ".- nxUnpacker v1.0.0"

## Settings
set unpack(clean)       1
set unpack(source)      [pwd]
set unpack(binUnzip)    "UnZIP.exe"
set unpack(binUnrar)    "UnRAR.exe"

set unpack(source) {E:\Site\REQS\Templates}

puts "|- Listing: $unpack(source)"
set curPath [pwd]
set dirList [glob -directory $unpack(source) -types d -nocomplain "*"]

if {![llength $dirList]} {
    puts "|- No releases found."
    puts "`- Exiting."
    return 1
}

## Move
proc FileMoveDest {sourceFile destPath} {
    file rename -force -- $sourceFile $destPath
}

proc FileMoveKnown {sourceFile destPath} {
    set destPath [file join $destPath {[Unpack]}]
    FileMoveDest $sourceFile $destPath
}

proc FileMoveUnknown {sourceFile destPath} {
    set destPath [file join $destPath {[Unpack]} {[Unknown]}]
    if {![file isdirectory $destPath]} {
        file mkdir $destPath
    }
    FileMoveDest $sourceFile $destPath
}

## UnRAR/UnZIP
proc FileUnRarDest {sourceFile destPath} {
    global unpack
    ## e   = Unrar files.
    ## -o- = Do not overwrite files.
    exec -- $unpack(binUnrar) e -o- $sourceFile $destPath
}

proc FileUnZipDest {sourceFile destPath} {
    global unpack
    ## -d = Destination directory.
    ## -n = Do not overwrite files.
    exec -- $unpack(binUnzip) -n $sourceFile -d $destPath
}

puts "|- Found [llength $dirList] release(s)."
puts "|"

foreach dirPath $dirList {
    puts "|  .- Unpacking: [file tail $dirPath]"
    if {![file isdirectory $dirPath]} {
        puts "| `- Unable to change CD to path: $errorMsg"
        continue
    }

    if {$unpack(clean)} {
        puts "|  |- Cleaning ioFTPD files."

        foreach filePath [glob -directory $dirPath -types f -nocomplain ".ioFTPD*"] {
            catch {file delete -force -- $filePath}
        }
        foreach filePath [glob -directory $dirPath -types {f hidden} -nocomplain ".ioFTPD*"] {
            catch {file delete -force -- $filePath}
        }
    }

    set fileList [glob -directory $dirPath -types f -nocomplain "*"]
    set subDirList [glob -directory $dirPath -types d -nocomplain "*"]
    puts "|  |- Found [llength $fileList] file(s)."

    puts "|  |- Creating directories."
    set unpackPath [file join $dirPath {[Unpack]}]
    set tempPath [file join $dirPath {[Unpack]} {[Temp]}]

    file mkdir $unpackPath
    file mkdir $tempPath

    if {[llength $subDirList]} {
        puts "|  |  .- Moving sub-directories."
        foreach subDirPath $subDirList {
            set subDirName [file tail $subDirPath]
            if {[string equal {[Unpack]} $subDirName]} {continue}

            puts "|  |  |- Moving dir: $subDirName"
            if {[catch {FileMoveKnown $subDirPath $dirPath} errorMsg]} {
                puts "|  |  |- Error moving dir: $errorMsg"
            }
        }
        puts "|  |  `- Finished moving sub-directories."
    }

    puts "|  |  .- Unzipping files."
    foreach filePath $fileList {
        set fileName [file tail $filePath]
        set moveProc "FileMoveKnown"

        switch -glob -- [string tolower $fileName] {
            {*.diz} -
            {*.nfo} -
            {*.sfv} {
                puts "|  |  |- Skipping file: $fileName"
            }
            {*.zip} {
                puts "|  |  |- Unzipping file: $fileName"
                catch {FileUnZipDest $filePath $tempPath}
            }
            default {
                puts "|  |  |- Unknown file: $fileName"
                set moveProc "FileMoveUnknown"
            }
        }

        if {[catch {$moveProc $filePath $dirPath} errorMsg]} {
            puts "|  |  |- Error moving file: $errorMsg"
        }
    }
    puts "|  |  `- Finished unzipping files."

    puts "|  |  .- Moving temporary files."
    set rarFiles ""
    set zipFiles ""
    set fileList [glob -directory $tempPath -types f -nocomplain "*"]

    foreach filePath $fileList {
        set fileName [file tail $filePath]

        if {[string match -nocase {*.zip} $fileName]} {
            lappend zipFiles $filePath
            continue
        }
        if {[regexp -nocase -- {\.(rar|[r-z]\d{2}|\d{3})$} $fileName]} {
            lappend rarFiles $filePath
            continue
        }

        puts "|  |  |- Moving file: $fileName"
        if {[catch {FileMoveDest $filePath $dirPath} errorMsg]} {
            puts "|  |  |- Error moving file: $errorMsg"
        }
    }
    puts "|  |  `- Finished moving temporary files."

    if {[llength $rarFiles]} {
        puts "|  |  .- Unraring files."
        foreach filePath $rarFiles {
            set fileName [file tail $filePath]
            puts "|  |  |- Unraring file: $fileName"
            catch {FileUnRarDest $filePath $dirPath}
        }
        puts "|  |  `- Finished unraring files."

    } elseif {[llength $zipFiles]} {
        puts "|  |- No RAR archives found."

        puts "|  |  .- Moving zip files."
        foreach filePath $rarFiles {
            set fileName [file tail $filePath]
            puts "|  |  |- Moving zip: $fileName"
            if {[catch {FileMoveDest $filePath $dirPath} errorMsg]} {
                puts "|  |  |- Error moving zip: $errorMsg"
            }
        }
        puts "|  |  `- Finished moving zip files."
    }

    puts "|  |- Removing temporary directory."
    catch {file delete -force -- $tempPath}

    puts "|  `- Finished release."
    puts "|"
}

puts "`- Finished."
return 0
