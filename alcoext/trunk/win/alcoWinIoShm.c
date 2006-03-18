/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    ioFTPD Shmem

Author:
    neoxed (neoxed@gmail.com) Mar 17, 2006

Abstract:
    ioFTPD shared memory functions.

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
int
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
    DebugPrint("ShmInit: interp=%p windowObj=%p session=%p \n",
        interp, windowObj, session);

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
ShmMemory *
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
    DebugPrint("ShmAlloc: interp=%p session=%p bytes=%lu\n", interp, session, bytes);

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
void
ShmFree(
    ShmSession *session,
    ShmMemory *memory
    )
{
    assert(session != NULL);
    assert(memory  != NULL);
    DebugPrint("ShmFree: session=%p memory=%p\n", session, memory);

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
DWORD
ShmQuery(
    ShmSession *session,
    ShmMemory *memory,
    DWORD queryType,
    DWORD timeOut
    )
{
    assert(session != NULL);
    assert(memory  != NULL);
    DebugPrint("ShmQuery: session=%p memory=%p queryType=%lu timeOut=%lu\n",
        session, memory, queryType, timeOut);

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

UserCreate

    Create a new user.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_NAMEID structure.

    userName    - The user name to create.

    userId      - Location to store the user ID of the created user.

Return Value:
    A standard Tcl result.

--*/
int
UserCreate(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName,
    int *userId
    )
{
    DC_NAMEID *dcNameId;
    DWORD result;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    assert(userId   != NULL);
    DebugPrint("UserCreate: userName=%s userId=0x%p\n", userName, userId);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), userName);

    result = ShmQuery(session, memory, DC_CREATE_USER, 5000);
    if (result != (DWORD)-1) {
        *userId = (int)result;
        DebugPrint("UserCreate: OKAY\n");
        return TCL_OK;
    }

    *userId = -1;
    DebugPrint("UserCreate: FAIL\n");
    return TCL_ERROR;
}

/*++

UserRename

    Rename an existing user.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_RENAME structure.

    userName    - The user name to rename.

    newName     - The new user name.

Return Value:
    A standard Tcl result.

--*/
int
UserRename(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName,
    const char *newName
    )
{
    DC_RENAME *dcRename;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_RENAME));
    assert(userName != NULL);
    assert(newName  != NULL);
    DebugPrint("UserRename: userName=%s newName=%s\n", userName, newName);

    // Initialise the DC_RENAME structure.
    dcRename = (DC_RENAME *)memory->block;
    StringCchCopyA(dcRename->tszName,    ARRAYSIZE(dcRename->tszName),    userName);
    StringCchCopyA(dcRename->tszNewName, ARRAYSIZE(dcRename->tszNewName), newName);

    if (!ShmQuery(session, memory, DC_RENAME_USER, 5000)) {
        DebugPrint("UserRename: OKAY\n");
        return TCL_OK;
    }

    DebugPrint("UserRename: FAIL\n");
    return TCL_ERROR;
}

/*++

UserDelete

    Delete a user.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_NAMEID structure.

    userName    - The user name to delete.

Return Value:
    A standard Tcl result.

--*/
int
UserDelete(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName
    )
{
    DC_NAMEID *dcNameId;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    DebugPrint("UserDelete: userName=%s\n", userName);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), userName);

    if (!ShmQuery(session, memory, DC_DELETE_USER, 5000)) {
        DebugPrint("UserDelete: OKAY\n");
        return TCL_OK;
    }

    DebugPrint("UserDelete: FAIL\n");
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
int
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
int
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
            // The lpInternal and lpParent members must not be changed!
            //
            CopyMemory(memory->block, userFile, offsetof(USERFILE, lpInternal));

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
int
UserIdToName(
    ShmSession *session,
    ShmMemory *memory,
    int userId,
    char *userName
    )
{
    DC_NAMEID *dcNameId;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    DebugPrint("UserIdToName: userId=%d userName=0x%p\n", userId, userName);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    dcNameId->Id = userId;

    if (!ShmQuery(session, memory, DC_UID_TO_USER, 5000)) {
        StringCchCopyA(userName, _MAX_NAME+1, dcNameId->tszName);

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
int
UserNameToId(
    ShmSession *session,
    ShmMemory *memory,
    const char *userName,
    int *userId
    )
{
    DC_NAMEID *dcNameId;
    DWORD result;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(userName != NULL);
    assert(userId   != NULL);
    DebugPrint("UserNameToId: userName=%s userId=0x%p\n", userName, userId);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), userName);

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

GroupCreate

    Create a new group.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_NAMEID structure.

    groupName   - The group name to create.

    groupId     - Location to store the group ID of the created group.

Return Value:
    A standard Tcl result.

--*/
int
GroupCreate(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName,
    int *groupId
    )
{
    DC_NAMEID *dcNameId;
    DWORD result;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    assert(groupId   != NULL);
    DebugPrint("GroupCreate: groupName=%s groupId=0x%p\n", groupName, groupId);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), groupName);

    result = ShmQuery(session, memory, DC_CREATE_GROUP, 5000);
    if (result != (DWORD)-1) {
        *groupId = (int)result;
        DebugPrint("GroupCreate: OKAY\n");
        return TCL_OK;
    }

    *groupId = -1;
    DebugPrint("GroupCreate: FAIL\n");
    return TCL_ERROR;
}

