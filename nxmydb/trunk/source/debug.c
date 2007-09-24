/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Utilities

Author:
    neoxed (neoxed@gmail.com) Jun 3, 2006

Abstract:
    Miscellaneous debugging utilities.

*/

#include <base.h>
#include <debug.h>

#ifdef DEBUG

//
// Macros to implement canary value checks.
//

//
// Guard structures surrounding the client buffer.
//

typedef struct {
    SIZE_T      MagicStart;     // Starting magic value
    SIZE_T      Length;         // Length of the client buffer, in bytes
    SIZE_T      MagicEnd;       // Ending magic value
    BYTE        *Buffer[0];     // Client buffer
} GUARD_HEAD;

typedef struct {
    SIZE_T      MagicStart;     // Starting magic value
    SIZE_T      Length;         // Length of the client buffer, in bytes
    SIZE_T      MagicEnd;       // Ending magic value
} GUARD_TAIL;

#define GUARD_SIZE  (sizeof(GUARD_HEAD) + sizeof(GUARD_TAIL))

//
// Magic values for marking the start and end of the guard structure.
//

#if defined(_M_IA64) || defined(_M_X64)
#   define GUARD_MAGIC_HEAD_START   (SIZE_T)0xAAAABBBBCCCCDDDD
#   define GUARD_MAGIC_HEAD_END     (SIZE_T)0xDDDDCCCCBBBBAAAA
#   define GUARD_MAGIC_TAIL_START   (SIZE_T)0xCCCCDDDDAAAABBBB
#   define GUARD_MAGIC_TAIL_END     (SIZE_T)0xBBBBAAAADDDDCCCC
#elif defined(_M_IX86)
#   define GUARD_MAGIC_HEAD_START   (SIZE_T)0xAABBCCDD
#   define GUARD_MAGIC_HEAD_END     (SIZE_T)0xDDCCBBAA
#   define GUARD_MAGIC_TAIL_START   (SIZE_T)0xCCDDAABB
#   define GUARD_MAGIC_TAIL_END     (SIZE_T)0xBBAADDCC
#else
#   error Unsupported platform.
#endif

//
// Protecting guard values from manipulation.
//

#if 0
#   define GUARD_DECODE(Value)      ((SIZE_T)DecodePointer((VOID *)(Value)))
#   define GUARD_ENCODE(Value)      ((SIZE_T)EncodePointer((VOID *)(Value)))
#else
#   define GUARD_DECODE(Value)      ((SIZE_T)(Value) ^ (SIZE_T)GetCurrentProcessId())
#   define GUARD_ENCODE(Value)      ((SIZE_T)(Value) ^ (SIZE_T)GetCurrentProcessId())
#endif

//
// Initialize head and tail structures.
//

#define GUARD_INIT_HEAD(Head, Bytes)                                            \
{                                                                               \
    (Head)->MagicStart = GUARD_ENCODE(GUARD_MAGIC_HEAD_START);                  \
    (Head)->Length     = (Bytes);                                               \
    (Head)->MagicEnd   = GUARD_ENCODE(GUARD_MAGIC_HEAD_END);                    \
}

#define GUARD_INIT_TAIL(Tail, Bytes)                                            \
{                                                                               \
    (Tail)->MagicStart = GUARD_ENCODE(GUARD_MAGIC_TAIL_START);                  \
    (Tail)->Length     = (Bytes);                                               \
    (Tail)->MagicEnd   = GUARD_ENCODE(GUARD_MAGIC_TAIL_END);                    \
}

//
// Verify head and tail structures.
//

#define GUARD_VERIFY_HEAD(Head)                                                 \
{                                                                               \
    ASSERT(GUARD_DECODE((Head)->MagicStart) == GUARD_MAGIC_HEAD_START);         \
    ASSERT(GUARD_DECODE((Head)->MagicEnd)   == GUARD_MAGIC_HEAD_END);           \
}

