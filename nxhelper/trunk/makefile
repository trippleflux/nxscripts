#
# nxHelper - Tcl extension for nxTools.
# Copyright (c) 2004-2008 neoxed
#
# File Name:
#   makefile
#
# Author:
#   neoxed (neoxed@gmail.com) Jun 12, 2005
#
# Abstract:
#   Build configuration file for NMAKE, or a compatible tool.
#
#   Usage:
#     nmake -f makefile [OPTION=1 ...]
#
#   Options:
#     DEBUG    - Toggle symbolic debugging.
#     INSTALL  - Target installation path.
#     MEMDEBUG - Toggle Tcl memory debugging.
#     THREADS  - Toggle threading support.
#
#   Profiling:
#     COVERAGE - Toggle Compuware DevPartner code coverage analysis.
#     PERF     - Toggle Compuware DevPartner performance analysis.
#

# PSDK Settings
APPVER      = 4.0
TARGETOS    = WINNT

# -------------------------------------------------------------------------

!if !exist("makefile")
!error You must run this makefile from the directory it is located in.
!endif

!include "package.def"

# -------------------------------------------------------------------------

# Directory paths.
TOP             = .
SOURCE_DIR      = $(TOP)\source
ZLIB_DIR        = $(TOP)\zlib

# Versions strings.
PKG_VERSION_DOT = $(PKG_MAJOR).$(PKG_MINOR).$(PKG_PATCH)
PKG_VERSION_RES = $(PKG_MAJOR),$(PKG_MINOR),$(PKG_PATCH),$(PKG_STATUS)

# Compiler and linker flags.
PKG_CFLAGS      = -D "PACKAGE_NAME=\"$(PKG_NAME)\"" -D "PACKAGE_DESC=\"$(PKG_DESC)\"" -D "PACKAGE_VERSION=\"$(PKG_VERSION_DOT)\"" -D "PACKAGE_VERSION_RES=$(PKG_VERSION_RES)"
PKG_LFLAGS      = /NODEFAULTLIB:msvcrt /OPT:NOWIN98
ZLIB_CFLAGS     =
TCL_CFLAGS      = -DUSE_TCL_STUBS

# File suffixes.
TCL_SUFFIX      = tg
PKG_SUFFIX      = DMTCP

# -------------------------------------------------------------------------

!if "$(DEBUG)" == "1"
!message *** Option: Symbolic Debugging
PKG_OPTIONS = Debug
PKG_OUTDIR  = Debug
# Make sure NODEBUG is not defined for debug builds.
!ifdef NODEBUG
!undef NODEBUG
!endif
!else
NODEBUG     = 1
PKG_OPTIONS = Release
PKG_OUTDIR  = Release
PKG_SUFFIX  = $(PKG_SUFFIX:D=)
TCL_SUFFIX  = $(TCL_SUFFIX:g=)
!endif

!if "$(MEMDEBUG)" == "1"
!message *** Option: Memory Debugging
PKG_OPTIONS = $(PKG_OPTIONS), Memory
TCL_CFLAGS  = $(TCL_CFLAGS) -DTCL_MEM_DEBUG=1
!else
PKG_SUFFIX  = $(PKG_SUFFIX:M=)
!endif

!ifndef INSTALL
INSTALL     = $(TOP)
!elseif !exist("$(INSTALL)")
!error The given installation path does not exist.
!endif

!if "$(THREADS)" == "1"
!message *** Option: Threads
PKG_OPTIONS = $(PKG_OPTIONS), Threads
TCL_CFLAGS  = $(TCL_CFLAGS) -DTCL_THREADS=1
!else
PKG_SUFFIX  = $(PKG_SUFFIX:T=)
TCL_SUFFIX  = $(TCL_SUFFIX:t=)
!endif

!include "win32.mak"

!if "$(COVERAGE)" == "1"
!message *** Option: DevPartner Code Coverage Analysis
CC          = NMCL /NMtcOn
LINK        = NMLINK /NMtcOn
PATH        = $(HOMEDRIVE)\Program Files\Common Files\Compuware\NMShared;$(PATH)
PKG_OPTIONS = $(PKG_OPTIONS), Coverage
!else
PKG_SUFFIX  = $(PKG_SUFFIX:C=)
!endif

