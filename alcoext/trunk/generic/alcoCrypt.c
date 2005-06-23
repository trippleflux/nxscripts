/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoCrypt.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 6, 2005
 *
 * Abstract:
 *   Cryptography extension, providing hashing and encryption support.
 *
 *   Cipher Modes:
 *     cbc - Cipher Block Chaining
 *     cfb - Cipher Feedback
 *     ctr - Counter
 *     ecb - Electronic Codebook
 *     ofb - Output Feedback
 *
 *   Cipher Commands:
 *     crypt decrypt [-iv <iv>] [-mode <mode>] [-rounds <count>] <cipher> <key> <data>
 *     crypt encrypt [-iv <iv>] [-mode <mode>] [-rounds <count>] <cipher> <key> <data>
 *
 *   Hash Commands:
 *     crypt hash    [-hmac <key>] <hash algorithm> <data>
 *     crypt start   [-hmac <key>] <hash algorithm>
 *     crypt update  <handle> <data>
 *     crypt end     <handle>
 *
 *   Other Commands:
 *     crypt info    <ciphers|handles|hashes|modes>
 *     crypt pkcs5   [-rounds <count>] <hash algorithm> <salt> <password>
 *     crypt rand    <bytes>
 */

#include <alcoExt.h>

/* Round 'a' up to a multiple of 'b'. */
#ifdef ROUNDUP
#undef ROUNDUP
#endif
#define ROUNDUP(a,b) ((((a) + ((b) - 1)) / (b)) * (b))

static int CryptProcessCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], unsigned char mode);
static int CryptHashCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int CryptStartCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int CryptUpdateCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int CryptEndCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int CryptInfoCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int CryptPkcs5Cmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int CryptRandCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

static CryptProcessProc DecryptCBC;
static CryptProcessProc DecryptCFB;
static CryptProcessProc DecryptCTR;
static CryptProcessProc DecryptECB;
static CryptProcessProc DecryptOFB;
static CryptProcessProc EncryptCBC;
static CryptProcessProc EncryptCFB;
static CryptProcessProc EncryptCTR;
static CryptProcessProc EncryptECB;
static CryptProcessProc EncryptOFB;

static const CryptCipherMode cipherModes[] = {
    {"cbc", DecryptCBC, EncryptCBC, CRYPT_REQUIRES_IV|CRYPT_PAD_PLAINTEXT},
    {"cfb", DecryptCFB, EncryptCFB, CRYPT_REQUIRES_IV},
    {"ctr", DecryptCTR, EncryptCTR, CRYPT_REQUIRES_IV},
    {"ecb", DecryptECB, EncryptECB, CRYPT_PAD_PLAINTEXT},
    {"ofb", DecryptOFB, EncryptOFB, CRYPT_REQUIRES_IV},
    {NULL}
};

/*
 * TODO:
 * - Zero sensitive stack/heap data.
 * - Check if there's a way to zero Tcl command objects (objv) without breaking something.
 */


/*
 * DecryptCBC
 * DecryptCFB
 * DecryptCTR
 * DecryptECB
 * DecryptOFB
 *
 *   Decrypts data from the specified cipher mode and algorithm.
 *
 * Arguments:
 *   cipher     - Index of the desired cipher.
 *   rounds     - Number of rounds.
 *   iv         - The initial vector, must be the length of one block.
 *   key        - The secret key.
 *   keyLength  - Length of the secret key.
 *   data       - Cipher text (data to decrypt).
 *   dataLength - Length of cipher text.
 *   dest       - Buffer to receive plain text (decrypted data).
 *
 * Returns:
 *   A LibTomCrypt status code; CRYPT_OK will be returned if successful.
 *
 * Remarks:
 *   None.
 */

static int
DecryptCBC(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_CBC state;

    status = cbc_start(cipher, iv, key, keyLength, rounds, &state);

    if (status == CRYPT_OK) {
        status = cbc_decrypt(data, dest, dataLength, &state);
        cbc_done(&state);
    }

    return status;
}

