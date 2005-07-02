#
# AlcoExt - Alcoholicz Tcl extension.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# File Name:
#   makefile
#
# Author:
#   neoxed (neoxed@gmail.com) June 12, 2005
#
# Abstract:
#   Build configuration file for NMAKE, or a compatible tool.
#
#   Usage:
#     nmake -f makefile [OPTION=1 ...]
#
#   Options:
#     DEBUG    - Symbolic debugging.
#     DEVPERF  - Compuware DevPartner performance analysis.
#     MEMDEBUG - Tcl memory debugging.
#     THREADS  - Threading support.
#

# PSDK Settings
APPVER      = 4.0
TARGETOS    = WINNT

# -------------------------------------------------------------------------

!if !exist("makefile")
!error You must run this makefile from the directory it is located in.
!endif

!include "../package.def"

# -------------------------------------------------------------------------

ROOT        = ..
PKG_SUFFIX  =
PKG_TOPDIR  = Release
PKG_OPTIONS = Release

PKG_VERSION_DOT = $(PKG_MAJOR).$(PKG_MINOR).$(PKG_PATCH)
PKG_VERSION_RES = $(PKG_MAJOR),$(PKG_MINOR),$(PKG_PATCH),$(PKG_STATUS)

PKG_CFLAGS  = "-DPACKAGE_NAME=\"$(PKG_NAME)\"" "-DPACKAGE_DESC=\"$(PKG_DESC)\"" "-DPACKAGE_VERSION=\"$(PKG_VERSION_DOT)\"" -DPACKAGE_VERSION_RES=$(PKG_VERSION_RES)
PKG_LFLAGS  = -nodefaultlib:msvcrt -opt:ref -opt:nowin98
LTC_CFLAGS  =
TCL_CFLAGS  = -DUSE_TCL_STUBS
ZLIB_CFLAGS = -DASMV -DASMINF

# -------------------------------------------------------------------------

NODEBUG=1

!ifdef DEBUG
!if $(DEBUG)
!message *** Option: Symbolic Debugging
PKG_SUFFIX  = $(PKG_SUFFIX)D
PKG_TOPDIR  = Debug
PKG_OPTIONS = Debug
!undef NODEBUG
!endif
!endif

!ifdef MEMDEBUG
!if $(MEMDEBUG)
!message *** Option: Memory Debugging
PKG_SUFFIX  = $(PKG_SUFFIX)M
PKG_OPTIONS = $(PKG_OPTIONS)Memory
TCL_CFLAGS  = $(TCL_CFLAGS) -DTCL_MEM_DEBUG=1
!endif
!endif

!ifdef THREADS
!if $(THREADS)
!message *** Option: Threads
PKG_SUFFIX  = $(PKG_SUFFIX)T
PKG_OPTIONS = $(PKG_OPTIONS)Threads
TCL_CFLAGS  = $(TCL_CFLAGS) -DTCL_THREADS=1
!endif
!endif

!include "win32.mak"

!ifdef DEVPERF
!if $(DEVPERF)
!message *** Option: DevPartner Performance Analysis
PKG_SUFFIX  = $(PKG_SUFFIX)P
PKG_OPTIONS = $(PKG_OPTIONS)Perf
CC      = NMCL /NMttOn
LINK    = NMLINK /NMttOn
!endif
!endif

# -------------------------------------------------------------------------

!if !exist(nmakehlp.exe)
!if [$(CC) -nologo -MT nmakehlp.c -link -subsystem:console > nul]
!endif
!endif

!if !defined(TCL_DIR)
!error Failed to find tcl.h; the TCL_DIR variable is incorrect.
!endif

TCL_DIR = $(TCL_DIR:/=\)
!if exist("$(TCL_DIR)\include\tcl.h")
TCL_H   = $(TCL_DIR)\include\tcl.h
!else
!error Failed to find tcl.h; the TCL_DIR variable is incorrect.
!endif

!if [nmakehlp -v $(TCL_H) ""] == 0
!include version.vc
!else
TCL_DOTVERSION  = 8.4
TCL_VERSION = $(TCL_DOTVERSION:.=)
!endif

TCL_SH       = "$(TCL_DIR)\bin\tclsh$(TCL_VERSION)$(SUFX).exe"
TCL_STUBLIB  = "$(TCL_DIR)\lib\tclstub$(TCL_VERSION).lib"
TCL_IMPLIB   = "$(TCL_DIR)\lib\tcl$(TCL_VERSION)$(SUFX).lib"
TCL_LIBRARY  = $(TCL_DIR)\lib
TCL_INCLUDES = $(TCL_DIR)\include

