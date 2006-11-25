/*
 * nxHelper - Tcl extension for nxTools.
 * Copyright (c) 2004-2006 neoxed
 *
 * File Name:
 *   nxBase64.c
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 22, 2005
 *
 * Abstract:
 *   Implements base64 encoding and decoding commands for Tcl.
 *
 *   Tcl Commands:
 *     ::nx::base64 encode <data>
 *       - Returns "data" encoded in base64.
 *       - An error is raised if an overflow occurs.
 *
 *     ::nx::base64 decode <data>
 *       - Returns "data" decoded from base64.
 *       - An error is raised if the encoded string is invalid.
 */

#include <nxHelper.h>

/* Status codes. */
#define BASE64_SUCCESS  0
#define BASE64_INVALID  1
#define BASE64_OVERFLOW 2

/* Required destination length. */
#define Base64EncodeGetDestLength(length) ((4 * ((length + 2) / 3)) + 1)
#define Base64DecodeGetDestLength(length) (length)

static const char *codes =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char map[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
     52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
    255, 254, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,
      7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
     19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
    255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
     37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
     49,  50,  51, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255
};


/*
 * Base64Decode
 *
 *   Decode a buffer of base64 data.
 *
 * Arguments:
 *   source       - Source buffer containing the base64 data to decode.
 *   sourceLength - Length of the source buffer.
 *   dest         - Destination buffer of the binary decoded data.
 *   destLength   - The max size of the destination buffer and resulting size.
 *
 * Returns:
 *   A base64 status code; BASE64_SUCCESS will be returned if successful.
 */
static int
Base64Decode(
    unsigned char *source,
    unsigned long sourceLength,
    unsigned char *dest,
    unsigned long *destLength
    )
{
    unsigned long t, x, y, z;
    unsigned char c;
    int           g;

    g = 3;
    for (x = y = z = t = 0; x < sourceLength; x++) {
        c = map[source[x]&0xFF];
        if (c == 255) {
            continue;
        }
        /* the final = symbols are read and used to trim the remaining bytes */
        if (c == 254) {
            c = 0;
            /* prevent g < 0 which would potentially allow an overflow later */
            if (--g < 0) {
                return BASE64_INVALID;
            }
        } else if (g != 3) {
            /* we only allow = to be at the end */
            return BASE64_INVALID;
        }

        t = (t<<6)|c;

        if (++y == 4) {
            if (z + g > *destLength) {
                return BASE64_OVERFLOW;
            }
            dest[z++] = (unsigned char)((t>>16)&255);
            if (g > 1) dest[z++] = (unsigned char)((t>>8)&255);
            if (g > 2) dest[z++] = (unsigned char)(t&255);
            y = t = 0;
        }
    }

    if (y != 0) {
        return BASE64_INVALID;
    }

    *destLength = z;
    return BASE64_SUCCESS;
}

/*
 * Base64Encode
 *
 *   Encode a buffer of data in base64.
 *
 * Arguments:
 *   source       - Source buffer to encode.
 *   sourceLength - Length of the source buffer.
 *   dest         - Destination buffer for the base64 encoded data.
 *   destLength   - The max size of the destination buffer and resulting size.
 *
 * Returns:
 *   A base64 status code; BASE64_SUCCESS will be returned if successful.
 */
static int
Base64Encode(
    unsigned char *source,
    unsigned long sourceLength,
    unsigned char *dest,
    unsigned long *destLength
    )
{
    unsigned long i, len2, leven;
    unsigned char *p;

    /* valid output size ? */
    len2 = 4 * ((sourceLength + 2) / 3);
    if (*destLength < len2 + 1) {
        return BASE64_OVERFLOW;
    }
    p = dest;
    leven = 3*(sourceLength / 3);
    for (i = 0; i < leven; i += 3) {
        *p++ = codes[(source[0] >> 2) & 0x3F];
        *p++ = codes[(((source[0] & 3) << 4) + (source[1] >> 4)) & 0x3F];
        *p++ = codes[(((source[1] & 0xf) << 2) + (source[2] >> 6)) & 0x3F];
        *p++ = codes[source[2] & 0x3F];
        source += 3;
    }
    /* Pad it if necessary...  */
    if (i < sourceLength) {
        unsigned a = source[0];
        unsigned b = (i+1 < sourceLength) ? source[1] : 0;

        *p++ = codes[(a >> 2) & 0x3F];
        *p++ = codes[(((a & 3) << 4) + (b >> 4)) & 0x3F];
        *p++ = (i+1 < sourceLength) ? codes[(((b & 0xf) << 2)) & 0x3F] : '=';
        *p++ = '=';
    }

    /* append a NULL byte */
    *p = '\0';

    /* return ok */
    *destLength = p - dest;
    return BASE64_SUCCESS;
}

/*
 * Base64GetError
 *
 *   Retrieve an error message.
 *
 * Arguments:
 *   status - A base64 status code.
 *
 * Returns:
 *   A human-readable error message.
 */
static char *
Base64GetError(
    unsigned short status
    )
{
    switch (status) {
        case BASE64_SUCCESS : return "no error";
        case BASE64_INVALID : return "invalid string";
        case BASE64_OVERFLOW: return "buffer overflow";
        default             : return "unknown error";
    }
}

/*
 * Base64ObjCmd
 *
 *   This function provides the "::nx::base64" Tcl command.
 *
 * Arguments:
 *   dummy  - Not used.
 *   interp - Current interpreter.
 *   objc   - Number of arguments.
 *   objv   - Argument objects.
 *
 * Returns:
 *   A standard Tcl result.
 */
int
Base64ObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    int sourceLength;
    unsigned char *dest;
    unsigned char *source;
    unsigned long destLength;
    unsigned short status;
    static const char *options[] = {
        "decode", "encode", NULL
    };
    enum optionIndices {
        OPTION_DECODE = 0, OPTION_ENCODE
    };

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "option data");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum optionIndices) index) {
        case OPTION_DECODE: {
            source = Tcl_GetStringFromObj(objv[2], &sourceLength);

            /* Create a destination byte object for decoded data. */
            destLength = Base64DecodeGetDestLength(sourceLength);
            dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

            status = Base64Decode(source, (unsigned long)sourceLength, dest, &destLength);
            if (status != BASE64_SUCCESS) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "unable to decode data: ",
                    Base64GetError(status), NULL);

                return TCL_ERROR;
            }

            /* Update the object's length. */
            Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);
            return TCL_OK;
        }
        case OPTION_ENCODE: {
            source = Tcl_GetByteArrayFromObj(objv[2], &sourceLength);

            /* Create a destination byte object for encoded data. */
            destLength = Base64EncodeGetDestLength(sourceLength);
            dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);

            status = Base64Encode(source, (unsigned long)sourceLength, dest, &destLength);
            if (status != BASE64_SUCCESS) {
                Tcl_ResetResult(interp);
                Tcl_AppendResult(interp, "unable to encode data: ",
                    Base64GetError(status), NULL);

                return TCL_ERROR;
            }

            /* Update the object's length. */
            Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), (int)destLength);
            return TCL_OK;
        }
    }

    /* This point is never reached. */
    assert(0);
    return TCL_ERROR;
}