static int
DecryptCFB(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_CFB state;

    status = cfb_start(cipher, iv, key, keyLength, rounds, &state);

    if (status == CRYPT_OK) {
        status = cfb_decrypt(data, dest, dataLength, &state);
        cfb_done(&state);
    }

    return status;
}

static int
DecryptCTR(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_CTR state;

    status = ctr_start(cipher, iv, key, keyLength, rounds, CTR_COUNTER_LITTLE_ENDIAN, &state);

    if (status == CRYPT_OK) {
        status = ctr_decrypt(data, dest, dataLength, &state);
        ctr_done(&state);
    }

    return status;
}

static int
DecryptECB(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_ECB state;

    status = ecb_start(cipher, key, keyLength, rounds, &state);

    if (status == CRYPT_OK) {
        status = ecb_decrypt(data, dest, dataLength, &state);
        ecb_done(&state);
    }

    return status;
}

static int
DecryptOFB(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_OFB state;

    status = ofb_start(cipher, iv, key, keyLength, rounds, &state);

    if (status == CRYPT_OK) {
        status = ofb_decrypt(data, dest, dataLength, &state);
        ofb_done(&state);
    }

    return status;
}


/*
 * EncryptCBC
 * EncryptCFB
 * EncryptCTR
 * EncryptECB
 * EncryptOFB
 *
 *   Encrypts data in the specified cipher mode and algorithm.
 *
 * Arguments:
 *   cipher     - Index of the desired cipher.
 *   rounds     - Number of rounds.
 *   iv         - The initial vector, must be the length of one block.
 *   key        - The secret key.
 *   keyLength  - Length of the secret key.
 *   data       - Plain text (data to encrypt).
 *   dataLength - Length of plain text.
 *   dest       - Buffer to receive cipher text (encrypted data).
 *
 * Returns:
 *   A LibTomCrypt status code; CRYPT_OK will be returned if successful.
 *
 * Remarks:
 *   None.
 */

static int
EncryptCBC(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_CBC state;

    status = cbc_start(cipher, iv, key, keyLength, rounds, &state);

    if (status == CRYPT_OK) {
        status = cbc_encrypt(data, dest, dataLength, &state);
        cbc_done(&state);
    }

    return status;
}

static int
EncryptCFB(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_CFB state;

    status = cfb_start(cipher, iv, key, keyLength, rounds, &state);

    if (status == CRYPT_OK) {
        status = cfb_encrypt(data, dest, dataLength, &state);
        cfb_done(&state);
    }

    return status;
}

static int
EncryptCTR(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_CTR state;

    status = ctr_start(cipher, iv, key, keyLength, rounds, CTR_COUNTER_LITTLE_ENDIAN, &state);

    if (status == CRYPT_OK) {
        status = ctr_encrypt(data, dest, dataLength, &state);
        ctr_done(&state);
    }

    return status;
}

static int
EncryptECB(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_ECB state;

    status = ecb_start(cipher, key, keyLength, rounds, &state);

    if (status == CRYPT_OK) {
        status = ecb_encrypt(data, dest, dataLength, &state);
        ecb_done(&state);
    }

    return status;
}

static int
EncryptOFB(int cipher, int rounds, unsigned char *iv, unsigned char *key, unsigned long keyLength,
    unsigned char *data, unsigned long dataLength, unsigned char *dest)
{
    int status;
    symmetric_OFB state;

    status = ofb_start(cipher, iv, key, keyLength, rounds, &state);

    if (status == CRYPT_OK) {
        status = ofb_encrypt(data, dest, dataLength, &state);
        ofb_done(&state);
    }

    return status;
}


