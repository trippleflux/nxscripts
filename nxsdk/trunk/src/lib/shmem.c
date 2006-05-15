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

    Initialises a shared memory session.

Arguments:
    windowName  - Name of ioFTPD's message window.

    session     - Pointer to an IO_SESSION structure that receives the
                  message window information.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_ShmInit(
    const char *windowName,
    IO_SESSION *session
    )
{
    // Validate arguments.
    if (windowName == NULL || session == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

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
    session - Pointer to an IO_SESSION structure initialised by Io_ShmInit.

    size    - Size of the shared memory block to allocate, in bytes.

Return Values:
    If the function succeeds, the return value is a pointer to an IO_MEMORY
    structure.

    If the function fails, the return value is NULL.

Remarks:
    The allocated IO_MEMORY structure must be freed by Io_ShmFree.

--*/
IO_MEMORY *
STDCALL
Io_ShmAlloc(
    const IO_SESSION *session,
    DWORD size
    )
{
    DC_MESSAGE *message = NULL;
    IO_MEMORY *memory;
    HANDLE event;
    HANDLE mapping = NULL;

    // Validate arguments.
    if (session == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    memory = HeapAlloc(GetProcessHeap(), 0, sizeof(IO_MEMORY));
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
    HeapFree(GetProcessHeap(), 0, memory);

    return NULL;
}

/*++

Io_ShmFree

    Frees shared memory.

Arguments:
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.

Return Values:
    None.

--*/
BOOL
STDCALL
Io_ShmFree(
    IO_MEMORY *memory
    )
{
    // Validate arguments.
    if (memory == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Free objects and resources.
    UnmapViewOfFile(memory->message);

    if (memory->event != NULL) {
        CloseHandle(memory->event);
    }
    if (memory->mapping != NULL) {
        CloseHandle(memory->mapping);
    }

    // Tell ioFTPD we're finished.
    PostMessage(memory->window, WM_DATACOPY_FREE, 0, (LPARAM)memory->remote);

    return HeapFree(GetProcessHeap(), 0, memory);
}

/*++

Io_ShmQuery

    Queries ioFTPD's message window.

Arguments:
    memory  - Pointer to an IO_MEMORY structure allocated by Io_ShmAlloc.

    queryId - Query identifier; defined in ioFTPD's DataCopy.h file.

    timeOut - Time out interval, in milliseconds.

Return Values:
    If the function succeeds, the return value varies between query identifiers.

    If the function fails, the return value is "(DWORD)-1".

--*/
DWORD
STDCALL
Io_ShmQuery(
    IO_MEMORY *memory,
    DWORD queryId,
    DWORD timeOut
    )
{
    // Validate arguments.
    if (memory == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    memory->message->dwReturn     = (DWORD)-1;
    memory->message->dwIdentifier = queryId;
    PostMessage(memory->window, WM_SHMEM, 0, (LPARAM)memory->remote);

    if (timeOut && memory->event != NULL) {
        if (WaitForSingleObject(memory->event, timeOut) == WAIT_TIMEOUT) {
            return (DWORD)-1;
        }
        return memory->message->dwReturn;
    }

    // No timeout or event, return value cannot be checked.
    return (DWORD)-1;
}
