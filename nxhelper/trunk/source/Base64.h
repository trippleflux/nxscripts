#ifndef __BASE64_H__
#define __BASE64_H__

#define BASE64_SUCCESS      1
#define BASE64_BADPARAM     2
#define BASE64_NOMEM        3
#define BASE64_OVERFLOW     4

static INT Base64EncodeBufferSize(INT nSrcLen);
static INT Base64Encode(const BYTE *pbSrcData, INT nSrcLen, LPSTR szDest, INT *pnDestLen);
static INT Base64DecodeBufferSize(INT nSrcLen);
__inline INT Base64DecodeChar(UINT ch);
static INT Base64Decode(LPCSTR szSrc, INT nSrcLen, BYTE *pbDest, INT *pnDestLen);

INT  TclDecodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);
INT  TclEncodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);
static VOID TclBase64Error(Tcl_Interp *interp, LPCSTR szMsg, INT nError);

#endif // __BASE64_H__
