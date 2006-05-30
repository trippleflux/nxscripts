/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    ioFTPD Tcl Commands

Author:
    neoxed (neoxed@gmail.com) Apr 16, 2005

Abstract:
    Implements a Tcl command-based interface for interaction with ioFTPD.

    User Commands:
    ioftpd user create  <msgWindow> <user>         - Create a user.
    ioftpd user rename  <msgWindow> <user> <new>   - Rename a user.
    ioftpd user delete  <msgWindow> <user>         - Delete a user.
    ioftpd user exists  <msgWindow> <user>         - Check if a user exists.
    ioftpd user get     <msgWindow> <user>         - Get user information.
    ioftpd user set     <msgWindow> <user> <data>  - Set user information.
    ioftpd user toid    <msgWindow> <user>         - User name to UID.
    ioftpd user toname  <msgWindow> <uid>          - UID to user name.

    Group Commands:
    ioftpd group create <msgWindow> <group>        - Create a group.
    ioftpd group rename <msgWindow> <group> <new>  - Rename a group.
    ioftpd group delete <msgWindow> <group>        - Delete a group.
    ioftpd group exists <msgWindow> <group>        - Check if a group exists.
    ioftpd group get    <msgWindow> <group>        - Get group information.
    ioftpd group set    <msgWindow> <group> <data> - Set group information.
    ioftpd group toid   <msgWindow> <group>        - Group name to GID.
    ioftpd group toname <msgWindow> <gid>          - GID to group name.

    Online Data Commands:
    ioftpd info         <msgWindow> <varName>      - Query the ioFTPD message window.
    ioftpd kick         <msgWindow> <uid>          - Kick a user ID.
    ioftpd kill         <msgWindow> <cid>          - Kick a connection ID.
    ioftpd who          <msgWindow> <fields>       - Query online users.

    VFS Commands:
    ioftpd vfs attributes <msgWindow> <path>
    ioftpd vfs attributes <msgWindow> <path> -switch
    ioftpd vfs attributes <msgWindow> <path> -switch value
    ioftpd vfs flush      <msgWindow> <path>

--*/

// Strings in ioFTPD's headers are declared as TCHAR's, even though
// the shared memory interface does not support wide characters.
#undef UNICODE
#undef _UNICODE

#include <alcoExt.h>
#include "ioftpd\ServerLimits.h"
#include "ioftpd\UserFile.h"
#include "ioftpd\GroupFile.h"
#include "ioftpd\WinMessages.h"
#include "ioftpd\DataCopy.h"
#include "alcoWinIoShm.h"

//
// Row data parsing functions.
//

enum {
    TYPE_BIN = 0,   // Binary
    TYPE_I32,       // 32-bit integer
    TYPE_I32LIST,   // List of 32-bit integers (last item is -1)
    TYPE_I64,       // 64-bit integer
    TYPE_STR,       // String
    TYPE_STRLIST    // List of strings (last item is NULL)
};

typedef struct {
    char *name;     // Name of the row.
    int offset;     // Offset in the data structure.
    int type;       // Type of data contained in the row.
    int values;     // Number of values in the row.
    int bytes;      // Size of each value, in bytes.
} RowData;

static int
RowDataGet(
    const RowData *rowData,
    const void *data,
    Tcl_Obj *listObj
    );

static int
RowDataSet(
    Tcl_Interp *interp,
    Tcl_Obj *listObj,
    const RowData *rowData,
    void *data
    );

static const RowData userRowDef[] = {
    {"admingroups", offsetof(USERFILE, AdminGroups), TYPE_I32LIST, MAX_GROUPS,     sizeof(int)},
    {"alldn",       offsetof(USERFILE, AllDn),       TYPE_I64,     MAX_SECTIONS*3, sizeof(INT64)},
    {"allup",       offsetof(USERFILE, AllUp),       TYPE_I64,     MAX_SECTIONS*3, sizeof(INT64)},
    {"credits",     offsetof(USERFILE, Credits),     TYPE_I64,     MAX_SECTIONS,   sizeof(INT64)},
    {"daydn",       offsetof(USERFILE, DayDn),       TYPE_I64,     MAX_SECTIONS*3, sizeof(INT64)},
    {"dayup",       offsetof(USERFILE, DayUp),       TYPE_I64,     MAX_SECTIONS*3, sizeof(INT64)},
    {"flags",       offsetof(USERFILE, Flags),       TYPE_STR,     1,              sizeof(char) * (32 + 1)},
    {"groups",      offsetof(USERFILE, Groups),      TYPE_I32LIST, MAX_GROUPS,     sizeof(int)},
    {"home",        offsetof(USERFILE, Home),        TYPE_STR,     1,              sizeof(char) * (_MAX_PATH + 1)},
    {"ips",         offsetof(USERFILE, Ip),          TYPE_STRLIST, MAX_IPS,        sizeof(char) * (_IP_LINE_LENGTH + 1)},
    {"limits",      offsetof(USERFILE, Limits),      TYPE_I32,     5,              sizeof(int)},
    {"monthdn",     offsetof(USERFILE, MonthDn),     TYPE_I64,     MAX_SECTIONS*3, sizeof(INT64)},
    {"monthup",     offsetof(USERFILE, MonthUp),     TYPE_I64,     MAX_SECTIONS*3, sizeof(INT64)},
    {"password",    offsetof(USERFILE, Password),    TYPE_BIN,     1,              sizeof(BYTE) * 20},
    {"ratio",       offsetof(USERFILE, Ratio),       TYPE_I32,     MAX_SECTIONS,   sizeof(int)},
    {"tagline",     offsetof(USERFILE, Tagline),     TYPE_STR,     1,              sizeof(char) * (128 + 1)},
    {"vfsfile",     offsetof(USERFILE, MountFile),   TYPE_STR,     1,              sizeof(char) * (_MAX_PATH + 1)},
    {"wkdn",        offsetof(USERFILE, WkDn),        TYPE_I64,     MAX_SECTIONS*3, sizeof(INT64)},
    {"wkup",        offsetof(USERFILE, WkUp),        TYPE_I64,     MAX_SECTIONS*3, sizeof(INT64)},
    {NULL}
};