/*
 * CryptProcessCmd
 *
 *   Encrypts or decrypts data with the specified cipher algorithm.
 *
 * Arguments:
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *   mode   - Must be either 'MODE_DECRYPT' or 'MODE_ENCRYPT'.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

static int
CryptProcessCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], unsigned char mode)
{
    int i;
    int index;
    int cipherIndex;
    int rounds = 0;
    int modeIndex = 3; /* ECB mode by default. */
    int status = CRYPT_ERROR;
    int dataLength;
    int ivLength = 0;
    int keyLength;
    unsigned char *data;
    unsigned char *dest;
    unsigned char *iv = NULL;
    unsigned char *key;
    static const char *switches[] = {"-iv", "-mode", "-rounds", NULL};
    enum switches {SWITCH_IV, SWITCH_MODE, SWITCH_ROUNDS};

    for (i = 2; i < objc; i++) {
        char *name = Tcl_GetString(objv[i]);

        if (name[0] != '-') {
            break;
        }

        if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }

        i++;
        switch ((enum switches) index) {
            case SWITCH_IV: {
                iv = Tcl_GetByteArrayFromObj(objv[i], &ivLength);
                break;
            }
            case SWITCH_MODE: {
                if (Tcl_GetIndexFromObjStruct(interp, objv[i], cipherModes,
                    sizeof(cipherModes[0]), "mode", TCL_EXACT, &modeIndex) != TCL_OK) {
                    return TCL_ERROR;
                }
                break;
            }
            case SWITCH_ROUNDS: {
                if (Tcl_GetIntFromObj(interp, objv[i], &rounds) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (rounds < 0) {
                    Tcl_AppendResult(interp, "invalid round count \"",
                        Tcl_GetString(objv[3]), "\": must be 0 or greater", NULL);
                    return TCL_ERROR;
                }
                break;
            }
        }
    }

    if (objc - i != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "?switches? cipher key data");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[objc-3], cipher_descriptor,
        sizeof(cipher_descriptor[0]), "cipher", TCL_EXACT, &cipherIndex) != TCL_OK) {
        return TCL_ERROR;
    }

    /* Certain cipher modes require an IV and others do not (i.e. ECB). */
    if (iv != NULL) {
        if (!(cipherModes[modeIndex].options & CRYPT_REQUIRES_IV)) {
            Tcl_AppendResult(interp, cipherModes[modeIndex].name,
                " mode does not require an initialisation vector", NULL);
            return TCL_ERROR;
        }

        /* Verify the IV's length. */
        if (ivLength != cipher_descriptor[cipherIndex].block_length) {
            char length[12];

#ifdef __WIN32__
        StringCchPrintfA(length, 12, "%d", cipher_descriptor[cipherIndex].block_length);
#else /* __WIN32__ */
        snprintf(length, 12, "%d", cipher_descriptor[cipherIndex].block_length);
        length[11] = '\0';
#endif /* __WIN32__ */

            Tcl_AppendResult(interp, "invalid initialisation vector size: must be ",
                length, " bytes", NULL);
            return TCL_ERROR;
        }

    } else if (cipherModes[modeIndex].options & CRYPT_REQUIRES_IV) {
        Tcl_AppendResult(interp, cipherModes[modeIndex].name,
            " mode requires an initialisation vector", NULL);
        return TCL_ERROR;
    }

    /*
     * Verify the given key's length. If the key exceeds the
     * maximum length, it will be truncated without warning.
     */
    key = Tcl_GetByteArrayFromObj(objv[objc-2], &keyLength);
    if (cipher_descriptor[cipherIndex].keysize(&keyLength) != CRYPT_OK) {
        char message[64];

        if (cipher_descriptor[cipherIndex].min_key_length ==
            cipher_descriptor[cipherIndex].max_key_length) {

            /* Fixed key length. */
#ifdef __WIN32__
            StringCchPrintfA(message, 64, "%d", cipher_descriptor[cipherIndex].min_key_length);
#else /* __WIN32__ */
            snprintf(message, 64, "%d", cipher_descriptor[cipherIndex].min_key_length);
            message[63] = '\0';
#endif /* __WIN32__ */

        } else {
            /* Ranging key length. */
#ifdef __WIN32__
            StringCchPrintfA(message, 64, "between %d and %d",
                cipher_descriptor[cipherIndex].min_key_length,
                cipher_descriptor[cipherIndex].max_key_length);
#else /* __WIN32__ */
            snprintf(message, 64, "between %d and %d",
                cipher_descriptor[cipherIndex].min_key_length,
                cipher_descriptor[cipherIndex].max_key_length);
            message[63] = '\0';
#endif /* __WIN32__ */

        }

        Tcl_AppendResult(interp, "invalid key size: must be ", message, " bytes", NULL);
        return TCL_ERROR;
    }

    data = Tcl_GetByteArrayFromObj(objv[objc-1], &dataLength);

    /* Pad the buffer to a multiple of the cipher's block-length.*/
    if (mode == MODE_ENCRYPT && cipherModes[modeIndex].options & CRYPT_PAD_PLAINTEXT) {
        int padLength;
        unsigned char *pad;

        padLength = ROUNDUP(dataLength, cipher_descriptor[cipherIndex].block_length);
        pad = ckalloc(padLength);

        /* Copy data and zero-pad the remaining bytes. */
        for (i = 0; i < dataLength; i++) {
            pad[i] = data[i];
        }
        for (; i < padLength; i++) {
            pad[i] = 0;
        }

        data = pad;
        dataLength = padLength;
    }

    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), dataLength);

    if (mode == MODE_DECRYPT) {
        status = cipherModes[modeIndex].Decrypt(
            cipherIndex, rounds, iv,
            key, (unsigned long)keyLength,
            data, (unsigned long)dataLength,
            dest);

    } else if (mode == MODE_ENCRYPT) {
        status = cipherModes[modeIndex].Encrypt(
            cipherIndex, rounds, iv,
            key, (unsigned long)keyLength,
            data, (unsigned long)dataLength,
            dest);

        if (cipherModes[modeIndex].options & CRYPT_PAD_PLAINTEXT) {
            ckfree(data);
        }
    }

    if (status != CRYPT_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to process data: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}


