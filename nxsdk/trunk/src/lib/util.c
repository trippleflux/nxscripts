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
    session     - Pointer to an IO_SESSION structure initialised by Io_ShmInit.

    path        - Pointer to the buffer that receives the executable's path.

    pathLength  - Size of the buffer pointed to by "path", in characters.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GetBinaryPath(
    const IO_SESSION *session,
    char *path,
    DWORD pathLength
    )
{
    BOOL result = FALSE;
    DWORD needed;
    HANDLE process;
    HMODULE module;

    // Validate arguments.
    if (session == NULL || path == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    process = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,
        FALSE, session->remoteProcId);
    if (process == NULL) {
        return FALSE;
    }

    if (EnumProcessModules(process, &module, sizeof(module), &needed) &&
            GetModuleFileNameExA(process, module, path, pathLength)) {
        result = TRUE;
    }

    CloseHandle(process);
    return result;
}

/*++

Io_GetStartTime

    Retrieves the time ioFTPD was started.

Arguments:
    session     - Pointer to an IO_SESSION structure initialised by Io_ShmInit.

    startTime   - Pointer to a FILETIME structure that receives the process
                  creation time.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
STDCALL
Io_GetStartTime(
    const IO_SESSION *session,
    FILETIME *startTime
    )
{
    BOOL result = FALSE;
    FILETIME dummy;
    HANDLE process;

    // Validate arguments.
    if (session == NULL || startTime == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, session->remoteProcId);
    if (process == NULL) {
        return FALSE;
    }

    // The lpExitTime, lpKernelTime, and lpUserTime parameters cannot be null.
    result = GetProcessTimes(process, startTime, &dummy, &dummy, &dummy);

    CloseHandle(process);
    return result;
}