#define GUARD_VERIFY_TAIL(Tail)                                                 \
{                                                                               \
    ASSERT(GUARD_DECODE((Tail)->MagicStart) == GUARD_MAGIC_TAIL_START);         \
    ASSERT(GUARD_DECODE((Tail)->MagicEnd)   == GUARD_MAGIC_TAIL_END);           \
}

#define GUARD_VERIFY_LENGTH(Head, Tail)                                         \
{                                                                               \
    ASSERT((Head)->Length == (Tail)->Length);                                   \
}

//
// Resolving guard structures to and from a client buffer.
//

#define GUARD_CLIENT_TO_HEAD(Client) ((VOID *)(((BYTE *)(Client)) - sizeof(GUARD_HEAD)))
#define GUARD_HEAD_TO_CLIENT(Head)   ((VOID *)(Head)->Buffer)

#define GUARD_HEAD_TO_TAIL(Head)     ((VOID *)(((BYTE *)(Head)->Buffer) + (Head)->Length))
#define GUARD_TAIL_TO_HEAD(Tail)     ((VOID *)(((BYTE *)(Tail)) - (sizeof(GUARD_HEAD) + (Tail)->Length)))

/*++

DebugAllocate

    Allocates a block of memory from the process heap.

Arguments:
    None.

Return Values:
    None.

--*/
VOID *SCALL DebugAllocate(SIZE_T Bytes, const CHAR *SourceFile, const CHAR *SourceFunc, INT SourceLine)
{
    GUARD_HEAD *head;
    GUARD_TAIL *tail;
    VOID *mem;

    ASSERT(SourceFile != NULL);
    ASSERT(SourceFunc != NULL);
    ASSERT(SourceLine > 0);

    // Allocate the memory block
    mem = HeapAlloc(GetProcessHeap(), 0, Bytes + GUARD_SIZE);

    if (mem == NULL) {
        TRACE("HeapAlloc() failed with %lu", GetLastError());
    } else {
        // Initialze the guard values
        head = mem;
        GUARD_INIT_HEAD(head, Bytes);

        tail = GUARD_HEAD_TO_TAIL(head);
        GUARD_INIT_TAIL(tail, Bytes);

        // Update pointer to the client buffer
        mem = GUARD_HEAD_TO_CLIENT(head);
    }

    return mem;
}

/*++

DebugReallocate

    Reallocates a block of memory from the process heap.

Arguments:
    None.

Return Values:
    None.

--*/
VOID *SCALL DebugReallocate(VOID *Mem, SIZE_T Bytes, const CHAR *SourceFile, const CHAR *SourceFunc, INT SourceLine)
{
    GUARD_HEAD *head;
    GUARD_TAIL *tail;
    VOID *newMem;

    ASSERT(SourceFile != NULL);
    ASSERT(SourceFunc != NULL);
    ASSERT(SourceLine > 0);

    if (Mem == NULL) {
        return DebugAllocate(Bytes, SourceFile, SourceFunc, SourceLine);
    }

    // Verify guard structures and magic values
    head = GUARD_CLIENT_TO_HEAD(Mem);
    GUARD_VERIFY_HEAD(head);

    tail = GUARD_HEAD_TO_TAIL(head);
    GUARD_VERIFY_TAIL(tail);

    GUARD_VERIFY_LENGTH(head, tail);

    // Update pointer to the head structure
    Mem = head;

    // Resize the memory block
    newMem = HeapReAlloc(GetProcessHeap(), 0, Mem, Bytes + GUARD_SIZE);

    if (newMem == NULL) {
        TRACE("HeapReAlloc() failed with %lu", GetLastError());
    } else {
        // Initialze the guard values
        head = newMem;
        GUARD_INIT_HEAD(head, Bytes);

        tail = GUARD_HEAD_TO_TAIL(head);
        GUARD_INIT_TAIL(tail, Bytes);

        // Update pointer to the client buffer
        newMem = GUARD_HEAD_TO_CLIENT(head);
    }

    return newMem;
}