/*
 * CryptHashCmd
 *
 *   Hashes data using the specified hash algorithm.
 *
 * Arguments:
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

static int
CryptHashCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int dataLength;
    int hashIndex;
    int status;
    unsigned char *data;
    unsigned char *dest;
    unsigned char hmac = 0;
    unsigned long destLength;

    switch (objc) {
        case 4: {
            break;
        }
        case 6: {
            if (PartialSwitchCompare(objv[2], "-hmac")) {
                hmac = 1;
                break;
            }
        }
        default: {
            Tcl_WrongNumArgs(interp, 2, objv, "?-hmac key? hash data");
            return TCL_ERROR;
        }
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[objc-2], hash_descriptor,
        sizeof(hash_descriptor[0]), "hash", TCL_EXACT, &hashIndex) != TCL_OK) {
        return TCL_ERROR;
    }

    data = Tcl_GetByteArrayFromObj(objv[objc-1], &dataLength);

    /* Create a destination byte object to hold the hash digest. */
    destLength = hash_descriptor[hashIndex].hashsize;
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    if (hmac) {
        unsigned char *key;
        int keyLength;

        key = Tcl_GetByteArrayFromObj(objv[3], &keyLength);

        status = hmac_memory(hashIndex,
            key, (unsigned long)keyLength,
            data, (unsigned long)dataLength,
            dest, &destLength);
    } else {
        status = hash_memory(hashIndex,
            data, (unsigned long)dataLength,
            dest, &destLength);
    }

    if (status != CRYPT_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "couldn't hash data: ", error_to_string(status), NULL);
        return TCL_ERROR;
    }

    /* Update the object's length. */
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    return TCL_OK;
}


