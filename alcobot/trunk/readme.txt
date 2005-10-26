################################################################################
#                  AlcoBot - Extensive multiplatform sitebot.                  #
################################################################################

Topics:
 1. Information
 2. Installation
 3. Zipscript
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
# 3. Zipscript                                                                 #
################################################################################

    AlcoBot supports multiple zipscripts, but you must load the appropriate
variable definition file. Select your desired zipscript, and add the .vars
file name to "varFiles", under [General], in the AlcoBot configuration file.

    iozs.vars     - ioZS for ioFTPD, written by StarDog.
                  - URL: http://www.iozs.com

    iojzs.vars    - iojZS for ioFTPD, written by jeza.
                  - URL: http://www.inicom.net/pages/en.ioftpd-scripts.php?id=20

    iosfv.vars    - ioSFV for ioFTPD, written by _tUFF.
                  - URL: http://www.inicom.net/pages/en.ioftpd-scripts.php?id=41

    php_psio.vars - php_psio for ioFTPD, written by SnypeTEST.
                  - URL: http://www.inicom.net/pages/en.ioftpd-scripts.php?id=49

    pzs.vars      - Project-ZS for ioFTPD, written by Caladan and esmandil.
                  - URL: http://www.inicom.net/pages/en.ioftpd-scripts.php?id=98

    pzs-ng.vars   - Project-ZS-NG for glFTPD, written by the PZS-NG team.
                  - URL: http://www.pzs-ng.com
                  - You must recompile PZS-NG using the provided constants.h file.
                    cp -f other/constants.h ~/project-zs-ng/src/zipscript/
                    cd ~/project-zs-ng/ && make clean && make


################################################################################
# 4. Modules                                                                   #
################################################################################

############################################################
# free                                                     #
############################################################

FTPD: All
Info: Display available drive space.
Path: modules/generic/free

1. Add "free" to the module list in AlcoBot.conf.
2. Configure the [Module::Free] section in AlcoBot.conf.
3. Reload AlcoBot by entering the command ".alcobot reload" in DCC chat.

############################################################
# groups                                                   #
############################################################

FTPD: All
Info: Manage affiliated and banned groups.
Path: modules/generic/groups

1. Add "groups" to the module list in AlcoBot.conf.
2. Copy "groups.conf" from the module's directory to AlcoBot's directory.
3. Configure the [Module::Groups] section in AlcoBot.conf.
4. Reload AlcoBot by entering the command ".alcobot reload" in DCC chat.
5. Edit groups.conf to your liking (changes to this file do not require a reload).

############################################################
# help                                                     #
############################################################

FTPD: All
Info: Display supported channel commands.
Path: modules/generic/help

1. Add "help" to the module list in AlcoBot.conf.
2. Reload AlcoBot by entering the command ".alcobot reload" in DCC chat.

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
4. Reload AlcoBot by entering the command ".alcobot reload" in DCC chat.
5. Instructions for installing the SITE commands are in the top of the
   siteInvite.tcl file, which is located in the module's directory.

############################################################
# nxtools                                                  #
############################################################

FTPD: ioFTPD
Info: Interact with nxTools databases.
Path: modules/generic/nxtools

1. Add "nxtools" to the module list in AlcoBot.conf.
2. Configure the [Module::NxTools] section in AlcoBot.conf.
3. Reload AlcoBot by entering the command ".alcobot reload" in DCC chat.

############################################################
# online                                                   #
############################################################

FTPD: glFTPD/ioFTPD
Info: Display online statistics.
Path: modules/*ftpd/online

1. Add "online" to the module list in AlcoBot.conf.
2. Configure the [Module::Online] section in AlcoBot.conf.
3. Reload AlcoBot by entering the command ".alcobot reload" in DCC chat.

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
4. Reload AlcoBot by entering the command ".alcobot reload" in DCC chat.

############################################################
# readlogs                                                 #
############################################################

FTPD: All
Info: Read and announce log entries.
Path: modules/generic/readlogs

1. Add "readlogs" to the module list in AlcoBot.conf.
2. Configure the [Module::ReadLogs] section in AlcoBot.conf.
3. Reload AlcoBot by entering the command ".alcobot reload" in DCC chat.


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
