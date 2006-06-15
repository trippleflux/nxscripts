################################################################################
#                      nxMyDB - MySQL Database for ioFTPD                      #
################################################################################

Topics:
 1. Information
 2. Configuration
 3. Installation
 4. Bugs and Comments
 5. License

################################################################################
# 1. Information                                                               #
################################################################################

################################################################################
# 2. Configuration                                                             #
################################################################################

   Explanation of options available to nxMyDB, a list of cipher groups, and a
table of individual ciphers.

  ############################################################
  # Options                                                  #
  ############################################################

  Host            - MySQL Server host
  Port            - MySQL Server port
  User            - MySQL Server username
  Password        - MySQL Server password
  Database        - Database name

  Compression     - Use compression for the server connection
  SSL_Enable      - Use SSL encryption for the server connection
  SSL_Ciphers     - List of allowable ciphers to use for SSL encryption
  SSL_Cert_File   - Path to the certificate file
  SSL_Key_File    - Path to the key file
  SSL_CA_File     - Path to the certificate authority file
  SSL_CA_Path     - Path to the directory containg CA certificates

  Pool_Minimum    - Minimum number of sustained connections (must be greater than zero)
  Pool_Average    - Average number of sustained connections (usually slightly more than minimum)
  Pool_Maximum    - Maximum number of sustained connections (usually double the average)
  Pool_Expiration - Seconds until a connection expires (usually less than MySQL's interactive_timeout)
  Pool_Timeout    - Seconds to wait for a connection to become available

  ############################################################
  # Cipher Groups                                            #
  ############################################################

  Cipher groups can be used instead of listing individual ciphers.

  ALL    - All ciphers suites except the eNULL ciphers which must be explicitly enabled.
  HIGH   - High encryption cipher suites, currently those with key lengths larger than 128 bits.
  MEDIUM - Medium encryption cipher suites, currently those using 128 bit encryption.
  LOW    - Low encryption cipher suites, currently those using 64 or 56 bit encryption algorithms.

  http://www.openssl.org/docs/apps/ciphers.html#CIPHER_STRINGS

  ############################################################
  # Cipher Names                                             #
  ############################################################

  Individual cipher names and their description (taken from "openssl ciphers -tls1 -v").

  ---------------------------------------------------------------------------------
   Cipher Name                 | Protocols   | Kx        | Au  | Encryption | Mac
  ---------------------------------------------------------------------------------
   DHE-RSA-AES256-SHA          | SSLv3 TLSv1 | DH        | RSA | AES(256)   | SHA1
   DHE-DSS-AES256-SHA          | SSLv3 TLSv1 | DH        | DSS | AES(256)   | SHA1
   AES256-SHA                  | SSLv3 TLSv1 | RSA       | RSA | AES(256)   | SHA1
   EDH-RSA-DES-CBC3-SHA        | SSLv3 TLSv1 | DH        | RSA | 3DES(168)  | SHA1
   EDH-DSS-DES-CBC3-SHA        | SSLv3 TLSv1 | DH        | DSS | 3DES(168)  | SHA1
   DES-CBC3-SHA                | SSLv3 TLSv1 | RSA       | RSA | 3DES(168)  | SHA1
   DHE-RSA-AES128-SHA          | SSLv3 TLSv1 | DH        | RSA | AES(128)   | SHA1
   DHE-DSS-AES128-SHA          | SSLv3 TLSv1 | DH        | DSS | AES(128)   | SHA1
   AES128-SHA                  | SSLv3 TLSv1 | RSA       | RSA | AES(128)   | SHA1
   IDEA-CBC-SHA                | SSLv3 TLSv1 | RSA       | RSA | IDEA(128)  | SHA1
   DHE-DSS-RC4-SHA             | SSLv3 TLSv1 | DH        | DSS | RC4(128)   | SHA1
   RC4-SHA                     | SSLv3 TLSv1 | RSA       | RSA | RC4(128)   | SHA1
   RC4-MD5                     | SSLv3 TLSv1 | RSA       | RSA | RC4(128)   | MD5
   EXP1024-DHE-DSS-DES-CBC-SHA | SSLv3 TLSv1 | DH(1024)  | DSS | DES(56)    | SHA1
   EXP1024-DES-CBC-SHA         | SSLv3 TLSv1 | RSA(1024) | RSA | DES(56)    | SHA1
   EXP1024-RC2-CBC-MD5         | SSLv3 TLSv1 | RSA(1024) | RSA | RC2(56)    | MD5
   EDH-RSA-DES-CBC-SHA         | SSLv3 TLSv1 | DH        | RSA | DES(56)    | SHA1
   EDH-DSS-DES-CBC-SHA         | SSLv3 TLSv1 | DH        | DSS | DES(56)    | SHA1
   DES-CBC-SHA                 | SSLv3 TLSv1 | RSA       | RSA | DES(56)    | SHA1
   EXP1024-DHE-DSS-RC4-SHA     | SSLv3 TLSv1 | DH(1024)  | DSS | RC4(56)    | SHA1
   EXP1024-RC4-SHA             | SSLv3 TLSv1 | RSA(1024) | RSA | RC4(56)    | SHA1
   EXP1024-RC4-MD5             | SSLv3 TLSv1 | RSA(1024) | RSA | RC4(56)    | MD5
   EXP-EDH-RSA-DES-CBC-SHA     | SSLv3 TLSv1 | DH(512)   | RSA | DES(40)    | SHA1
   EXP-EDH-DSS-DES-CBC-SHA     | SSLv3 TLSv1 | DH(512)   | DSS | DES(40)    | SHA1
   EXP-DES-CBC-SHA             | SSLv3 TLSv1 | RSA(512)  | RSA | DES(40)    | SHA1
   EXP-RC2-CBC-MD5             | SSLv3 TLSv1 | RSA(512)  | RSA | RC2(40)    | MD5
   EXP-RC4-MD5                 | SSLv3 TLSv1 | RSA(512)  | RSA | RC4(40)    | MD5

################################################################################
# 3. Installation                                                              #
################################################################################

1. Add the following configuration section to your ioFTPD.ini:

[nxMyDB]
Host            = localhost     # MySQL Server host
Port            = 3306          # MySQL Server port
User            = user          # MySQL Server username
Password        = pass          # MySQL Server password
Database        = ioftpd        # Database name

# Connection type
Compression     = True          # Use compression for the server connection
SSL_Enable      = True          # Use SSL encryption for the server connection
SSL_Ciphers     = HIGH          # List of allowable ciphers to use for SSL encryption

# Connection pools
Pool_Minimum    = 2             # Minimum number of sustained connections (must be greater than zero)
Pool_Average    = 3             # Average number of sustained connections (usually slightly more than minimum)
Pool_Maximum    = 5             # Maximum number of sustained connections (usually double the average)
Pool_Expiration = 7200          # Seconds until a connection expires (usually less than MySQL's interactive_timeout)
Pool_Timeout    = 5             # Seconds to wait for a connection to become available

################################################################################
# 4. Bugs and Comments                                                         #
################################################################################

   If you have any problems with this module, whether it is a bug, spelling
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
