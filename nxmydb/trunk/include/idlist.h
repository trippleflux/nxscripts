/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    ID List

Author:
    neoxed (neoxed@gmail.com) Sep 15, 2007

Abstract:
    ID list declarations and functions.

*/

#ifndef IDLIST_H_INCLUDED
#define IDLIST_H_INCLUDED

typedef struct {
    INT32   *array;
    SIZE_T  count;
    SIZE_T  total;
} ID_LIST;

typedef enum {
    ID_LIST_TYPE_GROUP = 0,
    ID_LIST_TYPE_USER,
} ID_LIST_TYPE;

DWORD FCALL IdListCreate(ID_LIST *list, ID_LIST_TYPE type);
DWORD FCALL IdListDestroy(ID_LIST *list);
BOOL  FCALL IdListExists(ID_LIST *list, INT32 id);
BOOL  FCALL IdListRemove(ID_LIST *list, INT32 id);

#endif // IDLIST_H_INCLUDED
