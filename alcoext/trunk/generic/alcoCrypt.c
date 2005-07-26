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
 *     crypt hash    <hash algorithm> <data>
 *     crypt hash    -hmac    <key> <hash algorithm> <data>
 *     crypt hash    -omac    <key> <cipher algorithm> <data>
 *     crypt hash    -pelican <key> <cipher algorithm> <data>
 *     crypt hash    -pmac    <key> <cipher algorithm> <data>
 *
 *     crypt start   <hash algorithm>
 *     crypt start   -hmac    <key> <hash algorithm>
 *     crypt start   -omac    <key> <cipher algorithm>
 *     crypt start   -pelican <key> <cipher algorithm>
 *     crypt start   -pmac    <key> <cipher algorithm>
 *     crypt update  <handle> <data>
 *     crypt end     <handle>
 *
 *   Other Commands:
 *     crypt info    <ciphers|handles|hashes|modes>
 *     crypt pkcs5   [-v1] [-v2] [-rounds <count>] <hash algorithm> <salt> <password>
 *     crypt rand    <bytes>
 */

#include <alcoExt.h>

/* Tcl command functions. */
static int CryptProcessCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], unsigned char mode);
static int CryptHashCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int CryptStartCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int CryptUpdateCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int CryptEndCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int CryptInfoCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], ExtState *statePtr);
static int CryptPkcs5Cmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int CryptRandCmd(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

/* Cipher mode functions. */
static CryptModeProc DecryptCBC;
static CryptModeProc DecryptCFB;
static CryptModeProc DecryptCTR;
static CryptModeProc DecryptECB;
static CryptModeProc DecryptOFB;
static CryptModeProc EncryptCBC;
static CryptModeProc EncryptCFB;
static CryptModeProc EncryptCTR;
static CryptModeProc EncryptECB;
static CryptModeProc EncryptOFB;

static const CryptCipherMode cipherModes[] = {
    {"cbc", DecryptCBC, EncryptCBC, CRYPT_REQUIRES_IV|CRYPT_PAD_PLAINTEXT},
    {"cfb", DecryptCFB, EncryptCFB, CRYPT_REQUIRES_IV},
    {"ctr", DecryptCTR, EncryptCTR, CRYPT_REQUIRES_IV},
    {"ecb", DecryptECB, EncryptECB, CRYPT_PAD_PLAINTEXT},
    {"ofb", DecryptOFB, EncryptOFB, CRYPT_REQUIRES_IV},
    {NULL}
};

static const char *macSwitches[] = {
    "-hmac", "-omac", "-pelican", "-pmac", NULL
};

/*
 * Implementation and Security Note:
 * Tcl argument objects (objv) cannot be modified without creating noticeable
 * problems. These argument objects are shared and could be referenced by other
 * variables within the Tcl interpreter. Therefore, clearing any other memory
 * blocks or stack space which may have contained sensitive data would be
 * meaningless (e.g. using SecureZeroMemory or defining LTC_CLEAN_STACK).
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

    /* Certain cipher modes require an IV and others do not (e.g. ECB). */
    if (iv != NULL) {
        if (!(cipherModes[modeIndex].options & CRYPT_REQUIRES_IV)) {
            Tcl_AppendResult(interp, cipherModes[modeIndex].name,
                " mode does not require an initialisation vector", NULL);
            return TCL_ERROR;
        }

        /* Verify the IV's length. */
        if (ivLength != cipher_descriptor[cipherIndex].block_length) {
            char length[12];

#ifdef _WINDOWS
        StringCchPrintfA(length, ARRAYSIZE(length), "%d",
            cipher_descriptor[cipherIndex].block_length);
#else /* _WINDOWS */
        snprintf(length, ARRAYSIZE(length), "%d",
            cipher_descriptor[cipherIndex].block_length);
        length[ARRAYSIZE(length)-1] = '\0';
#endif /* _WINDOWS */

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
#ifdef _WINDOWS
            StringCchPrintfA(message, ARRAYSIZE(message), "%d",
                cipher_descriptor[cipherIndex].min_key_length);
#else /* _WINDOWS */
            snprintf(message, ARRAYSIZE(message), "%d",
                cipher_descriptor[cipherIndex].min_key_length);
            message[ARRAYSIZE(message)-1] = '\0';
#endif /* _WINDOWS */

        } else {
            /* Ranging key length. */
#ifdef _WINDOWS
            StringCchPrintfA(message, ARRAYSIZE(message), "between %d and %d",
                cipher_descriptor[cipherIndex].min_key_length,
                cipher_descriptor[cipherIndex].max_key_length);
#else /* _WINDOWS */
            snprintf(message, ARRAYSIZE(message), "between %d and %d",
                cipher_descriptor[cipherIndex].min_key_length,
                cipher_descriptor[cipherIndex].max_key_length);
            message[ARRAYSIZE(message)-1] = '\0';
#endif /* _WINDOWS */

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
        status = cipherModes[modeIndex].decrypt(
            cipherIndex, rounds, iv,
            key, (unsigned long)keyLength,
            data, (unsigned long)dataLength,
            dest);

    } else if (mode == MODE_ENCRYPT) {
        status = cipherModes[modeIndex].encrypt(
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
    int index;
    int keyLength;
    int status;
    unsigned char *data;
    unsigned char *dest;
    unsigned char *key;
    unsigned char type = CRYPT_HASH;
    unsigned long destLength;

    /* Argument checks. */
    if (objc == 6) {
        if (Tcl_GetIndexFromObj(interp, objv[2], macSwitches, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        type = (unsigned char) index;
        key = Tcl_GetByteArrayFromObj(objv[3], &keyLength);

    } else if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "?switches? algorithm data");
        return TCL_ERROR;
    }

    /* Retrieve the hash/cipher algorithm index. */
    if (type == CRYPT_HASH || type == CRYPT_HMAC) {
        status = Tcl_GetIndexFromObjStruct(interp, objv[objc-2], hash_descriptor,
            sizeof(hash_descriptor[0]), "hash", TCL_EXACT, &index);
    } else {
        status = Tcl_GetIndexFromObjStruct(interp, objv[objc-2], cipher_descriptor,
            sizeof(cipher_descriptor[0]), "cipher", TCL_EXACT, &index);
    }
    if (status != TCL_OK) {
        return TCL_ERROR;
    }

    data = Tcl_GetByteArrayFromObj(objv[objc-1], &dataLength);

    /* Create a byte object to hold the hash digest. */
    destLength = MAXBLOCKSIZE;
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), MAXBLOCKSIZE);

    switch (type) {
        case CRYPT_HASH: {
            status = hash_memory(index,
                data, (unsigned long)dataLength,
                dest, &destLength);
            break;
        }
        case CRYPT_HMAC: {
            status = hmac_memory(index,
                key,  (unsigned long)keyLength,
                data, (unsigned long)dataLength,
                dest, &destLength);
            break;
        }
        case CRYPT_OMAC: {
            status = omac_memory(index,
                key,  (unsigned long)keyLength,
                data, (unsigned long)dataLength,
                dest, &destLength);
            break;
        }
        case CRYPT_PELICAN: {
            status = pelican_memory(index,
                key,  (unsigned long)keyLength,
                data, (unsigned long)dataLength,
                dest, &destLength);
            break;
        }
        case CRYPT_PMAC: {
            status = pmac_memory(index,
                key,  (unsigned long)keyLength,
                data, (unsigned long)dataLength,
                dest, &destLength);
            break;
        }
    }

    if (status != CRYPT_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to hash data: ", error_to_string(status), NULL);
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
    int index;
    int keyLength;
    int newEntry;
    int status;
    unsigned char *key;
    unsigned char type = CRYPT_HASH;
    CryptHandle *handlePtr;
    Tcl_HashEntry *hashEntryPtr;

    /* Argument checks. */
    if (objc == 5) {
        if (Tcl_GetIndexFromObj(interp, objv[2], macSwitches, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        type = (unsigned char) index;
        key = Tcl_GetByteArrayFromObj(objv[3], &keyLength);

    } else if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "?switches? algorithm");
        return TCL_ERROR;
    }

    /* Retrieve the hash/cipher algorithm index. */
    if (type == CRYPT_HASH || type == CRYPT_HMAC) {
        status = Tcl_GetIndexFromObjStruct(interp, objv[objc-1], hash_descriptor,
            sizeof(hash_descriptor[0]), "hash", TCL_EXACT, &index);
    } else {
        status = Tcl_GetIndexFromObjStruct(interp, objv[objc-1], cipher_descriptor,
            sizeof(cipher_descriptor[0]), "cipher", TCL_EXACT, &index);
    }
    if (status != TCL_OK) {
        return TCL_ERROR;
    }

    handlePtr = (CryptHandle *) ckalloc(sizeof(CryptHandle));
    handlePtr->algoIndex = index;
    handlePtr->type = type;

    /* Initialise hash state. */
    switch (type) {
        case CRYPT_HASH: {
            status = hash_descriptor[index].init(&handlePtr->state.hash);
            break;
        }
        case CRYPT_HMAC: {
            status = hmac_init(&handlePtr->state.hmac, index, key, (unsigned long)keyLength);
            break;
        }
        case CRYPT_OMAC: {
            status = omac_init(&handlePtr->state.omac, index, key, (unsigned long)keyLength);
            break;
        }
        case CRYPT_PELICAN: {
            status = pelican_init(&handlePtr->state.pelican, index, key, (unsigned long)keyLength);
            break;
        }
        case CRYPT_PMAC: {
            status = pmac_init(&handlePtr->state.pmac, index, key, (unsigned long)keyLength);
            break;
        }
    }

    if (status != CRYPT_OK) {
        ckfree((char *) handlePtr);
        Tcl_AppendResult(interp, "unable to initialise hash state: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

#ifdef _WINDOWS
    StringCchPrintfA(handleId, ARRAYSIZE(handleId), "hash%lu", statePtr->cryptHandle);
#else /* _WINDOWS */
    snprintf(handleId, ARRAYSIZE(handleId), "hash%lu", statePtr->cryptHandle);
    handleId[ARRAYSIZE(handleId)-1] = '\0';
#endif /* _WINDOWS */

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

    /* Update hash state. */
    switch (handlePtr->type) {
        case CRYPT_HASH: {
            status = hash_descriptor[handlePtr->algoIndex].process(&handlePtr->state.hash,
                data, (unsigned long)dataLength);
            break;
        }
        case CRYPT_HMAC: {
            status = hmac_process(&handlePtr->state.hmac,
                data, (unsigned long)dataLength);
            break;
        }
        case CRYPT_OMAC: {
            status = omac_process(&handlePtr->state.omac,
                data, (unsigned long)dataLength);
            break;
        }
        case CRYPT_PELICAN: {
            status = pelican_process(&handlePtr->state.pelican,
                data, (unsigned long)dataLength);
            break;
        }
        case CRYPT_PMAC: {
            status = pmac_process(&handlePtr->state.pmac,
                data, (unsigned long)dataLength);
            break;
        }
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
 *   Finalise a hash state, returning the digest.
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

    /* Create a byte object to hold the hash digest. */
    destLength = MAXBLOCKSIZE;
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), MAXBLOCKSIZE);

    /* Finalise hash state. */
    switch (handlePtr->type) {
        case CRYPT_HASH: {
            destLength = hash_descriptor[handlePtr->algoIndex].hashsize;
            status = hash_descriptor[handlePtr->algoIndex].done(&handlePtr->state.hash, dest);
            break;
        }
        case CRYPT_HMAC: {
            status = hmac_done(&handlePtr->state.hmac, dest, &destLength);
            break;
        }
        case CRYPT_OMAC: {
            status = omac_done(&handlePtr->state.omac, dest, &destLength);
            break;
        }
        case CRYPT_PELICAN: {
            status = pelican_done(&handlePtr->state.pelican, dest, &destLength);
            break;
        }
        case CRYPT_PMAC: {
            status = pmac_done(&handlePtr->state.pmac, dest, &destLength);
            break;
        }
    }

    /* Free handle structure and remove the hash table entry. */
    ckfree((char *) handlePtr);
    Tcl_DeleteHashEntry(hashEntryPtr);

    if (status != CRYPT_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to finalise hash state: ",
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

    for (entryPtr = Tcl_FirstHashEntry(tablePtr, &search);
        entryPtr != NULL;
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
 *   Create a PKCS #5 v1 or v2 compliant hash.
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
    int i;
    int index;
    int rounds = 1;
    int passLength;
    int saltLength;
    int status;
    unsigned char *dest;
    unsigned char *pass;
    unsigned char *salt;
    unsigned char pkcsFiveAlgo = 2;
    unsigned long destLength;
    static const char *switches[] = {"-v1", "-v2", "-rounds", NULL};
    enum switches {SWITCH_ALGO1, SWITCH_ALGO2, SWITCH_ROUNDS};

    for (i = 2; i+3 < objc; i++) {
        char *name = Tcl_GetString(objv[i]);
        if (name[0] != '-') {
            break;
        }

        if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }

        switch ((enum switches) index) {
            case SWITCH_ALGO1: {
                pkcsFiveAlgo = 1;
                break;
            }
            case SWITCH_ALGO2: {
                pkcsFiveAlgo = 2;
                break;
            }
            case SWITCH_ROUNDS: {
                i++;
                if (Tcl_GetIntFromObj(interp, objv[i], &rounds) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (rounds < 1) {
                    Tcl_AppendResult(interp, "invalid round count \"",
                        Tcl_GetString(objv[i]), "\": must be greater than 0", NULL);
                    return TCL_ERROR;
                }
                break;
            }
        }
    }

    if ((objc - i) != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "?switches? hash salt password");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[objc-3], hash_descriptor,
        sizeof(hash_descriptor[0]), "hash", TCL_EXACT, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    salt = Tcl_GetByteArrayFromObj(objv[objc-2], &saltLength);
    pass = Tcl_GetByteArrayFromObj(objv[objc-1], &passLength);

    /* Create a byte object to hold the hash digest. */
    destLength = hash_descriptor[index].hashsize;
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    if (pkcsFiveAlgo == 2) {
        status = pkcs_5_alg2(pass, (unsigned long)passLength,
                             salt, (unsigned long)saltLength,
                             rounds, index, dest, &destLength);
    } else if (saltLength != 8) {
        /* The salt must be 8 bytes for PKCS #5 v1. */
        status = CRYPT_INVALID_SALT;
    } else {
        status = pkcs_5_alg1(pass, (unsigned long)passLength,
                             salt, rounds, index, dest, &destLength);
    }

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
        Tcl_SetResult(interp, "unable to retrieve random data", TCL_STATIC);
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
