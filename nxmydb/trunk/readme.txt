################################################################################
#                      nxMyDB - MySQL Database for ioFTPD                      #
#                     Written by neoxed (neoxed@gmail.com)                     #
################################################################################

Topics:
 1. Information
 2. Requirements
 3. Installation
 4. Upgrading
 5. Configuration
   a) Options
   b) yaSSL Cipher Suites
   c) OpenSSL Cipher Suites
 6. FAQ
 7. Bugs and Comments
 8. License

################################################################################
# 1. Information                                                               #
################################################################################

    nxMyDB is a user and group database module for ioFTPD. It utilizes MySQL as
its database storage backend, to share users and groups amongst multiple ioFTPD
servers. nxMyDB also includes features such as:

- Central location for storing users and groups
- Database connection pool; reduces time spent waiting during high user activity
- Reading Default.User and Default.Group for newly created users and groups
- Support for compressing the server traffic
- Support for encrypting the server traffic using SSL
- User files and group files are updated regularly

################################################################################
# 2. Requirements                                                              #
################################################################################

- Windows operating system

- ioFTPD v6.9

- MySQL Server v5.1

################################################################################
# 3. Installation                                                              #
################################################################################

1. Create a MySQL database and import the schema.sql file.

   mysql -u root -p -h 192.168.1.1 -e "CREATE DATABASE ioftpd"
   mysql -u root -p -h 192.168.1.1 -D ioftpd --delimiter=$ < schema.sql

2. Create a MySQL user to access the "ioftpd" database.

3. Copy the nxmydb.dll file to ioFTPD\modules\.

4. Copy the libmysql.dll file to ioFTPD\system\.

5. Add the following options to your ioFTPD.ini file:

[Scheduler]
nxMyDBPurge     = 0 0 * * NXMYDB purge

[Modules]
EventModule     = ..\modules\nxmydb.dll
GroupModule     = ..\modules\nxmydb.dll
UserModule      = ..\modules\nxmydb.dll

[Events]
OnServerStart   = NXMYDB start
OnServerStop    = NXMYDB stop

[nxMyDB]
Host            = localhost     # MySQL Server host
Port            = 3306          # MySQL Server port
User            = user          # MySQL Server username
Password        = pass          # MySQL Server password
Database        = ioftpd        # Database name
Compression     = True          # Use compression for the server connection
Sync            = True          # Synchronization of users and groups
Sync_First      = 30            # Seconds until the first full synchronization
Sync_Interval   = 60            # Seconds between each incremental synchronization

6. Adjust these options as required. There are several other options to enable
   SSL encryption and fine-tune the connection pool. For a list of available
   options, see the "Configuration" section of this manual.

