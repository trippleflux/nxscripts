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
#include <idlist.h>


typedef INT (COMPARE_PROC)(const VOID *Elem1, const VOID *Elem2);

static INLINE VOID *ArraySearch(const VOID *Elem, VOID *Array, SIZE_T ElemCount, SIZE_T ElemSize, COMPARE_PROC *CompProc)
{
    BYTE    *low  = (BYTE *)Array;
    BYTE    *high = (BYTE *)Array + (ElemCount - 1) * ElemSize;
    BYTE    *middle;
    INT     result;
    SIZE_T  half;

    ASSERT(Elem != NULL);
    ASSERT(Array != NULL);
    ASSERT(ElemSize > 0);
    ASSERT(CompProc != NULL);

    while (low <= high) {
        half = ElemCount / 2;
        if (half != 0) {
            middle = low + ((ElemCount & 1) ? half : half-1) * ElemSize;

            result = CompProc(Elem, middle);
            if (result == 0) {
                return middle;

            } else if (result < 0) {
                // Look lower
                high      = middle - ElemSize;
                ElemCount = (ElemCount & 1) ? half : half-1;

            } else {
                // Look higher
                low       = middle + ElemSize;
                ElemCount = half;
            }

        } else if (ElemCount) {
            // Only one element in the array to compare with.
            if (CompProc(Elem, low) == 0) {
                return low;
            }
            break;

        } else {
            // Nothing left, no match
            break;
        }
    }

    return NULL;
}

static INLINE VOID ArraySort(VOID *Array, SIZE_T ElemCount, SIZE_T ElemSize, COMPARE_PROC *CompProc)
{
    ASSERT(Array != NULL);
    ASSERT(ElemSize > 0);
    ASSERT(CompProc != NULL);

    qsort(Array, ElemCount, ElemSize, CompProc);
}

static INLINE BOOL ArrayDelete(const VOID *Elem, VOID *Array, SIZE_T ElemCount, SIZE_T ElemSize, COMPARE_PROC *CompProc)
{
    BYTE    *ending;
    BYTE    *middle;
    SIZE_T  length;

    ASSERT(Elem != NULL);
    ASSERT(Array != NULL);
    ASSERT(ElemSize > 0);
    ASSERT(CompProc != NULL);

    middle = ArraySearch(Elem, Array, ElemCount, ElemSize, CompProc);

    if (middle != NULL) {
        // Amount of data located after the element that needs to be copied.
        ending = (BYTE *)Array + (ElemCount * ElemSize);
        length = ending - middle - ElemSize;

        // Deletion is done by moving all data up by one position.
        CopyMemory(middle, middle + ElemSize, length);

        return TRUE;
    }

    return FALSE;
}


static INT IdCompare(const VOID *Elem1, const VOID *Elem2)
{
    const INT32 *id1 = Elem1;
    const INT32 *id2 = Elem2;

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
    ArraySort(list->array, list->count, sizeof(INT32), IdCompare);

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

    search = ArraySearch(&id, list->array, list->count, sizeof(INT32), IdCompare);
    return (search == NULL) ? FALSE : TRUE;
}

BOOL FCALL IdListRemove(ID_LIST *list, INT32 id)
{
    BOOL result;

    ASSERT(list != NULL);

    result = ArrayDelete(&id, list->array, list->count, sizeof(INT32), IdCompare);
    if (result) {
        // Decrement the element count
        --list->count;
    }

    return result;
}
