/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Condition Variables

Author:
    neoxed (neoxed@gmail.com) Jun 12, 2006

Abstract:
    Condition variables, similar to those of POSIX Pthreads.

*/

#include <base.h>
#include <condvar.h>

/*++

ConditionVariableCreate

    Creates a condition variable.

Arguments:
    cond    - Pointer to the CONDITION_VAR structure to be initialized.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL FCALL ConditionVariableCreate(CONDITION_VAR *cond)
{
    ASSERT(cond != NULL);

    cond->waiting = 0;
    cond->semaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
    return (cond->semaphore != NULL) ? TRUE : FALSE;
}

/*++

ConditionVariableDestroy

    Destroys the given condition variable.

Arguments:
    cond    - Pointer to an initialized CONDITION_VAR structure.

Return Values:
    None.

--*/
VOID FCALL ConditionVariableDestroy(CONDITION_VAR *cond)
{
    ASSERT(cond != NULL);

    if (cond->semaphore != NULL) {
        CloseHandle(cond->semaphore);
    }
    ZeroMemory(cond, sizeof(CONDITION_VAR));
}

/*++

ConditionVariableBroadcast

    Signals all threads that are waiting on the given condition variable.

Arguments:
    cond    - Pointer to an initialized CONDITION_VAR structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL FCALL ConditionVariableBroadcast(CONDITION_VAR *cond)
{
    ASSERT(cond != NULL);

    if (cond->waiting > 0) {
        return ReleaseSemaphore(cond->semaphore, cond->waiting, NULL);
    }
    return TRUE;
}

/*++

ConditionVariableSignal

    Signals a single thread that is waiting on the given condition variable.

Arguments:
    cond    - Pointer to an initialized CONDITION_VAR structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL FCALL ConditionVariableSignal(CONDITION_VAR *cond)
{
    ASSERT(cond != NULL);

    if (cond->waiting > 0) {
        return ReleaseSemaphore(cond->semaphore, 1, NULL);
    }
    return TRUE;
}

/*++

ConditionVariableWait

    Initializes a condition variable.

Arguments:
    cond        - Pointer to an initialized CONDITION_VAR structure.

    critSection - Pointer to the critical section object. The caller must have
                  ownership of the critical section before calling this function.

    timeout     - The time-out interval, in milliseconds, or INFINITE.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL FCALL ConditionVariableWait(CONDITION_VAR *cond, CRITICAL_SECTION *critSection, DWORD timeout)
{
    DWORD result;

    ASSERT(cond != NULL);
    ASSERT(critSection != NULL);

    InterlockedIncrement(&cond->waiting);
    LeaveCriticalSection(critSection);

    result = WaitForSingleObject(cond->semaphore, timeout);

    InterlockedDecrement(&cond->waiting);
    EnterCriticalSection(critSection);

    return (result == WAIT_OBJECT_0) ? TRUE : FALSE;
}
