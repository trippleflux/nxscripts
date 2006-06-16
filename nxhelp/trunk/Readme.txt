################################################################################
#                       nxHelp - Help Script for ioFTPD                        #
#                     Written by neoxed (neoxed@gmail.com)                     #
################################################################################

Topics:
 1. Features
 2. Requirements
 3. Installation
 4. Usage
 5. Upgrading
 6. Changes
 7. Bugs or Comments
 8. License

################################################################################
# 1. Features                                                                  #
################################################################################

- Easily extendable and customizable.
- Additional help topics can be added by simply creating a text file.

################################################################################
# 2. Requirements                                                              #
################################################################################

   nxHelp should only be installed on Windows NT/2000/XP/2003, any other
operating system is unsupported. You must be using ioFTPD v5.6.3r or later
with the threaded Tcl DLL. nxHelp was tested on v5.8.x so I cannot guarantee
it will work on other versions.

################################################################################
# 3. Installation                                                              #
################################################################################

1. Copy the nxHelp directory to ioFTPD\scripts\ or where ever your
   ioFTPD is installed.

2. Configure the nxHelp/nxHelp.tcl file, the configuration is explained
   in that file.

3. Add the following to your ioFTPD.ini

[FTP_Custom_Commands]
help    = Tcl ..\scripts\nxHelp\nxHelp.tcl

[Ftp-SITE-Permissions]
help    = *

4. Rehash ioFTPD for changes to take effect.

5. Finished.

################################################################################
# 4. Usage                                                                     #
################################################################################

Note: <param> = Required
      [param] = Optional

- SITE HELP [command]
      Lists all available commands with a brief description.
      If the command parameter is specified, extended help on a command will be shown.
      Example: site help
               site help ginfo
               site help kill

################################################################################
# 5. Upgrading                                                                 #
################################################################################

  1.1.0 -> 2.0.0 - Replace nxHelp.tcl and reconfigure it.
                 - Replace all files in nxHelp\help\ and nxHelp\site\.

  1.0.1 -> 1.1.0 - Replace nxHelp.tcl and reconfigure it.
                 - Replace all files in nxHelp\site\.

  1.0.0 -> 1.0.1 - Replace all files in nxHelp\help\ and nxHelp\site\.

################################################################################
# 6. Changes                                                                   #
################################################################################

  2.0.0 - Code changes, moved all variables and and procedures into a namespace.

  1.1.0 - Added: More site help topics relating to ioA.
          Change: Minor code clean ups.

  1.0.1 - Minor changes.

  1.0.0 - Initial release.

################################################################################
# 7. Bugs or Comments                                                          #
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
# 8. License                                                                   #
################################################################################

   See the "license.txt" file for details.
