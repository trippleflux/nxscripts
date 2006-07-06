/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Utility function prototypes and macros.

--*/

#ifndef _UTILS_H_
#define _UTILS_H_

#undef ASSERT

#if (DEBUG_ASSERT == TRUE)
#   include <assert.h>
#   define ASSERT       assert
#else
#   define ASSERT(expr) ((void)0)
#endif // DEBUG_ASSERT


#undef ARRAYSIZE
#undef ISEOL
#undef ISSPACE
#undef MAX
#undef MIN

// ARRAYSIZE - Returns the number of entries in an array.
#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))

// ISEOL - Test if the given character marks the end of a line.
#define ISEOL(ch)       ((ch) == '\n' || (ch) == '\r')

// ISSPACE - Test if the given character is a space or a tab.
#define ISSPACE(ch)     ((ch) == ' ' || (ch) == '\t')

// MAX - Returns the maximum of two numeric values.
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))

// MIN - Returns the minimum of two numeric values.
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))


apr_status_t
BufferFile(
    const char *path,
    apr_byte_t **buffer,
    apr_size_t *length,
    apr_pool_t *pool
    );

const char *
GetErrorMessage(
    apr_status_t status
    );

#ifdef WINDOWS
BOOL
ReadConsoleFullW(
    HANDLE console,
    void *buffer,
    DWORD charsToRead,
    DWORD *charsRead
    );

BOOL
WriteConsoleFullW(
    HANDLE console,
    const void *buffer,
    DWORD charsToWrite,
    DWORD *charsWritten
    );
#endif // WINDOWS

#endif // _UTILS_H_
