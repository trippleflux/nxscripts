#include <nxHelper.h>

static const CHAR s_chBase64EncodingTable[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',
    'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',
    'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

static INT Base64EncodeBufferSize(INT nSrcLen)
{
    INT nRet, nOnLastLine;

    nRet = (nSrcLen*4/3) + (nSrcLen % 3);
    nOnLastLine = nRet % 76;

    if (nOnLastLine && nOnLastLine % 4) {
        nRet += 4-(nOnLastLine % 4);
    }

    return nRet;
}

static INT Base64Encode(const BYTE *pbSrcData, INT nSrcLen, LPSTR szDest, INT *pnDestLen)
{
    DWORD dwCurr;
    INT nLen1 = (nSrcLen/3)*4;
    INT nLen2 = nLen1/76;
    INT nLen3 = 19;
    INT nWritten = 0;
    INT i, j, k, n;

    if (!pbSrcData || !szDest || !pnDestLen) {
        return BASE64_BADPARAM;
    }

    for (i = 0; i <= nLen2; i++) {
        if (i==nLen2) {
            nLen3 = (nLen1%76)/4;
        }

        for (j = 0; j < nLen3; j++) {
            dwCurr = 0;
            for (n = 0; n < 3; n++) {
                dwCurr |= *pbSrcData++;
                dwCurr <<= 8;
            }
            for (k = 0; k < 4; k++) {
                BYTE b = (BYTE)(dwCurr>>26);
                *szDest++ = s_chBase64EncodingTable[b];
                dwCurr <<= 6;
            }
        }
        nWritten += nLen3*4;
    }

    nLen2 = nSrcLen%3 ? nSrcLen%3 + 1 : 0;
    if (nLen2) {
        BYTE b;
        dwCurr = 0;

        for (n = 0; n < 3; n++) {
            if (n<(nSrcLen%3)) {
                dwCurr |= *pbSrcData++;
            }
            dwCurr <<= 8;
        }
        for (k = 0; k < nLen2; k++) {
            b = (BYTE)(dwCurr>>26);
            *szDest++ = s_chBase64EncodingTable[b];
            dwCurr <<= 6;
        }
        nWritten += nLen2;

        nLen3 = nLen2 ? 4-nLen2 : 0;
        for (j = 0; j < nLen3; j++) {
            *szDest++ = '=';
        }
        nWritten += nLen3;
    }

    *pnDestLen = nWritten;
    return BASE64_SUCCESS;
}

static INT Base64DecodeBufferSize(INT nSrcLen)
{
    return nSrcLen;
}

__inline INT Base64DecodeChar(UINT ch)
{
    // returns -1 if the character is invalid
    // or should be skipped
    // otherwise, returns the 6-bit code for the character
    // from the encoding table
    if (ch >= 'A' && ch <= 'Z')
        return ch - 'A' + 0;    // 0 range starts at 'A'
    if (ch >= 'a' && ch <= 'z')
        return ch - 'a' + 26;   // 26 range starts at 'a'
    if (ch >= '0' && ch <= '9')
        return ch - '0' + 52;   // 52 range starts at '0'
    if (ch == '+')
        return 62;
    if (ch == '/')
        return 63;
    return -1;
}

static INT Base64Decode(LPCSTR szSrc, INT nSrcLen, BYTE *pbDest, INT *pnDestLen)
{
    BOOL bOverflow = (pbDest == NULL) ? TRUE : FALSE;
    DWORD dwCurr;
    INT nBits, nCh, i;
    INT nWritten = 0;
    LPCSTR szSrcEnd = szSrc + nSrcLen;

    if (szSrc == NULL || pnDestLen == NULL) {
        return BASE64_BADPARAM;
    }

    // walk the source buffer
    // each four character sequence is converted to 3 bytes
    // CRLFs and =, and any characters not in the encoding table
    // are skipped

    while (szSrc < szSrcEnd) {
        dwCurr = 0;
        nBits = 0;

        for (i = 0; i < 4; i++) {
            if (szSrc >= szSrcEnd) {
                break;
            }

            nCh = Base64DecodeChar(*szSrc);
            szSrc++;
            if (nCh == -1) {
                // skip this char
                i--;
                continue;
            }
            dwCurr <<= 6;
            dwCurr |= nCh;
            nBits += 6;
        }

        if (!bOverflow && nWritten + (nBits/8) > (*pnDestLen)) {
            bOverflow = TRUE;
        }

        // dwCurr has the 3 bytes to write to the output buffer
        // left to right
        dwCurr <<= 24-nBits;
        for (i = 0; i < nBits/8; i++) {
            if (!bOverflow) {
                *pbDest = (BYTE) ((dwCurr & 0x00ff0000) >> 16);
                pbDest++;
            }
            dwCurr <<= 8;
            nWritten++;
        }
    }

    *pnDestLen = nWritten;

    if (bOverflow) {
        return BASE64_OVERFLOW;
    }

    return BASE64_SUCCESS;
}

INT TclDecodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    INT nDestLen, nSrcLen, nError;
    BYTE *pbDest;
    CHAR *szSrc;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "string");
        return TCL_ERROR;
    }

    // Retrieve the data
    szSrc = Tcl_GetStringFromObj(objv[1], &nSrcLen);

    // Create and allocation a destination object
    nDestLen = Base64DecodeBufferSize(nSrcLen);
    if (!(pbDest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), nDestLen))) {
        TclBase64Error(interp, "unable to decode data: ", BASE64_NOMEM);
        return TCL_ERROR;
    }

    // Decode the buffer
    if ((nError = Base64Decode(szSrc, nSrcLen, pbDest, &nDestLen)) != BASE64_SUCCESS) {
        TclBase64Error(interp, "unable to decode data: ", nError);
        return TCL_ERROR;
    }

    // Set result object to actual length of decoded data
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), nDestLen);
    return TCL_OK;
}

INT TclEncodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    INT nDestLen, nSrcLen, nError;
    BYTE *pbDest, *pbSrc;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "data");
        return TCL_ERROR;
    }

    // Retrieve the data
    pbSrc = Tcl_GetByteArrayFromObj(objv[1], &nSrcLen);

    // Create and allocation a destination object
    nDestLen = Base64EncodeBufferSize(nSrcLen);
    if (!(pbDest = Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), nDestLen))) {
        TclBase64Error(interp, "unable to encode data: ", BASE64_NOMEM);
        return TCL_ERROR;
    }

    // Encode the buffer
    if ((nError = Base64Encode(pbSrc, nSrcLen, pbDest, &nDestLen)) != BASE64_SUCCESS) {
        TclBase64Error(interp, "unable to encode data: ", nError);
        return TCL_ERROR;
    }

    // Set result object to actual length of encoded data
    Tcl_SetByteArrayLength(Tcl_GetObjResult(interp), nDestLen);
    return TCL_OK;
}

static VOID TclBase64Error(Tcl_Interp *interp, LPCSTR szMsg, INT nError)
{
    CHAR *pszError;

    switch (nError) {
        case BASE64_SUCCESS : pszError = "no error"           ; break;
        case BASE64_BADPARAM: pszError = "invalid parameter"  ; break;
        case BASE64_NOMEM   : pszError = "insufficient memory"; break;
        case BASE64_OVERFLOW: pszError = "buffer overflow"    ; break;
        default             : pszError = "unknown error"      ;
    }

    Tcl_ResetResult(interp);
    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), szMsg, pszError, NULL);
}
