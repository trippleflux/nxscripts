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

int main(int argc, char **argv)
{
    IO_SESSION session;
    int id;

    if (argc != 4) {
        printf("Usage: %s <message window> cid <connection id>\n", argv[0]);
        printf("       %s <message window> uid <user id>\n", argv[0]);
        return -1;
    }

    // Locate ioFTPD's message window.
    if (!Io_ShmInit(argv[1], &session)) {
        printf("The message window \"%s\" does not exist.\n", argv[1]);
        return -1;
    }
    id = atoi(argv[3]);

    // Kick the connection/user ID.
    if (strcmp(argv[2], "cid") == 0) {
        Io_KickConnId(&session, id);
        printf("Kicked connection ID %d.\n", id);

    } else if (strcmp(argv[2], "uid") == 0) {
        Io_KickUserId(&session, id);
        printf("Kicked user ID %d.\n", id);

    } else {
        printf("Invalid argument \"%s\": must be cid or uid.\n", argv[2]);
        return -1;
    }

    return 0;
}
