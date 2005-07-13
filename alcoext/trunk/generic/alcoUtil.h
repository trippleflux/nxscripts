/*
 * AlcoExt - Alcoholicz Tcl extension.
 * Copyright (c) 2005 Alcoholicz Scripting Team
 *
 * File Name:
 *   alcoUtil.h
 *
 * Author:
 *   neoxed (neoxed@gmail.com) May 21, 2005
 *
 * Abstract:
 *   Miscellanenous utility definitions.
 */

#ifndef _ALCOUTIL_H_
#define _ALCOUTIL_H_

#ifdef ARRAYSIZE
#   undef ARRAYSIZE
#endif
#ifdef ROUNDUP
#   undef ROUNDUP
#endif

/* ARRAYSIZE - Returns the number of entries in an array. */
#define ARRAYSIZE(array)    (sizeof(array) / sizeof(array[0]))

/* ROUNDUP - Round 'a' up to a multiple of 'b'. */
#define ROUNDUP(a,b)        ((((a) + ((b) - 1)) / (b)) * (b))

Tcl_HashEntry *GetHandleTableEntry(Tcl_Interp *interp, Tcl_Obj *objPtr, Tcl_HashTable *tablePtr, const char *type);
int PartialSwitchCompare(Tcl_Obj *objPtr, const char *switchName);

#endif /* _ALCOUTIL_H_ */
