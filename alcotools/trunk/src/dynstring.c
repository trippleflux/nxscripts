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

struct DYNAMIC_STRING {
    char       *data;   // Pointer to the null-terminated data
    apr_size_t length;  // Length of the data, in bytes
    apr_size_t size;    // Size of the data buffer, in bytes
    apr_pool_t *pool;   // Pointer to the pool the buffer is allocated from
};

/*++

CreateString

    Creates a dynamic string structure.

Arguments:
    data    - Pointer to the null-terminated data.

    length  - Length of the data, in bytes.

    size    - Size of the data buffer, in bytes.

    pool    - Pointer to a memory pool.

Return Values:
    If the function succeeds, the return value is a pointer to a DYNAMIC_STRING structure.

    If the function fails, the return value is null.

--*/
static
inline
DYNAMIC_STRING *
CreateString(
    char *data,
    apr_size_t length,
    apr_size_t size,
    apr_pool_t *pool
    )
{
    DYNAMIC_STRING *dynStr;

    ASSERT(pool != NULL);
    ASSERT(data != NULL);
    ASSERT(size > 0);
    ASSERT(size > length);

    dynStr = apr_palloc(pool, sizeof(DYNAMIC_STRING));
    if (dynStr != NULL) {
        dynStr->data   = data;
        dynStr->length = length;
        dynStr->size   = size;
        dynStr->pool   = pool;
    }
    return dynStr;
}


/*++

DsCreate

    Creates an empty dynamic string.

Arguments:
    length  - Length of the initial buffer, in bytes.

    pool    - Pointer to a memory pool.

Return Values:
    If the function succeeds, the return value is a pointer to a DYNAMIC_STRING structure.

    If the function fails, the return value is null.

--*/
DYNAMIC_STRING *
DsCreate(
    apr_size_t length,
    apr_pool_t *pool
    )
{
    char *data;

    ASSERT(pool != NULL);

    data = apr_palloc(pool, length + 1);
    if (data == NULL) {
        return NULL;
    }

    // Create dynamic string structure
    data[0] = '\0';
    return CreateString(data, 0, length + 1, pool);
}

/*++

DsCreateFromStr

    Creates a dynamic string from a null-terminated string.

Arguments:
    str     - Pointer to a null-terminated string.

    pool    - Pointer to a memory pool.

Return Values:
    If the function succeeds, the return value is a pointer to a DYNAMIC_STRING structure.

    If the function fails, the return value is null.

--*/
DYNAMIC_STRING *
DsCreateFromStr(
    const char *str,
    apr_pool_t *pool
    )
{
    ASSERT(str != NULL);
    ASSERT(pool != NULL);

    return DsCreateFromData(str, strlen(str), pool);
}

/*++

DsCreateFromData

    Creates a dynamic string from data.

Arguments:
    buffer  - Pointer to the buffer.

    length  - Length of the buffer, in bytes.

    pool    - Pointer to a memory pool.

Return Values:
    If the function succeeds, the return value is a pointer to a DYNAMIC_STRING structure.

    If the function fails, the return value is null.

--*/
DYNAMIC_STRING *
DsCreateFromData(
    const char *buffer,
    apr_size_t length,
    apr_pool_t *pool
    )
{
    char *data;

    ASSERT(buffer != NULL);
    ASSERT(pool != NULL);

    data = apr_palloc(pool, length + 1);
    if (data == NULL) {
        return NULL;
    }

    // Create dynamic string structure
    memcpy(data, buffer, length);
    data[length] = '\0';
    return CreateString(data, length, length + 1, pool);
}

/*++

DsCreateFromFile

    Creates a dynamic string from the contents of a file.

Arguments:
    path    - Pointer to a null-terminated string specifying the path of a file.

    pool    - Pointer to a memory pool.

Return Values:
    If the function succeeds, the return value is a pointer to a DYNAMIC_STRING structure.

    If the function fails, the return value is null.

--*/
DYNAMIC_STRING *
DsCreateFromFile(
    const char *path,
    apr_pool_t *pool
    )
{
    apr_byte_t *buffer;
    apr_size_t length;
    apr_status_t status;

    ASSERT(path != NULL);
    ASSERT(pool != NULL);

    status = BufferFile(path, &buffer, &length, pool);
    if (status != APR_SUCCESS) {
        return NULL;
    }

    // TODO:
    // - check for a BOM or for UTF8 chars
    // - convert buffer to UTF8, if necessary

    return NULL;
}

/*++

DsClear

    Clears the contents of a dynamic string.

Arguments:
    dynStr  - Pointer to a dynamic string.

Return Values:
    None.

--*/
void
DsClear(
    DYNAMIC_STRING *dynStr
    )
{
    ASSERT(dynStr != NULL);
    ASSERT(dynStr->size > 0);

    dynStr->data[0] = '\0';
    dynStr->length  = 0;
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

    buffer  - Pointer to the buffer.

    length  - Length of the buffer, in bytes.

Return Values:
    Returns an APR status code.

--*/
apr_status_t
DsAppendData(
    DYNAMIC_STRING *dynStr,
    const char *buffer,
    apr_size_t length
    )
{
    apr_size_t total;
    apr_status_t status;

    ASSERT(dynStr != NULL);
    ASSERT(buffer != NULL);

    // Enlarge the buffer, if necessary
    total = dynStr->length + length;
    status = DsExpand(dynStr, total + 1);
    if (status != APR_SUCCESS) {
        return status;
    }

    memcpy(dynStr->data + dynStr->length, buffer, length);
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

DsGet

    Retrieves the value and/or length of a dynamic string.

Arguments:
    dynStr  - Pointer to a dynamic string.

    length  - Pointer to a variable to store the string's length. This argument can be null.

Return Values:
    A pointer to the dynamic string's value.

--*/
char *
DsGet(
    DYNAMIC_STRING *dynStr,
    apr_size_t *length
    )
{
    ASSERT(dynStr != NULL);

    if (length != NULL) {
        *length = dynStr->length;
    }
    return dynStr->data;
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
