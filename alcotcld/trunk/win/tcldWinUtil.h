/*++

AlcoTcld - Alcoholicz Tcl daemon.
Copyright (c) 2005-2008 Alcoholicz Scripting Team

Module Name:
    Windows Utilities

Author:
    neoxed (neoxed@gmail.com) Mar 13, 2006

Abstract:
    Windows utility function declarations.

--*/

#ifndef _TCLDWINUTIL_H_
#define _TCLDWINUTIL_H_

#ifdef DEBUG
void
DebugPrint(
    const char *format,
    ...
    );
#else
#   define DebugPrint
#endif

//
// Determine if the given file exists.
//
#define FileExists(path) \
    (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)

//
// Writes a timestamp to the given file.
//
#define WriteTime(handle)                                 \
    do {                                                  \
        SYSTEMTIME now;                                   \
        GetSystemTime(&now);                              \
                                                          \
        fprintf(handle, "%04d-%02d-%02d %02d:%02d:%02d ", \
            now.wYear, now.wMonth, now.wDay,              \
            now.wHour, now.wMinute, now.wSecond);         \
    } while (0);

#endif // _TCLDWINUTIL_H_
