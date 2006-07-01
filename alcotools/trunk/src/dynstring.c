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

    Creates a dynamic string from a null-terminated string.

Arguments:
    str     - Pointer to an unused dynamic string structure.

    pool    - Pointer to a memory pool.

    data    - Pointer to a null-terminated string.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsCreate(
    DYNAMIC_STRING *str,
    apr_pool_t *pool,
    const char *data
    )
{
    ASSERT(str != NULL);
    ASSERT(pool != NULL);
    ASSERT(data != NULL);

    return DsCreateN(str, pool, data, strlen(data));
}

/*++

DsCreateN

    Creates a dynamic string from data.

Arguments:
    str     - Pointer to an unused dynamic string structure.

    pool    - Pointer to a memory pool.

    data    - Pointer to the data.

    length  - Length of the data, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsCreateN(
    DYNAMIC_STRING *str,
    apr_pool_t *pool,
    const char *data,
    apr_size_t length
    )
{
    ASSERT(str != NULL);
    ASSERT(pool != NULL);
    ASSERT(data != NULL);

    str->data = apr_palloc(pool, length + 1);
    if (str->data == NULL) {
        return APR_ENOMEM;
    }

    // Populate dynamic string structure
    memcpy(str->data, data, length);
    str->data[length] = '\0';
    str->pool         = pool;
    str->length       = length;
    str->size         = length + 1;
    return APR_SUCCESS;
}

/*++

DsCreateFromFile

    Creates a dynamic string from the contents of a file.

Arguments:
    str     - Pointer to an unused dynamic string structure.

    pool    - Pointer to a memory pool.

    path    - Pointer to a null-terminated string specifying the path of a file.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsCreateFromFile(
    DYNAMIC_STRING *str,
    apr_pool_t *pool,
    const char *path
    )
{
    ASSERT(str != NULL);
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
    str     - Pointer to a dynamic string.

Return Values:
    None.

--*/
void
DsDestroy(
    DYNAMIC_STRING *str
    )
{
    ASSERT(str != NULL);

    // No-op
    memset(str, 0, sizeof(DYNAMIC_STRING));
}


/*++

DsAppend

    Appends a dynamic string to another dynamic string.

Arguments:
    strTarget - Pointer to the target dynamic string.

    strSource - Pointer to the source dynamic string.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsAppend(
    DYNAMIC_STRING *strTarget,
    const DYNAMIC_STRING *strSource
    )
{
    ASSERT(strTarget != NULL);
    ASSERT(strSource != NULL);

    return DsAppendStrN(strTarget, strSource->data, strSource->length);
}

/*++

DsAppendStr

    Appends a null-terminated string to a dynamic string.

Arguments:
    str     - Pointer to a dynamic string.

    data    - Pointer to the null-terminated string to be appended.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsAppendStr(
    DYNAMIC_STRING *str,
    const char *data
    )
{
    ASSERT(str != NULL);
    ASSERT(data != NULL);

    return DsAppendStrN(str, data, strlen(data));
}

/*++

DsAppendStrN

    Appends the given amount of data to a dynamic string.

Arguments:
    str     - Pointer to a dynamic string.

    data    - Pointer to the data to be appended.

    length  - Length of the data to be appended, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsAppendStrN(
    DYNAMIC_STRING *str,
    const char *data,
    apr_size_t length
    )
{
    apr_size_t total;
    apr_status_t status;

    ASSERT(str != NULL);
    ASSERT(data != NULL);

    // Enlarge the buffer, if necessary
    total = str->length + length;
    status = DsExpand(str, total + 1);
    if (status != APR_SUCCESS) {
        return status;
    }

    memcpy(str->data + str->length, data, length);
    str->data[total] = '\0';
    str->length = total;
    return APR_SUCCESS;
}

/*++

DsEqual

    Compares two dynamic strings for equality.

Arguments:
    str1    - Pointer to the first dynamic string.

    str2    - Pointer to the second dynamic string.

Return Values:
    Returns a boolean result.

--*/
bool_t
DsEqual(
    const DYNAMIC_STRING *str1,
    const DYNAMIC_STRING *str2
    )
{
    ASSERT(str1 != NULL);
    ASSERT(str2 != NULL);

    if (str1->length != str2->length) {
        return FALSE;
    }

    // Both strings are of identical lengths
    return (memcmp(str1->data, str2->data, str1->length) == 0) ? TRUE : FALSE;
}

/*++

DsExpand

    Expands the buffer of a dynamic string.

Arguments:
    str     - Pointer to a dynamic string.

    length  - Length to expand the dynamic string's buffer to, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsExpand(
    DYNAMIC_STRING *str,
    apr_size_t length
    )
{
    apr_size_t newSize;
    apr_size_t prevSize;
    char *newData;

    ASSERT(str != NULL);

    // Check if the buffer size is already sufficient
    if (str->size >= length) {
        return APR_SUCCESS;
    }

    if (str->size == 0) {
        newSize = length;
    } else {
        // Continue doubling the current size until we reach the requested length
        newSize = str->size;
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
    newData = apr_palloc(str->pool, newSize);
    if (newData == NULL) {
        return APR_ENOMEM;
    }

    // Copy the current data to the new buffer. Unfortunately, we have to leak
    // the previous block since there's no way to free pool blocks until the
    // pool is cleared or destroyed (i.e. need a apr_prealloc/apr_pfree function).
    memcpy(newData, str->data, str->size);

    // Update the string structure
    str->data = newData;
    str->size = newSize;
    return APR_SUCCESS;
}

/*++

DsTruncate

    Truncates a dynamic string at the given length.

Arguments:
    str     - Pointer to a dynamic string.

    length  - Position to truncate the string at, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsTruncate(
    DYNAMIC_STRING *str,
    apr_size_t length
    )
{
    ASSERT(str != NULL);

    if (length > str->length) {
        return APR_EINVAL;
    }

    str->length = length;
    str->data[length] = '\0';
    return APR_SUCCESS;
}
