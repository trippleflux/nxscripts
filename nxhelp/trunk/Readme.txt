##############################################################################
#                                                                            #
#                      nxHelp - Help Script for ioFTPD                       #
#                                                                            #
##############################################################################
# Author: neoxed (EFnet: #ioFTPD)                                            #
##############################################################################

Topics:
 1. Features
 2. Notes
 3. Requirements
 4. Installation
 5. Usage
 6. Upgrading
 7. Changes
 8. Bugs or Comments
 9. License


##############################################################################
# 1. Features                                                                #
##############################################################################

- Easily extendable and customizable.
- Additional help topics can be added by simply creating a text file.


##############################################################################
# 2. Notes                                                                   #
##############################################################################

- Thanks to dark0n3 for such a great FTPD.
- Thanks to WarC for ioA and for his manual layout (which I copied =P).


##############################################################################
# 3. Requirements                                                            #
##############################################################################

   nxHelp should only be installed on Windows NT/2000/XP/2003, any other
operating system is unsupported. You must be using ioFTPD v5.6.3r or later
with the threaded Tcl DLL. nxHelp was tested on v5.8.x so I cannot guarantee
it will work on other versions.


##############################################################################
# 4. Installation                                                            #
##############################################################################

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


##############################################################################
# 5. Usage                                                                   #
##############################################################################

Note: <param> = Required
      [param] = Optional

- SITE HELP [command]
      Lists all available commands with a brief description.
      If the command parameter is specified, extended help on a command will be shown.
      Example: site help
               site help ginfo
               site help kill


##############################################################################
# 6. Upgrading                                                               #
##############################################################################

  1.1.0 -> 2.0.0 - Replace nxHelp.tcl and reconfigure it.
                 - Replace all files in nxHelp\help\ and nxHelp\site\.

  1.0.1 -> 1.1.0 - Replace nxHelp.tcl and reconfigure it.
                 - Replace all files in nxHelp\site\.

  1.0.0 -> 1.0.1 - Replace all files in nxHelp\help\ and nxHelp\site\.


##############################################################################
# 7. Changes                                                                 #
##############################################################################

  2.0.0 - Code changes, moved all variables and and procedures into a namespace.

  1.1.0 - Added: More site help topics relating to ioA.
          Change: Minor code clean ups.

  1.0.1 - Minor changes.

  1.0.0 - Initial release.


##############################################################################
# 8. Bugs or Comments                                                        #
##############################################################################

   If you have problems with my scripts or you think you've found a bug, please
report it to me. Be sure you can reproduce the error/bug so I can find what
caused it. I can be reached on IRC, #ioFTPD on EFnet, or send a PM on the
ioFTPD forums. Your feedback and bug reports are appreciated.


##############################################################################
# 9. License                                                                 #
##############################################################################

   The package is available "as is" and the author does not take any
responsibility for whatever malfunction to software and hardware which
may derive from its use.
