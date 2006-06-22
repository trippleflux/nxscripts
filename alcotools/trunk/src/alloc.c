/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Memory

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Memory allocation functions.

--*/

#include "alcoholicz.h"

#ifdef WINDOWS
// Heap memory handle, created by MemInit().
static HANDLE memHeap = NULL;
#endif

#if (DEBUG_MEMORY == TRUE)
static MemoryRecord *recordHead = NULL; // Memory record list head.
static size_t allocatedCurrent  = 0;    // Bytes allocated currently.
static size_t allocatedPeak     = 0;    // Peak of allocated bytes.
static size_t allocatedTotal    = 0;    // Bytes allocated in total.
static size_t totalAllocs       = 0;    // Number of successful allocs.
static size_t totalFrees        = 0;    // Number of successful frees.
static size_t totalReallocs     = 0;    // Number of successful reallocs.
#endif // DEBUG_MEMORY


/*++

MemInit

    Initialise the memory subsystem.

Arguments:
    None.

Return Value:
    If the function succeeds, the return value is nonzero. If the function
    fails, the return value is zero. To get extended error information, call
    GetSystemErrorMessage.

--*/
bool_t
MemInit(
    void
    )
{
#ifdef WINDOWS
    // The memory subsystem should be initialised once.
    ASSERT(memHeap == NULL);

#if (SEPARATE_HEAP == TRUE)
    memHeap = HeapCreate(HEAP_NO_SERIALIZE, 256 * 1024, 0);
#else
    memHeap = GetProcessHeap();
#endif // SEPARATE_HEAP

    return (memHeap != NULL) ? TRUE : FALSE;
#else
    return TRUE;
#endif // WINDOWS
}

/*++

MemFinalise

    Finalise the memory subsystem. If the program was compiled with memory
    debugging, allocation statistics and any memory leaks are logged.

Arguments:
    None.

Return Value:
    If the function succeeds, the return value is nonzero. If the function
    fails, the return value is zero. To get extended error information, call
    GetSystemErrorMessage.

--*/
void
MemFinalise(
    void
    )
{
#ifdef WINDOWS
    ASSERT(memHeap != NULL);
#endif // WINDOWS

#if (DEBUG_MEMORY == TRUE)
    if (recordHead != NULL) {
        MemoryRecord *recordNext;

        ERROR(TEXT(".--------------------------------------------------------------------.\n"));
        ERROR(TEXT("|                        Memory Leak Detected                        |\n"));
        ERROR(TEXT("|--------------------------------------------------------------------|\n"));
        ERROR(TEXT("| Source File              |  Line  |  Bytes  | Base Address         |\n"));
        ERROR(TEXT("|--------------------------------------------------------------------|\n"));

        while (recordHead != NULL) {
            recordNext = recordHead->next;

            ERROR(TEXT("| %-24s | %6d | %7lu | 0x%-18p |\n"),
                recordHead->file, recordHead->line, recordHead->size, recordHead->memory);
#ifdef WINDOWS
            HeapFree(memHeap, 0, recordHead->memory);
            HeapFree(memHeap, 0, recordHead);
#else
            free(recordHead->memory);
            free(recordHead);
#endif // WINDOWS
            recordHead = recordNext;
        }
        ERROR(TEXT("`--------------------------------------------------------------------'\n"));
    }

    VERBOSE(TEXT("Currently allocated: %lu bytes\n"), allocatedCurrent);
    VERBOSE(TEXT("Peak allocated     : %lu bytes\n"), allocatedPeak);
    VERBOSE(TEXT("Total allocated    : %lu bytes\n"), allocatedTotal);
    VERBOSE(TEXT("Total allocations  : %lu\n"), totalAllocs);
    VERBOSE(TEXT("Total frees        : %lu\n"), totalFrees);
    VERBOSE(TEXT("Total reallocations: %lu\n"), totalReallocs);
#endif // DEBUG_MEMORY

#ifdef WINDOWS

#if (SEPARATE_HEAP == TRUE)
    if (!HeapDestroy(memHeap)) {
        ERROR(TEXT("Unable to destroy heap: %s\n"), GetSystemErrorMessage());
    }
#endif // SEPARATE_HEAP

    memHeap = NULL;
#endif // WINDOWS
}

#if (DEBUG_MEMORY == TRUE)

