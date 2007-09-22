/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Miscellaneous debugging utilities.

*/

#include <base.h>
#include <debug.h>

#ifdef DEBUG
/*++

TraceHeader

    Sends the header to a debugger.

Arguments:
    None.

Return Values:
    None.

--*/
VOID FCALL TraceHeader(VOID)
{
    // Preserve system error code
    DWORD error = GetLastError();

    OutputDebugStringA(".-------------------------------------------------------------------.\n");
    OutputDebugStringA("| ThID |    Function     |               Debug Message              |\n");
    OutputDebugStringA("|-------------------------------------------------------------------'\n");

    // Restore system error code
    SetLastError(error);
}

/*++

TraceFormat

    Sends the message to a debugger.

Arguments:
    funct   - Pointer to a null-terminated string that specifies the function.

    format  - Pointer to a null-terminated printf-style format string.

    ...     - Arguments to insert into "format".

Return Values:
    None.

--*/
VOID CCALL TraceFormat(const CHAR *funct, const CHAR *format, ...)
{
    CHAR    *end;
    CHAR    output[1024];
    DWORD   error;
    SIZE_T  remaining;
    va_list argList;

    // Preserve system error code
    error = GetLastError();

    ASSERT(funct != NULL);
    ASSERT(format != NULL);

    StringCchPrintfExA(output, ELEMENT_COUNT(output), &end, &remaining, 0,
        "| %4d | %17s | ", GetCurrentThreadId(), funct);
    va_start(argList, format);
    StringCchVPrintfA(end, remaining, format, argList);
    va_end(argList);

    OutputDebugStringA(output);

    // Restore system error code
    SetLastError(error);
}

/*++

TraceFooter

    Sends the footer to a debugger.

Arguments:
    None.

Return Values:
    None.

--*/
VOID FCALL TraceFooter(VOID)
{
    // Preserve system error code
    DWORD error = GetLastError();

    OutputDebugStringA("`--------------------------------------------------------------------\n");

    // Restore system error code
    SetLastError(error);
}
#endif // DEBUG
