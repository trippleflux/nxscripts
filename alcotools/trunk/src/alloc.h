/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Memory

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Memory allocation function prototypes.

--*/

#ifndef _ALLOC_H_
#define _ALLOC_H_

bool_t
MemInit(
    void
    );

void
MemFinalise(
    void
    );

void *
MemAlloc(
    size_t size
    );

void *
MemRealloc(
    void *memory,
    size_t size
    );

void
MemFree(
    void *memory
    );

#if (DEBUG_MEMORY == TRUE)

typedef struct MemoryRecord MemoryRecord;
struct MemoryRecord {
    MemoryRecord  *next;   // Pointer to the next record
    MemoryRecord  *prev;   // Pointer to the previous record
    const tchar_t *file;   // Pointer to a buffer containing the file name
    void          *memory; // Pointer to a block of allocated memory
    size_t        size;    // Size of the allocated block, in bytes
    int           line;    // Line number corresponding to the allocation call
};

void
MemRecordCreate(
    void *memory,
    size_t size,
    const tchar_t *file,
    int line
    );

MemoryRecord *
MemRecordGet(
    void *memory
    );

void
MemRecordDelete(
    MemoryRecord *record
    );

void *
MemDebugAlloc(
    size_t size,
    const tchar_t *file,
    int line
    );

void *
MemDebugRealloc(
    void *memory,
    size_t size,
    const tchar_t *file,
    int line
    );

void
MemDebugFree(
    void *memory,
    const tchar_t *file,
    int line
    );

//
// Map release functions to their debug equivalents.
//
#define MemAlloc(n)     MemDebugAlloc((n), TEXT(__FILE__), __LINE__)
#define MemRealloc(p,n) MemDebugRealloc((p), (n), TEXT(__FILE__), __LINE__)
#define MemFree(p)      MemDebugFree((p), TEXT(__FILE__), __LINE__)

#endif // DEBUG_MEMORY

#endif // _ALLOC_H_