/*++

MemRecordCreate

    Creates a memory record in the internal allocation list.

Arguments:
    memory  - Pointer to a block of memory.

    size    - Size of the memory block, in bytes.

    file    - Pointer to a string representing the source file the memory
              block was allocated from. This argument can be NULL.

    line    - Line number, within the source file, the memory block was allocated.

Return Value:
    None.

--*/
void
MemRecordCreate(
    void *memory,
    size_t size,
    const tchar_t *file,
    int line
    )
{
    MemoryRecord *record;

    ASSERT(memory != NULL);

#ifdef WINDOWS
    ASSERT(memHeap != NULL);
    record = HeapAlloc(memHeap, 0, sizeof(MemoryRecord));
#else
    record = malloc(sizeof(MemoryRecord));
#endif
    if (record == NULL) {
        Panic(TEXT("Unable to allocate %lu bytes for memory record: %s\n"),
            sizeof(MemoryRecord), GetSystemErrorMessage());
    }

    // Insert memory allocation record at the list head.
    record->next   = recordHead;
    record->prev   = NULL;
    record->memory = memory;
    record->size   = size;
    record->file   = (file != NULL) ? file : TEXT("unknown");
    record->line   = line;

    if (recordHead != NULL) {
        recordHead->prev = record;
    }
    recordHead = record;
}

/*++

MemRecordGet

    Retrieves a memory record from the internal allocation list.

Arguments:
    memory  - Pointer to a block of memory.

Return Value:
    If the function succeeds, the return value is a pointer to the memory
    record corresponding with the specified memory block. If the function
    fails, the return value is NULL.

Remarks:
    The block of memory must be registered by MemRecordCreate.

--*/
MemoryRecord *
MemRecordGet(
    void *memory
    )
{
    MemoryRecord *record;

    ASSERT(memory != NULL);

    for (record = recordHead; record != NULL; record = record->next) {
        if (record->memory == memory) {
            return record;
        }
    }
    return NULL;
}

/*++

MemRecordDelete

    Removes a memory record from the internal allocation list.

Arguments:
    record  - Pointer to the memory record to be removed.

Return Value:
    None.

--*/
void
MemRecordDelete(
    MemoryRecord *record
    )
{
    ASSERT(record != NULL);

    if (record->next != NULL) {
        record->next->prev = record->prev;
    }
    if (record->prev != NULL) {
        record->prev->next = record->next;
    }
    if (recordHead == record) {
        recordHead = record->next;
    }

#ifdef WINDOWS
    ASSERT(memHeap != NULL);
    HeapFree(memHeap, 0, record);
#else
    free(record);
#endif
}

/*++

MemDebugAlloc

    Allocates a block of memory. This function keeps an internal record of the
    allocation, which aids in detecting memory leaks.

Arguments:
    size    - Number of bytes to be allocated.

    file    - Pointer to a string representing the source file this function
              was called from.

    line    - Line number, within the source file, this function was called.

Return Value:
    If the function succeeds, the return value is a pointer to the allocated
    memory block. If the function fails, the return value is NULL.

--*/
void *
MemDebugAlloc(
    size_t size,
    const tchar_t *file,
    int line
    )
{
    void *memory;

#ifdef WINDOWS
    ASSERT(memHeap != NULL);
    memory = HeapAlloc(memHeap, 0, size);
#else
    memory = malloc(size);
#endif
    if (memory != NULL) {
        MemRecordCreate(memory, size, file, line);

        // Update memory statistics.
        allocatedCurrent += size;
        allocatedTotal   += size;
        if (allocatedCurrent > allocatedPeak) {
            allocatedPeak = allocatedCurrent;
        }
        totalAllocs++;
    }

    return memory;
}

