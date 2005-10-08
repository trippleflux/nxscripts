/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtomcrypt.org
 */
#include "tomcrypt.h"
#include <signal.h>

/**
  @file crypt_utils.c
  Miscellaneous utilities, Tom St Denis
*/

/**
   Burn some stack memory
   @param len amount of stack to burn in bytes
*/
void burn_stack(unsigned long len)
{
   unsigned char buf[32];
   zeromem(buf, sizeof(buf));
   if (len > (unsigned long)sizeof(buf))
      burn_stack(len - sizeof(buf));
}

#if (ARGTYPE == 0)
/**
   Perform argument checking
*/
void crypt_argchk(char *v, char *s, int d)
{
 fprintf(stderr, "LTC_ARGCHK '%s' failure on line %d of file %s\n",
         v, d, s);
 (void)raise(SIGABRT);
}
#endif

/**
   Zero a block of memory
   @param out    The destination of the area to zero
   @param outlen The length of the area to zero (octets)
*/
void zeromem(void *out, size_t outlen)
{
   unsigned char *mem = out;
   LTC_ARGCHK(out != NULL);
   while (outlen-- > 0) {
      *mem++ = 0;
   }
}

/**
   Convert an LTC error code to ASCII
   @param err    The error code
   @return A pointer to the ASCII NUL terminated string for the error or "Invalid error code." if the err code was not valid.
*/
const char *error_to_string(int err)
{
    static const char *err_2_str[] = {
       "success",
       "generic error",
       "no-operation requested",

       "invalid key length for block cipher",
       "invalid number of rounds for block cipher",
       "invalid salt length",
       "algorithm failed test vectors",

       "buffer overflow",
       "invalid input packet",

       "invalid number of bits for a PRNG",
       "error reading the PRNG",

       "invalid cipher specified",
       "invalid hash specified",
       "invalid PRNG specified",

       "out of memory",

       "invalid PK key or key type specified for function",
       "a private PK key is required",

       "invalid argument provided",
       "file not found",

       "invalid PK type",
       "invalid PK system",
       "duplicate PK key found on keyring",
       "key not found in keyring",
       "invalid sized parameter",

       "invalid size for prime"
    };

   if (err < 0 || err >= (int)(sizeof(err_2_str)/sizeof(err_2_str[0]))) {
      return "unknown error code";
   } else {
      return err_2_str[err];
   }
}
