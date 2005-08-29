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

typedef int (CryptModeProc)(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    );

typedef struct {
    char *name;               // Name of cipher mode.
    CryptModeProc *decrypt;   // Pointer to the cipher mode's decryption function.
    CryptModeProc *encrypt;   // Pointer to the cipher mode's encryption function.
    unsigned char requiresIv; // Boolean to indicate if the cipher mode requires
} CryptCipherMode;            // an initialisation vector.

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
