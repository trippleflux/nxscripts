################################################################################
#                nxAutoNuke - Extensive auto-nuker for ioFTPD.                 #
#                     Written by neoxed (neoxed@gmail.com)                     #
################################################################################

Topics:
 1. Information
 2. nxTools and AlcoBot Installation
 3. nxTools and dZSbot Installation
 4. Todo List
 5. Bugs and Comments

################################################################################
# 1. Information                                                               #
################################################################################

  nxAutoNuke is a fully featured auto-nuker. An auto-nuker allows you
to enforce strict rules on your FTP sites. Auto-nuke checks include:
MP3 genres and years; IMDB genres, ratings, and years; maximum CD/DVD/Disk
rules; empty directories; incomplete releases; banned keywords; and allowed
groups. Nuke reasons are customizable using cookies.

  Thanks to Harm for ioAUTONUKE, which showed me how I was able to use
ioA as an external nuker. ioA support is credited towards him. :)

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
# 3. nxTools and dZSbot Installation                                           #
################################################################################

1. UnRAR and copy all files and directories, maintaining the
   original directory structure, to ioFTPD\scripts\.

   The files must be placed as follows:
   ioFTPD\scripts\init.itcl
   ioFTPD\scripts\nxLib.tcl
   ioFTPD\scripts\nxAutoNuke.cfg
   ioFTPD\scripts\nxAutoNuke\nxAutoNuke.tcl

2. Configure nxAutoNuke.cfg to your liking. In order for dZSbot announcements
   to work correctly, misc(dZSbotLogging) must enabled in nxTools.cfg.

3. Add the following to your ioFTPD.ini:

[Scheduler]
nxAutoNuke = 0,10,20,30,40,50 * * * TCL ..\scripts\nxAutoNuke\nxAutoNuke.tcl

[FTP_Custom_Commands]
autonuke    = TCL ..\scripts\nxAutoNuke\nxAutoNuke.tcl

[FTP_SITE_Permissions]
autonuke    = M1

4. Rehash or restart ioFTPD.

5. Add the following to your dZSbot.tcl:

set disable(ANUKEALLOWED) 0
set disable(ANUKEBANNED)  0
set disable(ANUKEDISKS)   0
set disable(ANUKEEMPTY)   0
set disable(ANUKEIMDB)    0
set disable(ANUKEINC)     0
set disable(ANUKEKEYWORD) 0
set disable(ANUKEMP3)     0

set variables(ANUKEALLOWED) "%pf %group %age %timeleft %nuketime %multi %uploaders"
set variables(ANUKEBANNED)  "%pf %banned %age %timeleft %nuketime %multi %uploaders"
set variables(ANUKEDISKS)   "%pf %disks %maxdisks %age %timeleft %nuketime %multi %uploaders"
set variables(ANUKEEMPTY)   "%pf %age %timeleft %nuketime %multi %uploaders"
set variables(ANUKEIMDB)    "%pf %type %banned %age %timeleft %nuketime %multi %uploaders"
set variables(ANUKEINC)     "%pf %age %timeleft %nuketime %multi %uploaders"
set variables(ANUKEKEYWORD) "%pf %banned %age %timeleft %nuketime %multi %uploaders"
set variables(ANUKEMP3)     "%pf %type %banned %age %timeleft %nuketime %multi %uploaders"

set announce(ANUKEALLOWED) "-%sitename- \[%section\] + %path/%bold%release%bold is not from an allowed group, it will be nuked %bold%multi%boldx in %bold%timeleft%boldmins. Uploaders: %uploaders"
set announce(ANUKEBANNED)  "-%sitename- \[%section\] + %path/%bold%release%bold is a banned release (%bold%banned%bold), it will be nuked %bold%multi%boldx in %bold%timeleft%boldmins. Uploaders: %uploaders"
set announce(ANUKEDISKS)   "-%sitename- \[%section\] + %path/%bold%release%bold has %bold%disks%bold disks, exceeding the maximum of %bold%maxdisks%bold disks, it will be nuked %bold%multi%boldx in %bold%timeleft%boldmins. Uploaders: %uploaders"
set announce(ANUKEEMPTY)   "-%sitename- \[%section\] + %path/%bold%release%bold is still empty after %bold%age%boldmins, it will be nuked %bold%multi%boldx in %bold%timeleft%boldmins. Uploaders: %uploaders"
set announce(ANUKEIMDB)    "-%sitename- \[%section\] + %path/%bold%release%bold the IMDB %type %bold%banned%bold is banned, it will be nuked %bold%multi%boldx in %bold%timeleft%boldmins. Uploaders: %uploaders"
set announce(ANUKEINC)     "-%sitename- \[%section\] + %path/%bold%release%bold has been incomplete for %bold%age%boldmins, it will be nuked %bold%multi%boldx in %bold%timeleft%boldmins. Uploaders: %uploaders"
set announce(ANUKEKEYWORD) "-%sitename- \[%section\] + %path/%bold%release%bold is a banned type (%bold%banned%bold), it will be nuked %bold%multi%boldx in %bold%timeleft%boldmins. Uploaders: %uploaders"
set announce(ANUKEMP3)     "-%sitename- \[%section\] + %path/%bold%release%bold the MP3 %type %bold%banned%bold is banned, it will be nuked %bold%multi%boldx in %bold%timeleft%boldmins. Uploaders: %uploaders"

6. Find msgtypes(RACE) in your dZSbot.tcl file, and add the following events:

ANUKEALLOWED ANUKEBANNED ANUKEDISKS ANUKEEMPTY ANUKEIMDB ANUKEINC ANUKEKEYWORD ANUKEMP3

Example Line:
set msgtypes(RACE) "ANUKEALLOWED ANUKEBANNED ANUKEDISKS ANUKEEMPTY ANUKEIMDB ANUKEINC ANUKEKEYWORD ANUKEMP3 ..."

7. Rehash or restart Windrop.

################################################################################
# 4. Todo List                                                                 #
################################################################################

- RiA release name checks will only be added if someone can provide
  me with an original copy of the RiA standards/rules.

################################################################################
# 5. Bugs and Comments                                                         #
################################################################################

   If you have any problems with this script; or you found a bug, spelling
mistake or grammar error, please report it to me. If it's a technical issue,
be sure you can reproduce the problem, so I can find a solution.

IRC  : neoxed <#ioFTPD at EFnet>
Email: neoxed@gmail.com
