/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Condition Variables

Author:
    neoxed (neoxed@gmail.com) Jun 12, 2006

Abstract:
    Condition variables declarations.

*/

#ifndef _CONDVARS_H_
#define _CONDVARS_H_

//
// Condition variable structure
//

typedef struct {
    HANDLE semaphore;       // Semaphore to queue waiting threads
    volatile LONG waiting;  // Number of waiting threads
} CONDITION_VARIABLE;

//
// Condition variable functions
//

BOOL
ConditionVariableCreate(
    CONDITION_VARIABLE *cond
    );

void
ConditionVariableDestroy(
    CONDITION_VARIABLE *cond
    );

BOOL
ConditionVariableBroadcast(
    CONDITION_VARIABLE *cond
    );

BOOL
ConditionVariableSignal(
    CONDITION_VARIABLE *cond
    );

BOOL
ConditionVariableWait(
    CONDITION_VARIABLE *cond,
    CRITICAL_SECTION *critSection,
    DWORD timeout
    );

#endif // _CONDVARS_H_