/*
 * CryptStartCmd
 *
 *   Initialise a hash state.
 *
 * Arguments:
 *   interp   - Current interpreter.
 *   objc     - Number of arguments.
 *   objv     - Argument objects.
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

static int
CryptStartCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr)
{
    char handleId[20];
    int hashIndex;
    int newEntry;
    int status = CRYPT_ERROR;
    unsigned char type;
    CryptHandle *handlePtr;
    Tcl_HashEntry *hashEntryPtr;

    switch (objc) {
        case 3: {
            type = CRYPT_HASH;
            break;
        }
        case 5: {
            if (PartialSwitchCompare(objv[2], "-hmac")) {
                type = CRYPT_HMAC;
                break;
            }
        }
        default: {
            Tcl_WrongNumArgs(interp, 2, objv, "?-hmac key? hash");
            return TCL_ERROR;
        }
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[objc-1], hash_descriptor,
        sizeof(hash_descriptor[0]), "hash", TCL_EXACT, &hashIndex) != TCL_OK) {
        return TCL_ERROR;
    }

    handlePtr = (CryptHandle *) ckalloc(sizeof(CryptHandle));
    handlePtr->hashIndex = hashIndex;
    handlePtr->type = type;

    /* Initialise hash/hmac state. */
    if (type == CRYPT_HASH) {
        status = hash_descriptor[hashIndex].init(&handlePtr->state.hash);

    } else if (type == CRYPT_HMAC) {
        unsigned char *key;
        int keyLength;

        key = Tcl_GetByteArrayFromObj(objv[3], &keyLength);
        status = hmac_init(&handlePtr->state.hmac, hashIndex, key, (unsigned long)keyLength);
    }

    if (status != CRYPT_OK) {
        ckfree((char *) handlePtr);
        Tcl_AppendResult(interp, "unable to initialise hash state: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

#ifdef __WIN32__
    StringCchPrintfA(handleId, 20, "crypt%lu", statePtr->cryptHandle);
#else /* __WIN32__ */
    snprintf(handleId, 20, "crypt%lu", statePtr->cryptHandle);
    handleId[19] = '\0';
#endif /* __WIN32__ */

    statePtr->cryptHandle++;

    hashEntryPtr = Tcl_CreateHashEntry(statePtr->cryptTable, handleId, &newEntry);
    Tcl_SetHashValue(hashEntryPtr, (ClientData) handlePtr);

    Tcl_SetStringObj(Tcl_GetObjResult(interp), handleId, -1);
    return TCL_OK;
}


/*
 * CryptUpdateCmd
 *
 *   Process a block of data.
 *
 * Arguments:
 *   interp   - Current interpreter.
 *   objc     - Number of arguments.
 *   objv     - Argument objects.
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

static int
CryptUpdateCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr)
{
    int dataLength;
    int status = CRYPT_ERROR;
    unsigned char *data;
    CryptHandle *handlePtr;
    Tcl_HashEntry *hashEntryPtr;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "handle data");
        return TCL_ERROR;
    }

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->cryptTable, "crypt");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (CryptHandle *) Tcl_GetHashValue(hashEntryPtr);

    data = Tcl_GetByteArrayFromObj(objv[3], &dataLength);

    /* Update a hash/hmac state. */
    if (handlePtr->type == CRYPT_HASH) {
        status = hash_descriptor[handlePtr->hashIndex].process(&handlePtr->state.hash,
            data, (unsigned long)dataLength);
    } else if (handlePtr->type == CRYPT_HMAC) {
        status = hmac_process(&handlePtr->state.hmac, data, (unsigned long)dataLength);
    }

    if (status != CRYPT_OK) {
        Tcl_AppendResult(interp, "unable to update hash state: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}


/*
 * CryptEndCmd
 *
 *   Finalize a hash state, returning the digest.
 *
 * Arguments:
 *   interp   - Current interpreter.
 *   objc     - Number of arguments.
 *   objv     - Argument objects.
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

static int
CryptEndCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr)
{
    int status = CRYPT_ERROR;
    unsigned char *dest;
    unsigned long destLength;
    CryptHandle *handlePtr;
    Tcl_HashEntry *hashEntryPtr;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "handle");
        return TCL_ERROR;
    }

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->cryptTable, "crypt");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (CryptHandle *) Tcl_GetHashValue(hashEntryPtr);

    /* Create a destination byte object to hold the hash digest. */
    destLength = hash_descriptor[handlePtr->hashIndex].hashsize;
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    /* Finalize a hash/hmac state. */
    if (handlePtr->type == CRYPT_HASH) {
        status = hash_descriptor[handlePtr->hashIndex].done(&handlePtr->state.hash, dest);
    } else if (handlePtr->type == CRYPT_HMAC) {
        status = hmac_done(&handlePtr->state.hmac, dest, &destLength);
    }

    /* Free handle structure and remove the hash table entry. */
    ckfree((char *) handlePtr);
    Tcl_DeleteHashEntry(hashEntryPtr);

    if (status != CRYPT_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to finalize hash state: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

    /* Update the object's length. */
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    return TCL_OK;
}


/*
 * CryptCloseHandles
 *
 *   Close all 'crypt' handles in the given hash table.
 *
 * Arguments:
 *   tablePtr - Hash table of 'crypt' handles.
 *
 * Returns:
 *   None.
 *
 * Remarks:
 *   None.
 */

void
CryptCloseHandles(Tcl_HashTable *tablePtr)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;

    for (entryPtr = Tcl_FirstHashEntry(tablePtr, &search); entryPtr != NULL;
         entryPtr = Tcl_NextHashEntry(&search)) {

        ckfree((char *) Tcl_GetHashValue(entryPtr));
        Tcl_DeleteHashEntry(entryPtr);
    }
}


/*
 * CryptInfoCmd
 *
 *   Retrieves information about the cryptography extension.
 *
 * Arguments:
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *   statePtr - Pointer to a 'ExtState' structure.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

static int
CryptInfoCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr)
{
    int index;
    Tcl_Obj *resultPtr;
    static const char *options[] = {"ciphers", "handles", "hashes", "modes", NULL};
    enum options {OPTION_CIPHERS, OPTION_HANDLES, OPTION_HASHES, OPTION_MODES};

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "option");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    resultPtr = Tcl_GetObjResult(interp);

    switch ((enum options) index) {
        case OPTION_CIPHERS: {
            /* Create a list of supported ciphers. */
            for (index = 0; index < TAB_SIZE && cipher_descriptor[index].name != NULL; index++) {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    Tcl_NewStringObj(cipher_descriptor[index].name, -1));
            }
            return TCL_OK;
        }
        case OPTION_HANDLES: {
            char *name;
            Tcl_HashEntry *hashEntryPtr;
            Tcl_HashSearch hashSearch;

            /* Create a list of open crypt handles. */
            for (hashEntryPtr = Tcl_FirstHashEntry(statePtr->cryptTable, &hashSearch);
                hashEntryPtr != NULL;
                hashEntryPtr = Tcl_NextHashEntry(&hashSearch)) {

                name = Tcl_GetHashKey(statePtr->cryptTable, hashEntryPtr);
                Tcl_ListObjAppendElement(NULL, resultPtr, Tcl_NewStringObj(name, -1));
            }
            return TCL_OK;
        }
        case OPTION_HASHES: {
            /* Create a list of supported hashes. */
            for (index = 0; index < TAB_SIZE && hash_descriptor[index].name != NULL; index++) {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    Tcl_NewStringObj(hash_descriptor[index].name, -1));
            }
            return TCL_OK;
        }
        case OPTION_MODES: {
            /* Create a list of supported cipher modes. */
            for (index = 0; cipherModes[index].name != NULL; index++) {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    Tcl_NewStringObj(cipherModes[index].name, -1));
            }
            return TCL_OK;
        }
    }

    /* This point should never be reached. */
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}


