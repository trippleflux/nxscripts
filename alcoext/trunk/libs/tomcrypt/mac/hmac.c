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
  @file hmac.c
  HMAC implementation, Tom St Denis/Dobes Vandermeer
*/

#ifdef HMAC

#define HMAC_BLOCKSIZE hash_descriptor[hash].blocksize

/**
  Initialize an HMAC context.
  @param hmac     The HMAC state
  @param hash     The index of the hash you want to use
  @param key      The secret key
  @param keylen   The length of the secret key (octets)
  @return CRYPT_OK if successful
*/
int hmac_init(hmac_state *hmac, int hash, const unsigned char *key, unsigned long keylen)
{
    unsigned char *buf;
    unsigned long hashsize;
    unsigned long i, z;
    int err;

    LTC_ARGCHK(hmac != NULL);
    LTC_ARGCHK(key  != NULL);

    /* valid hash? */
    if ((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }
    hmac->hash = hash;
    hashsize   = hash_descriptor[hash].hashsize;

    /* valid key length? */
    if (keylen == 0) {
        return CRYPT_INVALID_KEYSIZE;
    }

    /* allocate ram for buf */
    buf = (unsigned char *) XMALLOC(HMAC_BLOCKSIZE);
    if (buf == NULL) {
       return CRYPT_MEM;
    }

    /* allocate memory for key */
    hmac->key = (unsigned char *) XMALLOC(HMAC_BLOCKSIZE);
    if (hmac->key == NULL) {
       XFREE(buf);
       return CRYPT_MEM;
    }

    /* (1) make sure we have a large enough key */
    if(keylen > HMAC_BLOCKSIZE) {
        z = HMAC_BLOCKSIZE;
        if ((err = hash_memory(hash, key, keylen, hmac->key, &z)) != CRYPT_OK) {
           goto LBL_ERR;
        }
        if(hashsize < HMAC_BLOCKSIZE) {
            zeromem((hmac->key) + hashsize, (size_t)(HMAC_BLOCKSIZE - hashsize));
        }
        keylen = hashsize;
    } else {
        XMEMCPY(hmac->key, key, (size_t)keylen);
        if(keylen < HMAC_BLOCKSIZE) {
            zeromem((hmac->key) + keylen, (size_t)(HMAC_BLOCKSIZE - keylen));
        }
    }

    /* Create the initial vector for step (3) */
    for(i=0; i < HMAC_BLOCKSIZE;   i++) {
       buf[i] = hmac->key[i] ^ 0x36;
    }

    /* Pre-pend that to the hash data */
    if ((err = hash_descriptor[hash].init(&hmac->md)) != CRYPT_OK) {
       goto LBL_ERR;
    }

    if ((err = hash_descriptor[hash].process(&hmac->md, buf, HMAC_BLOCKSIZE)) != CRYPT_OK) {
       goto LBL_ERR;
    }
    goto done;
LBL_ERR:
    /* free the key since we failed */
    XFREE(hmac->key);
done:
#ifdef LTC_CLEAN_STACK
   zeromem(buf, HMAC_BLOCKSIZE);
#endif

   XFREE(buf);
   return err;
}

/**
  Process data through HMAC
  @param hmac    The hmac state
  @param in      The data to send through HMAC
  @param inlen   The length of the data to HMAC (octets)
  @return CRYPT_OK if successful
*/
int hmac_process(hmac_state *hmac, const unsigned char *in, unsigned long inlen)
{
    int err;
    LTC_ARGCHK(hmac != NULL);
    LTC_ARGCHK(in != NULL);
    if ((err = hash_is_valid(hmac->hash)) != CRYPT_OK) {
        return err;
    }
    return hash_descriptor[hmac->hash].process(&hmac->md, in, inlen);
}

/**
  Terminate an HMAC session
  @param hmac    The HMAC state
  @param out     [out] The destination of the HMAC authentication tag
  @param outlen  [in/out]  The max size and resulting size of the HMAC authentication tag
  @return CRYPT_OK if successful
*/
int hmac_done(hmac_state *hmac, unsigned char *out, unsigned long *outlen)
{
    unsigned char *buf, *isha;
    unsigned long hashsize, i;
    int hash, err;

    LTC_ARGCHK(hmac  != NULL);
    LTC_ARGCHK(out   != NULL);

    /* test hash */
    hash = hmac->hash;
    if((err = hash_is_valid(hash)) != CRYPT_OK) {
        return err;
    }

    /* get the hash message digest size */
    hashsize = hash_descriptor[hash].hashsize;

    /* allocate buffers */
    buf  = (unsigned char *) XMALLOC(HMAC_BLOCKSIZE);
    isha = (unsigned char *) XMALLOC(hashsize);
    if (buf == NULL || isha == NULL) {
       if (buf != NULL) {
          XFREE(buf);
       }
       if (isha != NULL) {
          XFREE(isha);
       }
       return CRYPT_MEM;
    }

    /* Get the hash of the first HMAC vector plus the data */
    if ((err = hash_descriptor[hash].done(&hmac->md, isha)) != CRYPT_OK) {
       goto LBL_ERR;
    }

    /* Create the second HMAC vector vector for step (3) */
    for(i=0; i < HMAC_BLOCKSIZE; i++) {
        buf[i] = hmac->key[i] ^ 0x5C;
    }

    /* Now calculate the "outer" hash for step (5), (6), and (7) */
    if ((err = hash_descriptor[hash].init(&hmac->md)) != CRYPT_OK) {
       goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash].process(&hmac->md, buf, HMAC_BLOCKSIZE)) != CRYPT_OK) {
       goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash].process(&hmac->md, isha, hashsize)) != CRYPT_OK) {
       goto LBL_ERR;
    }
    if ((err = hash_descriptor[hash].done(&hmac->md, buf)) != CRYPT_OK) {
       goto LBL_ERR;
    }

    /* copy to output  */
    for (i = 0; i < hashsize && i < *outlen; i++) {
        out[i] = buf[i];
    }
    *outlen = i;

    err = CRYPT_OK;
