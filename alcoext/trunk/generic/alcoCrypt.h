/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoCrypt.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 6, 2005
 *
 * Abstract:
 *   Cryptography extension definitions.
 */

#ifndef __ALCOCRYPT_H__
#define __ALCOCRYPT_H__

/* Modes for CryptProcessCmd. */
#define MODE_DECRYPT 1
#define MODE_ENCRYPT 2

/* Flags for CryptCipherMode::options. */
#define CRYPT_REQUIRES_IV   0x0001
#define CRYPT_PAD_PLAINTEXT 0x0002

typedef int (CryptProcessProc)(
    int cipher,
    int rounds,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
);

typedef struct {
    char *name;
    CryptProcessProc *Decrypt;
    CryptProcessProc *Encrypt;
    unsigned short options;
} CryptCipherMode;

/* Hash state type. */
#define CRYPT_HASH 1
#define CRYPT_HMAC 2

/* Hash state structure. */
typedef struct {
    int hashIndex;
    unsigned char type;
    union {
        hash_state hash;
        hmac_state hmac;
    } state;
} CryptHandle;

Tcl_ObjCmdProc CryptObjCmd;
void CryptCloseHandles(Tcl_HashTable *tablePtr);

#endif /* __ALCOCRYPT_H__ */
