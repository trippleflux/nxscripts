/* Based on the test-vector generator included with LibTomCrypt. */

#include <tomcrypt.h>

void reg_algs(void)
{
    register_cipher(&des3_desc);
    register_cipher(&aes_desc);
    register_cipher(&anubis_desc);
    register_cipher(&blowfish_desc);
    register_cipher(&cast5_desc);
    register_cipher(&des_desc);
    register_cipher(&khazad_desc);
    register_cipher(&noekeon_desc);
    register_cipher(&rc2_desc);
    register_cipher(&rc5_desc);
    register_cipher(&rc6_desc);
    register_cipher(&saferp_desc);
    register_cipher(&safer_k128_desc);
    register_cipher(&safer_k64_desc);
    register_cipher(&safer_sk128_desc);
    register_cipher(&safer_sk64_desc);
    register_cipher(&skipjack_desc);
    register_cipher(&twofish_desc);
    register_cipher(&xtea_desc);
    register_hash(&md2_desc);
    register_hash(&md4_desc);
    register_hash(&md5_desc);
    register_hash(&rmd128_desc);
    register_hash(&rmd160_desc);
    register_hash(&rmd256_desc);
    register_hash(&rmd320_desc);
    register_hash(&sha1_desc);
    register_hash(&sha224_desc);
    register_hash(&sha256_desc);
    register_hash(&sha384_desc);
    register_hash(&sha512_desc);
    register_hash(&tiger_desc);
    register_hash(&whirlpool_desc);
}

void cipher_gen(void)
{
   unsigned char *key, pt[MAXBLOCKSIZE];
   unsigned long x, y, z, w;
   int err, kl, lastkl;
   FILE *out;
   symmetric_key skey;

   out = fopen("crypt-cipher.tv", "w");

   for (x = 0; cipher_descriptor[x].name != NULL; x++) {
      fprintf(out, "set cipherTest(%s) {\n", cipher_descriptor[x].name);

      /* three modes, smallest, medium, large keys */
      lastkl = 10000;
      for (y = 0; y < 3; y++) {
         switch (y) {
            case 0: kl = cipher_descriptor[x].min_key_length; break;
            case 1: kl = (cipher_descriptor[x].min_key_length + cipher_descriptor[x].max_key_length)/2; break;
            case 2: kl = cipher_descriptor[x].max_key_length; break;
         }
         if ((err = cipher_descriptor[x].keysize(&kl)) != CRYPT_OK) {
            printf("keysize error: %s\n", error_to_string(err));
            exit(EXIT_FAILURE);
         }
         if (kl == lastkl) break;
         lastkl = kl;

         key = XMALLOC(kl);
         if (key == NULL) {
            perror("can't malloc memory");
            exit(EXIT_FAILURE);
         }

         for (z = 0; (int)z < kl; z++) {
             key[z] = (unsigned char)z;
         }
         if ((err = cipher_descriptor[x].setup(key, kl, 0, &skey)) != CRYPT_OK) {
            printf("setup error: %s\n", error_to_string(err));
            exit(EXIT_FAILURE);
         }

         for (z = 0; (int)z < cipher_descriptor[x].block_length; z++) {
            pt[z] = (unsigned char)z;
         }
         for (w = 0; w < 50; w++) {
             /* print key */
             fprintf(out, "\"");
             for (z = 0; (int)z < kl; z++) {
                fprintf(out, "\\x%02X", key[z]);
             }
             fprintf(out, "\" ");

             /* print plain text */
             fprintf(out, "\"");
             for (z = 0; (int)z < cipher_descriptor[x].block_length; z++) {
                fprintf(out, "\\x%02X", pt[z]);
             }
             fprintf(out, "\" ");

             cipher_descriptor[x].ecb_encrypt(pt, pt, &skey);

             /* print cipher text */
             fprintf(out, "\"");
             for (z = 0; (int)z < cipher_descriptor[x].block_length; z++) {
                fprintf(out, "\\x%02X", pt[z]);
             }
             fprintf(out, "\"\n");

             /* reschedule a new key */
             for (z = 0; z < (unsigned long)kl; z++) {
                 key[z] = pt[z % cipher_descriptor[x].block_length];
             }
             if ((err = cipher_descriptor[x].setup(key, kl, 0, &skey)) != CRYPT_OK) {
                printf("cipher setup2 error: %s\n", error_to_string(err));
                exit(EXIT_FAILURE);
             }
         }
         fprintf(out, "\n");
         XFREE(key);
     }
     fprintf(out, "}\n\n");
  }
  fclose(out);
}

