/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2009 neoxed

Module Name:
    Condition Variables

Author:
    neoxed (neoxed@gmail.com) Jun 12, 2006

Abstract:
    Condition variables declarations.

*/

#ifndef CONDVAR_H_INCLUDED
#define CONDVAR_H_INCLUDED

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

DWORD FCALL ConditionVariableCreate(CONDITION_VAR *cond);
DWORD FCALL ConditionVariableDestroy(CONDITION_VAR *cond);

BOOL FCALL ConditionVariableBroadcast(CONDITION_VAR *cond);
BOOL FCALL ConditionVariableSignal(CONDITION_VAR *cond);
BOOL FCALL ConditionVariableWait(CONDITION_VAR *cond, CRITICAL_SECTION *critSection, DWORD timeout);

#endif // CONDVAR_H_INCLUDED
