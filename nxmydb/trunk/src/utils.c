/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Miscellaneous utility functions.

*/

#include "mydb.h"

/*++

OutputDebugger

    Sends debug output to the debugger.

Arguments:
    funct   - Pointer to a null-terminated string that specifies the function.

    format  - Pointer to a null-terminated printf-style format string.

    ...     - Arguments to insert into "format".

Return Values:
    None.

--*/
#ifdef DEBUG
void
OutputDebugger(
    const char *funct,
    const char *format,
    ...
    )
{
    char *end;
    char output[1024];
    DWORD error;
    size_t remaining;
    va_list argList;

    ASSERT(funct  != NULL);
    ASSERT(format != NULL);

    // Preserve system error code
    error = GetLastError();

    // Align function name and format arguments
    StringCchPrintfExA(output, ARRAYSIZE(output), &end, &remaining, 0, "%15s: ", funct);
    va_start(argList, format);
    StringCchVPrintfA(end, remaining, format, argList);
    va_end(argList);

    OutputDebugStringA(output);

    // Restore system error code
    SetLastError(error);
}
#endif // DEBUG

/*++

OutputFile

    Writes debug output to a file.

Arguments:
    funct   - Pointer to a null-terminated string that specifies the function.

    format  - Pointer to a null-terminated printf-style format string.

    ...     - Arguments to insert into "format".

Return Values:
    None.

--*/
#ifdef DEBUG
void
OutputFile(
    const char *funct,
    const char *format,
    ...
    )
{
    DWORD error;
    FILE *handle;
    SYSTEMTIME now;
    va_list argList;

    ASSERT(funct  != NULL);
    ASSERT(format != NULL);

    // Preserve system error code
    error = GetLastError();

    handle = fopen("nxMyDB.log", "a");
    if (handle != NULL) {
        GetSystemTime(&now);
        fprintf(handle, "%04d-%02d-%02d %02d:%02d:%02d [%15s] ",
            now.wYear, now.wMonth, now.wDay,
            now.wHour, now.wMinute, now.wSecond, funct);

        va_start(argList, format);
        vfprintf(handle, format, argList);
        va_end(argList);

        fclose(handle);
    }

    // Restore system error code
    SetLastError(error);
}
#endif // DEBUG
