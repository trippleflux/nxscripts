/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Dynamic String

Author:
    neoxed (neoxed@gmail.com) Feb 25, 2006

Abstract:
    Dynamic length string functions.

--*/

#include "alcoholicz.h"

/*++

DsCreate

    Creates an empty dynamic string.

Arguments:
    dynStr  - Pointer to an unused dynamic string structure.

    pool    - Pointer to a memory pool.

    length  - Length of the initial buffer, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsCreate(
    DYNAMIC_STRING *dynStr,
    apr_pool_t *pool,
    apr_size_t length
    )
{
    ASSERT(dynStr != NULL);
    ASSERT(pool != NULL);

    dynStr->data = apr_palloc(pool, length + 1);
    if (dynStr->data == NULL) {
        return APR_ENOMEM;
    }

    // Populate dynamic string structure
    dynStr->data[0] = '\0';
    dynStr->pool    = pool;
    dynStr->length  = 0;
    dynStr->size    = length + 1;
    return APR_SUCCESS;
}

/*++

DsCreateFromStr

    Creates a dynamic string from a null-terminated string.

Arguments:
    dynStr  - Pointer to an unused dynamic string structure.

    pool    - Pointer to a memory pool.

    str     - Pointer to a null-terminated string.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsCreateFromStr(
    DYNAMIC_STRING *dynStr,
    apr_pool_t *pool,
    const char *str
    )
{
    ASSERT(dynStr != NULL);
    ASSERT(pool != NULL);
    ASSERT(str != NULL);

    return DsCreateFromData(dynStr, pool, str, strlen(str));
}

/*++

DsCreateFromData

    Creates a dynamic string from data.

Arguments:
    dynStr  - Pointer to an unused dynamic string structure.

    pool    - Pointer to a memory pool.

    data    - Pointer to the data.

    length  - Length of the data, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsCreateFromData(
    DYNAMIC_STRING *dynStr,
    apr_pool_t *pool,
    const char *data,
    apr_size_t length
    )
{
    ASSERT(dynStr != NULL);
    ASSERT(pool != NULL);
    ASSERT(data != NULL);

    dynStr->data = apr_palloc(pool, length + 1);
    if (dynStr->data == NULL) {
        return APR_ENOMEM;
    }

    // Populate dynamic string structure
    memcpy(dynStr->data, data, length);
    dynStr->data[length] = '\0';
    dynStr->pool         = pool;
    dynStr->length       = length;
    dynStr->size         = length + 1;
    return APR_SUCCESS;
}

/*++

DsCreateFromFile

    Creates a dynamic string from the contents of a file.

Arguments:
    dynStr  - Pointer to an unused dynamic string structure.

    pool    - Pointer to a memory pool.

    path    - Pointer to a null-terminated string specifying the path of a file.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsCreateFromFile(
    DYNAMIC_STRING *dynStr,
    apr_pool_t *pool,
    const char *path
    )
{
    ASSERT(dynStr != NULL);
    ASSERT(pool != NULL);
    ASSERT(path != NULL);

    // TODO:
    // - open file for reading
    // - buffer file contents
    // - check for a BOM or for UTF8 chars
    // - convert buffer to UTF8, if necessary

    return APR_SUCCESS;
}

/*++

DsDestroy

    Destroys a dynamic string.

Arguments:
    dynStr  - Pointer to a dynamic string.

Return Values:
    None.

--*/
void
DsDestroy(
    DYNAMIC_STRING *dynStr
    )
{
    ASSERT(dynStr != NULL);

    // No-op
    memset(dynStr, 0, sizeof(DYNAMIC_STRING));
}


