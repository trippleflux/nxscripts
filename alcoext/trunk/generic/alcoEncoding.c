/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoEncoding.c

Author:
    neoxed (neoxed@gmail.com) May 21, 2005

Abstract:
    This module implements a interface to process base64 and hex encodings.

    encode <encoding> <data>
      - Encodes the given data in the specified format.
      - The "encoding" parameter must be "base64" or "hex".

    decode <encoding> <data>
      - Decodes the given data from the specified format.
      - The "encoding" parameter must be "base64" or "hex".
      - An error is raised if the given data cannot be decoded.

--*/

#include <alcoExt.h>

static EncLengthProc Base64DecodeGetDestLength;
static EncLengthProc Base64EncodeGetDestLength;
static EncLengthProc HexDecodeGetDestLength;
static EncLengthProc HexEncodeGetDestLength;
static EncProcessProc HexDecode;
static EncProcessProc HexEncode;
inline char HexDecodeChar(char c);

const EncodingFuncts encodeFuncts[ENCODING_TYPES] = {
    {"base64", Base64EncodeGetDestLength, base64_encode},
    {"hex",       HexEncodeGetDestLength, HexEncode},
    {NULL}
};

const EncodingFuncts decodeFuncts[ENCODING_TYPES] = {
    {"base64", Base64DecodeGetDestLength, base64_decode},
    {"hex",       HexDecodeGetDestLength, HexDecode},
    {NULL}
};


/*++

Base64DecodeGetDestLength
Base64EncodeGetDestLength
HexDecodeGetDestLength
HexEncodeGetDestLength

    Calculate the required destination buffer size for various encodings.

Arguments:
    sourceLength  - The length of the source buffer.

Return Value:
    The required destination buffer size to hold the encoded or decoded data.

--*/
static unsigned long
Base64DecodeGetDestLength(unsigned long sourceLength)
{
    return sourceLength;
}

static unsigned long
Base64EncodeGetDestLength(unsigned long sourceLength)
{
    return (4 * ((sourceLength + 2) / 3)) + 1;
}

static unsigned long
HexDecodeGetDestLength(unsigned long sourceLength)
{
    return (sourceLength / 2) + 1;
}

static unsigned long
HexEncodeGetDestLength(unsigned long sourceLength)
{
    return (sourceLength * 2) + 1;
}

/*++

HexDecode

    Decode a buffer of hex data.

Arguments:
    source       - Source buffer containing the hex data to decode.

    sourceLength - Length of the source buffer.

    dest         - Destination buffer of the binary decoded data.

    destLength   - The max size of the destination buffer and resulting size.

Return Value:
    A LibTomCrypt status code; CRYPT_OK will be returned if successful.

--*/
static int
HexDecode(
    const unsigned char *source,
    unsigned long sourceLength,
    unsigned char *dest,
    unsigned long *destLength
    )
{
    char c1;
    char c2;
    unsigned long i;
    unsigned long length = 0;

    if (*destLength < (sourceLength / 2) + 1) {
        return CRYPT_BUFFER_OVERFLOW;
    }

    for (i = 0; i < sourceLength; i += 2) {
        c1 = HexDecodeChar((char) *source++);
        c2 = HexDecodeChar((char) *source++);
        if (c1 < 0 || c2 < 0) {
            return CRYPT_INVALID_PACKET;
        }
        *dest++ = (unsigned char)(16 * c1 + c2);
        length++;
    }

    *destLength = length;
    return CRYPT_OK;
}

inline char
HexDecodeChar(
    char ch
    )
{
    if (ch >= '0' && ch <= '9') {
        return (ch - '0');
    }
    if (ch >= 'A' && ch <= 'F') {
        return (ch - 'A' + 10);
    }
    if (ch >= 'a' && ch <= 'f') {
        return (ch - 'a' + 10);
    }
    return -1;
}

/*++

HexEncode

    Encode a buffer of data in hex.

Arguments:
    source       - Source buffer to encode.

    sourceLength - Length of the source buffer.

    dest         - Destination buffer for the hex encoded data.

    destLength   - The max size of the destination buffer and resulting size.

Return Value:
    A LibTomCrypt status code; CRYPT_OK will be returned if successful.

--*/
static int
HexEncode(
    const unsigned char *source,
    unsigned long sourceLength,
    unsigned char *dest,
    unsigned long *destLength
    )
{
    unsigned char c;
    unsigned long i;
    unsigned long length = 0;
    static const char hexChars[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F'
    };

    if (*destLength < (sourceLength * 2) + 1) {
        return CRYPT_BUFFER_OVERFLOW;
    }

    for (i = 0; i < sourceLength; i++) {
        c = *source++;
        *dest++ = hexChars[(c >> 4) & 0x0F];
        *dest++ = hexChars[c & 0x0F];
        length += 2;
    }

    *destLength = length;
    return CRYPT_OK;
}

/*++

EncodingObjCmd

    This function provides the "encode" and "decode" Tcl commands.

Arguments:
    clientData - Pointer to an array of "EncodingFuncts" structures.

    interp     - Current interpreter.

    objc       - Number of arguments.

    objv       - Argument objects.

Return Value:
    A standard Tcl result.

--*/
int
EncodingObjCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    int sourceLength;
    int status;
    unsigned char *dest;
    unsigned char *source;
    unsigned long destLength;
    EncodingFuncts *functTable;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "encoding data");
        return TCL_ERROR;
    }

    functTable = (EncodingFuncts *)clientData;
    assert(functTable == decodeFuncts || functTable == encodeFuncts);

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], functTable,
        sizeof(EncodingFuncts), "encoding", TCL_EXACT, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    source = Tcl_GetByteArrayFromObj(objv[2], &sourceLength);

    // Create a byte object for the output data.
    destLength = functTable[index].GetDestLength(sourceLength);
    dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

    status = functTable[index].Process(source, (unsigned long)sourceLength, dest, &destLength);
    if (status != CRYPT_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to process data: ",
            error_to_string(status), NULL);

        return TCL_ERROR;
    }

    // Update the object's length.
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);
    return TCL_OK;
}
