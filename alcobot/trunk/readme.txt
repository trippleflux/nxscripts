################################################################################
#                  AlcoBot - Extensive multiplatform sitebot.                  #
################################################################################

Topics:
 1. Information
 2. Installation
 3. Bugs and Comments
 4. License

################################################################################
# 1. Information                                                               #
################################################################################

AlcoBot is a modular sitebot written in Tcl for ioFTPD and glFTPD.

- Easy-to-use flag system; to disable log messages or redirect them to another channel.
- Redirect channel output to private message or user notice, helps reduces spam.
- Text based configuration file; no fumbling with Tcl syntax errors.
- Module system; reducing code bloat by allowing users to add and remove functionality.
- Extensive theming and value formatting; the bot will convert duration, size, speed,
  and time values automatically.
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

2. If you checked out a revision from the Subversion repository:
   - Rename the AlcoBot-ioFTPD.conf file to AlcoBot.conf if you're using ioFTPD.
   - Rename the AlcoBot-glFTPD.conf file to AlcoBot.conf if you're using glFTPD.

3. Configure AlcoBot.conf to your liking. Read the comments carefully!

4. Add the following to your eggdrop.conf:
   source AlcoBot/AlcoBot.tcl

5. Rehash or restart Eggdrop.


################################################################################
# 3. Bugs and Comments                                                         #
################################################################################

   If you have any problems with this script; or you found a bug, spelling
mistake or grammar error, please report it to the team. If it's a technical
issue, be sure you can reproduce the problem, so we can find a solution.

IniCom Forum:
http://www.inicom.net/forum/forumdisplay.php?f=157

Issue Tracker:
http://www.alcoholicz.com/newticket

Website:
http://www.alcoholicz.com


################################################################################
# 4. License                                                                   #
################################################################################

   See the "license.txt" file for details.
