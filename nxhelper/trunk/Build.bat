@ECHO OFF
::
:: nxHelper - Tcl extension for nxTools.
:: Copyright (c) 2004-2008 neoxed
::
:: File Name:
::   build.bat
::
:: Author:
::   neoxed (neoxed@gmail.com) June 26, 2005
::
:: Abstract:
::   Batch script to build a release and debug version of nxHelper.
::

SET TCL_DIR=C:\Tools\Tcl
CALL "C:\Program Files\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat"

ECHO:=======================================================================
ECHO: nxHelper - Release Build
ECHO:=======================================================================

nmake -nologo -f makefile all THREADS=1
IF errorlevel 1 GOTO error

ECHO:=======================================================================
ECHO: nxHelper - Debug Build
ECHO:=======================================================================

nmake -nologo -f makefile all DEBUG=1 MEMDEBUG=1 THREADS=1
IF errorlevel 1 GOTO error

ECHO *** Finished ***
GOTO end

:error
ECHO *** Error ***

:end
PAUSE
