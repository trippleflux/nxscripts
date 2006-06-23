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

struct {
    EVENT_PROC *proc;   // Event callback
    const char *name;   // Event name
    apr_uint32_t crc;   // CRC32 checksum of the event name
} static const mainEvents[] = {
    // Command-based events
    {EventPostDele,     "POSTDELE",    0xFD930F3A},
    {EventPostMkd,      "POSTMKD",     0x6115AE4A},
    {EventPostRename,   "POSTRENAME",  0xCF212902},
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

Return Value:
    0 - Program executed successfully.
    1 - Intentional error (i.e. bad file, block directory creation).
    2 - Invalid arguments.
    3 - Abnormal termination (panic).

Remarks:
    None.

--*/
int __cdecl
main(
    int argc,
    char **argv
    )
{
    int i;
    apr_status_t status;
    apr_uint32_t crc;
    PERF_COUNTER perfCounter;

    PerfCounterStart(&perfCounter);

    // Initialize subsystems
    status = ConfigInit(NULL);
    if (status != APR_SUCCESS) {
         Panic("Unable to read configuration file (error %d).\n", status);
    }

#if (LOG_LEVEL > 0)
    status = LogInit(NULL);
    if (status != APR_SUCCESS) {
        Panic("Unable to open log file (error %d).\n", status);
    }
#endif

    VERBOSE("AlcoTools starting (%d arguments).\n", argc);
    for (i = 0; i < argc; i++) {
        VERBOSE("Argument[%d] = \"%s\"\n", i, argv[i]);
    }

#ifdef DEBUG
    //
    // Verify pre-computed CRC-32 checksums in the main-event table.
    //
    for (i = 0; i < ARRAYSIZE(mainEvents); i++) {
        crc = Crc32UpperString(mainEvents[i].name);

        if (crc != mainEvents[i].crc) {
            ERROR("Invalid CRC32 for main event \"%s\": current=0x%08X calculated=0x%08X\n"),
                mainEvents[i].name, mainEvents[i].crc, crc);
        }
    }
#endif

    // No parameter or unknown event.
    status = APR_EINVAL;

    if (argc < 2) {
        printf("No event specified.\n");
    } else {
        //
        // Calculate the CRC-32 checksum of the first command-line argument and
        // compare it with the pre-computed checksums in the main-event table.
        // This method is marginally faster than performing strcmp() multiple times.
        //
        crc = Crc32UpperString(argv[1]);
        VERBOSE("CRC32 checksum for \"%s\": 0x%08X\n", argv[1], crc);

        for (i = 0; i < ARRAYSIZE(mainEvents); i++) {
            if (crc == mainEvents[i].crc) {
                status = mainEvents[i].proc(argc-2, argv+2);

                if (status != APR_SUCCESS) {
                    ERROR("Event callback returned %d.\n", status);
                }
                break;
            }
        }

        if (i >= ARRAYSIZE(mainEvents)) {
            printf("Unknown event: %s\n", argv[1]);
            ERROR("Unknown event: %s\n", argv[1]);
        }
    }

    VERBOSE("Exit result: %d\n", status);

#ifdef WINDOWS
    //
    // Detach from ioFTPD before finalizing subsystems and freeing resources.
    //
    printf("!detach %d\n", status);
    fflush(stdout);
#endif

    PerfCounterStop(&perfCounter);
    WARNING("Completed in %.3f ms.\n", PerfCounterDiff(&perfCounter));

    // Finalize subsystems
    VERBOSE("Finalizing config subsystem.\n");
    ConfigFinalize();

#if (LOG_LEVEL > 0)
    VERBOSE("Finalizing log subsystem.\n");
    LogFinalize();
#endif

    return status;
}
