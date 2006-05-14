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
VERSION     = 1.0.0

## Compiler options
AR          = lib
CC          = cl
LD          = link
RC          = rc
AFLAGS      = /nologo
CFLAGS      = /nologo /I "$(IOFTPD_INC)" -I ".\src" /W4
LFLAGS      = /nologo
RFLAGS      = /r

# -------------------------------------------------------------------------

!if $(MSVCRT)
!message
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
!else
!message Debug Symbols: Off
CFLAGS      = $(CFLAGS) /Ox /GF $(CRTLIB) /DNDEBUG
LFLAGS      = $(LFLAGS) /RELEASE /OPT:REF
!endif

VERSION     = $(VERSION).0
VERSION_RES = $(VERSION:.=,)
RFLAGS      = $(RFLAGS) "/DVERSION=\"$(VERSION)\"" "/DVERSION_RES=$(VERSION_RES)"

# -------------------------------------------------------------------------

default: all

all: dynamic static tools

clean:
    @DEL /F *.dll *.ilk *.lib *.exp *.exe *.obj *.pdb *.res

release: all
    @MKDIR bins
    @MOVE /Y *.dll bins > nul
    @MOVE /Y *.exe bins > nul
    @MOVE /Y *.lib bins > nul
    @DEL /F *.ilk *.exp *.obj *.res

dynamic: nxsdk.dll

static: nxsdk-static.lib

tools: kick.exe resolve.exe who.exe

# -------------------------------------------------------------------------

LIB_OBJS = group.obj online.obj shmem.obj user.obj util.obj vfs.obj

{src\lib\}.c{}.obj::
    $(CC) /c $(CFLAGS) /Fo $<

{src\lib\}.rc{}.res:
	$(RC) $(RFLAGS) /fo $@ $<

nxsdk.dll: $(LIB_OBJS) lib.res
    $(LD) $(LFLAGS) /DEF:src\lib\lib.def /DLL /OUT:$@ psapi.lib user32.lib @<<
$**
<<

nxsdk.lib: nxsdk.dll

nxsdk-static.lib: $(LIB_OBJS)
    $(AR) $(AFLAGS) /OUT:$@ @<<
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
