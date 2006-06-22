@ECHO OFF
CALL "%VS80COMNTOOLS%\vsvars32.bat"
SET APR_DIR=E:\Projects\Libraries\APR-1.2.7
SET SQLITE_DIR=E:\Projects\Libraries\SQLite-3.3.6
SET ZLIB_DIR=E:\Projects\Libraries\Zlib-1.2.3

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

ECHO *** Finished ***
GOTO end

:error
ECHO *** Error ***

:end
PAUSE
