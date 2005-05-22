#ifndef __BASE64_H__
#define __BASE64_H__

#define BASE64_SUCCESS      1
#define BASE64_BADPARAM     2
#define BASE64_NOMEM        3
#define BASE64_OVERFLOW     4

INT TclDecodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);
INT TclEncodeCmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[]);

#endif // __BASE64_H__
