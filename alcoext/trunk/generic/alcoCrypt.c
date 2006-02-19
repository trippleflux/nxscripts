/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    alcoCrypt.c

Author:
    neoxed (neoxed@gmail.com) May 6, 2005

Abstract:
    This module implements a interface to LibTomCrypt. Providing support for
    symmetric block ciphers, one-way hashing, and random number generators.

    Cipher Modes:
       cbc - Cipher Block Chaining
       cfb - Cipher Feedback
       ctr - Counter
       ecb - Electronic Codebook
       ofb - Output Feedback

    Cipher Commands:
       crypt decrypt [-counter|-iv|-mode|-rounds <value> ...] <cipher> <key> <data>
       crypt encrypt [-counter|-iv|-mode|-rounds <value> ...] <cipher> <key> <data>

    Hash Commands:
       crypt hash    <hash algorithm> <data>
       crypt hash    -hmac    <key> <hash> <data>
       crypt hash    -omac    <key> <cipher> <data>
       crypt hash    -pelican <key> <cipher> <data>
       crypt hash    -pmac    <key> <cipher> <data>

       crypt start   <hash algorithm>
       crypt start   -hmac    <key> <hash>
       crypt start   -omac    <key> <cipher>
       crypt start   -pelican <key> <cipher>
       crypt start   -pmac    <key> <cipher>
       crypt update  <handle> <data>
       crypt end     <handle>

    Random Commands:
       crypt prng <type>
       crypt rand <bytes>

    Other Commands:
       crypt info  <ciphers|handles|hashes|modes|prngs>
       crypt pkcs5 [-v1] [-v2] [-rounds <count>] <hash algorithm> <salt> <password>

Implementation/Security Note:

    Tcl argument objects (objv) cannot be modified without creating noticeable
    problems. These argument objects are shared and could be referenced by other
    variables within the Tcl interpreter. Therefore, clearing any other memory
    blocks or stack space which may have contained sensitive data would be
    meaningless (e.g. using SecureZeroMemory or defining LTC_CLEAN_STACK).

--*/

#include <alcoExt.h>

// Modes for CryptProcessCmd.
#define MODE_DECRYPT 1
#define MODE_ENCRYPT 2

//
// Tcl command functions.
//

static int
CryptProcessCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    unsigned char mode
    );

static int
CryptHashCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
CryptStartCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    );

static int
CryptUpdateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    );

static int
CryptEndCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    );

static int
CryptInfoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    );

static int
CryptPkcs5Cmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
CryptPrngCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
CryptRandCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

//
// Cipher mode functions.
//

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

typedef struct {
    char *name;               // Name of cipher mode.
    CryptModeProc *decrypt;   // Pointer to the cipher mode's decryption function.
    CryptModeProc *encrypt;   // Pointer to the cipher mode's encryption function.
    unsigned char requiresIv; // Boolean to indicate if the cipher mode requires
} CryptCipherMode;            // an initialisation vector.

static const CryptCipherMode cipherModes[] = {
    {"cbc", DecryptCBC, EncryptCBC, 1},
    {"cfb", DecryptCFB, EncryptCFB, 1},
    {"ctr", DecryptCTR, EncryptCTR, 1},
    {"ecb", DecryptECB, EncryptECB, 0},
    {"ofb", DecryptOFB, EncryptOFB, 1},
    {NULL}
};

//
// MAC switches and handle structures.
//

static const char *macSwitches[] = {
    "-hmac", "-omac", "-pelican", "-pmac", NULL
};

enum {
    CRYPT_HMAC = 0,
    CRYPT_OMAC,
    CRYPT_PELICAN,
    CRYPT_PMAC,
    CRYPT_HASH
};

typedef struct {
    int descIndex;      // Cipher/hash descriptor index.
    unsigned char type; // Type of handle.
    union {
        hash_state    hash;
        hmac_state    hmac;
        omac_state    omac;
        pelican_state pelican;
        pmac_state    pmac;
    } state;
} CryptHandle;

//
// PRNG channel driver.
//

static Tcl_DriverBlockModeProc PrngSetBlocking;
static Tcl_DriverCloseProc     PrngClose;
static Tcl_DriverInputProc     PrngInput;
static Tcl_DriverOutputProc    PrngOutput;
static Tcl_DriverGetOptionProc PrngGetOption;
static Tcl_DriverSetOptionProc PrngSetOption;
static Tcl_DriverWatchProc     PrngWatch;

