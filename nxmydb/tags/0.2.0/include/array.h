/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Array

Author:
    neoxed (neoxed@gmail.com) Sep 16, 2007

Abstract:
    Array function declarations.

*/

#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

typedef INT (COMPARE_PROC)(const VOID *Element1, const VOID *Element2);

VOID *FCALL ArraySearch(const VOID *Elem, VOID *Array, SIZE_T ElemCount, SIZE_T ElemSize, COMPARE_PROC *CompProc);
VOID  FCALL ArraySort(VOID *Array, SIZE_T ElemCount, SIZE_T ElemSize, COMPARE_PROC *CompProc);
VOID  FCALL ArrayDelete(VOID *Elem, VOID *Array, SIZE_T ElemCount, SIZE_T ElemSize, COMPARE_PROC *CompProc);

VOID *FCALL ArrayPtrInsert(const VOID *Elem, VOID **Array, SIZE_T ElemCount, COMPARE_PROC *CompProc);
VOID *FCALL ArrayPtrSearch(const VOID *Elem, VOID **Array, SIZE_T ElemCount, COMPARE_PROC *CompProc);
VOID  FCALL ArrayPtrSort(VOID *Array, SIZE_T ElemCount, COMPARE_PROC *CompProc);
VOID  FCALL ArrayPtrDelete(VOID *Elem, VOID **Array, SIZE_T ElemCount, COMPARE_PROC *CompProc);

#endif // ARRAY_H_INCLUDED
