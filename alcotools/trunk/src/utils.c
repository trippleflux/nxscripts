/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    This module implements miscellaneous utilities.

--*/

#include "alcoholicz.h"

/*++

GetStatusMessage

    Retrieves a message explaining the specified ALCOHOLAPI status code.

Arguments:
    status  - The status code to look up.

Return Value:
    Pointer to a statically allocated buffer containing a human-readable
    message explaining the ALCOHOL status code. The contents of this buffer
    must not be modified.

--*/
const tchar_t *
GetStatusMessage(
    int status
    )
{
    static const tchar_t *messages[] = {
        TEXT("Successful."),                    // ALCOHOL_OK
        TEXT("General failure."),               // ALCOHOL_ERROR
        TEXT("Insufficient buffer size."),      // ALCOHOL_INSUFFICIENT_BUFFER
        TEXT("Insufficient system memory."),    // ALCOHOL_INSUFFICIENT_MEMORY
        TEXT("Invalid data."),                  // ALCOHOL_INVALID_DATA
        TEXT("Invalid parameter."),             // ALCOHOL_INVALID_PARAMETER
        TEXT("Unknown value.")                  // ALCOHOL_UNKNOWN
    };

    if (status >= 0 && status < ARRAYSIZE(messages)) {
        return messages[status];
    } else {
        return TEXT("Invalid status code.");
    }
}

/*++

GetSystemErrorMessage

    Retrieves a string explaining the last system error.

Arguments:
    None.

Return Value:
    Pointer to a statically allocated buffer containing a human-readable
    message explaining the last system error. The contents of this buffer
    must not be modified.

Remarks:
    This function is not thread-safe.

--*/
const char *
GetSystemErrorMessageA(
    void
    )
{
#ifdef WINDOWS
    static char message[512]; // THREADS: Static variable not thread-safe.

    if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        message,
        ARRAYSIZE(message),
        NULL) == 0) {
        StringCopyA(message, ARRAYSIZE(message), "Unknown error.");
    } else {
        size_t length;
        StringLengthA(message, ARRAYSIZE(message), &length);

        // Remove trailing CR/LF
        if (length >= 2 && message[length-2] == '\r' && message[length-1] == '\n') {
            message[length-2] = '\0';
        }
    }
    return message;
#else
    return strerror(errno);
#endif // WINDOWS
}

#ifdef UNICODE
const wchar_t *
GetSystemErrorMessageW(
    void
    )
{
#ifdef WINDOWS
    static wchar_t message[512]; // THREADS: Static variable not thread-safe.

    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        message,
        ARRAYSIZE(message),
        NULL) == 0) {
        StringCopyW(message, ARRAYSIZE(message), L"Unknown error.");
    } else {
        size_t length;
        StringLengthW(message, ARRAYSIZE(message), &length);

        // Remove trailing CR/LF
        if (length >= 2 && message[length-2] == L'\r' && message[length-1] == L'\n') {
            message[length-2] = L'\0';
        }
    }
    return message;
#else
    return wcserror(errno);
#endif // WINDOWS
}
#endif // UNICODE

/*++

Panic

    Causes the application to panic and exit. This is usually invoked if the
    application encounters an unrecoverable error and must exit immediately.

Arguments:
    format  - Pointer to a buffer containing a printf-style format string.

    ...     - Arguments to insert into 'format'.

Return Value:
    None.

--*/
void
PanicA(
    const char *format,
    ...
    )
{
    va_list argList;
    va_start(argList, format);

    vfprintf(stderr, format, argList);
#if (LOG_LEVEL > 0)
    LogFormatVA(LOG_LEVEL_FATAL, format, argList);
#endif

    va_end(argList);
    ExitProcess(3);
}

#ifdef UNICODE
void
PanicW(
    const wchar_t *format,
    ...
    )
{
    va_list argList;
    va_start(argList, format);

    vfwprintf(stderr, format, argList);
#if (LOG_LEVEL > 0)
    LogFormatVW(LOG_LEVEL_FATAL, format, argList);
#endif

    va_end(argList);
    ExitProcess(3);
}
#endif // UNICODE
