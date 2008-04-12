/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2008 neoxed

Module Name:
    Name List

Author:
    neoxed (neoxed@gmail.com) Sep 15, 2007

Abstract:
    Name list function declarations.

*/

#ifndef NAMELIST_H_INCLUDED
#define NAMELIST_H_INCLUDED

typedef struct {
    INT32       id;
    CHAR        name[_MAX_NAME + 1];
} NAME_ENTRY;

typedef struct {
    NAME_ENTRY  **array;
    SIZE_T      count;
    SIZE_T      total;
} NAME_LIST;

DWORD FCALL NameListCreate(NAME_LIST *list, const CHAR *path);
DWORD FCALL NameListCreateGroups(NAME_LIST *list);
DWORD FCALL NameListCreateUsers(NAME_LIST *list);
DWORD FCALL NameListDestroy(NAME_LIST *list);

BOOL  FCALL NameListExists(NAME_LIST *list, const CHAR *name);
BOOL  FCALL NameListRemove(NAME_LIST *list, const CHAR *name);

#endif // NAMELIST_H_INCLUDED