static const RowData groupRowDef[] = {
    {"desc",        offsetof(GROUPFILE, szDescription), TYPE_STR, 1, sizeof(char) * (128 + 1)},
    {"slots",       offsetof(GROUPFILE, Slots),         TYPE_I32, 2, sizeof(int)},
    {"users",       offsetof(GROUPFILE, Users),         TYPE_I32, 1, sizeof(int)},
    {"vfsfile",     offsetof(GROUPFILE, szVfsFile),     TYPE_STR, 1, sizeof(char) * (_MAX_PATH + 1)},
    {NULL}
};

//
// Online data functions.
//

#define ONLINE_GET_GROUPID  0x0001
#define ONLINE_GET_USERNAME 0x0002

static int
GetOnlineFields(
    ShmSession *session,
    Tcl_Interp *interp,
    unsigned char *fields,
    int fieldCount,
    unsigned short flags
    );

static const char *whoFields[] = {
    "action",
    "cid",
    "gid",
    "group",
    "host",
    "ident",
    "idletime",
    "ip",
    "logintime",
    "port",
    "realdatapath",
    "realpath",
    "service",
    "size",
    "speed",
    "status",
    "uid",
    "user",
    "vdatapath",
    "vpath",
    NULL
};

enum {
    WHO_ACTION = 0,
    WHO_CID,
    WHO_GID,
    WHO_GROUP,
    WHO_HOST,
    WHO_IDENT,
    WHO_IDLETIME,
    WHO_IP,
    WHO_LOGINTIME,
    WHO_PORT,
    WHO_REALDATAPATH,
    WHO_REALPATH,
    WHO_SERVICE,
    WHO_SIZE,
    WHO_SPEED,
    WHO_STATUS,
    WHO_UID,
    WHO_USER,
    WHO_VDATAPATH,
    WHO_VPATH
};

//
// Tcl command functions.
//

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
IoVfsCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

static int
IoVfsAttrsCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ShmSession *session,
    ShmMemory *memory,
    Tcl_DString *path
    );

static int
IoWhoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );


/*++

RowDataGet

    Parses data structure members into a Tcl list.

Arguments:
    rowData     - Row definition structure.

    data        - Buffer containing the data to be parsed.

    listObj     - Tcl object that receives the output.

Return Value:
    A standard Tcl result.

--*/
static int
RowDataGet(
    const RowData *rowData,
    const void *data,
    Tcl_Obj *listObj
    )
{
    int i;
    int j;
    void *dataOffset;
    Tcl_Obj *rowObj;
    Tcl_Obj *valueObj;

    assert(rowData == userRowDef || rowData == groupRowDef);
    assert(data    != NULL);
    assert(listObj != NULL);

    // Process each row.
    for (i = 0; rowData[i].name != NULL; i++) {
        assert(rowData[i].values > 0);
        assert(rowData[i].bytes > 0);

        Tcl_ListObjAppendElement(NULL, listObj, Tcl_NewStringObj(rowData[i].name, -1));
        rowObj = Tcl_NewObj();

        // Process each value in the row.
        for (j = 0; j < rowData[i].values; j++) {
            valueObj = NULL;
            dataOffset = (BYTE *)data + rowData[i].offset + (j * rowData[i].bytes);

            switch (rowData[i].type) {
                case TYPE_BIN: {
                    valueObj = Tcl_NewByteArrayObj((BYTE *)dataOffset, rowData[i].bytes);
                    break;
                }
                case TYPE_I32:
                case TYPE_I32LIST: {
                    int *value = (int *)dataOffset;

                    // The last list entry is -1.
                    if (rowData[i].type == TYPE_I32LIST && value[0] == -1) {
                        goto endOfRow;
                    }
                    valueObj = Tcl_NewIntObj(value[0]);
                    break;
                }
                case TYPE_I64: {
                    INT64 *value = (INT64 *)dataOffset;
                    valueObj = Tcl_NewWideIntObj((Tcl_WideInt)value[0]);
                    break;
                }
                case TYPE_STR:
                case TYPE_STRLIST: {
                    char *value = (char *)dataOffset;

                    // The last list entry is NULL.
                    if (rowData[i].type == TYPE_STRLIST && value[0] == '\0') {
                        goto endOfRow;
                    }
                    valueObj = Tcl_NewStringObj(value, -1);
                    break;
                }
                default: {
                    // Unknown data type.
                    assert(0);
                }
            }
            assert(valueObj != NULL);

            // Create a nested list if there's more than one value.
            if (rowData[i].values > 1) {
                Tcl_ListObjAppendElement(NULL, rowObj, valueObj);
            } else {
                Tcl_DecrRefCount(rowObj);
                rowObj = valueObj;
            }
        }

endOfRow:
        assert(rowObj != NULL);
        Tcl_ListObjAppendElement(NULL, listObj, rowObj);
    }

    return TCL_OK;
}

