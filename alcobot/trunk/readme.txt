################################################################################
#                  AlcoBot - Extensive multiplatform sitebot.                  #
################################################################################

Topics:
 1. Information
 2. Installation
 3. Scripts
 4. Modules
 5. Bugs and Comments
 6. License

################################################################################
# 1. Information                                                               #
################################################################################

AlcoBot is a modular sitebot written in Tcl for ioFTPD and glFTPD.

- Easy-to-use flag system; disable messages or redirect them to another channel.
- Redirect command output to private message or user notice, helps reduces spam.
- Text based configuration file; no fumbling with Tcl syntax errors.
- Module system; reducing bloat by allowing users to add and remove functionality.
- Extensive theming and value formatting; the bot will convert duration, size,
  speed, and time values automatically.
- Event callbacks; allow module writers to hook log messages and internal functions.
- Multiplatform and multi-FTPD; works with ioFTPD, and glFTPD.


################################################################################
# 2. Installation                                                              #
################################################################################

  ############################################################
  # Windows                                                  #
  ############################################################

  1. Download and install the Cygwin build of Eggdrop. I reccomend using
     Bounty's version, available at: http://users.skynet.be/bounty/index2.html

  2. Unpack the AlcoBot archive then copy all files and subdirectories to your
     Eggdrop directory. Be sure to maintain the following directory structure:

     eggdrop\AlcoBot\AlcoBot.conf
     eggdrop\AlcoBot\AlcoBot.tcl
     eggdrop\AlcoBot\AlcoBot.vars
     eggdrop\AlcoBot\modules\*
     eggdrop\AlcoBot\packages\*
     eggdrop\AlcoBot\themes\*

  3. Rename the AlcoBot-ioFTPD.conf file to AlcoBot.conf. If you downloaded
     the Windows binary package, this step may not be necessary.

  4. Configure AlcoBot.conf to your liking. Read the comments carefully!

  5. Add the following line to the bottom of your eggdrop.conf file:

     source AlcoBot/AlcoBot.tcl

  6. Rehash or restart Eggdrop for the changes to take affect.

  ############################################################
  # FreeBSD/Linux                                            #
  ############################################################

  1. Download and install Tcl v8.4, or newer.

     Debian : apt-get install tcl8.4-dev
     FreeBSD: portupgrade -N tcl84
     Source : http://sourceforge.net/project/showfiles.php?group_id=10894

  2. Download and install the AlcoExt Tcl extension.

     Source : http://www.alcoholicz.com

  3. The following packages are optional, but highly recommended.

     TclTLS   - SSL/TLS support for FTP connections.
       Debian : apt-get install tcltls
       FreeBSD: portupgrade -N tcltls
       Source : http://sourceforge.net/project/showfiles.php?group_id=13248

     MySQLTcl  - MySQL database support.
       Debian : apt-get install mysqltcl
       FreeBSD: portupgrade -N mysqltcl
       Source : http://www.xdobry.de/mysqltcl/

     pgTcl     - PostgreSQL database support.
       Debian : apt-get install libpgtcl
       Source : http://gborg.postgresql.org/project/pgtclng/download/download.php

     TclSQLite - SQLite database support.
       Debian : apt-get install libsqlite3-tcl
       FreeBSD: portupgrade -N sqlite3
       Source : http://www.sqlite.org/download.html

  4. Download and install Eggdrop.

     Source : http://www.eggheads.org

  5. Unpack the AlcoBot archive then copy all files and subdirectories to your
     Eggdrop directory. Be sure to maintain the following directory structure:

     eggdrop/AlcoBot/AlcoBot.conf
     eggdrop/AlcoBot/AlcoBot.tcl
     eggdrop/AlcoBot/AlcoBot.vars
     eggdrop/AlcoBot/modules/*
     eggdrop/AlcoBot/packages/*
     eggdrop/AlcoBot/themes/*

  6. Rename the AlcoBot-glFTPD.conf file to AlcoBot.conf.

  7. Configure AlcoBot.conf to your liking. Read the comments carefully!

  8. Add the following line to the bottom of your eggdrop.conf file:

     source AlcoBot/AlcoBot.tcl

  9. Rehash or restart Eggdrop for the changes to take affect.


################################################################################
# 3. Scripts                                                                   #
################################################################################

    AlcoBot supports many third party scripts, but they must be enabled in order
for AlcoBot to recognize them. Read the following installation instructions:

  ############################################################
  # ioA                                                      #
  ############################################################

  FTPD: ioFTPD
  Info: Site command package, written by WarC.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=5

  1. You must be using ioA v2.0.6, or newer.
  2. Change the options in your ioA.cfg file to match the following:

     [Wipe]
     Wipe_Log         = ""%vfs/%release" "%user" "%group" "%.0dirs" "%.0files" "%.3mb""

     [Search]
     Search_Using_Log = 1

     [Request]
     Request_Log      = ""%user" "%group" "%request""
     Reqdel_Log       = ""%user" "%group" "%request""
     Reqfilled_Log    = ""%user" "%group" "%request""
     Request_Wipe_Log = ""%release" "%.0dirs" "%.0files" "%.3mb""

     [Credits]
     Credits_Log      = ""%user" "%group" "%.3mb" "%target""

     [Newdate]
     Newdate_Log      = ""%vfs" "%area" "%desc""

     [PRE]
     Pre_Log          = ""%vfs/%release" "%user" "%group" "%type" "%.0files" "%.3mb""

     [Nuke]
     Nuke_Log         = ""%vfs" "%user" "%group" "%.0multi" "%reason" "%.3size" "%nukees""
     Nuke_Single_Nukees_Log_Line = 1

     [Unnuke]
     UnNuke_Log       = ""%vfs" "%user" "%group" "%.0multi" "%reason" "%.3size" "%nukees""

  3. Load the "ioa" module; see section 4 of this document for more information.

  ############################################################
  # iojZS                                                    #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by jeza.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=20

  1. Add "iojzs" to the module list in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # ioSFV                                                    #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by tUFF.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=41

  1. Add "iosfv" to the module list in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # ioZS                                                     #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by StarDog.
  URL : http://www.iozs.com

  1. Add "iozs" to the module list in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # nxAutoNuke                                               #
  ############################################################

  FTPD: ioFTPD
  Info: Auto-nuker, written by neoxed.
  URL : http://www.iozs.com

  1. Add "nxautonuke" to the module list in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # nxTools                                                  #
  ############################################################

  FTPD: ioFTPD
  Info: Site command package, written by neoxed.
  URL : http://www.iozs.com

  1. Load the "nxtools" module; see section 4 of this document for more information.

  ############################################################
  # php_psio                                                 #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by SnypeTEST.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=49

  1. Add "phppsio" to the module list in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # Project-ZS                                               #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by Caladan and esmandil.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=98

  1. Add "projectzs" to the module list in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # Project-ZS-NG                                            #
  ############################################################

  FTPD: glFTPD
  Info: Zipscript, written by the PZS-NG team.
  URL : http://www.pzs-ng.com

  1. Recompile PZS-NG using the provided constants.h file.
     cp -f AlcoBot/other/constants.h project-zs-ng/src/zipscript/
     cd project-zs-ng
     make clean install
  2. Add "pzsng" to the module list in AlcoBot.conf.
  3. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # WarChive                                                 #
  ############################################################

  FTPD: ioFTPD
  Info: Archiver, written by WarC.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=7

  1. Change the options in your WarChive.cfg file to match the following:

     [Log]
     WarcMove=""%oldvfs/%release" "%newvfs/%release" "%.2relsize" "%.2beforesize" "%.2aftersize""
     WarcWipe=""%oldvfs/%release" "%.2relsize" "%.2beforesize" "%.2aftersize""

  2. Add "warchive" to the module list in AlcoBot.conf.
  3. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

################################################################################
# 4. Modules                                                                   #
################################################################################

    AlcoBot includes a variety of built-in modules that must be configured
before use. Read the following installation instructions:

  ############################################################
  # bouncer                                                  #
  ############################################################

  FTPD: All
  Info: Display FTP bouncer status.
  Path: modules/generic/bouncer

  1. Add "bouncer" to the module list in AlcoBot.conf.
  2. Configure the [Module::Bouncer] section in AlcoBot.conf.
  3. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # free                                                     #
  ############################################################

  FTPD: All
  Info: Display available drive space.
  Path: modules/generic/free

  1. Add "free" to the module list in AlcoBot.conf.
  2. Configure the [Module::Free] section in AlcoBot.conf.
  3. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # gldata                                                   #
  ############################################################

  FTPD: glFTPD
  Info: Query data files used by glFTPD.
  Path: modules/glftpd/gldata

  1. Add "gldata" to the module list in AlcoBot.conf.
  2. Configure the [Module::GlData] section in AlcoBot.conf.
  3. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # groups                                                   #
  ############################################################

  FTPD: All
  Info: Display affiliated and banned groups.
  Path: modules/generic/groups

  1. Add "groups" to the module list in AlcoBot.conf.
  2. Copy "groups.conf" from the module's directory to AlcoBot's directory.
  3. Configure the [Module::Groups] section in AlcoBot.conf.
  4. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.
  5. Edit groups.conf to your liking (changes to this file do not require a reload).

  ############################################################
  # help                                                     #
  ############################################################

  FTPD: All
  Info: Display supported channel commands.
  Path: modules/generic/help

  1. Add "help" to the module list in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # invite                                                   #
  ############################################################

  FTPD: All
  Info: Invite users into selected IRC channel(s).
  Path: modules/generic/invite

  1. Create a MySQL, PostgreSQL, or SQLite database. The tables are
     automatically created when AlcoBot connects to the database.
  2. Add "invite" to the module list in AlcoBot.conf.
  3. Configure the [Module::Invite] section in AlcoBot.conf.
  4. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.
  5. Instructions for installing the SITE commands are in the top of the
     siteInvite.tcl file, which is located in the module's directory.

  ############################################################
  # ioa                                                      #
  ############################################################

  FTPD: ioFTPD
  Info: Announce log events and query log files used by ioA.
  Path: modules/generic/ioa

  1. Add "ioa" to the module list in AlcoBot.conf.
  2. Configure the [Module::IoA] section in AlcoBot.conf.
  3. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # nxtools                                                  #
  ############################################################

  FTPD: ioFTPD
  Info: Announce log events and query databases used by nxTools.
  Path: modules/generic/nxtools

  1. Add "nxtools" to the module list in AlcoBot.conf.
  2. Configure the [Module::NxTools] section in AlcoBot.conf.
  3. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # online                                                   #
  ############################################################

  FTPD: glFTPD/ioFTPD
  Info: Display online users and bandwidth usage.
  Path: modules/*ftpd/online

  1. Add "online" to the module list in AlcoBot.conf.
  2. Configure the [Module::Online] section in AlcoBot.conf.
  3. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # pretimes                                                 #
  ############################################################

  FTPD: All
  Info: Display and search for release pre times.
  Path: modules/generic/pretimes

  1. Create a MySQL, PostgreSQL, or SQLite database. The tables are
     automatically created when AlcoBot connects to the database.
  2. Add "pretimes" to the module list in AlcoBot.conf.
  3. Configure the [Module::PreTimes] section in AlcoBot.conf.
  4. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # readlogs                                                 #
  ############################################################

  FTPD: All
  Info: Read and announce log entries.
  Path: modules/generic/readlogs

  1. Add "readlogs" to the module list in AlcoBot.conf.
  2. Configure the [Module::ReadLogs] section in AlcoBot.conf.
  3. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # sitecmd                                                  #
  ############################################################

  FTPD: All
  Info: Issue SITE commands from IRC.
  Path: modules/generic/sitecmd

  1. Add "sitecmd" to the module list in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.


################################################################################
# 5. Bugs and Comments                                                         #
################################################################################

   If you have any problems with this script, whether it is a bug, spelling
mistake or grammatical error, please report it to the team. If it is a technical
issue, make sure you can reproduce the problem and provide us the necessary steps.

IniCom Forum:
http://www.inicom.net/forum/forumdisplay.php?f=157

Issue Tracker:
http://www.alcoholicz.com/newticket

Website:
http://www.alcoholicz.com


################################################################################
# 6. License                                                                   #
################################################################################

   See the "license.txt" file for details.