static Tcl_ChannelType prngChannelType = {
    "prng",                 // Channel type.
    TCL_CHANNEL_VERSION_2,  // Channel driver version.
    PrngClose,              // Close channel.
    PrngInput,              // Read from channel.
    PrngOutput,             // Write to channel.
    NULL,                   // Seek proc.
    PrngSetOption,          // Set channel options.
    PrngGetOption,          // Get channel options.
    PrngWatch,              // Event notifier.
    NULL,                   // Retrieve an OS handle.
    NULL,                   // Close 2 proc.
    PrngSetBlocking,        // Set blocking or non-blocking mode.
    NULL,                   // Flush proc.
    NULL,                   // Handler proc.
};

typedef struct {
    int descIndex;          // PRNG descriptor index.
    int ready;              // Boolean to indicate if the PRNG is ready.
    Tcl_Channel channel;    // Channel associated with this handle.
    prng_state state;       // PRNG state.
} PrngHandle;


/*++

DecryptCBC
DecryptCFB
DecryptCTR
DecryptECB
DecryptOFB

    Decrypts data from the specified cipher mode and algorithm.

Arguments:
    cipher      - Index of the desired cipher.

    rounds      - Number of rounds.

    counterMode - The counter mode (CTR_COUNTER_LITTLE_ENDIAN or CTR_COUNTER_BIG_ENDIAN).

    iv          - The initial vector, must be the length of one block.

    key         - The secret key.

    keyLength   - Length of the secret key, in bytes.

    data        - Cipher text (data to decrypt).

    dataLength  - Length of cipher text, in bytes.

    dest        - Buffer to receive plain text (decrypted data).

Return Value:
    A LibTomCrypt status code; CRYPT_OK will be returned if successful.

--*/
static int
DecryptCBC(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
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
DecryptCFB(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
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
DecryptCTR(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
{
    int status;
    symmetric_CTR state;

    status = ctr_start(cipher, iv, key, keyLength, rounds, counterMode, &state);
    if (status == CRYPT_OK) {
        status = ctr_decrypt(data, dest, dataLength, &state);
        ctr_done(&state);
    }

    return status;
}

static int
DecryptECB(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
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
DecryptOFB(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
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

/*++

EncryptCBC
EncryptCFB
EncryptCTR
EncryptECB
EncryptOFB

    Encrypts data in the specified cipher mode and algorithm.

Arguments:
    cipher      - Index of the desired cipher.

    rounds      - Number of rounds.

    counterMode - The counter mode (CTR_COUNTER_LITTLE_ENDIAN or CTR_COUNTER_BIG_ENDIAN).

    iv          - The initial vector, must be the length of one block.

    key         - The secret key.

    keyLength   - Length of the secret key, in bytes.

    data        - Plain text (data to encrypt).

    dataLength  - Length of plain text, in bytes.

    dest        - Buffer to receive cipher text (encrypted data).

Return Value:
    A LibTomCrypt status code; CRYPT_OK will be returned if successful.

--*/
static int
EncryptCBC(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
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
EncryptCFB(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
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
EncryptCTR(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
{
    int status;
    symmetric_CTR state;

    status = ctr_start(cipher, iv, key, keyLength, rounds, counterMode, &state);
    if (status == CRYPT_OK) {
        status = ctr_encrypt(data, dest, dataLength, &state);
        ctr_done(&state);
    }

    return status;
}

static int
EncryptECB(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
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
EncryptOFB(
    int cipher,
    int rounds,
    int counterMode,
    unsigned char *iv,
    unsigned char *key,
    unsigned long keyLength,
    unsigned char *data,
    unsigned long dataLength,
    unsigned char *dest
    )
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

/*++

CryptProcessCmd

    Encrypts or decrypts data with the specified cipher algorithm.

Arguments:
    interp  - Current interpreter.

    objc    - Number of arguments.

    objv    - Argument objects.

    mode    - Process mode (must be MODE_DECRYPT or MODE_ENCRYPT).

Return Value:
    A standard Tcl result.

--*/
static int
CryptProcessCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    unsigned char mode
    )
{
    int i;
    int index;
    int cipherIndex;
    int counterMode = CTR_COUNTER_LITTLE_ENDIAN; // Little-endian by default.
    int rounds = 0;
    int modeIndex = 3; // ECB mode by default.
    int status = CRYPT_ERROR;
    int dataLength;
    int ivLength = 0;
    int keyLength;
    unsigned char *data;
    unsigned char *dest;
    unsigned char *iv = NULL;
    unsigned char *key;
    static const char *switches[] = {
        "-counter", "-iv", "-mode", "-rounds", NULL
    };
    enum switchIndices {
        SWITCH_COUNTER = 0, SWITCH_IV, SWITCH_MODE, SWITCH_ROUNDS
    };

    assert(mode == MODE_DECRYPT || mode == MODE_ENCRYPT);

    for (i = 2; i+1 < objc; i++) {
        char *name = Tcl_GetString(objv[i]);
        if (name[0] != '-') {
            break;
        }

        if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }

        i++;
        switch ((enum switchIndices) index) {
            case SWITCH_COUNTER: {
                static const char *counterModes[] = {
                    "littleEndian", "bigEndian", NULL
                };

                //
                // Instead of switching through the index obtained by Tcl_GetIndexFromObj,
                // we ordered the counter modes in accordance to their defined values.
                //
                // counterModes[CTR_COUNTER_LITTLE_ENDIAN] == "littleEndian"
                // counterModes[CTR_COUNTER_BIG_ENDIAN]    == "bigEndian"
                //
                assert(CTR_COUNTER_LITTLE_ENDIAN == 0);
                assert(CTR_COUNTER_BIG_ENDIAN    == 1);

                if (Tcl_GetIndexFromObj(interp, objv[i], counterModes, "counter mode",
                    TCL_EXACT, &counterMode) != TCL_OK) {
                    return TCL_ERROR;
                }
                break;
            }
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

    // Certain cipher modes require an IV and others do not (e.g. ECB).
    if (iv != NULL) {
        if (cipherModes[modeIndex].requiresIv == 0) {
            Tcl_AppendResult(interp, cipherModes[modeIndex].name,
                " mode does not require an initialisation vector", NULL);
            return TCL_ERROR;
        }

        // Verify the IV's length.
        if (ivLength != cipher_descriptor[cipherIndex].block_length) {
            char length[12];

#ifdef _WINDOWS
        StringCchPrintfA(length, ARRAYSIZE(length), "%d",
            cipher_descriptor[cipherIndex].block_length);
#else // _WINDOWS
        snprintf(length, ARRAYSIZE(length), "%d",
            cipher_descriptor[cipherIndex].block_length);
        length[ARRAYSIZE(length)-1] = '\0';
#endif // _WINDOWS

            Tcl_AppendResult(interp, "invalid initialisation vector size: must be ",
                length, " bytes", NULL);
            return TCL_ERROR;
        }

    } else if (cipherModes[modeIndex].requiresIv != 0) {
        Tcl_AppendResult(interp, cipherModes[modeIndex].name,
            " mode requires an initialisation vector", NULL);
        return TCL_ERROR;
    }

    //
    // Verify the given key's length. If the key exceeds the
    // maximum length, it will be truncated without warning.
    //
    key = Tcl_GetByteArrayFromObj(objv[objc-2], &keyLength);
    if (cipher_descriptor[cipherIndex].keysize(&keyLength) != CRYPT_OK) {
        char message[64];

        if (cipher_descriptor[cipherIndex].min_key_length ==
            cipher_descriptor[cipherIndex].max_key_length) {

            // Fixed key length.
#ifdef _WINDOWS
            StringCchPrintfA(message, ARRAYSIZE(message), "%d",
                cipher_descriptor[cipherIndex].min_key_length);
#else // _WINDOWS
            snprintf(message, ARRAYSIZE(message), "%d",
                cipher_descriptor[cipherIndex].min_key_length);
            message[ARRAYSIZE(message)-1] = '\0';
#endif // _WINDOWS

        } else {
            // Ranging key length.
#ifdef _WINDOWS
            StringCchPrintfA(message, ARRAYSIZE(message), "between %d and %d",
                cipher_descriptor[cipherIndex].min_key_length,
                cipher_descriptor[cipherIndex].max_key_length);
#else // _WINDOWS
            snprintf(message, ARRAYSIZE(message), "between %d and %d",
                cipher_descriptor[cipherIndex].min_key_length,
                cipher_descriptor[cipherIndex].max_key_length);
            message[ARRAYSIZE(message)-1] = '\0';
#endif // _WINDOWS

        }

        Tcl_AppendResult(interp, "invalid key size: must be ", message, " bytes", NULL);
        return TCL_ERROR;
    }

    data = Tcl_GetByteArrayFromObj(objv[objc-1], &dataLength);
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), dataLength);

    if (mode == MODE_DECRYPT) {
        status = cipherModes[modeIndex].decrypt(
            cipherIndex, rounds, counterMode, iv,
            key,  (unsigned long)keyLength,
            data, (unsigned long)dataLength,
            dest);

    } else if (mode == MODE_ENCRYPT) {
        status = cipherModes[modeIndex].encrypt(
            cipherIndex, rounds, counterMode, iv,
            key,  (unsigned long)keyLength,
            data, (unsigned long)dataLength,
            dest);
    }

    if (status != CRYPT_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to process data: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*++

CryptHashCmd

    Hashes data using the specified hash algorithm.

Arguments:
    interp  - Current interpreter.

    objc    - Number of arguments.

    objv    - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
CryptHashCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int dataLength;
    int index;
    int keyLength = 0;
    int status;
    unsigned char *data;
    unsigned char *dest;
    unsigned char *key = NULL;
    unsigned char type = CRYPT_HASH;
    unsigned long destLength;

    // Argument checks.
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

    // Retrieve the hash/cipher algorithm index.
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

    // Create a byte object to hold the hash digest.
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

    // Update the object's length.
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    return TCL_OK;
}

/*++

CryptStartCmd

    Initialise a hash state.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
CryptStartCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
{
    char handleName[4 + (sizeof(void*) * 2) + 3]; // Handle name, pointer in hex, and a NULL.
    int index;
    int keyLength;
    int newEntry;
    int status;
    unsigned char *key = NULL;
    unsigned char type = CRYPT_HASH;
    CryptHandle *handlePtr;
    Tcl_HashEntry *hashEntryPtr;

    // Argument checks.
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

    // Retrieve the hash/cipher algorithm index.
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

    handlePtr = (CryptHandle *)ckalloc(sizeof(CryptHandle));
    handlePtr->descIndex = index;
    handlePtr->type = type;

    // Initialise hash state.
    switch (type) {
        case CRYPT_HASH: {
            status = hash_descriptor[index].init(&handlePtr->state.hash);
            break;
        }
        case CRYPT_HMAC: {
            status = hmac_init(&handlePtr->state.hmac, index,
                key, (unsigned long)keyLength);
            break;
        }
        case CRYPT_OMAC: {
            status = omac_init(&handlePtr->state.omac, index,
                key, (unsigned long)keyLength);
            break;
        }
        case CRYPT_PELICAN: {
            status = pelican_init(&handlePtr->state.pelican, index,
                key, (unsigned long)keyLength);
            break;
        }
        case CRYPT_PMAC: {
            status = pmac_init(&handlePtr->state.pmac, index,
                key, (unsigned long)keyLength);
            break;
        }
    }

    if (status != CRYPT_OK) {
        ckfree((char *)handlePtr);
        Tcl_AppendResult(interp, "unable to initialise hash: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

    // The handle identifier doubles as the hash key.
#ifdef _WINDOWS
    StringCchPrintfA(handleName, ARRAYSIZE(handleName), "hash%p", handlePtr);
#else // _WINDOWS
    snprintf(handleName, ARRAYSIZE(handleName), "hash%p", handlePtr);
    handleName[ARRAYSIZE(handleName)-1] = '\0';
#endif // _WINDOWS

    hashEntryPtr = Tcl_CreateHashEntry(statePtr->cryptTable, handleName, &newEntry);
    if (newEntry == 0) {
        Tcl_Panic("Duplicate crypt handle identifiers.");
    }
    Tcl_SetHashValue(hashEntryPtr, (ClientData)handlePtr);

    Tcl_SetStringObj(Tcl_GetObjResult(interp), handleName, -1);
    return TCL_OK;
}

/*++

CryptUpdateCmd

    Process a block of data.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
CryptUpdateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
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

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->cryptTable, "hash");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (CryptHandle *)Tcl_GetHashValue(hashEntryPtr);

    data = Tcl_GetByteArrayFromObj(objv[3], &dataLength);

    // Update hash state.
    switch (handlePtr->type) {
        case CRYPT_HASH: {
            status = hash_descriptor[handlePtr->descIndex].process(&handlePtr->state.hash,
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
        Tcl_AppendResult(interp, "unable to update hash: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*++

CryptEndCmd

    Finalise a hash state, returning the digest.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
CryptEndCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
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

    hashEntryPtr = GetHandleTableEntry(interp, objv[2], statePtr->cryptTable, "hash");
    if (hashEntryPtr == NULL) {
        return TCL_ERROR;
    }
    handlePtr = (CryptHandle *)Tcl_GetHashValue(hashEntryPtr);

    // Create a byte object to hold the hash digest.
    destLength = MAXBLOCKSIZE;
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), MAXBLOCKSIZE);

    // Finalise hash state.
    switch (handlePtr->type) {
        case CRYPT_HASH: {
            destLength = hash_descriptor[handlePtr->descIndex].hashsize;
            status = hash_descriptor[handlePtr->descIndex].done(&handlePtr->state.hash, dest);
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

    // Free handle structure and remove the hash table entry.
    ckfree((char *)handlePtr);
    Tcl_DeleteHashEntry(hashEntryPtr);

    if (status != CRYPT_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to finalise hash: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

    // Update the object's length.
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    return TCL_OK;
}

/*++

CryptCloseHandles

    Close all "crypt" handles in the given hash table.

Arguments:
    tablePtr - Hash table of "crypt" handles.

Return Value:
    None.

--*/
void
CryptCloseHandles(
    Tcl_HashTable *tablePtr
    )
{
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr;

    for (entryPtr = Tcl_FirstHashEntry(tablePtr, &search);
            entryPtr != NULL;
            entryPtr = Tcl_NextHashEntry(&search)) {

        ckfree((char *)Tcl_GetHashValue(entryPtr));
        Tcl_DeleteHashEntry(entryPtr);
    }
}

/*++

CryptInfoCmd

    Retrieves information about the cryptography extension.

Arguments:
    interp   - Current interpreter.

    objc     - Number of arguments.

    objv     - Argument objects.

    statePtr - Pointer to a "ExtState" structure.

Return Value:
    A standard Tcl result.

--*/
static int
CryptInfoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ExtState *statePtr
    )
{
    int index;
    Tcl_Obj *resultPtr;
    static const char *options[] = {
        "ciphers", "handles", "hashes", "modes", "prngs", NULL
    };
    enum optionIndices {
        OPTION_CIPHERS = 0, OPTION_HANDLES, OPTION_HASHES, OPTION_MODES, OPTION_PRNGS
    };

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "option");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    resultPtr = Tcl_GetObjResult(interp);

    switch ((enum optionIndices) index) {
        case OPTION_CIPHERS: {
            // Create a list of supported ciphers.
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

            // Create a list of open crypt handles.
            for (hashEntryPtr = Tcl_FirstHashEntry(statePtr->cryptTable, &hashSearch);
                    hashEntryPtr != NULL;
                    hashEntryPtr = Tcl_NextHashEntry(&hashSearch)) {

                name = Tcl_GetHashKey(statePtr->cryptTable, hashEntryPtr);
                Tcl_ListObjAppendElement(NULL, resultPtr, Tcl_NewStringObj(name, -1));
            }
            return TCL_OK;
        }
        case OPTION_HASHES: {
            // Create a list of supported hashes.
            for (index = 0; index < TAB_SIZE && hash_descriptor[index].name != NULL; index++) {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    Tcl_NewStringObj(hash_descriptor[index].name, -1));
            }
            return TCL_OK;
        }
        case OPTION_MODES: {
            // Create a list of supported cipher modes.
            for (index = 0; cipherModes[index].name != NULL; index++) {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    Tcl_NewStringObj(cipherModes[index].name, -1));
            }
            return TCL_OK;
        }
        case OPTION_PRNGS: {
            // Create a list of supported PRNGs.
            for (index = 0; index < TAB_SIZE && prng_descriptor[index].name != NULL; index++) {
                Tcl_ListObjAppendElement(NULL, resultPtr,
                    Tcl_NewStringObj(prng_descriptor[index].name, -1));
            }
            return TCL_OK;
        }
    }

    // This point should never be reached.
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}

/*++

CryptPkcs5Cmd

    Create a PKCS #5 v1 or v2 compliant hash.

Arguments:
    interp  - Current interpreter.

    objc    - Number of arguments.

    objv    - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
CryptPkcs5Cmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
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
    static const char *switches[] = {
        "-v1", "-v2", "-rounds", NULL
    };
    enum switchIndices {
        SWITCH_ALGO1 = 0, SWITCH_ALGO2, SWITCH_ROUNDS
    };

    for (i = 2; i+3 < objc; i++) {
        char *name = Tcl_GetString(objv[i]);
        if (name[0] != '-') {
            break;
        }

        if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", TCL_EXACT, &index) != TCL_OK) {
            return TCL_ERROR;
        }

        switch ((enum switchIndices) index) {
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

    // Create a byte object to hold the hash digest.
    destLength = hash_descriptor[index].hashsize;
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    if (pkcsFiveAlgo == 2) {
        status = pkcs_5_alg2(pass, (unsigned long)passLength,
                             salt, (unsigned long)saltLength,
                             rounds, index, dest, &destLength);
    } else if (saltLength != 8) {
        // The salt must be 8 bytes for PKCS #5 v1.
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

    // Update the object's length.
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    return TCL_OK;
}

/*++

CryptPrngCmd

    Create a PRNG Tcl channel.

Arguments:
    interp  - Current interpreter.

    objc    - Number of arguments.

    objv    - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
CryptPrngCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char channelName[4 + (sizeof(void*) * 2) + 3]; // Channel type, pointer in hex, and a NULL.
    int index;
    int status;
    PrngHandle *handlePtr;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "type");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[2], prng_descriptor,
            sizeof(prng_descriptor[0]), "prng", TCL_EXACT, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    // Initialise the PRNG.
    handlePtr = (PrngHandle *)ckalloc(sizeof(PrngHandle));
    handlePtr->descIndex = index;
    handlePtr->ready = 0;

    status = prng_descriptor[index].start(&handlePtr->state);
    if (status != CRYPT_OK) {
        ckfree((char *)handlePtr);
        Tcl_AppendResult(interp, "unable to initialise PRNG: ",
            error_to_string(status), NULL);
        return TCL_ERROR;
    }

#ifdef _WINDOWS
    StringCchPrintfA(channelName, ARRAYSIZE(channelName), "prng%p", handlePtr);
#else // _WINDOWS
    snprintf(channelName, ARRAYSIZE(channelName), "prng%p", handlePtr);
    channelName[ARRAYSIZE(channelName)-1] = '\0';
#endif // _WINDOWS

    handlePtr->channel = Tcl_CreateChannel(&prngChannelType, channelName,
	    (ClientData)handlePtr, TCL_READABLE | TCL_WRITABLE);

    // Set default channel options.
    Tcl_SetChannelOption(NULL, handlePtr->channel, "-buffering",   "none");
    Tcl_SetChannelOption(NULL, handlePtr->channel, "-blocking",    "0");
    Tcl_SetChannelOption(NULL, handlePtr->channel, "-translation", "binary");

    Tcl_RegisterChannel(interp, handlePtr->channel);

    Tcl_SetStringObj(Tcl_GetObjResult(interp), channelName, -1);
    return TCL_OK;
}

/*++

CryptRandCmd

    Retrieves random entropy from the system.

Arguments:
    interp  - Current interpreter.

    objc    - Number of arguments.

    objv    - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
CryptRandCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
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

/*++

PrngSetBlocking

    Sets the blocking mode on a PRNG channel.

Arguments:
    instanceData - Pointer to a "PrngHandle" structure.

    mode         - Requested blocking mode, TCL_MODE_BLOCKING
                   or TCL_MODE_NONBLOCKING.

Return Value:
    If the function succeeds, the return value is zero. If the function
    fails, the return value is an appropriate errno value.

--*/
static int
PrngSetBlocking(
	ClientData instanceData,
	int mode
	)
{
    // Always non-blocking.
    return (mode == TCL_MODE_NONBLOCKING) ? 0 : EINVAL;
}

/*++

PrngClose

    Closes the PRNG channel.

Arguments:
    instanceData - Pointer to a "PrngHandle" structure.

    interp       - Interpreter to use for error reporting. This
                   argument can be NULL.

Return Value:
    If the function succeeds, the return value is zero. If the function
    fails, the return value is an appropriate errno value.

--*/
static int
PrngClose(
    ClientData instanceData,
    Tcl_Interp *interp
    )
{
    PrngHandle *handlePtr = (PrngHandle *)instanceData;

    // Finalise the PRNG state and free resources.
    prng_descriptor[handlePtr->descIndex].done(&handlePtr->state);
    ckfree((char *)handlePtr);

    return 0;
}

/*++

PrngInput

    Reads random data from the PRNG channel.

Arguments:
    instanceData - Pointer to a "PrngHandle" structure.

    dest         - Buffer to receive random data.

    destLength   - Length of the buffer, in bytes.

    errorCodePtr - Location to store the error code.

Return Value:
    The return value is the number of bytes read from the PRNG.

--*/
static int
PrngInput(
    ClientData instanceData,
    char *dest,
    int destLength,
    int *errorCodePtr
    )
{
    PrngHandle *handlePtr = (PrngHandle *)instanceData;

    //
    // Not all PRNGs (i.e. the SPRNG) require the caller to mark it as ready
    // before using it, but I prefer to keep the subtleties consistent.
    //
    if (!handlePtr->ready) {
        *errorCodePtr = EACCES;
        return -1;
    }

    *errorCodePtr = 0;
    return (int)prng_descriptor[handlePtr->descIndex].read(
        (unsigned char *)dest, (unsigned long)destLength, &handlePtr->state);
}

/*++

PrngOutput

    Adds entropy to the PRNG channel.

Arguments:
    instanceData - Pointer to a "PrngHandle" structure.

    source       - Buffer containing the entropy to add.

    sourceLength - Length of the buffer, in bytes.

    errorCodePtr - Location to store the error code.

Return Value:
    The return value is the number of bytes added to the PRNG.

--*/
static int
PrngOutput(
	ClientData instanceData,
	CONST char *source,
	int sourceLength,
	int *errorCodePtr
    )
{
    int status;
    PrngHandle *handlePtr = (PrngHandle *)instanceData;

    // Add entropy to the PRNG.
    status = prng_descriptor[handlePtr->descIndex].add_entropy(
        (unsigned char *)source, (unsigned long)sourceLength, &handlePtr->state);

    if (status != CRYPT_OK) {
        // Try to map the LibTomCrypt status code to an errno value.
        switch (status) {
            case CRYPT_INVALID_ARG:
            case CRYPT_INVALID_CIPHER:
            case CRYPT_INVALID_HASH:
            case CRYPT_INVALID_KEYSIZE:
            case CRYPT_INVALID_PRNG: {
                *errorCodePtr = EINVAL;
                break;
            }
            default: {
                *errorCodePtr = EACCES;
                break;
            }
        }
        return -1;
    }

    *errorCodePtr = 0;
    return sourceLength;
}

/*++

PrngGetOption

    Retrieves an option value for a PRNG channel.

Arguments:
    instanceData - Pointer to a "PrngHandle" structure.

    interp       - Interpreter to use for error reporting. This
                   argument can be NULL.

    optionName   - Name of the option to retrieve the value for, or NULL
                   to retrieve all options and their values.

    optionValue  - Location to store the computed values, initialised by caller.

Return Value:
    A standard Tcl result.

--*/
static int
PrngGetOption(
    ClientData instanceData,
    Tcl_Interp *interp,
    CONST char *optionName,
    Tcl_DString *optionValue
    )
{
    size_t length = 0;
    PrngHandle *handlePtr = (PrngHandle *)instanceData;

    //
    // The option name will be NULL if all options were
    // requested, e.g. "fconfigure $prngChannel".
    //
    if (optionName != NULL) {
        length = strlen(optionName);
    }

    if (length != 0 && (length < 2 || strncmp("-ready", optionName, length) != 0)) {
        return Tcl_BadChannelOption(interp, optionName, "ready");
    }

    // Append the option name and its value.
    if (length == 0) {
        Tcl_DStringAppendElement(optionValue, "-ready");
    }
    Tcl_DStringAppendElement(optionValue, handlePtr->ready ? "1" : "0");

    return TCL_OK;
}

/*++

PrngSetOption

    Sets PRNG channel specific options.

Arguments:
    instanceData - Pointer to a "PrngHandle" structure.

    interp       - Interpreter to use for error reporting. This
                   argument can be NULL.

    optionName   - Name of the option to set.

    newValue     - New value for option.

Return Value:
    A standard Tcl result.

--*/
static int
PrngSetOption(
    ClientData instanceData,
    Tcl_Interp *interp,
    CONST char *optionName,
    CONST char *newValue
    )
{
    int ready;
    size_t length;
    PrngHandle *handlePtr = (PrngHandle *)instanceData;

    length = strlen(optionName);

    if (length < 2 || strncmp("-ready", optionName, length) != 0) {
        return Tcl_BadChannelOption(interp, optionName, "ready");
    }

    if (Tcl_GetBoolean(interp, newValue, &ready) != TCL_OK) {
        return TCL_ERROR;
    }

    if (ready) {
        // Set the PRNG as ready.
        int status = prng_descriptor[handlePtr->descIndex].ready(&handlePtr->state);

        if (status != CRYPT_OK) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "unable to ready the PRNG: ",
                    error_to_string(status), NULL);
            }
            return TCL_ERROR;
        }

        // Only update the status if ready() succeeds.
        handlePtr->ready = 1;
    } else {
        handlePtr->ready = 0;
    }

    return TCL_OK;
}

/*++

PrngWatch

    Called by the notifier to set up to watch for events on this channel.

Arguments:
    instanceData - Pointer to a "PrngHandle" structure.

    mask         - Events the caller is interested in noticing on this
                   channel. An OR-ed combination of TCL_READABLE,
                   TCL_WRITABLE, and TCL_EXCEPTION

Return Value:
    Mone.

--*/
static void
PrngWatch(
	ClientData instanceData,
	int mask
	)
{
    //
    // Even though this function is not used, it must be
    // present in the Tcl channel driver definition.
    //
    return;
}

/*++

CryptObjCmd

    This function provides the "crypt" Tcl command.

Arguments:
    clientData  - Pointer to a "ExtState" structure.

    interp      - Current interpreter.

    objc        - Number of arguments.

    objv        - Argument objects.

Return Value:
    A standard Tcl result.

--*/
int
CryptObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    ExtState *statePtr = (ExtState *)clientData;
    int index;
    static const char *options[] = {
        "decrypt", "encrypt", "end", "hash", "info",
        "pkcs5", "prng", "rand", "start", "update", NULL
    };
    enum optionIndices {
        OPTION_DECRYPT = 0, OPTION_ENCRYPT, OPTION_END, OPTION_HASH, OPTION_INFO,
        OPTION_PKCS5, OPTION_PRNG, OPTION_RAND, OPTION_START, OPTION_UPDATE
    };

    // Validate "macSwitches" indices.
    assert(!strcmp("-hmac",    macSwitches[CRYPT_HMAC]));
    assert(!strcmp("-omac",    macSwitches[CRYPT_OMAC]));
    assert(!strcmp("-pelican", macSwitches[CRYPT_PELICAN]));
    assert(!strcmp("-pmac",    macSwitches[CRYPT_PMAC]));

    // Check arguments.
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum optionIndices) index) {
        case OPTION_DECRYPT: return CryptProcessCmd(interp, objc, objv, MODE_DECRYPT);
        case OPTION_ENCRYPT: return CryptProcessCmd(interp, objc, objv, MODE_ENCRYPT);
        case OPTION_END:     return CryptEndCmd(interp, objc, objv, statePtr);
        case OPTION_HASH:    return CryptHashCmd(interp, objc, objv);
        case OPTION_INFO:    return CryptInfoCmd(interp, objc, objv, statePtr);
        case OPTION_PKCS5:   return CryptPkcs5Cmd(interp, objc, objv);
        case OPTION_PRNG:    return CryptPrngCmd(interp, objc, objv);
        case OPTION_RAND:    return CryptRandCmd(interp, objc, objv);
        case OPTION_START:   return CryptStartCmd(interp, objc, objv, statePtr);
        case OPTION_UPDATE:  return CryptUpdateCmd(interp, objc, objv, statePtr);
    }

    // This point should never be reached.
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
