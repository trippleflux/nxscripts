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
 5. Configuration
 6. Usage
 7. Upgrading
 8. Changelog
 8. TODO
10. Bugs or Comments
11. License


##############################################################################
# 1. Features                                                                #
##############################################################################

- Easily customizable.
- Extra site help commands can be simply added by creating a file.
- Coded in TCL which offers very fast speeds and customizable output.

##############################################################################
# 2. Notes                                                                   #
##############################################################################

- Thanks to dark0n3 for such a great FTPD.
- Thanks to WarC for ioA and for his manual layout (which I copied =P).


##############################################################################
# 3. Requirements                                                            #
##############################################################################

   nxHelp should only be installed on Windows NT/2000/XP/2003 any other
operating system is unsupported. You must be using ioFTPD v5.6.3r or later
with the fixed TCL DLL. nxHelp was tested on v5.8.5r so I cannot guarantee it
will work on any other versions, although it should.


##############################################################################
# 4. Installation                                                            #
##############################################################################

1. Copy the nxHelp directory to ioFTPD\scripts\ or where ever your
   ioFTPD is installed.

2. Configure the nxHelp/nxHelp.tcl file, the configuration is explained
   in that file.

3. Add the following to your ioFTPD.ini

[FTP_Custom_Commands]
help	= TCL ..\scripts\nxHelp\nxHelp.tcl

[Ftp-SITE-Permissions]
help		= *

4. Now rehash ioFTPD for changes to take effect.

5. And you're done.


##############################################################################
# 5. Configuration                                                           #
##############################################################################

help(files_help)	Path to help files.
help(files_site)	Path to site command help files.
help(permission)	Flag permissions for related help files. (Only show
                        siteop commands to siteops etc.)


##############################################################################
# 6. Usage                                                                   #
##############################################################################

Note: <param> = Required
      [param] = Optional

- site help [command]
      Lists all available commands with a breif description.
      If the command parameter is specified, extended help on a command will be shown.
      Example: site help
               site help ginfo
               site help kill


##############################################################################
# 7. Upgrading                                                               #
##############################################################################

  1.01 -> 1.10 - Replace nxHelp.itcl and reconfigure it.
               - Replace all nxHelp/site/*.site files.

  1.00 -> 1.01 - Replace all help/site files.


##############################################################################
# 8. Changelog                                                               #
##############################################################################

  1.10 - Added: More site help topics relating to ioA.
         Change: Cleaned up the code.
         Change: More configurable now.

  1.01 - Minor changes.

  1.00 - Initial release.


##############################################################################
# 9. TODO                                                                    #
##############################################################################

 - Add help commands for scripts. (ioBanana, ioA etc.)


##############################################################################
# 10. Bugs or Comments                                                        #
##############################################################################

   If you have problems with my scripts or you think you've found a bug, please
report it to me. Be sure you can reproduce the error/bug so I can find what
caused it. I can be reached on IRC, #ioFTPD on EFnet, or send a PM on the
ioFTPD forums. Your feedback and bug reports are appreciated.


##############################################################################
# 11. License                                                                #
##############################################################################

   The package is available "as is" and the author does not take any
responsibility for whatever malfunction to software and hardware which
may derive from its use.
