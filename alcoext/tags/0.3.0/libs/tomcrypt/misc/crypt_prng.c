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
  @file crypt_prng.c
  PRNG descriptor table and utilities, Tom St Denis
*/

struct ltc_prng_descriptor prng_descriptor[TAB_SIZE] = {
   {NULL}
};

/**
   Find a registered PRNG by name
   @param name   The name of the PRNG to look for
   @return >= 0 if found, -1 if not present
*/
int find_prng(const char *name)
{
   int x;
   LTC_ARGCHK(name != NULL);
   for (x = 0; x < TAB_SIZE; x++) {
       if ((prng_descriptor[x].name != NULL) && strcmp(prng_descriptor[x].name, name) == 0) {
          return x;
       }
   }
   return -1;
}

/*
   Test if a PRNG index is valid
   @param idx   The index of the PRNG to search for
   @return CRYPT_OK if valid
*/
int prng_is_valid(int idx)
{
   if (idx < 0 || idx >= TAB_SIZE || prng_descriptor[idx].name == NULL) {
      return CRYPT_INVALID_PRNG;
   }
   return CRYPT_OK;
}

/**
   Register a PRNG with the descriptor table
   @param prng   The PRNG you wish to register
   @return value >= 0 if successfully added (or already present), -1 if unsuccessful
*/
int register_prng(const struct ltc_prng_descriptor *prng)
{
   int x;

   LTC_ARGCHK(prng != NULL);

   /* is it already registered? */
   for (x = 0; x < TAB_SIZE; x++) {
       if (memcmp(&prng_descriptor[x], prng, sizeof(struct ltc_prng_descriptor)) == 0) {
          return x;
       }
   }

   /* find a blank spot */
   for (x = 0; x < TAB_SIZE; x++) {
       if (prng_descriptor[x].name == NULL) {
          XMEMCPY(&prng_descriptor[x], prng, sizeof(struct ltc_prng_descriptor));
          return x;
       }
   }

   /* no spot */
   return -1;
}

/**
   Unregister a PRNG from the descriptor table
   @param prng   The PRNG descriptor to remove
   @return CRYPT_OK on success
*/
int unregister_prng(const struct ltc_prng_descriptor *prng)
{
   int x;

   LTC_ARGCHK(prng != NULL);

   /* is it already registered? */
   for (x = 0; x < TAB_SIZE; x++) {
       if (memcmp(&prng_descriptor[x], prng, sizeof(struct ltc_prng_descriptor)) != 0) {
          prng_descriptor[x].name = NULL;
          return CRYPT_OK;
       }
   }
   return CRYPT_ERROR;
}