/*++

DsAppend

    Appends a dynamic string to another dynamic string.

Arguments:
    target  - Pointer to the target dynamic string.

    source  - Pointer to the source dynamic string.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsAppend(
    DYNAMIC_STRING *target,
    const DYNAMIC_STRING *source
    )
{
    ASSERT(target != NULL);
    ASSERT(source != NULL);

    return DsAppendData(target, source->data, source->length);
}

/*++

DsAppendStr

    Appends a null-terminated string to a dynamic string.

Arguments:
    dynStr  - Pointer to a dynamic string.

    str     - Pointer to the null-terminated string to be appended.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsAppendStr(
    DYNAMIC_STRING *dynStr,
    const char *str
    )
{
    ASSERT(dynStr != NULL);
    ASSERT(str != NULL);

    return DsAppendData(dynStr, str, strlen(str));
}

/*++

DsAppendData

    Appends data to a dynamic string.

Arguments:
    dynStr  - Pointer to a dynamic string.

    data    - Pointer to the data.

    length  - Length of the data, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsAppendData(
    DYNAMIC_STRING *dynStr,
    const char *data,
    apr_size_t length
    )
{
    apr_size_t total;
    apr_status_t status;

    ASSERT(dynStr != NULL);
    ASSERT(data != NULL);

    // Enlarge the buffer, if necessary
    total = dynStr->length + length;
    status = DsExpand(dynStr, total + 1);
    if (status != APR_SUCCESS) {
        return status;
    }

    memcpy(dynStr->data + dynStr->length, data, length);
    dynStr->data[total] = '\0';
    dynStr->length = total;
    return APR_SUCCESS;
}

/*++

DsEqual

    Compares two dynamic strings for equality.

Arguments:
    dynStr1 - Pointer to the first dynamic string.

    dynStr2 - Pointer to the second dynamic string.

Return Values:
    Returns a boolean result.

--*/
bool_t
DsEqual(
    const DYNAMIC_STRING *dynStr1,
    const DYNAMIC_STRING *dynStr2
    )
{
    ASSERT(dynStr1 != NULL);
    ASSERT(dynStr2 != NULL);

    if (dynStr1->length != dynStr2->length) {
        return FALSE;
    }

    // Both strings are of identical lengths
    return (memcmp(dynStr1->data, dynStr2->data, dynStr1->length) == 0) ? TRUE : FALSE;
}

/*++

DsExpand

    Expands the buffer of a dynamic string.

Arguments:
    dynStr  - Pointer to a dynamic string.

    length  - Length to expand the dynamic string's buffer to, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsExpand(
    DYNAMIC_STRING *dynStr,
    apr_size_t length
    )
{
    apr_size_t newSize;
    apr_size_t prevSize;
    char *newData;

    ASSERT(dynStr != NULL);

    // Check if the buffer size is already sufficient
    if (dynStr->size >= length) {
        return APR_SUCCESS;
    }

    if (dynStr->size == 0) {
        newSize = length;
    } else {
        // Continue doubling the current size until we reach the requested length
        newSize = dynStr->size;
        do {
            prevSize = newSize;
            newSize *= 2;

            // Use the requested length if we overflow
            if (prevSize > newSize) {
                newSize = length;
                break;
            }
        } while (newSize < length);
    }

    // Allocate a larger buffer size
    newData = apr_palloc(dynStr->pool, newSize);
    if (newData == NULL) {
        return APR_ENOMEM;
    }

    // Copy the current data to the new buffer. Unfortunately, we have to leak
    // the previous block since there's no way to free pool blocks until the
    // pool is cleared or destroyed (i.e. need a apr_prealloc/apr_pfree function).
    memcpy(newData, dynStr->data, dynStr->size);

    // Update the string structure
    dynStr->data = newData;
    dynStr->size = newSize;
    return APR_SUCCESS;
}

/*++

DsTruncate

    Truncates a dynamic string at the given length.

Arguments:
    dynStr  - Pointer to a dynamic string.

    length  - Position to truncate the string at, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsTruncate(
    DYNAMIC_STRING *dynStr,
    apr_size_t length
    )
{
    ASSERT(dynStr != NULL);

    if (length > dynStr->length) {
        return APR_EINVAL;
    }

    dynStr->length = length;
    dynStr->data[length] = '\0';
    return APR_SUCCESS;
}
