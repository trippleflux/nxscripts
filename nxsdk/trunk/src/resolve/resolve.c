/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Resolve

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Example application, demonstrates how to resolve names and IDs.

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

typedef BOOL (ResolveProc)(
    IO_MEMORY *memory,
    const char *input
    );

static ResolveProc ResolveGroupId;
static ResolveProc ResolveGroupName;
static ResolveProc ResolveUserId;
static ResolveProc ResolveUserName;

struct {
    char *name;
    ResolveProc *proc;
} static const types[] = {
    {"gid",   ResolveGroupId},
    {"group", ResolveGroupName},
    {"uid",   ResolveUserId},
    {"user",  ResolveUserName}
};


int
main(
    int argc,
    char **argv
    )
{
    BOOL result = FALSE;
    int i;
    IO_MEMORY *memory;
    IO_SESSION session;

    if (argc != 4) {
        Usage(argv[0]);
        return 1;
    }

    // Locate ioFTPD's message window.
    if (!Io_ShmInit(argv[1], &session)) {
        printf("The message window \"%s\" does not exist.\n", argv[1]);
        return 1;
    }

    // Allocate memory for user/group resolving.
    memory = Io_ShmAlloc(&session, sizeof(DC_NAMEID));
    if (memory == NULL) {
        printf("Unable to allocate shared memory (error %lu).\n", GetLastError());
        return 1;
    }

    // Call the resolve function.
    for (i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        if (strcmp(types[i].name, argv[2]) == 0) {
            result = types[i].proc(memory, argv[3]);
            break;
        }
    }

    if (i >= sizeof(types)/sizeof(types[0])) {
        Usage(argv[0]);
    }

    // Clean up.
    Io_ShmFree(memory);

    return (result == TRUE) ? 0 : 1;
}

static BOOL
ResolveGroupId(
    IO_MEMORY *memory,
    const char *idText
    )
{
    int id;
    char name[_MAX_NAME+1];

    id = atoi(idText);
    if (!Io_GroupIdToName(memory, id, name)) {
        printf("The group ID \"%d\" does not exist.\n", id);
        return FALSE;
    }

    printf("Resolved group ID \"%d\" to group name \"%s\".\n", id, name);
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

    printf("Resolved group name \"%s\" to group ID \"%d\".\n", name, id);
    return TRUE;
}

static BOOL
ResolveUserId(
    IO_MEMORY *memory,
    const char *idText
    )
{
    int id;
    char name[_MAX_NAME+1];

    id = atoi(idText);
    if (!Io_UserIdToName(memory, id, name)) {
        printf("The user ID \"%d\" does not exist.\n", id);
        return FALSE;
    }

    printf("Resolved user ID \"%d\" to user name \"%s\".\n", id, name);
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

    printf("Resolved user name \"%s\" to user ID \"%d\".\n", name, id);
    return TRUE;
}

static void
Usage(
    const char *argv0
    )
{
    printf("\n");
    printf("Usage: %s <window> <type> <input>\n\n", argv0);
    printf("Arguments:\n");
    printf("  window - ioFTPD's message window.\n");
    printf("  type   - Type of input data: gid, group, uid, or user.\n");
    printf("  input  - The ID or name of a group or user.\n\n");
    printf("Examples:\n");
    printf("  %s ioFTPD::MessageWindow gid   1\n", argv0);
    printf("  %s ioFTPD::MessageWindow group STAFF\n", argv0);
    printf("  %s ioFTPD::MessageWindow uid   0\n", argv0);
    printf("  %s ioFTPD::MessageWindow user  bill\n", argv0);
}
