/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006 neoxed

Module Name:
    Pool

Author:
    neoxed (neoxed@gmail.com) Jun 14, 2006

Abstract:
    Resource pool functions. This implementation is based on APR resource lists,
    but rewritten to use different list, locking, and synchronization constructs.

Notes:
    Container     - An empty resource (yet to be populated).
    Idle Resource - A populated resource that is not in use.
    Used Resource - A populated resource that is in use.

*/

#include "mydb.h"

/*++

ContainerPop

    Retrieves an existing container or allocates a new one.

Arguments:
    pool    - Pointer to an initialized POOL structure.

Return Values:
    If the function succeeds, the return value is a pointer to a POOL_RESOURCE
    structure (container).

    If the function fails, the return value is null.

Remarks:
    This function assumes the queues are locked.

--*/
static
POOL_RESOURCE *
ContainerPop(
    POOL *pool
    )
{
    POOL_RESOURCE *container;

    ASSERT(pool != NULL);
    DebugPrint("ContainerPop", "pool=%p\n", pool);

    if (!TAILQ_EMPTY(&pool->conQueue)) {
        // Retrieve an existing container
        container = TAILQ_FIRST(&pool->conQueue);
        TAILQ_REMOVE(&pool->conQueue, container, link);
    } else {
        // Allocate a new container
        container = Io_Allocate(sizeof(POOL_RESOURCE));
    }

    return container;
}

/*++

ContainerPush

    Places a container at the tail of the container queue.

Arguments:
    pool      - Pointer to an initialized POOL structure.

    container - Pointer to the POOL_RESOURCE structure (container) to be placed
                on the container queue.

Return Values:
    None.

Remarks:
    This function assumes the queues are locked.

--*/
static
void
ContainerPush(
    POOL *pool,
    POOL_RESOURCE *container
    )
{
    ASSERT(pool != NULL);
    ASSERT(container != NULL);
    DebugPrint("ContainerPush", "pool=%p container=%p\n", pool, container);

    // Insert container at the tail
    TAILQ_INSERT_TAIL(&pool->conQueue, container, link);
}

/*++

ResourceCreate

    Creates a new idle resource.

Arguments:
    pool     - Pointer to an initialized POOL structure.

    resource - Pointer to a pointer that receives the POOL_RESOURCE structure (idle resource).

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

Remarks:
    This function assumes the queues are locked.

--*/
static
BOOL
ResourceCreate(
    POOL *pool,
    POOL_RESOURCE **resource
    )
{
    POOL_RESOURCE *container;

    ASSERT(pool != NULL);
    ASSERT(resource != NULL);
    DebugPrint("ResourceCreate", "pool=%p resource=%p\n", pool, resource);

    // Retrieve a container for the resource
    container = ContainerPop(pool);
    if (container == NULL) {
        return FALSE;
    }

    // Populate the container
    if (!pool->constructor(pool->opaque, &container->data)) {
        // Place container back on the container queue
        ContainerPush(pool, container);
        return FALSE;
    }
    *resource = container;

    pool->total++;
    return TRUE;
}

/*++

ResourceDestroy

    Destroys an existing idle resource.

Arguments:
    pool    - Pointer to an initialized POOL structure.

    resData - Pointer to the POOL_RESOURCE structure's "data" member.

Return Values:
    None.

Remarks:
    This function assumes the queues are locked.

--*/
static
void
ResourceDestroy(
    POOL *pool,
    void *resData
    )
{
    ASSERT(pool != NULL);
    ASSERT(resData != NULL);
    DebugPrint("ResourceDestroy", "pool=%p resData=%p\n", pool, resData);

    pool->destructor(pool->opaque, resData);
    pool->total--;
}

/*++

ResourcePop

    Retrieves and removes a resource from the head of the resource queue.

Arguments:
    pool    - Pointer to an initialized POOL structure.

Return Values:
    The return value is a pointer to a POOL_RESOURCE structure.

Remarks:
    This function assumes the queues are locked.

--*/
static
POOL_RESOURCE *
ResourcePop(
    POOL *pool
    )
{
    POOL_RESOURCE *resource;

    ASSERT(pool != NULL);
    DebugPrint("ResourcePop", "pool=%p\n", pool);

    // Remove the first resource
    resource = TAILQ_FIRST(&pool->resQueue);
    TAILQ_REMOVE(&pool->resQueue, resource, link);

    pool->idle--;
    return resource;
}

