################################################################################
#                      nxMyDB - MySQL Database for ioFTPD                      #
#                     Written by neoxed (neoxed@gmail.com)                     #
################################################################################

Topics:
 1. Information
 2. Installation
 3. Configuration
   a) Options
   b) yaSSL Cipher Suites
   c) OpenSSL Cipher Suites
 4. FAQ
 5. Bugs and Comments
 6. License

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
# 2. Installation                                                              #
################################################################################

1. Create a MySQL database and import the schema.sql file.

   mysql -u root -p -h 192.168.1.200 -e "CREATE DATABASE ioftpd"
   mysql -u root -p -h 192.168.1.200 -D ioftpd < schema.sql

2. Copy the nxmydb.dll file to ioFTPD\modules\.

3. Copy the libmysql.dll file to ioFTPD\system\.

4. Add the following options to your ioFTPD.ini file:

[Modules]
GroupModule     = ..\modules\nxmydb.dll
UserModule      = ..\modules\nxmydb.dll

[nxMyDB]
Host            = localhost     # MySQL Server host
Port            = 3306          # MySQL Server port
User            = user          # MySQL Server username
Password        = pass          # MySQL Server password
Database        = ioftpd        # Database name
Refresh         = 60            # Seconds between each database refresh
Compression     = True          # Use compression for the server connection

5. Adjust these options as required. There are several other options to enable
   SSL encryption and fine-tune the connection pool. For a list of available
   options, see the "Configuration" section of this manual.

6. When configuring SSL, you will have to setup the certificate authority on the
   server, as well as generate/sign certificates for connecting clients. For more
   information on this, visit:

   http://dev.mysql.com/doc/refman/5.0/en/secure-using-ssl.html
   http://www.navicat.com/ssl_tutorial.php

   I will NOT assist you with this; direct any questions about MySQL Server and SSL
   to the appropriate places (e.g. MySQL's mailing list or a MySQL discussion board).

7. Restart ioFTPD for the changes to take effect.

################################################################################
# 3. Configuration                                                             #
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

  Refresh
    - Seconds between each database refresh (synchronizes users and groups)
    - Set to zero if the database is not shared with more than one server
    - Default: 0

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
# 4. FAQ                                                                       #
################################################################################

Q: What does "nxMyDB: Unable to connect to server: SSL connection error" mean?
A: SSL is configured incorrectly on either the client or server.

################################################################################
# 5. Bugs and Comments                                                         #
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
# 6. License                                                                   #
################################################################################

   See the "license.txt" file for details.
