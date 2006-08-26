/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Condition Variables

Author:
    neoxed (neoxed@gmail.com) Jun 12, 2006

Abstract:
    Condition variables, similar to those of POSIX Pthreads.

*/

#include "mydb.h"

/*++

ConditionVariableCreate

    Creates a condition variable.

Arguments:
    cond    - Pointer to the CONDITION_VARIABLE structure to be initialized.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
ConditionVariableCreate(
    CONDITION_VARIABLE *cond
    )
{
    Assert(cond != NULL);
    DebugPrint("CVInit", "cond=%p\n", cond);

    cond->waiting = 0;
    cond->semaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
    return (cond->semaphore != NULL) ? TRUE : FALSE;
}

/*++

ConditionVariableDestroy

    Destroys the given condition variable.

Arguments:
    cond    - Pointer to an initialized CONDITION_VARIABLE structure.

Return Values:
    None.

--*/
void
ConditionVariableDestroy(
    CONDITION_VARIABLE *cond
    )
{
    Assert(cond != NULL);
    DebugPrint("CVDestroy", "cond=%p\n", cond);

    CloseHandle(cond->semaphore);
}

/*++

ConditionVariableBroadcast

    Signals all threads that are waiting on the given condition variable.

Arguments:
    cond    - Pointer to an initialized CONDITION_VARIABLE structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
ConditionVariableBroadcast(
    CONDITION_VARIABLE *cond
    )
{
    Assert(cond != NULL);
    DebugPrint("CVBroadcast", "cond=%p\n", cond);

    if (cond->waiting > 0) {
        return ReleaseSemaphore(cond->semaphore, cond->waiting, NULL);
    }
    return TRUE;
}

/*++

ConditionVariableSignal

    Signals a single thread that is waiting on the given condition variable.

Arguments:
    cond    - Pointer to an initialized CONDITION_VARIABLE structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
ConditionVariableSignal(
    CONDITION_VARIABLE *cond
    )
{
    Assert(cond != NULL);
    DebugPrint("CVSignal", "cond=%p\n", cond);

    if (cond->waiting > 0) {
        return ReleaseSemaphore(cond->semaphore, 1, NULL);
    }
    return TRUE;
}

/*++

ConditionVariableWait

    Initializes a condition variable.

Arguments:
    cond        - Pointer to an initialized CONDITION_VARIABLE structure.

    critSection - Pointer to the critical section object. The caller must have
                  ownership of the critical section before calling this function.

    timeout     - The time-out interval, in milliseconds, or INFINITE.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL
ConditionVariableWait(
    CONDITION_VARIABLE *cond,
    CRITICAL_SECTION *critSection,
    DWORD timeout
    )
{
    DWORD result;

    Assert(cond != NULL);
    Assert(critSection != NULL);
    DebugPrint("CVSignal", "cond=%p critSection=%p timeout=%lu\n", cond, critSection, timeout);

    InterlockedIncrement(&cond->waiting);
    LeaveCriticalSection(critSection);

    result = WaitForSingleObject(cond->semaphore, timeout);

    InterlockedDecrement(&cond->waiting);
    EnterCriticalSection(critSection);

    return (result == WAIT_OBJECT_0) ? TRUE : FALSE;
}
