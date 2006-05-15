################################################################################
#                   nxSDK - ioFTPD Software Development Kit                    #
################################################################################

Topics:
 1. Information
 2. Components
 3. Developing
 4. Bugs and Comments
 5. License

################################################################################
# 1. Information                                                               #
################################################################################

   nxSDK is a development kit for software developers writing shared memory
applications for ioFTPD in C and C++. It provides a high-level interface to
all of the shared memory functions implemented in ioFTPD, such as:

- Creating, renaming, and deleting users and groups.
- Modifying user-file and group-file data structures.
- Resolving group IDs to group names, and vice versa.
- Resolving user IDs to user names, and vice versa.
- Retrieving information about online users.
- Kicking online users by their connection ID or user ID.
- Flushing the directory cache in specific directories.
- Reading and writing VFS ownership (uid/gid) and permissions (chmod).

################################################################################
# 2. Components                                                                #
################################################################################

  ############################################################
  # bin\                                                     #
  ############################################################

  Application binaries and dynamic libraries.

  kick.exe
    - Example application, demonstrates how to kick online users.
    - Source code is located in src\kick\.

  nxsdk.dll
    - SDK library, contains all the shared memory functions.
    - Source code is located in src\lib\.

  resolve.exe
    - Example application, demonstrates how to resolve users and groups.
    - Source code is located in src\resolve\.

  who.exe
    - Example application, demonstrates how to display online users.
    - Source code is located in src\kick\.

  ############################################################
  # doc\                                                     #
  ############################################################

  SDK documentation.

  functions.htm
    - Documents public data structures in nxSDK.

  structures.htm
    - Documents exported functions in nxSDK.

  ############################################################
  # include\                                                 #
  ############################################################

  SDK header files.

  nxsdk.h
    - Header file, defines all public data structures and exported functions.

  ############################################################
  # lib\                                                     #
  ############################################################

  SDK libraries.

  nxsdk.lib
    - Dynamic library (dependency on nxsdk.dll).

  nxsdk-static.lib
    - Static libraries (no dependency on nxsdk.dll).

  ############################################################
  # src\                                                     #
  ############################################################

  SDK source code.

  kick\
    - Source code for the "kick" application.

  lib\
    - Source code for the library.

  resolve\
    - Source code for the "resolve" application.

  who\
    - Source code for the "who" application.

################################################################################
# 3. Developing                                                                #
################################################################################

   Quick introduction on how to use the SDK.

1. Include the required header files, in the following order.

   #include <windows.h>
   #include <time.h>
   #include <nxsdk.h>

2. Add nxsdk.lib, or nxsdk-static.lib, to your application's input libraries.

3. Refer to the documentation in doc\ for information about nxSDK's functions.

4. For examples see the "kick", "resolve", and "who" applications.

5. Start coding :P.

################################################################################
# 4. Bugs and Comments                                                         #
################################################################################

   If you have any problems with this script, whether it is a bug, spelling
mistake or grammatical error, please report it to me. If it is a technical issue,
make sure you can reproduce the problem and provide us the necessary steps.

IniCom Forum:
http://www.inicom.net/forum/forumdisplay.php?f=157

IRC Network:
neoxed in #ioFTPD at EFnet

E-mail:
neoxed@gmail.com

################################################################################
# 5. License                                                                   #
################################################################################

   See the "license.txt" file for details.
