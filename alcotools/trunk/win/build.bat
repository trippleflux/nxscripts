@ECHO OFF
CALL "%VS80COMNTOOLS%\vsvars32.bat"
SET APR_DIR=E:\Projects\Libraries\StaticCRT\APR-1.2.7
SET SQLITE_DIR=E:\Projects\Libraries\StaticCRT\SQLite-3.3.6
SET ZLIB_DIR=E:\Projects\Libraries\StaticCRT\Zlib-1.2.3
SET STATIC_CRT=1
SET STATIC_LIB=1

ECHO:=======================================================================
ECHO: AlcoTools - Release Build
ECHO:=======================================================================

nmake -nologo -f makefile.win
IF errorlevel 1 GOTO error

ECHO:=======================================================================
ECHO: AlcoTools - Debug Build
ECHO:=======================================================================

nmake -nologo -f makefile.win DEBUG=1 MEMDEBUG=1
IF errorlevel 1 GOTO error

ECHO:=======================================================================
ECHO *** Finished ***
GOTO end

:error
ECHO:=======================================================================
ECHO *** Error ***

:end
PAUSE
