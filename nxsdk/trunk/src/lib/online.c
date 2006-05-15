/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Online Data

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Online data functions.

*/

#include "lib.h"

/*++

Io_GetOnlineDataEx

    Retrieves extended online data.

Arguments:
    memory   - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
               buffer size must be at least "sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2".

    callback - Pointer to a Io_OnlineDataExProc callback function that is
               called for each online user.

    opaque   - Argument passed to the callback, can be NULL if not required.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

Remarks:
    This function retrieves the user name, group name, user file, and group file,
    for all online users - so it is slower. Use the Io_GetOnlineData function
    if you do not require this additional information.

--*/
BOOL
STDCALL
Io_GetOnlineDataEx(
    IO_MEMORY *memory,
    Io_OnlineDataExProc *callback,
    void *opaque
    )
{
    DWORD amount;
    DWORD result;
    DC_ONLINEDATA *dcOnlineData;
    IO_MEMORY *memEx;
    IO_ONLINEDATAEX onlineDataEx;
    IO_SESSION session;

    // Validate arguments.
    if (memory == NULL || callback == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(DC_ONLINEDATA) + ((MAX_PATH + 1) * 2)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialise a session structure.
    session.window = memory->window;
    session.currentProcId = memory->procId;
    session.remoteProcId  = 0; // Not used by Io_ShmAlloc.

    // Allocate the largest single amount.
    amount = sizeof(USERFILE);
    amount = MAX(amount, sizeof(GROUPFILE));
    amount = MAX(amount, sizeof(DC_NAMEID));
    memEx = Io_ShmAlloc(&session, amount);
    if (memEx == NULL) {
        return FALSE;
    }

    // Initialise the data-copy structure for online information.
    dcOnlineData = (DC_ONLINEDATA *)memory->block;
    dcOnlineData->iOffset = 0;
    dcOnlineData->dwSharedMemorySize = memory->size;

    while (dcOnlineData->iOffset >= 0) {
        result = Io_ShmQuery(memory, DC_GET_ONLINEDATA, 5000);
        DebugPrint("Io_GetOnlineDataEx: offset=%d result=%lu size=%lu\n",
            dcOnlineData->iOffset, result, dcOnlineData->dwSharedMemorySize);

        if (!result) {
            // Initialise the IO_ONLINEDATAEX structure.
            onlineDataEx.connId = dcOnlineData->iOffset-1;
            CopyMemory(&onlineDataEx.onlineData, &dcOnlineData->OnlineData, sizeof(ONLINEDATA));

            // Retrieve user information.
            Io_UserGetFile(memEx, dcOnlineData->OnlineData.Uid, &onlineDataEx.userFile);
            Io_UserIdToName(memEx, dcOnlineData->OnlineData.Uid, onlineDataEx.userName);

            // Retrieve group information.
            Io_GroupGetFile(memEx, onlineDataEx.userFile.Gid, &onlineDataEx.groupFile);
            Io_GroupIdToName(memEx, onlineDataEx.userFile.Gid, onlineDataEx.groupName);

            if (callback(&onlineDataEx, opaque) == IO_ONLINEDATA_STOP) {
                // The caller requested to stop.
                break;
            }

        } else if (result == (DWORD)-1) {
            // An error occured or we've processed all online data.
            break;

        } else {
            // Unknown result, skip user.
            dcOnlineData->iOffset++;
        }
    }

    Io_ShmFree(memEx);
    return TRUE;
}

/*++

Io_GetOnlineData

    Retrieves online data.

Arguments:
    memory   - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
               buffer size must be at least "sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2".

    callback - Pointer to a Io_OnlineDataProc callback function that is
               called for each online user.

    opaque   - Argument passed to the callback, can be NULL if not required.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

Remarks:
    The Io_GetOnlineDataEx function retrieves the user name, group name,
    user file, and group file in addition to the online data. If you require
    this additional information, you should use Io_GetOnlineDataEx.

--*/
BOOL
STDCALL
Io_GetOnlineData(
    IO_MEMORY *memory,
    Io_OnlineDataProc *callback,
    void *opaque
    )
{
    DWORD result;
    DC_ONLINEDATA *dcOnlineData;

    // Validate arguments.
    if (memory == NULL || callback == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Check if the shared memory block is large enough.
    if (memory->size < sizeof(DC_ONLINEDATA) + ((MAX_PATH + 1) * 2)) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Initialise the data-copy structure for online information.
    dcOnlineData = (DC_ONLINEDATA *)memory->block;
    dcOnlineData->iOffset = 0;
    dcOnlineData->dwSharedMemorySize = memory->size;

    while (dcOnlineData->iOffset >= 0) {
        result = Io_ShmQuery(memory, DC_GET_ONLINEDATA, 5000);
        DebugPrint("Io_GetOnlineData: offset=%d result=%lu size=%lu\n",
            dcOnlineData->iOffset, result, dcOnlineData->dwSharedMemorySize);

        if (!result) {
            if (callback(dcOnlineData->iOffset-1, &dcOnlineData->OnlineData, opaque) == IO_ONLINEDATA_STOP) {
                // The caller requested to stop.
                break;
            }

        } else if (result == (DWORD)-1) {
            // An error occured or we've processed all online data.
            break;

        } else {
            // Unknown result, skip user.
            dcOnlineData->iOffset++;
        }
    }

    return TRUE;
}

/*++

Io_KickConnId

    Kicks the specified connection ID.

Arguments:
    session - Pointer to an IO_SESSION structure initialised by Io_ShmInit.

    connId  - Specifies the connection ID to kick.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
STDCALL
Io_KickConnId(
    const IO_SESSION *session,
    int connId
    )
{
    // Validate arguments.
    if (session == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    PostMessage(session->window, WM_KILL, (WPARAM)connId, (LPARAM)connId);
    return TRUE;
}

/*++

Io_KickUserId

    Kicks the specified user ID.

Arguments:
    session - Pointer to an IO_SESSION structure initialised by Io_ShmInit.

    userId  - Specifies the user ID to kick.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
STDCALL
Io_KickUserId(
    const IO_SESSION *session,
    int userId
    )
{
    // Validate arguments.
    if (session == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    PostMessage(session->window, WM_KICK, (WPARAM)userId, (LPARAM)userId);
    return TRUE;
}
