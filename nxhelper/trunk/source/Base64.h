#ifndef __BASE64_H__
#define __BASE64_H__

#define BASE64_SUCCESS      1
#define BASE64_BADPARAM     2
#define BASE64_NOMEM        3
#define BASE64_OVERFLOW     4

static LONG Base64EncodeBufferSize(LONG SourceLen);
static LONG Base64Encode(PBYTE SourceData, LONG SourceLen, PCHAR Dest, PLONG DestLen);
static LONG Base64DecodeBufferSize(LONG SourceLen);
__inline LONG Base64DecodeChar(ULONG ch);
static LONG Base64Decode(PCHAR SourceData, LONG SourceLen, PBYTE Dest, PLONG DestLen);

INT TclDecodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);
INT TclEncodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);
static VOID TclBase64Error(Tcl_Interp *interp, PCHAR Message, LONG ErrorNum);

#endif // __BASE64_H__
