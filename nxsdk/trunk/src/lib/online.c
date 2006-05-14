/*++

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Online

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Online data functions.

--*/

#include "lib.h"

/*++

Io_GetOnlineData

    Retrieves online data.

Arguments:
    memory  - Pointer to an allocated IO_MEMORY structure. The buffer
              size must be at least sizeof(DC_ONLINEDATA)+(MAX_PATH+1)*2.

    routine - Callback invoked for each online data entry.

    opaque  - Argument passed to the callback, can be null if not required.

Return Value:
    None.

--*/
void
STDCALL
Io_GetOnlineData(
    IO_MEMORY *memory,
    ONLINEDATA_ROUTINE *routine,
    void *opaque
    )
{
    DWORD result;
    DC_ONLINEDATA *dcOnlineData;

    assert(memory  != NULL);
    assert(memory->size >= sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2);
    assert(routine != NULL);
    DebugPrint("Io_GetOnlineData: memory=%p routine=%p opaque=%p\n", memory, routine, opaque);

    // Initialise the data-copy online structure.
    dcOnlineData = (DC_ONLINEDATA *)memory->block;
    dcOnlineData->iOffset = 0;
    dcOnlineData->dwSharedMemorySize = memory->size;

    for (;;) {
        result = Io_ShmQuery(memory, DC_GET_ONLINEDATA, 5000);
        DebugPrint("Io_GetOnlineData: offset=%d result=%lu\n", dcOnlineData->iOffset, result);

        if (result == 0) {
            if (routine(dcOnlineData->iOffset-1, &dcOnlineData->OnlineData, opaque) == ONLINEDATA_STOP) {
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
    session - Pointer to an initialised IO_SESSION structure.

    connId  - Connection ID to kick.

Return Value:
    None.

--*/
void
STDCALL
Io_KickConnId(
    IO_SESSION *session,
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
    session - Pointer to an initialised IO_SESSION structure.

    userId  - User ID to kick.

Return Value:
    None.

--*/
void
STDCALL
Io_KickUserId(
    IO_SESSION *session,
    int userId
    )
{
    assert(session != NULL);
    DebugPrint("Io_KickUserId: session=%p userId=%d\n", session, userId);
    PostMessage(session->window, WM_KICK, (WPARAM)userId, (LPARAM)userId);
}
