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
    ioftpd user exists  <msgWindow> <user>        - Check if a user exists.
    ioftpd user get     <msgWindow> <user>        - Query a user.
    ioftpd user id      <msgWindow> <user>        - Resolve a user name to its UID.
    ioftpd user list    <msgWindow> [-id] [-name] - List users and/or UIDs.
    ioftpd user name    <msgWindow> <uid>         - Resolve a UID to its user name.

    Group Commands:
    ioftpd group exists <msgWindow> <group>       - Check if a group exists.
    ioftpd group get    <msgWindow> <group>       - Query a group.
    ioftpd group id     <msgWindow> <group>       - Resolve a group name to its GID.
    ioftpd group list   <msgWindow> [-id] [-name] - List groups and/or GIDs.
    ioftpd group name   <msgWindow> <gid>         - Resolve a GID to its group name.

    Online Data Commands:
    ioftpd info         <msgWindow> <varName>     - Query the ioFTPD message window.
    ioftpd kick         <msgWindow> <user>        - Kick a user.
    ioftpd kill         <msgWindow> <cid>         - Kick a connection ID.
    ioftpd who          <msgWindow> <fields>      - Query online users.

--*/

#include <alcoExt.h>

// Relevant ioFTPD headers.
#include "ioftpd\ServerLimits.h"
#include "ioftpd\UserFile.h"
#include "ioftpd\GroupFile.h"
#include "ioftpd\WinMessages.h"
#include "ioftpd\DataCopy.h"

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
IoWhoCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    );

//
// Shared memory structures and functions.
//

typedef struct {
    HWND  messageWnd;       // Handle to ioFTPD's message window.
    DWORD processId;        // Current process ID.
} ShmSession;

typedef struct {
    DC_MESSAGE *message;    // Data-copy message structure.
    void       *block;      // Allocated memory block.
    void       *remote;     // ???
    HANDLE     event;       // Event handle.
    HANDLE     memMap;      // Memory mapping handle.
    DWORD      bytes;       // Size of the memory block, in bytes.
} ShmMemory;

static int
ShmInit(
    ShmSession *session,
    Tcl_Interp *interp,
    const char *windowName
    );

static ShmMemory *
ShmAlloc(
    ShmSession *session,
    BOOL createEvent,
    DWORD bytes
    );

static void
ShmFree(
    ShmSession *session,
    ShmMemory *memory
    );

static DWORD
ShmQuery(
    ShmSession *session,
    ShmMemory *memory,
    DWORD queryType,
    DWORD timeOut
    );

//
// Helper functions.
//

static int
GroupIdToName(
    ShmSession *session,
    ShmMemory *memory,
    int groupId,
    char *groupName
    );

static int
GroupNameToId(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName,
    int *groupId
    );

static int
UserIdToName(
    ShmSession *session,
    ShmMemory *memory,
    int userId,
    char *userName
    );

static int
UserNameToId(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName,
    int *userId
    );

// Flags for GetOnlineFields
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

//
// Who fields.
//

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
    WHO_SIZE,
    WHO_SPEED,
    WHO_STATUS,
    WHO_UID,
    WHO_USER,
    WHO_VDATAPATH,
    WHO_VPATH
};


