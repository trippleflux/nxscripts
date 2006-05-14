/*

nxSDK - ioFTPD Software Development Kit
Copyright (c) 2006 neoxed

Module Name:
    Public Defintions

Author:
    neoxed (neoxed@gmail.com) May 13, 2006

Abstract:
    Include file for public use.

*/

#ifndef _NXSDK_H_
#define _NXSDK_H_

// ioFTPD includes
#include <ServerLimits.h>
#include <UserFile.h>
#include <GroupFile.h>
#include <WinMessages.h>
#include <DataCopy.h>


//
// Shared memory functions
//

typedef struct {
    HWND  messageWnd;       // Handle to ioFTPD's message window.
    DWORD currentProcId;    // Current process ID.
    DWORD remoteProcId;     // ioFTPD's process ID.
} IO_SESSION;

typedef struct {
    DC_MESSAGE *message;    // Data-copy message structure.
    void       *block;      // Allocated memory block.
    void       *remote;     // Remote handle, used by ioFTPD.
    HANDLE     event;       // Event handle.
    HANDLE     memMap;      // Memory mapping handle.
    DWORD      bytes;       // Size of the memory block, in bytes.
} IO_MEMORY;

BOOL
Io_ShmInit(
    const char *window,
    IO_SESSION *session
    );

IO_MEMORY *
Io_ShmAlloc(
    IO_SESSION *session,
    DWORD bytes
    );

void
Io_ShmFree(
    IO_SESSION *session,
    IO_MEMORY *memory
    );

DWORD
Io_ShmQuery(
    IO_SESSION *session,
    IO_MEMORY *memory,
    DWORD queryId,
    DWORD timeOut
    );


//
// Utility functions
//

BOOL
Io_GetBinaryPath(
    IO_SESSION *session,
    char *path,
    DWORD pathLength
    );

BOOL
Io_GetStartTime(
    IO_SESSION *session,
    FILETIME *startTime
    );


//
// User functions
//

BOOL
Io_UserCreate(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *userName,
    int *userId
    );

BOOL
Io_UserRename(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *userName,
    const char *newName
    );

BOOL
Io_UserDelete(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *userName
    );

BOOL
Io_UserGetFile(
    IO_SESSION *session,
    IO_MEMORY *memory,
    int userId,
    USERFILE *userFile
    );

BOOL
Io_UserSetFile(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const USERFILE *userFile
    );

BOOL
Io_UserIdToName(
    IO_SESSION *session,
    IO_MEMORY *memory,
    int userId,
    char *userName
    );

BOOL
Io_UserNameToId(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *userName,
    int *userId
    );


//
// Group functions
//

BOOL
Io_GroupCreate(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *groupName,
    int *groupId
    );

BOOL
Io_GroupRename(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *groupName,
    const char *newName
    );

BOOL
Io_GroupDelete(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *groupName
    );

BOOL
Io_GroupGetFile(
    IO_SESSION *session,
    IO_MEMORY *memory,
    int groupId,
    GROUPFILE *groupFile
    );

BOOL
Io_GroupSetFile(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const GROUPFILE *groupFile
    );

BOOL
Io_GroupIdToName(
    IO_SESSION *session,
    IO_MEMORY *memory,
    int groupId,
    char *groupName
    );

BOOL
Io_GroupNameToId(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *groupName,
    int *groupId
    );


//
// Online data functions
//

typedef BOOL (ONLINEDATA_ROUTINE)(
    IO_SESSION *session,
    int connId,
    ONLINEDATA *onlineData,
    void *opaque
    );

void
Io_GetOnlineData(
    IO_SESSION *session,
    IO_MEMORY *memory,
    ONLINEDATA_ROUTINE *routine,
    void *opaque
    );

void
Io_KickConnId(
    IO_SESSION *session,
    int connId
    );

void
Io_KickUserId(
    IO_SESSION *session,
    int userId
    );


//
// VFS functions
//

typedef struct {
    UINT32 userId;
    UINT32 groupId;
    DWORD  fileMode;
} IO_VFS;

BOOL
Io_VfsFlush(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *dirPath
    );

BOOL
Io_VfsRead(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *path,
    IO_VFS *vfs
    );

BOOL
Io_VfsWrite(
    IO_SESSION *session,
    IO_MEMORY *memory,
    const char *path,
    const IO_VFS *vfs
    );

#endif // _NXSDK_H_
