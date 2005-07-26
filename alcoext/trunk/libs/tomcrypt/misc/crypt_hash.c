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
  @file crypt_hash.c
  Hash descriptor table and utilities, Tom St Denis
*/

struct ltc_hash_descriptor hash_descriptor[TAB_SIZE] = {
   {NULL}
};

/**
   Find a registered hash by name
   @param name   The name of the hash to look for
   @return >= 0 if found, -1 if not present
*/
int find_hash(const char *name)
{
   int x;
   LTC_ARGCHK(name != NULL);
   for (x = 0; x < TAB_SIZE; x++) {
       if (hash_descriptor[x].name != NULL && strcmp(hash_descriptor[x].name, name) == 0) {
          return x;
       }
   }
   return -1;
}

/**
   Find a hash flexibly.  First by name then if not present by digest size
   @param name        The name of the hash desired
   @param digestlen   The minimum length of the digest size (octets)
   @return >= 0 if found, -1 if not present
*/int find_hash_any(const char *name, int digestlen)
{
   int x, y, z;
   LTC_ARGCHK(name != NULL);

   x = find_hash(name);
   if (x != -1) return x;

   y = MAXBLOCKSIZE+1;
   z = -1;
   for (x = 0; x < TAB_SIZE; x++) {
       if (hash_descriptor[x].name == NULL) {
          continue;
       }
       if ((int)hash_descriptor[x].hashsize >= digestlen && (int)hash_descriptor[x].hashsize < y) {
          z = x;
          y = hash_descriptor[x].hashsize;
       }
   }
   return z;
}

/**
   Find a hash by ID number
   @param ID    The ID (not same as index) of the hash to find
   @return >= 0 if found, -1 if not present
*/
int find_hash_id(unsigned char ID)
{
   int x;
   for (x = 0; x < TAB_SIZE; x++) {
       if (hash_descriptor[x].ID == ID) {
          return (hash_descriptor[x].name == NULL) ? -1 : x;
       }
   }
   return -1;
}

/*
   Test if a hash index is valid
   @param idx   The index of the hash to search for
   @return CRYPT_OK if valid
*/
int hash_is_valid(int idx)
{
   if (idx < 0 || idx >= TAB_SIZE || hash_descriptor[idx].name == NULL) {
      return CRYPT_INVALID_HASH;
   }
   return CRYPT_OK;
}

/**
   Register a hash with the descriptor table
   @param hash   The hash you wish to register
   @return value >= 0 if successfully added (or already present), -1 if unsuccessful
*/
int register_hash(const struct ltc_hash_descriptor *hash)
{
   int x;

   LTC_ARGCHK(hash != NULL);

   /* is it already registered? */
   for (x = 0; x < TAB_SIZE; x++) {
       if (memcmp(&hash_descriptor[x], hash, sizeof(struct ltc_hash_descriptor)) == 0) {
          return x;
       }
   }

   /* find a blank spot */
   for (x = 0; x < TAB_SIZE; x++) {
       if (hash_descriptor[x].name == NULL) {
          XMEMCPY(&hash_descriptor[x], hash, sizeof(struct ltc_hash_descriptor));
          return x;
       }
   }

   /* no spot */
   return -1;
}

/**
  Unregister a hash from the descriptor table
  @param hash   The hash descriptor to remove
  @return CRYPT_OK on success
*/
int unregister_hash(const struct ltc_hash_descriptor *hash)
{
   int x;

   LTC_ARGCHK(hash != NULL);

   /* is it already registered? */
   for (x = 0; x < TAB_SIZE; x++) {
       if (memcmp(&hash_descriptor[x], hash, sizeof(struct ltc_hash_descriptor)) == 0) {
          hash_descriptor[x].name = NULL;
          return CRYPT_OK;
       }
   }
   return CRYPT_ERROR;
}
