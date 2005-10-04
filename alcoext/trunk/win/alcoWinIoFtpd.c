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
    ioftpd user get     <msgWindow> <user>        - Get user information.
    ioftpd user set     <msgWindow> <user> <data> - Set user information.
    ioftpd user toid    <msgWindow> <user>        - User name to UID.
    ioftpd user toname  <msgWindow> <uid>         - UID to user name.

    Group Commands:
    ioftpd group exists <msgWindow> <group>        - Check if a group exists.
    ioftpd group get    <msgWindow> <group>        - Get group information.
    ioftpd group set    <msgWindow> <group> <data> - Set group information.
    ioftpd group toid   <msgWindow> <group>        - Group name to GID.
    ioftpd group toname <msgWindow> <gid>          - GID to group name.

    Online Data Commands:
    ioftpd info         <msgWindow> <varName>      - Query the ioFTPD message window.
    ioftpd kick         <msgWindow> <user>         - Kick a user.
    ioftpd kill         <msgWindow> <cid>          - Kick a connection ID.
    ioftpd who          <msgWindow> <fields>       - Query online users.

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
    Tcl_Obj *windowObj
    );

static ShmMemory *
ShmAlloc(
    ShmSession *session,
    Tcl_Interp *interp,
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
GetGroupFile(
    ShmSession *session,
    ShmMemory *memory,
    int groupId,
    GROUPFILE *groupFile
    );

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
GetUserFile(
    ShmSession *session,
    ShmMemory *memory,
    int userId,
    USERFILE *userFile
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

    windowObj   - Object containing the name of ioFTPD's message window.

Return Value:
    A standard Tcl result.

--*/
static int
ShmInit(
    ShmSession *session,
    Tcl_Interp *interp,
    Tcl_Obj *windowObj
    )
{
    char *windowName;

    assert(session    != NULL);
    assert(interp     != NULL);
    assert(windowObj  != NULL);

    windowName = Tcl_GetString(windowObj);
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

    interp      - Interpreter to use for error reporting.

    bytes       - Number of bytes to be allocated.

Return Value:
    If the function succeeds, the return value is a pointer to a ShmMemory
    structure. This structure should be freed by the ShmFree function when
    it is no longer needed. If the function fails, the return value is NULL.

--*/
static ShmMemory *
ShmAlloc(
    ShmSession *session,
    Tcl_Interp *interp,
    DWORD bytes
    )
{
    DC_MESSAGE *message = NULL;
    ShmMemory *memory;
    BOOL success = FALSE;
    HANDLE event  = NULL;
    HANDLE memMap = NULL;
    void *remote;

    assert(session != NULL);
    assert(bytes > 0);

    event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (event == NULL) {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to create event: ",
            TclSetWinError(interp, GetLastError()), NULL);
        return NULL;
    }

    memory = (ShmMemory *)ckalloc(sizeof(ShmMemory));
    bytes += sizeof(DC_MESSAGE);

    // Allocate memory in local process.
    memMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE|SEC_COMMIT, 0, bytes, NULL);

    if (memMap != NULL) {
        message = (DC_MESSAGE *)MapViewOfFile(memMap,
            FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, bytes);

        if (message != NULL) {
            // Initialise data-copy message structure.
            message->hEvent       = event;
            message->hObject      = NULL;
            message->dwIdentifier = 0;
            message->dwReturn     = 0;
            message->lpMemoryBase = (void *)message;
            message->lpContext    = &message[1];

            SetLastError(ERROR_SUCCESS);
            remote = (void *)SendMessage(session->messageWnd, WM_DATACOPY_FILEMAP,
                (WPARAM)session->processId, (LPARAM)memMap);

            if (remote != NULL) {
                success = TRUE;
            } else if (GetLastError() == ERROR_SUCCESS) {
                // I'm not sure if the SendMessage function updates the
                // system error code on failure, since MSDN does not mention
                // this behaviour. So this bullshit error will suffice.
                SetLastError(ERROR_INVALID_PARAMETER);
            }
        }
    }

    if (success) {
        // Update memory allocation structure.
        memory->message = message;
        memory->block   = &message[1];
        memory->remote  = remote;
        memory->event   = event;
        memory->memMap  = memMap;
        memory->bytes   = bytes - sizeof(DC_MESSAGE);
    } else {
        // Leave an error message in the interpreter's result.
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "unable to map memory: ",
            TclSetWinError(interp, GetLastError()), NULL);

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

        ckfree((char *)memory);
        memory = NULL;
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

    PostMessage(session->messageWnd, WM_DATACOPY_FREE, 0, (LPARAM)memory->remote);
    ckfree((char *)memory);
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
    PostMessage(session->messageWnd, WM_SHMEM, 0, (LPARAM)memory->remote);

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

GetGroupFile

    Retrieve the GROUPFILE structure for a given a group ID.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the GROUPFILE structure.

    groupId     - The group ID to look up.

    groupFile   - Pointer to a buffer to receive the GROUPFILE structure.

Return Value:
    A standard Tcl result.

--*/
static int
GetGroupFile(
    ShmSession *session,
    ShmMemory *memory,
    int groupId,
    GROUPFILE *groupFile
    )
{
    DebugPrint("GetGroupFile: START\n");
    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(GROUPFILE));
    assert(groupFile != NULL);

    // Set the requested group ID.
    ((GROUPFILE *)memory->block)->Gid = groupId;

    if (!ShmQuery(session, memory, DC_GROUPFILE_OPEN, 5000)) {
        CopyMemory(groupFile, memory->block, sizeof(GROUPFILE));

        // Close the group-file before returning.
        ShmQuery(session, memory, DC_GROUPFILE_CLOSE, 5000);

        DebugPrint("GetGroupFile: OKAY\n");
        return TCL_OK;
    }

    // Clear the group-file on failure.
    ZeroMemory(groupFile, sizeof(GROUPFILE));
    groupFile->Gid = -1;

    DebugPrint("GetGroupFile: FAIL\n");
    return TCL_ERROR;
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

    DebugPrint("GroupIdToName: START\n");
    assert(session   != NULL);
    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);

    // Initialise the DC_NAMEID structure.
    nameId = (DC_NAMEID *)memory->block;
    nameId->Id = groupId;

    if (!ShmQuery(session, memory, DC_GID_TO_GROUP, 5000)) {
        StringCchCopyA(groupName, _MAX_NAME+1, nameId->tszName);

        DebugPrint("GroupIdToName: OKAY\n");
        return TCL_OK;
    }

    groupName[0] = '\0';
    DebugPrint("GroupIdToName: FAIL\n");
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

    DebugPrint("GroupNameToId: START\n");
    assert(session   != NULL);
    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    assert(groupId   != NULL);

    // Initialise the DC_NAMEID structure.
    nameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(nameId->tszName, ARRAYSIZE(nameId->tszName), groupName);

    if (!ShmQuery(session, memory, DC_GROUP_TO_GID, 5000)) {
        *groupId = nameId->Id;

        DebugPrint("GroupNameToId: OKAY\n");
        return TCL_OK;
    }

    *groupId = -1;
    DebugPrint("GroupNameToId: FAIL\n");
    return TCL_ERROR;
}

