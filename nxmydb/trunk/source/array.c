/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Array

Author:
    neoxed (neoxed@gmail.com) Sep 16, 2007

Abstract:
    Manipulate sorted binary arrays.

*/

#include <base.h>
#include <array.h>

VOID *FCALL ArraySearch(const VOID *Elem, VOID *Array, SIZE_T ElemCount, SIZE_T ElemSize, COMPARE_PROC *CompProc)
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

VOID FCALL ArraySort(VOID *Array, SIZE_T ElemCount, SIZE_T ElemSize, COMPARE_PROC *CompProc)
{
    ASSERT(Array != NULL);
    ASSERT(ElemSize > 0);
    ASSERT(CompProc != NULL);

    qsort(Array, ElemCount, ElemSize, CompProc);
}

BOOL FCALL ArrayDelete(const VOID *Elem, VOID *Array, SIZE_T ElemCount, SIZE_T ElemSize, COMPARE_PROC *CompProc)
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


VOID *FCALL ArrayPtrInsert(const VOID *Elem, VOID **Array, SIZE_T ElemCount, COMPARE_PROC *CompProc)
{
    INT     result;
    SIZE_T  best;
    SIZE_T  left;
    SIZE_T  shift;

    ASSERT(Elem != NULL);
    ASSERT(Array != NULL);
    ASSERT(CompProc != NULL);

    best  = 0;
    left  = ElemCount;
    shift = ElemCount;

    for (; shift != 0; left -= shift) {
        shift = left >> 1;

        result = CompProc(&Elem, &Array[shift + best]);
        if (result == 0) {
            return &Array[shift + best];
        }
        if (result > 0) {
            // Look higher
            best += (shift ? shift : 1);
        }
    }

    ElemCount -= best;
    if (ElemCount > 0) {
        // Create an open slot for the new item
        MoveMemory(&Array[best + 1], &Array[best], ElemCount * sizeof(VOID *));
    }

    Array[best] = (VOID *)Elem;
    return NULL;
}

VOID *FCALL ArrayPtrSearch(const VOID *Elem, VOID **Array, SIZE_T ElemCount, COMPARE_PROC *CompProc)
{
    INT     result;
    SIZE_T  best;
    SIZE_T  left;
    SIZE_T  shift;

    ASSERT(Elem != NULL);
    ASSERT(Array != NULL);
    ASSERT(CompProc != NULL);

    best  = 0;
    left  = ElemCount;
    shift = ElemCount;

    for (; shift != 0; left -= shift) {
        shift = left >> 1;

        result = CompProc(&Elem, &Array[shift + best]);
        if (result == 0) {
            return &Array[shift + best];
        }
        if (result > 0) {
            // Look higher
            best += (shift ? shift : 1);
        }
    }

    return NULL;
}

VOID FCALL ArrayPtrSort(VOID *Array, SIZE_T ElemCount, COMPARE_PROC *CompProc)
{
    ASSERT(Array != NULL);
    ASSERT(CompProc != NULL);

    qsort(Array, ElemCount, sizeof(VOID *), CompProc);
}

BOOL FCALL ArrayPtrDelete(const VOID *Elem, VOID **Array, SIZE_T ElemCount, COMPARE_PROC *CompProc)
{
    VOID    **search;
    SIZE_T  length;

    ASSERT(Elem != NULL);
    ASSERT(Array != NULL);
    ASSERT(CompProc != NULL);

    search = ArrayPtrSearch(Elem, Array, ElemCount, CompProc);

    if (search != NULL) {
        length = &Array[ElemCount] - &search[1];
        CopyMemory(&search[0], &search[1], length * sizeof(VOID *));
        return TRUE;
    }

    return FALSE;
}
