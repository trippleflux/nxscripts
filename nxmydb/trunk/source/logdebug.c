/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Debugger Logging

Author:
    neoxed (neoxed@gmail.com) Sep 30, 2007

Abstract:
    Logging information to an attached debugger.

*/

#include <base.h>
#include <logging.h>

DWORD SCALL LogDebuggerInit(VOID)
{
    return ERROR_SUCCESS;
}

DWORD SCALL LogDebuggerFinalize(VOID)
{
    return ERROR_SUCCESS;
}

VOID SCALL LogDebuggerFormat(const CHAR *format, ...)
{
    va_list argList;

    va_start(argList, format);
    LogDebuggerFormatV(format, argList);
    va_end(argList);
}

VOID SCALL LogDebuggerFormatV(const CHAR *format, va_list argList)
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

VOID SCALL LogDebuggerTrace(const CHAR *file, const CHAR *func, INT line, const CHAR *format, ...)
{
    va_list argList;

    va_start(argList, format);
    LogDebuggerTraceV(file, func, line, format, argList);
    va_end(argList);
}

VOID SCALL LogDebuggerTraceV(const CHAR *file, const CHAR *func, INT line, const CHAR *format, va_list argList)
{
    CHAR    message[512];
    DWORD   errorCode;
    DWORD   processId;
    DWORD   threadId;

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

    StringCchVPrintfA(message, ELEMENT_COUNT(message), format, argList);
    OutputDebugStringA(message);

    // Restore system error code
    SetLastError(errorCode);
}