!if "$(PERF)" == "1"
!message *** Option: DevPartner Performance Analysis
CC          = NMCL /NMttOn
LINK        = NMLINK /NMttOn
PATH        = $(HOMEDRIVE)\Program Files\Common Files\Compuware\NMShared;$(PATH)
PKG_OPTIONS = $(PKG_OPTIONS), Perf
!else
PKG_SUFFIX  = $(PKG_SUFFIX:P=)
!endif

!if "$(COVERAGE)" == "1" && "$(PERF)" == "1"
!error You cannot profile both code coverage and performance.
!endif

# -------------------------------------------------------------------------

!if !exist(nmakehlp.exe)
!if [$(CC) -nologo -MT nmakehlp.c -link -subsystem:console > nul]
!endif
!endif

!if !defined(TCL_DIR)
!error Unable to find tcl.h; the TCL_DIR variable is incorrect.
!endif

TCL_DIR = $(TCL_DIR:/=\)
!if exist("$(TCL_DIR)\include\tcl.h")
TCL_H   = $(TCL_DIR)\include\tcl.h
!else
!error Unable to find tcl.h; the TCL_DIR variable is incorrect.
!endif

!if [nmakehlp -g $(TCL_H) TCL_VERSION] == 0
!error Unable to retrieve version from "$(TCL_H)".
!elseif [nmakehlp -g $(TCL_H) TCL_VERSION] == 83
TCL_DOTVERSION	= 8.3
!elseif [nmakehlp -g $(TCL_H) TCL_VERSION] == 84
TCL_DOTVERSION	= 8.4
!elseif [nmakehlp -g $(TCL_H) TCL_VERSION] == 85
TCL_DOTVERSION	= 8.5
!elseif [nmakehlp -g $(TCL_H) TCL_VERSION] == 86
TCL_DOTVERSION	= 8.6
!elseif [nmakehlp -g $(TCL_H) TCL_VERSION] == 90
TCL_DOTVERSION	= 9.0
!else
!error Unknown version of Tcl, check "$(TCL_H)".
!endif

TCL_VERSION = $(TCL_DOTVERSION:.=)
TCL_LIB     = "$(TCL_DIR)\bin\tcl$(TCL_VERSION)$(TCL_SUFFIX).dll"
TCL_SH      = "$(TCL_DIR)\bin\tclsh$(TCL_VERSION)$(TCL_SUFFIX).exe"
TCL_STUBLIB = "$(TCL_DIR)\lib\tclstub$(TCL_VERSION).lib"
TCL_IMPLIB  = "$(TCL_DIR)\lib\tcl$(TCL_VERSION)$(TCL_SUFFIX).lib"
TCL_LIBRARY = $(TCL_DIR)\lib
TCL_INCLUDES= $(TCL_DIR)\include

!message *** Tcl Version: $(TCL_DOTVERSION)
!message *** Tcl Stub: $(TCL_STUBLIB)

# -------------------------------------------------------------------------

!if "$(PKG_SUFFIX)" != ""
PKG_SUFFIX  = -$(PKG_SUFFIX)
!endif

# File and directory paths.
PKG_FILE    = $(PKG_NAME)$(PKG_SUFFIX).dll
PKG_OUTFILE = $(PKG_OUTDIR)\$(PKG_FILE)
PKG_INTDIR  = $(PKG_OUTDIR)\$(PKG_OPTIONS:, =)
PKG_INSTALL_FILE = $(PKG_NAME)$(PKG_MAJOR)$(PKG_MINOR).dll
PKG_INSTALL_DIR  = $(INSTALL)\$(PKG_NAME)$(PKG_MAJOR).$(PKG_MINOR)

# Append file and option definitions to cflags.
PKG_CFLAGS  = $(PKG_CFLAGS) "-DPACKAGE_FILE=\"$(PKG_FILE)\"" "-DPACKAGE_OPTIONS=\"$(PKG_OPTIONS)\""

# -------------------------------------------------------------------------

INCLUDE_DIRS=\
    -I"$(TCL_INCLUDES)"\
    -I"$(SOURCE_DIR)"\
    -I"$(ZLIB_DIR)"

