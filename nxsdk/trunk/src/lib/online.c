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
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
              buffer size must be at least "sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2".

    routine - Pointer to a Io_OnlineDataExRoutine callback function that is
              called for each online user.

    opaque  - Argument passed to the callback, can be NULL if not required.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GetOnlineDataEx(
    IO_MEMORY *memory,
    Io_OnlineDataExRoutine *routine,
    void *opaque
    )
{
    DWORD amount;
    DWORD result;
    DC_ONLINEDATA *dcOnlineData;
    IO_MEMORY *memEx;
    IO_ONLINEDATAEX onlineDataEx;
    IO_SESSION session;

    assert(memory  != NULL);
    assert(memory->size >= sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2);
    assert(routine != NULL);
    DebugPrint("Io_GetOnlineDataEx: memory=%p routine=%p opaque=%p\n", memory, routine, opaque);

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

    for (;;) {
        result = Io_ShmQuery(memory, DC_GET_ONLINEDATA, 5000);
        DebugPrint("Io_GetOnlineDataEx: offset=%d result=%lu\n", dcOnlineData->iOffset, result);

        if (result == 0) {
            // Initialise the IO_ONLINEDATAEX structure.
            onlineDataEx.connId = dcOnlineData->iOffset-1;
            CopyMemory(&onlineDataEx.onlineData, &dcOnlineData->OnlineData, sizeof(ONLINEDATA));

            // Retrieve user information.
            Io_UserGetFile(memEx, dcOnlineData->OnlineData.Uid, &onlineDataEx.userFile);
            Io_UserIdToName(memEx, dcOnlineData->OnlineData.Uid, onlineDataEx.userName);

            // Retrieve group information.
            Io_GroupGetFile(memEx, onlineDataEx.userFile.Gid, &onlineDataEx.groupFile);
            Io_GroupIdToName(memEx, onlineDataEx.userFile.Gid, onlineDataEx.groupName);

            if (routine(&onlineDataEx, opaque) == IO_ONLINEDATA_STOP) {
                // The caller requested to stop.
                break;
            }

        } else if (result == (DWORD)-1) {
            // An error occured.
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
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc. The
              buffer size must be at least "sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2".

    routine - Pointer to a Io_OnlineDataRoutine callback function that is
              called for each online user.

    opaque  - Argument passed to the callback, can be NULL if not required.

Return Values:
    None.

--*/
void
STDCALL
Io_GetOnlineData(
    IO_MEMORY *memory,
    Io_OnlineDataRoutine *routine,
    void *opaque
    )
{
    DWORD result;
    DC_ONLINEDATA *dcOnlineData;

    assert(memory  != NULL);
    assert(memory->size >= sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2);
    assert(routine != NULL);
    DebugPrint("Io_GetOnlineData: memory=%p routine=%p opaque=%p\n", memory, routine, opaque);

    // Initialise the data-copy structure for online information.
    dcOnlineData = (DC_ONLINEDATA *)memory->block;
    dcOnlineData->iOffset = 0;
    dcOnlineData->dwSharedMemorySize = memory->size;

    for (;;) {
        result = Io_ShmQuery(memory, DC_GET_ONLINEDATA, 5000);
        DebugPrint("Io_GetOnlineData: offset=%d result=%lu\n", dcOnlineData->iOffset, result);

        if (result == 0) {
            if (routine(dcOnlineData->iOffset-1, &dcOnlineData->OnlineData, opaque) == IO_ONLINEDATA_STOP) {
                // The caller requested to stop.
                break;
            }

        } else if (result == (DWORD)-1) {
            // An error occured.
            break;

        } else {
            // Unknown result, skip user.
            dcOnlineData->iOffset++;
        }
    }
}

/*++

Io_KickConnId

    Kicks the specified connection ID.

Arguments:
    session - Pointer to an IO_SESSION structure initialised by Io_ShmInit.

    connId  - Specifies the connection ID to kick.

Return Values:
    None.

--*/
void
STDCALL
Io_KickConnId(
    const IO_SESSION *session,
    int connId
    )
{
    assert(session != NULL);
    DebugPrint("Io_KickConnId: session=%p connId=%d\n", session, connId);
    PostMessage(session->window, WM_KILL, (WPARAM)connId, (LPARAM)connId);
}

/*++

Io_KickUserId

    Kicks the specified user ID.

Arguments:
    session - Pointer to an IO_SESSION structure initialised by Io_ShmInit.

    userId  - Specifies the user ID to kick.

Return Values:
    None.

--*/
void
STDCALL
Io_KickUserId(
    const IO_SESSION *session,
    int userId
    )
{
    assert(session != NULL);
    DebugPrint("Io_KickUserId: session=%p userId=%d\n", session, userId);
    PostMessage(session->window, WM_KICK, (WPARAM)userId, (LPARAM)userId);
}