!message *** Tcl Version: $(TCL_DOTVERSION)
!message *** Tcl Stub: $(TCL_STUBLIB)

# -------------------------------------------------------------------------

!if "$(PKG_SUFFIX)" != ""
PKG_SUFFIX  = -$(PKG_SUFFIX)
!endif

PKG_FILE    = $(PKG_NAME)$(PKG_SUFFIX).dll
PKG_OUTFILE = $(PKG_TOPDIR)\$(PKG_FILE)
PKG_INTDIR  = $(PKG_TOPDIR)\$(PKG_OPTIONS)

INCLUDE_DIRS=\
    -I"$(TCL_INCLUDES)"\
    -I"$(ROOT)\libs\tomcrypt\headers"\
    -I"$(ROOT)\libs\zlib\headers"\
    -I"$(ROOT)\generic"\
    -I"$(ROOT)\win"

INTERMEDIATE_FILES=\
    $(PKG_INTDIR)\aes.obj\
    $(PKG_INTDIR)\anubis.obj\
    $(PKG_INTDIR)\blowfish.obj\
    $(PKG_INTDIR)\cast5.obj\
    $(PKG_INTDIR)\des.obj\
    $(PKG_INTDIR)\khazad.obj\
    $(PKG_INTDIR)\noekeon.obj\
    $(PKG_INTDIR)\rc2.obj\
    $(PKG_INTDIR)\rc5.obj\
    $(PKG_INTDIR)\rc6.obj\
    $(PKG_INTDIR)\safer.obj\
    $(PKG_INTDIR)\safer_tab.obj\
    $(PKG_INTDIR)\saferp.obj\
    $(PKG_INTDIR)\skipjack.obj\
    $(PKG_INTDIR)\twofish.obj\
    $(PKG_INTDIR)\xtea.obj\
    $(PKG_INTDIR)\hash_memory.obj\
    $(PKG_INTDIR)\md2.obj\
    $(PKG_INTDIR)\md4.obj\
    $(PKG_INTDIR)\md5.obj\
    $(PKG_INTDIR)\rmd128.obj\
    $(PKG_INTDIR)\rmd160.obj\
    $(PKG_INTDIR)\sha1.obj\
    $(PKG_INTDIR)\sha256.obj\
    $(PKG_INTDIR)\sha512.obj\
    $(PKG_INTDIR)\tiger.obj\
    $(PKG_INTDIR)\whirl.obj\
    $(PKG_INTDIR)\hmac_done.obj\
    $(PKG_INTDIR)\hmac_init.obj\
    $(PKG_INTDIR)\hmac_memory.obj\
    $(PKG_INTDIR)\hmac_process.obj\
    $(PKG_INTDIR)\base64_decode.obj\
    $(PKG_INTDIR)\base64_encode.obj\
    $(PKG_INTDIR)\burn_stack.obj\
    $(PKG_INTDIR)\crypt_argchk.obj\
    $(PKG_INTDIR)\crypt_cipher_descriptor.obj\
    $(PKG_INTDIR)\crypt_cipher_is_valid.obj\
    $(PKG_INTDIR)\crypt_find_cipher.obj\
    $(PKG_INTDIR)\crypt_find_cipher_any.obj\
    $(PKG_INTDIR)\crypt_find_cipher_id.obj\
    $(PKG_INTDIR)\crypt_find_hash.obj\
    $(PKG_INTDIR)\crypt_find_hash_any.obj\
    $(PKG_INTDIR)\crypt_find_hash_id.obj\
    $(PKG_INTDIR)\crypt_find_prng.obj\
    $(PKG_INTDIR)\crypt_hash_descriptor.obj\
    $(PKG_INTDIR)\crypt_hash_is_valid.obj\
    $(PKG_INTDIR)\crypt_prng_descriptor.obj\
    $(PKG_INTDIR)\crypt_prng_is_valid.obj\
    $(PKG_INTDIR)\crypt_register_cipher.obj\
    $(PKG_INTDIR)\crypt_register_hash.obj\
    $(PKG_INTDIR)\crypt_register_prng.obj\
    $(PKG_INTDIR)\crypt_unregister_cipher.obj\
    $(PKG_INTDIR)\crypt_unregister_hash.obj\
    $(PKG_INTDIR)\crypt_unregister_prng.obj\
    $(PKG_INTDIR)\error_to_string.obj\
    $(PKG_INTDIR)\pkcs_5_1.obj\
    $(PKG_INTDIR)\pkcs_5_2.obj\
    $(PKG_INTDIR)\zeromem.obj\
    $(PKG_INTDIR)\cbc_decrypt.obj\
    $(PKG_INTDIR)\cbc_done.obj\
    $(PKG_INTDIR)\cbc_encrypt.obj\
    $(PKG_INTDIR)\cbc_getiv.obj\
    $(PKG_INTDIR)\cbc_setiv.obj\
    $(PKG_INTDIR)\cbc_start.obj\
    $(PKG_INTDIR)\cfb_decrypt.obj\
    $(PKG_INTDIR)\cfb_done.obj\
    $(PKG_INTDIR)\cfb_encrypt.obj\
    $(PKG_INTDIR)\cfb_getiv.obj\
    $(PKG_INTDIR)\cfb_setiv.obj\
    $(PKG_INTDIR)\cfb_start.obj\
    $(PKG_INTDIR)\ctr_decrypt.obj\
    $(PKG_INTDIR)\ctr_done.obj\
    $(PKG_INTDIR)\ctr_encrypt.obj\
    $(PKG_INTDIR)\ctr_getiv.obj\
    $(PKG_INTDIR)\ctr_setiv.obj\
    $(PKG_INTDIR)\ctr_start.obj\
    $(PKG_INTDIR)\ecb_decrypt.obj\
    $(PKG_INTDIR)\ecb_done.obj\
    $(PKG_INTDIR)\ecb_encrypt.obj\
    $(PKG_INTDIR)\ecb_start.obj\
    $(PKG_INTDIR)\ofb_decrypt.obj\
    $(PKG_INTDIR)\ofb_done.obj\
    $(PKG_INTDIR)\ofb_encrypt.obj\
    $(PKG_INTDIR)\ofb_getiv.obj\
    $(PKG_INTDIR)\ofb_setiv.obj\
    $(PKG_INTDIR)\ofb_start.obj\
    $(PKG_INTDIR)\fortuna.obj\
    $(PKG_INTDIR)\rc4.obj\
    $(PKG_INTDIR)\rng_get_bytes.obj\
    $(PKG_INTDIR)\rng_make_prng.obj\
    $(PKG_INTDIR)\sober128.obj\
    $(PKG_INTDIR)\sprng.obj\
    $(PKG_INTDIR)\yarrow.obj\
    $(PKG_INTDIR)\adler32.obj\
    $(PKG_INTDIR)\compress.obj\
    $(PKG_INTDIR)\crc32.obj\
    $(PKG_INTDIR)\deflate.obj\
    $(PKG_INTDIR)\gvmat32.obj\
    $(PKG_INTDIR)\gvmat32c.obj\
    $(PKG_INTDIR)\gzio.obj\
    $(PKG_INTDIR)\infback.obj\
    $(PKG_INTDIR)\inffas32.obj\
    $(PKG_INTDIR)\inffast.obj\
    $(PKG_INTDIR)\inflate.obj\
    $(PKG_INTDIR)\inftrees.obj\
    $(PKG_INTDIR)\trees.obj\
    $(PKG_INTDIR)\uncompr.obj\
    $(PKG_INTDIR)\zutil.obj\
    $(PKG_INTDIR)\alcoExt.obj\
    $(PKG_INTDIR)\alcoCrypt.obj\
    $(PKG_INTDIR)\alcoEncoding.obj\
    $(PKG_INTDIR)\alcoUtil.obj\
    $(PKG_INTDIR)\alcoVolume.obj\
    $(PKG_INTDIR)\alcoZlib.obj\
    $(PKG_INTDIR)\alcoWinIoFtpd.obj\
    $(PKG_INTDIR)\alcoWinUtil.obj\
    $(PKG_INTDIR)\alcoWinVolume.obj\
    $(PKG_INTDIR)\alcoExt.res