/*++

RowDataSet

    Parses Tcl list elements into a data structure.

Arguments:
    interp      - Interpreter to use for error reporting.

    listObj     - Tcl list object to be parsed.

    rowData     - Row definition structure.

    data        - Buffer that receives the output.

Return Value:
    A standard Tcl result.

--*/
static int
RowDataSet(
    Tcl_Interp *interp,
    Tcl_Obj *listObj,
    const RowData *rowData,
    void *data
    )
{
    int elementCount;
    int nestedCount;
    int i;
    int j;
    int rowIndex;
    void *dataOffset;
    Tcl_Obj **elementObjs;
    Tcl_Obj **nestedObjs;

    assert(interp  != NULL);
    assert(listObj != NULL);
    assert(rowData == userRowDef || rowData == groupRowDef);
    assert(data    != NULL);

    if (Tcl_ListObjGetElements(interp, listObj, &elementCount, &elementObjs) != TCL_OK) {
        return TCL_ERROR;
    }

    // The list elements are paired with a name and data value.
    // Example: alldn {1 2 3} allup {4 5 6} tagline {blah blah blah}
    if (elementCount & 1) {
        Tcl_SetResult(interp, "list must have an even number of elements", TCL_STATIC);
        return TCL_ERROR;
    }

    for (i = 0; i < elementCount; i++) {
        // Process the name value.
        if (Tcl_GetIndexFromObjStruct(interp, elementObjs[i], rowData,
                sizeof(RowData), "field", TCL_EXACT, &rowIndex) != TCL_OK) {
            return TCL_ERROR;
        }

        // Process the data value.
        i++;
        assert(rowData[rowIndex].values > 0);
        assert(rowData[rowIndex].bytes > 0);

        if (rowData[rowIndex].values == 1) {
            // Single data value.
            nestedCount = 1;
            nestedObjs = &elementObjs[i];
        } else {
            // Nested data values.
            if (Tcl_ListObjGetElements(interp, elementObjs[i], &nestedCount, &nestedObjs) != TCL_OK) {
                return TCL_ERROR;
            }

            if (rowData[rowIndex].type == TYPE_I32LIST || rowData[rowIndex].type == TYPE_STRLIST) {
                // The xxxLIST rows do not have a fixed number of values.
                if (nestedCount > rowData[rowIndex].values) {
                    Tcl_AppendResult(interp, "too many list elements for the \"",
                        rowData[rowIndex].name, "\" field", NULL);
                    return TCL_ERROR;
                }

            } else if (nestedCount != rowData[rowIndex].values) {
                char num[12];
                StringCchPrintfA(num, ARRAYSIZE(num), "%d", rowData[rowIndex].values);

                Tcl_AppendResult(interp, "the \"", rowData[rowIndex].name,
                    "\" field must have ", num, " list elements", NULL);
                return TCL_ERROR;
            }
        }

        for (j = 0; j < nestedCount; j++) {
            dataOffset = (BYTE *)data + rowData[rowIndex].offset + (j * rowData[rowIndex].bytes);

            switch (rowData[rowIndex].type) {
                case TYPE_BIN: {
                    BYTE *value;
                    int length;

                    value = Tcl_GetByteArrayFromObj(nestedObjs[j], &length);

                    // Do not copy more data than the row's size.
                    length = MIN(length, rowData[rowIndex].bytes);
                    CopyMemory(dataOffset, value, length);

                    // Zero the remaining bytes.
                    if (length < rowData[rowIndex].bytes) {
                        ZeroMemory((BYTE *)dataOffset + length, rowData[rowIndex].bytes - length);
                    }
                    break;
                }
                case TYPE_I32:
                case TYPE_I32LIST: {
                    int value;

                    if (Tcl_GetIntFromObj(interp, nestedObjs[j], &value) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    ((int *)dataOffset)[0] = value;
                    break;
                }
                case TYPE_I64: {
                    INT64 value;

                    if (Tcl_GetWideIntFromObj(interp, nestedObjs[j], (Tcl_WideInt *)&value) != TCL_OK) {
                        return TCL_ERROR;
                    }
                    ((INT64 *)dataOffset)[0] = value;
                    break;
                }
                case TYPE_STR:
                case TYPE_STRLIST: {
                    char *value = Tcl_GetString(nestedObjs[j]);
                    StringCchCopyA((char *)dataOffset, rowData[rowIndex].bytes, value);
                    break;
                }
                default: {
                    // Unknown data type.
                    assert(0);
                }
            }
        }

        if (j < rowData[rowIndex].values) {
            dataOffset = (BYTE *)data + rowData[rowIndex].offset + (j * rowData[rowIndex].bytes);

            // Terminate the end of a integer/string list.
            switch (rowData[rowIndex].type) {
                case TYPE_I32LIST: {
                    ((int *)dataOffset)[0] = -1;
                    break;
                }
                case TYPE_STRLIST: {
                    ((char *)dataOffset)[0] = '\0';
                    break;
                }
            }
        }
    }

    return TCL_OK;
}


/*++

GetOnlineFields

    Retrieve a list of online users and the requested fields.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    interp      - Current interpreter.

    fields      - Array of fields to retrieve.

    fieldCount  - Number of fields given for the "fields" parameter.

    flags       - Bit mask of ONLINE_GET_GROUPID and/or ONLINE_GET_USERNAME.

Return Value:
    A standard Tcl result.

Remarks:
    If the function succeeds, the online list is left in the interpreter's
    result. If the function fails, an error message is left instead.

--*/
static int
GetOnlineFields(
    ShmSession *session,
    Tcl_Interp *interp,
    unsigned char *fields,
    int fieldCount,
    unsigned short flags
    )
{
    DWORD result;
    int groupId;
    int i;
    DC_ONLINEDATA *dcOnlineData;
    ShmMemory *memOnline;
    ShmMemory *memUser = NULL;
    Tcl_Obj *fieldObj;
    Tcl_Obj *resultObj;
    Tcl_Obj *userObj;

    memOnline = ShmAlloc(interp, session, sizeof(DC_ONLINEDATA) + ((MAX_PATH + 1) * 2));
    if (memOnline == NULL) {
        return TCL_ERROR;
    }

    if (flags & ONLINE_GET_GROUPID || flags & ONLINE_GET_USERNAME) {
        // Allocate a buffer large enough to hold a DC_NAMEID or USERFILE structure.
        memUser = ShmAlloc(interp, session, MAX(sizeof(DC_NAMEID), sizeof(USERFILE)));

        if (memUser == NULL) {
            ShmFree(session, memOnline);
            return TCL_ERROR;
        }
    }

    // Initialise the data-copy online structure.
    dcOnlineData = (DC_ONLINEDATA *)memOnline->block;
    dcOnlineData->iOffset = 0;
    dcOnlineData->dwSharedMemorySize = memOnline->bytes;

    //
    // Create a nested list of users and requested fields.
    // Ex: {fieldOne fieldTwo ...} {fieldOne fieldTwo ...} {fieldOne fieldTwo ...}
    //
    resultObj = Tcl_GetObjResult(interp);

    while (dcOnlineData->iOffset >= 0) {
        result = ShmQuery(session, memOnline, DC_GET_ONLINEDATA, 5000);
        DebugPrint("GetOnlineFields: offset=%d result=%lu size=%lu\n",
            dcOnlineData->iOffset, result, dcOnlineData->dwSharedMemorySize);

        if (result != 0) {
            if (result == (DWORD)-1) {
                // An error occured.
                break;
            }

            // Skip user.
            dcOnlineData->iOffset++;
            continue;
        }
        userObj = Tcl_NewObj();

        if (flags & ONLINE_GET_GROUPID) {
            USERFILE userFile;

            // Retrieve the group ID from the user-file.
            UserGetFile(session, memUser, dcOnlineData->OnlineData.Uid, &userFile);
            groupId = userFile.Gid;
        }

        for (i = 0; i < fieldCount; i++) {
            fieldObj = NULL;

            switch ((int)fields[i]) {
                case WHO_ACTION: {
                    fieldObj = Tcl_NewStringObj(dcOnlineData->OnlineData.tszAction, -1);
                    break;
                }
                case WHO_CID: {
                    // The connection ID is one lower than the offset.
                    fieldObj = Tcl_NewIntObj(dcOnlineData->iOffset-1);
                    break;
                }
                case WHO_GID: {
                    fieldObj = Tcl_NewIntObj(groupId);
                    break;
                }
                case WHO_GROUP: {
                    char groupName[_MAX_NAME+1];
                    GroupIdToName(session, memUser, groupId, groupName);
                    fieldObj = Tcl_NewStringObj(groupName, -1);
                    break;
                }
                case WHO_HOST: {
                    fieldObj = Tcl_NewStringObj(dcOnlineData->OnlineData.szHostName, -1);
                    break;
                }
                case WHO_IDENT: {
                    fieldObj = Tcl_NewStringObj(dcOnlineData->OnlineData.szIdent, -1);
                    break;
                }
                case WHO_IDLETIME: {
                    fieldObj = Tcl_NewLongObj((long)(GetTickCount() -
                        dcOnlineData->OnlineData.dwIdleTickCount) / 1000);
                    break;
                }
                case WHO_IP: {
                    char clientIp[16];
                    BYTE *data = (BYTE *)&dcOnlineData->OnlineData.ulClientIp;

                    StringCchPrintfA(clientIp, ARRAYSIZE(clientIp), "%d.%d.%d.%d",
                        data[0] & 0xFF, data[1] & 0xFF, data[2] & 0xFF, data[3] & 0xFF);

                    fieldObj = Tcl_NewStringObj(clientIp, -1);
                    break;
                }
                case WHO_LOGINTIME: {
                    fieldObj = Tcl_NewLongObj((long)dcOnlineData->OnlineData.tLoginTime);
                    break;
                }
                case WHO_PORT: {
                    fieldObj = Tcl_NewLongObj((long)dcOnlineData->OnlineData.usClientPort);
                    break;
                }
                case WHO_REALDATAPATH: {
                    if (dcOnlineData->OnlineData.tszRealDataPath == NULL) {
                        fieldObj = Tcl_NewStringObj("", 0);
                    } else {
                        fieldObj = Tcl_NewStringObj(dcOnlineData->OnlineData.tszRealDataPath, -1);
                    }
                    break;
                }
                case WHO_REALPATH: {
                    if (dcOnlineData->OnlineData.tszRealPath == NULL) {
                        fieldObj = Tcl_NewStringObj("", 0);
                    } else {
                        fieldObj = Tcl_NewStringObj(dcOnlineData->OnlineData.tszRealPath, -1);
                    }
                    break;
                }
                case WHO_SERVICE: {
                    fieldObj = Tcl_NewStringObj(dcOnlineData->OnlineData.tszServiceName, -1);
                    break;
                }
                case WHO_SIZE: {
                    fieldObj = Tcl_NewWideIntObj((Tcl_WideInt)
                        dcOnlineData->OnlineData.i64TotalBytesTransfered);
                    break;
                }
                case WHO_SPEED: {
                    double speed = 0.0;
                    if (dcOnlineData->OnlineData.dwBytesTransfered > 0 &&
                            dcOnlineData->OnlineData.dwIntervalLength > 0) {
                        // Kilobytes per second.
                        speed = (double)dcOnlineData->OnlineData.dwBytesTransfered /
                            (double)dcOnlineData->OnlineData.dwIntervalLength;
                    }

                    fieldObj = Tcl_NewDoubleObj(speed);
                    break;
                }
                case WHO_STATUS: {
                    //
                    // 0 - Idle
                    // 1 - Download
                    // 2 - Upload
                    // 3 - List
                    //
                    fieldObj = Tcl_NewLongObj((long)dcOnlineData->OnlineData.bTransferStatus);
                    break;
                }
                case WHO_UID: {
                    fieldObj = Tcl_NewIntObj(dcOnlineData->OnlineData.Uid);
                    break;
                }
                case WHO_USER: {
                    char userName[_MAX_NAME+1];
                    UserIdToName(session, memUser, dcOnlineData->OnlineData.Uid, userName);
                    fieldObj = Tcl_NewStringObj(userName, -1);
                    break;
                }
                case WHO_VDATAPATH: {
                    fieldObj = Tcl_NewStringObj(dcOnlineData->OnlineData.tszVirtualDataPath, -1);
                    break;
                }
                case WHO_VPATH: {
                    fieldObj = Tcl_NewStringObj(dcOnlineData->OnlineData.tszVirtualPath, -1);
                    break;
                }
            }

            assert(fieldObj != NULL);
            Tcl_ListObjAppendElement(NULL, userObj, fieldObj);
        }

        Tcl_ListObjAppendElement(NULL, resultObj, userObj);
    }

    ShmFree(session, memOnline);
    if (memUser != NULL) {
        ShmFree(session, memUser);
    }
    return TCL_OK;
}


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
    int result;
    ShmMemory *memory;
    ShmSession session;
    Tcl_Obj *resultObj;
    static const char *options[] = {
        "create", "delete", "exists", "get",
        "rename", "set", "toid", "toname", NULL
    };
    enum optionIndices {
        GROUP_CREATE = 0, GROUP_DELETE, GROUP_EXISTS, GROUP_GET,
        GROUP_RENAME, GROUP_SET, GROUP_TO_ID, GROUP_TO_NAME
    };

    // Check arguments.
    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "option msgWindow ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK ||
            ShmInit(interp, objv[3], &session) != TCL_OK) {
        return TCL_ERROR;
    }

    resultObj = Tcl_GetObjResult(interp);
    switch ((enum optionIndices) index) {
        case GROUP_CREATE: {
            char *groupName;
            int groupId;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group");
                return TCL_ERROR;
            }

            memory = ShmAlloc(interp, &session, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            groupName = Tcl_GetString(objv[4]);

            result = GroupCreate(&session, memory, groupName, &groupId);
            if (result == TCL_OK) {
                Tcl_SetIntObj(resultObj, groupId);
            } else {
                Tcl_AppendResult(interp, "unable to create group \"",
                    groupName, "\"", NULL);
            }

            ShmFree(&session, memory);
            return result;
        }
        case GROUP_RENAME: {
            char *groupName;
            char *newName;

            if (objc != 6) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group newGroup");
                return TCL_ERROR;
            }

            memory = ShmAlloc(interp, &session, sizeof(DC_RENAME));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            groupName = Tcl_GetString(objv[4]);
            newName = Tcl_GetString(objv[5]);

            result = GroupRename(&session, memory, groupName, newName);
            if (result != TCL_OK) {
                Tcl_AppendResult(interp, "unable to rename group \"",
                    groupName, "\" to \"", newName, "\"", NULL);
            }

            ShmFree(&session, memory);
            return result;
        }
        case GROUP_DELETE: {
            char *groupName;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group");
                return TCL_ERROR;
            }

            memory = ShmAlloc(interp, &session, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            groupName = Tcl_GetString(objv[4]);

            result = GroupDelete(&session, memory, groupName);
            if (result != TCL_OK) {
                Tcl_AppendResult(interp, "unable to delete group \"",
                    groupName, "\"", NULL);
            }

            ShmFree(&session, memory);
            return result;
        }
        case GROUP_GET:
        case GROUP_SET: {
            char *groupName;
            int groupId;
            GROUPFILE groupFile;

            if (index == GROUP_GET && objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group");
                return TCL_ERROR;
            } else if (index == GROUP_SET && objc != 6) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group data");
                return TCL_ERROR;
            }

            memory = ShmAlloc(interp, &session, MAX(sizeof(DC_NAMEID), sizeof(USERFILE)));
            if (memory == NULL) {
                return TCL_ERROR;
            }

            // Group name to group ID.
            groupName = Tcl_GetString(objv[4]);
            if (GroupNameToId(&session, memory, groupName, &groupId) != TCL_OK) {
                ShmFree(&session, memory);

                Tcl_AppendResult(interp, "invalid group \"", groupName, "\"", NULL);
                return TCL_ERROR;
            }

            // Retrieve the group-file.
            if (GroupGetFile(&session, memory, groupId, &groupFile) != TCL_OK) {
                ShmFree(&session, memory);

                Tcl_AppendResult(interp, "unable to retrieve group file for \"",
                    groupName, "\"", NULL);
                return TCL_ERROR;
            }

            if (index == GROUP_GET) {
                result = RowDataGet(groupRowDef, &groupFile, resultObj);
            } else {
                result = RowDataSet(interp, objv[5], groupRowDef, &groupFile);
                if (result == TCL_OK) {
                    // Update the group-file.
                    if (GroupSetFile(&session, memory, &groupFile) != TCL_OK) {
                        Tcl_AppendResult(interp, "unable to update group file for \"",
                            groupName, "\"", NULL);
                        result = TCL_ERROR;
                    }
                }
            }

            ShmFree(&session, memory);
            return result;
        }
        case GROUP_EXISTS:
        case GROUP_TO_ID: {
            int groupId;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group");
                return TCL_ERROR;
            }

            // Group name to group ID.
            memory = ShmAlloc(interp, &session, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            result = GroupNameToId(&session, memory, Tcl_GetString(objv[4]), &groupId);
            ShmFree(&session, memory);

            if (index == GROUP_EXISTS) {
                Tcl_SetBooleanObj(resultObj, result == TCL_OK);
            } else {
                Tcl_SetIntObj(resultObj, groupId);
            }
            return TCL_OK;
        }
        case GROUP_TO_NAME: {
            int groupId;
            char groupName[_MAX_NAME+1];

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow gid");
                return TCL_ERROR;
            }

            if (Tcl_GetIntFromObj(interp, objv[4], &groupId) != TCL_OK) {
                return TCL_ERROR;
            }

            // Group ID to group name.
            memory = ShmAlloc(interp, &session, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            GroupIdToName(&session, memory, groupId, groupName);
            ShmFree(&session, memory);

            Tcl_SetStringObj(resultObj, groupName, -1);
            return TCL_OK;
        }
    }

    // This point is never reached.
    assert(0);
    return TCL_ERROR;
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
    char   processPath[MAX_PATH];
    DWORD  processId;
    HANDLE processHandle;
    long   processTime;
    DWORD  needed;
    HMODULE module;
    FILETIME creationTime;
    FILETIME dummy;
    ShmSession session;
    Tcl_Obj *fieldObj;
    Tcl_Obj *valueObj;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow varName");
        return TCL_ERROR;
    }

    if (ShmInit(interp, objv[2], &session) != TCL_OK) {
        return TCL_ERROR;
    }

    // Retrieve the message windows's process ID and attempt to open it.
    GetWindowThreadProcessId(session.messageWnd, &processId);
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, processId);
    if (processHandle == NULL) {
        Tcl_AppendResult(interp, "unable to open process: ",
            TclSetWinError(interp, GetLastError()), NULL);
        return TCL_ERROR;
    }

    // The exit time, kernel time, and user time parameters cannot be NULL.
    if (GetProcessTimes(processHandle, &creationTime, &dummy, &dummy, &dummy)) {
        processTime = FileTimeToEpoch(&creationTime);
    } else {
        processTime = 0;
    }

    // Find the process image path.
    processPath[0] = '\0';
    if (EnumProcessModules(processHandle, &module, sizeof(module), &needed)) {
        GetModuleFileNameEx(processHandle, module, processPath, MAX_PATH);
    }

    CloseHandle(processHandle);

    //
    // Tcl_ObjSetVar2() won't create a copy of the field object,
    // so the caller must free the object once finished with it.
    //
    fieldObj = Tcl_NewObj();
    Tcl_IncrRefCount(fieldObj);

