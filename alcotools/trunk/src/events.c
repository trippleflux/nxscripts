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
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: post-DELE with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostMkd(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: post-MKD with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRnfr(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: post-RNFR with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRnto(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: post-RNTO with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRmd(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: post-RMD with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPreMkd(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: pre-MKD with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPreStor(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: pre-STOR with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventUpload(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("EventUpload with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventUploadError(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("EventUploadError with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteDupe(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: SITE DUPE with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteFileDupe(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: SITE FDUPE with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteNew(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: SITE NEW with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteRescan(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: SITE RESCAN with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteUndupe(
    int argc,
    char **argv,
    apr_pool_t *pool
    )
{
    LOG_DEBUG("Event: SITE UNDUPE with %d argument(s).", argc);
    return APR_SUCCESS;
}
