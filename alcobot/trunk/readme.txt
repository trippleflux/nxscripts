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

1. UnRAR and copy all files and directories, maintaining the
   original directory structure, to your Eggdrop directory.

   The files and directories should be placed as follows:
   Eggdrop/AlcoBot/AlcoBot.conf
   Eggdrop/AlcoBot/AlcoBot.tcl
   Eggdrop/AlcoBot/AlcoBot.vars
   Eggdrop/AlcoBot/libs/
   Eggdrop/AlcoBot/modules/
   Eggdrop/AlcoBot/themes/
   Eggdrop/AlcoBot/vars/

2. If you checked out a copy from the Subversion repository, you must rename
   the appropriate .conf file for your FTP daemon. For example, if you are
   using ioFTPD, rename the AlcoBot-ioFTPD.conf file to AlcoBot.conf.

3. Configure AlcoBot.conf to your liking. Read the comments carefully!

4. Follow module specific instructions in section 3 of this manual.

5. Add the following to your eggdrop.conf:
   source AlcoBot/AlcoBot.tcl

6. Rehash or restart Eggdrop.


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

  1. Load the "ioa" module; see section 4 of this manual for more information.

  ############################################################
  # iojZS                                                    #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by jeza.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=20

  1. Add "iojzs.vars" to varFiles, under [General], in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # ioSFV                                                    #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by tUFF.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=41

  1. Add "iosfv.vars" to varFiles, under [General], in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # ioZS                                                     #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by StarDog.
  URL : http://www.iozs.com

  1. Add "iozs.vars" to varFiles, under [General], in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # nxAutoNuke                                               #
  ############################################################

  FTPD: ioFTPD
  Info: Auto-nuker, written by neoxed.
  URL : http://www.iozs.com

  1. Add "nxautonuke.vars" to varFiles, under [General], in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # nxTools                                                  #
  ############################################################

  FTPD: ioFTPD
  Info: Site command package, written by neoxed.
  URL : http://www.iozs.com

  1. Load the "nxtools" module; see section 4 of this manual for more information.

  ############################################################
  # php_psio                                                 #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by SnypeTEST.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=49

  1. Add "php_psio.vars" to varFiles, under [General], in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # Project-ZS                                               #
  ############################################################

  FTPD: ioFTPD
  Info: Zipscript, written by Caladan and esmandil.
  URL : http://www.inicom.net/pages/en.ioftpd-scripts.php?id=98

  1. Add "pzs.vars" to varFiles, under [General], in AlcoBot.conf.
  2. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.

  ############################################################
  # Project-ZS-NG                                            #
  ############################################################

  FTPD: glFTPD
  Info: Zipscript, written by the PZS-NG team.
  URL : http://www.pzs-ng.com

  1. Recompile PZS-NG using the provided constants.h file.
     cp -f other/constants.h ~/project-zs-ng/src/zipscript/
     cd ~/project-zs-ng
     make clean install
  2. Add "pzs-ng.vars" to varFiles, under [General], in AlcoBot.conf.
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

  1. Add "invite" to the module list in AlcoBot.conf.
  2. Create an ODBC DSN connection to your database. Use the table schema
     provided in invite.sql, which is located in the module's directory.
  3. Configure the [Module::Invite] section in AlcoBot.conf.
  4. Reload AlcoBot by entering the command ".alcobot reload" in a DCC chat session.
  5. Instructions for installing the SITE commands are in the top of the
     siteInvite.tcl file, which is located in the module's directory.

  ############################################################
  # ioa                                                      #
  ############################################################

  FTPD: ioFTPD
  Info: Announce log events and query data files used by ioA.
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
  Info: Display online statistics.
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

  1. Add "pretimes" to the module list in AlcoBot.conf.
  2. Create an ODBC DSN connection to your database. Use the table schema
     provided in pretimes.sql, which is located in the module's directory.
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
