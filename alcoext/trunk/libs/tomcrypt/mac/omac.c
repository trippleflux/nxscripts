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
  @file omac_init.c
  OMAC1 support, by Tom St Denis
*/

#ifdef OMAC

/**
  Initialize an OMAC state
  @param omac    The OMAC state to initialize
  @param cipher  The index of the desired cipher
  @param key     The secret key
  @param keylen  The length of the secret key (octets)
  @return CRYPT_OK if successful
*/
int omac_init(omac_state *omac, int cipher, const unsigned char *key, unsigned long keylen)
{
   int err, x, y, mask, msb, len;

   LTC_ARGCHK(omac != NULL);
   LTC_ARGCHK(key  != NULL);

   /* schedule the key */
   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }

#ifdef LTC_FAST
   if (cipher_descriptor[cipher].block_length % sizeof(LTC_FAST_TYPE)) {
       return CRYPT_INVALID_ARG;
   }
#endif

   /* now setup the system */
   switch (cipher_descriptor[cipher].block_length) {
       case 8:  mask = 0x1B;
                len  = 8;
                break;
       case 16: mask = 0x87;
                len  = 16;
                break;
       default: return CRYPT_INVALID_ARG;
   }

   if ((err = cipher_descriptor[cipher].setup(key, keylen, 0, &omac->key)) != CRYPT_OK) {
      return err;
   }

   /* ok now we need Lu and Lu^2 [calc one from the other] */

   /* first calc L which is Ek(0) */
   zeromem(omac->Lu[0], cipher_descriptor[cipher].block_length);
   cipher_descriptor[cipher].ecb_encrypt(omac->Lu[0], omac->Lu[0], &omac->key);

   /* now do the mults, whoopy! */
   for (x = 0; x < 2; x++) {
       /* if msb(L * u^(x+1)) = 0 then just shift, otherwise shift and xor constant mask */
       msb = omac->Lu[x][0] >> 7;

       /* shift left */
       for (y = 0; y < (len - 1); y++) {
           omac->Lu[x][y] = ((omac->Lu[x][y] << 1) | (omac->Lu[x][y+1] >> 7)) & 255;
       }
       omac->Lu[x][len - 1] = ((omac->Lu[x][len - 1] << 1) ^ (msb ? mask : 0)) & 255;

       /* copy up as require */
       if (x == 0) {
          XMEMCPY(omac->Lu[1], omac->Lu[0], sizeof(omac->Lu[0]));
       }
   }

   /* setup state */
   omac->cipher_idx = cipher;
   omac->buflen     = 0;
   omac->blklen     = len;
   zeromem(omac->prev,  sizeof(omac->prev));
   zeromem(omac->block, sizeof(omac->block));

   return CRYPT_OK;
}

/**
  Process data through OMAC
  @param omac     The OMAC state
  @param in       The input data to send through OMAC
  @param inlen    The length of the input (octets)
  @return CRYPT_OK if successful
*/
int omac_process(omac_state *omac, const unsigned char *in, unsigned long inlen)
{
   unsigned long n, x;
   int           err;

   LTC_ARGCHK(omac  != NULL);
   LTC_ARGCHK(in    != NULL);
   if ((err = cipher_is_valid(omac->cipher_idx)) != CRYPT_OK) {
      return err;
   }

   if ((omac->buflen > (int)sizeof(omac->block)) || (omac->buflen < 0) ||
       (omac->blklen > (int)sizeof(omac->block)) || (omac->buflen > omac->blklen)) {
      return CRYPT_INVALID_ARG;
   }

#ifdef LTC_FAST
   if (omac->buflen == 0 && inlen > 16) {
      int y;
      for (x = 0; x < (inlen - 16); x += 16) {
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)(&omac->prev[y])) ^= *((LTC_FAST_TYPE*)(&in[y]));
          }
          in += 16;
          cipher_descriptor[omac->cipher_idx].ecb_encrypt(omac->prev, omac->prev, &omac->key);
      }
      inlen -= x;
    }
