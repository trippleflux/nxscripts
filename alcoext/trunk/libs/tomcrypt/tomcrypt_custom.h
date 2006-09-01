#ifndef TOMCRYPT_CUSTOM_H_
#define TOMCRYPT_CUSTOM_H_

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

// Use Tcl's memory allocation functions.
#include <tcl.h>

//
// Tcl_AttemptAlloc and Tcl_AttemptRealloc do not cause the interpreter to
// panic if the requested memory allocation fails. Since LibTomCrypt checks
// return value of XMALLOC/XREALLOC, the 'attempt' functions are better suited.
//
#ifdef TCL_MEM_DEBUG
#   define XMALLOC(n)       ((void *)Tcl_AttemptDbCkalloc((n), __FILE__, __LINE__))
#   define XREALLOC(p,n)    ((void *)Tcl_AttemptDbCkrealloc((char *)(p), (n), __FILE__, __LINE__))
#   define XFREE(p)         Tcl_DbCkfree((char *)(p), __FILE__, __LINE__)
#else
#   define XMALLOC(n)       ((void *)Tcl_AttemptAlloc((n)))
#   define XREALLOC(p,n)    ((void *)Tcl_AttemptRealloc((char *)(p),(n)))
#   define XFREE(p)         Tcl_Free((char *)(p))
#endif // TCL_MEM_DEBUG

#define XCLOCK          clock
#define XCLOCKS_PER_SEC CLOCKS_PER_SEC
#define XMEMCMP         memcmp
#define XMEMCPY         memcpy
#define XMEMSET         memset
#define XQSORT          qsort

/* type of argument checking, 0=default, 1=fatal and 2=error+continue, 3=nothing */
#ifdef DEBUG
#   define ARGTYPE 1
#else
#   define ARGTYPE 3
#endif

/* Disable self-test vector checking */
#define LTC_NO_TEST

/* Disable all file related functions */
#define LTC_NO_FILE

/* Symmetric Block Ciphers */
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

/* Block Cipher Modes of Operation */
#define LTC_CBC_MODE
#define LTC_CFB_MODE
#define LTC_CTR_MODE
#define LTC_ECB_MODE
#define LTC_OFB_MODE

/* One-Way Hash Functions */
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

/* MAC functions */
#define HMAC
#define OMAC
#define PELICAN
#define PMAC

#if defined(PELICAN) && !defined(RIJNDAEL)
   #error Pelican-MAC requires RIJNDAEL
#endif

/* Various tidbits of modern neatoness */
#define BASE64

/* Pseudo Random Number Generators */

#define YARROW
/* which descriptor of AES to use?  */
/* 0 = rijndael_enc 1 = aes_enc, 2 = rijndael [full], 3 = aes [full] */
#define YARROW_AES 2

/* a PRNG that simply reads from an available system source */
#define SPRNG

/* The RC4 stream cipher */
#define RC4

/* Fortuna PRNG */
#define FORTUNA
/* reseed every N calls to the read function */
#define FORTUNA_WD    10
/* number of pools (4..32) can save a bit of ram by lowering the count */
#define FORTUNA_POOLS 32

/* Greg's SOBER128 PRNG */
#define SOBER128

/* the *nix style /dev/random device */
#define DEVRANDOM
/* try /dev/urandom before trying /dev/random */
#define TRY_URANDOM_FIRST

/* PKCS #1 (RSA) and #5 (Password Handling) stuff */
#define PKCS_5

/* No thread management */
#define LTC_MUTEX_GLOBAL(x)
#define LTC_MUTEX_PROTO(x)
#define LTC_MUTEX_TYPE(x)
#define LTC_MUTEX_INIT(x)
#define LTC_MUTEX_LOCK(x)
#define LTC_MUTEX_UNLOCK(x)

#endif
