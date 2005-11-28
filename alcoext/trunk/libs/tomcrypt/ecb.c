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
  @file ecb.c
  ECB implementation, Tom St Denis
*/

#ifdef ECB

/**
  Initialize a ECB context
  @param cipher      The index of the cipher desired
  @param key         The secret key
  @param keylen      The length of the secret key (octets)
  @param num_rounds  Number of rounds in the cipher desired (0 for default)
  @param ecb         The ECB state to initialize
  @return CRYPT_OK if successful
*/
int ecb_start(int cipher, const unsigned char *key, int keylen, int num_rounds, symmetric_ECB *ecb)
{
   int err;
   LTC_ARGCHK(key != NULL);
   LTC_ARGCHK(ecb != NULL);

   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }
   ecb->cipher = cipher;
   ecb->blocklen = cipher_descriptor[cipher].block_length;
   return cipher_descriptor[cipher].setup(key, keylen, num_rounds, &ecb->key);
}

/**
  ECB decrypt
  @param ct     Ciphertext
  @param pt     [out] Plaintext
  @param len    The number of octets to process (must be multiple of the cipher block size)
  @param ecb    ECB state
  @return CRYPT_OK if successful
*/
int ecb_decrypt(const unsigned char *ct, unsigned char *pt, unsigned long len, symmetric_ECB *ecb)
{
   int err;
   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(ecb != NULL);
   if ((err = cipher_is_valid(ecb->cipher)) != CRYPT_OK) {
       return err;
   }
   if (len % cipher_descriptor[ecb->cipher].block_length) {
      return CRYPT_INVALID_ARG;
   }

   /* check for accel */
   if (cipher_descriptor[ecb->cipher].accel_ecb_decrypt != NULL) {
      cipher_descriptor[ecb->cipher].accel_ecb_decrypt(ct, pt, len / cipher_descriptor[ecb->cipher].block_length, &ecb->key);
   } else {
      while (len) {
         cipher_descriptor[ecb->cipher].ecb_decrypt(ct, pt, &ecb->key);
         pt  += cipher_descriptor[ecb->cipher].block_length;
         ct  += cipher_descriptor[ecb->cipher].block_length;
         len -= cipher_descriptor[ecb->cipher].block_length;
      }
   }
   return CRYPT_OK;
}

/**
  ECB encrypt
  @param pt     Plaintext
  @param ct     [out] Ciphertext
  @param len    The number of octets to process (must be multiple of the cipher block size)
  @param ecb    ECB state
  @return CRYPT_OK if successful
*/
int ecb_encrypt(const unsigned char *pt, unsigned char *ct, unsigned long len, symmetric_ECB *ecb)
{
   int err;
   LTC_ARGCHK(pt != NULL);
   LTC_ARGCHK(ct != NULL);
   LTC_ARGCHK(ecb != NULL);
   if ((err = cipher_is_valid(ecb->cipher)) != CRYPT_OK) {
       return err;
   }
   if (len % cipher_descriptor[ecb->cipher].block_length) {
      return CRYPT_INVALID_ARG;
   }

   /* check for accel */
   if (cipher_descriptor[ecb->cipher].accel_ecb_encrypt != NULL) {
      cipher_descriptor[ecb->cipher].accel_ecb_encrypt(pt, ct, len / cipher_descriptor[ecb->cipher].block_length, &ecb->key);
   } else {
      while (len) {
         cipher_descriptor[ecb->cipher].ecb_encrypt(pt, ct, &ecb->key);
         pt  += cipher_descriptor[ecb->cipher].block_length;
         ct  += cipher_descriptor[ecb->cipher].block_length;
         len -= cipher_descriptor[ecb->cipher].block_length;
      }
   }
   return CRYPT_OK;
}

/**
  Terminate the chain
  @param ecb    The ECB chain to terminate
  @return CRYPT_OK on success
*/
int ecb_done(symmetric_ECB *ecb)
{
   int err;
   LTC_ARGCHK(ecb != NULL);

   if ((err = cipher_is_valid(ecb->cipher)) != CRYPT_OK) {
      return err;
   }
   cipher_descriptor[ecb->cipher].done(&ecb->key);
   return CRYPT_OK;
}

#endif