#endif

   while (inlen != 0) {
       /* ok if the block is full we xor in prev, encrypt and replace prev */
       if (omac->buflen == omac->blklen) {
          for (x = 0; x < (unsigned long)omac->blklen; x++) {
              omac->block[x] ^= omac->prev[x];
          }
          cipher_descriptor[omac->cipher_idx].ecb_encrypt(omac->block, omac->prev, &omac->key);
          omac->buflen = 0;
       }

       /* add bytes */
       n = MIN(inlen, (unsigned long)(omac->blklen - omac->buflen));
       XMEMCPY(omac->block + omac->buflen, in, n);
       omac->buflen  += n;
       inlen         -= n;
       in            += n;
   }

   return CRYPT_OK;
}

/**
 Terminate an OMAC stream
 @param omac   The OMAC state
 @param out    [out] Destination for the authentication tag
 @param outlen [in/out]  The max size and resulting size of the authentication tag
 @return CRYPT_OK if successful
*/
int omac_done(omac_state *omac, unsigned char *out, unsigned long *outlen)
{
   int       err, mode;
   unsigned  x;

   LTC_ARGCHK(omac   != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);
   if ((err = cipher_is_valid(omac->cipher_idx)) != CRYPT_OK) {
      return err;
   }

   if ((omac->buflen > (int)sizeof(omac->block)) || (omac->buflen < 0) ||
       (omac->blklen > (int)sizeof(omac->block)) || (omac->buflen > omac->blklen)) {
      return CRYPT_INVALID_ARG;
   }

   /* figure out mode */
   if (omac->buflen != omac->blklen) {
      /* add the 0x80 byte */
      omac->block[omac->buflen++] = 0x80;

      /* pad with 0x00 */
      while (omac->buflen < omac->blklen) {
         omac->block[omac->buflen++] = 0x00;
      }
      mode = 1;
   } else {
      mode = 0;
   }

   /* now xor prev + Lu[mode] */
   for (x = 0; x < (unsigned)omac->blklen; x++) {
       omac->block[x] ^= omac->prev[x] ^ omac->Lu[mode][x];
   }

   /* encrypt it */
   cipher_descriptor[omac->cipher_idx].ecb_encrypt(omac->block, omac->block, &omac->key);
   cipher_descriptor[omac->cipher_idx].done(&omac->key);

   /* output it */
   for (x = 0; x < (unsigned)omac->blklen && x < *outlen; x++) {
       out[x] = omac->block[x];
   }
   *outlen = x;

#ifdef LTC_CLEAN_STACK
   zeromem(omac, sizeof(*omac));
#endif
   return CRYPT_OK;
}

/**
  OMAC a block of memory
  @param cipher    The index of the desired cipher
  @param key       The secret key
  @param keylen    The length of the secret key (octets)
  @param in        The data to send through OMAC
  @param inlen     The length of the data to send through OMAC (octets)
  @param out       [out] The destination of the authentication tag
  @param outlen    [in/out]  The max size and resulting size of the authentication tag (octets)
  @return CRYPT_OK if successful
*/
int omac_memory(int cipher,
                const unsigned char *key, unsigned long keylen,
                const unsigned char *in,  unsigned long inlen,
                      unsigned char *out, unsigned long *outlen)
{
   int err;
   omac_state *omac;

   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* allocate ram for omac state */
   omac = (omac_state *) XMALLOC(sizeof(omac_state));
   if (omac == NULL) {
      return CRYPT_MEM;
   }

   /* omac process the message */
   if ((err = omac_init(omac, cipher, key, keylen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   if ((err = omac_process(omac, in, inlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   if ((err = omac_done(omac, out, outlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(omac, sizeof(omac_state));
#endif

   XFREE(omac);
   return err;
}

#endif
