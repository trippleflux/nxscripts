/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

Module Name:
    Debugging

Author:
    neoxed (neoxed@gmail.com) Aug 3, 2009

Abstract:
    Debugging utility functions.

*/

#include <base.h>
#include <debug.h>

#ifdef DEBUG

/*++

CriticalSectionIsOwned

    Determines if the critical section is owned.

Arguments:
    critSection - Pointer to the critical section object.

Return Values:
    If the critical section is owned, the return value is nonzero (true).

    If the critical section is not owned, the return value is zero (false).

Remarks:
    http://msdn.microsoft.com/en-us/magazine/cc164040.aspx

--*/
BOOL FCALL CriticalSectionIsOwned(CRITICAL_SECTION *critSection)
{
    ASSERT(critSection != NULL);

    if (critSection->LockCount != -1) {
        return TRUE;
    }

    return FALSE;
}

/*++

CriticalSectionIsOwner

    Determines if the current thread owns a critical section.

Arguments:
    critSection - Pointer to the critical section object.

Return Values:
    If the current thread is the owner, the return value is nonzero (true).

    If the current thread is not the owner, the return value is zero (false).

Remarks:
    http://msdn.microsoft.com/en-us/magazine/cc164040.aspx

--*/
BOOL FCALL CriticalSectionIsOwner(CRITICAL_SECTION *critSection)
{
    ASSERT(critSection != NULL);

    //
    // Members of CRITICAL_SECTION that concern us:
    //
    // LockCount - This is the most important field in a critical section. It
    // is initialized to a value of -1; a value of 0 or greater indicates that
    // the critical section is held or owned. When it's not equal to -1, the
    // OwningThread field contains the thread ID that owns this critical section.
    //
    // OwningThread - This field contains the thread identifier for the thread
    // that currently holds the critical section. This is same thread ID that APIs
    // like GetCurrentThreadId return (this field is incorrectly defined in WINNT.H,
    // it should be a DWORD instead of a HANDLE).
    //

    if (critSection->LockCount != -1 && GetCurrentThreadId() == (DWORD)critSection->OwningThread) {
        return TRUE;
    }

    return FALSE;
}

/*++

WaitForDebugger

    Block until a debugger is attached to the current process.

Arguments:
    None.

Return Values:
    None.

--*/
VOID FCALL WaitForDebugger(VOID)
{
    while (!IsDebuggerPresent()) {
        TRACE("Waiting for debugger to attach...");
        Sleep(250);
    }

    TRACE("Debugger attached, continuing execution.");
}

#endif // DEBUG
