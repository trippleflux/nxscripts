/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Info

Author:
    neoxed (neoxed@gmail.com) May 16, 2006

Abstract:
    Example application, demonstrates how to retrieve information about ioFTPD.

*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <nxsdk.h>

// Silence C4100: unreferenced formal parameter
#pragma warning(disable : 4100)

static void
Usage(
    const char *argv0
    );

static Io_OnlineDataProc CountUsers;


int
main(
    int argc,
    char **argv
    )
{
    char binaryPath[MAX_PATH];
    int users;
    FILETIME fileTime;
    IO_MEMORY *memory;
    IO_SESSION session;
    SYSTEMTIME sysTime;

    if (argc != 2) {
        Usage(argv[0]);
        return 1;
    }

    // Locate ioFTPD's message window.
    if (!Io_ShmInit(argv[1], &session)) {
        printf("The message window \"%s\" does not exist.\n", argv[1]);
        return 1;
    }

    // Retrieve information.
    if (!Io_GetBinaryPath(&session, binaryPath, MAX_PATH) ||
            !Io_GetStartTime(&session, &fileTime)) {
        printf("Unable to retrieve information (error %lu).\n", GetLastError());
        return 1;
    }
    FileTimeToSystemTime(&fileTime, &sysTime);

    // Allocate memory for online data.
    memory = Io_ShmAlloc(&session, sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2);
    if (memory == NULL) {
        printf("Unable to allocate shared memory (error %lu).\n", GetLastError());
        return 1;
    }

    // Count online users.
    users = 0;
    Io_GetOnlineData(memory, CountUsers, &users);

    // Display information.
    printf(" Binary Path: %s\n", binaryPath);

    printf("  Process ID: %lu\n", session.remoteProcId);

    printf("  Start Time: %02d:%02d:%02d %04d-%02d-%02d\n",
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
        sysTime.wYear, sysTime.wMonth, sysTime.wDay);

    printf("Online Users: %d\n", users);

    // Clean up.
    Io_ShmFree(memory);

    return 0;
}

static int
STDCALL
CountUsers(
    int connId,
    ONLINEDATA *onlineData,
    void *opaque
    )
{
    ++*((int *)opaque);
    return IO_ONLINEDATA_CONTINUE;
}

static void
Usage(
    const char *argv0
    )
{
    printf("\n");
    printf("Usage: %s <window>\n\n", argv0);
    printf("Arguments:\n");
    printf("  window - ioFTPD's message window.\n\n");
    printf("Examples:\n");
    printf("  %s ioFTPD::MessageWindow\n", argv0);
}