// Easier than repeating this...
#define TCL_STORE_ARRAY(name, value)                                                      \
    valueObj = (value);                                                                   \
    Tcl_SetStringObj(fieldObj, (name), -1);                                               \
    if (Tcl_ObjSetVar2(interp, objv[3], fieldObj, valueObj, TCL_LEAVE_ERR_MSG) == NULL) { \
        Tcl_DecrRefCount(fieldObj);                                                       \
        return TCL_ERROR;                                                                 \
    }

    TCL_STORE_ARRAY("path", Tcl_NewStringObj(processPath, -1));
    TCL_STORE_ARRAY("pid",  Tcl_NewLongObj((long)processId));
    TCL_STORE_ARRAY("time", Tcl_NewLongObj((long)processTime));

    Tcl_DecrRefCount(fieldObj);
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
    int userId;
    ShmSession session;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow uid");
        return TCL_ERROR;
    }

    if (ShmInit(interp, objv[2], &session) != TCL_OK ||
            Tcl_GetIntFromObj(interp, objv[3], &userId) != TCL_OK) {
        return TCL_ERROR;
    }

    PostMessage(session.messageWnd, WM_KICK, (WPARAM)userId, (LPARAM)userId);
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
    int connId;
    ShmSession session;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow cid");
        return TCL_ERROR;
    }

    if (ShmInit(interp, objv[2], &session) != TCL_OK ||
            Tcl_GetIntFromObj(interp, objv[3], &connId) != TCL_OK) {
        return TCL_ERROR;
    }

    PostMessage(session.messageWnd, WM_KILL, (WPARAM)connId, (LPARAM)connId);
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
    int result;
    ShmMemory *memory;
    ShmSession session;
    Tcl_Obj *resultObj;
    static const char *options[] = {
        "create", "delete", "exists", "get",
        "rename", "set", "toid", "toname", NULL
    };
    enum optionIndices {
        USER_CREATE = 0, USER_DELETE, USER_EXISTS, USER_GET,
        USER_RENAME, USER_SET, USER_TO_ID, USER_TO_NAME
    };

    // Check arguments.
    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "option msgWindow ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK ||
            ShmInit(interp, objv[3], &session) != TCL_OK) {
        return TCL_ERROR;
    }

    resultObj = Tcl_GetObjResult(interp);
    switch ((enum optionIndices) index) {
        case USER_CREATE: {
            char *userName;
            int userId;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user");
                return TCL_ERROR;
            }

            memory = ShmAlloc(interp, &session, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            userName = Tcl_GetString(objv[4]);

            result = UserCreate(&session, memory, userName, &userId);
            if (result == TCL_OK) {
                Tcl_SetIntObj(resultObj, userId);
            } else {
                Tcl_AppendResult(interp, "unable to create user \"",
                    userName, "\"", NULL);
            }

            ShmFree(&session, memory);
            return result;
        }
        case USER_RENAME: {
            char *userName;
            char *newName;

            if (objc != 6) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user newUser");
                return TCL_ERROR;
            }

            memory = ShmAlloc(interp, &session, sizeof(DC_RENAME));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            userName = Tcl_GetString(objv[4]);
            newName = Tcl_GetString(objv[5]);

            result = UserRename(&session, memory, userName, newName);
            if (result != TCL_OK) {
                Tcl_AppendResult(interp, "unable to rename user \"",
                    userName, "\" to \"", newName, "\"", NULL);
            }

            ShmFree(&session, memory);
            return result;
        }
        case USER_DELETE: {
            char *userName;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user");
                return TCL_ERROR;
            }

            memory = ShmAlloc(interp, &session, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            userName = Tcl_GetString(objv[4]);

            result = UserDelete(&session, memory, userName);
            if (result != TCL_OK) {
                Tcl_AppendResult(interp, "unable to delete user \"",
                    userName, "\"", NULL);
            }

            ShmFree(&session, memory);
            return result;
        }
        case USER_GET:
        case USER_SET: {
            char *userName;
            int userId;
            USERFILE userFile;

            if (index == USER_GET && objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user");
                return TCL_ERROR;
            } else if (index == USER_SET && objc != 6) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user data");
                return TCL_ERROR;
            }

            memory = ShmAlloc(interp, &session, MAX(sizeof(DC_NAMEID), sizeof(USERFILE)));
            if (memory == NULL) {
                return TCL_ERROR;
            }

            // User name to user ID.
            userName = Tcl_GetString(objv[4]);
            if (UserNameToId(&session, memory, userName, &userId) != TCL_OK) {
                ShmFree(&session, memory);

                Tcl_AppendResult(interp, "invalid user \"", userName, "\"", NULL);
                return TCL_ERROR;
            }

            // Retrieve the user-file.
            if (UserGetFile(&session, memory, userId, &userFile) != TCL_OK) {
                ShmFree(&session, memory);

                Tcl_AppendResult(interp, "unable to retrieve user file for \"",
                    userName, "\"", NULL);
                return TCL_ERROR;
            }

            if (index == USER_GET) {
                result = RowDataGet(userRowDef, &userFile, resultObj);
            } else {
                result = RowDataSet(interp, objv[5], userRowDef, &userFile);
                if (result == TCL_OK) {
                    // Update the user-file.
                    if (UserSetFile(&session, memory, &userFile) != TCL_OK) {
                        Tcl_AppendResult(interp, "unable to update user file for \"",
                            userName, "\"", NULL);
                        result = TCL_ERROR;
                    }
                }
            }

            ShmFree(&session, memory);
            return result;
        }
        case USER_EXISTS:
        case USER_TO_ID: {
            int userId;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user");
                return TCL_ERROR;
            }

            // User name to user ID.
            memory = ShmAlloc(interp, &session, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            result = UserNameToId(&session, memory, Tcl_GetString(objv[4]), &userId);
            ShmFree(&session, memory);

            if (index == USER_EXISTS) {
                Tcl_SetBooleanObj(resultObj, result == TCL_OK);
            } else {
                Tcl_SetIntObj(resultObj, userId);
            }
            return TCL_OK;
        }
        case USER_TO_NAME: {
            int userId;
            char userName[_MAX_NAME+1];

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow uid");
                return TCL_ERROR;
            }

            if (Tcl_GetIntFromObj(interp, objv[4], &userId) != TCL_OK) {
                return TCL_ERROR;
            }

            // User ID to user name.
            memory = ShmAlloc(interp, &session, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            UserIdToName(&session, memory, userId, userName);
            ShmFree(&session, memory);

            Tcl_SetStringObj(resultObj, userName, -1);
            return TCL_OK;
        }
    }

    // This point is never reached.
    assert(0);
    return TCL_ERROR;
}

