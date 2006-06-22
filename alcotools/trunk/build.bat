@ECHO OFF
CALL "E:\Projects\vc2005.bat"
SET SQLITE_DIR=E:\Projects\Libraries\sqlite3
SET ZLIB_DIR=E:\Projects\Libraries\zlib

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
