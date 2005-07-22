#
# AlcoExt - Alcoholicz Tcl extension.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# File Name:
#   makefile
#
# Author:
#   neoxed (neoxed@gmail.com) June 12, 2005
#
# Abstract:
#   Build configuration file for NMAKE, or a compatible tool.
#
#   Usage:
#     nmake -f makefile [OPTION=1 ...]
#
#   Options:
#     DEBUG    - Toggle symbolic debugging.
#     DEVPERF  - Toggle Compuware DevPartner performance analysis.
#     INSTALL  - Target installation path.
#     MEMDEBUG - Toggle Tcl memory debugging.
#     THREADS  - Toggle threading support.
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

ROOT        = .
TCL_SUFFIX  = tg
PKG_SUFFIX  = DMTP

PKG_VERSION_DOT = $(PKG_MAJOR).$(PKG_MINOR).$(PKG_PATCH)
PKG_VERSION_RES = $(PKG_MAJOR),$(PKG_MINOR),$(PKG_PATCH),$(PKG_STATUS)

PKG_CFLAGS  = "-DPACKAGE_NAME=\"$(PKG_NAME)\"" "-DPACKAGE_DESC=\"$(PKG_DESC)\"" "-DPACKAGE_VERSION=\"$(PKG_VERSION_DOT)\"" -DPACKAGE_VERSION_RES=$(PKG_VERSION_RES)
PKG_LFLAGS  = -nodefaultlib:msvcrt -opt:ref -opt:nowin98
LTC_CFLAGS  =
TCL_CFLAGS  = -DUSE_TCL_STUBS
ZLIB_CFLAGS = -DASMV -DASMINF

# -------------------------------------------------------------------------

!if "$(DEBUG)" == "1"
!message *** Option: Symbolic Debugging
PKG_OPTIONS = Debug
PKG_TOPDIR  = Debug
# Make sure NODEBUG is not defined for debug builds.
!ifdef NODEBUG
!undef NODEBUG
!endif
!else
NODEBUG     = 1
PKG_OPTIONS = Release
PKG_TOPDIR  = Release
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
INSTALL     = $(ROOT)
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

!if "$(DEVPERF)" == "1"
!message *** Option: DevPartner Performance Analysis
CC          = NMCL /NMttOn
LINK        = NMLINK /NMttOn
PKG_OPTIONS = $(PKG_OPTIONS), Perf
!else
PKG_SUFFIX  = $(PKG_SUFFIX:P=)
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

!if [nmakehlp -v $(TCL_H) ""] == 0
!include version.vc
!else
TCL_DOTVERSION  = 8.4
TCL_VERSION = $(TCL_DOTVERSION:.=)
!endif

TCL_SH       = "$(TCL_DIR)\bin\tclsh$(TCL_VERSION)$(TCL_SUFFIX).exe"
TCL_STUBLIB  = "$(TCL_DIR)\lib\tclstub$(TCL_VERSION).lib"
TCL_IMPLIB   = "$(TCL_DIR)\lib\tcl$(TCL_VERSION)$(TCL_SUFFIX).lib"
TCL_LIBRARY  = $(TCL_DIR)\lib
TCL_INCLUDES = $(TCL_DIR)\include

!message *** Tcl Version: $(TCL_DOTVERSION)
!message *** Tcl Stub: $(TCL_STUBLIB)

# -------------------------------------------------------------------------

!if "$(PKG_SUFFIX)" != ""
PKG_SUFFIX  = -$(PKG_SUFFIX)
!endif

# File and directory paths.
PKG_FILE    = $(PKG_NAME)$(PKG_SUFFIX).dll
PKG_OUTFILE = $(PKG_TOPDIR)\$(PKG_FILE)
PKG_INTDIR  = $(PKG_TOPDIR)\$(PKG_OPTIONS:, =)
PKG_INSTALL_FILE = $(PKG_NAME)$(PKG_MAJOR)$(PKG_MINOR).dll
PKG_INSTALL_DIR  = $(INSTALL)\$(PKG_NAME)$(PKG_MAJOR).$(PKG_MINOR)

# Append file and option definitions to cflags.
PKG_CFLAGS  = $(PKG_CFLAGS) "-DPACKAGE_FILE=\"$(PKG_FILE)\"" "-DPACKAGE_OPTIONS=\"$(PKG_OPTIONS)\""

INCLUDE_DIRS=\
    -I"$(TCL_INCLUDES)"\
    -I"$(ROOT)\source"\
    -I"$(ROOT)\zlib"

INTERMEDIATE_FILES=\
    $(PKG_INTDIR)\adler32.obj\
    $(PKG_INTDIR)\compress.obj\
    $(PKG_INTDIR)\crc32.obj\
    $(PKG_INTDIR)\deflate.obj\
    $(PKG_INTDIR)\gvmat32.obj\
    $(PKG_INTDIR)\gvmat32c.obj\
    $(PKG_INTDIR)\gzio.obj\
    $(PKG_INTDIR)\infback.obj\
    $(PKG_INTDIR)\inffas32.obj\
    $(PKG_INTDIR)\inffast.obj\
    $(PKG_INTDIR)\inflate.obj\
    $(PKG_INTDIR)\inftrees.obj\
    $(PKG_INTDIR)\trees.obj\
    $(PKG_INTDIR)\uncompr.obj\
    $(PKG_INTDIR)\zutil.obj\
    $(PKG_INTDIR)\nxHelper.obj\
    $(PKG_INTDIR)\nxBase64.obj\
    $(PKG_INTDIR)\nxMP3.obj\
    $(PKG_INTDIR)\nxMP3Info.obj\
    $(PKG_INTDIR)\nxTime.obj\
    $(PKG_INTDIR)\nxTouch.obj\
    $(PKG_INTDIR)\nxUtil.obj\
    $(PKG_INTDIR)\nxVolume.obj\
    $(PKG_INTDIR)\nxZlib.obj\
    $(PKG_INTDIR)\nxHelper.res

