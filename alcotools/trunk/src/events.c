/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Events

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    This module implements callbacks for FTP events and custom SITE commands.

--*/

#include "alcoholicz.h"

apr_status_t
EventPostDele(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventPostDele: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostMkd(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventPostMkd: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRename(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventPostRename: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRmd(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventPostRmd: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPreMkd(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventPreMkd: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPreStor(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventPreStor: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventUpload(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventUpload: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventUploadError(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventUploadError: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteDupe(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventSiteDupe: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteFileDupe(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventSiteFileDupe: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteNew(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventSiteNew: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteRescan(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventSiteRescan: %d arguments\n", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteUndupe(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    VERBOSE("EventSiteUndupe: %d arguments\n", argc);
    return APR_SUCCESS;
}
