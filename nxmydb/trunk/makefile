#
# nxMyDB - MySQL Database for ioFTPD
# Copyright (c) 2006 neoxed
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

## Build options
DEBUG       = 1
MSVCRT      = 0
PLATFORM    = X86
IOFTPD_INC  = E:\ioFTPD\source\Include
MYSQL_DIR   = C:\Program Files\MySQL\MySQL Server 5.0

## Release version.
VERSION     = 0.1.0

## Compiler options
CC          = cl
LD          = link
RC          = rc
CFLAGS      = /nologo /I "$(IOFTPD_INC)" -I"$(MYSQL_DIR)\include" /W4
LFLAGS      = /nologo /MACHINE:$(PLATFORM)
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
CFLAGS      = $(CFLAGS) /DDYNAMIC_CRT
!else
!message CRT Library: libcmt (static)
CRTLIB      = /MT
CFLAGS      = $(CFLAGS) /DSTATIC_CRT
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

all: lib

clean:
    @ECHO Deleting object files...
    -@DEL /F *.cod *.dll *.ilk *.lib *.exp *.obj *.pdb *.res 2> nul

release: all
    -@DEL /F *.cod *.ilk *.lib *.exp *.obj *.pdb *.res 2> nul

lib: nxmydb.dll
    @COPY /Y "$(MYSQL_LIB)\libmysql.dll" . > nul

# -------------------------------------------------------------------------

FILE_OBJS = mydb.res condvar.obj dbconn.obj debug.obj dllentry.obj group.obj\
            groupdb.obj groupfile.obj pool.obj proctable.obj user.obj userdb.obj\
            userfile.obj

{src\}.c{}.obj::
    $(CC) /c $(CFLAGS) /Fo $<

{src\}.rc{}.res:
	$(RC) $(RFLAGS) /fo $@ $<

nxmydb.dll: $(FILE_OBJS)
    $(LD) $(LFLAGS) /DEF:src\mydb.def /DLL /OUT:$@ "$(MYSQL_LIB)\libmysql.lib" @<<
$**
<<

# -------------------------------------------------------------------------
