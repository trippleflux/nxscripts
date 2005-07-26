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
  @file ofb.c
  OFB implementation, Tom St Denis
*/

#ifdef OFB

/**
  Initialize a OFB context
  @param cipher      The index of the cipher desired
  @param IV          The initial vector
  @param key         The secret key
  @param keylen      The length of the secret key (octets)
  @param num_rounds  Number of rounds in the cipher desired (0 for default)
  @param ofb         The OFB state to initialize
  @return CRYPT_OK if successful
*/
int ofb_start(int cipher, const unsigned char *IV, const unsigned char *key,
              int keylen, int num_rounds, symmetric_OFB *ofb)
{
   int x, err;

   LTC_ARGCHK(IV != NULL);
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(ofb != NULL);

   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }

   /* copy details */
   ofb->cipher = cipher;
   ofb->blocklen = cipher_descriptor[cipher].block_length;
   for (x = 0; x < ofb->blocklen; x++) {
       ofb->IV[x] = IV[x];
   }

   /* init the cipher */
   ofb->padlen = ofb->blocklen;
   return cipher_descriptor[cipher].setup(key, keylen, num_rounds, &ofb->key);
}

/**
   Get the current initial vector
  @param IV   [out] The destination of the initial vector
  @param len  [in/out]  The max size and resulting size of the initial vector
  @param ofb  The OFB state
  @return CRYPT_OK if successful
*/
int ofb_getiv(unsigned char *IV, unsigned long *len, symmetric_OFB *ofb)
{
   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(len != NULL);
   LTC_ARGCHK(ofb != NULL);
   if ((unsigned long)ofb->blocklen > *len) {
      return CRYPT_BUFFER_OVERFLOW;
   }
   XMEMCPY(IV, ofb->IV, ofb->blocklen);
   *len = ofb->blocklen;

   return CRYPT_OK;
}

/**
   Set an initial vector
  @param IV   The initial vector
  @param len  The length of the vector (in octets)
  @param ofb  The OFB state
  @return CRYPT_OK if successful
*/
int ofb_setiv(const unsigned char *IV, unsigned long len, symmetric_OFB *ofb)
{
   int err;

   LTC_ARGCHK(IV  != NULL);
   LTC_ARGCHK(ofb != NULL);

   if ((err = cipher_is_valid(ofb->cipher)) != CRYPT_OK) {
       return err;
   }

   if (len != (unsigned long)ofb->blocklen) {
      return CRYPT_INVALID_ARG;
   }

   /* force next block */
   ofb->padlen = 0;
   cipher_descriptor[ofb->cipher].ecb_encrypt(IV, ofb->IV, &ofb->key);
   return CRYPT_OK;
}

/**
  OFB decrypt
  @param ct      Ciphertext
  @param pt      [out] Plaintext
  @param len     Length of ciphertext (octets)
  @param ofb     OFB state
  @return CRYPT_OK if successful
*/
int ofb_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_OFB *ofb)
{
   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(ofb != NULL);
   return ofb_encrypt(ct, pt, len, ofb);
}

/**
  OFB encrypt
  @param pt     Plaintext
  @param ct     [out] Ciphertext
  @param len    Length of plaintext (octets)
  @param ofb    OFB state
  @return CRYPT_OK if successful
*/
int ofb_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_OFB *ofb)
{
   int err;
   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(ofb != NULL);
   if ((err = cipher_is_valid(ofb->cipher)) != CRYPT_OK) {
       return err;
   }

   /* is blocklen/padlen valid? */
   if (ofb->blocklen < 0 || ofb->blocklen > (int)sizeof(ofb->IV) ||
       ofb->padlen   < 0 || ofb->padlen   > (int)sizeof(ofb->IV)) {
      return CRYPT_INVALID_ARG;
   }

   while (len-- > 0) {
       if (ofb->padlen == ofb->blocklen) {
          cipher_descriptor[ofb->cipher].ecb_encrypt(ofb->IV, ofb->IV, &ofb->key);
          ofb->padlen = 0;
       }
       *ct++ = *pt++ ^ ofb->IV[ofb->padlen++];
   }
   return CRYPT_OK;
}

/**
  Terminate the chain
  @param ofb    The OFB chain to terminate
  @return CRYPT_OK on success
*/
int ofb_done(symmetric_OFB *ofb)
{
   int err;
   LTC_ARGCHK(ofb != NULL);

   if ((err = cipher_is_valid(ofb->cipher)) != CRYPT_OK) {
      return err;
   }
   cipher_descriptor[ofb->cipher].done(&ofb->key);
   return CRYPT_OK;
}

#endif
