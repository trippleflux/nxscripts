/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoWinIoFtpd.c

Author:
    neoxed (neoxed@gmail.com) Apr 16, 2005

Abstract:
    Implements a Tcl command-based interface for interaction with ioFTPD.

    User Commands:
    ioftpd user exists  <msgWindow> <user>            - Check if a user exists.
    ioftpd user get     <msgWindow> <user> <varName>  - Query a user.
    ioftpd user id      <msgWindow> <user>            - Resolve a user name to its UID.
    ioftpd user list    <msgWindow> [-id] [-name]     - List users and/or UIDs.
    ioftpd user name    <msgWindow> <uid>             - Resolve a UID to its user name.

    Group Commands:
    ioftpd group exists <msgWindow> <group>           - Check if a group exists.
    ioftpd group get    <msgWindow> <group> <varName> - Query a group.
    ioftpd group id     <msgWindow> <group>           - Resolve a group name to its GID.
    ioftpd group list   <msgWindow> [-id] [-name]     - List groups and/or GIDs.
    ioftpd group name   <msgWindow> <gid>             - Resolve a GID to its group name.

    Online Data Commands:
    ioftpd info         <msgWindow> <varName>         - Query the ioFTPD process.
    ioftpd kick         <msgWindow> <user>            - Kick a user.
    ioftpd kill         <msgWindow> <cid>             - Kick a connection ID.
    ioftpd who          <msgWindow> <fields>          - Query online users.

--*/

#include <alcoExt.h>

static int
IoGroupCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
IoInfoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
IoKickCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
IoKillCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
IoUserCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
IoWhoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );


// TODO: Add user, gid, and group fields (not available in struct).
static const char *whoFields[] = {
    "action",
    "cid",
    "host",
    "ident",
    "idletime",
    "ip",
    "logintime",
    "realdatapath",
    "realpath",
    "size",
    "speed",
    "status",
    "uid",
    "vdatapath",
    "vpath",
    NULL
};

enum {
    WHO_ACTION = 0,
    WHO_CID,
    WHO_HOST,
    WHO_IDENT,
    WHO_IDLETIME,
    WHO_IP,
    WHO_LOGINTIME,
    WHO_REALDATAPATH,
    WHO_REALPATH,
    WHO_SIZE,
    WHO_SPEED,
    WHO_STATUS,
    WHO_UID,
    WHO_VDATAPATH,
    WHO_VPATH,
};


/*++

IoGroupCmd

    List, query, and resolve ioFTPD groups.

Arguments:
    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
IoGroupCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    static const char *options[] = {
        "exists", "get", "id", "list", "name", NULL
    };
    enum options {
        GROUP_EXISTS, GROUP_GET, GROUP_ID, GROUP_LIST, GROUP_NAME
    };

    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "option msgWindow ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case GROUP_EXISTS: {
            break;
        }
        case GROUP_GET: {
            break;
        }
        case GROUP_ID: {
            break;
        }
        case GROUP_LIST: {
            break;
        }
        case GROUP_NAME: {
            break;
        }
    }

    // TODO: IPC stuff.

    return TCL_OK;
}

/*++

IoInfoCmd

    Retrieves information from an ioFTPD message window.

Arguments:
    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
IoInfoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow varName");
        return TCL_ERROR;
    }

    // TODO: Process stuff.

    //
    // path - ioFTPD image path (C:\ioFTPD\system\ioFTPD.exe).
    // pid  - Process ID.
    // time - Start time, seconds since epoch.
    //

    return TCL_OK;
}

/*++

IoKickCmd

    Kicks the specified ioFTPD user.

Arguments:
    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
IoKickCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow user");
        return TCL_ERROR;
    }

    // TODO: IPC stuff.

    return TCL_OK;
}

/*++

IoKillCmd

    Kills the specified ioFTPD connection ID.

Arguments:
    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
IoKillCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow cid");
        return TCL_ERROR;
    }

    // TODO: IPC stuff.

    return TCL_OK;
}

/*++

IoUserCmd

    List, query, and resolve ioFTPD users.

Arguments:
    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
IoUserCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    static const char *options[] = {
        "exists", "get", "id", "list", "name", NULL
    };
    enum options {
        USER_EXISTS, USER_GET, USER_ID, USER_LIST, USER_NAME
    };

    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "option msgWindow ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case USER_EXISTS: {
            break;
        }
        case USER_GET: {
            break;
        }
        case USER_ID: {
            break;
        }
        case USER_LIST: {
            break;
        }
        case USER_NAME: {
            break;
        }
    }

    // TODO: IPC stuff.

    return TCL_OK;
}

/*++

IoWhoCmd

    Retrieves online user information from ioFTPD's shared memory.

Arguments:
    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
IoWhoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int elementCount;
    int fieldIndex;
    int i;
    int result = TCL_ERROR;
    unsigned char *fields;
    Tcl_Obj **elementPtrs;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow fields");
        return TCL_ERROR;
    }

    if (Tcl_ListObjGetElements(interp, objv[3], &elementCount, &elementPtrs) != TCL_OK) {
        return TCL_ERROR;
    }

    fields = (unsigned char *)ckalloc(elementCount * sizeof(unsigned char));

    // Create an array of indices from 'whoFields'.
    for (i = 0; i < elementCount; i++) {
        if (Tcl_GetIndexFromObj(interp, elementPtrs[i], whoFields, "field", 0,
            &fieldIndex) != TCL_OK) {
            goto end;
        }

        fields[i] = (unsigned char)fieldIndex;
    }

    // TODO: IPC stuff.
    result = TCL_OK;

    end:
    ckfree((char *) fields);
    return result;
}

/*++

IoFtpdObjCmd

  	This function provides the "ioftpd" Tcl command.

Arguments:
    dummy  - Not used.

    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
int
IoFtpdObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    int index;
    static const char *options[] = {
        "group", "info", "kick", "kill", "user", "who", NULL
    };
    enum options {
        OPTION_GROUP, OPTION_INFO, OPTION_KICK, OPTION_KILL, OPTION_USER, OPTION_WHO
    };

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum options) index) {
        case OPTION_GROUP: return IoGroupCmd(interp, objc, objv);
        case OPTION_INFO:  return IoInfoCmd(interp, objc, objv);
        case OPTION_KICK:  return IoKickCmd(interp, objc, objv);
        case OPTION_KILL:  return IoKillCmd(interp, objc, objv);
        case OPTION_USER:  return IoUserCmd(interp, objc, objv);
        case OPTION_WHO:   return IoWhoCmd(interp, objc, objv);
    }

    // This point should never be reached.
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
