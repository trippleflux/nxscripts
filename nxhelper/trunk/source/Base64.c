#include <nxHelper.h>

static LONG Base64EncodeBufferSize(LONG SourceLen);
static LONG Base64Encode(PBYTE SourceData, LONG SourceLen, PCHAR Dest, PLONG DestLen);
static LONG Base64DecodeBufferSize(LONG SourceLen);
__inline LONG Base64DecodeChar(ULONG ch);
static LONG Base64Decode(PCHAR SourceData, LONG SourceLen, PBYTE Dest, PLONG DestLen);
static VOID TclBase64Error(Tcl_Interp *interp, PCHAR Message, LONG ErrorNum);

static const CHAR Base64EncodingTable[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
    'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
    'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};


static LONG Base64EncodeBufferSize(LONG SourceLen)
{
    LONG Length;
    LONG OnLastLine;

    Length = (SourceLen*4/3) + (SourceLen % 3);
    OnLastLine = Length % 76;

    if (OnLastLine && OnLastLine % 4) {
        Length += 4-(OnLastLine % 4);
    }

    return Length;
}


static LONG Base64Encode(PBYTE SourceData, LONG SourceLen, PCHAR Dest, PLONG DestLen)
{
    ULONG Current;
    LONG Len1 = (SourceLen/3)*4;
    LONG Len2 = Len1/76;
    LONG Len3 = 19;
    LONG Written = 0;
    LONG i, j, k, n;

    if (!SourceData || !Dest || !DestLen) {
        return BASE64_BADPARAM;
    }

    for (i = 0; i <= Len2; i++) {
        if (i == Len2) {
            Len3 = (Len1%76)/4;
        }

        for (j = 0; j < Len3; j++) {
            Current = 0;
            for (n = 0; n < 3; n++) {
                Current |= *SourceData++;
                Current <<= 8;
            }
            for (k = 0; k < 4; k++) {
                BYTE b = (BYTE)(Current>>26);
                *Dest++ = Base64EncodingTable[b];
                Current <<= 6;
            }
        }
        Written += Len3*4;
    }

    Len2 = SourceLen%3 ? SourceLen%3 + 1 : 0;
    if (Len2) {
        BYTE b;
        Current = 0;

        for (n = 0; n < 3; n++) {
            if (n<(SourceLen%3)) {
                Current |= *SourceData++;
            }
            Current <<= 8;
        }
        for (k = 0; k < Len2; k++) {
            b = (BYTE)(Current>>26);
            *Dest++ = Base64EncodingTable[b];
            Current <<= 6;
        }
        Written += Len2;

        Len3 = Len2 ? 4-Len2 : 0;
        for (j = 0; j < Len3; j++) {
            *Dest++ = '=';
        }
        Written += Len3;
    }

    *DestLen = Written;
    return BASE64_SUCCESS;
}


static LONG Base64DecodeBufferSize(LONG SourceLen)
{
    return SourceLen;
}


__inline LONG Base64DecodeChar(ULONG Char)
{
    // returns -1 if the character is invalid
    // or should be skipped
    // otherwise, returns the 6-bit code for the character
    // from the encoding table
    if (Char >= 'A' && Char <= 'Z')
        return Char - 'A' + 0;    // 0 range starts at 'A'
    if (Char >= 'a' && Char <= 'z')
        return Char - 'a' + 26;   // 26 range starts at 'a'
    if (Char >= '0' && Char <= '9')
        return Char - '0' + 52;   // 52 range starts at '0'
    if (Char == '+')
        return 62;
    if (Char == '/')
        return 63;
    return -1;
}


static LONG Base64Decode(PCHAR SourceData, LONG SourceLen, PBYTE Dest, PLONG DestLen)
{
    BOOL Overflow = (Dest == NULL) ? TRUE : FALSE;
    LONG Bits;
    LONG Char;
    LONG i;
    LONG Written = 0;
    PCHAR SourceDataEnd = SourceData + SourceLen;
    ULONG Current;

    if (SourceData == NULL || DestLen == NULL) {
        return BASE64_BADPARAM;
    }

    // walk the source buffer
    // each four character sequence is converted to 3 bytes
    // CRLFs and =, and any characters not in the encoding table
    // are skipped

    while (SourceData < SourceDataEnd) {
        Current = 0;
        Bits = 0;

        for (i = 0; i < 4; i++) {
            if (SourceData >= SourceDataEnd) {
                break;
            }

            Char = Base64DecodeChar(*SourceData);
            SourceData++;
            if (Char == -1) {
                // skip this char
                i--;
                continue;
            }
            Current <<= 6;
            Current |= Char;
            Bits += 6;
        }

        if (!Overflow && Written + (Bits/8) > (*DestLen)) {
            Overflow = TRUE;
        }

        // Current has the 3 bytes to write to the output buffer
        // left to right
        Current <<= 24-Bits;
        for (i = 0; i < Bits/8; i++) {
            if (!Overflow) {
                *Dest = (BYTE) ((Current & 0x00ff0000) >> 16);
                Dest++;
            }
            Current <<= 8;
            Written++;
        }
    }

    *DestLen = Written;

    if (Overflow) {
        return BASE64_OVERFLOW;
    }

    return BASE64_SUCCESS;
}


INT TclDecodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    LONG DestLen;
    LONG ErrorNum;
    LONG SourceLen;
    PBYTE Dest;
    PCHAR SourceData;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "string");
        return TCL_ERROR;
    }

    // Retrieve the data
    SourceData = Tcl_GetStringFromObj(objv[1], &SourceLen);

    // Create and allocation a destination object
    DestLen = Base64DecodeBufferSize(SourceLen);
    if (!(Dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), DestLen))) {
        TclBase64Error(interp, "unable to decode data: ", BASE64_NOMEM);
        return TCL_ERROR;
    }

    // Decode the buffer
    if ((ErrorNum = Base64Decode(SourceData, SourceLen, Dest, &DestLen)) != BASE64_SUCCESS) {
        TclBase64Error(interp, "unable to decode data: ", ErrorNum);
        return TCL_ERROR;
    }

    // Set result object to actual length of decoded data
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), DestLen);
    return TCL_OK;
}


