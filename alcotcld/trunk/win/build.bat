@ECHO OFF
::
:: AlcoTcld - Alcoholicz Tcl daemon.
:: Copyright (c) 2005 Alcoholicz Scripting Team
::
:: File Name:
::   build.bat
::
:: Author:
::   neoxed (neoxed@gmail.com) July 17, 2005
::
:: Abstract:
::   Batch script to build a release and debug version of AlcoTcld.
::

CALL "C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat"

ECHO:=======================================================================
ECHO: AlcoTcld - Release Build
ECHO:=======================================================================

SET TCL_DIR=D:\Projects\TclTest\8.4.11-Release
nmake -nologo -f makefile all THREADS=1
IF errorlevel 1 GOTO error

ECHO:=======================================================================
ECHO: AlcoTcld - Debug Build
ECHO:=======================================================================

SET TCL_DIR=D:\Projects\TclTest\8.4.11-Debug
nmake -nologo -f makefile all DEBUG=1 MEMDEBUG=1 THREADS=1
IF errorlevel 1 GOTO error

ECHO *** Finished ***
GOTO end

:error
ECHO *** Error ***

:end
PAUSE
