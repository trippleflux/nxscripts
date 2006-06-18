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

BOOL
ConditionVariableInit(
    CONDITION_VARIABLE *cond
    )
{
    ASSERT(cond != NULL);
    DebugPrint("CVInit", "cond=%p\n", cond);

    cond->waiting = 0;
    cond->semaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
    return (cond->semaphore != NULL) ? TRUE : FALSE;
}

void
ConditionVariableDestroy(
    CONDITION_VARIABLE *cond
    )
{
    ASSERT(cond != NULL);
    DebugPrint("CVDestroy", "cond=%p\n", cond);

    CloseHandle(cond->semaphore);
}

BOOL
ConditionVariableBroadcast(
    CONDITION_VARIABLE *cond
    )
{
    ASSERT(cond != NULL);
    DebugPrint("CVBroadcast", "cond=%p\n", cond);

    if (cond->waiting > 0) {
        return ReleaseSemaphore(cond->semaphore, cond->waiting, NULL);
    }
    return TRUE;
}

BOOL
ConditionVariableSignal(
    CONDITION_VARIABLE *cond
    )
{
    ASSERT(cond != NULL);
    DebugPrint("CVSignal", "cond=%p\n", cond);

    if (cond->waiting > 0) {
        return ReleaseSemaphore(cond->semaphore, 1, NULL);
    }
    return TRUE;
}

BOOL
ConditionVariableWait(
    CONDITION_VARIABLE *cond,
    CRITICAL_SECTION *critSection,
    DWORD timeout
    )
{
    DWORD result;

    ASSERT(cond != NULL);
    ASSERT(critSection != NULL);
    DebugPrint("CVSignal", "cond=%p critSection=%p timeout=%lu\n", cond, critSection, timeout);

    InterlockedIncrement(&cond->waiting);
    LeaveCriticalSection(critSection);

    result = WaitForSingleObject(cond->semaphore, timeout);

    InterlockedDecrement(&cond->waiting);
    EnterCriticalSection(critSection);

    return (result == WAIT_OBJECT_0) ? TRUE : FALSE;
}