OBJECT_FILES=\
    $(ROOT)\zlib\adler32.obj\
    $(ROOT)\zlib\compress.obj\
    $(ROOT)\zlib\crc32.obj\
    $(ROOT)\zlib\deflate.obj\
    $(ROOT)\zlib\gvmat32.obj\
    $(ROOT)\zlib\gvmat32c.obj\
    $(ROOT)\zlib\gzio.obj\
    $(ROOT)\zlib\infback.obj\
    $(ROOT)\zlib\inffas32.obj\
    $(ROOT)\zlib\inffast.obj\
    $(ROOT)\zlib\inflate.obj\
    $(ROOT)\zlib\inftrees.obj\
    $(ROOT)\zlib\trees.obj\
    $(ROOT)\zlib\uncompr.obj\
    $(ROOT)\zlib\zutil.obj\
    $(ROOT)\source\nxHelper.obj\
    $(ROOT)\source\nxBase64.obj\
    $(ROOT)\source\nxMP3.obj\
    $(ROOT)\source\nxMP3Info.obj\
    $(ROOT)\source\nxTime.obj\
    $(ROOT)\source\nxTouch.obj\
    $(ROOT)\source\nxUtil.obj\
    $(ROOT)\source\nxVolume.obj\
    $(ROOT)\source\nxZlib.obj\
    $(ROOT)\source\nxHelper.res

# -------------------------------------------------------------------------

AFLAGS  = -nologo -coff
CFLAGS  = $(cdebug) $(cflags) $(cvarsmt) $(PKG_CFLAGS) $(LTC_CFLAGS) $(TCL_CFLAGS) $(ZLIB_CFLAGS) $(INCLUDE_DIRS)
LFLAGS  = $(ldebug) $(dlllflags) $(conlibsmt) $(PKG_LFLAGS) $(TCL_STUBLIB)
RFLAGS  = $(rcflags) $(rcvars) $(PKG_CFLAGS)

# -------------------------------------------------------------------------

.asm.obj::
    $(AS) -c $(AFLAGS) -Fo"$(PKG_INTDIR)\\" @<<
$<
<<

.c.obj::
    $(CC) -c $(CFLAGS) -Fo"$(PKG_INTDIR)\\" @<<
$<
<<

.rc.res:
    $(RC) $(RFLAGS) -Fo"$(PKG_INTDIR)\$(<B).res" $<

.SUFFIXES:
.SUFFIXES:.asm .c .rc

# -------------------------------------------------------------------------

default: all
all:     library

clean:
    @IF EXIST "$(PKG_FILE)" DEL /F "$(PKG_FILE)"
    @IF EXIST "$(PKG_OUTFILE)" DEL /F "$(PKG_OUTFILE)"
    @IF EXIST "$(PKG_INTDIR)" RMDIR /Q /S "$(PKG_INTDIR)"
    @IF EXIST version.vc DEL /F version.vc

distclean: clean
    @DEL "$(PKG_NAME)*.dll" *.obj *.pdb
    @IF EXIST Debug RMDIR /Q /S Debug
    @IF EXIST Release RMDIR /Q /S Release
    @IF EXIST nmakehlp.exe DEL /F nmakehlp.exe
    @IF EXIST nmakehlp.obj DEL /F nmakehlp.obj

install: library package-index
	@ECHO Installing to: $(PKG_INSTALL_DIR)
    @IF NOT EXIST "$(PKG_INSTALL_DIR)" MKDIR "$(PKG_INSTALL_DIR)"
    @COPY /Y "$(PKG_OUTFILE)" "$(PKG_INSTALL_DIR)\$(PKG_INSTALL_FILE)" >NUL
    @COPY /Y "$(PKG_INTDIR)\pkgIndex.tcl" "$(PKG_INSTALL_DIR)" >NUL

library: setup $(OBJECT_FILES)
    $(LINK) $(LFLAGS) "-out:$(PKG_OUTFILE)" $(INTERMEDIATE_FILES)

package-index: setup
    @TYPE << >"$(PKG_INTDIR)\pkgIndex.tcl"
# Tcl package index file.
if {![package vsatisfies [package provide Tcl] 8.4]} {return}
package ifneeded $(PKG_NAME) $(PKG_VERSION_DOT) [list load [file join $$dir $(PKG_INSTALL_FILE)]]
<<

setup:
    @IF NOT EXIST "$(PKG_TOPDIR)" MKDIR "$(PKG_TOPDIR)"
    @IF NOT EXIST "$(PKG_INTDIR)" MKDIR "$(PKG_INTDIR)"
