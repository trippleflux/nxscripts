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
    LOG_DEBUG("Event: post-DELE with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostMkd(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: post-MKD with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRnfr(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: post-RNFR with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRnto(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: post-RNTO with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPostRmd(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: post-RMD with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPreMkd(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: pre-MKD with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventPreStor(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: pre-STOR with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventUpload(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventUpload with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventUploadError(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("EventUploadError with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteDupe(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: SITE DUPE with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteFileDupe(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: SITE FDUPE with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteNew(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: SITE NEW with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteRescan(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: SITE RESCAN with %d argument(s).", argc);
    return APR_SUCCESS;
}

apr_status_t
EventSiteUndupe(
    apr_pool_t *pool,
    int argc,
    char **argv
    )
{
    LOG_DEBUG("Event: SITE UNDUPE with %d argument(s).", argc);
    return APR_SUCCESS;
}
