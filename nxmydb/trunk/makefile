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
IOFTPD_INC  = E:\ioFTPD\source\Include

## Release version.
VERSION     = 0.1.0

## Compiler options
CC          = cl
LD          = link
RC          = rc
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
!else
!message Debug Symbols: Off
CFLAGS      = $(CFLAGS) /Ox /GF $(CRTLIB) /DNDEBUG
LFLAGS      = $(LFLAGS) /RELEASE /OPT:REF
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

# -------------------------------------------------------------------------

FILE_OBJS = mydb.res condvars.obj dbconn.obj dllentry.obj group.obj groupdb.obj groupfile.obj\
            proctable.obj user.obj userdb.obj userfile.obj utils.obj

{src\}.c{}.obj::
    $(CC) /c $(CFLAGS) /Fo $<

{src\}.rc{}.res:
	$(RC) $(RFLAGS) /fo $@ $<

nxmydb.dll: $(FILE_OBJS)
    $(LD) $(LFLAGS) /DEF:src\mydb.def /DLL /OUT:$@ @<<
$**
<<

# -------------------------------------------------------------------------
