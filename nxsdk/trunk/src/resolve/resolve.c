/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Resolve

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Example tool to resolve group IDs, group names, user IDs, and user names.

*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <nxsdk.h>

typedef BOOL (RESOLVE_ROUTINE)(
    IO_MEMORY *memory,
    const char *input
    );

static RESOLVE_ROUTINE ResolveGroupId;
static RESOLVE_ROUTINE ResolveGroupName;
static RESOLVE_ROUTINE ResolveUserId;
static RESOLVE_ROUTINE ResolveUserName;

struct {
    char *name;
    RESOLVE_ROUTINE *routine;
} static const types[] = {
    {"gid",   ResolveGroupId},
    {"group", ResolveGroupName},
    {"uid",   ResolveUserId},
    {"user",  ResolveUserName}
};


int main(int argc, char **argv)
{
    BOOL result = FALSE;
    int i;
    IO_MEMORY *memory;
    IO_SESSION session;

    if (argc != 4) {
        printf("Usage: %s <message window> gid   <group id>\n",   argv[0]);
        printf("       %s <message window> group <group name>\n", argv[0]);
        printf("       %s <message window> uid   <user id>\n",    argv[0]);
        printf("       %s <message window> user  <user name>\n",  argv[0]);
        return -1;
    }

    // Locate ioFTPD's message window.
    if (!Io_ShmInit(argv[1], &session)) {
        printf("The message window \"%s\" does not exist.\n", argv[1]);
        return -1;
    }

    // Allocate memory for user ID to name resolving.
    memory = Io_ShmAlloc(&session, sizeof(DC_NAMEID));
    if (memory == NULL) {
        printf("Unable to allocate shared memory.\n");
        return -1;
    }

    for (i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        if (strcmp(types[i].name, argv[2]) == 0) {
            // Resolve the ID/name.
            result = types[i].routine(memory, argv[3]);
            break;
        }
    }

    if (i >= sizeof(types)/sizeof(types[0])) {
        printf("Invalid argument \"%s\": must be gid, group, uid, or user.\n", argv[2]);
    }

    Io_ShmFree(memory);
    return (result == TRUE) ? 0 : -1;
}

static BOOL
ResolveGroupId(
    IO_MEMORY *memory,
    const char *groupId
    )
{
    int id;
    char name[_MAX_NAME+1];

    id = atoi(groupId);
    if (!Io_GroupIdToName(memory, id, name)) {
        printf("The group ID \"%d\" does not exist.\n", id);
        return FALSE;
    }

    printf("Resolved the group ID \"%d\" to the group name \"%s\".\n", id, name);
    return TRUE;
}

static BOOL
ResolveGroupName(
    IO_MEMORY *memory,
    const char *name
    )
{
    int id;

    if (!Io_GroupNameToId(memory, name, &id)) {
        printf("The group name \"%s\" does not exist.\n", name);
        return FALSE;
    }

    printf("Resolved the group name \"%s\" to the group ID \"%d\".\n", name, id);
    return TRUE;
}

static BOOL
ResolveUserId(
    IO_MEMORY *memory,
    const char *userId
    )
{
    int id;
    char name[_MAX_NAME+1];

    id = atoi(userId);
    if (!Io_UserIdToName(memory, id, name)) {
        printf("The user ID \"%d\" does not exist.\n", id);
        return FALSE;
    }

    printf("Resolved the user ID \"%d\" to the user name \"%s\".\n", id, name);
    return TRUE;
}

static BOOL
ResolveUserName(
    IO_MEMORY *memory,
    const char *name
    )
{
    int id;

    if (!Io_UserNameToId(memory, name, &id)) {
        printf("The user name \"%s\" does not exist.\n", name);
        return FALSE;
    }

    printf("Resolved the user name \"%s\" to the user ID \"%d\".\n", name, id);
    return TRUE;
}