7. When configuring SSL, you will have to setup the certificate authority on the
   server, as well as generate/sign certificates for connecting clients. For more
   information on this, visit:

   http://dev.mysql.com/doc/refman/5.1/en/secure-using-ssl.html
   http://www.navicat.com/ssl_tutorial.php

   I will NOT assist you with this; direct any questions about MySQL Server and SSL
   to the appropriate places (e.g. MySQL's mailing list or a MySQL discussion board).

8. Restart ioFTPD for the changes to take effect.

################################################################################
# 4. Upgrading                                                                 #
################################################################################

v1.0 -> v2.0
 - Add scheduler entry to your ioFTPD configuration.
 - Replace nxmydb.dll and libmysql.dll files.
 - Upgrade database schema using v1.0-to-v2.0.sql (see file for instructions).

################################################################################
# 5. Configuration                                                             #
################################################################################

    Explanation of options available to nxMyDB and a list of cipher suites
supported by OpenSSL/yaSSL.

  ############################################################
  # a) Options                                               #
  ############################################################

  If any option is left undefined, the default value is used.

  Host
    - MySQL Server host
    - Default: localhost

  Port
    - MySQL Server port
    - Default: 3306

  User
    - MySQL Server username
    - Default: MySQL's default user

  Password
    - MySQL Server password
    - Default: MySQL's default password

  Database
    - Database name
    - Default: MySQL's default database

  Log_Level
    - Log verbosity level
    - Value 0 for off
    - Value 1 for errors
    - Value 2 for errors and warnings
    - Value 3 for errors, warnings, and information
    - Default: 1

  Sync
    - Synchronization of users and groups
    - Set to "true" if the database is shared with more than one server
    - Default: false

  Sync_First
    - Seconds until the first full database synchronization
    - Only performed after initialization
    - Default: 30

  Sync_Interval
    - Seconds between each incremental database synchronization
    - Default: 60

  Sync_Purge
    - Seconds after which to purge entries in the "changes" tables
    - This should be substantially larger than the Sync_Interval
    - Default: Sync_Interval x 100

  Compression
    - Use compression for the server connection
    - Default: false

  SSL_Enable
    - Use SSL encryption for the server connection
    - Default: false

  SSL_Ciphers
    - List of allowable ciphers to use with SSL encryption
    - I recommend using DHE-RSA-AES256-SHA
    - Default: null

  SSL_Cert_File
    - Path to the certificate file
    - Default: null

  SSL_Key_File
    - Path to the key file
    - Default: null

  SSL_CA_File
    - Path to the certificate authority file
    - Default: null

  SSL_CA_Path
    - Path to the directory containing CA certificates
    - Default: null

  Lock_Expire
    - Seconds until a lock expires
    - Default: 60 (1 minute)

  Lock_Timeout
    - Seconds to wait for a lock to become available
    - Default: 5

  Pool_Minimum
    - Minimum number of sustained connections
    - Must be greater than zero
    - Default: 1

  Pool_Average
    - Average number of sustained connections (usually slightly more than minimum)
    - Default: Pool_Minimum + 1

  Pool_Maximum
    - Maximum number of sustained connections (usually double the average)
    - Default: Pool_Average * 2

  Pool_Check
    - Seconds until an idle connection is checked
    - Default: 60 (1 minute)

  Pool_Expire
    - Seconds until a connection expires
    - Should be less than MySQL's "interactive_timeout" value
    - Default: 3600 (1 hour)

  Pool_Timeout
    - Seconds to wait for a connection to become available
    - Default: 5

  ############################################################
  # b) yaSSL Cipher Suites                                   #
  ############################################################

  MySQL's official Windows binaries are built using the yaSSL library.

  -------------------------------------------------------------------------------
   Cipher Name                |  Protocols  | Key Xchg | Auth | Encryption | Mac
  -------------------------------------------------------------------------------
  AES128-RMD                  | SSLv3 TLSv1 | RSA      | RSA  |  AES 128   | RMD
  AES128-SHA                  | SSLv3 TLSv1 | RSA      | RSA  |  AES 128   | SHA1
  AES256-RMD                  | SSLv3 TLSv1 | RSA      | RSA  |  AES 256   | RMD
  AES256-SHA                  | SSLv3 TLSv1 | RSA      | RSA  |  AES 256   | SHA1
  DES-CBC-SHA                 | SSLv3 TLSv1 | RSA      | RSA  |  DES       | SHA1
  DES-CBC3-RMD                | SSLv3 TLSv1 | RSA      | RSA  | 3DES 168   | RMD
  DES-CBC3-SHA                | SSLv3 TLSv1 | RSA      | RSA  | 3DES 168   | SHA1
  DHE-DSS-AES128-RMD          | SSLv3 TLSv1 | DH       | DSS  |  AES 128   | RMD
  DHE-DSS-AES128-SHA          | SSLv3 TLSv1 | DH       | DSS  |  AES 128   | SHA1
  DHE-DSS-AES256-RMD          | SSLv3 TLSv1 | DH       | DSS  |  AES 256   | RMD
  DHE-DSS-AES256-SHA          | SSLv3 TLSv1 | DH       | DSS  |  AES 256   | SHA1
  DHE-DSS-DES-CBC3-RMD        | SSLv3 TLSv1 | DH       | DSS  | 3DES 168   | RMD
  DHE-RSA-AES128-RMD          | SSLv3 TLSv1 | DH       | RSA  |  AES 128   | RMD
  DHE-RSA-AES128-SHA          | SSLv3 TLSv1 | DH       | RSA  |  AES 128   | SHA1
  DHE-RSA-AES256-RMD          | SSLv3 TLSv1 | DH       | RSA  |  AES 256   | RMD
  DHE-RSA-AES256-SHA          | SSLv3 TLSv1 | DH       | RSA  |  AES 256   | SHA1
  DHE-RSA-DES-CBC3-RMD        | SSLv3 TLSv1 | DH       | RSA  | 3DES 168   | RMD
  EDH-DSS-DES-CBC-SHA         | SSLv3 TLSv1 | DH       | DSS  |  DES       | SHA1
  EDH-DSS-DES-CBC3-SHA        | SSLv3 TLSv1 | DH       | DSS  | 3DES 168   | SHA1
  EDH-RSA-DES-CBC-SHA         | SSLv3 TLSv1 | DH       | RSA  |  DES       | SHA1
  EDH-RSA-DES-CBC3-SHA        | SSLv3 TLSv1 | DH       | RSA  | 3DES 168   | SHA1
  RC4-MD5                     | SSLv3 TLSv1 | RSA      | RSA  |  RC4       | MD5
  RC4-SHA                     | SSLv3 TLSv1 | RSA      | RSA  |  RC4       | SHA1

  ############################################################
  # c) OpenSSL Cipher Suites                                 #
  ############################################################

  Cipher strings can be used instead of listing individual ciphers.

  ALL    - All ciphers suites, except the eNULL ciphers which must be explicitly enabled.
  HIGH   - High encryption cipher suites, currently those with key lengths larger than 128 bits.
  MEDIUM - Medium encryption cipher suites, currently those using 128 bit encryption.
  LOW    - Low encryption cipher suites, currently those using 64 or 56 bit encryption algorithms.

  http://www.openssl.org/docs/apps/ciphers.html#CIPHER_STRINGS

  Individual ciphers and their description (obtained from "openssl ciphers -tls1 -v").

  -------------------------------------------------------------------------------
   Cipher Name                |  Protocols  | Key Xchg | Auth | Encryption | Mac
  -------------------------------------------------------------------------------
  AES128-SHA                  | SSLv3 TLSv1 | RSA      | RSA  |  AES 128   | SHA1
  AES256-SHA                  | SSLv3 TLSv1 | RSA      | RSA  |  AES 256   | SHA1
  DES-CBC-SHA                 | SSLv3 TLSv1 | RSA      | RSA  |  DES 56    | SHA1
  DES-CBC3-SHA                | SSLv3 TLSv1 | RSA      | RSA  | 3DES 168   | SHA1
  DHE-DSS-AES128-SHA          | SSLv3 TLSv1 | DH       | DSS  |  AES 128   | SHA1
  DHE-DSS-AES256-SHA          | SSLv3 TLSv1 | DH       | DSS  |  AES 256   | SHA1
  DHE-DSS-RC4-SHA             | SSLv3 TLSv1 | DH       | DSS  |  RC4 128   | SHA1
  DHE-RSA-AES128-SHA          | SSLv3 TLSv1 | DH       | RSA  |  AES 128   | SHA1
  DHE-RSA-AES256-SHA          | SSLv3 TLSv1 | DH       | RSA  |  AES 256   | SHA1
  EDH-DSS-DES-CBC-SHA         | SSLv3 TLSv1 | DH       | DSS  |  DES 56    | SHA1
  EDH-DSS-DES-CBC3-SHA        | SSLv3 TLSv1 | DH       | DSS  | 3DES 168   | SHA1
  EDH-RSA-DES-CBC-SHA         | SSLv3 TLSv1 | DH       | RSA  |  DES 56    | SHA1
  EDH-RSA-DES-CBC3-SHA        | SSLv3 TLSv1 | DH       | RSA  | 3DES 168   | SHA1
  EXP-DES-CBC-SHA             | SSLv3 TLSv1 | RSA      | RSA  |  DES 40    | SHA1
  EXP-EDH-DSS-DES-CBC-SHA     | SSLv3 TLSv1 | DH       | DSS  |  DES 40    | SHA1
  EXP-EDH-RSA-DES-CBC-SHA     | SSLv3 TLSv1 | DH       | RSA  |  DES 40    | SHA1
  EXP-RC2-CBC-MD5             | SSLv3 TLSv1 | RSA      | RSA  |  RC2 40    | MD5
  EXP-RC4-MD5                 | SSLv3 TLSv1 | RSA      | RSA  |  RC4 40    | MD5
  EXP1024-DES-CBC-SHA         | SSLv3 TLSv1 | RSA      | RSA  |  DES 56    | SHA1
  EXP1024-DHE-DSS-DES-CBC-SHA | SSLv3 TLSv1 | DH       | DSS  |  DES 56    | SHA1
  EXP1024-DHE-DSS-RC4-SHA     | SSLv3 TLSv1 | DH       | DSS  |  RC4 56    | SHA1
  EXP1024-RC2-CBC-MD5         | SSLv3 TLSv1 | RSA      | RSA  |  RC2 56    | MD5
  EXP1024-RC4-MD5             | SSLv3 TLSv1 | RSA      | RSA  |  RC4 56    | MD5
  EXP1024-RC4-SHA             | SSLv3 TLSv1 | RSA      | RSA  |  RC4 56    | SHA1
  IDEA-CBC-SHA                | SSLv3 TLSv1 | RSA      | RSA  | IDEA 128   | SHA1
  RC4-MD5                     | SSLv3 TLSv1 | RSA      | RSA  |  RC4 128   | MD5
  RC4-SHA                     | SSLv3 TLSv1 | RSA      | RSA  |  RC4 128   | SHA1

################################################################################
# 6. FAQ                                                                       #
################################################################################

Q: What does "nxMyDB: Unable to connect to server: SSL connection error" mean?
A: SSL is configured incorrectly on either the client or server.

Q: How do I enable debug logging?
A: Set "Log_Level" to "3" in the ioFTPD.ini file and restart ioFTPD.

Q: Multiple servers are not synchronizing with the database.
A: If the servers are not synchronizing, it's usually due to a configuration error.
   - Enable debug logging and check the debug log for clues.
   - Check that "Sync" is set to "True" in the ioFTPD.ini file.
   - Check that the start and stop events exist under [Events] in the ioFTPD.ini file.

Q: What does "Group locking failed." or "User locking failed." mean?
A: This error can be caused by a number of failures, try the following.
   - Enable debug logging and check the debug log for clues.
   - Check that the module is able to establish a connection with the MySQL server.
   - Check that all tables and stored procedures exist.
   - Check that the MySQL user account has execute privileges (required for stored procedures).
   - Check that the MySQL server is the latest v5.1 version.

################################################################################
# 7. Bugs and Comments                                                         #
################################################################################

   If you have ideas for improvements or are experiencing problems with this
script, please do not hesitate to contact me. If your problem is a technical
issue (i.e. a crash or operational defect), be sure to provide me with the steps
necessary to reproduce it.

FlashFXP Forum:
http://www.flashfxp.com/forum/forumdisplay.php?f=68

IRC Network:
neoxed in #ioFTPD at EFnet

E-mail:
neoxed@gmail.com

################################################################################
# 8. License                                                                   #
################################################################################

   See the "license.txt" file for details.