/*++

ShmInit

    Initialise a shared memory session.

Arguments:
    session     - Pointer to the ShmSession structure to be initialised.

    interp      - Interpreter to use for error reporting.

    windowName  - Name of ioFTPD's message window.

Return Value:
    A standard Tcl result.

--*/
static int
ShmInit(
    ShmSession *session,
    Tcl_Interp *interp,
    const char *windowName
    )
{
    assert(session    != NULL);
    assert(interp     != NULL);
    assert(windowName != NULL);

    session->messageWnd = FindWindowA(windowName, NULL);
    session->processId  = GetCurrentProcessId();

    if (session->messageWnd == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to find window \"", windowName,
            "\": ", TclSetWinError(interp, GetLastError()), NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}

/*++

ShmAlloc

    Allocates shared memory.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    createEvent - Create an event to signal.

    bytes       - Number of bytes to be allocated.

Return Value:
    If the function succeeds, the return value is a pointer to a ShmMemory
    structure. This structure should be freed by the ShmFree function when
    it is no longer needed. If the function fails, the return value is NULL.
    To get extended error information, call GetLastError.

--*/
static ShmMemory *
ShmAlloc(
    ShmSession *session,
    BOOL createEvent,
    DWORD bytes
    )
{
    DC_MESSAGE *message = NULL;
    ShmMemory  *memory;
    DWORD  error  = ERROR_SUCCESS;
    HANDLE event  = NULL;
    HANDLE memMap = NULL;
    void   *remote;

    assert(session != NULL);
    assert(bytes > 0);

    if (createEvent && !(event = CreateEvent(NULL, FALSE, FALSE, NULL))) {
        return NULL;
    }

    memory = (ShmMemory *) ckalloc(sizeof(ShmMemory));
    bytes += sizeof(DC_MESSAGE);

    // Allocate memory in local process.
    memMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE|SEC_COMMIT, 0, bytes, NULL);

    if (memMap != NULL) {
        message = (DC_MESSAGE *) MapViewOfFile(memMap,
            FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, bytes);

        if (message != NULL) {
            // Initialise data-copy message structure.
            message->hEvent       = event;
            message->hObject      = NULL;
            message->dwIdentifier = 0;
            message->dwReturn     = 0;
            message->lpMemoryBase = (void *) message;
            message->lpContext    = &message[1];

            SetLastError(ERROR_SUCCESS);
            remote = (void *) SendMessage(session->messageWnd, WM_DATACOPY_FILEMAP,
                (WPARAM) session->processId, (LPARAM) memMap);

            error = GetLastError();
            if (remote == NULL && error == ERROR_SUCCESS) {
                // Not sure if SendMessage updates the error code on failure.
                error = ERROR_INVALID_PARAMETER;
            }
        } else {
            error = GetLastError();
        }
    } else {
        error = GetLastError();
    }

    if (error == ERROR_SUCCESS) {
        // Update memory allocation structure.
        memory->message = message;
        memory->block   = &message[1];
        memory->remote  = remote;
        memory->event   = event;
        memory->memMap  = memMap;
        memory->bytes   = bytes - sizeof(DC_MESSAGE);
    } else {
        // Free objects and resources.
        if (message != NULL) {
            UnmapViewOfFile(message);
        }
        if (memMap != NULL) {
            CloseHandle(memMap);
        }
        if (event != NULL) {
            CloseHandle(event);
        }

        ckfree((char *) memory);
        memory = NULL;

        // Restore the previous error code, since the UnmapViewOfFile
        // or CloseHandle function could change it.
        SetLastError(error);
    }

    return memory;
}

/*++

ShmFree

    Frees shared memory.

Arguments:
    session - Pointer to an initialised ShmSession structure.

    memory  - Pointer to an ShmMemory structure allocated by the
              ShmAlloc function.

Return Value:
    None.

--*/
static void
ShmFree(
    ShmSession *session,
    ShmMemory *memory
    )
{
    assert(session != NULL);
    assert(memory  != NULL);

    // Free objects and resources.
    UnmapViewOfFile(memory->message);

    if (memory->event != NULL) {
        CloseHandle(memory->event);
    }
    if (memory->memMap != NULL) {
        CloseHandle(memory->memMap);
    }

    PostMessage(session->messageWnd, WM_DATACOPY_FREE, 0, (LPARAM) memory->remote);
    ckfree((char *) memory);
}

/*++

ShmQuery

    Queries the ioFTPD daemon.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the
                  ShmAlloc function.

    queryType   - Query identifier, defined in DataCopy.h.

    timeOut     - Time-out interval, in milliseconds.

Return Value:
    If the function succeeds, the return value is zero. If the function
    fails, the return value is non-zero.

--*/
static DWORD
ShmQuery(
    ShmSession *session,
    ShmMemory *memory,
    DWORD queryType,
    DWORD timeOut
    )
{
    assert(session != NULL);
    assert(memory  != NULL);

    memory->message->dwIdentifier = queryType;
    PostMessage(session->messageWnd, WM_SHMEM, 0, (LPARAM) memory->remote);

    if (timeOut && memory->event != NULL) {
        if (WaitForSingleObject(memory->event, timeOut) == WAIT_TIMEOUT) {
            return (DWORD)-1;
        }
        return memory->message->dwReturn;
    }

    //  No timeout or event, return value cannot be checked.
    return (DWORD)-1;
}


/*++

GroupIdToName

    Resolve a group ID to its corresponding group name.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_NAMEID structure.

    groupId     - The group ID to resolve.

    groupName   - Pointer to a buffer to receive the user name. The
                  buffer must be able to hold _MAX_NAME+1 characters.

Return Value:
    A standard Tcl result.

--*/
static int
GroupIdToName(
    ShmSession *session,
    ShmMemory *memory,
    int groupId,
    char *groupName
    )
{
    DC_NAMEID *nameId;

    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);

    // Initialise the DC_NAMEID structure.
    nameId     = (DC_NAMEID *) memory->message->lpContext;
    nameId->Id = groupId;
    nameId->tszName[0] = '\0';

    if (!ShmQuery(session, memory, DC_GID_TO_GROUP, 5000)) {
        StringCchCopyA(nameId->tszName, _MAX_NAME+1, groupName);
        return TCL_OK;
    }

    groupName[0] = '\0';
    return TCL_ERROR;
}

