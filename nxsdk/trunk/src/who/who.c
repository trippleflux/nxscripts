/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Who

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Example application, demonstrates how to display online users.

*/

#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <nxsdk.h>

typedef struct {
    double speedDn;
    double speedUp;
    int usersDn;
    int usersUp;
    int usersIdle;
} WHO_TOTAL;

static Io_OnlineDataExProc DisplayUser;


int main(int argc, char **argv)
{
    char message[32];
    IO_MEMORY *memory;
    IO_SESSION session;
    WHO_TOTAL whoTotal;

    if (argc != 2) {
        printf("Usage: %s <message window>\n", argv[0]);
        return -1;
    }
    ZeroMemory(&whoTotal, sizeof(WHO_TOTAL));

    // Locate ioFTPD's message window.
    if (!Io_ShmInit(argv[1], &session)) {
        printf("The message window \"%s\" does not exist.\n", argv[1]);
        return -1;
    }

    // Allocate memory for online data.
    memory = Io_ShmAlloc(&session, sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2);
    if (memory == NULL) {
        printf("Unable to allocate shared memory.\n");
        return -1;
    }

    printf(".------------------------------------------------------------.\n");
    printf("| CID | User Name  | Group Name |  Status  | IP Address      |\n");
    printf("|------------------------------------------------------------|\n");

    // Retrieve online data.
    Io_GetOnlineDataEx(memory, DisplayUser, &whoTotal);

    printf("|------------------------------------------------------------|\n");

    // Display download totals.
    StringCchPrintfA(message, sizeof(message), "%d@%.0fKB/s",
        whoTotal.usersDn, whoTotal.speedDn);
    printf("| Dn: %-13s ", message);

    // Display upload totals.
    StringCchPrintfA(message, sizeof(message), "%d@%.0fKB/s",
        whoTotal.usersUp, whoTotal.speedUp);
    printf("| Up: %-13s ", message);

    // Display download, upload, and idle totals.
    StringCchPrintfA(message, sizeof(message), "%d@%.0fKB/s",
        whoTotal.usersDn + whoTotal.usersUp + whoTotal.usersIdle,
        whoTotal.speedDn + whoTotal.speedUp);
    printf("| All: %-13s |\n", message);

    printf("`------------------------------------------------------------'\n");

    // Clean up.
    Io_ShmFree(memory);

    return 0;
}

static BOOL
STDCALL
DisplayUser(
    IO_ONLINEDATAEX *info,
    void *opaque
    )
{
    BYTE *ipData;
    char *status;
    char clientIp[16];
    double speed;
    WHO_TOTAL *whoTotal = (WHO_TOTAL *)opaque;

    // Format the client IP.
    ipData = (BYTE *)&info->onlineData.ulClientIp;
    StringCchPrintfA(clientIp, sizeof(clientIp), "%d.%d.%d.%d",
        ipData[0] & 0xFF, ipData[1] & 0xFF, ipData[2] & 0xFF, ipData[3] & 0xFF);

    // Calculate the speed of the user in kilobytes/second.
    if (info->onlineData.dwIntervalLength > 0) {
        speed = (double)info->onlineData.dwBytesTransfered / (double)info->onlineData.dwIntervalLength;
    } else {
        speed = 0.0;
    }

    // Update bandwidth and user totals.
    switch (info->onlineData.bTransferStatus) {
        case 0:
            status = "Idle";
            whoTotal->usersIdle++;
            break;
        case 1:
            status = "Download";
            whoTotal->usersDn++;
            whoTotal->speedDn =+ speed;
            break;
        case 2:
            status = "Upload";
            whoTotal->usersUp++;
            whoTotal->speedUp =+ speed;
            break;
        case 3:
            status = "List";
            whoTotal->usersIdle++;
            break;
        default:
            status = "Unknown";
            whoTotal->usersIdle++;
            break;
    }

    printf("| %3d | %-10s | %-10s | %-8s | %-15s |\n",
        info->connId, info->userName, info->groupName, status, clientIp);

    return IO_ONLINEDATA_CONTINUE;
}