/*++

GetUserFile

    Retrieve the USERFILE structure for a given a user ID.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the USERFILE structure.

    userId      - The user ID to look up.

    userFile    - Pointer to a buffer to receive the USERFILE structure.

Return Value:
    A standard Tcl result.

--*/
static int
GetUserFile(
    ShmSession *session,
    ShmMemory *memory,
    int userId,
    USERFILE *userFile
    )
{
    DebugPrint("GetUserFile: START\n");
    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(USERFILE));
    assert(userFile != NULL);

    // Set the requested user ID.
    ((USERFILE *)memory->block)->Uid = userId;

    if (!ShmQuery(session, memory, DC_USERFILE_OPEN, 5000)) {
        CopyMemory(userFile, memory->block, sizeof(USERFILE));

        // Close the user-file before returning.
        ShmQuery(session, memory, DC_USERFILE_CLOSE, 5000);

        DebugPrint("GetUserFile: OKAY\n");
        return TCL_OK;
    }

    // Clear the user-file on failure.
    ZeroMemory(userFile, sizeof(USERFILE));
    userFile->Uid = -1;
    userFile->Gid = -1;

    DebugPrint("GetUserFile: FAIL\n");
    return TCL_ERROR;
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

    DebugPrint("UserIdToName: START\n");
    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);

    // Initialise the DC_NAMEID structure.
    nameId = (DC_NAMEID *)memory->block;
    nameId->Id = userId;

    if (!ShmQuery(session, memory, DC_UID_TO_USER, 5000)) {
        StringCchCopyA(userName, _MAX_NAME+1, nameId->tszName);

        DebugPrint("UserIdToName: OKAY\n");
        return TCL_OK;
    }

    userName[0] = '\0';
    DebugPrint("UserIdToName: FAIL\n");
    return TCL_ERROR;
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

    DebugPrint("UserNameToId: START\n");
    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    assert(userId   != NULL);

    // Initialise the DC_NAMEID structure.
    nameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(nameId->tszName, ARRAYSIZE(nameId->tszName), userName);

    if (!ShmQuery(session, memory, DC_USER_TO_UID, 5000)) {
        *userId = nameId->Id;

        DebugPrint("UserNameToId: OKAY\n");
        return TCL_OK;
    }

    *userId = -1;
    DebugPrint("UserNameToId: FAIL\n");
    return TCL_ERROR;
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
    int groupId;
    int i;
    DC_ONLINEDATA *dcOnlineData;
    ShmMemory *memOnline;
    ShmMemory *memUser = NULL;
    Tcl_Obj *fieldObj;
    Tcl_Obj *resultObj;
    Tcl_Obj *userObj;

    memOnline = ShmAlloc(session, interp, sizeof(DC_ONLINEDATA) + (MAX_PATH+1) * 2);
    if (memOnline == NULL) {
        return TCL_ERROR;
    }

    if (flags & ONLINE_GET_GROUPID || flags & ONLINE_GET_USERNAME) {
        // Allocate a buffer large enough to hold a DC_NAMEID or USERFILE structure.
        memUser = ShmAlloc(session, interp, MAX(sizeof(DC_NAMEID), sizeof(USERFILE)));

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
            USERFILE userFile;

            // Retrieve the group ID from the user-file.
            GetUserFile(session, memUser, dcOnlineData->OnlineData.Uid, &userFile);
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
                    fieldObj = Tcl_NewLongObj((long) (GetTickCount() -
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
                    // 1 - Download
                    // 2 - Upload
                    // 3 - List
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
    ShmMemory *memory;
    ShmSession session;
    Tcl_Obj *resultObj;
    static const char *options[] = {
        "exists", "get", "set", "toid", "toname", NULL
    };
    enum options {
        GROUP_EXISTS, GROUP_GET, GROUP_SET, GROUP_TO_ID, GROUP_TO_NAME
    };

    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK ||
            ShmInit(&session, interp, objv[3]) != TCL_OK) {
        return TCL_ERROR;
    }

    resultObj = Tcl_GetObjResult(interp);
    switch ((enum options) index) {
        case GROUP_EXISTS: {
            int groupId;
            int result;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group");
                return TCL_ERROR;
            }

            // Check the group name resolves successfully.
            memory = ShmAlloc(&session, interp, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            result = GroupNameToId(&session, memory, Tcl_GetString(objv[4]), &groupId);
            ShmFree(&session, memory);

            Tcl_SetBooleanObj(resultObj, result == TCL_OK);
            return TCL_OK;
        }
        case GROUP_GET: {
            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group");
                return TCL_ERROR;
            }
            return TCL_OK;
        }
        case GROUP_SET: {
            if (objc != 6) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group data");
                return TCL_ERROR;
            }
            Tcl_SetResult(interp, "not implemented", TCL_STATIC);
            return TCL_ERROR;
        }
        case GROUP_TO_ID: {
            int groupId;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow group");
                return TCL_ERROR;
            }

            // Group name to group ID.
            memory = ShmAlloc(&session, interp, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            GroupNameToId(&session, memory, Tcl_GetString(objv[4]), &groupId);
            ShmFree(&session, memory);

            Tcl_SetIntObj(resultObj, groupId);
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
            memory = ShmAlloc(&session, interp, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            GroupIdToName(&session, memory, groupId, groupName);
            ShmFree(&session, memory);

            Tcl_SetStringObj(resultObj, groupName, -1);
            return TCL_OK;
        }
    }

    // This point should never be reached.
    Tcl_Panic("unexpected fallthrough");
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

    if (ShmInit(&session, interp, objv[2]) != TCL_OK) {
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
        Tcl_DecrRefCount(valueObj);                                                       \
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

    if (ShmInit(&session, interp, objv[2]) != TCL_OK ||
            Tcl_GetIntFromObj(interp, objv[3], &userId) != TCL_OK) {
        return TCL_ERROR;
    }

    PostMessage(session.messageWnd, WM_KICK, (WPARAM)userId, 0);
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

    if (ShmInit(&session, interp, objv[2]) != TCL_OK ||
            Tcl_GetIntFromObj(interp, objv[3], &connId) != TCL_OK) {
        return TCL_ERROR;
    }

    PostMessage(session.messageWnd, WM_KILL, (WPARAM)connId, 0);
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
    ShmMemory *memory;
    ShmSession session;
    Tcl_Obj *resultObj;
    static const char *options[] = {
        "exists", "get", "set", "toid", "toname", NULL
    };
    enum options {
        USER_EXISTS, USER_GET, USER_SET, USER_TO_ID, USER_TO_NAME
    };

    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK ||
            ShmInit(&session, interp, objv[3]) != TCL_OK) {
        return TCL_ERROR;
    }

    resultObj = Tcl_GetObjResult(interp);
    switch ((enum options) index) {
        case USER_EXISTS: {
            int userId;
            int result;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user");
                return TCL_ERROR;
            }

            // Check the user name resolves successfully.
            memory = ShmAlloc(&session, interp, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            result = UserNameToId(&session, memory, Tcl_GetString(objv[4]), &userId);
            ShmFree(&session, memory);

            Tcl_SetBooleanObj(resultObj, result == TCL_OK);
            return TCL_OK;
        }
        case USER_GET: {
            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user");
                return TCL_ERROR;
            }
            return TCL_OK;
        }
        case USER_SET: {
            if (objc != 6) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user data");
                return TCL_ERROR;
            }
            Tcl_SetResult(interp, "not implemented", TCL_STATIC);
            return TCL_ERROR;
        }
        case USER_TO_ID: {
            int userId;

            if (objc != 5) {
                Tcl_WrongNumArgs(interp, 3, objv, "msgWindow user");
                return TCL_ERROR;
            }

            // User name to user ID.
            memory = ShmAlloc(&session, interp, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            UserNameToId(&session, memory, Tcl_GetString(objv[4]), &userId);
            ShmFree(&session, memory);

            Tcl_SetIntObj(resultObj, userId);
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
            memory = ShmAlloc(&session, interp, sizeof(DC_NAMEID));
            if (memory == NULL) {
                return TCL_ERROR;
            }
            UserIdToName(&session, memory, userId, userName);
            ShmFree(&session, memory);

            Tcl_SetStringObj(resultObj, userName, -1);
            return TCL_OK;
        }
    }

    // This point should never be reached.
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
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

    if (ShmInit(&session, interp, objv[2]) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_ListObjGetElements(interp, objv[3], &elementCount, &elementPtrs) != TCL_OK) {
        return TCL_ERROR;
    }

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
    assert(!strcmp("size",         whoFields[WHO_SIZE]));
    assert(!strcmp("speed",        whoFields[WHO_SPEED]));
    assert(!strcmp("status",       whoFields[WHO_STATUS]));
    assert(!strcmp("uid",          whoFields[WHO_UID]));
    assert(!strcmp("user",         whoFields[WHO_USER]));
    assert(!strcmp("vdatapath",    whoFields[WHO_VDATAPATH]));
    assert(!strcmp("vpath",        whoFields[WHO_VPATH]));

    // Create an array of indices from "whoFields".
    fields = (unsigned char *)ckalloc(elementCount * sizeof(unsigned char));

    for (i = 0; i < elementCount; i++) {
        if (Tcl_GetIndexFromObj(interp, elementPtrs[i], whoFields,
                "field", 0, &fieldIndex) != TCL_OK) {
            goto end;
        }

        if (fieldIndex == WHO_GID || fieldIndex == WHO_GROUP) {
            flags |= ONLINE_GET_GROUPID;
        } else if (fieldIndex == WHO_USER) {
            flags |= ONLINE_GET_USERNAME;
        }

        fields[i] = (unsigned char) fieldIndex;
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
