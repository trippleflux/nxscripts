/*++

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Who

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    An example tool to display online users.

--*/

#include <windows.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <nxsdk.h>

typedef struct {
    IO_MEMORY *memUser;
    double speedDn;
    double speedUp;
    INT usersDn;
    INT usersUp;
    INT usersIdle;
} WHO_DATA;

static ONLINEDATA_ROUTINE DisplayUser;


int main(int argc, char **argv)
{
    char message[64];
    IO_MEMORY *memOnline;
    IO_MEMORY *memUser;
    IO_SESSION session;
    WHO_DATA whoData;

    if (argc != 2) {
        printf("Usage: %s <message window>\n", argv[0]);
        return -1;
    }

    // Locate ioFTPD's message window.
    if (!Io_ShmInit(argv[1], &session)) {
        printf("The message window \"%s\" does not exist.\n", argv[1]);
        return -1;
    }

    // Allocate memory for online data.
    memOnline = Io_ShmAlloc(&session, sizeof(DC_ONLINEDATA) + (MAX_PATH+1)*2);
    if (memOnline == NULL) {
        printf("Unable to allocate shared memory.\n");
        return -1;
    }

    // Allocate memory for user ID to name resolving.
    memUser = Io_ShmAlloc(&session, sizeof(DC_NAMEID));
    if (memUser == NULL) {
        Io_ShmFree(memOnline);
        printf("Unable to allocate shared memory.\n");
        return -1;
    }

    printf(".----------------------------------------------------------.\n");
    printf("| CID |  Username  |  Status  | IP Address                 |\n");
    printf("|----------------------------------------------------------|\n");

    // Retrieve online data.
    ZeroMemory(&whoData, sizeof(WHO_DATA));
    whoData.memUser = memUser;
    Io_GetOnlineData(memOnline, DisplayUser, &whoData);

    printf("|----------------------------------------------------------|\n");

    StringCchPrintfA(message, sizeof(message), "%d user(s) at %.1f KB/s", whoData.usersDn, whoData.speedDn);
    printf("| Down: %-50s |\n", message);

    StringCchPrintfA(message, sizeof(message), "%d user(s) at %.1f KB/s", whoData.usersUp, whoData.speedUp);
    printf("| Up  : %-50s |\n", message);

    StringCchPrintfA(message, sizeof(message), "%d user(s)", whoData.usersIdle);
    printf("| Idle: %-50s |\n", message);

    printf("`----------------------------------------------------------'\n");

    Io_ShmFree(memOnline);
    Io_ShmFree(memUser);
    return 0;
}

static BOOL
STDCALL
DisplayUser(
    int connId,
    ONLINEDATA *onlineData,
    void *opaque
    )
{
    BYTE *ipData;
    char *status;
    char clientIp[16];
    char userName[_MAX_NAME+1];
    double speed = 0.0;
    WHO_DATA *whoData = (WHO_DATA *)opaque;

    // Resolve the user ID to its name.
    if (!Io_UserIdToName(whoData->memUser, onlineData->Uid, userName)) {
        StringCchCopyA(userName, sizeof(userName), "NoUser");
    }

    // Format the client IP.
    ipData = (BYTE *)&onlineData->ulClientIp;
    StringCchPrintfA(clientIp, sizeof(clientIp), "%d.%d.%d.%d",
        ipData[0] & 0xFF, ipData[1] & 0xFF, ipData[2] & 0xFF, ipData[3] & 0xFF);

    // Update totals and map the transfer status to a textual description.
    if (onlineData->dwIntervalLength > 0) {
        speed = (double)onlineData->dwBytesTransfered / (double)onlineData->dwIntervalLength;
    }

    switch (onlineData->bTransferStatus) {
        case 0:
            status = "Idle";
            whoData->usersIdle++;
            break;
        case 1:
            status = "Download";
            whoData->usersDn++;
            whoData->speedDn =+ speed;
            break;
        case 2:
            status = "Upload";
            whoData->usersUp++;
            whoData->speedUp =+ speed;
            break;
        case 3:
            status = "List";
            whoData->usersIdle++;
            break;
        default:
            status = "Unknown";
            whoData->usersIdle++;
            break;
    }

    printf("| %3d | %-10s | %8s | %-26s |\n", connId, userName, status, clientIp);
    return ONLINEDATA_CONTINUE;
}
