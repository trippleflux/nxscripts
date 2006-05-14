/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Online

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Online data functions.

*/

#include "lib.h"

/*++

Io_GetOnlineData

    Retrieves online data.

Arguments:
    session - Pointer to an initialised IO_SESSION structure.

    memory  - Pointer to an allocated IO_MEMORY structure. The buffer
              size must be at least sizeof(DC_ONLINEDATA)+(MAX_PATH+1)*2.

    routine - Callback invoked for each online data entry.

    opaque  - Argument passed to the callback, can be null if not required.

Return Value:
    None.

--*/
void
Io_GetOnlineData(
    IO_SESSION *session,
    IO_MEMORY *memory,
    ONLINEDATA_ROUTINE *routine,
    void *opaque
    )
{
    DWORD result;
    DC_ONLINEDATA *dcOnlineData;

    assert(session != NULL);
    assert(memory  != NULL);
    assert(memory->bytes >= sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2);
    assert(routine != NULL);
    DebugPrint("Io_GetOnlineData: session=%p memory=%p routine=%p opaque=%p\n",
        session, memory, routine, opaque);

    // Initialise the data-copy online structure.
    dcOnlineData = (DC_ONLINEDATA *)memory->block;
    dcOnlineData->iOffset = 0;
    dcOnlineData->dwSharedMemorySize = memory->bytes;

    for (;;) {
        result = Io_ShmQuery(session, memory, DC_GET_ONLINEDATA, 5000);
        DebugPrint("Io_GetOnlineData: offset=%d result=%lu\n", dcOnlineData->iOffset, result);

        if (result == 0) {
            // Stop if the callback returns zero.
            if (!routine(session, dcOnlineData->iOffset-1, &dcOnlineData->OnlineData, opaque)) {
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
Io_KickConnId(
    IO_SESSION *session,
    int connId
    )
{
    assert(session != NULL);
    DebugPrint("Io_KickConnId: session=%p connId=%d\n", session, connId);
    PostMessage(session->messageWnd, WM_KILL, (WPARAM)connId, (LPARAM)connId);
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
Io_KickUserId(
    IO_SESSION *session,
    int userId
    )
{
    assert(session != NULL);
    DebugPrint("Io_KickUserId: session=%p userId=%d\n", session, userId);
    PostMessage(session->messageWnd, WM_KICK, (WPARAM)userId, (LPARAM)userId);
}
