@ECHO OFF
::
:: AlcoExt - Alcoholicz Tcl extension.
:: Copyright (c) 2005 Alcoholicz Scripting Team
::
:: File Name:
::   clean.bat
::
:: Author:
::   neoxed (neoxed@gmail.com) June 26, 2005
::
:: Abstract:
::   Batch script to clean release and debug builds.
::

:: Tcl installation path.
SET TCL_DIR=D:\Projects\Tcl

:: -------------------------------------------------------------------------

CALL "C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat"
SET CURR_DIR=%CD%
CD ..

ECHO:========================================================================
ECHO: AlcoExt - Cleaning
ECHO:========================================================================

nmake -nologo -f makefile.win distclean
IF errorlevel 1 GOTO error

ECHO *** Finished ***
GOTO end

:error
ECHO *** Error ***

CD %CURR_DIR%
:end
PAUSE
