/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Pool

Author:
    neoxed (neoxed@gmail.com) Jun 14, 2006

Abstract:
    Resource pool declarations.

*/

#ifndef _POOL_H_
#define _POOL_H_

//
// Pool resource callbacks
//

typedef BOOL (POOL_CONSTRUCTOR_PROC)(
    void *opaque,   // Opaque argument passed to PoolInit()
    void **data     // Opaque data set by this callback
    );

typedef void (POOL_DESTRUCTOR_PROC)(
    void *opaque,   // Opaque argument passed to PoolInit()
    void *data      // Opaque data set by the constructor callback
    );

//
// Pool resource
//

struct POOL_RESOURCE {
    DWORD created;  // Time when the resource was created
    void *data;     // Opaque data set by the constructor callback
    TAILQ_ENTRY(POOL_RESOURCE) link;
};
typedef struct POOL_RESOURCE POOL_RESOURCE;

//
// Pool structures
//

TAILQ_HEAD(POOL_TAIL_QUEUE, POOL_RESOURCE);
typedef struct POOL_TAIL_QUEUE POOL_TAIL_QUEUE;

typedef struct {
    DWORD                 total;        // Total number of resources
    DWORD                 idle;         // Number of idle resources
    DWORD                 minimum;      // Minimum number of resources to have available
    DWORD                 average;      // Average number of resources to have available
    DWORD                 maximum;      // Maximum number of resources to have available
    DWORD                 expiration;   // Milliseconds until a resource expires
    DWORD                 timeout;      // Milliseconds to wait for a resource to become available
    POOL_CONSTRUCTOR_PROC *constructor; // Procedure called when a resource is created
    POOL_DESTRUCTOR_PROC  *destructor;  // Procedure called when a resource is destroyed
    void                  *opaque;      // Opaque argument passed to the constructor and destructor
    POOL_TAIL_QUEUE       resQueue;     // Queue of resources
    POOL_TAIL_QUEUE       conQueue;     // Queue of containers
    CONDITION_VARIABLE    availCond;    // Signaled once a used resource becomes available
    CRITICAL_SECTION      queueLock;    // Synchronize access to the tail queues
} POOL;

//
// Pool functions
//

BOOL
PoolInit(
    POOL *pool,
    DWORD minimum,
    DWORD average,
    DWORD maximum,
    DWORD expiration,
    DWORD timeout,
    POOL_CONSTRUCTOR_PROC *constructor,
    POOL_DESTRUCTOR_PROC *destructor,
    void *opaque
    );

void
PoolDestroy(
    POOL *pool
    );

BOOL
PoolAcquire(
    POOL *pool,
    void **data
    );

BOOL
PoolRelease(
    POOL *pool,
    void *data
    );

void
PoolInvalidate(
    POOL *pool,
    void *data
    );

#endif // _POOL_H_