OBJECT_FILES=\
    $(ROOT)\libs\tomcrypt\ciphers\aes\aes.obj\
    $(ROOT)\libs\tomcrypt\ciphers\anubis.obj\
    $(ROOT)\libs\tomcrypt\ciphers\blowfish.obj\
    $(ROOT)\libs\tomcrypt\ciphers\cast5.obj\
    $(ROOT)\libs\tomcrypt\ciphers\des.obj\
    $(ROOT)\libs\tomcrypt\ciphers\khazad.obj\
    $(ROOT)\libs\tomcrypt\ciphers\noekeon.obj\
    $(ROOT)\libs\tomcrypt\ciphers\rc2.obj\
    $(ROOT)\libs\tomcrypt\ciphers\rc5.obj\
    $(ROOT)\libs\tomcrypt\ciphers\rc6.obj\
    $(ROOT)\libs\tomcrypt\ciphers\safer\safer.obj\
    $(ROOT)\libs\tomcrypt\ciphers\safer\safer_tab.obj\
    $(ROOT)\libs\tomcrypt\ciphers\safer\saferp.obj\
    $(ROOT)\libs\tomcrypt\ciphers\skipjack.obj\
    $(ROOT)\libs\tomcrypt\ciphers\twofish\twofish.obj\
    $(ROOT)\libs\tomcrypt\ciphers\xtea.obj\
    $(ROOT)\libs\tomcrypt\hashes\helper\hash_memory.obj\
    $(ROOT)\libs\tomcrypt\hashes\md2.obj\
    $(ROOT)\libs\tomcrypt\hashes\md4.obj\
    $(ROOT)\libs\tomcrypt\hashes\md5.obj\
    $(ROOT)\libs\tomcrypt\hashes\rmd128.obj\
    $(ROOT)\libs\tomcrypt\hashes\rmd160.obj\
    $(ROOT)\libs\tomcrypt\hashes\sha1.obj\
    $(ROOT)\libs\tomcrypt\hashes\sha2\sha256.obj\
    $(ROOT)\libs\tomcrypt\hashes\sha2\sha512.obj\
    $(ROOT)\libs\tomcrypt\hashes\tiger.obj\
    $(ROOT)\libs\tomcrypt\hashes\whirl\whirl.obj\
    $(ROOT)\libs\tomcrypt\hmac\hmac_done.obj\
    $(ROOT)\libs\tomcrypt\hmac\hmac_init.obj\
    $(ROOT)\libs\tomcrypt\hmac\hmac_memory.obj\
    $(ROOT)\libs\tomcrypt\hmac\hmac_process.obj\
    $(ROOT)\libs\tomcrypt\misc\base64\base64_decode.obj\
    $(ROOT)\libs\tomcrypt\misc\base64\base64_encode.obj\
    $(ROOT)\libs\tomcrypt\misc\burn_stack.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_argchk.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_cipher_descriptor.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_cipher_is_valid.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_find_cipher.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_find_cipher_any.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_find_cipher_id.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_find_hash.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_find_hash_any.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_find_hash_id.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_find_prng.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_hash_descriptor.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_hash_is_valid.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_prng_descriptor.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_prng_is_valid.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_register_cipher.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_register_hash.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_register_prng.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_unregister_cipher.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_unregister_hash.obj\
    $(ROOT)\libs\tomcrypt\misc\crypt\crypt_unregister_prng.obj\
    $(ROOT)\libs\tomcrypt\misc\error_to_string.obj\
    $(ROOT)\libs\tomcrypt\misc\pkcs5\pkcs_5_1.obj\
    $(ROOT)\libs\tomcrypt\misc\pkcs5\pkcs_5_2.obj\
    $(ROOT)\libs\tomcrypt\misc\zeromem.obj\
    $(ROOT)\libs\tomcrypt\modes\cbc\cbc_decrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\cbc\cbc_done.obj\
    $(ROOT)\libs\tomcrypt\modes\cbc\cbc_encrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\cbc\cbc_getiv.obj\
    $(ROOT)\libs\tomcrypt\modes\cbc\cbc_setiv.obj\
    $(ROOT)\libs\tomcrypt\modes\cbc\cbc_start.obj\
    $(ROOT)\libs\tomcrypt\modes\cfb\cfb_decrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\cfb\cfb_done.obj\
    $(ROOT)\libs\tomcrypt\modes\cfb\cfb_encrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\cfb\cfb_getiv.obj\
    $(ROOT)\libs\tomcrypt\modes\cfb\cfb_setiv.obj\
    $(ROOT)\libs\tomcrypt\modes\cfb\cfb_start.obj\
    $(ROOT)\libs\tomcrypt\modes\ctr\ctr_decrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\ctr\ctr_done.obj\
    $(ROOT)\libs\tomcrypt\modes\ctr\ctr_encrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\ctr\ctr_getiv.obj\
    $(ROOT)\libs\tomcrypt\modes\ctr\ctr_setiv.obj\
    $(ROOT)\libs\tomcrypt\modes\ctr\ctr_start.obj\
    $(ROOT)\libs\tomcrypt\modes\ecb\ecb_decrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\ecb\ecb_done.obj\
    $(ROOT)\libs\tomcrypt\modes\ecb\ecb_encrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\ecb\ecb_start.obj\
    $(ROOT)\libs\tomcrypt\modes\ofb\ofb_decrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\ofb\ofb_done.obj\
    $(ROOT)\libs\tomcrypt\modes\ofb\ofb_encrypt.obj\
    $(ROOT)\libs\tomcrypt\modes\ofb\ofb_getiv.obj\
    $(ROOT)\libs\tomcrypt\modes\ofb\ofb_setiv.obj\
    $(ROOT)\libs\tomcrypt\modes\ofb\ofb_start.obj\
    $(ROOT)\libs\tomcrypt\prngs\fortuna.obj\
    $(ROOT)\libs\tomcrypt\prngs\rc4.obj\
    $(ROOT)\libs\tomcrypt\prngs\rng_get_bytes.obj\
    $(ROOT)\libs\tomcrypt\prngs\rng_make_prng.obj\
    $(ROOT)\libs\tomcrypt\prngs\sober128.obj\
    $(ROOT)\libs\tomcrypt\prngs\sprng.obj\
    $(ROOT)\libs\tomcrypt\prngs\yarrow.obj\
    $(ROOT)\libs\zlib\source\adler32.obj\
    $(ROOT)\libs\zlib\source\compress.obj\
    $(ROOT)\libs\zlib\source\crc32.obj\
    $(ROOT)\libs\zlib\source\deflate.obj\
    $(ROOT)\libs\zlib\source\gvmat32.obj\
    $(ROOT)\libs\zlib\source\gvmat32c.obj\
    $(ROOT)\libs\zlib\source\gzio.obj\
    $(ROOT)\libs\zlib\source\infback.obj\
    $(ROOT)\libs\zlib\source\inffas32.obj\
    $(ROOT)\libs\zlib\source\inffast.obj\
    $(ROOT)\libs\zlib\source\inflate.obj\
    $(ROOT)\libs\zlib\source\inftrees.obj\
    $(ROOT)\libs\zlib\source\trees.obj\
    $(ROOT)\libs\zlib\source\uncompr.obj\
    $(ROOT)\libs\zlib\source\zutil.obj\
    $(ROOT)\generic\alcoExt.obj\
    $(ROOT)\generic\alcoCrypt.obj\
    $(ROOT)\generic\alcoEncoding.obj\
    $(ROOT)\generic\alcoUtil.obj\
    $(ROOT)\generic\alcoVolume.obj\
    $(ROOT)\generic\alcoZlib.obj\
    $(ROOT)\win\alcoWinIoFtpd.obj\
    $(ROOT)\win\alcoWinUtil.obj\
    $(ROOT)\win\alcoWinVolume.obj\
    $(ROOT)\win\alcoExt.res

