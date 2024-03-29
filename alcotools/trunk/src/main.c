/*++

AlcoTools - Alcoholicz dupe checker, zipscript, and utilities.
Copyright (c) 2005-2006 Alcoholicz Scripting Team

Module Name:
    Main

Author:
    neoxed (neoxed@gmail.com) Jul 17, 2005

Abstract:
    This module implements the application entry point which dispatches
    events based on the first user-specified command line argument.

--*/

#include "alcoholicz.h"

// Standard error and output streams
extern STREAM *streamErr = NULL;
extern STREAM *streamOut = NULL;

struct {
    EVENT_PROC *proc;   // Event callback
    const char *name;   // Event name
    apr_uint32_t crc;   // CRC-32 checksum of the event name
} static const events[] = {
    // Command-based events
    {EventPostDele,     "POSTDELE",    0xFD930F3A},
    {EventPostMkd,      "POSTMKD",     0x6115AE4A},
    {EventPostRnfr,     "POSTRNFR",    0xFD8885D5},
    {EventPostRnto,     "POSTRNTO",    0xE67A99DF},
    {EventPostRmd,      "POSTRMD",     0x2035ED81},
    {EventPreMkd,       "PREMKD",      0x492B9024},
    {EventPreStor,      "PRESTOR",     0x2F650D3E},
    {EventUpload,       "UPLOAD",      0xE7452199},
    {EventUploadError,  "UPLOADERROR", 0x097FD9FB},

    // SITE commands
    {EventSiteDupe,     "DUPE",        0xE3481466},
    {EventSiteFileDupe, "FILEDUPE",    0x71C6F2FD},
    {EventSiteNew,      "NEW",         0xFD4406CF},
    {EventSiteRescan,   "RESCAN",      0xB3F55E60},
    {EventSiteUndupe,   "UNDUPE",      0xC84E15B5}
};

/*++

main

    Application entry point.

Arguments:
    argc - Number of command-line arguments.

    argv - Array of pointers to strings that represent command-line arguments.

Return Values:
    If the function succeeds, the return value is zero.

    If the function fails, the return value is nonzero.

--*/
int
main(
    int argc,
    char **argv
    )
{
    int i;
    apr_pool_t *pool;
    apr_pool_t *eventPool;
    apr_status_t status;
    apr_time_t counter;
    apr_uint32_t crc;

    // Count the time elapsed while running
    counter = apr_time_now();

    if (argc < 2) {
        printf("No event specified.\n");
        return -1;
    }

    // Initialize APR memory pools and convert arguments to UTF8
    status = apr_app_initialize(&argc, &argv, NULL);
    if (status != APR_SUCCESS) {
        printf("APR library failed to initialize: %s\n", GetErrorMessage(status));
        return -1;
    }

    // Create an application memory pool
    status = apr_pool_create(&pool, NULL);
    if (status != APR_SUCCESS) {
        printf("Unable to create memory pool: %s\n", GetErrorMessage(status));
        apr_terminate();
        return -1;
    }

    // Initialize subsystems
    status = EncInit(pool);
    if (status != APR_SUCCESS) {
         printf("Unable to set system encoding: %s\n", GetErrorMessage(status));
         goto exit;
    }
    status = ConfigInit(pool);
    if (status != APR_SUCCESS) {
         printf("Unable to read configuration file: %s\n", GetErrorMessage(status));
         goto exit;
    }
#if (LOG_LEVEL > 0)
    status = LogInit(pool);
    if (status != APR_SUCCESS) {
        printf("Unable to open log file: %s\n", GetErrorMessage(status));
        goto exit;
    }
#endif

    // Create standard streams
    streamErr = StreamCreateTextConsole(CONSOLE_ERROR, pool);
    streamOut = StreamCreateTextConsole(CONSOLE_OUTPUT, pool);
    if (streamErr == NULL || streamOut == NULL) {
        printf("Unable to open standard error and output streams.\n");
        status = APR_ENOMEM;
        goto exit;
    }

    LOG_VERBOSE("AlcoTools v%s starting, received %d arguments:", APR_STRINGIFY(VERSION), argc);
    for (i = 0; i < argc; i++) {
        LOG_VERBOSE("Argument %d = \"%s\"", i, argv[i]);
    }

#ifdef DEBUG
    // Verify pre-computed CRC-32 checksums in the event table
    for (i = 0; i < ARRAYSIZE(events); i++) {
        crc = Crc32UpperString(events[i].name);

        if (crc != events[i].crc) {
            LOG_ERROR("Invalid CRC-32 checksum for event \"%s\": current=0x%08X calculated=0x%08X",
                events[i].name, events[i].crc, crc);
        }
    }
#endif

    // Create a sub-pool for the event callback
    status = apr_pool_create(&eventPool, pool);
    if (status != APR_SUCCESS) {
        LOG_ERROR("Unable to create memory sub-pool: %s", GetErrorMessage(status));
        goto exit;
    }

    //
    // Calculate the CRC-32 checksum of the first command-line argument and
    // compare it with the pre-computed checksums in the event table. This
    // method is marginally faster than performing strcmp() multiple times.
    //
    crc = Crc32UpperString(argv[1]);
    LOG_VERBOSE("CRC-32 checksum for event \"%s\" is 0x%08X.", argv[1], crc);

    for (i = 0; i < ARRAYSIZE(events); i++) {
        if (crc == events[i].crc) {
            status = events[i].proc(argc-2, argv+2, eventPool);

            if (status != APR_SUCCESS) {
                LOG_ERROR("Event callback returned %d: %s", status, GetErrorMessage(status));
            }
            break;
        }
    }

    if (i >= ARRAYSIZE(events)) {
        printf("Unknown event: %s\n", argv[1]);
        LOG_ERROR("Unknown event: %s", argv[1]);
        status = APR_EINVAL;
    }

    LOG_VERBOSE("Time taken: %.3f ms", (apr_time_now() - counter)/1000.0);
    LOG_VERBOSE("Exit status: %d", status);

exit:
    if (status != APR_SUCCESS) {
        status = 1;
    }

#ifdef WINDOWS
    // Detach from ioFTPD before finalizing subsystems
    printf("!detach %d\n", status);
    fflush(stdout);
#endif

    apr_pool_destroy(pool);
    apr_terminate();
    return status;
}
