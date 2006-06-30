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

void
DsDestroy(
    DYNAMIC_STRING *str
    )
{
    ASSERT(str != NULL);
    memset(str, 0, sizeof(DYNAMIC_STRING));
}


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
    // the previous block since there's no way to free pool memory until the
    // pool is cleared or destroyed (i.e. need a apr_prealloc/apr_pfree function).
    memcpy(newData, str->data, str->size);

    // Update the string structure
    str->data = newData;
    str->size = newSize;
    return APR_SUCCESS;
}

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
