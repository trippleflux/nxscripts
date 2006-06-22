@ECHO OFF
CALL "%VS80COMNTOOLS%\vsvars32.bat"
SET APR_DIR=E:\Projects\Libraries\APR-1.2.7
SET SQLITE_DIR=E:\Projects\Libraries\SQLite-3.3.6
SET ZLIB_DIR=E:\Projects\Libraries\Zlib-1.2.3

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
