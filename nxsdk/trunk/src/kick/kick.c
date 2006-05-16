/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Kick

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Example application, demonstrates how to kick online users.

*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <nxsdk.h>

static void
Usage(
    const char *argv0
    );


int
main(
    int argc,
    char **argv
    )
{
    IO_SESSION session;
    int id;

    if (argc != 4) {
        Usage(argv[0]);
        return 1;
    }

    // Locate ioFTPD's message window.
    if (!Io_ShmInit(argv[1], &session)) {
        printf("The message window \"%s\" does not exist.\n", argv[1]);
        return 1;
    }
    id = atoi(argv[3]);

    // Kick the connection ID or user ID.
    if (strcmp(argv[2], "cid") == 0) {
        Io_KickConnId(&session, id);
        printf("Kicked connection ID %d.\n", id);

    } else if (strcmp(argv[2], "uid") == 0) {
        Io_KickUserId(&session, id);
        printf("Kicked user ID %d.\n", id);

    } else {
        Usage(argv[0]);
        return 1;
    }

    return 0;
}

static void
Usage(
    const char *argv0
    )
{
    printf("\n");
    printf("Usage: %s <window> <type> <id>\n\n", argv0);
    printf("Arguments:\n");
    printf("  window - ioFTPD's message window.\n");
    printf("  type   - Type of ID to kick: cid or uid.\n");
    printf("  id     - Connection ID or user ID to be kicked.\n\n");
    printf("Examples:\n");
    printf("  %s ioFTPD::MessageWindow cid 2\n", argv0);
    printf("  %s ioFTPD::MessageWindow uid 100\n", argv0);
}