/*++

IoVfsCmd

    Manipulate ioFTPD's directory cache and file/directory permissions.

Arguments:
    interp - Current interpreter.

    objc   - Number of arguments.

    objv   - Argument objects.

Return Value:
    A standard Tcl result.

--*/
static int
IoVfsCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    char *path;
    int index;
    int pathLength;
    int result = TCL_ERROR;
    ShmMemory *memory = NULL;
    ShmSession session;
    Tcl_DString buffer;
    static const char *options[] = {
        "attributes", "flush", NULL
    };
    enum optionIndices {
        VFS_ATTRIBS = 0, VFS_FLUSH
    };

    // Check arguments.
    if (objc < 5) {
        Tcl_WrongNumArgs(interp, 2, objv, "option msgWindow path ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK ||
            ShmInit(interp, objv[3], &session) != TCL_OK) {
        return TCL_ERROR;
    }

    // Translate the provided path.
    if (TranslatePathFromObj(interp, objv[4], &buffer) != TCL_OK) {
        return TCL_ERROR;
    }
    path = Tcl_DStringValue(&buffer);
    pathLength = Tcl_DStringLength(&buffer);

    switch ((enum optionIndices) index) {
        case VFS_ATTRIBS: {
            if (objc != 6 && !(objc & 1)) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow path ?switch? ?value? ?switch value?...");
            } else {
                memory = ShmAlloc(interp, &session, sizeof(DC_VFS) + pathLength + 1);
                if (memory != NULL) {
                    result = IoVfsAttrsCmd(interp, objc, objv, &session, memory, &buffer);
                }
            }
            break;
        }
        case VFS_FLUSH: {
            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow path");
            } else {
                memory = ShmAlloc(interp, &session, pathLength + sizeof(char));
                if (memory != NULL) {
                    result = VfsFlush(&session, memory, path);

                    if (result != TCL_OK) {
                        Tcl_AppendResult(interp, "unable to flush path \"",
                            Tcl_GetString(objv[4]), "\"", NULL);
                    }
                }
            }
            break;
        }
        default: {
            // This point is never reached.
            assert(0);
            return TCL_ERROR;
        }
    }

    Tcl_DStringFree(&buffer);
    if (memory != NULL) {
        ShmFree(&session, memory);
    }
    return result;
}