/*
 * CryptPkcs5Cmd
 *
 *   Create a PKCS #5 v2 compliant hash.
 *
 * Arguments:
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

static int
CryptPkcs5Cmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int hashIndex;
    int rounds = 0;
    int passLength;
    int saltLength;
    int status;
    unsigned char *dest;
    unsigned char *pass;
    unsigned char *salt;
    unsigned long destLength;

    switch (objc) {
        case 5: {
            break;
        }
        case 7: {
            if (PartialSwitchCompare(objv[2], "-rounds")) {
                if (Tcl_GetIntFromObj(interp, objv[3], &rounds) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (rounds < 0) {
                    Tcl_AppendResult(interp, "invalid round count \"",
                        Tcl_GetString(objv[3]), "\": must be 0 or greater", NULL);
                    return TCL_ERROR;
                }
                break;
            }
        }
        default: {
            Tcl_WrongNumArgs(interp, 2, objv, "?-rounds count? hash salt password");
            return TCL_ERROR;
        }
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[objc-3], hash_descriptor,
        sizeof(hash_descriptor[0]), "hash", TCL_EXACT, &hashIndex) != TCL_OK) {
        return TCL_ERROR;
    }

    salt = Tcl_GetByteArrayFromObj(objv[objc-2], &saltLength);
    pass = Tcl_GetByteArrayFromObj(objv[objc-1], &passLength);

    /* Create a destination byte object to hold the hash digest. */
    destLength = hash_descriptor[hashIndex].hashsize;
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    status = pkcs_5_alg2(pass, (unsigned long)passLength,
                         salt, (unsigned long)saltLength,
                         rounds, hashIndex, dest, &destLength);

    if (status != CRYPT_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to hash password: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

    /* Update the object's length. */
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    return TCL_OK;
}