void hash_gen(void)
{
   unsigned char md[MAXBLOCKSIZE], *buf;
   unsigned long outlen, x, y, z;
   FILE *out;
   int   err;

   out = fopen("crypt-hash.tv", "w");
   if (out == NULL) {
      perror("can't open hash_tv");
   }

   for (x = 0; hash_descriptor[x].name != NULL; x++) {
      buf = XMALLOC(2 * hash_descriptor[x].blocksize + 1);
      if (buf == NULL) {
         perror("can't alloc mem");
         exit(EXIT_FAILURE);
      }
      fprintf(out, "set hashTest(%s) {\n", hash_descriptor[x].name);
      for (y = 0; y <= (hash_descriptor[x].blocksize * 2); y++) {
         for (z = 0; z < y; z++) {
            buf[z] = (unsigned char)(z & 255);
         }
         outlen = sizeof(md);
         if ((err = hash_memory(x, buf, y, md, &outlen)) != CRYPT_OK) {
            printf("hash_memory error: %s\n", error_to_string(err));
            exit(EXIT_FAILURE);
         }

         /* print plain text */
         fprintf(out, "\"");
         for (z = 0; z < y; z++) {
            fprintf(out, "\\x%02X", buf[z]);
         }
         fprintf(out, "\" ");

         /* print hash */
         fprintf(out, "\"");
         for (z = 0; z < outlen; z++) {
            fprintf(out, "\\x%02X", md[z]);
         }
         fprintf(out, "\"\n");
      }
      fprintf(out, "}\n\n");
      XFREE(buf);
   }
   fclose(out);
}

void hmac_gen(void)
{
   unsigned char key[MAXBLOCKSIZE], output[MAXBLOCKSIZE], *input;
   int x, y, z, err;
   FILE *out;
   unsigned long len;

   out = fopen("crypt-hmac.tv", "w");

   for (x = 0; hash_descriptor[x].name != NULL; x++) {
      fprintf(out, "set hmacTest(%s) {\n", hash_descriptor[x].name);

      /* initial key */
      for (y = 0; y < (int)hash_descriptor[x].hashsize; y++) {
          key[y] = (y&255);
      }

      input = XMALLOC(hash_descriptor[x].blocksize * 2 + 1);
      if (input == NULL) {
         perror("Can't malloc memory");
         exit(EXIT_FAILURE);
      }

      for (y = 0; y <= (int)(hash_descriptor[x].blocksize * 2); y++) {
         for (z = 0; z < y; z++) {
            input[z] = (unsigned char)(z & 255);
         }
         len = sizeof(output);
         if ((err = hmac_memory(x, key, hash_descriptor[x].hashsize, input, y, output, &len)) != CRYPT_OK) {
            printf("Error hmacing: %s\n", error_to_string(err));
            exit(EXIT_FAILURE);
         }

         /* print key */
         fprintf(out, "\"");
         for (z = 0; z <(int) hash_descriptor[x].hashsize; z++) {
            fprintf(out, "\\x%02X", key[z]);
         }
         fprintf(out, "\" ");

         /* print plain text */
         fprintf(out, "\"");
         for (z = 0; z <(int) y; z++) {
            fprintf(out, "\\x%02X", input[z]);
         }
         fprintf(out, "\" ");

         /* print hash */
         fprintf(out, "\"");
         for (z = 0; z <(int) len; z++) {
            fprintf(out, "\\x%02X", output[z]);
         }
         fprintf(out, "\"\n");

         /* forward the key */
         memcpy(key, output, hash_descriptor[x].hashsize);
      }
      XFREE(input);
      fprintf(out, "}\n\n");
   }
   fclose(out);
}