/*++

GroupRename

    Rename an existing group.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_RENAME structure.

    groupName   - The group name to rename.

    newName     - The new group name.

Return Value:
    A standard Tcl result.

--*/
int
GroupRename(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName,
    const char *newName
    )
{
    DC_RENAME *dcRename;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_RENAME));
    assert(groupName != NULL);
    assert(newName  != NULL);
    DebugPrint("GroupRename: groupName=%s newName=%s\n", groupName, newName);

    // Initialise the DC_RENAME structure.
    dcRename = (DC_RENAME *)memory->block;
    StringCchCopyA(dcRename->tszName,    ARRAYSIZE(dcRename->tszName),    groupName);
    StringCchCopyA(dcRename->tszNewName, ARRAYSIZE(dcRename->tszNewName), newName);

    if (!ShmQuery(session, memory, DC_RENAME_GROUP, 5000)) {
        DebugPrint("GroupRename: OKAY\n");
        return TCL_OK;
    }

    DebugPrint("GroupRename: FAIL\n");
    return TCL_ERROR;
}

/*++

GroupDelete

    Delete a group.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold the DC_NAMEID structure.

    groupName   - The group name to delete.

Return Value:
    A standard Tcl result.

--*/
int
GroupDelete(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName
    )
{
    DC_NAMEID *dcNameId;

    assert(session  != NULL);
    assert(memory   != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    DebugPrint("GroupDelete: groupName=%s\n", groupName);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), groupName);

    if (!ShmQuery(session, memory, DC_DELETE_GROUP, 5000)) {
        DebugPrint("GroupDelete: OKAY\n");
        return TCL_OK;
    }

    DebugPrint("GroupDelete: FAIL\n");
    return TCL_ERROR;
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
int
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
int
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
            // The lpInternal and lpParent members must not be changed!
            //
            CopyMemory(memory->block, groupFile, offsetof(GROUPFILE, lpInternal));

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
int
GroupIdToName(
    ShmSession *session,
    ShmMemory *memory,
    int groupId,
    char *groupName
    )
{
    DC_NAMEID *dcNameId;

    assert(session   != NULL);
    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    DebugPrint("GroupIdToName: groupId=%d groupName=0x%p\n", groupId, groupName);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    dcNameId->Id = groupId;

    if (!ShmQuery(session, memory, DC_GID_TO_GROUP, 5000)) {
        StringCchCopyA(groupName, _MAX_NAME+1, dcNameId->tszName);

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
int
GroupNameToId(
    ShmSession *session,
    ShmMemory *memory,
    const char *groupName,
    int *groupId
    )
{
    DC_NAMEID *dcNameId;
    DWORD result;

    assert(session   != NULL);
    assert(memory    != NULL);
    assert(memory->bytes >= sizeof(DC_NAMEID));
    assert(groupName != NULL);
    assert(groupId   != NULL);
    DebugPrint("GroupNameToId: groupName=%s groupId=0x%p\n", groupName, groupId);

    // Initialise the DC_NAMEID structure.
    dcNameId = (DC_NAMEID *)memory->block;
    StringCchCopyA(dcNameId->tszName, ARRAYSIZE(dcNameId->tszName), groupName);

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

VfsFlush

    Flush the directory cache for a specified path.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be large enough to hold dirPath an a null.

    dirPath     - The directory path to flush.

Return Value:
    A standard Tcl result.

--*/
int
VfsFlush(
    ShmSession *session,
    ShmMemory *memory,
    const char *dirPath
    )
{
    assert(session != NULL);
    assert(memory  != NULL);
    assert(dirPath != NULL);
    assert(memory->bytes >= strlen(dirPath) + 1);
    DebugPrint("VfsFlush: dirPath=%s\n", dirPath);

    StringCchCopyA((char *)memory->block, (size_t)memory->bytes, dirPath);

    // ioFTPD appears to return 1 on both success and failure.
    if (ShmQuery(session, memory, DC_DIRECTORY_MARKDIRTY, 5000) == 1) {
        DebugPrint("VfsFlush: OKAY\n");
        return TCL_OK;
    }

    DebugPrint("VfsFlush: FAIL\n");
    return TCL_ERROR;
}

/*++

VfsRead

    Retrieves the ownership and permissions for a file or directory.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be at least MAX_CONTEXT + DC_VFS + pathLength + 1.

    path        - The file or directory path to query.

    pathLength  - Length of the path, in bytes.

    vfsPerm     - Pointer to a VfsPerm structure.

Return Value:
    A standard Tcl result.

--*/
int
VfsRead(
    ShmSession *session,
    ShmMemory *memory,
    const char *path,
    int pathLength,
    VfsPerm *vfsPerm
    )
{
    DC_VFS *dcVfs;

    assert(session != NULL);
    assert(memory  != NULL);
    assert(path    != NULL);
    assert(vfsPerm != NULL);
    assert(memory->bytes >= MAX_CONTEXT + sizeof(DC_VFS) + pathLength + 1);
    DebugPrint("VfsRead: path=%s vfsPerm=%p\n", path, vfsPerm);

    // Initialise the DC_VFS structure.
    dcVfs = (DC_VFS *)memory->block;
    dcVfs->dwBuffer = pathLength + 1;
    CopyMemory(dcVfs->pBuffer, path, pathLength + 1);

    if (!ShmQuery(session, memory, DC_FILEINFO_READ, 5000)) {
        vfsPerm->userId   = dcVfs->Uid;
        vfsPerm->groupId  = dcVfs->Gid;
        vfsPerm->fileMode = dcVfs->dwFileMode;

        DebugPrint("VfsRead: OKAY\n");
        return TCL_OK;
    }

    DebugPrint("VfsRead: FAIL\n");
    return TCL_ERROR;
}

/*++

VfsWrite

    Sets the ownership and permissions for a file or directory.

Arguments:
    session     - Pointer to an initialised ShmSession structure.

    memory      - Pointer to an ShmMemory structure allocated by the ShmAlloc
                  function. Must be at least MAX_CONTEXT + DC_VFS + pathLength + 1.

    path        - The file or directory path to query.

    pathLength  - Length of the path, in bytes.

    vfsPerm     - Pointer to a VfsPerm structure.

Return Value:
    A standard Tcl result.

--*/
int
VfsWrite(
    ShmSession *session,
    ShmMemory *memory,
    const char *path,
    int pathLength,
    const VfsPerm *vfsPerm
    )
{
    DC_VFS *dcVfs;

    assert(session != NULL);
    assert(memory  != NULL);
    assert(path    != NULL);
    assert(vfsPerm != NULL);
    assert(memory->bytes >= MAX_CONTEXT + sizeof(DC_VFS) + pathLength + 1);
    DebugPrint("VfsWrite: path=%s vfsPerm=%p\n", path, vfsPerm);

    // Initialise the DC_VFS structure.
    dcVfs = (DC_VFS *)memory->block;
    dcVfs->Uid        = vfsPerm->userId;
    dcVfs->Gid        = vfsPerm->groupId;
    dcVfs->dwFileMode = vfsPerm->fileMode;
    dcVfs->dwBuffer   = pathLength + 1;
    CopyMemory(dcVfs->pBuffer, path, pathLength + 1);

    if (!ShmQuery(session, memory, DC_FILEINFO_WRITE, 5000)) {
        DebugPrint("VfsWrite: OKAY\n");
        return TCL_OK;
    }

    DebugPrint("VfsWrite: FAIL\n");
    return TCL_ERROR;
}
