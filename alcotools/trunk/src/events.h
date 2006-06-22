/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Events

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    Event function prototypes.

--*/

#ifndef _EVENTS_H_
#define _EVENTS_H_

typedef int (EVENT_PROC)(
    int argc,
    tchar_t **argv
    );

// Command-based events
EVENT_PROC EventPostDele;
EVENT_PROC EventPostMkd;
EVENT_PROC EventPostRename;
EVENT_PROC EventPostRmd;
EVENT_PROC EventPreMkd;
EVENT_PROC EventPreStor;
EVENT_PROC EventUpload;
EVENT_PROC EventUploadError;

// SITE commands
EVENT_PROC EventSiteDupe;
EVENT_PROC EventSiteFileDupe;
EVENT_PROC EventSiteNew;
EVENT_PROC EventSiteRescan;
EVENT_PROC EventSiteUndupe;

#endif // _EVENTS_H_
