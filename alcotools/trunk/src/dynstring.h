/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Dynamic String

Author:
    neoxed (neoxed@gmail.com) Feb 25, 2006

Abstract:
    Dynamic length string function prototypes and structures.

--*/

#ifndef _DYNSTRING_H_
#define _DYNSTRING_H_

typedef struct {
    char       *data;   // Pointer to the null-terminated data
    apr_pool_t *pool;   // Pointer to the pool the buffer is allocated from
    apr_size_t length;  // Length of the data, in bytes
    apr_size_t size;    // Size of the buffer, in bytes
} DYNAMIC_STRING;

//
// Macros for accessing members of the DYNAMIC_STRING structure.
//

#define DsGetData(dynStr)   ((dynStr)->data)
#define DsGetLength(dynStr) ((dynStr)->length)

//
// Functions for creating and deleting dynamic strings.
//

apr_status_t
DsCreate(
    DYNAMIC_STRING *dynStr,
    apr_pool_t *pool,
    apr_size_t length
    );

apr_status_t
DsCreateFromStr(
    DYNAMIC_STRING *dynStr,
    apr_pool_t *pool,
    const char *str
    );

apr_status_t
DsCreateFromData(
    DYNAMIC_STRING *dynStr,
    apr_pool_t *pool,
    const char *data,
    apr_size_t length
    );

apr_status_t
DsCreateFromFile(
    DYNAMIC_STRING *dynStr,
    apr_pool_t *pool,
    const char *path
    );

void
DsDestroy(
    DYNAMIC_STRING *dynStr
    );

//
// Functions for manipulating dynamic strings.
//

apr_status_t
DsAppend(
    DYNAMIC_STRING *target,
    const DYNAMIC_STRING *source
    );

apr_status_t
DsAppendStr(
    DYNAMIC_STRING *dynStr,
    const char *str
    );

apr_status_t
DsAppendData(
    DYNAMIC_STRING *dynStr,
    const char *data,
    apr_size_t length
    );

bool_t
DsEqual(
    const DYNAMIC_STRING *dynStr1,
    const DYNAMIC_STRING *dynStr2
    );

apr_status_t
DsExpand(
    DYNAMIC_STRING *dynStr,
    apr_size_t length
    );

apr_status_t
DsTruncate(
    DYNAMIC_STRING *dynStr,
    apr_size_t length
    );

#endif // _DYNSTRING_H_
