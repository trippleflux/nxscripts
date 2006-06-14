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
// Pool callbacks
//

typedef BOOL (POOL_CONSTRUCTOR_PROC)(
    void *opaque,   // Opaque argument passed to PoolInit()
    void **data     // Opaque data set by this callback
    );

typedef BOOL (POOL_DESTRUCTOR_PROC)(
    void *opaque,   // Opaque argument passed to PoolInit()
    void *data      // Opaque data set by the constructor callback
    );

//
// Pool resource
//

struct POOL_RESOURCE {
    DWORD created;  // Time when resource was created
    void *data;     // Opaque data set by the constructor callback
    TAILQ_ENTRY(POOL_RESOURCE) link;
};
typedef struct POOL_RESOURCE POOL_RESOURCE;

//
// Pool structures
//

TAILQ_HEAD(POOL_QUEUE, POOL_RESOURCE);
typedef struct POOL_QUEUE POOL_QUEUE;

typedef struct {
    DWORD                 total;        // Total number of resources
    DWORD                 idle;         // Number of available resources
    DWORD                 minimum;      // Minimum number of resources to have available
    DWORD                 average;      // Average number of resources to have available
    DWORD                 maximum;      // Maximum number of resources to have available
    DWORD                 expiration;   // Milliseconds until a resource expires
    DWORD                 timeout;      // Milliseconds to wait for an available resource
    POOL_CONSTRUCTOR_PROC *constructor; // Procedure called when a resource is created
    POOL_DESTRUCTOR_PROC  *destructor;  // Procedure called when a resource is destroyed
    void                  *opaque;      // Opaque argument passed to the constructor and destructor
    POOL_QUEUE            availQueue;   // Queue of available resources (populated containers)
    POOL_QUEUE            freeQueue;    // Queue of free resources (empty containers)
    CONDITION_VARIABLE    availCond;    // Signaled once a resource becomes available
    CRITICAL_SECTION      queueLock;    // Synchronize access to the tail queues
} POOL;

//
// Pool functions
//

#endif // _POOL_H_
