/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Miscellaneous utility functions.

*/

#include "lib.h"

/*++

Io_GetBinaryPath

    Retrieves the path to ioFTPD's executable.

Arguments:
    session     - Pointer to an initialised IO_SESSION structure.

    path        - Location to store the executable's path.

    pathLength  - Length of the buffer pointed to by path, in characters.

Return Value:
    A standard boolean result.

--*/
BOOL
Io_GetBinaryPath(
    IO_SESSION *session,
    char *path,
    DWORD pathLength
    )
{
    BOOL result = FALSE;
    DWORD needed;
    HANDLE process;
    HMODULE module;

    assert(session != NULL);
    assert(path    != NULL);

    // Open the process for querying.
    process = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE,
        session->remoteProcId);
    if (process == NULL) {
        return FALSE;
    }

    // Look-up the image path.
    ZeroMemory(path, pathLength);
    if (EnumProcessModules(process, &module, sizeof(module), &needed) &&
            GetModuleFileNameEx(process, module, path, pathLength)) {
        result = TRUE;
    }

    CloseHandle(process);
    return result;
}

/*++

Io_GetStartTime

    Retrieves the time ioFTPD was started.

Arguments:
    session     - Pointer to an initialised IO_SESSION structure.

    startTime   - Location to store the start time.

Return Value:
    A standard boolean result.

--*/
BOOL
Io_GetStartTime(
    IO_SESSION *session,
    FILETIME *startTime
    )
{
    BOOL result = FALSE;
    FILETIME dummy;
    HANDLE process;

    assert(session   != NULL);
    assert(startTime != NULL);

    // Open the process for querying.
    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, session->remoteProcId);
    if (process == NULL) {
        return FALSE;
    }

    // The lpExitTime, lpKernelTime, and lpUserTime parameters cannot be null.
    result = GetProcessTimes(process, startTime, &dummy, &dummy, &dummy);

    CloseHandle(process);
    return result;
}
