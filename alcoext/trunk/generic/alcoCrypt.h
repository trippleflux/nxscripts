/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoCrypt.h

Author:
    neoxed (neoxed@gmail.com) May 6, 2005

Abstract:
    Cryptographic command definitions.

--*/

#ifndef _ALCOCRYPT_H_
#define _ALCOCRYPT_H_

// Flags for CryptCipherMode::options.
#define CRYPT_REQUIRES_IV   0x0001
#define CRYPT_PAD_PLAINTEXT 0x0002

typedef int (CryptModeProc)(
    int cipher,
    int rounds,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest);

typedef struct {
    char *name;
    CryptModeProc *decrypt;
    CryptModeProc *encrypt;
    unsigned short options;
} CryptCipherMode;

// Generic crypt handle.
typedef struct {
    int descIndex;      // Descriptor table index (cipher, hash, or prng).
    unsigned char type; // Type of handle.
    union {
        hash_state    hash;
        hmac_state    hmac;
        omac_state    omac;
        pelican_state pelican;
        pmac_state    pmac;
        prng_state    prng;
    } state;
} CryptHandle;

void
CryptCloseHandles(
    Tcl_HashTable *tablePtr
    );

Tcl_ObjCmdProc CryptObjCmd;

#endif // _ALCOCRYPT_H_
