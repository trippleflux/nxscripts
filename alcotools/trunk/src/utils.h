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

//
// ARRAYSIZE - Returns the number of entries in an array.
//
#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))

//
// ISEOL - Test if the given character marks the end of a line.
//
#define ISEOL(ch)       ((ch) == TEXT('\n') || (ch) == TEXT('\r'))

//
// ISSPACE - Test if the given character is a space or a tab.
//
#define ISSPACE(ch)     ((ch) == TEXT(' ') || (ch) == TEXT('\t'))

//
// MAX - Returns the maximum of two numeric values.
//
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))

//
// MIN - Returns the minimum of two numeric values.
//
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))


const tchar_t *
GetStatusMessage(
    int status
    );

const char *
GetSystemErrorMessageA(
    void
    );

#ifdef UNICODE
const wchar_t *
GetSystemErrorMessageW(
    void
    );
#endif // UNICODE

void
PanicA(
    const char *format,
    ...
    );

#ifdef UNICODE
void
PanicW(
    const wchar_t *format,
    ...
    );
#endif // UNICODE

#ifdef UNICODE
#   define GetSystemErrorMessage    GetSystemErrorMessageW
#   define Panic                    PanicW
#else
#   define GetSystemErrorMessage    GetSystemErrorMessageA
#   define Panic                    PanicA
#endif // UNICODE

#endif // _UTILS_H_
