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
    LOG_DEBUG("EventPostDele: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostMkd(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventPostMkd: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRename(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventPostRename: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRmd(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventPostRmd: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPreMkd(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventPreMkd: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPreStor(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventPreStor: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventUpload(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventUpload: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventUploadError(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventUploadError: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteDupe(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventSiteDupe: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteFileDupe(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventSiteFileDupe: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteNew(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventSiteNew: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteRescan(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventSiteRescan: %d arguments", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteUndupe(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventSiteUndupe: %d arguments", argc);
    return APR_SUCCESS;
}