/*++

IoVfsAttrsCmd

    Manipulate ioFTPD's file and directory permissions.

Arguments:
    interp  - Current interpreter.

    objc    - Number of arguments.

    objv    - Argument objects.

    session - Pointer to an initialised ShmSession structure.

    memory  - Pointer to an allocated ShmMemory structure.

    path    - Path to manipulate.

Return Value:
    A standard Tcl result.

--*/
static int
IoVfsAttrsCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[],
    ShmSession *session,
    ShmMemory *memory,
    Tcl_DString *path
    )
{
    int i;
    int index;
    int result;
    VfsPerm vfsPerm;
    static const char *switches[] = {
        "-chmod", "-gid", "-uid", NULL
    };
    enum switchIndices {
        SWITCH_CHMOD = 0, SWITCH_GID, SWITCH_UID
    };

    // Read VFS permissions beforehand.
    result = VfsRead(session, memory, Tcl_DStringValue(path),
            Tcl_DStringLength(path), &vfsPerm);
    if (result != TCL_OK) {
        Tcl_AppendResult(interp, "unable to read permissions from \"",
            Tcl_GetString(objv[4]), "\"", NULL);
        return TCL_ERROR;
    }

    //
    // Retrieve all values.
    // Cmd: ioftpd vfs attributes <msgWindow> <path>
    //
    if (objc == 5) {
        Tcl_Obj *listObj = Tcl_NewObj();
        Tcl_ListObjAppendElement(NULL, listObj, Tcl_NewLongObj((long)vfsPerm.userId));
        Tcl_ListObjAppendElement(NULL, listObj, Tcl_NewLongObj((long)vfsPerm.groupId));
        Tcl_ListObjAppendElement(NULL, listObj, TclNewOctalObj(vfsPerm.fileMode));
        Tcl_SetObjResult(interp, listObj);
        return TCL_OK;
    }

    //
    // Retrieve the value of an option.
    // Cmd: ioftpd vfs attributes <msgWindow> <path> -switch
    //
    if (objc == 6) {
        Tcl_Obj *resultObj;

        if (Tcl_GetIndexFromObj(interp, objv[5], switches, "switch", 0, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        resultObj = Tcl_GetObjResult(interp);

        switch ((enum optionIndices) index) {
            case SWITCH_CHMOD: {
                TclSetOctalObj(resultObj, vfsPerm.fileMode);
                break;
            }
            case SWITCH_GID: {
                Tcl_SetLongObj(resultObj, (long)vfsPerm.groupId);
                break;
            }
            case SWITCH_UID: {
                Tcl_SetLongObj(resultObj, (long)vfsPerm.userId);
                break;
            }
        }
        return TCL_OK;
    }

    //
    // Change one or more options.
    // Cmd: ioftpd vfs attributes <msgWindow> <path> -switch value
    // Cmd: ioftpd vfs attributes <msgWindow> <path> -switch value -switch value
    //
    for (i = 5; i < objc; i++) {
        if (Tcl_GetIndexFromObj(interp, objv[i], switches, "switch", 0, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        i++;

        switch ((enum optionIndices) index) {
            case SWITCH_CHMOD: {
                result = TclGetOctalFromObj(interp, objv[i], &vfsPerm.fileMode);
                break;
            }
            case SWITCH_GID: {
                result = Tcl_GetLongFromObj(interp, objv[i], (long *)&vfsPerm.groupId);
                break;
            }
            case SWITCH_UID: {
                result = Tcl_GetLongFromObj(interp, objv[i], (long *)&vfsPerm.userId);
                break;
            }
        }
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }

    result = VfsWrite(session, memory, Tcl_DStringValue(path),
        Tcl_DStringLength(path), &vfsPerm);
    if (result != TCL_OK) {
        Tcl_AppendResult(interp, "unable to write permissions to \"",
            Tcl_GetString(objv[4]), "\"", NULL);
    }
    return result;
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
    unsigned short flags = 0;
    ShmSession session;
    Tcl_Obj **elementObjs;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow fields");
        return TCL_ERROR;
    }

    if (ShmInit(interp, objv[2], &session) != TCL_OK ||
            Tcl_ListObjGetElements(interp, objv[3], &elementCount, &elementObjs) != TCL_OK) {
        return TCL_ERROR;
    }

    // Create an array of indices from "whoFields".
    fields = (unsigned char *)ckalloc(elementCount * sizeof(unsigned char));

    for (i = 0; i < elementCount; i++) {
        if (Tcl_GetIndexFromObj(interp, elementObjs[i], whoFields,
                "field", 0, &fieldIndex) != TCL_OK) {
            goto end;
        }

        if (fieldIndex == WHO_GID || fieldIndex == WHO_GROUP) {
            flags |= ONLINE_GET_GROUPID;
        } else if (fieldIndex == WHO_USER) {
            flags |= ONLINE_GET_USERNAME;
        }

        assert(fieldIndex < 255);
        fields[i] = (unsigned char)fieldIndex;
    }

    result = GetOnlineFields(&session, interp, fields, elementCount, flags);

end:
    ckfree((char *)fields);
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
        "group", "info", "kick", "kill", "user", "vfs", "who", NULL
    };
    enum optionIndices {
        OPTION_GROUP = 0, OPTION_INFO, OPTION_KICK,
        OPTION_KILL, OPTION_USER, OPTION_VFS, OPTION_WHO
    };

    // Validate "whoFields" indices.
    assert(!strcmp("action",       whoFields[WHO_ACTION]));
    assert(!strcmp("cid",          whoFields[WHO_CID]));
    assert(!strcmp("gid",          whoFields[WHO_GID]));
    assert(!strcmp("group",        whoFields[WHO_GROUP]));
    assert(!strcmp("host",         whoFields[WHO_HOST]));
    assert(!strcmp("ident",        whoFields[WHO_IDENT]));
    assert(!strcmp("idletime",     whoFields[WHO_IDLETIME]));
    assert(!strcmp("ip",           whoFields[WHO_IP]));
    assert(!strcmp("logintime",    whoFields[WHO_LOGINTIME]));
    assert(!strcmp("port",         whoFields[WHO_PORT]));
    assert(!strcmp("realdatapath", whoFields[WHO_REALDATAPATH]));
    assert(!strcmp("realpath",     whoFields[WHO_REALPATH]));
    assert(!strcmp("service",      whoFields[WHO_SERVICE]));
    assert(!strcmp("size",         whoFields[WHO_SIZE]));
    assert(!strcmp("speed",        whoFields[WHO_SPEED]));
    assert(!strcmp("status",       whoFields[WHO_STATUS]));
    assert(!strcmp("uid",          whoFields[WHO_UID]));
    assert(!strcmp("user",         whoFields[WHO_USER]));
    assert(!strcmp("vdatapath",    whoFields[WHO_VDATAPATH]));
    assert(!strcmp("vpath",        whoFields[WHO_VPATH]));

    // Check arguments.
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0, &index) != TCL_OK) {
        return TCL_ERROR;
    }

    switch ((enum optionIndices) index) {
        case OPTION_GROUP: return IoGroupCmd(interp, objc, objv);
        case OPTION_INFO:  return IoInfoCmd(interp, objc, objv);
        case OPTION_KICK:  return IoKickCmd(interp, objc, objv);
        case OPTION_KILL:  return IoKillCmd(interp, objc, objv);
        case OPTION_USER:  return IoUserCmd(interp, objc, objv);
        case OPTION_VFS:   return IoVfsCmd(interp, objc, objv);
        case OPTION_WHO:   return IoWhoCmd(interp, objc, objv);
    }

    // This point is never reached.
    assert(0);
    return TCL_ERROR;
}
