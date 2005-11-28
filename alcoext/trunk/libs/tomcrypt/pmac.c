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
  @file pmac.c
  PMAC implementation, by Tom St Denis
*/

#ifdef PMAC

static int pmac_ntz(unsigned long x);
static void pmac_shift_xor(pmac_state *pmac);

/**
  Internal PMAC function
*/
static int pmac_ntz(unsigned long x)
{
   int c;
   x &= 0xFFFFFFFFUL;
   c = 0;
   while ((x & 1) == 0) {
      ++c;
      x >>= 1;
   }
   return c;
}

/**
  Performs the state update (adding correct multiple)
  @param pmac   The PMAC state.
*/
static void pmac_shift_xor(pmac_state *pmac)
{
   int x, y;
   y = pmac_ntz(pmac->block_index++);
#ifdef LTC_FAST
   for (x = 0; x < pmac->block_len; x += sizeof(LTC_FAST_TYPE)) {
       *((LTC_FAST_TYPE*)((unsigned char *)pmac->Li + x)) ^=
       *((LTC_FAST_TYPE*)((unsigned char *)pmac->Ls[y] + x));
   }
#else
   for (x = 0; x < pmac->block_len; x++) {
       pmac->Li[x] ^= pmac->Ls[y][x];
   }
#endif
}

static const struct {
    int           len;
    unsigned char poly_div[MAXBLOCKSIZE],
                  poly_mul[MAXBLOCKSIZE];
} polys[] = {
{
    8,
    { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B }
}, {
    16,
    { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x43 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87 }
}
};

/**
  Initialize a PMAC state
  @param pmac      The PMAC state to initialize
  @param cipher    The index of the desired cipher
  @param key       The secret key
  @param keylen    The length of the secret key (octets)
  @return CRYPT_OK if successful
*/
int pmac_init(pmac_state *pmac, int cipher, const unsigned char *key, unsigned long keylen)
{
   int poly, x, y, m, err;
   unsigned char *L;

   LTC_ARGCHK(pmac  != NULL);
   LTC_ARGCHK(key   != NULL);

   /* valid cipher? */
   if ((err = cipher_is_valid(cipher)) != CRYPT_OK) {
      return err;
   }

   /* determine which polys to use */
   pmac->block_len = cipher_descriptor[cipher].block_length;
   for (poly = 0; poly < (int)(sizeof(polys)/sizeof(polys[0])); poly++) {
       if (polys[poly].len == pmac->block_len) {
          break;
       }
   }
   if (polys[poly].len != pmac->block_len) {
      return CRYPT_INVALID_ARG;
   }

#ifdef LTC_FAST
   if (pmac->block_len % sizeof(LTC_FAST_TYPE)) {
      return CRYPT_INVALID_ARG;
   }
#endif

   /* schedule the key */
   if ((err = cipher_descriptor[cipher].setup(key, keylen, 0, &pmac->key)) != CRYPT_OK) {
      return err;
   }

   /* allocate L */
   L = (unsigned char *) XMALLOC(pmac->block_len);
   if (L == NULL) {
      return CRYPT_MEM;
   }

   /* find L = E[0] */
   zeromem(L, pmac->block_len);
   cipher_descriptor[cipher].ecb_encrypt(L, L, &pmac->key);

   /* find Ls[i] = L << i for i == 0..31 */
   XMEMCPY(pmac->Ls[0], L, pmac->block_len);
   for (x = 1; x < 32; x++) {
       m = pmac->Ls[x-1][0] >> 7;
       for (y = 0; y < pmac->block_len-1; y++) {
           pmac->Ls[x][y] = ((pmac->Ls[x-1][y] << 1) | (pmac->Ls[x-1][y+1] >> 7)) & 255;
       }
       pmac->Ls[x][pmac->block_len-1] = (pmac->Ls[x-1][pmac->block_len-1] << 1) & 255;

       if (m == 1) {
          for (y = 0; y < pmac->block_len; y++) {
              pmac->Ls[x][y] ^= polys[poly].poly_mul[y];
          }
       }
    }

    /* find Lr = L / x */
    m = L[pmac->block_len-1] & 1;

    /* shift right */
    for (x = pmac->block_len - 1; x > 0; x--) {
        pmac->Lr[x] = ((L[x] >> 1) | (L[x-1] << 7)) & 255;
    }
    pmac->Lr[0] = L[0] >> 1;

    if (m == 1) {
       for (x = 0; x < pmac->block_len; x++) {
           pmac->Lr[x] ^= polys[poly].poly_div[x];
       }
    }

    /* zero buffer, counters, etc... */
    pmac->block_index = 1;
    pmac->cipher_idx  = cipher;
    pmac->buflen      = 0;
    zeromem(pmac->block,    sizeof(pmac->block));
    zeromem(pmac->Li,       sizeof(pmac->Li));
    zeromem(pmac->checksum, sizeof(pmac->checksum));

#ifdef LTC_CLEAN_STACK
    zeromem(L, pmac->block_len);
#endif

    XFREE(L);

    return CRYPT_OK;
}