LBL_ERR:
    XFREE(hmac->key);
#ifdef LTC_CLEAN_STACK
    zeromem(isha, hashsize);
    zeromem(buf,  hashsize);
    zeromem(hmac, sizeof(*hmac));
#endif

    XFREE(isha);
    XFREE(buf);

    return err;
}

/**
  HMAC a block of memory to produce the authentication tag
  @param hash      The index of the hash to use
  @param key       The secret key
  @param keylen    The length of the secret key (octets)
  @param in        The data to HMAC
  @param inlen     The length of the data to HMAC (octets)
  @param out       [out] Destination of the authentication tag
  @param outlen    [in/out] Max size and resulting size of authentication tag
  @return CRYPT_OK if successful
*/
int hmac_memory(int hash,
                const unsigned char *key,  unsigned long keylen,
                const unsigned char *in,   unsigned long inlen,
                      unsigned char *out,  unsigned long *outlen)
{
    hmac_state *hmac;
    int err;

    LTC_ARGCHK(key    != NULL);
    LTC_ARGCHK(in   != NULL);
    LTC_ARGCHK(out    != NULL);
    LTC_ARGCHK(outlen != NULL);

    /* allocate ram for hmac state */
    hmac = (hmac_state *) XMALLOC(sizeof(hmac_state));
    if (hmac == NULL) {
       return CRYPT_MEM;
    }

    if ((err = hmac_init(hmac, hash, key, keylen)) != CRYPT_OK) {
       goto LBL_ERR;
    }

    if ((err = hmac_process(hmac, in, inlen)) != CRYPT_OK) {
       goto LBL_ERR;
    }

    if ((err = hmac_done(hmac, out, outlen)) != CRYPT_OK) {
       goto LBL_ERR;
    }

   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(hmac, sizeof(hmac_state));
#endif

   XFREE(hmac);
   return err;
}

#endif
