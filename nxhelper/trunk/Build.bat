@ECHO OFF
::
:: nxHelper Library - Tcl extension for the nxHelper sitebot.
:: Copyright (c) 2005 nxHelper Development Team
::
:: File Name:
::   Build.bat
::
:: Author:
::   neoxed (neoxed@gmail.com) May 22, 2005
::
:: Abstract:
::   Build script for Microsoft's C/C++ compiler.
::

:: ----------------------------------------------------------
:: Configuration
::
:: Tcl installation path.
SET TCLDIR=D:\Projects\Tcl

:: Zlib installation path.
SET ZLIBDIR=.\zlib
:: ----------------------------------------------------------

CALL "C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"

TITLE Building nxHelper Tcl Extension - Compiling...

ECHO:=======================================================================
ECHO: Support Libraries
ECHO:=======================================================================

SET CURRPATH=%CD%

IF EXIST "%ZLIBDIR%\zlib.lib" (
    ECHO zLib already built.
) ELSE (
    cd %ZLIBDIR%
    nmake -nologo -f makefile.vc
    cd %CURRPATH%
)

ECHO:=======================================================================
ECHO: nxHelper - Release Build
ECHO:=======================================================================

nmake -nologo -f makefile.vc release OPTS=threads
IF errorlevel 1 GOTO error

ECHO:=======================================================================
ECHO: nxHelper - Debug Build
ECHO:=======================================================================

nmake -nologo -f makefile.vc release OPTS=threads,symbols STATS=memdbg
IF errorlevel 1 GOTO error

TITLE Building nxHelper Tcl Extension - Finished
ECHO *** Finished ***
GOTO end

:error
TITLE Building Inferno Tcl Extension - Error
ECHO *** Error ***

:end
PAUSE
