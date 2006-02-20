/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    alcoWinIoFtpd.c

Author:
    neoxed (neoxed@gmail.com) Apr 16, 2005

Abstract:
    Implements a Tcl command-based interface for interaction with ioFTPD.

    User Commands:
    ioftpd user exists  <msgWindow> <user>         - Check if a user exists.
    ioftpd user get     <msgWindow> <user>         - Get user information.
    ioftpd user set     <msgWindow> <user> <data>  - Set user information.
    ioftpd user toid    <msgWindow> <user>         - User name to UID.
    ioftpd user toname  <msgWindow> <uid>          - UID to user name.

    Group Commands:
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

--*/

// Strings in ioFTPD's headers are declared as TCHAR's, even though
// the shared memory interface does not support wide characters.
#undef UNICODE
#undef _UNICODE

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
    Tcl_Interp *interp,
    Tcl_Obj *windowObj,
    ShmSession *session
    );

static ShmMemory *
ShmAlloc(
    Tcl_Interp *interp,
    ShmSession *session,
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
// Helper functions.
//

static int
GroupGetFile(
    ShmSession *session,
    ShmMemory *memory,
    int groupId,
    GROUPFILE *groupFile
    );

static int
GroupSetFile(
    ShmSession *session,
    ShmMemory *memory,
    const GROUPFILE *groupFile
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
UserGetFile(
    ShmSession *session,
    ShmMemory *memory,
    int userId,
    USERFILE *userFile
    );

static int
UserSetFile(
    ShmSession *session,
    ShmMemory *memory,
    const USERFILE *userFile
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


/*++

ShmInit

    Initialise a shared memory session.

Arguments:
    interp      - Interpreter to use for error reporting.

    windowObj   - Object containing the name of ioFTPD's message window.

    session     - Pointer to the ShmSession structure to be initialised.

Return Value:
    A standard Tcl result.

--*/
static int
ShmInit(
    Tcl_Interp *interp,
    Tcl_Obj *windowObj,
    ShmSession *session
    )
{
    char *windowName;

    assert(interp    != NULL);
    assert(session   != NULL);
    assert(windowObj != NULL);

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
    interp      - Interpreter to use for error reporting.

    session     - Pointer to an initialised ShmSession structure.

    bytes       - Number of bytes to be allocated.

Return Value:
    If the function succeeds, the return value is a pointer to a ShmMemory
    structure. This structure should be freed by the ShmFree function when
    it is no longer needed. If the function fails, the return value is NULL.

--*/
static ShmMemory *
ShmAlloc(
    Tcl_Interp *interp,
    ShmSession *session,
    DWORD bytes
    )
{
    DC_MESSAGE *message = NULL;
    ShmMemory *memory;
    BOOL success = FALSE;
    HANDLE event  = NULL;
    HANDLE memMap = NULL;
    void *remote;

    assert(interp  != NULL);
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

    memory->message->dwReturn     = (DWORD)-1;
    memory->message->dwIdentifier = queryType;
    PostMessage(session->messageWnd, WM_SHMEM, 0, (LPARAM)memory->remote);

    if (timeOut && memory->event != NULL) {
        if (WaitForSingleObject(memory->event, timeOut) == WAIT_TIMEOUT) {
            DebugPrint("ShmQuery: Timed out (%lu)\n", GetLastError());
            return (DWORD)-1;
        }

        DebugPrint("ShmQuery: Return=%lu\n", memory->message->dwReturn);
        return memory->message->dwReturn;
    }

    // No timeout or event, return value cannot be checked.
    DebugPrint("ShmQuery: No event or time out!\n");
    return (DWORD)-1;
}


/*++

RowDataGet

    Parses row data from a data structure into a Tcl list.

Arguments:
    rowData     - Pointer to the row definition structure.

    data        - Pointer to a buffer (usually a data structure) containing
                  the data to be parsed.

    listObj     - Pointer to a Tcl object which receives the parsed data as a list.

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
        assert(rowData[i].type == TYPE_BIN ||
               rowData[i].type == TYPE_I32 ||
               rowData[i].type == TYPE_I32LIST ||
               rowData[i].type == TYPE_I64 ||
               rowData[i].type == TYPE_STR ||
               rowData[i].type == TYPE_STRLIST);
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

    Parses row data from a Tcl list into a data structure.

Arguments:
    interp      - Interpreter to use for error reporting.

    listObj     - Pointer to a Tcl object containing the list to be parsed.

    rowData     - Pointer to the row definition structure.

    data        - Pointer to a buffer (usually a data structure) which
                  receives the parsed data as a list.

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
    int i;
    int rowIndex;
    Tcl_Obj **elementPtrs;

    assert(interp  != NULL);
    assert(listObj != NULL);
    assert(rowData == userRowDef || rowData == groupRowDef);
    assert(data    != NULL);

    if (Tcl_ListObjGetElements(interp, listObj, &elementCount, &elementPtrs) != TCL_OK) {
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
        if (Tcl_GetIndexFromObjStruct(interp, elementPtrs[i], rowData,
                sizeof(RowData), "field", TCL_EXACT, &rowIndex) != TCL_OK) {
            return TCL_ERROR;
        }

        // Process the data value.
        i++;

        // TODO
    }

    return TCL_OK;
}


/*++

GroupGetFile

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
GroupGetFile(
    ShmSession *session,
    ShmMemory *memory,
    int groupId,
    GROUPFILE *groupFile
    )
{
    assert(session   != NULL);
    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(GROUPFILE));
    assert(groupFile != NULL);
    DebugPrint("GroupGetFile: groupId=%d groupFile=0x%p\n", groupId, groupFile);

    // Set the requested group ID.
    ((GROUPFILE *)memory->block)->Gid = groupId;

    if (!ShmQuery(session, memory, DC_GROUPFILE_OPEN, 5000)) {
        CopyMemory(groupFile, memory->block, sizeof(GROUPFILE));

        // Close the group-file before returning.
        ShmQuery(session, memory, DC_GROUPFILE_CLOSE, 5000);

        DebugPrint("GroupGetFile: OKAY\n");
        return TCL_OK;
    }

    // Clear the group-file on failure.
    ZeroMemory(groupFile, sizeof(GROUPFILE));
    groupFile->Gid = -1;

    DebugPrint("GroupGetFile: FAIL\n");
    return TCL_ERROR;
}

/*++

GroupSetFile

    Update the GROUPFILE structure for a group.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the GROUPFILE structure.

    groupFile   - Pointer to an initialised GROUPFILE structure.

Return Value:
    A standard Tcl result.

--*/
static int
GroupSetFile(
    ShmSession *session,
    ShmMemory *memory,
    const GROUPFILE *groupFile
    )
{
    int status = TCL_ERROR;

    assert(session   != NULL);
    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(GROUPFILE));
    assert(groupFile != NULL);
    DebugPrint("GroupSetFile: groupFile=0x%p groupFile->Gid=%d\n", groupFile, groupFile->Gid);

    // Set the requested group ID.
    ((GROUPFILE *)memory->block)->Gid = groupFile->Gid;

    if (!ShmQuery(session, memory, DC_GROUPFILE_OPEN, 5000)) {
        if (!ShmQuery(session, memory, DC_GROUPFILE_LOCK, 5000)) {
            //
            // Copy the GROUPFILE structure to the shared memory block
            // after locking, since the open call will overwrite it.
            //
            CopyMemory(memory->block, groupFile, sizeof(GROUPFILE));

            // Unlock will update the group-file.
            ShmQuery(session, memory, DC_GROUPFILE_UNLOCK, 5000);

            status = TCL_OK;
            DebugPrint("GroupSetFile: OKAY\n");
        } else {
            DebugPrint("GroupSetFile: LOCK FAIL\n");
        }

        // Close the group-file before returning.
        ShmQuery(session, memory, DC_GROUPFILE_CLOSE, 5000);
    } else {
        DebugPrint("GroupSetFile: OPEN FAIL\n");
    }

    return status;
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

    assert(session   != NULL);
    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    DebugPrint("GroupIdToName: groupId=%d groupName=0x%p\n", groupId, groupName);

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
    DWORD result;

    assert(session   != NULL);
    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    assert(groupId   != NULL);
    DebugPrint("GroupNameToId: groupName=%s groupId=0x%p\n", groupName, groupId);

    // Initialise the DC_NAMEID structure.
    nameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(nameId->tszName, ARRAYSIZE(nameId->tszName), groupName);

    //
    // The DC_NAMEID structure is not updated with the group ID,
    // instead the group ID is the return value (DC_MESSAGE::dwReturn).
    // So much for consistency...
    //
    result = ShmQuery(session, memory, DC_GROUP_TO_GID, 5000);
    if (result != (DWORD)-1) {
        *groupId = (int)result;

        DebugPrint("GroupNameToId: OKAY\n");
        return TCL_OK;
    }

    *groupId = -1;
    DebugPrint("GroupNameToId: FAIL\n");
    return TCL_ERROR;
}

/*++

UserGetFile

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
UserGetFile(
    ShmSession *session,
    ShmMemory *memory,
    int userId,
    USERFILE *userFile
    )
{
    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(USERFILE));
    assert(userFile != NULL);
    DebugPrint("UserGetFile: userId=%d userFile=0x%p\n", userId, userFile);

    // Set the requested user ID.
    ((USERFILE *)memory->block)->Uid = userId;

    if (!ShmQuery(session, memory, DC_USERFILE_OPEN, 5000)) {
        CopyMemory(userFile, memory->block, sizeof(USERFILE));

        // Close the user-file before returning.
        ShmQuery(session, memory, DC_USERFILE_CLOSE, 5000);

        DebugPrint("UserGetFile: OKAY\n");
        return TCL_OK;
    }

    // Clear the user-file on failure.
    ZeroMemory(userFile, sizeof(USERFILE));
    userFile->Uid = -1;
    userFile->Gid = -1;

    DebugPrint("UserGetFile: FAIL\n");
    return TCL_ERROR;
}

/*++

UserSetFile

    Update the USERFILE structure for a user.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the USERFILE structure.

    userFile    - Pointer to an initialised USERFILE structure.

Return Value:
    A standard Tcl result.

--*/
static int
UserSetFile(
    ShmSession *session,
    ShmMemory *memory,
    const USERFILE *userFile
    )
{
    int status = TCL_ERROR;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(USERFILE));
    assert(userFile != NULL);
    DebugPrint("UserSetFile: userFile=0x%p userFile->Uid=%d\n", userFile, userFile->Uid);

    // Set the requested user ID.
    ((USERFILE *)memory->block)->Uid = userFile->Uid;

    if (!ShmQuery(session, memory, DC_USERFILE_OPEN, 5000)) {
        if (!ShmQuery(session, memory, DC_USERFILE_LOCK, 5000)) {
            //
            // Copy the USERFILE structure to the shared memory block
            // after locking, since the open call will overwrite it.
            //
            CopyMemory(memory->block, userFile, sizeof(USERFILE));

            // Unlock will update the user-file.
            ShmQuery(session, memory, DC_USERFILE_UNLOCK, 5000);

            status = TCL_OK;
            DebugPrint("UserSetFile: OKAY\n");
        } else {
            DebugPrint("UserSetFile: LOCK FAIL\n");
        }

        // Close the user-file before returning.
        ShmQuery(session, memory, DC_USERFILE_CLOSE, 5000);
    } else {
        DebugPrint("UserSetFile: OPEN FAIL\n");
    }

    return status;
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

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    DebugPrint("UserIdToName: userId=%d userName=0x%p\n", userId, userName);

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
    DWORD result;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    assert(userId   != NULL);
    DebugPrint("UserNameToId: userName=%s userId=0x%p\n", userName, userId);

    // Initialise the DC_NAMEID structure.
    nameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(nameId->tszName, ARRAYSIZE(nameId->tszName), userName);

    //
    // The DC_NAMEID structure is not updated with the user ID,
    // instead the user ID is the return value (DC_MESSAGE::dwReturn).
    // So much for consistency...
    //
    result = ShmQuery(session, memory, DC_USER_TO_UID, 5000);
    if (result != (DWORD)-1) {
        *userId = (int)result;

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

    for (;;) {
        result = ShmQuery(session, memOnline, DC_GET_ONLINEDATA, 5000);
        DebugPrint("GetOnlineFields: offset=%d result=%lu\n",
            dcOnlineData->iOffset, result);

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
                    if (dcOnlineData->OnlineData.dwIntervalLength > 0) {
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
    ShmMemory *memory;
    ShmSession session;
    Tcl_Obj *resultObj;
    static const char *options[] = {
        "exists", "get", "set", "toid", "toname", NULL
    };
    enum optionIndices {
        GROUP_EXISTS = 0, GROUP_GET, GROUP_SET, GROUP_TO_ID, GROUP_TO_NAME
    };

    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK ||
            ShmInit(interp, objv[3], &session) != TCL_OK) {
        return TCL_ERROR;
    }

    resultObj = Tcl_GetObjResult(interp);
    switch ((enum optionIndices) index) {
        case GROUP_GET:
        case GROUP_SET: {
            char *groupName;
            int groupId;
            int result;
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

                Tcl_AppendResult(interp, "invalid group name \"",
                    groupName, "\"", NULL);
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
            int result;

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

    if (ShmInit(interp, objv[2], &session) != TCL_OK ||
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

    if (ShmInit(interp, objv[2], &session) != TCL_OK ||
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
    enum optionIndices {
        USER_EXISTS = 0, USER_GET, USER_SET, USER_TO_ID, USER_TO_NAME
    };

    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "option arg ?arg ...?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], options, "option", 0, &index) != TCL_OK ||
            ShmInit(interp, objv[3], &session) != TCL_OK) {
        return TCL_ERROR;
    }

    resultObj = Tcl_GetObjResult(interp);
    switch ((enum optionIndices) index) {
        case USER_GET:
        case USER_SET: {
            char *userName;
            int userId;
            int result;
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

                Tcl_AppendResult(interp, "invalid user name \"",
                    userName, "\"", NULL);
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
            int result;

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

    if (ShmInit(interp, objv[2], &session) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Tcl_ListObjGetElements(interp, objv[3], &elementCount, &elementPtrs) != TCL_OK) {
        return TCL_ERROR;
    }

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
    enum optionIndices {
        OPTION_GROUP = 0, OPTION_INFO, OPTION_KICK,
        OPTION_KILL, OPTION_USER, OPTION_WHO
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
        case OPTION_WHO:   return IoWhoCmd(interp, objc, objv);
    }

    // This point should never be reached.
    Tcl_Panic("unexpected fallthrough");
    return TCL_ERROR;
}
