/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Main Header

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Main development kit header file.

*/

#ifndef _NXSDK_H_
#define _NXSDK_H_

// ioFTPD headers
#include <ServerLimits.h>
#include <UserFile.h>
#include <GroupFile.h>
#include <WinMessages.h>
#include <DataCopy.h>

// Calling convetion for exported functions
#ifdef _MSC_VER
#   define STDCALL __stdcall
#else
#   define STDCALL
#endif


/*++

IO_SESSION

    Shared memory session.

Members:
    window          - Handle to ioFTPD's message window.

    currentProcId   - Current process ID.

    remoteProcId    - ioFTPD's process ID.

--*/
typedef struct {
    HWND  window;
    DWORD currentProcId;
    DWORD remoteProcId;
} IO_SESSION;

/*++

IO_MEMORY

    Shared memory block.

Members:
    block   - Block of mapped memory, allocated in the current process.

    message - Data-copy message structure.

    remote  - Remote handle, used by ioFTPD.

    event   - Handle to the request completion event.

    mapping - Handle to the mapped memory.

    window  - Handle to ioFTPD's message window.

    procId  - Current process ID.

    size    - Size of the mapped memory block, in bytes.

--*/
typedef struct {
    void       *block;
    DC_MESSAGE *message;
    void       *remote;
    HANDLE     event;
    HANDLE     mapping;
    HWND       window;
    DWORD      procId;
    DWORD      size;
} IO_MEMORY;

/*++

IO_ONLINEDATAEX

    Extended online data.

Members:
    connId      - Specifies the connection ID.

    userName    - A null-terminated string that specifies the user's name.

    groupName   - A null-terminated string that specifies the user's primary
                  group name.

    userFile    - A USERFILE structure for the user.

    groupFile   - A GROUPFILE structure for the user's primary group.

    onlineData  - An ONLINEDATA structure for the user's connection.

--*/
typedef struct {
    int        connId;
    char       userName[_MAX_NAME+1];
    char       groupName[_MAX_NAME+1];
    USERFILE   userFile;
    GROUPFILE  groupFile;
    ONLINEDATA onlineData;
} IO_ONLINEDATAEX;

/*++

IO_VFS

    Virtual file system ownership and permission.

Members:
    userId      - User identifier.

    groupId     - Group identifier.

    fileMode    - Octal value of chmod-style permissions.

--*/
typedef struct {
    UINT32 userId;
    UINT32 groupId;
    DWORD  fileMode;
} IO_VFS;


//
// Callback functions
//

#define IO_ONLINEDATA_STOP     0
#define IO_ONLINEDATA_CONTINUE 1

/*++

Io_OnlineDataProc

    Callback function used by Io_GetOnlineData.

Arguments:
    connId      - Specifies the connection ID.

    onlineData  - Pointer to the connection's ONLINEDATA structure.

    opaque      - Value passed to Io_GetOnlineData's opaque argument.

Return Values:
    IO_ONLINEDATA_CONTINUE - Continues to the next online user.

    IO_ONLINEDATA_STOP     - Stops the operation and returns.

--*/
typedef BOOL (STDCALL Io_OnlineDataProc)(
    int connId,
    ONLINEDATA *onlineData,
    void *opaque
    );

/*++

Io_OnlineDataExProc

    Callback function used by Io_GetOnlineDataEx.

Arguments:
    onlineDataEx - Pointer to the connection's IO_ONLINEDATAEX structure.

    opaque       - Value passed to Io_GetOnlineDataEx's opaque argument.

Return Values:
    IO_ONLINEDATA_CONTINUE - Continues to the next online user.

    IO_ONLINEDATA_STOP     - Stops the operation and returns.

--*/
typedef BOOL (STDCALL Io_OnlineDataExProc)(
    IO_ONLINEDATAEX *onlineDataEx,
    void *opaque
    );


//
// Shared memory functions
//

BOOL
STDCALL
Io_ShmInit(
    const char *windowName,
    IO_SESSION *session
    );

IO_MEMORY *
STDCALL
Io_ShmAlloc(
    const IO_SESSION *session,
    DWORD size
    );

void
STDCALL
Io_ShmFree(
    IO_MEMORY *memory
    );

DWORD
STDCALL
Io_ShmQuery(
    IO_MEMORY *memory,
    DWORD queryId,
    DWORD timeOut
    );


//
// Utility functions
//

BOOL
STDCALL
Io_GetBinaryPath(
    const IO_SESSION *session,
    char *path,
    DWORD pathLength
    );

BOOL
STDCALL
Io_GetStartTime(
    const IO_SESSION *session,
    FILETIME *startTime
    );


//
// User functions
//

BOOL
STDCALL
Io_UserCreate(
    IO_MEMORY *memory,
    const char *userName,
    int *userId
    );

BOOL
STDCALL
Io_UserRename(
    IO_MEMORY *memory,
    const char *userName,
    const char *newName
    );

BOOL
STDCALL
Io_UserDelete(
    IO_MEMORY *memory,
    const char *userName
    );

BOOL
STDCALL
Io_UserGetFile(
    IO_MEMORY *memory,
    int userId,
    USERFILE *userFile
    );

BOOL
STDCALL
Io_UserSetFile(
    IO_MEMORY *memory,
    const USERFILE *userFile
    );

BOOL
STDCALL
Io_UserIdToName(
    IO_MEMORY *memory,
    int userId,
    char *userName
    );

BOOL
STDCALL
Io_UserNameToId(
    IO_MEMORY *memory,
    const char *userName,
    int *userId
    );


//
// Group functions
//

BOOL
STDCALL
Io_GroupCreate(
    IO_MEMORY *memory,
    const char *groupName,
    int *groupId
    );

BOOL
STDCALL
Io_GroupRename(
    IO_MEMORY *memory,
    const char *groupName,
    const char *newName
    );

BOOL
STDCALL
Io_GroupDelete(
    IO_MEMORY *memory,
    const char *groupName
    );

BOOL
STDCALL
Io_GroupGetFile(
    IO_MEMORY *memory,
    int groupId,
    GROUPFILE *groupFile
    );

BOOL
STDCALL
Io_GroupSetFile(
    IO_MEMORY *memory,
    const GROUPFILE *groupFile
    );

BOOL
STDCALL
Io_GroupIdToName(
    IO_MEMORY *memory,
    int groupId,
    char *groupName
    );

BOOL
STDCALL
Io_GroupNameToId(
    IO_MEMORY *memory,
    const char *groupName,
    int *groupId
    );


//
// Online data functions
//

void
STDCALL
Io_GetOnlineData(
    IO_MEMORY *memory,
    Io_OnlineDataProc *callback,
    void *opaque
    );

BOOL
STDCALL
Io_GetOnlineDataEx(
    IO_MEMORY *memory,
    Io_OnlineDataExProc *callback,
    void *opaque
    );

void
STDCALL
Io_KickConnId(
    const IO_SESSION *session,
    int connId
    );

void
STDCALL
Io_KickUserId(
    const IO_SESSION *session,
    int userId
    );


//
// VFS functions
//

BOOL
STDCALL
Io_VfsFlush(
    IO_MEMORY *memory,
    const char *dirPath
    );

BOOL
STDCALL
Io_VfsRead(
    IO_MEMORY *memory,
    const char *path,
    IO_VFS *vfs
    );

BOOL
STDCALL
Io_VfsWrite(
    IO_MEMORY *memory,
    const char *path,
    const IO_VFS *vfs
    );

#endif // _NXSDK_H_
