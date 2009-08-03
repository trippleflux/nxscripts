/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

Module Name:
    Debugger Logging

Author:
    neoxed (neoxed@gmail.com) Sep 30, 2007

Abstract:
    Logging information to an attached debugger.

*/

#include <base.h>
#include <logging.h>

DWORD FCALL LogDebuggerInit(VOID)
{
    // Output message header
    OutputDebugStringA(".-----------------------------------------------------------------------------------------------------.\n");
    OutputDebugStringA("|       Location       |       Function       |                        Message                        |\n");
    OutputDebugStringA("|-----------------------------------------------------------------------------------------------------|\n");

    return ERROR_SUCCESS;
}

DWORD FCALL LogDebuggerFinalize(VOID)
{
    // Output message footer
    OutputDebugStringA("`-----------------------------------------------------------------------------------------------------'\n");

    return ERROR_SUCCESS;
}

VOID CCALL LogDebuggerFormat(const CHAR *format, ...)
{
    va_list argList;

    va_start(argList, format);
    LogDebuggerFormatV(format, argList);
    va_end(argList);
}

VOID FCALL LogDebuggerFormatV(const CHAR *format, va_list argList)
{
    CHAR    message[512];
    DWORD   errorCode;

    // Preserve system error code
    errorCode = GetLastError();

    ASSERT(format != NULL);

    StringCchVPrintfA(message, ELEMENT_COUNT(message), format, argList);
    OutputDebugStringA(message);

    // Restore system error code
    SetLastError(errorCode);
}

VOID CCALL LogDebuggerTrace(const CHAR *file, const CHAR *func, INT line, const CHAR *format, ...)
{
    va_list argList;

    va_start(argList, format);
    LogDebuggerTraceV(file, func, line, format, argList);
    va_end(argList);
}

VOID SCALL LogDebuggerTraceV(const CHAR *file, const CHAR *func, INT line, const CHAR *format, va_list argList)
{
    CHAR    location[MAX_PATH];
    CHAR    message[512];
    CHAR    *messageEnd;
    DWORD   errorCode;
    DWORD   processId;
    DWORD   threadId;
    size_t  remaining;

    // Preserve system error code
    errorCode = GetLastError();

    ASSERT(file != NULL);
    ASSERT(func != NULL);
    ASSERT(format != NULL);

    processId = GetCurrentProcessId();
    threadId  = GetCurrentThreadId();

    //
    // Available trace information:
    //
    // file      - Source file
    // func      - Function name in the source file
    // line      - Line number in the source file
    // processId - Current process ID
    // threadId  - Current thread ID
    //

    StringCchPrintfA(location, ELEMENT_COUNT(location), "%s:%d", LogFileName(file), line);

    StringCchPrintfExA(message, ELEMENT_COUNT(message), &messageEnd,
        &remaining, 0, "| %-20s | %-20s | ", location, func);

    StringCchVPrintfA(messageEnd, remaining, format, argList);

    // Output message to debugger
    OutputDebugStringA(message);

    // Restore system error code
    SetLastError(errorCode);
}