/**
  Process data in a PMAC stream
  @param pmac     The PMAC state
  @param in       The data to send through PMAC
  @param inlen    The length of the data to send through PMAC
  @return CRYPT_OK if successful
*/
int pmac_process(pmac_state *pmac, const unsigned char *in, unsigned long inlen)
{
   int err, n;
   unsigned long x;
   unsigned char Z[MAXBLOCKSIZE];

   LTC_ARGCHK(pmac != NULL);
   LTC_ARGCHK(in   != NULL);
   if ((err = cipher_is_valid(pmac->cipher_idx)) != CRYPT_OK) {
      return err;
   }

   if ((pmac->buflen > (int)sizeof(pmac->block)) || (pmac->buflen < 0) ||
       (pmac->block_len > (int)sizeof(pmac->block)) || (pmac->buflen > pmac->block_len)) {
      return CRYPT_INVALID_ARG;
   }

#ifdef LTC_FAST
   if (pmac->buflen == 0 && inlen > 16) {
      unsigned long y;
      for (x = 0; x < (inlen - 16); x += 16) {
          pmac_shift_xor(pmac);
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)(&Z[y])) = *((LTC_FAST_TYPE*)(&in[y])) ^ *((LTC_FAST_TYPE*)(&pmac->Li[y]));
          }
          cipher_descriptor[pmac->cipher_idx].ecb_encrypt(Z, Z, &pmac->key);
          for (y = 0; y < 16; y += sizeof(LTC_FAST_TYPE)) {
              *((LTC_FAST_TYPE*)(&pmac->checksum[y])) ^= *((LTC_FAST_TYPE*)(&Z[y]));
          }
          in += 16;
      }
      inlen -= x;
   }
#endif

   while (inlen != 0) {
       /* ok if the block is full we xor in prev, encrypt and replace prev */
       if (pmac->buflen == pmac->block_len) {
          pmac_shift_xor(pmac);
          for (x = 0; x < (unsigned long)pmac->block_len; x++) {
               Z[x] = pmac->Li[x] ^ pmac->block[x];
          }
          cipher_descriptor[pmac->cipher_idx].ecb_encrypt(Z, Z, &pmac->key);
          for (x = 0; x < (unsigned long)pmac->block_len; x++) {
              pmac->checksum[x] ^= Z[x];
          }
          pmac->buflen = 0;
       }

       /* add bytes */
       n = MIN(inlen, (unsigned long)(pmac->block_len - pmac->buflen));
       XMEMCPY(pmac->block + pmac->buflen, in, n);
       pmac->buflen  += n;
       inlen         -= n;
       in            += n;
   }

#ifdef LTC_CLEAN_STACK
   zeromem(Z, sizeof(Z));
#endif

   return CRYPT_OK;
}

/**
  Terminate an PMAC session
  @param hmac    The PMAC state
  @param out     [out] The destination of the PMAC authentication tag
  @param outlen  [in/out]  The max size and resulting size of the PMAC authentication tag
  @return CRYPT_OK if successful
*/
int pmac_done(pmac_state *state, unsigned char *out, unsigned long *outlen)
{
   int err, x;

   LTC_ARGCHK(state != NULL);
   LTC_ARGCHK(out   != NULL);
   if ((err = cipher_is_valid(state->cipher_idx)) != CRYPT_OK) {
      return err;
   }

   if ((state->buflen > (int)sizeof(state->block)) || (state->buflen < 0) ||
       (state->block_len > (int)sizeof(state->block)) || (state->buflen > state->block_len)) {
      return CRYPT_INVALID_ARG;
   }


   /* handle padding.  If multiple xor in L/x */

   if (state->buflen == state->block_len) {
      /* xor Lr against the checksum */
      for (x = 0; x < state->block_len; x++) {
          state->checksum[x] ^= state->block[x] ^ state->Lr[x];
      }
   } else {
      /* otherwise xor message bytes then the 0x80 byte */
      for (x = 0; x < state->buflen; x++) {
          state->checksum[x] ^= state->block[x];
      }
      state->checksum[x] ^= 0x80;
   }

   /* encrypt it */
   cipher_descriptor[state->cipher_idx].ecb_encrypt(state->checksum, state->checksum, &state->key);
   cipher_descriptor[state->cipher_idx].done(&state->key);

   /* store it */
   for (x = 0; x < state->block_len && x <= (int)*outlen; x++) {
       out[x] = state->checksum[x];
   }
   *outlen = x;

#ifdef LTC_CLEAN_STACK
   zeromem(state, sizeof(*state));
#endif
   return CRYPT_OK;
}

/**
  PMAC a block of memory
  @param cipher   The index of the cipher desired
  @param key      The secret key
  @param keylen   The length of the secret key (octets)
  @param in       The data you wish to send through PMAC
  @param inlen    The length of data you wish to send through PMAC (octets)
  @param out      [out] Destination for the authentication tag
  @param outlen   [in/out] The max size and resulting size of the authentication tag
  @return CRYPT_OK if successful
*/
int pmac_memory(int cipher,
                const unsigned char *key, unsigned long keylen,
                const unsigned char *in,  unsigned long inlen,
                      unsigned char *out, unsigned long *outlen)
{
   int err;
   pmac_state *pmac;

   LTC_ARGCHK(key    != NULL);
   LTC_ARGCHK(in     != NULL);
   LTC_ARGCHK(out    != NULL);
   LTC_ARGCHK(outlen != NULL);

   /* allocate ram for pmac state */
   pmac = (pmac_state *)XMALLOC(sizeof(pmac_state));
   if (pmac == NULL) {
      return CRYPT_MEM;
   }

   if ((err = pmac_init(pmac, cipher, key, keylen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   if ((err = pmac_process(pmac, in, inlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }
   if ((err = pmac_done(pmac, out, outlen)) != CRYPT_OK) {
      goto LBL_ERR;
   }

   err = CRYPT_OK;
LBL_ERR:
#ifdef LTC_CLEAN_STACK
   zeromem(pmac, sizeof(pmac_state));
#endif

   XFREE(pmac);
   return err;
}

#endif
