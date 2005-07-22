@ECHO OFF
::
:: nxHelper - Tcl extension for nxTools.
:: Copyright (c) 2005 neoxed
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

ECHO:========================================================================
ECHO: nxHelper - Cleaning
ECHO:========================================================================

nmake -nologo -f makefile distclean
IF errorlevel 1 GOTO error

ECHO *** Finished ***
GOTO end

:error
ECHO *** Error ***

:end
PAUSE
