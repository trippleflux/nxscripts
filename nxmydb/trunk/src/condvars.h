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

#ifndef _CONDVARS_H_
#define _CONDVARS_H_

//
// Condition variable structure
//

typedef struct {
    void *foo;
} CONDITION_VARIABLE;

//
// Condition variable functions
//

BOOL
ConditionVariableInit(
    CONDITION_VARIABLE *condVar
    );

BOOL
ConditionVariableDestroy(
    CONDITION_VARIABLE *condVar
    );

BOOL
ConditionVariableBroadcast(
    CONDITION_VARIABLE *condVar
    );

BOOL
ConditionVariableSignal(
    CONDITION_VARIABLE *condVar
    );

BOOL
ConditionVariableWait(
    CONDITION_VARIABLE *condVar,
    CRITICAL_SECTION *critSection,
    DWORD timeout
    );

#endif // _CONDVARS_H_
