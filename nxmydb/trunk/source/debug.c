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
#include <logging.h>

#ifdef DEBUG

/*++

CriticalSectionIsOwned

    Determines if the critical section is owned.

Arguments:
    critSection - Pointer to the critical section object.

Return Values:
    If the critical section is owned, the return value is nonzero (true).

    If the critical section is not owned, the return value is zero (false).

--*/
BOOL CriticalSectionIsOwned(CRITICAL_SECTION *critSection)
{
    ASSERT(critSection != NULL);

    // TODO

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

--*/
BOOL CriticalSectionIsOwner(CRITICAL_SECTION *critSection)
{
    ASSERT(critSection != NULL);

    // TODO

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
VOID WaitForDebugger(VOID)
{
    while (!IsDebuggerPresent()) {
        TRACE("Waiting for debugger to attach...");
        Sleep(250);
    }
    TRACE("Debugger attached, continuing execution.");
}

#endif // DEBUG
