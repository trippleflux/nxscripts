/* This header is meant to be included before mycrypt.h in projects where
 * you don't want to throw all the defines in a makefile.
 */
#ifndef TOMCRYPT_CUSTOM_H_
#define TOMCRYPT_CUSTOM_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* For Tcl memory allocation functions. */
#include <tcl.h>

/*
 * Tcl_AttemptAlloc and Tcl_AttemptRealloc do not cause the interpreter to
 * panic if the requested memory allocation fails. Since LibTomCrypt checks
 * return value of XMALLOC/XREALLOC, the 'attempt' functions are better suited.
 */
#ifdef TCL_MEM_DEBUG
#   define XMALLOC(n)       Tcl_AttemptDbCkalloc((n), __FILE__, __LINE__)
#   define XREALLOC(p,n)    Tcl_AttemptDbCkrealloc((char *)(p), (n), __FILE__, __LINE__)
#   define XFREE(p)         Tcl_DbCkfree((char *)(p), __FILE__, __LINE__)
#else
#   define XMALLOC(n)       Tcl_AttemptAlloc((n))
#   define XREALLOC(p,n)    Tcl_AttemptRealloc((char *)(p),(n))
#   define XFREE(p)         Tcl_Free((char *)(p))
#endif /* TCL_MEM_DEBUG */

#define XMEMSET  memset
#define XMEMCPY  memcpy

#define XCLOCK   clock
#define XCLOCKS_PER_SEC CLOCKS_PER_SEC

/* type of argument checking, 0=default, 1=fatal and 2=error+continue, 3=nothing */
#define ARGTYPE 3

/* Use small code where possible */
/* #define LTC_SMALL_CODE */

/* Enable self-test test vector checking */
/* #define LTC_TEST */

/* clean the stack of functions which put private information on stack */
/* #define LTC_CLEAN_STACK */

/* disable all file related functions */
#define LTC_NO_FILE

/* disable all forms of ASM */
/* #define LTC_NO_ASM */

/* disable FAST mode */
/* #define LTC_NO_FAST */

/* disable BSWAP on x86 */
/* #define LTC_NO_BSWAP */

/* ---> Symmetric Block Ciphers <--- */
#define ANUBIS
#define ANUBIS_TWEAK
#define BLOWFISH
#define CAST5
#define DES
#define KHAZAD
#define NOEKEON
#define RC2
#define RC5
#define RC6
#define RIJNDAEL
#define SAFER
#define SAFERP
#define SKIPJACK
#define TWOFISH
#define TWOFISH_TABLES
#define XTEA

/* ---> Block Cipher Modes of Operation <--- */
#define CBC
#define CFB
#define CTR
#define ECB
#define OFB

/* ---> One-Way Hash Functions <--- */
#define MD2
#define MD4
#define MD5
#define RIPEMD128
#define RIPEMD160
#define SHA1
#define SHA224
#define SHA256
#define SHA384
#define SHA512
#define TIGER
#define WHIRLPOOL

/* ---> MAC functions <--- */
#define HMAC
#define OMAC
#define PMAC
#define PELICAN

/* Various tidbits of modern neatoness */
#define BASE64

/* --> Pseudo Random Number Generators <--- */
/* Yarrow */
#define YARROW

/* which descriptor of AES to use?  */
/* 0 = rijndael_enc 1 = aes_enc, 2 = rijndael [full], 3 = aes [full] */
#define YARROW_AES 2

/* a PRNG that simply reads from an available system source */
#define SPRNG

/* Fortuna PRNG */
#define FORTUNA

/* reseed every N calls to the read function */
#define FORTUNA_WD    10

/* number of pools (4..32) can save a bit of ram by lowering the count */
#define FORTUNA_POOLS 32

/* Greg's SOBER128 PRNG */
#define SOBER128

/* the *nix style /dev/random device */
#if !defined(_WIN32) && !defined(_WIN64)
#   define DEVRANDOM
#endif

/* try /dev/urandom before trying /dev/random */
#define TRY_URANDOM_FIRST

/* The RC4 stream cipher */
#define RC4

/* PKCS #5 (Password Handling) stuff */
#define PKCS_5

/* Dependency checks. */
#if defined(PELICAN) && !defined(RIJNDAEL)
#   error Pelican-MAC requires RIJNDAEL
#endif

#if defined(YARROW) && !defined(CTR)
#   error YARROW requires CTR chaining mode to be defined!
#endif

#endif