# -------------------------------------------------------------------------

AFLAGS  = -nologo -coff
CFLAGS  = $(cdebug) $(cflags) $(cvarsmt) $(PKG_CFLAGS) $(LTC_CFLAGS) $(TCL_CFLAGS) $(ZLIB_CFLAGS) $(INCLUDE_DIRS)
LFLAGS  = $(ldebug) $(dlllflags) $(conlibsmt) $(PKG_LFLAGS) $(TCL_STUBLIB)
RFLAGS  = $(rcflags) $(rcvars) $(PKG_CFLAGS)

# -------------------------------------------------------------------------

.asm.obj::
    $(AS) -c $(AFLAGS) -Fo"$(PKG_INTDIR)\\" @<<
$<
<<

.c.obj::
    $(CC) -c $(CFLAGS) -Fo"$(PKG_INTDIR)\\" @<<
$<
<<

.rc.res:
    $(RC) $(RFLAGS) -Fo"$(PKG_INTDIR)\$(<B).res" $<

.SUFFIXES:
.SUFFIXES:.asm .c .rc

# -------------------------------------------------------------------------

default: all
all:     setup library install
release: setup library

clean:
    @IF EXIST $(PKG_FILE) DEL /F $(PKG_FILE)
    @IF EXIST $(PKG_OUTFILE) DEL /F $(PKG_OUTFILE)
    @IF EXIST $(PKG_INTDIR)\nul RMDIR /Q /S $(PKG_INTDIR)
    @IF EXIST version.vc DEL /F version.vc

distclean: clean
    @DEL $(PKG_NAME)*.dll *.obj *.pdb
    @IF EXIST Debug\nul RMDIR /Q /S Debug
    @IF EXIST Release\nul RMDIR /Q /S Release
    @IF EXIST nmakehlp.exe DEL /F nmakehlp.exe
    @IF EXIST nmakehlp.obj DEL /F nmakehlp.obj

install:
    @COPY /Y $(PKG_OUTFILE) $(ROOT)

library: $(OBJECT_FILES)
    $(LINK) $(LFLAGS) -out:$(PKG_OUTFILE) $(INTERMEDIATE_FILES)

setup:
    @IF NOT EXIST $(PKG_TOPDIR)\nul MKDIR $(PKG_TOPDIR)
    @IF NOT EXIST $(PKG_INTDIR)\nul MKDIR $(PKG_INTDIR)