/*++

ResourcePush

    Places a resource at the tail of the resource queue.

Arguments:
    pool     - Pointer to an initialized POOL structure.

    resource - Pointer to the POOL_RESOURCE structure.

Return Values:
    None.

Remarks:
    This function assumes the queues are locked.

--*/
static
void
ResourcePush(
    POOL *pool,
    POOL_RESOURCE *resource
    )
{
    ASSERT(pool != NULL);
    ASSERT(resource != NULL);
    DebugPrint("ResourcePush", "pool=%p resource=%p\n", pool, resource);

    // Update creation time
    resource->created = GetTickCount();

    // Insert resource at the tail
    TAILQ_INSERT_TAIL(&pool->resQueue, resource, link);
    pool->idle++;
}

/*++

ResourceUpdate

    Updates the resource queue.

Arguments:
    pool     - Pointer to an initialized POOL structure.

    resource - Pointer to the POOL_RESOURCE structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

Remarks:
    This function may create or destroy expired resources.

--*/
static
BOOL
ResourceUpdate(
    POOL *pool
    )
{
    BOOL newResource = FALSE;
    DWORD currentTime;
    POOL_RESOURCE *resource;

    ASSERT(pool != NULL);
    DebugPrint("ResourceUpdate", "pool=%p\n", pool);

    EnterCriticalSection(&pool->queueLock);

    // Create more resources if we're under the minimum and maximum limits
    while (pool->idle < pool->minimum && pool->total <= pool->maximum) {
        // Create a new resource
        if (!ResourceCreate(pool, &resource)) {
            LeaveCriticalSection(&pool->queueLock);
            return FALSE;
        }

        // Add resource to the queue
        ResourcePush(pool, resource);

        // Notify waiting threads that a new resource is available
        if (!ConditionVariableSignal(&pool->availCond)) {
            LeaveCriticalSection(&pool->queueLock);
            return FALSE;
        }
        newResource = TRUE;
    }

    // If a resource was created, we're already under the maximum limit
    if (newResource) {
        LeaveCriticalSection(&pool->queueLock);
        return TRUE;
    }

    // Expire old resources, moving from head to tail
    currentTime = GetTickCount();
    while (pool->idle > pool->average && pool->idle > 0) {
        resource = TAILQ_FIRST(&pool->resQueue);

        if ((currentTime - resource->created) < pool->expiration) {
            // New resources are added to the tail of the list. So if this
            // one is too young, the following resources will be as well.
            break;
        }

        // Remove from the resource queue
        resource = ResourcePop(pool);

        // Destroy resource and add it to the container queue
        ResourceDestroy(pool, resource->data);
        ContainerPush(pool, resource);
    }

    LeaveCriticalSection(&pool->queueLock);
    return TRUE;
}

