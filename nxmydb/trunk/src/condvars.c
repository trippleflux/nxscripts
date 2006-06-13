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
}

BOOL
ConditionVariableDestroy(
    CONDITION_VARIABLE *condVar
    )
{
}

BOOL
ConditionVariableBroadcast(
    CONDITION_VARIABLE *condVar
    )
{
}

BOOL
ConditionVariableSignal(
    CONDITION_VARIABLE *condVar
    )
{
}

BOOL
ConditionVariableWait(
    CONDITION_VARIABLE *condVar,
    CRITICAL_SECTION *critSection,
    DWORD timeout
    )
{
}
