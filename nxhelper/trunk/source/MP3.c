#include <nxHelper.h>

INT TclMp3Cmd(ClientData dummy, Tcl_Interp *interp, INT objc, Tcl_Obj *CONST objv[])
{
    CHAR *pszFile;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "file varName");
        return TCL_ERROR;
    }
    pszFile = Tcl_GetString(objv[1]);

    Tcl_SetIntObj(Tcl_GetObjResult(interp), 0);

    return TCL_OK;
}