/*++

DebugFree

    Frees a memory block allocated from the process heap.

Arguments:
    None.

Return Values:
    None.

--*/
BOOL SCALL DebugFree(VOID *Mem, const CHAR *SourceFile, const CHAR *SourceFunc, INT SourceLine)
{
    BOOL result;
    GUARD_HEAD *head;
    GUARD_TAIL *tail;

    ASSERT(SourceFile != NULL);
    ASSERT(SourceFunc != NULL);
    ASSERT(SourceLine > 0);

    if (Mem == NULL) {
        return FALSE;
    }

    // Verify guard structures and magic values
    head = GUARD_CLIENT_TO_HEAD(Mem);
    GUARD_VERIFY_HEAD(head);

    tail = GUARD_HEAD_TO_TAIL(head);
    GUARD_VERIFY_TAIL(tail);

    GUARD_VERIFY_LENGTH(head, tail);

    // Update pointer to the head structure
    Mem = head;

    // Destroy guard data and client buffer
    ZeroMemory(Mem, head->Length + GUARD_SIZE);

    // Free the memory block
    result = HeapFree(GetProcessHeap(), 0, Mem);

    if (!result) {
        TRACE("HeapFree() failed with %lu", GetLastError());
    }
    return result;
}


/*++

DebugFree

    Checks the specified a memory block for an overrun or underrun.

Arguments:
    None.

Return Values:
    None.

--*/
VOID SCALL DebugCheck(VOID *Mem, const CHAR *SourceFile, const CHAR *SourceFunc, INT SourceLine)
{
    GUARD_HEAD *head;
    GUARD_TAIL *tail;

    ASSERT(Mem != NULL);
    ASSERT(SourceFile != NULL);
    ASSERT(SourceFunc != NULL);
    ASSERT(SourceLine > 0);

    // Verify guard structures and magic values
    head = GUARD_CLIENT_TO_HEAD(Mem);
    GUARD_VERIFY_HEAD(head);

    tail = GUARD_HEAD_TO_TAIL(head);
    GUARD_VERIFY_TAIL(tail);

    GUARD_VERIFY_LENGTH(head, tail);

    // Update pointer to the head structure
    Mem = head;

    // Check the heap block
    ASSERT(HeapValidate(GetProcessHeap(), 0, Mem) == TRUE);
}


/*++

TraceHeader

    Sends the header to a debugger.

Arguments:
    None.

Return Values:
    None.

--*/
VOID FCALL TraceHeader(VOID)
{
    // Preserve system error code
    DWORD error = GetLastError();

    OutputDebugStringA(".-------------------------------------------------------------------.\n");
    OutputDebugStringA("| ThID |    Function     |               Debug Message              |\n");
    OutputDebugStringA("|-------------------------------------------------------------------'\n");

    // Restore system error code
    SetLastError(error);
}

/*++

TraceFormat

    Sends the message to a debugger.

Arguments:
    funct   - Pointer to a null-terminated string that specifies the function.

    format  - Pointer to a null-terminated printf-style format string.

    ...     - Arguments to insert into "format".

Return Values:
    None.

--*/
VOID CCALL TraceFormat(const CHAR *funct, const CHAR *format, ...)
{
    CHAR    *end;
    CHAR    output[1024];
    DWORD   error;
    size_t  remaining;
    va_list argList;

    // Preserve system error code
    error = GetLastError();

    ASSERT(funct != NULL);
    ASSERT(format != NULL);

    StringCchPrintfExA(output, ELEMENT_COUNT(output), &end, &remaining, 0,
        "| %4d | %17s | ", GetCurrentThreadId(), funct);
    va_start(argList, format);
    StringCchVPrintfA(end, remaining, format, argList);
    va_end(argList);

    OutputDebugStringA(output);

    // Restore system error code
    SetLastError(error);
}

/*++

TraceFooter

    Sends the footer to a debugger.

Arguments:
    None.

Return Values:
    None.

--*/
VOID FCALL TraceFooter(VOID)
{
    // Preserve system error code
    DWORD error = GetLastError();

    OutputDebugStringA("`--------------------------------------------------------------------\n");

    // Restore system error code
    SetLastError(error);
}
#endif // DEBUG
