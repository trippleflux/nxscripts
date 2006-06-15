/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Condition Variables

Author:
    neoxed (neoxed@gmail.com) Jun 12, 2006

Abstract:
    Condition variables, similar to the POSIX Pthreads specification.

*/

#include "mydb.h"

BOOL
ConditionVariableInit(
    CONDITION_VARIABLE *condVar
    )
{
    ASSERT(condVar != NULL);
    DebugPrint("CondVarInit", "condVar=%p\n", condVar);

    return TRUE;
}

BOOL
ConditionVariableDestroy(
    CONDITION_VARIABLE *condVar
    )
{
    ASSERT(condVar != NULL);
    DebugPrint("CondVarDestroy", "condVar=%p\n", condVar);

    return TRUE;
}

BOOL
ConditionVariableBroadcast(
    CONDITION_VARIABLE *condVar
    )
{
    ASSERT(condVar != NULL);
    DebugPrint("CondVarBroadcast", "condVar=%p\n", condVar);

    return TRUE;
}

BOOL
ConditionVariableSignal(
    CONDITION_VARIABLE *condVar
    )
{
    ASSERT(condVar != NULL);
    DebugPrint("CondVarSignal", "condVar=%p\n", condVar);

    return TRUE;
}

BOOL
ConditionVariableWait(
    CONDITION_VARIABLE *condVar,
    CRITICAL_SECTION *critSection,
    DWORD timeout
    )
{
    ASSERT(condVar != NULL);
    ASSERT(critSection != NULL);
    DebugPrint("CondVarSignal", "condVar=%p critSection=%p timeout=%lu\n", condVar, critSection, timeout);

    return TRUE;
}