/*
 * CryptRandCmd
 *
 *   Retrieves cryptographically random bytes.
 *
 * Arguments:
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

static int
CryptRandCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int bytes;
    unsigned char *buffer;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "bytes");
        return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[2], &bytes) != TCL_OK) {
        return TCL_ERROR;
    }
    if (bytes < 1) {
        Tcl_AppendResult(interp, "invalid byte count \"",
            Tcl_GetString(objv[2]), "\": must be greater than 0", NULL);
        return TCL_ERROR;
    }

    buffer = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), bytes);

    if (rng_get_bytes(buffer, (unsigned long)bytes, NULL) != (unsigned long)bytes) {
        Tcl_SetResult(interp, "unable to retrieve random bytes", TCL_STATIC);
        return TCL_ERROR;
    }

    return TCL_OK;
}


/*
 * CryptObjCmd
 *
 *   This function provides the "crypt" Tcl command.
 *
 * Arguments:
 *   clientData - Pointer to a 'ExtState' structure.
 *   interp     - Current interpreter.
 *   objc       - Number of arguments.
 *   objv       - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 *
 * Remarks:
 *   None.
 */

int
CryptObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    ExtState *statePtr = (ExtState *) clientData;
    int index;
    static const char *options[] = {
        "decrypt", "encrypt", "end", "hash", "info",
        "pkcs5", "rand", "start", "update", NULL
    };
    enum options {
        OPTION_DECRYPT, OPTION_ENCRYPT, OPTION_END, OPTION_HASH, OPTION_INFO,
        OPTION_PKCS5, OPTION_RAND, OPTION_START, OPTION_UPDATE
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case OPTION_DECRYPT: return CryptProcessCmd(interp, objc, objv, MODE_DECRYPT);
        case OPTION_ENCRYPT: return CryptProcessCmd(interp, objc, objv, MODE_ENCRYPT);
        case OPTION_END:     return CryptEndCmd(interp, objc, objv, statePtr);
        case OPTION_HASH:    return CryptHashCmd(interp, objc, objv);
        case OPTION_INFO:    return CryptInfoCmd(interp, objc, objv, statePtr);
        case OPTION_PKCS5:   return CryptPkcs5Cmd(interp, objc, objv);
        case OPTION_RAND:    return CryptRandCmd(interp, objc, objv);
        case OPTION_START:   return CryptStartCmd(interp, objc, objv, statePtr);
        case OPTION_UPDATE:  return CryptUpdateCmd(interp, objc, objv, statePtr);
    }

    /* This point should never be reached. */
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
