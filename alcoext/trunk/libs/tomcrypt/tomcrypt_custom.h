#ifndef TOMCRYPT_CUSTOM_H_
#define TOMCRYPT_CUSTOM_H_

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

// Use Tcl's memory allocation functions.
#include <tcl.h>

#ifndef LTC_CALL
   #define LTC_CALL
#endif

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
#define XSTRCMP         strcmp
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
#define LTC_ANUBIS
#define LTC_ANUBIS_TWEAK
#define LTC_BLOWFISH
#define LTC_CAST5
#define LTC_DES
//#define LTC_KASUMI                /* TODO */
#define LTC_KHAZAD
//#define LTC_KSEED                 /* TODO */
#define LTC_NOEKEON
#define LTC_RC2
#define LTC_RC5
#define LTC_RC6
#define LTC_RIJNDAEL
#define LTC_SAFER
#define LTC_SAFERP
#define LTC_SKIPJACK
#define LTC_TWOFISH
#define LTC_TWOFISH_TABLES
#define LTC_XTEA

/* Block Cipher Modes of Operation */
#define LTC_CBC_MODE
#define LTC_CFB_MODE
#define LTC_CTR_MODE
#define LTC_ECB_MODE
//#define LTC_F8_MODE               /* TODO */
//#define LTC_LRW_MODE              /* TODO */
//#define LTC_LRW_TABLES            /* TODO */
#define LTC_OFB_MODE
//#define LTC_XTS_MODE              /* TODO */

/* One-Way Hash Functions */
//#define LTC_CHC_HASH              /* TODO */
#define LTC_MD2
#define LTC_MD4
#define LTC_MD5
#define LTC_RIPEMD128
#define LTC_RIPEMD160
//#define LTC_RIPEMD256             /* TODO */
//#define LTC_RIPEMD320             /* TODO */
#define LTC_SHA1
#define LTC_SHA224
#define LTC_SHA256
#define LTC_SHA384
#define LTC_SHA512
#define LTC_TIGER
#define LTC_WHIRLPOOL

/* MAC functions */
//#define LTC_F9_MODE               /* TODO */
#define LTC_HMAC
#define LTC_OMAC
#define LTC_PELICAN
#define LTC_PMAC
//#define LTC_XCBC                  /* TODO */

#if defined(LTC_PELICAN) && !defined(LTC_RIJNDAEL)
   #error Pelican-MAC requires LTC_RIJNDAEL
#endif

/* Various tidbits of modern neatoness */
#define LTC_BASE64

/* Pseudo Random Number Generators */

/* Yarrow */
#define LTC_YARROW
/* which descriptor of AES to use?  */
/* 0 = rijndael_enc 1 = aes_enc, 2 = rijndael [full], 3 = aes [full] */
#define LTC_YARROW_AES 2

#if defined(LTC_YARROW) && !defined(LTC_CTR_MODE)
   #error LTC_YARROW requires LTC_CTR_MODE chaining mode to be defined!
#endif

/* a PRNG that simply reads from an available system source */
#define LTC_SPRNG

/* The LTC_RC4 stream cipher */
#define LTC_RC4

/* Fortuna PRNG */
#define LTC_FORTUNA
/* reseed every N calls to the read function */
#define LTC_FORTUNA_WD    10
/* number of pools (4..32) can save a bit of ram by lowering the count */
#define LTC_FORTUNA_POOLS 32

/* Greg's LTC_SOBER128 PRNG */
#define LTC_SOBER128

/* the *nix style /dev/random device */
#define LTC_DEVRANDOM

/* try /dev/urandom before trying /dev/random */
#define TRY_URANDOM_FIRST

/* LTC_PKCS #5 (Password Handling) stuff */
#define LTC_PKCS_1
#define LTC_PKCS_5

/* No thread management */
#define LTC_MUTEX_GLOBAL(x)
#define LTC_MUTEX_PROTO(x)
#define LTC_MUTEX_TYPE(x)
#define LTC_MUTEX_INIT(x)
#define LTC_MUTEX_LOCK(x)
#define LTC_MUTEX_UNLOCK(x)

#endif
