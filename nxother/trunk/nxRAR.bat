@echo off
CD /D %SYSTEMPATH%
ECHO RARing Current Directory...
ECHO !detach 0
DIR /A /B > .ioFTPD.rarlist
"C:\Program Files\WinRAR\rar.exe" a -m0 -x.ioFTPD* !CurrentDir.rar @.ioFTPD.rarlist
DEL /F .ioFTPD.rarlist
EXIT 0
