/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2008 neoxed

Module Name:
    Pool

Author:
    neoxed (neoxed@gmail.com) Jun 14, 2006

Abstract:
    Resource pool declarations.

*/

#include <condvar.h>
#include <queue.h>

#ifndef POOL_H_INCLUDED
#define POOL_H_INCLUDED

/*++

POOL_CONSTRUCTOR_PROC

    Creates a resource.

Arguments:
    context - Opaque context passed to <PoolCreate>.

    data    - Opaque data set by this callback.

Return Values:
    If the resource was created successfully, the return must be nonzero (true).

    If the resource was not created, the return must be zero (false).

Remarks:
    The system error code must be set on failure.

--*/
typedef BOOL (FCALL POOL_CONSTRUCTOR_PROC)(VOID *context, VOID **data);

/*++

POOL_VALIDATOR_PROC

    Validates a resource.

Arguments:
    context - Opaque context passed to <PoolCreate>.

    data    - Opaque data set by the constructor callback.

Return Values:
    If the resource is valid, the return must be nonzero (true).

    If the resource is invalid, the return must be zero (false).

Remarks:
    The system error code must be set if invalid.

--*/
typedef BOOL (FCALL POOL_VALIDATOR_PROC)(VOID *context, VOID *data);

/*++

POOL_DESTRUCTOR_PROC

    Destroys a resource.

Arguments:
    context - Opaque context passed to <PoolCreate>.

    data    - Opaque data set by the constructor callback.

Return Values:
    None.

--*/
typedef VOID (FCALL POOL_DESTRUCTOR_PROC)(VOID *context, VOID *data);

//
// Pool resource
//

struct POOL_RESOURCE {
    VOID *data;                         // Opaque data set by the constructor callback
    TAILQ_ENTRY(POOL_RESOURCE) link;    // Link to the previous and next resources
};
typedef struct POOL_RESOURCE POOL_RESOURCE;

//
// Pool structure
//

TAILQ_HEAD(POOL_TAIL_QUEUE, POOL_RESOURCE);
typedef struct POOL_TAIL_QUEUE POOL_TAIL_QUEUE;

typedef struct {
    DWORD                 idle;         // Number of idle resources
    DWORD                 total;        // Total number of resources
    DWORD                 minimum;      // Minimum number of resources to have available
    DWORD                 average;      // Average number of resources to have available
    DWORD                 maximum;      // Maximum number of resources to have available
    DWORD                 timeout;      // Milliseconds to wait for a resource to become available
    POOL_CONSTRUCTOR_PROC *constructor; // Procedure called when a resource is created
    POOL_VALIDATOR_PROC   *validator;   // Procedure called when a resource requires validation
    POOL_DESTRUCTOR_PROC  *destructor;  // Procedure called when a resource is destroyed
    VOID                  *context;     // Opaque argument passed to the constructor and destructor
    POOL_TAIL_QUEUE       resQueue;     // Queue of resources
    POOL_TAIL_QUEUE       conQueue;     // Queue of containers
    CONDITION_VAR         condition;    // Condition signaled when a used resource becomes available
    CRITICAL_SECTION      lock;         // Synchronize access to the pool structure
} POOL;

//
// Pool functions
//

DWORD SCALL PoolCreate(
    POOL *pool,
    DWORD minimum,
    DWORD average,
    DWORD maximum,
    DWORD timeout,
    POOL_CONSTRUCTOR_PROC *constructor,
    POOL_VALIDATOR_PROC *validator,
    POOL_DESTRUCTOR_PROC *destructor,
    VOID *context
    );

DWORD FCALL PoolDestroy(POOL *pool);

BOOL FCALL PoolAcquire(POOL *pool, VOID **data);
BOOL FCALL PoolRelease(POOL *pool, VOID *data);

BOOL FCALL PoolValidate(POOL *pool, VOID *data);
VOID FCALL PoolInvalidate(POOL *pool, VOID *data);

#endif // POOL_H_INCLUDED
