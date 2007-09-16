#
# nxMyDB - MySQL Database for ioFTPD
# Copyright (c) 2006-2007 neoxed
#
# Module Name:
#   Makefile
#
# Author:
#   neoxed (neoxed@gmail.com) Jun 3, 2006
#
# Abstract:
#   Project makefile for Microsoft's NMAKE.
#

# Build options
DEBUG       = 1
MSVCRT      = 0
PLATFORM    = X86
IOFTPD_INC  = D:\ioFTPD\source\Include
MYSQL_DIR   = C:\Program Files\MySQL\MySQL Server 5.0

# Release version
VERSION     = 0.2.0

# Compiler options
CC          = cl.exe
LD          = link.exe
RC          = rc.exe
CFLAGS      = /nologo /I ".\include" /I "$(IOFTPD_INC)" /I "$(MYSQL_DIR)\include" /W4
LFLAGS      = /nologo /MACHINE:$(PLATFORM) /MAP
RFLAGS      = /r

# -------------------------------------------------------------------------

!if !exist("makefile")
!error You must run this makefile from the directory it is located in.
!endif

!if !exist("$(IOFTPD_INC)\ioFTPD.h")
!error Unable to find ioFTPD.h; the IOFTPD_INC variable is incorrect.
!endif

!if !exist("$(MYSQL_DIR)\include\mysql.h")
!error Unable to find mysql.h; the MYSQL_DIR variable is incorrect.
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
MYSQL_LIB   = $(MYSQL_DIR)\lib\debug
!else
!message Debug Symbols: Off
CFLAGS      = $(CFLAGS) /Ox /GF /GL /GS- $(CRTLIB) /DNDEBUG
LFLAGS      = $(LFLAGS) /RELEASE /LTCG:STATUS /OPT:REF /OPT:NOWIN98
MYSQL_LIB   = $(MYSQL_DIR)\lib\opt
!endif

VERSION_RES = $(VERSION:.=,),0
CFLAGS      = $(CFLAGS) "/DVERSION=$(VERSION)" "/DVERSION_RES=$(VERSION_RES)"
RFLAGS      = $(RFLAGS) "/DVERSION=$(VERSION)" "/DVERSION_RES=$(VERSION_RES)"

# -------------------------------------------------------------------------

default: all

all: build

build: lib

clean: distclean
    -@DEL /F *.dll *.map *.pdb 2> nul

distclean:
    -@DEL /F *.ilk *.lib *.exp *.obj *.res vc*.pdb 2> nul

release: build distclean

lib: nxmydb.dll
    @COPY /Y "$(MYSQL_LIB)\libmysql.dll" . > nul

# -------------------------------------------------------------------------

FILE_OBJS = \
    mydb.res\
    array.obj\
    condvar.obj\
    database.obj\
    debug.obj\
    dllentry.obj\
    group.obj\
    groupdb.obj\
    groupdbsync.obj\
    groupfile.obj\
    idlist.obj\
    pool.obj\
    proctable.obj\
    user.obj\
    userdb.obj\
    userdbsync.obj\
    userfile.obj

{source\}.c{}.obj::
    $(CC) /c $(CFLAGS) /Fo $<

{resource\}.rc{}.res:
	$(RC) $(RFLAGS) /fo $@ $<

nxmydb.dll: $(FILE_OBJS)
    $(LD) $(LFLAGS) /DEF:resource\mydb.def /DLL /OUT:$@ rpcrt4.lib "$(MYSQL_LIB)\libmysql.lib" @<<
$**
<<

# -------------------------------------------------------------------------
