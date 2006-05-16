#
# nxSDK - ioFTPD Software Development Kit
# Copyright (c) 2006 neoxed
#
# Module Name:
#   Makefile
#
# Author:
#   neoxed (neoxed@gmail.com) May 13, 2006
#
# Abstract:
#   Project makefile for Microsoft's NMAKE.
#

## Build options
DEBUG       = 0
MEMDEBUG    = 0
MSVCRT      = 0
IOFTPD_INC  = D:\ioFTPD\source\Include

## Release version.
VERSION     = 0.3.0

## Compiler options
AR          = lib
CC          = cl
LD          = link
RC          = rc
AFLAGS      = /nologo
CFLAGS      = /nologo /I "$(IOFTPD_INC)" -I ".\include" /W4
LFLAGS      = /nologo
RFLAGS      = /r

# -------------------------------------------------------------------------

!if !exist("makefile")
!error You must run this makefile from the directory it is located in.
!endif

!if !exist("$(IOFTPD_INC)\ioFTPD.h")
!error Unable to find ioFTPD.h; the IOFTPD_INC variable is incorrect.
!endif

# -------------------------------------------------------------------------

!if $(MSVCRT)
!message CRT Library: msvcrt (dynamic)
CRTLIB      = /MD
!else
!message CRT Library: libcmt (static)
CRTLIB      = /MT
!endif

!if $(DEBUG)
!message Debug Symbols: On
CFLAGS      = $(CFLAGS) /Zi $(CRTLIB)d /DDEBUG
LFLAGS      = $(LFLAGS) /DEBUG
LIBENTRY    =
!else
!message Debug Symbols: Off
CFLAGS      = $(CFLAGS) /Ox /GF $(CRTLIB) /DNDEBUG
LFLAGS      = $(LFLAGS) /RELEASE /OPT:REF
LIBENTRY    = /ENTRY:DllMain
!endif

VERSION     = $(VERSION).0
VERSION_RES = $(VERSION:.=,)
RFLAGS      = $(RFLAGS) "/DVERSION=\"$(VERSION)\"" "/DVERSION_RES=$(VERSION_RES)"

# -------------------------------------------------------------------------

default: all

all: dynamic static tools

clean:
    @ECHO Deleting object files...
    -@DEL /F *.cod *.dll *.ilk *.lib *.exp *.exe *.obj *.pdb *.res 2> nul
    @ECHO Deleting binary files...
    -@DEL /F bin\*.dll 2> nul
    -@DEL /F bin\*.exe 2> nul
    -@DEL /F lib\*.lib 2> nul

release: all
    ECHO Moving binary files...
    @IF NOT EXIST bin MKDIR bin
    @MOVE /Y *.dll bin > nul
    @MOVE /Y *.exe bin > nul
    @IF NOT EXIST lib MKDIR lib
    @MOVE /Y *.lib lib > nul
    @ECHO Deleting object files...
    -@DEL /F *.cod *.ilk *.exp *.obj *.res 2> nul
    @ECHO Generating documentation...
    @CD doc
    @TCLSH gendocs.tcl > nul
    @CD ..

dynamic: nxsdk.dll

static: nxsdk-static.lib

tools: credits.exe kick.exe resolve.exe who.exe

# -------------------------------------------------------------------------

DLL_OBJS = dllentry.obj lib.res
LIB_OBJS = group.obj online.obj shmem.obj user.obj util.obj vfs.obj

{src\lib\}.c{}.obj::
    $(CC) /c $(CFLAGS) /Fo $<

{src\lib\}.rc{}.res:
	$(RC) $(RFLAGS) /fo $@ $<

nxsdk.dll: $(DLL_OBJS) $(LIB_OBJS)
    $(LD) $(LFLAGS) /DEF:src\lib\lib.def /DLL $(LIBENTRY) /OUT:$@ kernel32.lib psapi.lib user32.lib @<<
$**
<<

nxsdk.lib: nxsdk.dll

nxsdk-static.lib: $(LIB_OBJS)
    $(AR) $(AFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

CREDITS_OBJS = credits.obj credits.res

{src\credits\}.c{}.obj::
    $(CC) /c $(CFLAGS) /Fo $<

{src\credits\}.rc{}.res:
	$(RC) $(RFLAGS) /fo $@ $<

credits.exe: nxsdk.lib $(CREDITS_OBJS)
    $(LD) $(LFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

KICK_OBJS = kick.obj kick.res

{src\kick\}.c{}.obj::
    $(CC) /c $(CFLAGS) /Fo $<

{src\kick\}.rc{}.res:
	$(RC) $(RFLAGS) /fo $@ $<

kick.exe: nxsdk.lib $(KICK_OBJS)
    $(LD) $(LFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

RESOLVE_OBJS = resolve.obj resolve.res

{src\resolve\}.c{}.obj::
    $(CC) /c $(CFLAGS) /Fo $<

{src\resolve\}.rc{}.res:
	$(RC) $(RFLAGS) /fo $@ $<

resolve.exe: nxsdk.lib $(RESOLVE_OBJS)
    $(LD) $(LFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

WHO_OBJS = who.obj who.res

{src\who\}.c{}.obj::
    $(CC) /c $(CFLAGS) /Fo $<

{src\who\}.rc{}.res:
	$(RC) $(RFLAGS) /fo $@ $<

who.exe: nxsdk.lib $(WHO_OBJS)
    $(LD) $(LFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------