/*++

GroupNameToId

    Resolve a group name to its corresponding group ID.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_NAMEID structure.

    groupName   - The group name to resolve.

    groupId     - Location to store the group ID.

Return Value:
    A standard Tcl result.

--*/
static int
GroupNameToId(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName,
    int *groupId
    )
{
    DC_NAMEID *nameId;

    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    assert(groupId   != NULL);

    // Initialise the DC_NAMEID structure.
    nameId     = (DC_NAMEID *) memory->message->lpContext;
    nameId->Id = -1;
    StringCchCopyA(nameId->tszName, ARRAYSIZE(nameId->tszName), groupName);

    if (!ShmQuery(session, memory, DC_GROUP_TO_GID, 5000)) {
        *groupId = nameId->Id;
        return TCL_OK;
    }

    *groupId = -1;
    return TCL_OK;
}

/*++

UserIdToName

    Resolve a user ID to its corresponding user name.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_NAMEID structure.

    userId      - The user ID to resolve.

    userName    - Pointer to a buffer to receive the user name. The
                  buffer must be able to hold _MAX_NAME+1 characters.

Return Value:
    A standard Tcl result.

--*/
static int
UserIdToName(
    ShmSession *session,
    ShmMemory *memory,
    int userId,
    char *userName
    )
{
    DC_NAMEID *nameId;

    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);

    // Initialise the DC_NAMEID structure.
    nameId     = (DC_NAMEID *) memory->message->lpContext;
    nameId->Id = userId;
    nameId->tszName[0] = '\0';

    if (!ShmQuery(session, memory, DC_UID_TO_USER, 5000)) {
        StringCchCopyA(nameId->tszName, _MAX_NAME+1, userName);
        return TCL_OK;
    }

    userName[0] = '\0';
    return TCL_OK;
}

/*++

UserNameToId

    Resolve a user name to its corresponding user ID.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_NAMEID structure.

    userName    - The user name to resolve.

    userId      - Location to store the user ID.

Return Value:
    A standard Tcl result.

--*/
static int
UserNameToId(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName,
    int *userId
    )
{
    DC_NAMEID *nameId;

    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    assert(userId   != NULL);

    // Initialise the DC_NAMEID structure.
    nameId     = (DC_NAMEID *) memory->message->lpContext;
    nameId->Id = -1;
    StringCchCopyA(nameId->tszName, ARRAYSIZE(nameId->tszName), userName);

    if (!ShmQuery(session, memory, DC_USER_TO_UID, 5000)) {
        *userId = nameId->Id;
        return TCL_OK;
    }

    *userId = -1;
    return TCL_OK;
}

