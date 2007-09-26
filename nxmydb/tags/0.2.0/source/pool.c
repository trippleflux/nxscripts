/*

nxMyDB - MySQL Database for ioFTPD
Copyright (c) 2006-2007 neoxed

Module Name:
    Pool

Author:
    neoxed (neoxed@gmail.com) Jun 14, 2006

Abstract:
    Resource pool functions. This implementation is based on APR resource lists,
    but rewritten to use different list, locking, and synchronization constructs.

    Container     - An empty resource (yet to be populated).
    Idle Resource - A populated resource that is not in use.
    Used Resource - A populated resource that is in use.

    LeaveCriticalSection() appears not to alter the system error code in free builds
    of Windows, however it could in checked builds (but who really uses those anyway :P).

*/

#include <base.h>
#include <condvar.h>
#include <pool.h>
#include <queue.h>

// Silence C4127: conditional expression is constant
#pragma warning(disable : 4127)


/*++

ContainerPush

    Places a container at the tail of the container queue.

Arguments:
    pool        - Pointer to an initialized POOL structure.

    container   - Pointer to the POOL_RESOURCE structure (container) to be placed
                  on the container queue.

Return Values:
    None.

Remarks:
    This function assumes the pool is locked.

--*/
static INLINE VOID ContainerPush(POOL *pool, POOL_RESOURCE *container)
{
    ASSERT(pool != NULL);
    ASSERT(container != NULL);

    // Insert container at the tail
    TAILQ_INSERT_TAIL(&pool->conQueue, container, link);
}