INT TclEncodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    LONG DestLen;
    LONG ErrorNum;
    LONG SourceLen;
    PBYTE Dest;
    PBYTE SourceData;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "data");
        return TCL_ERROR;
    }

    // Retrieve the data
    SourceData = Tcl_GetByteArrayFromObj(objv[1], &SourceLen);

    // Create and allocation a destination object
    DestLen = Base64EncodeBufferSize(SourceLen);
    if (!(Dest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), DestLen))) {
        TclBase64Error(interp, "unable to encode data: ", BASE64_NOMEM);
        return TCL_ERROR;
    }

    // Encode the buffer
    if ((ErrorNum = Base64Encode(SourceData, SourceLen, Dest, &DestLen)) != BASE64_SUCCESS) {
        TclBase64Error(interp, "unable to encode data: ", ErrorNum);
        return TCL_ERROR;
    }

    // Set result object to actual length of encoded data
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), DestLen);
    return TCL_OK;
}


static VOID TclBase64Error(Tcl_Interp *interp, PCHAR Message, LONG ErrorNum)
{
    PCHAR Error;

    switch (ErrorNum) {
        case BASE64_SUCCESS : Error = "no error"           ; break;
        case BASE64_BADPARAM: Error = "invalid parameter"  ; break;
        case BASE64_NOMEM   : Error = "insufficient memory"; break;
        case BASE64_OVERFLOW: Error = "buffer overflow"    ; break;
        default             : Error = "unknown error"      ;
    }

    Tcl_ResetResult(interp);
    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), Message, Error, NULL);
}
