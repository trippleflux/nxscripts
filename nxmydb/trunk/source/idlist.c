/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    ID List

Author:
    neoxed (neoxed@gmail.com) Sep 15, 2007

Abstract:
    Create and search a through an array of IDs.

*/

#include <base.h>
#include <array.h>
#include <idlist.h>

static INT CompareId(const VOID *elem1, const VOID *elem2)
{
    const INT32 *id1 = elem1;
    const INT32 *id2 = elem2;

    return id1[0] - id2[0];
}

DWORD FCALL IdListCreate(ID_LIST *list, ID_LIST_TYPE type)
{
    ASSERT(list != NULL);
    ASSERT(type == ID_LIST_TYPE_GROUP || type == ID_LIST_TYPE_USER);

    //
    // Initialize the ID_LIST structure.
    //
    switch (type) {
        case ID_LIST_TYPE_GROUP:
            list->array = Io_GetGroups(&list->count);
            break;

        case ID_LIST_TYPE_USER:
            list->array = Io_GetUsers(&list->count);
            break;

        default:
            return ERROR_INVALID_PARAMETER;
    }

    if (list->array == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // The array of IDs returned by ioFTPD cannot be guarenteed to be in
    // any specific order. We must sort the IDs using a numerical sort.
    //
    list->total = list->count;
    ArraySort(list->array, list->count, sizeof(INT32), CompareId);

    return ERROR_SUCCESS;
}

DWORD FCALL IdListDestroy(ID_LIST *list)
{
    ASSERT(list != NULL);

    if (list->array != NULL) {
        Io_Free(list->array);
    }
    ZeroMemory(list, sizeof(ID_LIST));
    return ERROR_SUCCESS;
}

BOOL FCALL IdListExists(ID_LIST *list, INT32 id)
{
    INT32 *search;

    ASSERT(list != NULL);

    search = ArraySearch(&id, list->array, list->count, sizeof(INT32), CompareId);
    return (search == NULL) ? FALSE : TRUE;
}

BOOL FCALL IdListRemove(ID_LIST *list, INT32 id)
{
    BOOL result;

    ASSERT(list != NULL);

    result = ArrayDelete(&id, list->array, list->count, sizeof(INT32), CompareId);
    if (result) {
        // Decrement the element count
        --list->count;
    }

    return result;
}