/*++

MemDebugRealloc

    Re-allocates a block of memory, allowing you to resize it. This function
    keeps an internal record of the allocation, which aids in detecting memory
    leaks.

Arguments:
    memory  - Pointer to the block of memory to be reallocated.

    size    - New size of the memory block, in bytes.

    file    - Pointer to a string representing the source file this function
              was called from.

    line    - Line number, within the source file, this function was called.

Return Value:
    If the function succeeds, the return value is a pointer to the reallocated
    memory block. If the function fails, the return value is NULL.

--*/
void *
MemDebugRealloc(
    void *memory,
    size_t size,
    const tchar_t *file,
    int line
    )
{
    MemoryRecord *record;

    ASSERT(memory != NULL);
    if (memory == NULL) {
        ERROR(TEXT("Attempt to reallocate a NULL pointer: file %s, line %d.\n"), file, line);
        return MemDebugAlloc(size, file, line);
    }

    record = MemRecordGet(memory);
    if (record == NULL) {
        //
        // Prevent users from mixing memory blocks allocated by other
        // functions (i.e. LocalAlloc, GlobalAlloc, or malloc).
        //
        Panic(TEXT("Unknown memory block %p: file %s, line %d.\n"), memory, file, line);
    }

#ifdef WINDOWS
    ASSERT(memHeap != NULL);
    memory = HeapReAlloc(memHeap, 0, memory, size);
#else
    memory = realloc(memory, size);
#endif
    if (memory == NULL) {
        // Remove memory record since the allocation failed.
        MemRecordDelete(record);
    } else {
        // Update memory statistics.
        allocatedCurrent += (size - record->size);
        allocatedTotal   += (size - record->size);
        if (allocatedCurrent > allocatedPeak) {
            allocatedPeak = allocatedCurrent;
        }
        totalReallocs++;

        // Update record.
        record->memory = memory;
        record->size   = size;
        record->file   = file;
        record->line   = line;
    }

    return memory;
}

/*++

MemDebugFree

    Frees a block of memory allocated by MemDebugAlloc or MemDebugRealloc.

Arguments:
    memory  - Pointer to the memory block to be freed.

    file    - Pointer to a string representing the source file this function
              was called from.

    line    - Line number, within the source file, this function was called.

Return Value:
    None.

Remarks:
    Attempting to free a block of memory that was not allocated by one of
    the four functions mentioned above will cause the application to panic.

--*/
void
MemDebugFree(
    void *memory,
    const tchar_t *file,
    int line
    )
{
    MemoryRecord *record;

    ASSERT(memory != NULL);
    if (memory == NULL) {
        ERROR(TEXT("Attempt to free a NULL pointer: file %s, line %d.\n"), file, line);
        return;
    }

    record = MemRecordGet(memory);
    if (record == NULL) {
        //
        // Prevent users from mixing memory blocks allocated by other
        // functions (i.e. LocalAlloc, GlobalAlloc, or malloc).
        //
        Panic(TEXT("Unknown memory block %p: file %s, line %d.\n"), memory, file, line);
    }

    // Update memory statistics.
    allocatedCurrent -= record->size;
    totalFrees++;
    MemRecordDelete(record);

#ifdef WINDOWS
    ASSERT(memHeap != NULL);
    HeapFree(memHeap, 0, memory);
#else
    free(memory);
#endif
}

#else // DEBUG_MEMORY

/*++

MemAlloc

    Allocates a block of memory.

Arguments:
    size    - Number of bytes to be allocated.

Return Value:
    If the function succeeds, the return value is a pointer to the allocated
    memory block. If the function fails, the return value is NULL.

--*/
void *
MemAlloc(
    size_t size
    )
{
#ifdef WINDOWS
    ASSERT(memHeap != NULL);
    return HeapAlloc(memHeap, 0, size);
#else
    return malloc(size);
#endif
}

/*++

MemRealloc

    Re-allocates a block of memory, allowing you to resize it.

Arguments:
    memory  - Pointer to the block of memory to be reallocated.

    size    - New size of the memory block, in bytes.

Return Value:
    If the function succeeds, the return value is a pointer to the reallocated
    memory block. If the function fails, the return value is NULL.

--*/
void *
MemRealloc(
    void *memory,
    size_t size
    )
{
    ASSERT(memory != NULL);
    if (memory == NULL) {
        ERROR(TEXT("Attempt to reallocate a NULL pointer.\n"));
        return MemAlloc(size);
    }

#ifdef WINDOWS
    ASSERT(memHeap != NULL);
    return HeapReAlloc(memHeap, 0, memory, size);
#else
    return realloc(memory, size);
#endif
}

/*++

MemFree

    Frees a block of memory allocated by MemAlloc or MemRealloc.

Arguments:
    memory  - Pointer to the memory block to be freed.

Return Value:
    None.

--*/
void
MemFree(
    void *memory
    )
{
    ASSERT(memory != NULL);
    if (memory == NULL) {
        ERROR(TEXT("Attempt to free a NULL pointer.\n"));
        return;
    }

#ifdef WINDOWS
    ASSERT(memHeap != NULL);
    HeapFree(memHeap, 0, memory);
#else
    free(memory);
#endif
}

#endif // DEBUG_MEMORY
