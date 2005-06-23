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

#ifndef __ALCOUTIL_H__
#define __ALCOUTIL_H__

Tcl_HashEntry *GetHandleTableEntry(Tcl_Interp *interp, Tcl_Obj *objPtr, Tcl_HashTable *tablePtr, const char *type);
int PartialSwitchCompare(Tcl_Obj *objPtr, const char *switchName);

#endif /* __ALCOUTIL_H__ */
