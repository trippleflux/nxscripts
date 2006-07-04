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

typedef struct DYNAMIC_STRING DYNAMIC_STRING;

//
// Functions for creating and deleting dynamic strings.
//

DYNAMIC_STRING *
DsCreate(
    apr_size_t length,
    apr_pool_t *pool
    );

DYNAMIC_STRING *
DsCreateFromStr(
    const char *str,
    apr_pool_t *pool
    );

DYNAMIC_STRING *
DsCreateFromData(
    const char *buffer,
    apr_size_t length,
    apr_pool_t *pool
    );

DYNAMIC_STRING *
DsCreateFromFile(
    const char *path,
    apr_pool_t *pool
    );

void
DsClear(
    DYNAMIC_STRING *dynStr
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
    const char *buffer,
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

char *
DsGet(
    DYNAMIC_STRING *dynStr,
    apr_size_t *length
    );

apr_status_t
DsTruncate(
    DYNAMIC_STRING *dynStr,
    apr_size_t length
    );

#endif // _DYNSTRING_H_
