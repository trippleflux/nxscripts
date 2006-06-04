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
MSVCRT      = 0
IOFTPD_INC  = E:\ioFTPD\source\Include

## Release version.
VERSION     = 0.4.0

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

all: lib mod tools

clean:
    @ECHO Deleting object files...
    -@DEL /F /S *.cod *.dll *.ilk *.lib *.exp *.exe *.obj *.pdb *.res 1> nul 2> nul
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

lib: nxsdk.dll nxsdk-static.lib

mod: nxmod.dll

tools: credits.exe info.exe kick.exe resolve.exe who.exe

# -------------------------------------------------------------------------

LIB_PATH = src\lib
DLL_OBJS = $(LIB_PATH)\dllentry.obj $(LIB_PATH)\lib.res
LIB_OBJS = $(LIB_PATH)\group.obj $(LIB_PATH)\online.obj $(LIB_PATH)\shmem.obj \
           $(LIB_PATH)\user.obj $(LIB_PATH)\util.obj $(LIB_PATH)\vfs.obj

{$(LIB_PATH)}.c{$(LIB_PATH)}.obj::
    $(CC) /c $(CFLAGS) /Fo$(LIB_PATH)\ $<

{$(LIB_PATH)}.rc{$(LIB_PATH)}.res:
	$(RC) $(RFLAGS) -fo $@ $<

nxsdk.dll: $(DLL_OBJS) $(LIB_OBJS)
    $(LD) $(LFLAGS) /DEF:$(LIB_PATH)\lib.def /DLL $(LIBENTRY) /OUT:$@ kernel32.lib psapi.lib user32.lib @<<
$**
<<

nxsdk.lib: nxsdk.dll

nxsdk-static.lib: $(LIB_OBJS)
    $(AR) $(AFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

MOD_PATH = src\mod
MOD_OBJS = $(MOD_PATH)\dllentry.obj $(MOD_PATH)\group.obj $(MOD_PATH)\proctable.obj \
           $(MOD_PATH)\user.obj $(MOD_PATH)\utils.obj $(MOD_PATH)\mod.res

{$(MOD_PATH)}.c{$(MOD_PATH)}.obj::
    $(CC) /c $(CFLAGS) /Fo$(MOD_PATH)\ $<

{$(MOD_PATH)}.rc{$(MOD_PATH)}.res:
	$(RC) $(RFLAGS) -fo $@ $<

nxmod.dll: $(MOD_OBJS)
    $(LD) $(LFLAGS) /DEF:$(MOD_PATH)\mod.def /DLL /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

CREDITS_PATH = src\credits
CREDITS_OBJS = $(CREDITS_PATH)\credits.obj $(CREDITS_PATH)\credits.res

{$(CREDITS_PATH)}.c{$(CREDITS_PATH)}.obj::
    $(CC) /c $(CFLAGS) /Fo$(CREDITS_PATH)\ $<

{$(CREDITS_PATH)}.rc{$(CREDITS_PATH)}.res:
	$(RC) $(RFLAGS) -fo $@ $<

credits.exe: nxsdk.lib $(CREDITS_OBJS)
    $(LD) $(LFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

INFO_PATH = src\info
INFO_OBJS = $(INFO_PATH)\info.obj $(INFO_PATH)\info.res

{$(INFO_PATH)}.c{$(INFO_PATH)}.obj::
    $(CC) /c $(CFLAGS) /Fo$(INFO_PATH)\ $<

{$(INFO_PATH)}.rc{$(INFO_PATH)}.res:
	$(RC) $(RFLAGS) -fo $@ $<

info.exe: nxsdk.lib $(INFO_OBJS)
    $(LD) $(LFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

KICK_PATH = src\kick
KICK_OBJS = $(KICK_PATH)\kick.obj $(KICK_PATH)\kick.res

{$(KICK_PATH)}.c{$(KICK_PATH)}.obj::
    $(CC) /c $(CFLAGS) /Fo$(KICK_PATH)\ $<

{$(KICK_PATH)}.rc{$(KICK_PATH)}.res:
	$(RC) $(RFLAGS) -fo $@ $<

kick.exe: nxsdk.lib $(KICK_OBJS)
    $(LD) $(LFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

RESOLVE_PATH = src\resolve
RESOLVE_OBJS = $(RESOLVE_PATH)\resolve.obj $(RESOLVE_PATH)\resolve.res

{$(RESOLVE_PATH)}.c{$(RESOLVE_PATH)}.obj::
    $(CC) /c $(CFLAGS) /Fo$(RESOLVE_PATH)\ $<

{$(RESOLVE_PATH)}.rc{$(RESOLVE_PATH)}.res:
	$(RC) $(RFLAGS) -fo $@ $<

resolve.exe: nxsdk.lib $(RESOLVE_OBJS)
    $(LD) $(LFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------

WHO_PATH = src\who
WHO_OBJS = $(WHO_PATH)\who.obj $(WHO_PATH)\who.res

{$(WHO_PATH)}.c{$(WHO_PATH)}.obj::
    $(CC) /c $(CFLAGS) /Fo$(WHO_PATH)\ $<

{$(WHO_PATH)}.rc{$(WHO_PATH)}.res:
	$(RC) $(RFLAGS) -fo $@ $<

who.exe: nxsdk.lib $(WHO_OBJS)
    $(LD) $(LFLAGS) /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------
