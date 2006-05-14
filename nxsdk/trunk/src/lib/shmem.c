/*++

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Shared Memory

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Shared memory functions.

--*/

#include "lib.h"

/*++

Io_ShmInit

    Initialise a shared memory session.

Arguments:
    windowName  - Name of ioFTPD's message window.

    session     - Pointer to the IO_SESSION structure to be initialised.

Return Value:
    A standard boolean result.

--*/
BOOL
STDCALL
Io_ShmInit(
    const char *windowName,
    IO_SESSION *session
    )
{
    assert(windowName != NULL);
    assert(session    != NULL);
    DebugPrint("Io_ShmInit: windowName=%s session=%p\n", windowName, session);

    session->window = FindWindowA(windowName, NULL);
    if (session->window == NULL) {
        session->currentProcId = 0;
        session->remoteProcId  = 0;
        return FALSE;
    }

    session->currentProcId = GetCurrentProcessId();
    GetWindowThreadProcessId(session->window, &session->remoteProcId);
    return TRUE;
}

/*++

Io_ShmAlloc

    Allocates shared memory.

Arguments:
    session - Pointer to an initialised IO_SESSION structure.

    size    - Size of the shared memory block to be allocated, in bytes.

Remarks:
    The allocated IO_MEMORY structure must be freed by Io_ShmFree.

Return Value:
    If the function succeeds, the return value is a pointer to a IO_MEMORY
    structure. If the function fails, the return value is NULL.

--*/
IO_MEMORY *
STDCALL
Io_ShmAlloc(
    IO_SESSION *session,
    DWORD size
    )
{
    DC_MESSAGE *message = NULL;
    IO_MEMORY *memory;
    HANDLE event;
    HANDLE mapping = NULL;

    assert(session != NULL);
    assert(size > 0);
    DebugPrint("Io_ShmAlloc: session=%p size=%lu\n", session, size);

    memory = malloc(sizeof(IO_MEMORY));
    if (memory == NULL) {
        return NULL;
    }

    event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (event != NULL) {
        size += sizeof(DC_MESSAGE);

        // Allocate memory in local process.
        mapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
            PAGE_READWRITE|SEC_COMMIT, 0, size, NULL);

        if (mapping != NULL) {
            message = (DC_MESSAGE *)MapViewOfFile(mapping,
                FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, size);

            if (message != NULL) {
                void *remote;

                // Initialise data-copy message structure.
                message->hEvent       = event;
                message->hObject      = NULL;
                message->lpMemoryBase = message;
                message->lpContext    = &message[1];

                SetLastError(ERROR_SUCCESS);
                remote = (void *)SendMessage(session->window, WM_DATACOPY_FILEMAP,
                    (WPARAM)session->currentProcId, (LPARAM)mapping);

                if (remote != NULL) {
                    // Populate memory allocation structure.
                    memory->block   = &message[1];
                    memory->message = message;
                    memory->remote  = remote;
                    memory->event   = event;
                    memory->mapping = mapping;
                    memory->window  = session->window;
                    memory->procId  = session->currentProcId;
                    memory->size    = size - sizeof(DC_MESSAGE);
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
    if (mapping != NULL) {
        CloseHandle(mapping);
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
    memory  - Pointer to an allocated IO_MEMORY structure.

Return Value:
    None.

--*/
void
STDCALL
Io_ShmFree(
    IO_MEMORY *memory
    )
{
    assert(memory != NULL);
    DebugPrint("Io_ShmFree: memory=%p\n", memory);

    // Free objects and resources.
    UnmapViewOfFile(memory->message);

    if (memory->event != NULL) {
        CloseHandle(memory->event);
    }
    if (memory->mapping != NULL) {
        CloseHandle(memory->mapping);
    }

    PostMessage(memory->window, WM_DATACOPY_FREE, 0, (LPARAM)memory->remote);
    free(memory);
}

/*++

Io_ShmQuery

    Queries the ioFTPD daemon.

Arguments:
    memory  - Pointer to an allocated IO_MEMORY structure.

    queryId - Query identifier, defined in ioFTPD's DataCopy.h.

    timeOut - Time-out interval, in milliseconds.

Return Value:
    ioFTPD's query result, varies between query identifiers.

--*/
DWORD
STDCALL
Io_ShmQuery(
    IO_MEMORY *memory,
    DWORD queryId,
    DWORD timeOut
    )
{
    assert(memory != NULL);
    DebugPrint("Io_ShmQuery: memory=%p queryId=%lu timeOut=%lu\n", memory, queryId, timeOut);

    memory->message->dwReturn     = (DWORD)-1;
    memory->message->dwIdentifier = queryId;
    PostMessage(memory->window, WM_SHMEM, 0, (LPARAM)memory->remote);

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
