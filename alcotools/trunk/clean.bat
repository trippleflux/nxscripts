@ECHO OFF
CALL "E:\Projects\vc2005.bat"
SET SQLITE_DIR=E:\Projects\Libraries\sqlite3
SET ZLIB_DIR=E:\Projects\Libraries\zlib

ECHO:========================================================================
ECHO: AlcoTools - Cleaning
ECHO:========================================================================

nmake -nologo -f makefile.win distclean
IF errorlevel 1 GOTO error

ECHO *** Finished ***
GOTO end

:error
ECHO *** Error ***

:end
PAUSE
