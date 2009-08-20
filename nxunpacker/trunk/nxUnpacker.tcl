#
# nxUnpacker - Unpack releases quickly.
#

set unpack(Clean)       1
set unpack(Rename)      1
set unpack(Source)      "."
set unpack(BinUnzip)    "UnZIP.exe"
set unpack(BinUnrar)    "UnRAR.exe"


#
# Common
#

proc ListMatchRanked {patternRankList value} {
    foreach {pattern rank} $patternRankList {
        if {[string match -nocase $pattern $value]} {return $rank}
    }
    return 0
}

proc GetGroupName {rlsName} {
    set pos [string last "-" $rlsName]
    if {$pos == -1} {
        return ""
    }
    return [string range $rlsName [incr pos] end]
}


#
# Locating files
#

proc GetFileFromList {dirList patternRankList} {
    set partialCount 0
    set partialResult ""


    foreach entry $dirList {
        set rank [ListMatchRanked $patternRankList [file tail $entry]]

        #
        # Rank result:
        # 0 - No match
        # 1 - Exact or reliable match
        # 2 - Partial match
        #
        if {$rank == 1} {
            return $entry

        } elseif {$rank == 2} {
            set partialResult $entry
            incr partialCount
        }
    }

    # If there was only one partial match, we can assume it's reliable.
    if {$partialCount == 1} {
        return $partialResult
    }
    return ""
}

proc GetKeygenFile {dirList} {
    set patternRank [list "keygen.exe" 1 "keymaker.exe" 1]
    return [GetFileFromList $dirList $patternRank]
}

proc GetNfoFile {dirList groupNfo} {
    set patternRank [list $groupNfo 1 "*.nfo" 2]
    return [GetFileFromList $dirList $patternRank]
}

proc GetPatchFile {dirList} {
    set patternRank [list "patch.exe" 1 "patcher.exe" 1]
    return [GetFileFromList $dirList $patternRank]
}

proc GetSetupFile {dirList} {
    set patternRank [list "*setup*.exe" 1 "*install*.exe" 1 "*.exe" 2]
    return [GetFileFromList $dirList $patternRank]
}


#
# Moving files
#

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

proc RenameTo {sourcePath destName} {
    set sourceName [file tail $sourcePath]

    # Check if a rename is required
    if {[string equal $sourceName $destName]} {
        return
    }

    puts "|  |  |- Renaming \"$sourceName\" to \"$destName\""

    # Check if a temp rename is required to work around Windows
    if {[string equal -nocase $sourceName $destName]} {
        if {[catch {file rename -force -- $sourcePath [append sourcePath _]} errorMsg]} {
            puts "|  |  |- $errorMsg"
        }
    }

    # Perform actual rename
    set destPath [file join [file dirname $sourcePath] $destName]
    if {[catch {file rename -force -- $sourcePath $destPath} errorMsg]} {
        puts "|  |  |- $errorMsg"
    }
}


#
# Unpacking archives
#

proc FileUnRarDest {sourceFile destPath} {
    global unpack
    ## e   = Unrar files.
    ## -o- = Do not overwrite files.
    exec -- $unpack(BinUnrar) e -o- $sourceFile $destPath
}

proc FileUnZipDest {sourceFile destPath} {
    global unpack
    ## -d = Destination directory.
    ## -n = Do not overwrite files.
    exec -- $unpack(BinUnzip) -n $sourceFile -d $destPath
}

puts ".- nxUnpacker v1.1.0"

puts -nonewline "|- Target Dir \[$unpack(Source)\]: "
flush stdout
gets stdin inputPath
set inputPath [string trim $inputPath]
if {$inputPath ne "" } {
    set unpack(Source) [file normalize $inputPath]
}

set dirList [glob -directory $unpack(Source) -types d -nocomplain "*"]
if {![llength $dirList]} {
    puts "|- No releases found."
    puts "`- Exiting."
    return 1
}

puts "|- Found [llength $dirList] release(s)."
puts "|"

foreach dirPath $dirList {
    set rlsName [file tail $dirPath]
    puts "|  .- Unpacking: $rlsName"

    if {![file isdirectory $dirPath]} {
        puts "| `- Unable to change CD to path: $errorMsg"
        continue
    }

    # Remove all .ioFTPD files
    if {$unpack(Clean)} {
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
    } else {
        puts "|  |- No RAR archives found."
    }

    if {[llength $zipFiles]} {
        puts "|  |  .- Unzipping files."
        foreach filePath $zipFiles {
            set fileName [file tail $filePath]
            puts "|  |  |- Unzipping file: $fileName"
            catch {FileUnZipDest $filePath $dirPath}
        }
        puts "|  |  `- Finished unzipping files."
    }

    if {$unpack(Rename)} {
        puts "|  |  .- Renaming files."
        set fileList [glob -directory $dirPath -types f -nocomplain "*"]

        # Get group name
        set groupFile [GetGroupName $rlsName]
        if {$groupFile ne ""} {
            append groupFile ".nfo"

            # Locate NFO file and rename
            set filePath [GetNfoFile $fileList $groupFile]
            if {$filePath ne ""} {
                RenameTo $filePath $groupFile
            }
        }

        # Locate keygen file and rename
        set filePath [GetKeygenFile $fileList]
        if {$filePath ne ""} {
            RenameTo $filePath "Keygen.exe"
        }

        # Locate patch file and rename
        set filePath [GetPatchFile $fileList]
        if {$filePath ne ""} {
            RenameTo $filePath "Patch.exe"
        }

        # Locate setup file and rename
        set filePath [GetSetupFile $fileList]
        if {$filePath ne ""} {
            RenameTo $filePath "Setup.exe"
        }

        puts "|  |  `- Finished renaming files."
    }

    puts "|  |- Removing temporary directory."
    catch {file delete -force -- $tempPath}

    puts "|  `- Finished release."
    puts "|"
}

puts "`- Finished."
return 0
