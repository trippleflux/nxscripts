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
    uint32_t crc;       // CRC32 checksum of the event name
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
t_main(
    int argc,
    tchar_t **argv
    )
{
    int i;
    int result = 2;
    uint32_t crc;
    PERF_COUNTER perfCounter;

    PerfCounterStart(&perfCounter);

    // Verify status code/message mappings
    ASSERT(!t_strcmp(GetStatusMessage(ALCOHOL_OK),                  TEXT("Successful.")));
    ASSERT(!t_strcmp(GetStatusMessage(ALCOHOL_ERROR),               TEXT("General failure.")));
    ASSERT(!t_strcmp(GetStatusMessage(ALCOHOL_INSUFFICIENT_BUFFER), TEXT("Insufficient buffer size.")));
    ASSERT(!t_strcmp(GetStatusMessage(ALCOHOL_INSUFFICIENT_MEMORY), TEXT("Insufficient system memory.")));
    ASSERT(!t_strcmp(GetStatusMessage(ALCOHOL_INVALID_DATA),        TEXT("Invalid data.")));
    ASSERT(!t_strcmp(GetStatusMessage(ALCOHOL_INVALID_PARAMETER),   TEXT("Invalid parameter.")));
    ASSERT(!t_strcmp(GetStatusMessage(ALCOHOL_UNKNOWN),             TEXT("Unknown value.")));

    //
    // Load Order:
    // The configuration file reader relies on the memory subsystem for
    // buffering. The logging subsystem must be able to retrieve the
    // user-defined log file and verbosity level from the configuration file.
    //

    if (!MemInit()) {
        Panic(TEXT("Unable to initialise memory subsystem: %s\n"),
            GetSystemErrorMessage());
    }

    if (!ConfigInit()) {
         Panic(TEXT("Unable to read configuration file: %s\n"),
             GetSystemErrorMessage());
    }

#if (LOG_LEVEL > 0)
    if (!LogInit()) {
        Panic(TEXT("Unable to open log file: %s\n"), GetSystemErrorMessage());
    }
#endif

    VERBOSE(TEXT("AlcoTools starting (%d arguments).\n"), argc);
    for (i = 0; i < argc; i++) {
        VERBOSE(TEXT("Argument[%d] = \"%s\"\n"), i, argv[i]);
    }

#ifdef DEBUG
    //
    // Verify pre-computed CRC-32 checksums in the main-event table.
    //
    for (i = 0; i < ARRAYSIZE(mainEvents); i++) {
        crc = Crc32UpperString(mainEvents[i].name);

        if (crc != mainEvents[i].crc) {
            ERROR(TEXT("Invalid CRC32 for main event %d: current=0x%08X calculated=0x%08X\n"),
                i, mainEvents[i].crc, crc);
        }
    }
#endif

    if (argc < 2) {
        t_printf(TEXT("No event specified.\n"));
    } else {
        //
        // Calculate the CRC-32 checksum of the first command-line argument and
        // compare it with the pre-computed checksums in the main-event table.
        // This method is marginally faster than performing CompareString
        // (or similar) multiple times.
        //
#ifdef UNICODE
        char ansiName[24];
        //
        // We're using a fixed-size buffer for performance reasons, even
        // though it is more than enough to hold the longest event name.
        //
        WideCharToMultiByte(CP_ACP, 0, argv[1], -1,
            ansiName, ARRAYSIZE(ansiName), NULL, NULL);

        //
        // WideCharToMultiByte will not NULL terminate the string if the
        // destination buffer is too small, so we must do it ourselves.
        //
        ansiName[ARRAYSIZE(ansiName)-1] = '\0';
        crc = Crc32UpperString(ansiName);
#else
        crc = Crc32UpperString(argv[1]);
#endif

        VERBOSE(TEXT("CRC32 checksum for \"%s\": 0x%08X\n"), argv[1], crc);

        for (i = 0; i < ARRAYSIZE(mainEvents); i++) {
            if (crc == mainEvents[i].crc) {
                result = mainEvents[i].proc(argc-2, argv+2);

                if (result != ALCOHOL_OK) {
                    ERROR((TEXT("Event callback returned \"%d\": %s\n"),
                        result, GetStatusMessage(result)));
                }
                break;
            }
        }

        if (i >= ARRAYSIZE(mainEvents)) {
            t_printf(TEXT("Unknown event: %s\n"), argv[1]);
            ERROR(TEXT("Unknown event: %s\n"), argv[1]);
        }
    }

    VERBOSE(TEXT("Exit result: %d\n"), result);

#ifdef WINDOWS
    //
    // Detach from ioFTPD before finalising subsystems and freeing resources.
    //
    t_printf(TEXT("!detach %d\n"), result);
    fflush(stdout);
#endif

    PerfCounterStop(&perfCounter);
    WARNING(TEXT("Completed in %.3f ms.\n"), PerfCounterDiff(&perfCounter));

    VERBOSE(TEXT("Finalising config subsystem.\n"));
    ConfigFinalise();

    VERBOSE(TEXT("Finalising memory subsystem.\n"));
    MemFinalise();

#if (LOG_LEVEL > 0)
    VERBOSE(TEXT("Finalising log subsystem.\n"));
    LogFinalise();
#endif

    return result;
}
