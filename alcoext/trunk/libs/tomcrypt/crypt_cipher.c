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
  @file crypt_cipher.c
  Cipher descriptor table and utilities, Tom St Denis
*/

struct ltc_cipher_descriptor cipher_descriptor[TAB_SIZE] = {
   {NULL}
};

/**
   Find a registered cipher by name
   @param name   The name of the cipher to look for
   @return >= 0 if found, -1 if not present
*/
int find_cipher(const char *name)
{
   int x;
   LTC_ARGCHK(name != NULL);
   for (x = 0; x < TAB_SIZE; x++) {
       if (cipher_descriptor[x].name != NULL && !strcmp(cipher_descriptor[x].name, name)) {
          return x;
       }
   }
   return -1;
}

/**
   Find a cipher flexibly.  First by name then if not present by block and key size
   @param name        The name of the cipher desired
   @param blocklen    The minimum length of the block cipher desired (octets)
   @param keylen      The minimum length of the key size desired (octets)
   @return >= 0 if found, -1 if not present
*/
int find_cipher_any(const char *name, int blocklen, int keylen)
{
   int x;

   LTC_ARGCHK(name != NULL);

   x = find_cipher(name);
   if (x != -1) return x;

   for (x = 0; x < TAB_SIZE; x++) {
       if (cipher_descriptor[x].name == NULL) {
          continue;
       }
       if (blocklen <= (int)cipher_descriptor[x].block_length && keylen <= (int)cipher_descriptor[x].max_key_length) {
          return x;
       }
   }
   return -1;
}

/**
   Find a cipher by ID number
   @param ID    The ID (not same as index) of the cipher to find
   @return >= 0 if found, -1 if not present
*/
int find_cipher_id(unsigned char ID)
{
   int x;
   for (x = 0; x < TAB_SIZE; x++) {
       if (cipher_descriptor[x].ID == ID) {
          return (cipher_descriptor[x].name == NULL) ? -1 : x;
       }
   }
   return -1;
}

/*
   Test if a cipher index is valid
   @param idx   The index of the cipher to search for
   @return CRYPT_OK if valid
*/
int cipher_is_valid(int idx)
{
   if (idx < 0 || idx >= TAB_SIZE || cipher_descriptor[idx].name == NULL) {
      return CRYPT_INVALID_CIPHER;
   }
   return CRYPT_OK;
}

/**
   Register a cipher with the descriptor table
   @param cipher   The cipher you wish to register
   @return value >= 0 if successfully added (or already present), -1 if unsuccessful
*/
int register_cipher(const struct ltc_cipher_descriptor *cipher)
{
   int x;

   LTC_ARGCHK(cipher != NULL);

   /* is it already registered? */
   for (x = 0; x < TAB_SIZE; x++) {
       if (cipher_descriptor[x].name != NULL && cipher_descriptor[x].ID == cipher->ID) {
          return x;
       }
   }

   /* find a blank spot */
   for (x = 0; x < TAB_SIZE; x++) {
       if (cipher_descriptor[x].name == NULL) {
          XMEMCPY(&cipher_descriptor[x], cipher, sizeof(struct ltc_cipher_descriptor));
          return x;
       }
   }

   /* no spot */
   return -1;
}

/**
  Unregister a cipher from the descriptor table
  @param cipher   The cipher descriptor to remove
  @return CRYPT_OK on success
*/
int unregister_cipher(const struct ltc_cipher_descriptor *cipher)
{
   int x;

   LTC_ARGCHK(cipher != NULL);

   /* is it already registered? */
   for (x = 0; x < TAB_SIZE; x++) {
       if (memcmp(&cipher_descriptor[x], cipher, sizeof(struct ltc_cipher_descriptor)) == 0) {
          cipher_descriptor[x].name = NULL;
          cipher_descriptor[x].ID   = 255;
          return CRYPT_OK;
       }
   }
   return CRYPT_ERROR;
}
