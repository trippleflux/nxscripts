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

int
EventPostDele(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventPostDele: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventPostMkd(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventPostMkd: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventPostRename(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventPostRename: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventPostRmd(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventPostRmd: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventPreMkd(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventPreMkd: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventPreStor(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventPreStor: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventUpload(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventUpload: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventUploadError(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventUploadError: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventSiteDupe(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventSiteDupe: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventSiteFileDupe(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventSiteFileDupe: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventSiteNew(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventSiteNew: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventSiteRescan(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventSiteRescan: %d arguments\n"), argc);
    return ALCOHOL_OK;
}

int
EventSiteUndupe(
    int argc,
    tchar_t **argv
    )
{
    VERBOSE(TEXT("EventSiteUndupe: %d arguments\n"), argc);
    return ALCOHOL_OK;
}
