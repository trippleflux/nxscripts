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
} CONDITION_VAR;

//
// Condition variable functions
//

BOOL
ConditionVariableCreate(
    CONDITION_VAR *cond
    );

void
ConditionVariableDestroy(
    CONDITION_VAR *cond
    );

BOOL
ConditionVariableBroadcast(
    CONDITION_VAR *cond
    );

BOOL
ConditionVariableSignal(
    CONDITION_VAR *cond
    );

BOOL
ConditionVariableWait(
    CONDITION_VAR *cond,
    CRITICAL_SECTION *critSection,
    DWORD timeout
    );

#endif // _CONDVARS_H_