/*++

ContainerPop

    Retrieves an existing container or allocates a new one.

Arguments:
    pool    - Pointer to an initialized POOL structure.

Return Values:
    If the function succeeds, the return value is a pointer to a POOL_RESOURCE
    structure (container).

    If the function fails, the return value is null. To get extended error
    information, call GetLastError.

Remarks:
    This function assumes the pool is locked.

--*/
static INLINE POOL_RESOURCE *ContainerPop(POOL *pool)
{
    POOL_RESOURCE *container;

    ASSERT(pool != NULL);

    if (!TAILQ_EMPTY(&pool->conQueue)) {
        // Retrieve an existing container
        container = TAILQ_FIRST(&pool->conQueue);
        TAILQ_REMOVE(&pool->conQueue, container, link);
    } else {
        // Allocate a new container
        container = MemAllocate(sizeof(POOL_RESOURCE));
        if (container == NULL) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return container;
}

/*++

ResourceCreate

    Creates a new idle resource.

Arguments:
    pool    - Pointer to an initialized POOL structure.

Return Values:
    If the function succeeds, the return value is a pointer to a POOL_RESOURCE
    structure (idle resource).

    If the function fails, the return value is null. To get extended error
    information, call GetLastError.

Remarks:
    This function assumes the pool is locked.

--*/
static INLINE POOL_RESOURCE *ResourceCreate(POOL *pool)
{
    POOL_RESOURCE *container;

    ASSERT(pool != NULL);

    // Retrieve a container for the resource
    container = ContainerPop(pool);
    if (container == NULL) {
        return NULL;
    }

    // Populate the container
    if (!pool->constructor(pool->context, &container->data)) {
        ASSERT(GetLastError() != ERROR_SUCCESS);

        // Place container back in the container queue
        ContainerPush(pool, container);
        return NULL;
    }

    pool->total++;
    return container;
}

/*++

ResourceCheck

    Validates an existing idle resource.

Arguments:
    pool    - Pointer to an initialized POOL structure.

    resData - Pointer to the POOL_RESOURCE structure's "data" member.

Return Values:
    If the resource is still valid, the return value is nonzero (true).

    If the resource is no longer valid, the return value is zero (false). To get
    extended error information, call GetLastError.

Remarks:
    This function assumes the pool is locked.

--*/
static INLINE BOOL ResourceCheck(POOL *pool, VOID *resData)
{
    ASSERT(pool != NULL);
    ASSERT(resData != NULL);

    if (!pool->validator(pool->context, resData)) {
        ASSERT(GetLastError() != ERROR_SUCCESS);
        return FALSE;
    }
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
    This function assumes the pool is locked.

--*/
static INLINE VOID ResourceDestroy(POOL *pool, VOID *resData)
{
    ASSERT(pool != NULL);
    ASSERT(resData != NULL);

    pool->destructor(pool->context, resData);
    pool->total--;
}

/*++

ResourcePush

    Places a resource at the tail of the resource queue.

Arguments:
    pool        - Pointer to an initialized POOL structure.

    resource    - Pointer to the POOL_RESOURCE structure.

Return Values:
    None.

Remarks:
    This function assumes the pool is locked.

--*/
static INLINE VOID ResourcePush(POOL *pool, POOL_RESOURCE *resource)
{
    ASSERT(pool != NULL);
    ASSERT(resource != NULL);

    // Insert resource at the tail
    TAILQ_INSERT_TAIL(&pool->resQueue, resource, link);
    pool->idle++;
}

/*++

ResourcePop

    Retrieves and removes a resource from the head of the resource queue.

Arguments:
    pool    - Pointer to an initialized POOL structure.

Return Values:
    The return value is a pointer to a POOL_RESOURCE structure.

Remarks:
    This function assumes the pool is locked.

--*/
static INLINE POOL_RESOURCE *ResourcePop(POOL *pool)
{
    POOL_RESOURCE *resource;

    ASSERT(pool != NULL);

    // Remove the first resource
    resource = TAILQ_FIRST(&pool->resQueue);
    TAILQ_REMOVE(&pool->resQueue, resource, link);

    pool->idle--;
    return resource;
}

/*++

ResourcePopCheck

    Retrieves, removes, and checks a resource from the head of the resource queue.

Arguments:
    pool    - Pointer to an initialized POOL structure.

Return Values:
    If a valid resource is found, the return value is a pointer to a
    POOL_RESOURCE structure.

    If no valid resources are found, the return value is null.

Remarks:
    This function assumes the pool is locked.

--*/
static POOL_RESOURCE *ResourcePopCheck(POOL *pool)
{
    POOL_RESOURCE *resource;

    ASSERT(pool != NULL);

    // Try to find a valid idle resource
    while (pool->idle > 0) {
        resource = ResourcePop(pool);

        // Determine if the resource is valid
        if (ResourceCheck(pool, resource->data)) {
            return resource;
        }

        // Destroy invalid resource
        ResourceDestroy(pool, resource->data);
        ContainerPush(pool, resource);
    }

    // Nothing found
    return NULL;
}

/*++

ResourceUpdate

    Updates the resource queue.

Arguments:
    pool        - Pointer to an initialized POOL structure.

    resource    - Pointer to the POOL_RESOURCE structure.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false).

Remarks:
    This function may create or destroy expired resources.

--*/
static BOOL FCALL ResourceUpdate(POOL *pool)
{
    BOOL newResource = FALSE;
    POOL_RESOURCE *resource;
    POOL_RESOURCE *resourceTemp;

    ASSERT(pool != NULL);

    EnterCriticalSection(&pool->lock);

    // Create more resources if we're under the minimum and maximum limits
    while (pool->idle < pool->minimum && pool->total < pool->maximum) {
        // Create a new resource
        resource = ResourceCreate(pool);
        if (resource == NULL) {
            // Fail silently if we cannot create a resource
            break;
        }

        // Add resource to the queue
        ResourcePush(pool, resource);

        // Notify waiting threads that a new resource is available
        if (!ConditionVariableSignal(&pool->condition)) {
            LeaveCriticalSection(&pool->lock);
            return FALSE;
        }
        newResource = TRUE;
    }

    // If a resource was created, we're already under the maximum limit
    if (newResource) {
        LeaveCriticalSection(&pool->lock);
        return TRUE;
    }

    // Check if any idle resources can be removed
    if (pool->idle > 0 && pool->idle > pool->average) {
        TAILQ_FOREACH_SAFE(resource, &pool->resQueue, link, resourceTemp) {
            if (ResourceCheck(pool, resource->data)) {
                // Ignore valid resources
                continue;
            }
            ResourceDestroy(pool, resource->data);

            // The idle counter is usually decremented by ResourcePop(), but since
            // we're removing queue items on our own, we have to decrement it.
            pool->idle--;

            // Remove resource from queue and add it to the container queue
            TAILQ_REMOVE(&pool->resQueue, resource, link);
            ContainerPush(pool, resource);
        }
    }

    LeaveCriticalSection(&pool->lock);
    return TRUE;
}

/*++

PoolCreate

    Creates a resource pool.

Arguments:
    pool        - Pointer to the POOL structure to be initialized.

    minimum     - Minimum number of resources to have available. This argument
                  must be greater than zero.

    average     - Average number of resources to have available. This argument
                  must be greater than or equal to "minimum".

    maximum     - Maximum number of resources to have available. This argument
                  must be greater than or equal to "average".

    timeout     - Milliseconds to wait for a resource to become available. If this
                  argument is zero, it will wait forever.

    constructor - Procedure called when a resource is created.

    validator   - Procedure called when a resource requires validation.

    destructor  - Procedure called when a resource is destroyed.

    context     - Opaque argument passed to the callbacks. This argument can
                  be null if not required.

Return Values:
    If the function succeeds, the return value is nonzero (true).

    If the function fails, the return value is zero (false). To get extended error
    information, call GetLastError.

--*/
BOOL SCALL PoolCreate(
    POOL *pool,
    DWORD minimum,
    DWORD average,
    DWORD maximum,
    DWORD timeout,
    POOL_CONSTRUCTOR_PROC *constructor,
    POOL_VALIDATOR_PROC *validator,
    POOL_DESTRUCTOR_PROC *destructor,
    VOID *context
    )
{
    ASSERT(pool != NULL);

    if (minimum < 1 || average < minimum || maximum < average || !constructor || !validator || !destructor) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Initialize pool structure
    pool->idle        = 0;
    pool->total       = 0;
    pool->minimum     = minimum;
    pool->average     = average;
    pool->maximum     = maximum;
    pool->timeout     = (timeout == 0) ? INFINITE : timeout;
    pool->constructor = constructor;
    pool->validator   = validator;
    pool->destructor  = destructor;
    pool->context     = context;

    TAILQ_INIT(&pool->resQueue);
    TAILQ_INIT(&pool->conQueue);

    if (!InitializeCriticalSectionAndSpinCount(&pool->lock, 250)) {
        return FALSE;
    }

    if (!ConditionVariableCreate(&pool->condition)) {
        DeleteCriticalSection(&pool->lock);
        return FALSE;
    }

    // Update resource queues
    if (!ResourceUpdate(pool)) {
        PoolDestroy(pool);
        return FALSE;
    }

    return TRUE;
}

/*++

PoolDestroy

    Destroys a resource pool.

Arguments:
    pool    - Pointer to the POOL structure to be destroyed.

Return Values:
    None.

--*/
VOID FCALL PoolDestroy(POOL *pool)
{
    POOL_RESOURCE *resource;

    ASSERT(pool != NULL);

    EnterCriticalSection(&pool->lock);

    while (pool->idle > 0) {
        // Remove from the resource queue
        resource = ResourcePop(pool);

        // Destroy resource and free the structure
        ResourceDestroy(pool, resource->data);
        MemFree(resource);
    }

    ASSERT(pool->idle == 0);
    ASSERT(pool->total == 0);

    ConditionVariableDestroy(&pool->condition);
    DeleteCriticalSection(&pool->lock);
    ZeroMemory(pool, sizeof(POOL));
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

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

Remarks:
    This function will block until a resource becomes available.

--*/
BOOL FCALL PoolAcquire(POOL *pool, VOID **data)
{
    POOL_RESOURCE *resource;

    ASSERT(pool != NULL);
    ASSERT(data != NULL);

    EnterCriticalSection(&pool->lock);

    // Use idle resources, if available
    resource = ResourcePopCheck(pool);
    if (resource == NULL) {
        // If we've hit the maximum limit, block until a resource
        // becomes available or we're allowed to create one.
        while (pool->idle <= 0 && pool->total >= pool->maximum) {
            if (!ConditionVariableWait(&pool->condition, &pool->lock, pool->timeout)) {
                LeaveCriticalSection(&pool->lock);
                return FALSE;
            }
        }

        // Check if any resources became available
        resource = ResourcePopCheck(pool);
        if (resource == NULL && pool->total < pool->maximum) {

            // Create a new resource since there are no available ones
            resource = ResourceCreate(pool);
        }
    }

    if (resource != NULL) {
        *data = resource->data;
        ContainerPush(pool, resource);

        LeaveCriticalSection(&pool->lock);
        return TRUE;
    }

    LeaveCriticalSection(&pool->lock);
    return FALSE;
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

    If the function fails, the return value is zero (false). To get extended
    error information, call GetLastError.

--*/
BOOL FCALL PoolRelease(POOL *pool, VOID *data)
{
    DWORD error;
    POOL_RESOURCE *container;

    ASSERT(pool != NULL);
    ASSERT(data != NULL);

    EnterCriticalSection(&pool->lock);

    // Retrieve a container for the data
    container = ContainerPop(pool);
    if (container == NULL) {
        error = GetLastError();
        ResourceDestroy(pool, data);
        LeaveCriticalSection(&pool->lock);

        // Restore system error code in case the destructor changed it
        SetLastError(error);
        return FALSE;
    }

    // Add resource back to the queue
    container->data = data;
    ResourcePush(pool, container);

    // Notify waiting threads that a new resource is available
    ConditionVariableSignal(&pool->condition);

    LeaveCriticalSection(&pool->lock);
    return ResourceUpdate(pool);
}

/*++

PoolValidate

    Validates a resource (e.g. an old database connection).

Arguments:
    pool    - Pointer to an initialized POOL structure.

    data    - Pointer to the data to be validated, the same "data" value
              provided by PoolAcquire().

Return Values:
    If the resource is valid, the return value is nonzero (true).

    If the resource is invalid, the return value is zero (false) and the resource
    is destroyed. To get extended error information, call GetLastError.

--*/
BOOL FCALL PoolValidate(POOL *pool, VOID *data)
{
    BOOL result;

    ASSERT(pool != NULL);
    ASSERT(data != NULL);

    EnterCriticalSection(&pool->lock);

    // Validate resource
    result = ResourceCheck(pool, data);
    if (!result) {
        ResourceDestroy(pool, data);
    }

    LeaveCriticalSection(&pool->lock);
    return result;
}

/*++

PoolInvalidate

    Invalidates a resource (e.g. an expired database connection).

Arguments:
    pool    - Pointer to an initialized POOL structure.

    data    - Pointer to the data to be destroyed, the same "data" value
              provided by PoolAcquire().

Return Values:
    None.

--*/
VOID FCALL PoolInvalidate(POOL *pool, VOID *data)
{
    ASSERT(pool != NULL);
    ASSERT(data != NULL);

    EnterCriticalSection(&pool->lock);

    // Destroy resource
    ResourceDestroy(pool, data);

    LeaveCriticalSection(&pool->lock);
}
