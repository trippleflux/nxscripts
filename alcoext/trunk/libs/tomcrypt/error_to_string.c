/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtomcrypt.com
 */

#include "tomcrypt.h"

/**
  @file error_to_string.c
  Convert error codes to ASCII strings, Tom St Denis
*/

static const char *err_2_str[] =
{
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

/**
   Convert an LTC error code to ASCII
   @param err    The error code
   @return A pointer to the ASCII NUL terminated string for the error or "Invalid error code." if the err code was not valid.
*/
const char *error_to_string(int err)
{
   if (err < 0 || err >= (int)(sizeof(err_2_str)/sizeof(err_2_str[0]))) {
      return "unknown error";
   } else {
      return err_2_str[err];
   }
}
