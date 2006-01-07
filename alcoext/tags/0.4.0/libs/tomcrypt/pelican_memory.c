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

/**
   @file pelican_memory.c
   Pelican MAC, MAC a block of memory, by Tom St Denis
*/

#ifdef PELICAN

/**
  Pelican block of memory
  @param cipher   The index of the desired cipher, must be AES
  @param key      The key for the MAC
  @param keylen   The length of the key (octets)
  @param in       The input to MAC
  @param inlen    The length of the input (octets)
  @param out      [out] The output TAG
  @param outlen   [out] The resulting size of the authentication tag
  @return CRYPT_OK on success
*/
int pelican_memory(int cipher,
                   const unsigned char *key, unsigned long keylen,
                   const unsigned char *in,  unsigned long inlen,
                         unsigned char *out, unsigned long *outlen)
{
   pelican_state *pel;
   int err;

   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   pel = XMALLOC(sizeof(pelican_state));
   if (pel == NULL) {
      return CRYPT_MEM;
   }

   if ((err = pelican_init(pel, cipher, key, keylen)) != CRYPT_OK) {
      XFREE(pel);
      return err;
   }
   if ((err = pelican_process(pel, in ,inlen)) != CRYPT_OK) {
      XFREE(pel);
      return err;
   }
   err = pelican_done(pel, out, outlen);
   XFREE(pel);
   return err;
}


#endif

/* $Source: /cvs/libtom/libtomcrypt/src/mac/pelican/pelican_memory.c,v $ */
/* $Revision: 1.5 $ */
/* $Date: 2005/05/05 14:35:59 $ */
