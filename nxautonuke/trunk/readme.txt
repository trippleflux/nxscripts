################################################################################
#                nxAutoNuke - Extensive auto-nuker for ioFTPD.                 #
#                     Written by neoxed (neoxed@gmail.com)                     #
################################################################################

Topics:
 1. Information
 2. nxTools and AlcoBot Installation
 3. Directory Tags
 4. Bugs and Comments
 5. License

################################################################################
# 1. Information                                                               #
################################################################################

  nxAutoNuke is a fully featured auto-nuker. An auto-nuker allows you
to enforce strict rules on your FTP sites. Auto-nuke checks include:
MP3 genres and years; IMDB genres, ratings, and years; maximum CD/DVD/Disk
rules; empty directories; incomplete releases; banned keywords; and allowed
groups. Nuke reasons are customizable using cookies.

################################################################################
# 2. nxTools and AlcoBot Installation                                          #
################################################################################

1. UnRAR and copy all files and directories, maintaining the
   original directory structure, to ioFTPD\scripts\.

   The files must be placed as follows:
   ioFTPD\scripts\init.itcl
   ioFTPD\scripts\nxLib.tcl
   ioFTPD\scripts\nxAutoNuke\nxAutoNuke.cfg
   ioFTPD\scripts\nxAutoNuke\nxAutoNuke.tcl

2. Configure nxAutoNuke.cfg to your liking. Read the comments carefully!

3. Add the following to your ioFTPD.ini:

[Scheduler]
nxAutoNuke = 0,10,20,30,40,50 * * * TCL ..\scripts\nxAutoNuke\nxAutoNuke.tcl

[FTP_Custom_Commands]
autonuke    = TCL ..\scripts\nxAutoNuke\nxAutoNuke.tcl

[FTP_SITE_Permissions]
autonuke    = M1

4. Rehash or restart ioFTPD.

################################################################################
# 3. Directory Tags                                                            #
################################################################################

    Parsing is done using regular expressions. For more information on regular
expressions, see: http://www.tcl.tk/man/tcl8.4/TclCmd/re_syntax.htm

- dZSbot IMDB Tag:
  [IMDB] - Action (2004) - 2.7 of 10 - [IMDB]
  set anuke(ImdbMatch) {^\[IMDB\] - (.+) \((\d+)\) - ([\d\.]+) of 10 - \[IMDB\]$}
  set anuke(ImdbOrder) {genre year rating}

- ioSFV MP3 Tag:
  -[100%]-[62.71MB in 11 files with 11 tracks - Death Metal 2005 256kbps]-[race won by user]-
  set anuke(MP3Match) {^-\[.*\]-\[.+ in \d+ files with \d+ tracks - (.+) (\d+) (\d+)kbps\]-\[race won by .+\]-$}
  set anuke(MP3Order) {genre year bitrate}

- ioSFV Incomplete Tag:
  -[%%%%%.....] - 5F Of 10F done - 50% complete - [SITE]-
  set anuke(IncTag)   {-\[*\] - *F Of *F done - * complete - \[*\]-}

- Project-ZS MP3 Tag:
  [SITE] - ( 62.7MB 11F - COMPLETE - Death Metal 2005 ) - [SITE]
  set anuke(MP3Match) {^\[.*\] - \( .* - COMPLETE - (.+) (\d+) \) - \[.*\]$}
  set anuke(MP3Order) {genre year}

- Project-ZS Incomplete Tag:
  [SITE] - 50% Complete - [SITE]
  set anuke(IncTag)   {\[*\] - *% Complete - \[*\]}

################################################################################
# 4. Bugs and Comments                                                         #
################################################################################

   If you have ideas for improvements or are experiencing problems with this
script, please do not hesitate to contact me. If your problem is a technical
issue (i.e. a crash or operational defect), be sure to provide me with the steps
necessary to reproduce it.

IniCom Forum:
http://www.inicom.net/forum/forumdisplay.php?f=68

IRC Network:
neoxed in #ioFTPD at EFnet

E-mail:
neoxed@gmail.com

################################################################################
# 5. License                                                                   #
################################################################################

   See the "license.txt" file for details.
