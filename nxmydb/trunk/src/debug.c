/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Miscellaneous debugging utilities.

*/

#include "mydb.h"

#ifdef DEBUG
/*++

LogDebuggerHeader

    Sends the header to a debugger.

Arguments:
    None.

Return Values:
    None.

--*/
void
LogDebuggerHeader(
    void
    )
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

LogDebuggerFormat

    Sends the message to a debugger.

Arguments:
    funct   - Pointer to a null-terminated string that specifies the function.

    format  - Pointer to a null-terminated printf-style format string.

    ...     - Arguments to insert into "format".

Return Values:
    None.

--*/
void
LogDebuggerFormat(
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

    ASSERT(funct != NULL);
    ASSERT(format != NULL);

    // Preserve system error code
    error = GetLastError();

    StringCchPrintfExA(output, ARRAYSIZE(output), &end, &remaining, 0,
        "| %4d | %15s | ", GetCurrentThreadId(), funct);
    va_start(argList, format);
    StringCchVPrintfA(end, remaining, format, argList);
    va_end(argList);

    OutputDebugStringA(output);

    // Restore system error code
    SetLastError(error);
}

/*++

LogDebuggerFooter

    Sends the footer to a debugger.

Arguments:
    None.

Return Values:
    None.

--*/
void
LogDebuggerFooter(
    void
    )
{
    // Preserve system error code
    DWORD error = GetLastError();

    OutputDebugStringA("`--------------------------------------------------------------------\n");

    // Restore system error code
    SetLastError(error);
}
#endif // DEBUG


#ifdef DEBUG

/*++

LogFile

    Writes the text to a log file.

Arguments:
    funct   - Pointer to a null-terminated string that specifies the text to be written.

Return Values:
    None.

--*/
void
LogFile(
    char *text
    )
{
    HANDLE file;

    ASSERT(text != NULL);

    file = CreateFileA("nxMyDB.log", GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);

    if (file != INVALID_HANDLE_VALUE) {
        // Append text to the end of the file
        SetFilePointer(file, 0, NULL, FILE_END);
        WriteFile(file, text, strlen(text), NULL, NULL);
        CloseHandle(file);
    }
}

/*++

LogFileHeader

    Writes the header to a log file.

Arguments:
    None.

Return Values:
    None.

--*/
void
LogFileHeader(
    void
    )
{
    // Preserve system error code
    DWORD error = GetLastError();

    LogFile("\n"
            ".-----------------------------------------------------------------------------------------.\n"
            "|     Time Stamp      | ThID |    Function     |               Debug Message              |\n"
            "|-----------------------------------------------------------------------------------------'\n");

    // Restore system error code
    SetLastError(error);
}

/*++

LogFileFormat

    Writes the message to a log file.

Arguments:
    funct   - Pointer to a null-terminated string that specifies the function.

    format  - Pointer to a null-terminated printf-style format string.

    ...     - Arguments to insert into "format".

Return Values:
    None.

--*/
void
LogFileFormat(
    const char *funct,
    const char *format,
    ...
    )
{
    char *end;
    char output[1024];
    DWORD error;
    size_t remaining;
    SYSTEMTIME now;
    va_list argList;

    ASSERT(funct != NULL);
    ASSERT(format != NULL);

    // Preserve system error code
    error = GetLastError();

    GetSystemTime(&now);
    StringCchPrintfExA(output, ARRAYSIZE(output), &end, &remaining, 0,
        "| %04d-%02d-%02d %02d:%02d:%02d | %4d | %15s | ",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
        GetCurrentThreadId(), funct);

    va_start(argList, format);
    StringCchVPrintfA(end, remaining, format, argList);
    va_end(argList);

    LogFile(output);

    // Restore system error code
    SetLastError(error);
}

/*++

LogFileFooter

    Writes the footer to a log file.

Arguments:
    None.

Return Values:
    None.

--*/
void
LogFileFooter(
    void
    )
{
    // Preserve system error code
    DWORD error = GetLastError();

    LogFile("`------------------------------------------------------------------------------------------\n");

    // Restore system error code
    SetLastError(error);
}
#endif // DEBUG
