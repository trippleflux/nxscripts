################################################################################
#                nxAutoNuke - Extensive auto-nuker for ioFTPD.                 #
#                     Written by neoxed (neoxed@gmail.com)                     #
################################################################################

Topics:
 1. Information
 2. nxTools and AlcoBot Installation
 3. Todo List
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
   ioFTPD\scripts\nxAutoNuke.cfg
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
# 3. Todo List                                                                 #
################################################################################

- RiA release name checks will only be added if someone can provide
  me with an original copy of the RiA standards/rules.


################################################################################
# 4. Bugs and Comments                                                         #
################################################################################

   If you have any problems with this script; or you found a bug, spelling
mistake or grammar error, please report it to me. If it's a technical issue,
be sure you can reproduce the problem, so I can find a solution.

IRC  : neoxed <#ioFTPD at EFnet>
Email: neoxed@gmail.com


################################################################################
# 5. License                                                                   #
################################################################################

   See the "license.txt" file for details.
