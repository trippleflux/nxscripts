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
    char       *data;   // Pointer to the string
    apr_pool_t *pool;   // Pointer to the pool the string is allocated from
    apr_size_t length;  // Length of the string, in characters
    apr_size_t size;    // Size of the buffer allocated, in bytes
} DYNAMIC_STRING;

//
// Macros for accessing members of the DYNAMIC_STRING structure.
//

#define DsGetData(str)    ((str)->data)
#define DsGetLength(str)  ((str)->length)

//
// Functions for creating and deleting dynamic strings.
//

apr_status_t
DsCreate(
    DYNAMIC_STRING *str,
    apr_pool_t *pool,
    const char *data
    );

apr_status_t
DsCreateN(
    DYNAMIC_STRING *str,
    apr_pool_t *pool,
    const char *data,
    apr_size_t length
    );

apr_status_t
DsCreateFromFile(
    DYNAMIC_STRING *str,
    apr_pool_t *pool,
    const char *path
    );

void
DsDestroy(
    DYNAMIC_STRING *str
    );

//
// Functions for manipulating dynamic strings.
//

apr_status_t
DsAppend(
    DYNAMIC_STRING *strTarget,
    DYNAMIC_STRING *strSource
    );

apr_status_t
DsAppendStr(
    DYNAMIC_STRING *str,
    const char *data
    );

apr_status_t
DsAppendStrN(
    DYNAMIC_STRING *str,
    const char *data,
    apr_size_t length
    );

bool_t
DsEqual(
    DYNAMIC_STRING *str1,
    DYNAMIC_STRING *str2
    );

apr_status_t
DsExpand(
    DYNAMIC_STRING *str,
    apr_size_t length
    );

apr_status_t
DsTruncate(
    DYNAMIC_STRING *str,
    apr_size_t length
    );

#endif // _DYNSTRING_H_