OBJECT_FILES=\
    $(PKG_INTDIR)\adler32.obj\
    $(PKG_INTDIR)\crc32.obj\
    $(PKG_INTDIR)\deflate.obj\
    $(PKG_INTDIR)\inffast.obj\
    $(PKG_INTDIR)\inflate.obj\
    $(PKG_INTDIR)\inftrees.obj\
    $(PKG_INTDIR)\trees.obj\
    $(PKG_INTDIR)\zutil.obj\
    $(PKG_INTDIR)\nxBase64.obj\
    $(PKG_INTDIR)\nxHelper.obj\
    $(PKG_INTDIR)\nxKey.obj\
    $(PKG_INTDIR)\nxMP3.obj\
    $(PKG_INTDIR)\nxMP3Info.obj\
    $(PKG_INTDIR)\nxTime.obj\
    $(PKG_INTDIR)\nxTouch.obj\
    $(PKG_INTDIR)\nxUtil.obj\
    $(PKG_INTDIR)\nxVolume.obj\
    $(PKG_INTDIR)\nxZlib.obj\
    $(PKG_INTDIR)\nxHelper.res

CFLAGS  = -c $(cdebug) $(cflags) $(cvarsmt) $(PKG_CFLAGS) $(TCL_CFLAGS) $(INCLUDE_DIRS)
LFLAGS  = $(ldebug) $(dlllflags) $(conlibsmt) $(PKG_LFLAGS) shlwapi.lib $(TCL_STUBLIB)
RFLAGS  = $(rcflags) $(rcvars) $(PKG_CFLAGS)

# -------------------------------------------------------------------------

{$(ZLIB_DIR)}.c{$(PKG_INTDIR)}.obj::
    $(CC) $(CFLAGS) $(ZLIB_CFLAGS) -Fo$(PKG_INTDIR)\ @<<
$<
<<

{$(SOURCE_DIR)}.c{$(PKG_INTDIR)}.obj::
    $(CC) $(CFLAGS) -Fo$(PKG_INTDIR)\ @<<
$<
<<

{$(SOURCE_DIR)}.rc{$(PKG_INTDIR)}.res:
	$(RC) $(RFLAGS) -fo $@ $<

.SUFFIXES:
.SUFFIXES:.c .rc

# -------------------------------------------------------------------------

default: all
all:     library

clean:
    @IF EXIST "$(PKG_FILE)" DEL /F "$(PKG_FILE)"
    @IF EXIST "$(PKG_OUTFILE)" DEL /F "$(PKG_OUTFILE)"
    @IF EXIST "$(PKG_INTDIR)" RMDIR /Q /S "$(PKG_INTDIR)"

distclean: clean
    @DEL "$(PKG_NAME)*.dll" *.exp *.obj *.pdb
    @IF EXIST Debug RMDIR /Q /S Debug
    @IF EXIST Release RMDIR /Q /S Release
    @IF EXIST nmakehlp.exe DEL /F nmakehlp.exe
    @IF EXIST nmakehlp.obj DEL /F nmakehlp.obj

install: library package-index
	@ECHO Installing to: $(PKG_INSTALL_DIR)
    @IF NOT EXIST "$(PKG_INSTALL_DIR)" MKDIR "$(PKG_INSTALL_DIR)"
    @COPY /Y "$(PKG_OUTFILE)" "$(PKG_INSTALL_DIR)\$(PKG_INSTALL_FILE)" >NUL
    @COPY /Y "$(PKG_INTDIR)\pkgIndex.tcl" "$(PKG_INSTALL_DIR)" >NUL

$(PKG_OUTFILE): $(OBJECT_FILES)
    $(LINK) $(LFLAGS) -out:$@ @<<
$**
<<

library: setup $(PKG_OUTFILE)

package-index: setup
    @TYPE << >"$(PKG_INTDIR)\pkgIndex.tcl"
#
# Tcl package index file.
#
if {![package vsatisfies [package provide Tcl] 8.4]} {return}
package ifneeded $(PKG_NAME) $(PKG_VERSION_DOT) [list load [file join $$dir $(PKG_INSTALL_FILE)]]
<<

setup:
    @IF NOT EXIST "$(PKG_OUTDIR)" MKDIR "$(PKG_OUTDIR)"
    @IF NOT EXIST "$(PKG_INTDIR)" MKDIR "$(PKG_INTDIR)"