/*++

PoolInit

    Initializes a resource pool.

Arguments:
    pool        - Pointer to the POOL structure to be initialized.

    minimum     - Minimum number of resources to have available. This argument
                  must be greater than zero.

    average     - Average number of resources to have available. This argument
                  must be greater than or equal to "minimum".

    maximum     - Maximum number of resources to have available. This argument
                  must be greater than or equal to "average".

    expiration  - Milliseconds until a resource expires. This argument must be
                  greater than zero.

    timeout     - Milliseconds to wait for a resource to become available. If this
                  argument is zero, it will wait forever.

    constructor - Procedure called when a resource is created.

    destructor  - Procedure called when a resource is destroyed.

    opaque      - Opaque argument passed to the constructor and destructor. This
                  argument can be null if not required.


Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
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
    )
{
    ASSERT(pool != NULL);
    DebugPrint("PoolInit", "pool=%p constructor=%p destructor=%p opaque=%p\n",
        pool, constructor, destructor, opaque);

    if (minimum < 1 || average < minimum || maximum < average ||
            expiration < 1 || constructor == NULL || destructor == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Initialize pool structure
    pool->idle        = 0;
    pool->total       = 0;
    pool->minimum     = minimum;
    pool->average     = average;
    pool->maximum     = maximum;
    pool->expiration  = expiration;
    pool->timeout     = timeout;
    pool->constructor = constructor;
    pool->destructor  = destructor;
    pool->opaque      = opaque;

    TAILQ_INIT(&pool->resQueue);
    TAILQ_INIT(&pool->conQueue);

    if (!ConditionVariableInit(&pool->availCond)) {
        return FALSE;
    }
    InitializeCriticalSection(&pool->queueLock);

    // Update resource queues
    if (!ResourceUpdate(pool)) {
        PoolDestroy(pool);
        return FALSE;
    }

    return TRUE;
}

/*++

PoolInit

    Destroys a resource pool.

Arguments:
    pool    - Pointer to the POOL structure to be destroyed.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
void
PoolDestroy(
    POOL *pool
    )
{
    POOL_RESOURCE *resource;

    ASSERT(pool != NULL);
    DebugPrint("PoolDestroy", "pool=%p\n", pool);

    EnterCriticalSection(&pool->queueLock);

    while (pool->idle > 0) {
        // Remove from the resource queue
        resource = ResourcePop(pool);

        // Destroy resource and free the structure
        ResourceDestroy(pool, resource->data);
        Io_Free(resource);
    }

    ASSERT(pool->idle == 0);
    ASSERT(pool->total == 0);

    DeleteCriticalSection(&pool->queueLock);
    ConditionVariableDestroy(&pool->availCond);
}

/*++

PoolAcquire

    Acquires a resource from the resource queue.

Arguments:
    pool    - Pointer to an initialized POOL structure.

    data    - Pointer to a pointer to receive the opaque data set by the
              constructor callback.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

Remarks:
    This function will block until a resource becomes available.

--*/
BOOL
PoolAcquire(
    POOL *pool,
    void **data
    )
{
    BOOL result;
    POOL_RESOURCE *resource;

    ASSERT(pool != NULL);
    ASSERT(data != NULL);
    DebugPrint("PoolAcquire", "pool=%p data=%p\n", pool, data);

    EnterCriticalSection(&pool->queueLock);

    // Use idle resources, if available
    if (pool->idle > 0) {
        resource = ResourcePop(pool);
        *data = resource->data;

        // Discard container
        ContainerPush(pool, resource);

        LeaveCriticalSection(&pool->queueLock);
        return TRUE;
    }

    // If we've hit the maximum limit, block until a resource
    // becomes available or we're allowed to create one.
    while (pool->total >= pool->maximum && pool->idle <= 0) {
        if (pool->timeout) {
            if (!ConditionVariableWait(&pool->availCond, &pool->queueLock, pool->timeout)) {
                LeaveCriticalSection(&pool->queueLock);
                return FALSE;
            }
        } else {
            ConditionVariableWait(&pool->availCond, &pool->queueLock, INFINITE);
        }
    }

    // Check if there are any resources
    if (pool->idle > 0) {
        resource = ResourcePop(pool);
        *data = resource->data;
        ContainerPush(pool, resource);

        LeaveCriticalSection(&pool->queueLock);
        return TRUE;
    }

    // Create a new resource since there are no available ones
    result = ResourceCreate(pool, &resource);
    if (result) {
        *data = resource->data;
        ContainerPush(pool, resource);
    }

    LeaveCriticalSection(&pool->queueLock);
    return result;
}

/*++

PoolRelease

    Returns a resource back to the resource queue.

Arguments:
    pool    - Pointer to an initialized POOL structure.

    data    - Pointer to the data to be returned, the same "data" value
              provided by PoolAcquire().

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

--*/
BOOL
PoolRelease(
    POOL *pool,
    void *data
    )
{
    POOL_RESOURCE *container;

    ASSERT(pool != NULL);
    ASSERT(data != NULL);
    DebugPrint("PoolRelease", "pool=%p data=%p\n", pool, data);

    EnterCriticalSection(&pool->queueLock);

    // Retrieve a container for the data
    container = ContainerPop(pool);
    if (container == NULL) {
        // Destroy resource since we cannot obtain a container
        ResourceDestroy(pool, data);

        LeaveCriticalSection(&pool->queueLock);
        return FALSE;
    }

    // Add resource to the queue
    container->data = data;
    ResourcePush(pool, container);

    // Notify waiting threads that a new resource is available
    ConditionVariableSignal(&pool->availCond);

    LeaveCriticalSection(&pool->queueLock);
    return ResourceUpdate(pool);
}

/*++

PoolInvalidate

    Invalidates a resource (e.g. a database connection).

Arguments:
    pool    - Pointer to an initialized POOL structure.

    data    - Pointer to the data to be destroyed, the same "data" value
              provided by PoolAcquire().

Return Values:
    None.

--*/
void
PoolInvalidate(
    POOL *pool,
    void *data
    )
{
    ASSERT(pool != NULL);
    ASSERT(data != NULL);
    DebugPrint("PoolInvalidate", "pool=%p data=%p\n", pool, data);

    // Destroy resource
    EnterCriticalSection(&pool->queueLock);
    ResourceDestroy(pool, data);
    LeaveCriticalSection(&pool->queueLock);
}
