/*++

AlcoExt - Alcoholicz Tcl extension.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    alcoUtil.h

Author:
    neoxed (neoxed@gmail.com) May 21, 2005

Abstract:
    Utility function definitions and macros.

--*/

#ifndef _ALCOUTIL_H_
#define _ALCOUTIL_H_

#undef ARRAYSIZE
#undef MAX
#undef MIN
#undef ROUNDUP

// ARRAYSIZE - Returns the number of entries in an array.
#define ARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))

// MAX - Returns the maximum of two numeric values.
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))

// MIN - Returns the minimum of two numeric values.
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))

// ROUNDUP - Round 'a' up to a multiple of 'b'.
#define ROUNDUP(a,b)    ((((a) + ((b) - 1)) / (b)) * (b))


Tcl_HashEntry *
GetHandleTableEntry(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Tcl_HashTable *tablePtr,
    const char *type
    );

int
PartialSwitchCompare(
    Tcl_Obj *objPtr,
    const char *switchName
    );

#endif // _ALCOUTIL_H_
