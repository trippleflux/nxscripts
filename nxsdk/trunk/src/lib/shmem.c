/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Shared Memory

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Shared memory functions.

*/

#include "lib.h"

/*++

Io_ShmInit

    Initialise a shared memory session.

Arguments:
    window  - Name of ioFTPD's message window.

    session - Pointer to the IO_SESSION structure to be initialised.

Return Value:
    A standard boolean result.

--*/
BOOL
Io_ShmInit(
    const char *window,
    IO_SESSION *session
    )
{
    assert(window  != NULL);
    assert(session != NULL);
    DebugPrint("Io_ShmInit: window=%s session=%p\n", window, session);

    session->messageWnd = FindWindowA(window, NULL);
    if (session->messageWnd == NULL) {
        session->currentProcId = 0;
        session->remoteProcId  = 0;
        return FALSE;
    }

    session->currentProcId = GetCurrentProcessId();
    GetWindowThreadProcessId(session->messageWnd, &session->remoteProcId);
    return TRUE;
}

/*++

Io_ShmAlloc

    Allocates shared memory.

Arguments:
    session - Pointer to an initialised IO_SESSION structure.

    bytes   - Number of bytes to be allocated.

Remarks:
    The allocated IO_MEMORY structure must be freed with by Io_ShmFree.

Return Value:
    If the function succeeds, the return value is a pointer to a IO_MEMORY
    structure. If the function fails, the return value is NULL.

--*/
IO_MEMORY *
Io_ShmAlloc(
    IO_SESSION *session,
    DWORD bytes
    )
{
    DC_MESSAGE *message = NULL;
    IO_MEMORY *memory;
    HANDLE event;
    HANDLE memMap = NULL;

    assert(session != NULL);
    assert(bytes > 0);
    DebugPrint("Io_ShmAlloc: session=%p bytes=%lu\n", session, bytes);

    memory = malloc(sizeof(IO_MEMORY));
    if (memory == NULL) {
        return NULL;
    }

    event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (event != NULL) {
        bytes += sizeof(DC_MESSAGE);

        // Allocate memory in local process.
        memMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
            PAGE_READWRITE|SEC_COMMIT, 0, bytes, NULL);

        if (memMap != NULL) {
            message = (DC_MESSAGE *)MapViewOfFile(memMap,
                FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, bytes);

            if (message != NULL) {
                void *remote;

                // Initialise data-copy message structure.
                message->hEvent       = event;
                message->hObject      = NULL;
                message->lpMemoryBase = (void *)message;
                message->lpContext    = &message[1];

                SetLastError(ERROR_SUCCESS);
                remote = (void *)SendMessage(session->messageWnd, WM_DATACOPY_FILEMAP,
                    (WPARAM)session->currentProcId, (LPARAM)memMap);

                if (remote != NULL) {
                    // Populate memory allocation structure.
                    memory->message = message;
                    memory->block   = &message[1];
                    memory->remote  = remote;
                    memory->event   = event;
                    memory->memMap  = memMap;
                    memory->bytes   = bytes - sizeof(DC_MESSAGE);
                    return memory;
                }

                // I'm not sure if the SendMessage function updates the system error
                // code on failure, since MSDN does not mention this behaviour. So
                // this error code will suffice.
                if (GetLastError() == ERROR_SUCCESS) {
                    SetLastError(ERROR_INVALID_PARAMETER);
                }
            }
        }
    }

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
    free(memory);
    return NULL;
}

/*++

Io_ShmFree

    Frees shared memory.

Arguments:
    session - Pointer to an initialised IO_SESSION structure.

    memory  - Pointer to an allocated IO_MEMORY structure.

Return Value:
    None.

--*/
void
Io_ShmFree(
    IO_SESSION *session,
    IO_MEMORY *memory
    )
{
    assert(session != NULL);
    assert(memory  != NULL);
    DebugPrint("Io_ShmFree: session=%p memory=%p\n", session, memory);

    // Free objects and resources.
    UnmapViewOfFile(memory->message);

    if (memory->event != NULL) {
        CloseHandle(memory->event);
    }
    if (memory->memMap != NULL) {
        CloseHandle(memory->memMap);
    }

    PostMessage(session->messageWnd, WM_DATACOPY_FREE, 0, (LPARAM)memory->remote);
    free(memory);
}

/*++

Io_ShmQuery

    Queries the ioFTPD daemon.

Arguments:
    session - Pointer to an initialised IO_SESSION structure.

    memory  - Pointer to an allocated IO_MEMORY structure.

    queryId - Query identifier, defined in ioFTPD's DataCopy.h.

    timeOut - Time-out interval, in milliseconds.

Return Value:
    ioFTPD's query result, varies between query identifiers.

--*/
DWORD
Io_ShmQuery(
    IO_SESSION *session,
    IO_MEMORY *memory,
    DWORD queryId,
    DWORD timeOut
    )
{
    assert(session != NULL);
    assert(memory  != NULL);
    DebugPrint("Io_ShmQuery: session=%p memory=%p queryId=%lu timeOut=%lu\n",
        session, memory, queryId, timeOut);

    memory->message->dwReturn     = (DWORD)-1;
    memory->message->dwIdentifier = queryId;
    PostMessage(session->messageWnd, WM_SHMEM, 0, (LPARAM)memory->remote);

    if (timeOut && memory->event != NULL) {
        if (WaitForSingleObject(memory->event, timeOut) == WAIT_TIMEOUT) {
            DebugPrint("Io_ShmQuery: Timed out (%lu)\n", GetLastError());
            return (DWORD)-1;
        }

        DebugPrint("Io_ShmQuery: Return=%lu\n", memory->message->dwReturn);
        return memory->message->dwReturn;
    }

    // No timeout or event, return value cannot be checked.
    DebugPrint("Io_ShmQuery: No event or time out!\n");
    return (DWORD)-1;
}