void omac_gen(void)
{
   unsigned char key[MAXBLOCKSIZE], output[MAXBLOCKSIZE], input[MAXBLOCKSIZE*2+2];
   int err, x, y, z, kl;
   FILE *out;
   unsigned long len;

   out = fopen("crypt-omac.tv", "w");

   for (x = 0; cipher_descriptor[x].name != NULL; x++) {
      kl = cipher_descriptor[x].block_length;

      /* skip ciphers which do not have 64 or 128 bit block sizes */
      if (kl != 8 && kl != 16) continue;

      if (cipher_descriptor[x].keysize(&kl) != CRYPT_OK) {
         kl = cipher_descriptor[x].max_key_length;
      }
      fprintf(out, "set omacTest(%s) {\n", cipher_descriptor[x].name);

      /* initial key/block */
      for (y = 0; y < kl; y++) {
          key[y] = (y & 255);
      }

      for (y = 0; y <= (int)(cipher_descriptor[x].block_length*2); y++) {
         for (z = 0; z < y; z++) {
            input[z] = (unsigned char)(z & 255);
         }
         len = sizeof(output);
         if ((err = omac_memory(x, key, kl, input, y, output, &len)) != CRYPT_OK) {
            printf("Error omacing: %s\n", error_to_string(err));
            exit(EXIT_FAILURE);
         }

         /* print key */
         fprintf(out, "\"");
         for (z = 0; z < kl; z++) {
            fprintf(out, "\\x%02X", key[z]);
         }
         fprintf(out, "\" ");

         /* print plain text */
         fprintf(out, "\"");
         for (z = 0; z <(int) y; z++) {
            fprintf(out, "\\x%02X", input[z]);
         }
         fprintf(out, "\" ");

         /* print hash */
         fprintf(out, "\"");
         for (z = 0; z <(int) len; z++) {
            fprintf(out, "\\x%02X", output[z]);
         }
         fprintf(out, "\"\n");

         /* forward the key */
         for (z = 0; z < kl; z++) {
             key[z] = output[z % len];
         }
      }
      fprintf(out, "}\n\n");
   }
   fclose(out);
}

void pmac_gen(void)
{
   unsigned char key[MAXBLOCKSIZE], output[MAXBLOCKSIZE], input[MAXBLOCKSIZE*2+2];
   int err, x, y, z, kl;
   FILE *out;
   unsigned long len;

   out = fopen("crypt-pmac.tv", "w");

   for (x = 0; cipher_descriptor[x].name != NULL; x++) {
      kl = cipher_descriptor[x].block_length;

      /* skip ciphers which do not have 64 or 128 bit block sizes */
      if (kl != 8 && kl != 16) continue;

      if (cipher_descriptor[x].keysize(&kl) != CRYPT_OK) {
         kl = cipher_descriptor[x].max_key_length;
      }
      fprintf(out, "set pmacTest(%s) {\n", cipher_descriptor[x].name);

      /* initial key/block */
      for (y = 0; y < kl; y++) {
          key[y] = (y & 255);
      }

      for (y = 0; y <= (int)(cipher_descriptor[x].block_length*2); y++) {
         for (z = 0; z < y; z++) {
            input[z] = (unsigned char)(z & 255);
         }
         len = sizeof(output);
         if ((err = pmac_memory(x, key, kl, input, y, output, &len)) != CRYPT_OK) {
            printf("Error omacing: %s\n", error_to_string(err));
            exit(EXIT_FAILURE);
         }

         /* print key */
         fprintf(out, "\"");
         for (z = 0; z < kl; z++) {
            fprintf(out, "\\x%02X", key[z]);
         }
         fprintf(out, "\" ");

         /* print plain text */
         fprintf(out, "\"");
         for (z = 0; z <(int) y; z++) {
            fprintf(out, "\\x%02X", input[z]);
         }
         fprintf(out, "\" ");

         /* print hash */
         fprintf(out, "\"");
         for (z = 0; z <(int) len; z++) {
            fprintf(out, "\\x%02X", output[z]);
         }
         fprintf(out, "\"\n");

         /* forward the key */
         for (z = 0; z < kl; z++) {
             key[z] = output[z % len];
         }
      }
      fprintf(out, "}\n\n");
   }
   fclose(out);
}

int main(void)
{
   reg_algs();
   printf("Generating hash   vectors..."); fflush(stdout); hash_gen(); printf("done\n");
   printf("Generating cipher vectors..."); fflush(stdout); cipher_gen(); printf("done\n");
   printf("Generating HMAC   vectors..."); fflush(stdout); hmac_gen(); printf("done\n");
   printf("Generating OMAC   vectors..."); fflush(stdout); omac_gen(); printf("done\n");
   printf("Generating PMAC   vectors..."); fflush(stdout); pmac_gen(); printf("done\n");
   return 0;
}
