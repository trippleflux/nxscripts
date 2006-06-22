/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Dynamic String

Author:
    neoxed (neoxed@gmail.com) Feb 25, 2006

Abstract:
    Dynamic length string function prototypes and structures.

--*/

#ifndef _DYNSTRING_H_
#define _DYNSTRING_H_

typedef struct {
    tchar_t *value;     // Pointer to a null terminated string.
    size_t  length;     // Length of string in "value", in characters (without the null).
    size_t  maxLength;  // Size of the buffer pointed to by "value", in bytes.
} DynString;

//
// Macros for accessing members of the "DynString" structure.
//

#define DynStringGet(dynStr)    ((dynStr)->value)
#define DynStringLength(dynStr) ((dynStr)->length)

//
// Functions for creating and deleting dynamic strings.
//

DynString *
DynStringCreate(
    const tchar_t *value
    );

DynString *
DynStringCreateN(
    const tchar_t *value,
    size_t length
    );

DynString *
DynStringFromFile(
    const tchar_t *path
    );

void
DynStringDestroy(
    DynString *dynStr
    );

//
// Functions for manipulating dynamic strings.
//

int
DynStringAppend(
    DynString *dynStr,
    const tchar_t *value
    );

int
DynStringAppendN(
    DynString *dynStr,
    const tchar_t *value,
    size_t length
    );

DynString *
DynStringDup(
    DynString *dynStr
    );

bool_t
DynStringEqual(
    DynString *dynStr1,
    DynString *dynStr2
    );

int
DynStringFormat(
    DynString *dynStr,
    const tchar_t *format,
    ...
    );

int
DynStringFormatV(
    DynString *dynStr,
    const tchar_t *format,
    va_list argList
    );

int
DynStringSet(
    DynString *dynStr,
    const tchar_t *value
    );

int
DynStringTruncate(
    DynString *dynStr,
    size_t length
    );

#endif // _DYNSTRING_H_
