@ECHO OFF
::
:: AlcoExt - Alcoholicz Tcl extension.
:: Copyright (c) 2005 Alcoholicz Scripting Team
::
:: File Name:
::   build.bat
::
:: Author:
::   neoxed (neoxed@gmail.com) June 26, 2005
::
:: Abstract:
::   Batch script to build a release and debug version of AlcoExt.
::

:: Tcl installation path.
SET TCL_DIR=D:\Projects\Tcl

:: -------------------------------------------------------------------------

CALL "C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat"
SET CURR_DIR=%CD%
CD ..

ECHO:=======================================================================
ECHO: AlcoExt - Release Build
ECHO:=======================================================================

nmake -nologo -f makefile.win all THREADS=1
IF errorlevel 1 GOTO error

ECHO:=======================================================================
ECHO: AlcoExt - Debug Build
ECHO:=======================================================================

nmake -nologo -f makefile.win all DEBUG=1 MEMDEBUG=1 THREADS=1
IF errorlevel 1 GOTO error

ECHO *** Finished ***
GOTO end

:error
ECHO *** Error ***

CD %CURR_DIR%
:end
PAUSE