/*++

GetOnlineFields

    Retrieve a list of online users and the requested fields.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    interp      - Current interpreter.

    fields      - Array of fields to retrieve.

    fieldCount  - Number of fields given for the 'fields' parameter.

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
    int i;
    DC_ONLINEDATA *dcOnlineData;
    ShmMemory *memOnline;
    ShmMemory *memUser = NULL;
    Tcl_Obj *fieldObj;
    Tcl_Obj *resultObj;
    Tcl_Obj *userObj;

    memOnline = ShmAlloc(session, TRUE, sizeof(DC_ONLINEDATA) + _MAX_PATH * 2);
    if (memOnline == NULL) {
        Tcl_AppendResult(interp, "unable to retrieve online data: ",
            TclSetWinError(interp, GetLastError()), NULL);
        return TCL_ERROR;
    }

    if (flags & ONLINE_GET_GROUPID || flags & ONLINE_GET_USERNAME) {
        // Allocate a buffer large enough to hold a DC_NAMEID or USERFILE structure.
        memUser = ShmAlloc(session, TRUE, MAX(sizeof(USERFILE), sizeof(DC_NAMEID)));

        if (memUser == NULL) {
            ShmFree(session, memOnline);
            Tcl_AppendResult(interp, "unable to retrieve online data: ",
                TclSetWinError(interp, GetLastError()), NULL);
            return TCL_ERROR;
        }
    }

    // Initialise the data-copy online structure.
    dcOnlineData          = (DC_ONLINEDATA *) memOnline->block;
    dcOnlineData->iOffset = 0;
    dcOnlineData->dwSharedMemorySize = memOnline->bytes;

    //
    // Create a nested list of users and requested fields.
    // Ex: {fieldOne fieldTwo ...} {fieldOne fieldTwo ...} {fieldOne fieldTwo ...}
    //
    resultObj = Tcl_GetObjResult(interp);

    for (;;) {
        result = ShmQuery(session, memOnline, DC_GET_ONLINEDATA, 5000);
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
            // TODO: Get the primary group ID.
        }

        for (i = 0; i < fieldCount; i++) {
            fieldObj = NULL;

            switch ((int) fields[i]) {
                case WHO_ACTION: {
                    fieldObj = Tcl_NewStringObj(dcOnlineData->OnlineData.tszAction, -1);
                    break;
                }
                case WHO_CID: {
                    // The connection ID is one lower than the offset.
                    fieldObj = Tcl_NewLongObj((long) dcOnlineData->iOffset-1);
                    break;
                }
                case WHO_GID: {
                    // TODO: User file crap.
                    fieldObj = Tcl_NewLongObj(-1);
                    break;
                }
                case WHO_GROUP: {
                    // TODO: User file crap.
                    fieldObj = Tcl_NewStringObj("TODO", -1);
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
                    fieldObj = Tcl_NewLongObj((long) (GetTickCount() -
                        dcOnlineData->OnlineData.dwIdleTickCount) / 1000);
                    break;
                }
                case WHO_IP: {
                    char clientIp[16];
                    BYTE *data = (BYTE *) &dcOnlineData->OnlineData.ulClientIp;

                    StringCchPrintfA(clientIp, ARRAYSIZE(clientIp), "%d.%d.%d.%d",
                        data[0] & 0xFF, data[1] & 0xFF, data[2] & 0xFF, data[3] & 0xFF);

                    fieldObj = Tcl_NewStringObj(clientIp, -1);
                    break;
                }
                case WHO_LOGINTIME: {
                    fieldObj = Tcl_NewLongObj((long) dcOnlineData->OnlineData.tLoginTime);
                    break;
                }
                case WHO_PORT: {
                    fieldObj = Tcl_NewLongObj((long) dcOnlineData->OnlineData.usClientPort);
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
                case WHO_SIZE: {
                    fieldObj = Tcl_NewWideIntObj((Tcl_WideInt)
                        dcOnlineData->OnlineData.i64TotalBytesTransfered);
                    break;
                }
                case WHO_SPEED: {
                    double speed = 0.0;
                    if (dcOnlineData->OnlineData.dwIntervalLength > 0) {
                        // Kilobytes per second.
                        speed = (double)dcOnlineData->OnlineData.dwBytesTransfered /
                            (double)dcOnlineData->OnlineData.dwIntervalLength;
                    }
                    fieldObj = Tcl_NewDoubleObj(speed);
                    break;
                }
                case WHO_STATUS: {
                    // 0 - Idle
                    // 1 - Upload
                    // 2 - Download
                    // 3 - List
                    fieldObj = Tcl_NewLongObj((long) dcOnlineData->OnlineData.bTransferStatus);
                    break;
                }
                case WHO_UID: {
                    fieldObj = Tcl_NewLongObj((long) dcOnlineData->OnlineData.Uid);
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

    if (ShmInit(&session, interp, Tcl_GetString(objv[2])) != TCL_OK) {
        return TCL_ERROR;
    }

    // Retrieve the message windows's process ID and attempt to open it.
    GetWindowThreadProcessId(session.messageWnd, &processId);
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, processId);
    if (processHandle == NULL) {
        Tcl_AppendResult(interp, "unable to open process: ",
            TclSetWinError(interp, GetLastError()));
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
        Tcl_DecrRefCount(valueObj);                                                       \
        return TCL_ERROR;                                                                 \
    }

    TCL_STORE_ARRAY("path", Tcl_NewStringObj(processPath, -1));
    TCL_STORE_ARRAY("pid",  Tcl_NewLongObj((long) processId));
    TCL_STORE_ARRAY("time", Tcl_NewLongObj((long) processTime));

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
    unsigned short flags = 0;
    ShmSession session;
    Tcl_Obj **elementPtrs;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "msgWindow fields");
        return TCL_ERROR;
    }

    if (ShmInit(&session, interp, Tcl_GetString(objv[2])) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_ListObjGetElements(interp, objv[3], &elementCount, &elementPtrs) != TCL_OK) {
        return TCL_ERROR;
    }

    fields = (unsigned char *) ckalloc(elementCount * sizeof(unsigned char));

    // Create an array of indices from 'whoFields'.
    for (i = 0; i < elementCount; i++) {
        if (Tcl_GetIndexFromObj(interp, elementPtrs[i], whoFields,
                "field", 0, &fieldIndex) != TCL_OK) {
            goto end;
        }

        switch (fieldIndex) {
            case WHO_GID:
            case WHO_GROUP: {
                flags |= ONLINE_GET_GROUPID;
            }
            case WHO_USER: {
                flags |= ONLINE_GET_USERNAME;
            }
        }

        fields[i] = (unsigned char) fieldIndex;
    }

    result = GetOnlineFields(&session, interp, fields, elementCount, flags);

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
